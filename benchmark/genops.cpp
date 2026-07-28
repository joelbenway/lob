// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include <benchmark/benchmark.h>

#include <array>
#include <cstddef>
#include <cstdint>

#include "lob/lob.hpp"
#include "tables.hpp"

namespace benchmark {
namespace {

constexpr uint32_t kYardToFt = 3;
constexpr uint32_t kRangeStepYds = 10;
constexpr size_t kDenseRangeCount = 101;

// Common test ranges in feet (100/300/500/800/1000 yards)
constexpr std::array<uint32_t, 5> kRanges = {300, 900, 1500, 2400, 3000};

// Dense ranges every 10 yards from 0 to 1000 yards (in feet)
const std::array<uint32_t, kDenseRangeCount> kDenseRanges = []() noexcept {
  std::array<uint32_t, kDenseRangeCount> r{};
  for (size_t i = 0; i < r.size(); ++i) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    r[i] = static_cast<uint32_t>(i * kYardToFt * kRangeStepYds);
  }
  return r;
}();

// Common projectile (Sierra GameKing 130gr .277")
constexpr double kBC = 0.436;
constexpr uint16_t kVelocity = 3100U;
constexpr double kZeroAngle = 6.11;
constexpr double kOpticHeight = 1.5;
constexpr double kDiameter = 0.277;
constexpr double kMass = 130.0;
constexpr double kBarrelTwist = 10.0;
constexpr double kNoseLength = 0.705;
constexpr double kTailLength = 0.070;
constexpr double kBaseDiameter = 0.245;
constexpr double kMeplatDiameter = 0.0;
constexpr double kOgiveRtR = 0.88;
constexpr double kBulletLength = 1.234;

void GenopsMinimal(benchmark::State& state) {
  const lob::Context kCtx = lob::Builder()
                                .BallisticCoefficientPsi(kBC)
                                .InitialVelocityFps(kVelocity)
                                .ZeroAngleMOA(kZeroAngle)
                                .Build();
  for (auto _ : state) {
    std::array<lob::Output, kRanges.size()> outs{};
    auto n = lob::Solve(kCtx, kRanges, &outs);
    benchmark::DoNotOptimize(n);
    benchmark::DoNotOptimize(outs);
  }
}
BENCHMARK(GenopsMinimal);

void GenopsZeroBuild(benchmark::State& state) {
  const lob::Context kCtx = lob::Builder()
                                .BallisticCoefficientPsi(kBC)
                                .InitialVelocityFps(kVelocity)
                                .ZeroDistanceYds(100.0)
                                .Build();
  for (auto _ : state) {
    std::array<lob::Output, kRanges.size()> outs{};
    auto n = lob::Solve(kCtx, kRanges, &outs);
    benchmark::DoNotOptimize(n);
    benchmark::DoNotOptimize(outs);
  }
}
BENCHMARK(GenopsZeroBuild);

void GenopsCustomTable(benchmark::State& state) {
  const lob::Context kCtx = lob::Builder()
                                .BCAtmosphere(lob::AtmosphereReferenceT::kIcao)
                                .InitialVelocityFps(kVelocity)
                                .ZeroAngleMOA(kZeroAngle)
                                .MachVsDragTable(lob::dragtable::kMachs,
                                                 lob::dragtable::kG1Drags)
                                .Build();
  for (auto _ : state) {
    std::array<lob::Output, kRanges.size()> outs{};
    auto n = lob::Solve(kCtx, kRanges, &outs);
    benchmark::DoNotOptimize(n);
    benchmark::DoNotOptimize(outs);
  }
}
BENCHMARK(GenopsCustomTable);

void GenopsBoatright(benchmark::State& state) {
  const lob::Context kCtx = lob::Builder()
                                .BallisticCoefficientPsi(kBC)
                                .BCDragFunction(lob::DragFunctionT::kG1)
                                .BCAtmosphere(lob::AtmosphereReferenceT::kIcao)
                                .DiameterInch(kDiameter)
                                .LengthInch(kBulletLength)
                                .MassGrains(kMass)
                                .InitialVelocityFps(kVelocity)
                                .OpticHeightInches(kOpticHeight)
                                .ZeroAngleMOA(kZeroAngle)
                                .TwistInchesPerTurn(kBarrelTwist)
                                .NoseLengthInch(kNoseLength)
                                .TailLengthInch(kTailLength)
                                .BaseDiameterInch(kBaseDiameter)
                                .MeplatDiameterInch(kMeplatDiameter)
                                .OgiveRtR(kOgiveRtR)
                                .Build();
  for (auto _ : state) {
    std::array<lob::Output, kRanges.size()> outs{};
    auto n = lob::Solve(kCtx, kRanges, &outs);
    benchmark::DoNotOptimize(n);
    benchmark::DoNotOptimize(outs);
  }
}
BENCHMARK(GenopsBoatright);

void GenopsSolveMany(benchmark::State& state) {
  const lob::Context kCtx = lob::Builder()
                                .BallisticCoefficientPsi(kBC)
                                .InitialVelocityFps(kVelocity)
                                .ZeroAngleMOA(kZeroAngle)
                                .Build();
  for (auto _ : state) {
    std::array<lob::Output, kDenseRanges.size()> outs{};
    auto n = lob::Solve(kCtx, kDenseRanges, &outs);
    benchmark::DoNotOptimize(n);
    benchmark::DoNotOptimize(outs);
  }
}
BENCHMARK(GenopsSolveMany);

void GenopsSolveLong(benchmark::State& state) {
  const lob::Context kCtx = lob::Builder()
                                .BallisticCoefficientPsi(kBC)
                                .InitialVelocityFps(kVelocity)
                                .ZeroAngleMOA(kZeroAngle)
                                .Build();
  constexpr uint32_t kLongRangeFt = 15000;  // 5000 yards
  for (auto _ : state) {
    lob::Output out{};
    auto n = lob::Solve(kCtx, kLongRangeFt, &out);
    benchmark::DoNotOptimize(n);
    benchmark::DoNotOptimize(out);
  }
}
BENCHMARK(GenopsSolveLong);

}  // namespace
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
