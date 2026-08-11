// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <utility>

#include "eng_units.hpp"
#include "lob/lob.h"
#include "tables.hpp"

namespace lob {
namespace spline {

namespace detail {

template <typename T>
constexpr T Secant(const T* x, const T* y, size_t i) {
  return (y[i + 1] - y[i]) / (x[i + 1] - x[i]);
}

template <typename T>
constexpr T Tangent(const T* x, const T* y, size_t n, size_t i) {
  if (n == 2) {
    return Secant(x, y, 0);
  }
  if (i == 0) {
    return Secant(x, y, 0);
  }
  if (i == n - 1) {
    return Secant(x, y, n - 2);
  }

  const T kDPrev = Secant(x, y, i - 1);
  const T kDNext = Secant(x, y, i);
  if (kDPrev * kDNext <= T(0)) {
    return T(0);
  }

  const T kHPrev = x[i] - x[i - 1];
  const T kHNext = x[i + 1] - x[i];
  const T kW1 = (T(2) * kHNext) + kHPrev;
  const T kW2 = kHNext + (T(2) * kHPrev);
  return (kW1 + kW2) / (kW1 / kDPrev + kW2 / kDNext);
}

template <typename T>
constexpr void Hermite(T x0, T x1, T y0, T y1, T m0, T m1, T* out) {
  const T kH = x1 - x0;
  const T kD = (y1 - y0) / kH;
  out[0] = y0;
  out[1] = m0;
  out[2] = (T(3) * kD - T(2) * m0 - m1) / kH;
  out[3] = (m0 + m1 - T(2) * kD) / (kH * kH);
}

template <typename T>
constexpr size_t FindInterval(const T* x, size_t n, T v) {
  assert(n >= 2);
  size_t lo = 0;
  size_t hi = n - 1;
  while (hi - lo > 1) {
    const size_t kMid = lo + ((hi - lo) / 2);
    if (x[kMid] <= v) {
      lo = kMid;
    } else {
      hi = kMid;
    }
  }
  return lo;
}

template <typename T>
struct Result {
  T y;
  T dy;
};

template <typename T>
constexpr T PolyVal(const T* c, T t) {
  return c[0] + (t * (c[1] + t * (c[2] + t * c[3])));
}

template <typename T>
constexpr T PolyDeriv(const T* c, T t) {
  return c[1] + (t * (T(2) * c[2] + T(3) * c[3] * t));
}

template <typename T>
constexpr Result<T> EvalWithDeriv(const T* x, const T* y, size_t n, T v) {
  assert(n >= 2);
  const size_t kI = FindInterval(x, n, v);
  T c[4] = {};  // NOLINT
  Hermite(x[kI], x[kI + 1], y[kI], y[kI + 1], Tangent(x, y, n, kI),
          Tangent(x, y, n, kI + 1), &c[0]);
  const T kT = v - x[kI];
  return Result<T>{PolyVal(&c[0], kT), PolyDeriv(&c[0], kT)};
}

}  // namespace detail

constexpr size_t kKnotCount = 16;
constexpr size_t kSegmentCount = kKnotCount - 1;
static_assert(kSegmentCount == LOB_SPLINE_SEGMENTS,
              "kSegmentCount must match LOB_SPLINE_SEGMENTS");
constexpr size_t kCoefsSize = kSegmentCount * 4;

constexpr std::array<float, kKnotCount> kKnots = {
    0.000000000F, 0.555277646F, 0.597798944F, 0.760380208F,
    0.885442734F, 0.940470278F, 1.003001571F, 1.018009067F,
    1.075537801F, 1.213106632F, 1.373186588F, 1.688344240F,
    2.126063108F, 2.808904648F, 3.926963568F, 5.000000000F};

template <typename T = float, size_t N = kKnotCount>
class Cursor {
 public:
  constexpr Cursor(const T* pknots, const T* pcoefs)
      : knots_(pknots), coefs_(pcoefs) {}

  constexpr Cursor(const std::array<T, N>& knots,
                   const std::array<T, (N - 1) * 4>& coefs)
      // NOLINTNEXTLINE(readability-container-data-pointer)
      : knots_(&knots[0]), coefs_(&coefs[0]) {}

  constexpr Cursor(std::array<T, N>&& knots,
                   std::array<T, (N - 1) * 4>&& coefs) = delete;

  constexpr size_t GetSegment() const { return idx_; }

  constexpr T Eval(T m) {
    Clamp(m);
    Seek(m);
    return detail::PolyVal(coefs_ + (4 * idx_), m - knots_[idx_]);
  }

  constexpr double Eval(MachT m) {
    return static_cast<double>(Eval(m.Float()));
  }

  constexpr T Deriv(T m) {
    Clamp(m);
    Seek(m);
    return detail::PolyDeriv(coefs_ + (4 * idx_), m - knots_[idx_]);
  }

 private:
  constexpr void Clamp(T& m) const {
    if (m < knots_[0]) {
      m = knots_[0];
    }
    if (m > knots_[N - 1]) {
      m = knots_[N - 1];
    }
  }

  constexpr void Seek(T m) {
    while (idx_ > 0 && m < knots_[idx_]) {
      --idx_;
    }
    while (idx_ + 2 < N && m >= knots_[idx_ + 1]) {
      ++idx_;
    }
  }

