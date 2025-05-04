// This file is a part of lob, an exterior ballistics calculation library
// Copyright (c) 2025  Joel Benway
// Please see end of file for extended copyright information
#include <benchmark/benchmark.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iterator>
#include <vector>

#include "tables.hpp"

namespace benchmarks {
namespace {
template <typename T>
double NaiveLobLerp(const T* x_lut, const T* y_lut, const size_t size,
                    const double x_in) {
  if (x_in < static_cast<double>(x_lut[0])) {
    return static_cast<double>(y_lut[0]);
  }

  size_t index = size - 1;

  while (index > 0 && x_in < static_cast<double>(x_lut[index])) {
    index--;
  }

  if (index == size - 1) {
    return static_cast<double>(y_lut[size - 1]);
  }

  const auto kX0 = static_cast<double>(x_lut[index]);
  const auto kX1 = static_cast<double>(x_lut[index + 1]);
  const auto kY0 = static_cast<double>(y_lut[index]);
  const auto kY1 = static_cast<double>(y_lut[index + 1]);

  return ((kY1 - kY0) / (kX1 - kX0) * (x_in - kX0)) + kY0;
}
}  // namespace

namespace {
template <typename T>
double BranchlessLobLerp(const T* x_lut, const T* y_lut, const size_t size,
                         const double x_in) {
  size_t index = size - 1;

  while (index > 0 && x_in < static_cast<double>(x_lut[index])) {
    index--;
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
template <typename T>
double CachedLobLerp(const T* x_lut, const T* y_lut, const size_t size,
                     const double x_in) {
  static thread_local size_t index = size - 1;  // index is cached

  if (x_in > static_cast<double>(x_lut[index])) {
    index = size - 1;
  }

  while (index > 0 && x_in < static_cast<double>(x_lut[index])) {
    index--;
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
template <typename T>
double UpperboundLobLerp(const T* x_lut, const T* y_lut, const size_t size,
                         const double x_in) {
  if (x_in >= static_cast<double>(x_lut[size - 1])) {
    return static_cast<double>(y_lut[size - 1]);
  }

  const T* pupper = std::upper_bound(
      x_lut, x_lut + size, x_in, [](double val, const T& element) {
        return val < static_cast<double>(element);
      });

  const auto kDiff = std::distance(x_lut, pupper);
  const auto kIndex = static_cast<size_t>(kDiff - 1);

  const auto kX0 = static_cast<double>(x_lut[kIndex]);
  const auto kX1 = static_cast<double>(x_lut[kIndex + 1]);
  const auto kY0 = static_cast<double>(y_lut[kIndex]);
  const auto kY1 = static_cast<double>(y_lut[kIndex + 1]);

  const auto kDx = kX1 - kX0;
  const double kT = (x_in - kX0) / kDx;
  return kY0 + (kT * (kY1 - kY0));
}
}  // namespace

namespace {
template <typename T>
double ScanLobLerp(const T* x_lut, const T* y_lut, const size_t size,
                   const double x_in) {
  if (x_in >= static_cast<double>(x_lut[size - 1])) {
    return static_cast<double>(y_lut[size - 1]);
  }

  size_t index = 0;
  for (size_t i = 0; i < size - 1; i++) {
    if (x_in < static_cast<double>(x_lut[i + 1])) {
      index = i;
      break;
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
template <typename T>
double BestLobLerp(const T* x_lut, const T* y_lut, const size_t size,
                   const double x_in) {
  return UpperboundLobLerp(x_lut, y_lut, size, x_in);
}
}  // namespace

namespace {
const double kInitMachSpeed = 2.5 * lob::kTableScale;
const double kFinalMachSpeed = 0.25 * lob::kTableScale;
const double kDecrement = 1E-4 * lob::kTableScale;
const auto kResultsSize = static_cast<size_t>(
    std::ceil((kInitMachSpeed - kFinalMachSpeed) / kDecrement));
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
std::vector<double> results;
}  // namespace

namespace {
void NaiveBM(benchmark::State& state) {
  results.resize(kResultsSize);
  results.clear();
  for (auto _ : state) {
    double velocity = kInitMachSpeed;
    while (velocity > kFinalMachSpeed) {
      const double kResult = NaiveLobLerp(
          lob::kMachs.data(), lob::kG1Drags.data(), lob::kTableSize, velocity);
      results.push_back(kResult);
      velocity -= kDecrement;
    }
  }
}
}  // namespace

namespace {
void BranchlessBM(benchmark::State& state) {
  results.resize(kResultsSize);
  results.clear();
  for (auto _ : state) {
    double velocity = kInitMachSpeed;
    while (velocity > kFinalMachSpeed) {
      const double kResult = BranchlessLobLerp(
          lob::kMachs.data(), lob::kG1Drags.data(), lob::kTableSize, velocity);
      results.push_back(kResult);
      velocity -= kDecrement;
    }
  }
}
}  // namespace

namespace {
void CachedBM(benchmark::State& state) {
  results.resize(kResultsSize);
  results.clear();
  for (auto _ : state) {
    double velocity = kInitMachSpeed;
    while (velocity > kFinalMachSpeed) {
      const double kResult = CachedLobLerp(
          lob::kMachs.data(), lob::kG1Drags.data(), lob::kTableSize, velocity);
      results.push_back(kResult);
      velocity -= kDecrement;
    }
  }
}
}  // namespace

namespace {
void BinaryBM(benchmark::State& state) {
  results.resize(kResultsSize);
  results.clear();
  for (auto _ : state) {
    double velocity = kInitMachSpeed;
    while (velocity > kFinalMachSpeed) {
      const double kResult = BinaryLobLerp(
          lob::kMachs.data(), lob::kG1Drags.data(), lob::kTableSize, velocity);
      results.push_back(kResult);
      velocity -= kDecrement;
    }
  }
}
}  // namespace

namespace {
void UpperboundBM(benchmark::State& state) {
  results.resize(kResultsSize);
  results.clear();
  for (auto _ : state) {
    double velocity = kInitMachSpeed;
    while (velocity > kFinalMachSpeed) {
      const double kResult = UpperboundLobLerp(
          lob::kMachs.data(), lob::kG1Drags.data(), lob::kTableSize, velocity);
      results.push_back(kResult);
      velocity -= kDecrement;
    }
  }
}
}  // namespace

namespace {
void ScanBM(benchmark::State& state) {
  results.resize(kResultsSize);
  results.clear();
  for (auto _ : state) {
    double velocity = kInitMachSpeed;
    while (velocity > kFinalMachSpeed) {
      const double kResult = ScanLobLerp(
          lob::kMachs.data(), lob::kG1Drags.data(), lob::kTableSize, velocity);
      results.push_back(kResult);
      velocity -= kDecrement;
    }
  }
}
}  // namespace

namespace {
void BestBM(benchmark::State& state) {
  results.resize(kResultsSize);
  results.clear();
  for (auto _ : state) {
    double velocity = kInitMachSpeed;
    while (velocity > kFinalMachSpeed) {
      const double kResult = BestLobLerp(
          lob::kMachs.data(), lob::kG1Drags.data(), lob::kTableSize, velocity);
      results.push_back(kResult);
      velocity -= kDecrement;
    }
  }
}
}  // namespace

// NOLINTBEGIN
// Register the benchmark functions
BENCHMARK(NaiveBM);
BENCHMARK(BranchlessBM);
BENCHMARK(CachedBM);
BENCHMARK(BinaryBM);
BENCHMARK(UpperboundBM);
BENCHMARK(ScanBM);
BENCHMARK(BestBM);

}  // namespace benchmarks

// macro that expands into main function
BENCHMARK_MAIN();
// NOLINTEND

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.