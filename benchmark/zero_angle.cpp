// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include <benchmark/benchmark.h>

#include <cstdint>

#include "lob/lob.hpp"

namespace benchmark {
namespace {

constexpr double kBC = 0.436;
constexpr uint16_t kVelocity = 3100U;
constexpr double kZeroHeight = 3.0;
constexpr double kZero100 = 100.0;
constexpr double kZero500 = 500.0;
constexpr double kZero1000 = 1000.0;
constexpr double kZeroAnglePreset = 6.11;

void ZeroSearchBaseline100yd(benchmark::State& state) {
  for (auto _ : state) {
    const lob::Context kResult =
        lob::Builder()
            .BallisticCoefficientPsi(kBC)
            .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
            .InitialVelocityFps(kVelocity)
            .ZeroDistanceYds(kZero100)
            .ZeroImpactHeightInches(kZeroHeight)
            .Build();
    benchmark::DoNotOptimize(&kResult);
  }
}

void ZeroSearchBaseline500yd(benchmark::State& state) {
  for (auto _ : state) {
    const lob::Context kResult =
        lob::Builder()
            .BallisticCoefficientPsi(kBC)
            .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
            .InitialVelocityFps(kVelocity)
            .ZeroDistanceYds(kZero500)
            .ZeroImpactHeightInches(kZeroHeight)
            .Build();
    benchmark::DoNotOptimize(&kResult);
  }
}

void ZeroSearchBaseline1000yd(benchmark::State& state) {
  for (auto _ : state) {
    const lob::Context kResult =
        lob::Builder()
            .BallisticCoefficientPsi(kBC)
            .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
            .InitialVelocityFps(kVelocity)
            .ZeroDistanceYds(kZero1000)
            .ZeroImpactHeightInches(kZeroHeight)
            .Build();
    benchmark::DoNotOptimize(&kResult);
  }
}

void ZeroSearchPreset(benchmark::State& state) {
  for (auto _ : state) {
    const lob::Context kResult =
        lob::Builder()
            .BallisticCoefficientPsi(kBC)
            .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
            .InitialVelocityFps(kVelocity)
            .ZeroDistanceYds(kZero100)
            .ZeroImpactHeightInches(kZeroHeight)
            .ZeroAngleMOA(kZeroAnglePreset)
            .Build();
    benchmark::DoNotOptimize(&kResult);
  }
}

}  // namespace

BENCHMARK(ZeroSearchBaseline100yd);
BENCHMARK(ZeroSearchBaseline500yd);
BENCHMARK(ZeroSearchBaseline1000yd);
BENCHMARK(ZeroSearchPreset);

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
