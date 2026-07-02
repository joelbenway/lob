// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include <benchmark/benchmark.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "tables.hpp"

namespace benchmarks {
// BranchlessLobLerp: binary search with conditional moves instead of if/else.
// Pros: eliminates branch mispredictions on the data-dependent compare.
// Cons: still does 7 iterations of uint16->double conversion per lookup.
//       1.07x faster than BinaryLobLerp on this workload — modest.
namespace {
template <typename T>
double BranchlessLobLerp(const T* x_lut, const T* y_lut, const size_t size,
                         const double x_in) {
  if (x_in >= static_cast<double>(x_lut[size - 1])) {
    return static_cast<double>(y_lut[size - 1]);
  }

  size_t low = 0;
  size_t high = size - 1;
  size_t index = 0;

  constexpr int kSearchDepth = 7;  // ceil(log2(85))
  for (int i = 0; i < kSearchDepth; ++i) {
    if (low <= high) {
      const size_t kMid = low + ((high - low) / 2);
      const bool kGe = !(x_in < static_cast<double>(x_lut[kMid]));
      low = kGe ? kMid + 1 : low;
      high = kGe ? high : kMid - 1;
      index = kGe ? kMid : index;
    }
  }

  const auto kX0 = static_cast<double>(x_lut[index]);
  const auto kX1 = static_cast<double>(x_lut[index + 1]);
  const auto kY0 = static_cast<double>(y_lut[index]);
  const auto kY1 = static_cast<double>(y_lut[index + 1]);
  const auto kDx = kX1 - kX0;
  const double kT = (x_in - kX0) / kDx;
  return kY0 + (kT * (kY1 - kY0));
}
}  // namespace

// MonotonicLobLerp: forward linear scan from index 0.
// Included as a cautionary reference — linear scan does ~32 comparisons
// per lookup on this table (each requiring uint16->double conversion),
// which is slower than binary search's ~7 despite perfect branch prediction.
// With a cached index passed across calls this could be O(1) amortized for
// the solver's descending-Mach access pattern, but thread-local storage
// overhead negated the benefit in this benchmark.
namespace {
template <typename T>
double MonotonicLobLerp(const T* x_lut, const T* y_lut, const size_t size,
                        const double x_in) {
  if (x_in >= static_cast<double>(x_lut[size - 1])) {
    return static_cast<double>(y_lut[size - 1]);
  }

  size_t index = 0;
  while (index < size - 1 && x_in >= static_cast<double>(x_lut[index + 1])) {
    ++index;
  }

  const auto kX0 = static_cast<double>(x_lut[index]);
  const auto kX1 = static_cast<double>(x_lut[index + 1]);
  const auto kY0 = static_cast<double>(y_lut[index]);
  const auto kY1 = static_cast<double>(y_lut[index + 1]);
  const auto kDx = kX1 - kX0;
  const double kT = (x_in - kX0) / kDx;
  return kY0 + (kT * (kY1 - kY0));
}
}  // namespace

// BinaryLobLerp: standard binary search + linear interpolation.
// Baseline reference. 7 iterations for 85 entries. Branch mispredictions
// on the data-dependent compare are the main cost.
namespace {
template <typename T>
double BinaryLobLerp(const T* x_lut, const T* y_lut, const size_t size,
                     const double x_in) {
  if (x_in >= static_cast<double>(x_lut[size - 1])) {
    return static_cast<double>(y_lut[size - 1]);
  }

  size_t low = 0;
  size_t high = size - 1;
  size_t index = 0;

  while (low <= high) {
    const size_t kMid = low + ((high - low) / 2);
    if (static_cast<double>(x_lut[kMid]) <= x_in) {
      index = kMid;
      low = kMid + 1;
    } else {
      high = kMid - 1;
    }
  }

  const auto kX0 = static_cast<double>(x_lut[index]);
  const auto kX1 = static_cast<double>(x_lut[index + 1]);
  const auto kY0 = static_cast<double>(y_lut[index]);
  const auto kY1 = static_cast<double>(y_lut[index + 1]);
  const auto kDx = kX1 - kX0;
  const double kT = (x_in - kX0) / kDx;
  return kY0 + (kT * (kY1 - kY0));
}
}  // namespace

namespace {
constexpr size_t kUniformSize = 50'001;

const std::vector<uint16_t>& GetUniformG1() {
  static const std::vector<uint16_t> kSGrid = []() {
    std::vector<uint16_t> grid(kUniformSize);
    for (size_t i = 0; i < kUniformSize; ++i) {
      const double kVal =
          BinaryLobLerp(lob::kMachs.data(), lob::kG1Drags.data(),
                        lob::kTableSize, static_cast<double>(i));
      grid[i] = static_cast<uint16_t>(std::lround(kVal));
    }
    return grid;
  }();
  return kSGrid;
}

// UniformGridLobLerp: precompute G1 drag at every integer Mach*10000
// (50001 entries), then O(1) lookup via index + linear interpolation.
// Pros: no search, no branches, no uint16->double conversions in hot path.
//       3.3x faster than BinaryLobLerp on this workload.
// Cons: ~100KB .rodata per drag function; only works for
//       compile-time-known tables (G1-G8); precomputation cost ~1-2ms.
double UniformGridLobLerp(const double x_in) {
  const auto& grid = GetUniformG1();
  const auto kIdx = static_cast<size_t>(x_in);
  const double kFrac = x_in - static_cast<double>(kIdx);
  return static_cast<double>(grid.at(kIdx)) +
         (kFrac * static_cast<double>(grid.at(kIdx + 1) - grid.at(kIdx)));
}
}  // namespace

// SplineFitLobLerp: C1-continuous piecewise cubic fit to G1 with 9 pieces.
// Knots chosen to capture the transonic hump, peak, and long descent.
// Derivatives at knots are optimized via linear least-squares to minimize
// error across all 85 table points (max 0.46% for G1).
// Runtime: branchless piece selection (bool-sum → cmov) + Horner evaluation.
namespace {
constexpr int kNumKnots = 10;
constexpr int kNumPieces = 9;
constexpr std::array<size_t, kNumKnots> kSplineKnots = {0,  8,  16, 24, 28,
                                                        31, 34, 37, 43, 84};

struct CubicCoeffs {
  double a, b, c, d;
};  // a + b*t + c*t^2 + d*t^3
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
std::array<double, kNumKnots> g_spline_x0{};
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
std::array<CubicCoeffs, kNumPieces> g_spline{};

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void InitSplineG1() noexcept {
  std::array<double, kNumKnots> xk{};
  std::array<double, kNumKnots> yk{};
  for (size_t i = 0; i < static_cast<size_t>(kNumKnots); ++i) {
    xk.at(i) = static_cast<double>(lob::kMachs.at(kSplineKnots.at(i)));
    yk.at(i) = static_cast<double>(lob::kG1Drags.at(kSplineKnots.at(i)));
    g_spline_x0.at(i) = xk.at(i);
  }

  std::array<double, kNumPieces> h{};
  for (size_t i = 0; i < static_cast<size_t>(kNumPieces); ++i) {
    h.at(i) = xk.at(i + 1) - xk.at(i);
  }

  auto a = std::array<std::array<double, kNumKnots>, kNumKnots>{};
  auto bb = std::array<double, kNumKnots>{};

  for (size_t pi = 0; pi < lob::kTableSize; ++pi) {
    const auto kXi = static_cast<double>(lob::kMachs.at(pi));
    const auto kYi = static_cast<double>(lob::kG1Drags.at(pi));
    if (kXi < xk.at(0) || kXi >= xk.at(static_cast<size_t>(kNumPieces))) {
      continue;
    }
    size_t j = 0;
    while (j < static_cast<size_t>(kNumPieces) && kXi >= xk.at(j + 1)) {
      ++j;
    }
    if (j >= static_cast<size_t>(kNumPieces)) {
      continue;
    }

    const double kT = (kXi - xk.at(j)) / h.at(j);
    const double kT2 = kT * kT;
    const double kT3 = kT2 * kT;
    const double kH00 = (2.0 * kT3) - (3.0 * kT2) + 1.0;
    const double kH01 = (-2.0 * kT3) + (3.0 * kT2);
    const double kH10 = (kT3 - (2.0 * kT2) + kT) * h.at(j);
    const double kH11 = (kT3 - kT2) * h.at(j);
    const double kBase = (yk.at(j) * kH00) + (yk.at(j + 1) * kH01);
    const double kResid = kYi - kBase;

    a.at(j).at(j) += kH10 * kH10;
    a.at(j).at(j + 1) += kH10 * kH11;
    a.at(j + 1).at(j) = a.at(j).at(j + 1);
    a.at(j + 1).at(j + 1) += kH11 * kH11;
    bb.at(j) += kResid * kH10;
    bb.at(j + 1) += kResid * kH11;
  }

  constexpr double kLambda = 1e-7;
  for (size_t i = 0; i < static_cast<size_t>(kNumKnots); ++i) {
    a.at(i).at(i) += kLambda;
  }

  auto l = std::array<std::array<double, kNumKnots>, kNumKnots>{};
  for (size_t i = 0; i < static_cast<size_t>(kNumKnots); ++i) {
    for (size_t j = 0; j <= i; ++j) {
      double sum = a.at(i).at(j);
      for (size_t k = 0; k < j; ++k) {
        sum -= l.at(i).at(k) * l.at(j).at(k);
      }
      l.at(i).at(j) = (i == j) ? std::sqrt(sum) : sum / l.at(j).at(j);
    }
  }

  auto y = std::array<double, kNumKnots>{};
  for (size_t i = 0; i < static_cast<size_t>(kNumKnots); ++i) {
    double sum = bb.at(i);
    for (size_t j = 0; j < i; ++j) {
      sum -= l.at(i).at(j) * y.at(j);
    }
    y.at(i) = sum / l.at(i).at(i);
  }

  auto d = std::array<double, kNumKnots>{};
  for (size_t i = static_cast<size_t>(kNumPieces) + 1; i > 0; --i) {
    const size_t kIdx = i - 1;
    double sum = y.at(kIdx);
    for (size_t j = kIdx + 1; j < static_cast<size_t>(kNumKnots); ++j) {
      sum -= l.at(j).at(kIdx) * d.at(j);
    }
    d.at(kIdx) = sum / l.at(kIdx).at(kIdx);
  }

  for (size_t i = 0; i < static_cast<size_t>(kNumPieces); ++i) {
    const double kDy = yk.at(i + 1) - yk.at(i);
    const double kHi = h.at(i);
    const double kHi2 = kHi * kHi;
    const double kHi3 = kHi2 * kHi;
    const double kCoeff1 = 3.0;
    const double kCoeff2 = 2.0;
    g_spline.at(i).a = yk.at(i);
    g_spline.at(i).b = d.at(i);
    g_spline.at(i).c =
        ((kCoeff1* kDy) - (kCoeff2 * d.at(i) * kHi) - (d.at(i + 1) * kHi)) / kHi2;
    g_spline.at(i).d =
        ((-kCoeff2 * kDy) + (d.at(i) * kHi) + (d.at(i + 1) * kHi)) / kHi3;
  }
}

const bool kSplineReady = []() noexcept {
  InitSplineG1();
  return true;
}();

// SplineFitLobLerp: 9-piece C1 cubic fit.
// Pros: fully branchless lookup (cmov chain + Horner), tiny binary
//       (9×4 doubles = 288 bytes per drag function), C1 continuous,
//       no precomputation at solver time.
// Cons: fitting accuracy depends on knot placement.
double SplineFitLobLerp(const double x_in) {
  if (x_in <= g_spline_x0[0]) {
    return static_cast<double>(lob::kG1Drags.front());
  }
  if (x_in >= g_spline_x0[kNumPieces]) {
    return static_cast<double>(lob::kG1Drags.back());
  }

  size_t idx = 0;
  for (size_t i = 1; i < kNumPieces; i++) {
    idx += (x_in >= g_spline_x0.at(i)) ? size_t{1} : size_t{0};
  }

  const auto& p = g_spline.at(idx);
  const double kT = x_in - g_spline_x0.at(idx);
  return p.a + (kT * (p.b + (kT * (p.c + (kT * p.d)))));
}

}  // namespace

namespace {
const double kInitMachSpeed = 2.5 * lob::kTableScale;
const double kFinalMachSpeed = 0.25 * lob::kTableScale;
const double kDecrement = 1E-4 * lob::kTableScale;
const auto kResultsSize = static_cast<size_t>(
    std::ceil((kInitMachSpeed - kFinalMachSpeed) / kDecrement));
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
std::vector<double> results(kResultsSize);  // NOLINT(cert-err58-cpp)
}  // namespace

namespace {
void BranchlessBM(benchmark::State& state) {
  size_t index = 0;
  for (auto _ : state) {
    double velocity = kInitMachSpeed;
    while (velocity > kFinalMachSpeed) {
      const double kResult = BranchlessLobLerp(
          lob::kMachs.data(), lob::kG1Drags.data(), lob::kTableSize, velocity);
      results.at(index++) = kResult;
      velocity -= kDecrement;
    }
    index = 0;
  }
}
}  // namespace

namespace {
void MonotonicBM(benchmark::State& state) {
  size_t index = 0;
  for (auto _ : state) {
    double velocity = kInitMachSpeed;
    while (velocity > kFinalMachSpeed) {
      const double kResult = MonotonicLobLerp(
          lob::kMachs.data(), lob::kG1Drags.data(), lob::kTableSize, velocity);
      results.at(index++) = kResult;
      velocity -= kDecrement;
    }
    index = 0;
  }
}
}  // namespace

namespace {
void BinaryBM(benchmark::State& state) {
  size_t index = 0;
  for (auto _ : state) {
    double velocity = kInitMachSpeed;
    while (velocity > kFinalMachSpeed) {
      const double kResult = BinaryLobLerp(
          lob::kMachs.data(), lob::kG1Drags.data(), lob::kTableSize, velocity);
      results.at(index++) = kResult;
      velocity -= kDecrement;
    }
    index = 0;
  }
}
}  // namespace

namespace {
void UniformGridBM(benchmark::State& state) {
  size_t index = 0;
  for (auto _ : state) {
    double velocity = kInitMachSpeed;
    while (velocity > kFinalMachSpeed) {
      const double kResult = UniformGridLobLerp(velocity);
      results.at(index++) = kResult;
      velocity -= kDecrement;
    }
    index = 0;
  }
}
}  // namespace

namespace {
void SplineFitBM(benchmark::State& state) {
  (void)kSplineReady;
  size_t index = 0;
  for (auto _ : state) {
    double velocity = kInitMachSpeed;
    while (velocity > kFinalMachSpeed) {
      const double kResult = SplineFitLobLerp(velocity);
      results.at(index++) = kResult;
      velocity -= kDecrement;
    }
    index = 0;
  }
}
}  // namespace

// NOLINTBEGIN
// Register the benchmark functions
BENCHMARK(BranchlessBM);
BENCHMARK(MonotonicBM);
BENCHMARK(BinaryBM);
BENCHMARK(UniformGridBM);
BENCHMARK(SplineFitBM);

}  // namespace benchmarks

// macro that expands into main function
BENCHMARK_MAIN();
// NOLINTEND

// This file is part of lob.
//
// lob is free software: you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version.
//
// lob is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
// A PARTICULAR PURPOSE. See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along with
// lob. If not, see <https://www.gnu.org/licenses/>.