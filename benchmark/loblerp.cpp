// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include <benchmark/benchmark.h>

#include <cmath>
#include <cstddef>
#include <vector>

#include "splines.hpp"
#include "tables.hpp"

namespace benchmark {
namespace {
constexpr double kInitMach = 2.5;
constexpr double kFinalMach = 0.25;
constexpr double kDecrement = 1E-4;
const auto kResultsSize =
    static_cast<size_t>(std::ceil((kInitMach - kFinalMach) / kDecrement));

// Benchmark 1: LobLerp — binary search + linear interp from tables.hpp
void LobLerpBM(benchmark::State& state) {
  static std::vector<double> results(kResultsSize);
  size_t index = 0;
  for (auto _ : state) {
    double mach = kInitMach;
    while (mach > kFinalMach) {
      const auto kResult = lob::dragtable::LobLerp(
          lob::dragtable::kMachs, lob::dragtable::kG1Drags, mach);
      results.at(index++) = kResult;
      mach -= kDecrement;
    }
    index = 0;
  }
}

// Benchmark 2: spline::CurveView — caching spline evaluation (stateful index)
void CurveViewBM(benchmark::State& state) {
  static std::vector<double> results(kResultsSize);
  size_t index = 0;
  for (auto _ : state) {
    lob::spline::CurveView curve{lob::spline::kKnots, lob::spline::kG1Coefs};
    double mach = kInitMach;
    while (mach > kFinalMach) {
      const auto kResult =
          static_cast<double>(curve.Eval(static_cast<float>(mach)));
      results.at(index++) = kResult;
      mach -= kDecrement;
    }
    index = 0;
  }
}
}  // namespace

BENCHMARK(LobLerpBM);
BENCHMARK(CurveViewBM);

}  // namespace benchmark

BENCHMARK_MAIN();

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
