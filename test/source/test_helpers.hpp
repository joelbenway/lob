// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#pragma once

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "lob/lob.hpp"

namespace tests {

constexpr uint16_t kDefaultVelocityError = 1;
constexpr int kDefaultEnergyError = -1;
constexpr double kDefaultMoaError = 0.1;
constexpr double kDefaultInchError = -1.0;
constexpr double kDefaultTimeOfFlightError = 0.01;

struct SolutionTolerances {
  uint16_t velocity = kDefaultVelocityError;
  int energy = kDefaultEnergyError;
  double moa = kDefaultMoaError;
  double inch = kDefaultInchError;
  double time_of_flight = kDefaultTimeOfFlightError;
};

template <size_t N>
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void VerifySolutions(const std::array<lob::Output, N>& solutions,
                     const std::vector<lob::Output>& expected,
                     const SolutionTolerances& tolerances = {}) {
  for (size_t i = 0; i < N; i++) {
    EXPECT_EQ(solutions.at(i).range, expected.at(i).range);
    EXPECT_NEAR(solutions.at(i).velocity,
                static_cast<double>(expected.at(i).velocity),
                static_cast<double>(tolerances.velocity));
    if (tolerances.energy >= 0) {
      EXPECT_NEAR(solutions.at(i).energy,
                  static_cast<double>(expected.at(i).energy),
                  static_cast<double>(tolerances.energy));
    }
    const double kSolutionElevationMoa =
        lob::InchToMoa(solutions.at(i).elevation, solutions.at(i).range);
    const double kExpectedElevationMoa =
        lob::InchToMoa(expected.at(i).elevation, expected.at(i).range);
    EXPECT_NEAR(kSolutionElevationMoa, kExpectedElevationMoa, tolerances.moa);
    if (tolerances.inch >= 0.0) {
      EXPECT_NEAR(solutions.at(i).elevation, expected.at(i).elevation,
                  tolerances.inch);
    }
    const double kSolutionDeflectionMoa =
        lob::InchToMoa(solutions.at(i).deflection, solutions.at(i).range);
    const double kExpectedDeflectionMoa =
        lob::InchToMoa(expected.at(i).deflection, expected.at(i).range);
    EXPECT_NEAR(kSolutionDeflectionMoa, kExpectedDeflectionMoa, tolerances.moa);
    if (tolerances.inch >= 0.0) {
      EXPECT_NEAR(solutions.at(i).deflection, expected.at(i).deflection,
                  tolerances.inch);
    }
    EXPECT_NEAR(solutions.at(i).time_of_flight, expected.at(i).time_of_flight,
                tolerances.time_of_flight);
  }
}

template <size_t N>
void VerifySolutionDifferences(
    const std::array<lob::Output, N>& solutions1,
    const std::array<lob::Output, N>& solutions2,
    const std::array<double, N>& expected_elevation_diff,
    const std::array<double, N>& expected_deflection_diff, double tolerance) {
  for (size_t i = 0; i < N; i++) {
    const auto kElevationDifference =
        solutions2.at(i).elevation - solutions1.at(i).elevation;
    const auto kDeflectionDifference =
        solutions2.at(i).deflection - solutions1.at(i).deflection;
    EXPECT_NEAR(kElevationDifference, expected_elevation_diff.at(i), tolerance);
    EXPECT_NEAR(kDeflectionDifference, expected_deflection_diff.at(i),
                tolerance);
  }
}

inline void SetupTestBuilder(lob::Builder& b) {
  constexpr double kTestBC = 0.425;
  constexpr double kTestDiameter = 0.308;
  constexpr double kTestWeight = 180.0;
  constexpr uint16_t kTestMuzzleVelocity = 2700U;
  constexpr double kTestZeroAngle = 3.38;
  b.BallisticCoefficientPsi(kTestBC)
      .DiameterInch(kTestDiameter)
      .MassGrains(kTestWeight)
      .InitialVelocityFps(kTestMuzzleVelocity)
      .ZeroAngleMOA(kTestZeroAngle);
}

}  // namespace tests
