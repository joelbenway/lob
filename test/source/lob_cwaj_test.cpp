// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <memory>
#include <vector>

#include "lob/lob.hpp"

namespace tests {

struct LobCWAJTestFixture : public testing::Test {
  // Unit under test
  std::unique_ptr<lob::Builder> puut;

  LobCWAJTestFixture() : puut(nullptr) {}

  const double kZeroDistance = 300;
  const double kBarrelTwist = 11.0;
  const double kOgiveLength = 0.748;
  const double kTailLength = 0.257;
  const double kMeplatDiameter = 0.069;
  const double kBaseDiameter = 0.276;
  const double kRtR = 0.99;

  void SetUp() override {
    ASSERT_EQ(puut, nullptr);
    puut = std::make_unique<lob::Builder>();
    ASSERT_NE(puut, nullptr);

    // Lapua 250gr FMJBT pg 656, Ballistic Performance of Rifle Bullets - Litz
    const double kTestBC = 0.308;
    const lob::DragFunctionT kDragFunction = lob::DragFunctionT::kG7;
    const double kTestDiameter = 0.338;
    const double kTestWeight = 250.0;
    const double kBulletLength = 1.471;
    const uint16_t kTestMuzzleVelocity = 3071;
    const double kTestZeroAngle = 6.53;
    const double kTestOpticHeight = 2.0;
    const uint16_t kStep = 100U;

    puut->BallisticCoefficientPsi(kTestBC)
        .BCDragFunction(kDragFunction)
        .BCAtmosphere(lob::AtmosphereReferenceT::kIcao)
        .DiameterInch(kTestDiameter)
        .MassGrains(kTestWeight)
        .LengthInch(kBulletLength)
        .InitialVelocityFps(kTestMuzzleVelocity)
        .OpticHeightInches(kTestOpticHeight)
        .ZeroAngleMOA(kTestZeroAngle)
        .StepSize(kStep);
  }

