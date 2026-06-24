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

struct SolutionTolerances {
  uint16_t velocity = 1;
  int energy = -1;
  double moa = 0.1;
  double inch = -1.0;
  double time_of_flight = 0.01;
};

template <size_t N>
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
  b.BallisticCoefficientPsi(0.425)
      .DiameterInch(0.308)
      .MassGrains(180.0)
      .InitialVelocityFps(2700U)
      .ZeroAngleMOA(3.38);
}

}  // namespace tests