  const T* knots_;
  const T* coefs_;
  size_t idx_{0};
};

using CurveView = Cursor<float, kKnotCount>;

template <typename T>
constexpr void Segment(const T* machs, const T* drags, size_t size, T knot1,
                       T knot2, T* coefs_out) {
  const detail::Result<T> kP = detail::EvalWithDeriv(machs, drags, size, knot1);
  const detail::Result<T> kQ = detail::EvalWithDeriv(machs, drags, size, knot2);
  detail::Hermite(knot1, knot2, kP.y, kQ.y, kP.dy, kQ.dy, coefs_out);
}

template <typename T>
constexpr size_t Build(const T* machs, const T* drags, size_t size,
                       const T* knots, size_t n_knots, T* coefs_out) {
  for (size_t i = 0; i + 1 < n_knots; ++i) {
    Segment(machs, drags, size, knots[i], knots[i + 1], coefs_out + (4 * i));
  }
  return n_knots - 1;
}

template <size_t... Is>
constexpr std::array<float, sizeof...(Is)> ToArray(
    const float* data,
    // NOLINTNEXTLINE(readability-named-parameter, hicpp-named-parameter)
    std::index_sequence<Is...>) {
  return {data[Is]...};
}

template <size_t N>
constexpr std::array<float, kCoefsSize> MakeCoefs(
    const std::array<float, N>& drags) {
  float coefs[kCoefsSize]{};  // NOLINT
  // NOLINTNEXTLINE(readability-container-data-pointer)
  Build(&lob::dragtable::kMachs[0], &drags[0], N, &kKnots[0], kKnotCount,
        &coefs[0]);
  return ToArray(&coefs[0], std::make_index_sequence<kCoefsSize>{});
}

constexpr auto kG1Coefs = MakeCoefs(lob::dragtable::kG1Drags);
constexpr auto kG2Coefs = MakeCoefs(lob::dragtable::kG2Drags);
constexpr auto kG5Coefs = MakeCoefs(lob::dragtable::kG5Drags);
constexpr auto kG6Coefs = MakeCoefs(lob::dragtable::kG6Drags);
constexpr auto kG7Coefs = MakeCoefs(lob::dragtable::kG7Drags);
constexpr auto kG8Coefs = MakeCoefs(lob::dragtable::kG8Drags);

template <typename T = float>
constexpr void MakeFormFactorCoefs(T sectional_density, const T* input_machs,
                                   const T* input_bcs, size_t size,
                                   T* coefs_out) {
  assert(size >= 2 && size <= kKnotCount);
  // 1. Add boundary padding points at kKnots[0] (0.0) and kKnots[N-1] (5.0).
  // This enforces flat (constant) form factor extrapolation outside the
  // user's Mach range.
  constexpr size_t kPaddedSize = kKnotCount + 2;
  std::array<T, kPaddedSize> machs{};
  std::array<T, kPaddedSize> i_factors{};

  // Low Mach clamp (Mach 0.0) using the lowest-velocity BC
  machs[0] = kKnots[0];
  i_factors[0] = sectional_density / input_bcs[0];

  // User input points
  for (size_t k = 0; k < size; ++k) {
    machs[k + 1] = input_machs[k];
    i_factors[k + 1] = sectional_density / input_bcs[k];
  }

  // High Mach clamp (Mach 5.0) using the highest-velocity BC
  machs[size + 1] = kKnots[kKnotCount - 1];
  i_factors[size + 1] = sectional_density / input_bcs[size - 1];

  // 2. Project the PCHIP-interpolated form factor curve onto kKnots
  Build(machs.data(), i_factors.data(), size + 2, kKnots.data(), kKnotCount,
        coefs_out);
}

template <typename T = float, size_t N>
constexpr std::array<T, kCoefsSize> MakeFormFactorCoefs(
    T sectional_density, const std::array<T, N>& input_machs,
    const std::array<T, N>& input_bcs) {
  static_assert(N >= 2, "At least two BC bands are required");
  std::array<T, kCoefsSize> coefs{};
  MakeFormFactorCoefs(sectional_density, input_machs.data(), input_bcs.data(),
                      N, coefs.data());
  return coefs;
}

template <typename T = float, size_t N = kKnotCount>
constexpr std::array<T, (N - 1) * 4> Merge(
    Cursor<T, N>& curve_a,
    Cursor<T, N>& curve_b,
    const std::array<T, N>& knots = kKnots) {
  
  std::array<T, N> y_fused{};
  std::array<T, N> dy_fused{};

  for (size_t i = 0; i < N; i++) {
    const T kM = knots[i];
    const T kYa = curve_a.Eval(kM);
    const T kDya = curve_a.Deriv(kM);
    const T kYb = curve_b.Eval(kM);
    const T kDyb = curve_b.Deriv(kM);

    y_fused[i] = kYa * kYb;
    dy_fused[i] = (kDya * kYb) + (kYa * kDyb);
  }

  std::array<T, (N - 1) * 4> coefs_out{};
  for (size_t i = 0; i + 1 < N; i++) {
    detail::Hermite(knots[i], knots[i + 1], 
                    y_fused[i], y_fused[i + 1],
                    dy_fused[i], dy_fused[i + 1], 
                    &coefs_out[i * 4]);
  }

  return coefs_out;
}

}  // namespace spline
}  // namespace lob

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