  void TearDown() override { puut.reset(); }
};

TEST_F(LobCWAJTestFixture, ZeroAngleSearch) {
  ASSERT_NE(puut, nullptr);
  auto input1 = puut->Build();
  auto input2 = puut->ZeroAngleMOA(std::numeric_limits<double>::quiet_NaN())
                    .ZeroDistanceYds(kZeroDistance)
                    .Build();
  const double kError = 0.01;
  EXPECT_NEAR(input1.zero_angle, input2.zero_angle, kError);
}

TEST_F(LobCWAJTestFixture, GetSpeedOfSoundFps) {
  ASSERT_NE(puut, nullptr);
  const auto kInput = puut->Build();
  const double kExpectedFps = 1116.45;
  const double kError = 0.001;
  EXPECT_NEAR(kInput.speed_of_sound, kExpectedFps, kError);
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
TEST_F(LobCWAJTestFixture, SolveWithoutSpin) {
  ASSERT_NE(puut, nullptr);
  constexpr uint16_t kVelocityError = 1;
  constexpr uint16_t kEnergyError = 5;
  constexpr double kMoaError = 0.1;
  constexpr double kInchError = 0.5;
  constexpr double kTimeOfFlightError = 0.01;
  constexpr size_t kSolutionLength = 14;
  const auto kInput = puut->Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0,    150,  300,  600,  900,  1200, 1500,
      1800, 2100, 2400, 2700, 3000, 4500, 6000};
  const std::vector<lob::Output> kExpected = {
      {0, 3071, 5230, -2.00, 0.00, 0.000},
      {150, 2992, 4963, 0.95, 0.00, 0.049},
      {300, 2913, 4706, 2.93, 0.00, 0.100},
      {600, 2759, 4221, 3.76, 0.00, 0.206},
      {900, 2609, 3775, 0.02, 0.00, 0.318},
      {1200, 2464, 3366, -8.83, 0.00, 0.436},
      {1500, 2323, 2992, -23.42, 0.00, 0.562},
      {1800, 2187, 2652, -44.45, 0.00, 0.695},
      {2100, 2056, 2344, -72.76, 0.00, 0.836},
      {2400, 1929, 2064, -109.32, 0.00, 0.987},
      {2700, 1806, 1810, -155.22, 0.00, 1.148},
      {3000, 1687, 1579, -211.78, 0.00, 1.319},
      {4500, 1150, 734, -721.83, 0.00, 2.397},
      {6000, 949, 500, -1882.00, 0.00, 3.867}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  const size_t kSize = lob::Solve(kInput, kRanges, solutions);
  EXPECT_EQ(kSize, kSolutionLength);
  for (size_t i = 0; i < kSolutionLength; i++) {
    EXPECT_EQ(solutions.at(i).range, kExpected.at(i).range);
    EXPECT_NEAR(solutions.at(i).velocity, kExpected.at(i).velocity,
                kVelocityError);
    EXPECT_NEAR(solutions.at(i).energy, kExpected.at(i).energy, kEnergyError);
    const double kSolutionElevationMoa =
        lob::InchToMoa(solutions.at(i).elevation, solutions.at(i).range);
    const double kExpectedElevationMoa =
        lob::InchToMoa(kExpected.at(i).elevation, kExpected.at(i).range);
    EXPECT_NEAR(kSolutionElevationMoa, kExpectedElevationMoa, kMoaError);
    EXPECT_NEAR(solutions.at(i).elevation, kExpected.at(i).elevation,
                kInchError);
    const double kSolutionDeflectionMoa =
        lob::InchToMoa(solutions.at(i).deflection, solutions.at(i).range);
    const double kExpectedDeflectionMoa =
        lob::InchToMoa(kExpected.at(i).deflection, kExpected.at(i).range);
    EXPECT_NEAR(kSolutionDeflectionMoa, kExpectedDeflectionMoa, kMoaError);
    EXPECT_NEAR(solutions.at(i).deflection, kExpected.at(i).deflection,
                kInchError);
    EXPECT_NEAR(solutions.at(i).time_of_flight, kExpected.at(i).time_of_flight,
                kTimeOfFlightError);
  }
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
TEST_F(LobCWAJTestFixture, LitzRightHandSpinLeftwardWind) {
  ASSERT_NE(puut, nullptr);
  constexpr double kWind = 15.0;
  constexpr lob::ClockAngleT kWindHeading = lob::ClockAngleT::kIX;
  constexpr uint16_t kVelocityError = 1;
  constexpr uint16_t kEnergyError = 5;
  constexpr double kMoaError = 0.1;
  constexpr double kInchError = 0.5;
  constexpr double kTimeOfFlightError = 0.01;
  constexpr double kAerodynamicJump = 0.650208;
  constexpr size_t kSolutionLength = 14;
  const auto kInput = puut->TwistInchesPerTurn(kBarrelTwist)
                          .WindSpeedMph(kWind)
                          .WindHeading(kWindHeading)
                          .Build();
  EXPECT_NEAR(kInput.aerodynamic_jump, kAerodynamicJump, kMoaError);
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0,    150,  300,  600,  900,  1200, 1500,
      1800, 2100, 2400, 2700, 3000, 4500, 6000};
  const std::vector<lob::Output> kExpected = {
      {0, 3071, 5230, -2.00, 0.00, 0.000},
      {150, 2992, 4963, 1.29, -0.15, 0.049},
      {300, 2913, 4706, 3.61, -0.63, 0.100},
      {600, 2759, 4221, 5.12, -2.60, 0.206},
      {900, 2609, 3775, 2.06, -6.05, 0.318},
      {1200, 2464, 3366, -6.11, -11.09, 0.436},
      {1500, 2323, 2992, -20.02, -17.87, 0.562},
      {1800, 2187, 2652, -40.37, -26.52, 0.695},
      {2100, 2056, 2344, -68.00, -37.21, 0.836},
      {2400, 1929, 2064, -103.88, -50.11, 0.987},
      {2700, 1806, 1810, -149.10, -65.44, 1.148},
      {3000, 1687, 1579, -204.97, -83.43, 1.319},
      {4500, 1150, 734, -711.68, -225.13, 2.397},
      {6000, 949, 500, -1868.40, -455.01, 3.867}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  const size_t kSize = lob::Solve(kInput, kRanges, solutions);
  EXPECT_EQ(kSize, kSolutionLength);
  for (size_t i = 0; i < kSolutionLength; i++) {
    EXPECT_EQ(solutions.at(i).range, kExpected.at(i).range);
    EXPECT_NEAR(solutions.at(i).velocity, kExpected.at(i).velocity,
                kVelocityError);
    EXPECT_NEAR(solutions.at(i).energy, kExpected.at(i).energy, kEnergyError);
    const double kSolutionElevationMoa =
        lob::InchToMoa(solutions.at(i).elevation, solutions.at(i).range);
    const double kExpectedElevationMoa =
        lob::InchToMoa(kExpected.at(i).elevation, kExpected.at(i).range);
    EXPECT_NEAR(kSolutionElevationMoa, kExpectedElevationMoa, kMoaError);
    EXPECT_NEAR(solutions.at(i).elevation, kExpected.at(i).elevation,
                kInchError);
    const double kSolutionDeflectionMoa =
        lob::InchToMoa(solutions.at(i).deflection, solutions.at(i).range);
    const double kExpectedDeflectionMoa =
        lob::InchToMoa(kExpected.at(i).deflection, kExpected.at(i).range);
    EXPECT_NEAR(kSolutionDeflectionMoa, kExpectedDeflectionMoa, kMoaError);
    EXPECT_NEAR(solutions.at(i).deflection, kExpected.at(i).deflection,
                kInchError);
    EXPECT_NEAR(solutions.at(i).time_of_flight, kExpected.at(i).time_of_flight,
                kTimeOfFlightError);
  }
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
TEST_F(LobCWAJTestFixture, LitzLeftHandSpinLeftwardWind) {
  ASSERT_NE(puut, nullptr);
  const double kLeftTwist = -1.0 * kBarrelTwist;
  constexpr double kWind = 15.0;
  constexpr lob::ClockAngleT kWindHeading = lob::ClockAngleT::kIX;
  constexpr uint16_t kVelocityError = 1;
  constexpr uint16_t kEnergyError = 5;
  constexpr double kMoaError = 0.1;
  constexpr double kInchError = 0.5;
  constexpr double kTimeOfFlightError = 0.01;
  constexpr double kAerodynamicJump = -0.650208;
  constexpr size_t kSolutionLength = 14;
  const auto kInput = puut->TwistInchesPerTurn(kLeftTwist)
                          .WindSpeedMph(kWind)
                          .WindHeading(kWindHeading)
                          .Build();
  EXPECT_NEAR(kInput.aerodynamic_jump, kAerodynamicJump, kMoaError);
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0,    150,  300,  600,  900,  1200, 1500,
      1800, 2100, 2400, 2700, 3000, 4500, 6000};
  const std::vector<lob::Output> kExpected = {
      {0, 3071, 5230, -2.00, 0.00, 0.000},
      {150, 2992, 4963, 0.61, -0.19, 0.049},
      {300, 2913, 4706, 2.25, -0.75, 0.100},
      {600, 2759, 4221, 2.40, -3.07, 0.206},
      {900, 2609, 3775, -2.03, -7.09, 0.318},
      {1200, 2464, 3366, -11.56, -12.94, 0.436},
      {1500, 2323, 2992, -26.82, -20.81, 0.562},
      {1800, 2187, 2652, -48.54, -30.86, 0.695},
      {2100, 2056, 2344, -77.53, -43.30, 0.836},
      {2400, 1929, 2064, -114.77, -58.36, 0.987},
      {2700, 1806, 1810, -161.35, -76.31, 1.148},
      {3000, 1687, 1579, -218.59, -97.46, 1.319},
      {4500, 1150, 734, -732.11, -266.99, 2.397},
      {6000, 949, 500, -1895.73, -555.46, 3.868}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  const size_t kSize = lob::Solve(kInput, kRanges, solutions);
  EXPECT_EQ(kSize, kSolutionLength);
  for (size_t i = 0; i < kSolutionLength; i++) {
    EXPECT_EQ(solutions.at(i).range, kExpected.at(i).range);
    EXPECT_NEAR(solutions.at(i).velocity, kExpected.at(i).velocity,
                kVelocityError);
    EXPECT_NEAR(solutions.at(i).energy, kExpected.at(i).energy, kEnergyError);
    const double kSolutionElevationMoa =
        lob::InchToMoa(solutions.at(i).elevation, solutions.at(i).range);
    const double kExpectedElevationMoa =
        lob::InchToMoa(kExpected.at(i).elevation, kExpected.at(i).range);
    EXPECT_NEAR(kSolutionElevationMoa, kExpectedElevationMoa, kMoaError);
    EXPECT_NEAR(solutions.at(i).elevation, kExpected.at(i).elevation,
                kInchError);
    const double kSolutionDeflectionMoa =
        lob::InchToMoa(solutions.at(i).deflection, solutions.at(i).range);
    const double kExpectedDeflectionMoa =
        lob::InchToMoa(kExpected.at(i).deflection, kExpected.at(i).range);
    EXPECT_NEAR(kSolutionDeflectionMoa, kExpectedDeflectionMoa, kMoaError);
    EXPECT_NEAR(solutions.at(i).deflection, kExpected.at(i).deflection,
                kInchError);
    EXPECT_NEAR(solutions.at(i).time_of_flight, kExpected.at(i).time_of_flight,
                kTimeOfFlightError);
  }
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
TEST_F(LobCWAJTestFixture, LitzRightHandSpinRightwardWind) {
  ASSERT_NE(puut, nullptr);
  constexpr double kWind = 15.0;
  constexpr lob::ClockAngleT kWindHeading = lob::ClockAngleT::kIII;
  constexpr uint16_t kVelocityError = 1;
  constexpr uint16_t kEnergyError = 5;
  constexpr double kMoaError = 0.1;
  constexpr double kInchError = 0.5;
  constexpr double kTimeOfFlightError = 0.01;
  constexpr double kAerodynamicJump = -0.650208;
  constexpr size_t kSolutionLength = 14;
  const auto kInput = puut->TwistInchesPerTurn(kBarrelTwist)
                          .WindSpeedMph(kWind)
                          .WindHeading(kWindHeading)
                          .Build();
  EXPECT_NEAR(kInput.aerodynamic_jump, kAerodynamicJump, kMoaError);
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0,    150,  300,  600,  900,  1200, 1500,
      1800, 2100, 2400, 2700, 3000, 4500, 6000};
  const std::vector<lob::Output> kExpected = {
      {0, 3071, 5230, -2.00, 0.00, 0.000},
      {150, 2992, 4963, 0.61, 0.19, 0.049},
      {300, 2913, 4706, 2.25, 0.75, 0.100},
      {600, 2759, 4221, 2.40, 3.07, 0.206},
      {900, 2609, 3775, -2.03, 7.09, 0.318},
      {1200, 2464, 3366, -11.56, 12.94, 0.436},
      {1500, 2323, 2992, -26.82, 20.81, 0.562},
      {1800, 2187, 2652, -48.54, 30.86, 0.695},
      {2100, 2056, 2344, -77.53, 43.30, 0.836},
      {2400, 1929, 2064, -114.77, 58.36, 0.987},
      {2700, 1806, 1810, -161.35, 76.31, 1.148},
      {3000, 1687, 1579, -218.59, 97.46, 1.319},
      {4500, 1150, 734, -732.11, 266.99, 2.397},
      {6000, 949, 500, -1895.73, 555.46, 3.868}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  const size_t kSize = lob::Solve(kInput, kRanges, solutions);
  EXPECT_EQ(kSize, kSolutionLength);
  for (size_t i = 0; i < kSolutionLength; i++) {
    EXPECT_EQ(solutions.at(i).range, kExpected.at(i).range);
    EXPECT_NEAR(solutions.at(i).velocity, kExpected.at(i).velocity,
                kVelocityError);
    EXPECT_NEAR(solutions.at(i).energy, kExpected.at(i).energy, kEnergyError);
    const double kSolutionElevationMoa =
        lob::InchToMoa(solutions.at(i).elevation, solutions.at(i).range);
    const double kExpectedElevationMoa =
        lob::InchToMoa(kExpected.at(i).elevation, kExpected.at(i).range);
    EXPECT_NEAR(kSolutionElevationMoa, kExpectedElevationMoa, kMoaError);
    EXPECT_NEAR(solutions.at(i).elevation, kExpected.at(i).elevation,
                kInchError);
    const double kSolutionDeflectionMoa =
        lob::InchToMoa(solutions.at(i).deflection, solutions.at(i).range);
    const double kExpectedDeflectionMoa =
        lob::InchToMoa(kExpected.at(i).deflection, kExpected.at(i).range);
    EXPECT_NEAR(kSolutionDeflectionMoa, kExpectedDeflectionMoa, kMoaError);
    EXPECT_NEAR(solutions.at(i).deflection, kExpected.at(i).deflection,
                kInchError);
    EXPECT_NEAR(solutions.at(i).time_of_flight, kExpected.at(i).time_of_flight,
                kTimeOfFlightError);
  }
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
TEST_F(LobCWAJTestFixture, LitzLeftHandSpinRightwardWind) {
  ASSERT_NE(puut, nullptr);
  const double kLeftTwist = -1.0 * kBarrelTwist;
  constexpr double kWind = 15.0;
  constexpr lob::ClockAngleT kWindHeading = lob::ClockAngleT::kIII;
  constexpr uint16_t kVelocityError = 1;
  constexpr uint16_t kEnergyError = 5;
  constexpr double kMoaError = 0.1;
  constexpr double kInchError = 0.5;
  constexpr double kTimeOfFlightError = 0.01;
  constexpr double kAerodynamicJump = 0.650208;
  constexpr size_t kSolutionLength = 14;
  const auto kInput = puut->TwistInchesPerTurn(kLeftTwist)
                          .WindSpeedMph(kWind)
                          .WindHeading(kWindHeading)
                          .Build();
  EXPECT_NEAR(kInput.aerodynamic_jump, kAerodynamicJump, kMoaError);
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0,    150,  300,  600,  900,  1200, 1500,
      1800, 2100, 2400, 2700, 3000, 4500, 6000};
  const std::vector<lob::Output> kExpected = {
      {0, 3071, 5230, -2.00, 0.00, 0.000},
      {150, 2992, 4963, 1.29, 0.15, 0.049},
      {300, 2913, 4706, 3.61, 0.63, 0.100},
      {600, 2759, 4221, 5.12, 2.60, 0.206},
      {900, 2609, 3775, 2.06, 6.05, 0.318},
      {1200, 2464, 3366, -6.11, 11.09, 0.436},
      {1500, 2323, 2992, -20.02, 17.87, 0.562},
      {1800, 2187, 2652, -40.37, 26.52, 0.695},
      {2100, 2056, 2344, -68.00, 37.21, 0.836},
      {2400, 1929, 2064, -103.88, 50.11, 0.987},
      {2700, 1806, 1810, -149.10, 65.44, 1.148},
      {3000, 1687, 1579, -204.97, 83.43, 1.319},
      {4500, 1150, 734, -711.68, 225.13, 2.397},
      {6000, 949, 500, -1868.40, 455.01, 3.867}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  const size_t kSize = lob::Solve(kInput, kRanges, solutions);
  EXPECT_EQ(kSize, kSolutionLength);
  for (size_t i = 0; i < kSolutionLength; i++) {
    EXPECT_EQ(solutions.at(i).range, kExpected.at(i).range);
    EXPECT_NEAR(solutions.at(i).velocity, kExpected.at(i).velocity,
                kVelocityError);
    EXPECT_NEAR(solutions.at(i).energy, kExpected.at(i).energy, kEnergyError);
    const double kSolutionElevationMoa =
        lob::InchToMoa(solutions.at(i).elevation, solutions.at(i).range);
    const double kExpectedElevationMoa =
        lob::InchToMoa(kExpected.at(i).elevation, kExpected.at(i).range);
    EXPECT_NEAR(kSolutionElevationMoa, kExpectedElevationMoa, kMoaError);
    EXPECT_NEAR(solutions.at(i).elevation, kExpected.at(i).elevation,
                kInchError);
    const double kSolutionDeflectionMoa =
        lob::InchToMoa(solutions.at(i).deflection, solutions.at(i).range);
    const double kExpectedDeflectionMoa =
        lob::InchToMoa(kExpected.at(i).deflection, kExpected.at(i).range);
    EXPECT_NEAR(kSolutionDeflectionMoa, kExpectedDeflectionMoa, kMoaError);
    EXPECT_NEAR(solutions.at(i).deflection, kExpected.at(i).deflection,
                kInchError);
    EXPECT_NEAR(solutions.at(i).time_of_flight, kExpected.at(i).time_of_flight,
                kTimeOfFlightError);
  }
}
/*
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
TEST_F(LobCWAJTestFixture, BoatrightRightHandSpinLeftwardWind) {
  ASSERT_NE(puut, nullptr);
  constexpr double kWind = 15.0;
  constexpr lob::ClockAngleT kWindHeading = lob::ClockAngleT::kIX;
  constexpr uint16_t kVelocityError = 1;
  constexpr uint16_t kEnergyError = 5;
  constexpr double kMoaError = 0.1;
  constexpr double kInchError = 0.5;
  constexpr double kTimeOfFlightError = 0.01;
  constexpr double kAerodynamicJump = 1.015683;
  constexpr size_t kSolutionLength = 14;
  const auto kInput = puut->TwistInchesPerTurn(kBarrelTwist)
                          .WindSpeedMph(kWind)
                          .WindHeading(kWindHeading)
                          .NoseLengthInch(kOgiveLength)
                          .TailLengthInch(kTailLength)
                          .BaseDiameterInch(kBaseDiameter)
                          .MeplatDiameterInch(kMeplatDiameter)
                          .OgiveRtR(kRtR)
                          .Build();
  EXPECT_NEAR(kInput.aerodynamic_jump, kAerodynamicJump, kMoaError);
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0,    150,  300,  600,  900,  1200, 1500,
      1800, 2100, 2400, 2700, 3000, 4500, 6000};
  const std::vector<lob::Output> kExpected = {
      {0, 3071, 5230, -2.00, 0.00, 0.000},
      {150, 2992, 4963, 1.48, -0.15, 0.049},
      {300, 2913, 4706, 3.99, -0.63, 0.100},
      {600, 2759, 4221, 5.89, -2.60, 0.206},
      {900, 2609, 3775, 3.21, -6.05, 0.318},
      {1200, 2464, 3366, -4.58, -11.09, 0.436},
      {1500, 2323, 2992, -18.10, -17.87, 0.562},
      {1800, 2187, 2652, -38.07, -26.52, 0.695},
      {2100, 2056, 2344, -65.32, -37.21, 0.836},
      {2400, 1929, 2064, -100.82, -50.11, 0.987},
      {2700, 1806, 1810, -145.65, -65.44, 1.148},
      {3000, 1687, 1579, -201.15, -83.43, 1.319},
      {4500, 1150, 734, -705.94, -225.13, 2.397},
      {6000, 949, 500, -1860.74, -455.01, 3.867}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  const size_t kSize = lob::Solve(kInput, kRanges, solutions);
  EXPECT_EQ(kSize, kSolutionLength);
  for (size_t i = 0; i < kSolutionLength; i++) {
    EXPECT_EQ(solutions.at(i).range, kExpected.at(i).range);
    EXPECT_NEAR(solutions.at(i).velocity, kExpected.at(i).velocity,
                kVelocityError);
    EXPECT_NEAR(solutions.at(i).energy, kExpected.at(i).energy, kEnergyError);
    const double kSolutionElevationMoa =
        lob::InchToMoa(solutions.at(i).elevation, solutions.at(i).range);
    const double kExpectedElevationMoa =
        lob::InchToMoa(kExpected.at(i).elevation, kExpected.at(i).range);
    EXPECT_NEAR(kSolutionElevationMoa, kExpectedElevationMoa, kMoaError);
    EXPECT_NEAR(solutions.at(i).elevation, kExpected.at(i).elevation,
                kInchError);
    const double kSolutionDeflectionMoa =
        lob::InchToMoa(solutions.at(i).deflection, solutions.at(i).range);
    const double kExpectedDeflectionMoa =
        lob::InchToMoa(kExpected.at(i).deflection, kExpected.at(i).range);
    EXPECT_NEAR(kSolutionDeflectionMoa, kExpectedDeflectionMoa, kMoaError);
    EXPECT_NEAR(solutions.at(i).deflection, kExpected.at(i).deflection,
                kInchError);
    EXPECT_NEAR(solutions.at(i).time_of_flight, kExpected.at(i).time_of_flight,
                kTimeOfFlightError);
  }
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
TEST_F(LobCWAJTestFixture, BoatrightLeftHandSpinLeftwardWind) {
  ASSERT_NE(puut, nullptr);
  const double kLeftTwist = -1.0 * kBarrelTwist;
  constexpr double kWind = 15.0;
  constexpr lob::ClockAngleT kWindHeading = lob::ClockAngleT::kIX;
  constexpr uint16_t kVelocityError = 1;
  constexpr uint16_t kEnergyError = 5;
  constexpr double kMoaError = 0.1;
  constexpr double kInchError = 0.5;
  constexpr double kTimeOfFlightError = 0.01;
  constexpr double kAerodynamicJump = -1.015683;
  constexpr size_t kSolutionLength = 14;
  const auto kInput = puut->TwistInchesPerTurn(kLeftTwist)
                          .WindSpeedMph(kWind)
                          .WindHeading(kWindHeading)
                          .NoseLengthInch(kOgiveLength)
                          .TailLengthInch(kTailLength)
                          .BaseDiameterInch(kBaseDiameter)
                          .MeplatDiameterInch(kMeplatDiameter)
                          .OgiveRtR(kRtR)
                          .Build();
  EXPECT_NEAR(kInput.aerodynamic_jump, kAerodynamicJump, kMoaError);
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0,    150,  300,  600,  900,  1200, 1500,
      1800, 2100, 2400, 2700, 3000, 4500, 6000};
  const std::vector<lob::Output> kExpected = {
      {0, 3071, 5230, -2.00, 0.00, 0.000},
      {150, 2992, 4963, 0.42, -0.19, 0.049},
      {300, 2913, 4706, 1.87, -0.75, 0.100},
      {600, 2759, 4221, 1.63, -3.07, 0.206},
      {900, 2609, 3775, -3.18, -7.09, 0.318},
      {1200, 2464, 3366, -13.09, -12.94, 0.436},
      {1500, 2323, 2992, -28.74, -20.81, 0.562},
      {1800, 2187, 2652, -50.84, -30.86, 0.695},
      {2100, 2056, 2344, -80.21, -43.30, 0.836},
      {2400, 1929, 2064, -117.84, -58.36, 0.987},
      {2700, 1806, 1810, -164.80, -76.31, 1.148},
      {3000, 1687, 1579, -222.42, -97.46, 1.319},
      {4500, 1150, 734, -737.85, -266.99, 2.397},
      {6000, 949, 500, -1903.39, -555.46, 3.868}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  const size_t kSize = lob::Solve(kInput, kRanges, solutions);
  EXPECT_EQ(kSize, kSolutionLength);
  for (size_t i = 0; i < kSolutionLength; i++) {
    EXPECT_EQ(solutions.at(i).range, kExpected.at(i).range);
    EXPECT_NEAR(solutions.at(i).velocity, kExpected.at(i).velocity,
                kVelocityError);
    EXPECT_NEAR(solutions.at(i).energy, kExpected.at(i).energy, kEnergyError);
    const double kSolutionElevationMoa =
        lob::InchToMoa(solutions.at(i).elevation, solutions.at(i).range);
    const double kExpectedElevationMoa =
        lob::InchToMoa(kExpected.at(i).elevation, kExpected.at(i).range);
    EXPECT_NEAR(kSolutionElevationMoa, kExpectedElevationMoa, kMoaError);
    EXPECT_NEAR(solutions.at(i).elevation, kExpected.at(i).elevation,
                kInchError);
    const double kSolutionDeflectionMoa =
        lob::InchToMoa(solutions.at(i).deflection, solutions.at(i).range);
    const double kExpectedDeflectionMoa =
        lob::InchToMoa(kExpected.at(i).deflection, kExpected.at(i).range);
    EXPECT_NEAR(kSolutionDeflectionMoa, kExpectedDeflectionMoa, kMoaError);
    EXPECT_NEAR(solutions.at(i).deflection, kExpected.at(i).deflection,
                kInchError);
    EXPECT_NEAR(solutions.at(i).time_of_flight, kExpected.at(i).time_of_flight,
                kTimeOfFlightError);
  }
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
TEST_F(LobCWAJTestFixture, BoatrightRightHandSpinRightwardWind) {
  ASSERT_NE(puut, nullptr);
  constexpr double kWind = 15.0;
  constexpr lob::ClockAngleT kWindHeading = lob::ClockAngleT::kIII;
  constexpr uint16_t kVelocityError = 1;
  constexpr uint16_t kEnergyError = 5;
  constexpr double kMoaError = 0.1;
  constexpr double kInchError = 0.5;
  constexpr double kTimeOfFlightError = 0.01;
  constexpr double kAerodynamicJump = -1.015683;
  constexpr size_t kSolutionLength = 14;
  const auto kInput = puut->TwistInchesPerTurn(kBarrelTwist)
                          .WindSpeedMph(kWind)
                          .WindHeading(kWindHeading)
                          .NoseLengthInch(kOgiveLength)
                          .TailLengthInch(kTailLength)
                          .BaseDiameterInch(kBaseDiameter)
                          .MeplatDiameterInch(kMeplatDiameter)
                          .OgiveRtR(kRtR)
                          .Build();
  EXPECT_NEAR(kInput.aerodynamic_jump, kAerodynamicJump, kMoaError);
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0,    150,  300,  600,  900,  1200, 1500,
      1800, 2100, 2400, 2700, 3000, 4500, 6000};
  const std::vector<lob::Output> kExpected = {
      {0, 3071, 5230, -2.00, 0.00, 0.000},
      {150, 2992, 4963, 0.42, 0.19, 0.049},
      {300, 2913, 4706, 1.87, 0.75, 0.100},
      {600, 2759, 4221, 1.63, 3.07, 0.206},
      {900, 2609, 3775, -3.18, 7.09, 0.318},
      {1200, 2464, 3366, -13.09, 12.94, 0.436},
      {1500, 2323, 2992, -28.74, 20.81, 0.562},
      {1800, 2187, 2652, -50.84, 30.86, 0.695},
      {2100, 2056, 2344, -80.21, 43.30, 0.836},
      {2400, 1929, 2064, -117.84, 58.36, 0.987},
      {2700, 1806, 1810, -164.80, 76.31, 1.148},
      {3000, 1687, 1579, -222.42, 97.46, 1.319},
      {4500, 1150, 734, -737.85, 266.99, 2.397},
      {6000, 949, 500, -1903.39, 555.46, 3.868}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  const size_t kSize = lob::Solve(kInput, kRanges, solutions);
  EXPECT_EQ(kSize, kSolutionLength);
  for (size_t i = 0; i < kSolutionLength; i++) {
    EXPECT_EQ(solutions.at(i).range, kExpected.at(i).range);
    EXPECT_NEAR(solutions.at(i).velocity, kExpected.at(i).velocity,
                kVelocityError);
    EXPECT_NEAR(solutions.at(i).energy, kExpected.at(i).energy, kEnergyError);
    const double kSolutionElevationMoa =
        lob::InchToMoa(solutions.at(i).elevation, solutions.at(i).range);
    const double kExpectedElevationMoa =
        lob::InchToMoa(kExpected.at(i).elevation, kExpected.at(i).range);
    EXPECT_NEAR(kSolutionElevationMoa, kExpectedElevationMoa, kMoaError);
    EXPECT_NEAR(solutions.at(i).elevation, kExpected.at(i).elevation,
                kInchError);
    const double kSolutionDeflectionMoa =
        lob::InchToMoa(solutions.at(i).deflection, solutions.at(i).range);
    const double kExpectedDeflectionMoa =
        lob::InchToMoa(kExpected.at(i).deflection, kExpected.at(i).range);
    EXPECT_NEAR(kSolutionDeflectionMoa, kExpectedDeflectionMoa, kMoaError);
    EXPECT_NEAR(solutions.at(i).deflection, kExpected.at(i).deflection,
                kInchError);
    EXPECT_NEAR(solutions.at(i).time_of_flight, kExpected.at(i).time_of_flight,
                kTimeOfFlightError);
  }
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
TEST_F(LobCWAJTestFixture, BoatrightLeftHandSpinRightwardWind) {
  ASSERT_NE(puut, nullptr);
  const double kLeftTwist = -1.0 * kBarrelTwist;
  constexpr double kWind = 15.0;
  constexpr lob::ClockAngleT kWindHeading = lob::ClockAngleT::kIII;
  constexpr uint16_t kVelocityError = 1;
  constexpr uint16_t kEnergyError = 5;
  constexpr double kMoaError = 0.1;
  constexpr double kInchError = 0.5;
  constexpr double kTimeOfFlightError = 0.01;
  constexpr double kAerodynamicJump = 1.015683;
  constexpr size_t kSolutionLength = 14;
  const auto kInput = puut->TwistInchesPerTurn(kLeftTwist)
                          .WindSpeedMph(kWind)
                          .WindHeading(kWindHeading)
                          .NoseLengthInch(kOgiveLength)
                          .TailLengthInch(kTailLength)
                          .BaseDiameterInch(kBaseDiameter)
                          .MeplatDiameterInch(kMeplatDiameter)
                          .OgiveRtR(kRtR)
                          .Build();
  EXPECT_NEAR(kInput.aerodynamic_jump, kAerodynamicJump, kMoaError);
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0,    150,  300,  600,  900,  1200, 1500,
      1800, 2100, 2400, 2700, 3000, 4500, 6000};
  const std::vector<lob::Output> kExpected = {
      {0, 3071, 5230, -2.00, 0.00, 0.000},
      {150, 2992, 4963, 1.48, 0.15, 0.049},
      {300, 2913, 4706, 3.99, 0.63, 0.100},
      {600, 2759, 4221, 5.89, 2.60, 0.206},
      {900, 2609, 3775, 3.21, 6.05, 0.318},
      {1200, 2464, 3366, -4.58, 11.09, 0.436},
      {1500, 2323, 2992, -18.10, 17.87, 0.562},
      {1800, 2187, 2652, -38.07, 26.52, 0.695},
      {2100, 2056, 2344, -65.32, 37.21, 0.836},
      {2400, 1929, 2064, -100.82, 50.11, 0.987},
      {2700, 1806, 1810, -145.65, 65.44, 1.148},
      {3000, 1687, 1579, -201.15, 83.43, 1.319},
      {4500, 1150, 734, -705.94, 225.13, 2.397},
      {6000, 949, 500, -1860.74, 455.01, 3.867}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  const size_t kSize = lob::Solve(kInput, kRanges, solutions);
  EXPECT_EQ(kSize, kSolutionLength);
  for (size_t i = 0; i < kSolutionLength; i++) {
    EXPECT_EQ(solutions.at(i).range, kExpected.at(i).range);
    EXPECT_NEAR(solutions.at(i).velocity, kExpected.at(i).velocity,
                kVelocityError);
    EXPECT_NEAR(solutions.at(i).energy, kExpected.at(i).energy, kEnergyError);
    const double kSolutionElevationMoa =
        lob::InchToMoa(solutions.at(i).elevation, solutions.at(i).range);
    const double kExpectedElevationMoa =
        lob::InchToMoa(kExpected.at(i).elevation, kExpected.at(i).range);
    EXPECT_NEAR(kSolutionElevationMoa, kExpectedElevationMoa, kMoaError);
    EXPECT_NEAR(solutions.at(i).elevation, kExpected.at(i).elevation,
                kInchError);
    const double kSolutionDeflectionMoa =
        lob::InchToMoa(solutions.at(i).deflection, solutions.at(i).range);
    const double kExpectedDeflectionMoa =
        lob::InchToMoa(kExpected.at(i).deflection, kExpected.at(i).range);
    EXPECT_NEAR(kSolutionDeflectionMoa, kExpectedDeflectionMoa, kMoaError);
    EXPECT_NEAR(solutions.at(i).deflection, kExpected.at(i).deflection,
                kInchError);
    EXPECT_NEAR(solutions.at(i).time_of_flight, kExpected.at(i).time_of_flight,
                kTimeOfFlightError);
  }
}
*/

struct Shot {
  double diameter;
  double length;
  double mass;
  double nose_length;
  double tail_length;
  double base_diameter;
  double meplat_diameter;
  double ogive_rtr;
  double g1_bc;
  uint16_t velocity;
  double twist;
  double litz;
  double boatright;
};

struct CWAJParameterizedFixture : public ::testing::TestWithParam<Shot> {
  lob::Builder builder;
  void SetUp() override {
    const double kZero = 5.0;
    const double kZWind = 10.0;
    builder.BCAtmosphere(lob::AtmosphereReferenceT::kIcao)
        .BCDragFunction(lob::DragFunctionT::kG1)
        .ZeroAngleMOA(kZero)
        .WindHeading(lob::ClockAngleT::kIII)
        .WindSpeedMph(kZWind);
  }
};

TEST_P(CWAJParameterizedFixture, Boatright) {
  const Shot kShot = GetParam();
  const auto kA = builder.DiameterInch(kShot.diameter)
                      .LengthInch(kShot.length)
                      .MassGrains(kShot.mass)
                      .NoseLengthInch(kShot.nose_length)
                      .TailLengthInch(kShot.tail_length)
                      .MeplatDiameterInch(kShot.meplat_diameter)
                      .BaseDiameterInch(kShot.base_diameter)
                      .OgiveRtR(kShot.ogive_rtr)
                      .BallisticCoefficientPsi(kShot.g1_bc)
                      .InitialVelocityFps(kShot.velocity)
                      .TwistInchesPerTurn(kShot.twist)
                      .Build();

  const double kError = std::abs(kShot.boatright) * 0.10;
  EXPECT_NEAR(kA.aerodynamic_jump, kShot.boatright, kError);
}

TEST_P(CWAJParameterizedFixture, Litz) {
  const Shot kShot = GetParam();
  const auto kA = builder.DiameterInch(kShot.diameter)
                      .LengthInch(kShot.length)
                      .MassGrains(kShot.mass)
                      .BallisticCoefficientPsi(kShot.g1_bc)
                      .InitialVelocityFps(kShot.velocity)
                      .TwistInchesPerTurn(kShot.twist)
                      .Build();
  EXPECT_NEAR(kA.aerodynamic_jump, kShot.litz, .01);
}

namespace {
// This test data was originally supposed to come from the solution data
// presented in Calculating Aerodynamic Jump for Firing Point Conditions –
// Boatright & Ruiz – rev. June/2018
// However, I believe the table of solutions published to be the result of
// errant calculations so they may not match the data here. Oddly enough the
// Litz calculations are fine.

const Shot kBarnesLRXBT{0.308, 1.621, 200.0, 0.780, 0.210,  0.268, 0.0,
                        0.80,  0.549, 2900U, 10.0,  -0.324, -0.642};
const Shot kCuttingEdgeHPBT{0.308, 1.458, 180.0, 0.602, 0.240,  0.249, 0.060,
                            0.70,  0.478, 3000U, 10.0,  -0.368, -0.568};
const Shot kLehighMatchSolid{0.408, 2.085, 400.0, 1.155, 0.320,  0.326, 0.0,
                             0.78,  0.759, 2700U, 11.0,  -0.370, -0.426};
/*const Shot kSMK168{0.308, 1.215, 168.0, 0.690, 0.1401, 0.242, 0.065,
                   0.900, 0.426, 2800U,  12.0,   -0.400,  -0.424};*/
const Shot kGSCustomSP{0.338, 1.771, 232.0, 1.036, 0.346,  0.238, 0.020,
                       0.60,  0.604, 3100U, 9.0,   -0.370, -0.363};
const Shot kSMK220{0.308, 1.489, 220.0, 0.672, 0.230,  0.234, 0.070,
                   0.95,  0.607, 2700U, 10.0,  -0.384, -0.415};
const Shot kNoslerBT{0.277, 1.293, 140.0, 0.688, 0.080,  0.243, 0.00,
                     1.00,  0.440, 3100U, 9.0,   -0.390, -0.744};
const Shot kSMK80{0.224, 1.066, 80.0,  0.629, 0.135,  0.183, 0.060,
                  0.98,  0.425, 3100U, 7.0,   -0.407, -0.760};
const Shot kBergerBTFB{0.308, 1.250, 155.5, 0.825, 0.160,  0.264, 0.062,
                       0.96,  0.464, 2800U, 10.0,  -0.437, -0.824};
const Shot kBergerVLD{0.224, 0.976, 70.0,  0.471, 0.150,  0.177, 0.052,
                      0.53,  0.371, 3000U, 7.0,   -0.440, -0.711};
const Shot kHornadyBTHP{0.338, 1.724, 285.0, 0.871, 0.260,  0.265, 0.075,
                        0.82,  0.696, 2800U, 9.0,   -0.425, -0.680};
}  // namespace

INSTANTIATE_TEST_SUITE_P(
    CWAJTests, CWAJParameterizedFixture,
    ::testing::Values(
        kBarnesLRXBT,       // Barnes .308 caliber 200 gr LRXBT
        kCuttingEdgeHPBT,   // Cutting Edge .308 caliber 180 gr HPBT
        kLehighMatchSolid,  // Lehigh .408 caliber 400 gr Match Solid
        kGSCustomSP,        // GS Custom .338 caliber 232 gr SP
        kSMK220,            // Sierra .308 caliber 220 gr MatchKing
        kNoslerBT,          // Nosler .270 caliber 140 gr Ballistic Tip
        kSMK80,             // Sierra .224 caliber 80 gr MatchKing
        kBergerBTFB,        // Berger .308 caliber 155.5 gr BT FULLBORE
        kBergerVLD,         // Berger .224 caliber 70 gr VLD
        kHornadyBTHP));     // Hornady .338 caliber 285 gr BTHP Match
}  // namespace tests

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