// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include <benchmark/benchmark.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <vector>

#include "splines.hpp"
#include "tables.hpp"

namespace benchmarks {

constexpr double kInitMach = 2.5;
constexpr double kFinalMach = 0.25;
constexpr double kDecrement = 1E-4;
const auto kResultsSize = static_cast<size_t>(
    std::ceil((kInitMach - kFinalMach) / kDecrement));
std::vector<double> results(kResultsSize);

// Benchmark 1: LobLerp — binary search + linear interp from tables.hpp
void BM_LobLerp(benchmark::State& state) {
  size_t index = 0;
  for (auto _ : state) {
    double mach = kInitMach;
    while (mach > kFinalMach) {
      const double kResult = lob::dragtable::LobLerp(
          lob::dragtable::kMachs, lob::dragtable::kG1Drags, mach);
      results.at(index++) = kResult;
      mach -= kDecrement;
    }
    index = 0;
  }
}

// Benchmark 2: spline::View — no-cache spline evaluation
void BM_View(benchmark::State& state) {
  lob::spline::View<float> view{lob::spline::kKnots.data(),
                                lob::spline::kG1Coefs.data(),
                                lob::spline::kKnotCount};
  size_t index = 0;
  for (auto _ : state) {
    double mach = kInitMach;
    while (mach > kFinalMach) {
      const double kResult =
          static_cast<double>(view.Eval(static_cast<float>(mach)));
      results.at(index++) = kResult;
      mach -= kDecrement;
    }
    index = 0;
  }
}

// Benchmark 3: spline::Cursor — caching spline evaluation (stateful index)
void BM_Cursor(benchmark::State& state) {
  size_t index = 0;
  for (auto _ : state) {
    lob::spline::Cursor<float, lob::spline::kKnotCount> cursor{
        lob::spline::kKnots.data(), lob::spline::kG1Coefs.data()};
    double mach = kInitMach;
    while (mach > kFinalMach) {
      const double kResult =
          static_cast<double>(cursor.Eval(static_cast<float>(mach)));
      results.at(index++) = kResult;
      mach -= kDecrement;
    }
    index = 0;
  }
}

BENCHMARK(BM_LobLerp);
BENCHMARK(BM_View);
BENCHMARK(BM_Cursor);

}  // namespace benchmarks

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
