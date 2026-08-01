// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "lob/lob.hpp"
#include "tables.hpp"  // internal: dragtable data used as custom table

namespace {

constexpr std::size_t kNumRanges = 7;
constexpr std::array<std::uint32_t, kNumRanges> kRanges = {
    0U, 300U, 600U, 900U, 1200U, 1500U, 1800U};

constexpr double kBC = 0.425;
constexpr uint16_t kVelocity = 2700U;
constexpr double kMeplatDiameter = 0.065;
constexpr double kDiameter = 0.308;
constexpr double kBaseDiameter = 0.242;
constexpr double kNoseLength = 0.690;
constexpr double kLength = 1.215;
constexpr double kTailLength = 0.140;
constexpr double kOgiveRtR = 0.90;
constexpr double kMass = 168.0;
constexpr double kOpticHeight = 2.5;
constexpr double kTwist = 10.0;
constexpr double kPressure = 30.3;
constexpr double kTemperature = 63.1;
constexpr double kHumidity = 77.0;
constexpr double kWindSpeed = 5.0;
constexpr double kLatitude = 43.04;
constexpr double kAzimuth = 180.0;
constexpr uint16_t kStepSize = 36U;
constexpr double kZeroDistance = 100.0;
constexpr double kZeroAngle = 5.0;

lob::Context BuildBasic() {
  const auto kContext = lob::Builder()
                            .BallisticCoefficientPsi(kBC)
                            .BCAtmosphere(lob::AtmosphereReferenceT::kIcao)
                            .BCDragFunction(lob::DragFunctionT::kG7)
                            .InitialVelocityFps(kVelocity)
                            .ZeroAngleMOA(kZeroAngle)
                            .Build();
  return kContext;
}

lob::Context BuildFull() {
  const auto kContext = lob::Builder()
                            .BallisticCoefficientPsi(kBC)
                            .BCAtmosphere(lob::AtmosphereReferenceT::kIcao)
                            .BCDragFunction(lob::DragFunctionT::kG7)
                            .DiameterInch(kDiameter)
                            .LengthInch(kLength)
                            .MassGrains(kMass)
                            .InitialVelocityFps(kVelocity)
                            .ZeroAngleMOA(kZeroAngle)
                            .OpticHeightInches(kOpticHeight)
                            .TwistInchesPerTurn(kTwist)
                            .AirPressureInHg(kPressure)
                            .TemperatureDegF(kTemperature)
                            .RelativeHumidityPercent(kHumidity)
                            .WindHeading(lob::ClockAngleT::kIII)
                            .WindSpeedMph(kWindSpeed)
                            .LatitudeDeg(kLatitude)
                            .AzimuthDeg(kAzimuth)
                            .StepSize(kStepSize)
                            .Build();
  return kContext;
}

lob::Context BuildCustomTable() {
  const auto kContext =
      lob::Builder()
          .MachVsDragTable(lob::dragtable::kMachs, lob::dragtable::kG6Drags)
          .InitialVelocityFps(kVelocity)
          .ZeroAngleMOA(kZeroAngle)
          .Build();
  return kContext;
}

lob::Context BuildBoatright() {
  const auto kContext = lob::Builder()
                            .BallisticCoefficientPsi(kBC)
                            .BCAtmosphere(lob::AtmosphereReferenceT::kIcao)
                            .BCDragFunction(lob::DragFunctionT::kG7)
                            .DiameterInch(kDiameter)
                            .MeplatDiameterInch(kMeplatDiameter)
                            .BaseDiameterInch(kBaseDiameter)
                            .LengthInch(kLength)
                            .NoseLengthInch(kNoseLength)
                            .TailLengthInch(kTailLength)
                            .OgiveRtR(kOgiveRtR)
                            .MassGrains(kMass)
                            .TwistInchesPerTurn(kTwist)
                            .InitialVelocityFps(kVelocity)
                            .ZeroAngleMOA(kZeroAngle)
                            .OpticHeightInches(kOpticHeight)
                            .Build();
  return kContext;
}

// Zero-by-distance triggers internal solves; subtract build_basic for the search cost.
lob::Context BuildZeroSearch() {
  const auto kContext = lob::Builder()
                            .BallisticCoefficientPsi(kBC)
                            .BCAtmosphere(lob::AtmosphereReferenceT::kIcao)
                            .BCDragFunction(lob::DragFunctionT::kG7)
                            .InitialVelocityFps(kVelocity)
                            .ZeroDistanceYds(kZeroDistance)
                            .Build();
  return kContext;
}

// Exit 3 if the builder bailed early, so a broken chain is never measured silently.
int RequireOk(const lob::Context& ctx, const char* what) {
  if (ctx.error != lob::ErrorT::kNone) {
    std::cerr << "FATAL: " << what << " built with error "
              << static_cast<unsigned>(ctx.error)
              << " -- the benchmark would be measuring the builder's "
                 "early-out path\n";
    return 3;
  }
  return 0;
}

template <lob::Context (*Fn)()>
void RunBuild(std::uint64_t reps) {
  double acc = 0.0;
  for (std::uint64_t i = 0; i < reps; ++i) {
    const lob::Context kCtx = Fn();
    acc +=
        kCtx.zero_angle + kCtx.drag_coeff + static_cast<double>(kCtx.velocity);
  }
  static volatile double sink = 0.0;
  (void)(sink = acc);
}

template <lob::Context (*Fn)()>
void RunSolve(std::uint64_t reps) {
  const lob::Context kCtx = Fn();  // setup outside the loop
  std::array<lob::Output, kNumRanges> outputs = {};
  double acc = 0.0;
  for (std::uint64_t i = 0; i < reps; ++i) {
    const std::size_t kSolved = lob::Solve(kCtx, kRanges, &outputs);
    acc += static_cast<double>(kSolved) + outputs.at(kNumRanges - 1U).elevation;
  }
  static volatile double sink = 0.0;
  (void)(sink = acc);
}

struct Case {
  const char* name;
  void (*run)(std::uint64_t);
  lob::Context (*build)();
};

constexpr std::array<Case, 7> kCases = {{
    {"build_basic", &RunBuild<&BuildBasic>, &BuildBasic},
    {"build_full", &RunBuild<&BuildFull>, &BuildFull},
    {"build_custom_table", &RunBuild<&BuildCustomTable>, &BuildCustomTable},
    {"build_boatright", &RunBuild<&BuildBoatright>, &BuildBoatright},
    {"build_zero_search", &RunBuild<&BuildZeroSearch>, &BuildZeroSearch},
    {"solve_basic", &RunSolve<&BuildBasic>, &BuildBasic},
    {"solve_boatright", &RunSolve<&BuildBoatright>, &BuildBoatright},
}};

}  // namespace

int main(int argc, char** argv) {
  if (argc == 2 && std::strcmp(argv[1], "--list") == 0) {
    for (const auto& c : kCases) {
      std::cout << c.name << '\n';
    }
    return 0;
  }

  // Cheap sanity gate for ctest: every case must build cleanly.
  if (argc == 2 && std::strcmp(argv[1], "--verify") == 0) {
    for (const auto& c : kCases) {
      const int kOk = RequireOk(c.build(), c.name);
      if (kOk != 0) {
        return kOk;
      }
      std::cout << "ok  " << c.name << '\n';
    }
    return 0;
  }

  if (argc != 3) {
    std::cerr << "usage: " << argv[0]
              << " <case> <reps> | --list | --verify\n";
    return 2;
  }

  const std::uint64_t kReps = std::strtoull(argv[2], nullptr, 10);

  for (const auto& c : kCases) {
    if (std::strcmp(c.name, argv[1]) == 0) {
      // Runs in BOTH the reps=0 and reps=N passes, so it cancels exactly.
      const int kOk = RequireOk(c.build(), c.name);
      if (kOk != 0) {
        return kOk;
      }
      c.run(kReps);
      return 0;
    }
  }
  std::cerr << "unknown case: " << argv[1] << '\n';
  return 2;
}

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