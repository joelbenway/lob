// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <vector>

#include "lob/lob.hpp"

namespace tests {

struct LobSpinTestFixture : public testing::Test {
  // Unit under test
  std::unique_ptr<lob::Builder> puut;

  LobSpinTestFixture() : puut(nullptr) {}

  const double kZeroDistance = 300;
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

TEST_F(LobSpinTestFixture, ZeroAngleSearch) {
  ASSERT_NE(puut, nullptr);
  auto input1 = puut->Build();
  auto input2 = puut->ZeroAngleMOA(std::numeric_limits<double>::quiet_NaN())
                    .ZeroDistanceYds(kZeroDistance)
                    .Build();
  const double kError = 0.01;
  EXPECT_NEAR(input1.zero_angle, input2.zero_angle, kError);
}

TEST_F(LobSpinTestFixture, GetSpeedOfSoundFps) {
  ASSERT_NE(puut, nullptr);
  const auto kInput = puut->Build();
  const double kExpectedFps = 1116.45;
  const double kError = 0.001;
  EXPECT_NEAR(kInput.speed_of_sound, kExpectedFps, kError);
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
TEST_F(LobSpinTestFixture, SolveWithoutSpin) {
  ASSERT_NE(puut, nullptr);
  constexpr uint16_t kVelocityError = 1;
  constexpr uint16_t kEnergyError = 5;
  constexpr double kMoaError = 0.1;
  constexpr double kInchError = 0.5;
  constexpr double kTimeOfFlightError = 0.01;
  constexpr size_t kSolutionLength = 14;
  const auto kInput = puut->Build();
  EXPECT_TRUE(std::isnan(kInput.spindrift_factor));
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
TEST_F(LobSpinTestFixture, LitzRightHandSpinDrift) {
  ASSERT_NE(puut, nullptr);
  const double kBarrelTwist = 11.0;
  constexpr uint16_t kVelocityError = 1;
  constexpr uint16_t kEnergyError = 5;
  constexpr double kMoaError = 0.1;
  constexpr double kInchError = 0.5;
  constexpr double kTimeOfFlightError = 0.01;
  constexpr size_t kSolutionLength = 14;
  const auto kInput = puut->TwistInchesPerTurn(kBarrelTwist).Build();
  EXPECT_TRUE(std::isnan(kInput.spindrift_factor));
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0,    150,  300,  600,  900,  1200, 1500,
      1800, 2100, 2400, 2700, 3000, 4500, 6000};
  const std::vector<lob::Output> kExpected = {
      {0, 3071, 5230, -2.00, 0.00, 0.000},
      {150, 2992, 4963, 0.95, 0.02, 0.049},
      {300, 2913, 4706, 2.93, 0.06, 0.100},
      {600, 2759, 4221, 3.76, 0.23, 0.206},
      {900, 2609, 3775, 0.02, 0.52, 0.318},
      {1200, 2464, 3366, -8.84, 0.93, 0.436},
      {1500, 2323, 2992, -23.42, 1.47, 0.562},
      {1800, 2187, 2652, -44.45, 2.17, 0.695},
      {2100, 2056, 2344, -72.77, 3.05, 0.836},
      {2400, 1929, 2064, -109.33, 4.12, 0.987},
      {2700, 1806, 1810, -155.23, 5.44, 1.148},
      {3000, 1687, 1579, -211.78, 7.02, 1.319},
      {4500, 1150, 734, -721.84, 20.93, 2.397},
      {6000, 949, 500, -1882.01, 50.22, 3.867}};

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
TEST_F(LobSpinTestFixture, LitzRightHandSpinDriftFast) {
  ASSERT_NE(puut, nullptr);
  constexpr double kBarrelTwist = 9.375;
  constexpr uint16_t kVelocityError = 1;
  constexpr uint16_t kEnergyError = 5;
  constexpr double kMoaError = 0.1;
  constexpr double kInchError = 0.5;
  constexpr double kTimeOfFlightError = 0.01;
  constexpr size_t kSolutionLength = 14;
  const auto kInput = puut->TwistInchesPerTurn(kBarrelTwist).Build();
  EXPECT_TRUE(std::isnan(kInput.spindrift_factor));
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0,    150,  300,  600,  900,  1200, 1500,
      1800, 2100, 2400, 2700, 3000, 4500, 6000};
  const std::vector<lob::Output> kExpected = {
      {0, 3071, 5230, -2.00, 0.00, 0.000},
      {150, 2992, 4963, 0.95, 0.02, 0.049},
      {300, 2913, 4706, 2.93, 0.08, 0.100},
      {600, 2759, 4221, 3.76, 0.29, 0.206},
      {900, 2609, 3775, 0.02, 0.64, 0.318},
      {1200, 2464, 3366, -8.84, 1.15, 0.436},
      {1500, 2323, 2992, -23.42, 1.83, 0.562},
      {1800, 2187, 2652, -44.45, 2.70, 0.695},
      {2100, 2056, 2344, -72.77, 3.79, 0.836},
      {2400, 1929, 2064, -109.33, 5.13, 0.987},
      {2700, 1806, 1810, -155.23, 6.76, 1.148},
      {3000, 1687, 1579, -211.78, 8.72, 1.319},
      {4500, 1150, 734, -721.84, 26.01, 2.397},
      {6000, 949, 500, -1882.01, 62.42, 3.867}};

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
TEST_F(LobSpinTestFixture, LitzLeftHandSpinDrift) {
  ASSERT_NE(puut, nullptr);
  constexpr double kBarrelTwist = -11.0;
  constexpr uint16_t kVelocityError = 1;
  constexpr uint16_t kEnergyError = 5;
  constexpr double kMoaError = 0.1;
  constexpr double kInchError = 0.5;
  constexpr double kTimeOfFlightError = 0.01;
  constexpr size_t kSolutionLength = 14;
  const auto kInput = puut->TwistInchesPerTurn(kBarrelTwist).Build();
  EXPECT_TRUE(std::isnan(kInput.spindrift_factor));
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0,    150,  300,  600,  900,  1200, 1500,
      1800, 2100, 2400, 2700, 3000, 4500, 6000};
  const std::vector<lob::Output> kExpected = {
      {0, 3071, 5230, -2.00, 0.00, 0.000},
      {150, 2992, 4963, 0.95, -0.02, 0.049},
      {300, 2913, 4706, 2.93, -0.06, 0.100},
      {600, 2759, 4221, 3.76, -0.23, 0.206},
      {900, 2609, 3775, 0.02, -0.52, 0.318},
      {1200, 2464, 3366, -8.84, -0.93, 0.436},
      {1500, 2323, 2992, -23.42, -1.47, 0.562},
      {1800, 2187, 2652, -44.45, -2.17, 0.695},
      {2100, 2056, 2344, -72.77, -3.05, 0.836},
      {2400, 1929, 2064, -109.33, -4.12, 0.987},
      {2700, 1806, 1810, -155.23, -5.44, 1.148},
      {3000, 1687, 1579, -211.78, -7.02, 1.319},
      {4500, 1150, 734, -721.84, -20.93, 2.397},
      {6000, 949, 500, -1882.01, -50.22, 3.867}};

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
TEST_F(LobSpinTestFixture, LitzLeftHandSpinDriftFast) {
  ASSERT_NE(puut, nullptr);
  constexpr double kBarrelTwist = -9.375;
  constexpr uint16_t kVelocityError = 1;
  constexpr uint16_t kEnergyError = 5;
  constexpr double kMoaError = 0.1;
  constexpr double kInchError = 0.5;
  constexpr double kTimeOfFlightError = 0.01;
  constexpr size_t kSolutionLength = 14;
  const auto kInput = puut->TwistInchesPerTurn(kBarrelTwist).Build();
  EXPECT_TRUE(std::isnan(kInput.spindrift_factor));
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0,    150,  300,  600,  900,  1200, 1500,
      1800, 2100, 2400, 2700, 3000, 4500, 6000};
  const std::vector<lob::Output> kExpected = {
      {0, 3071, 5230, -2.00, 0.00, 0.000},
      {150, 2992, 4963, 0.95, -0.02, 0.049},
      {300, 2913, 4706, 2.93, -0.08, 0.100},
      {600, 2759, 4221, 3.76, -0.29, 0.206},
      {900, 2609, 3775, 0.02, -0.64, 0.318},
      {1200, 2464, 3366, -8.84, -1.15, 0.436},
      {1500, 2323, 2992, -23.42, -1.83, 0.562},
      {1800, 2187, 2652, -44.45, -2.70, 0.695},
      {2100, 2056, 2344, -72.77, -3.79, 0.836},
      {2400, 1929, 2064, -109.33, -5.13, 0.987},
      {2700, 1806, 1810, -155.23, -6.76, 1.148},
      {3000, 1687, 1579, -211.78, -8.72, 1.319},
      {4500, 1150, 734, -721.84, -26.01, 2.397},
      {6000, 949, 500, -1882.01, -62.42, 3.867}};

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
TEST_F(LobSpinTestFixture, BoatrightRightHandSpinDrift) {
  ASSERT_NE(puut, nullptr);
  const double kBarrelTwist = 11.0;
  constexpr uint16_t kVelocityError = 1;
  constexpr uint16_t kEnergyError = 5;
  constexpr double kMoaError = 0.1;
  constexpr double kInchError = 0.5;
  constexpr double kTimeOfFlightError = 0.01;
  constexpr size_t kSolutionLength = 14;
  const auto kInput = puut->TwistInchesPerTurn(kBarrelTwist)
                          .NoseLengthInch(kOgiveLength)
                          .TailLengthInch(kTailLength)
                          .BaseDiameterInch(kBaseDiameter)
                          .MeplatDiameterInch(kMeplatDiameter)
                          .OgiveRtR(kRtR)
                          .Build();
  EXPECT_FALSE(std::isnan(kInput.spindrift_factor));
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0,    150,  300,  600,  900,  1200, 1500,
      1800, 2100, 2400, 2700, 3000, 4500, 6000};
  const std::vector<lob::Output> kExpected = {
      {0, 3071, 5230, -2.00, 0.00, 0.000},
      {150, 2992, 4963, 0.95, 0.02, 0.049},
      {300, 2913, 4706, 2.93, 0.06, 0.100},
      {600, 2759, 4221, 3.76, 0.23, 0.206},
      {900, 2609, 3775, 0.02, 0.52, 0.318},
      {1200, 2464, 3366, -8.84, 0.93, 0.436},
      {1500, 2323, 2992, -23.42, 1.47, 0.562},
      {1800, 2187, 2652, -44.45, 2.17, 0.695},
      {2100, 2056, 2344, -72.77, 3.05, 0.836},
      {2400, 1929, 2064, -109.33, 4.12, 0.987},
      {2700, 1806, 1810, -155.23, 5.44, 1.148},
      {3000, 1687, 1579, -211.78, 7.02, 1.319},
      {4500, 1150, 734, -721.84, 20.93, 2.397},
      {6000, 949, 500, -1882.01, 50.22, 3.867}};

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
TEST_F(LobSpinTestFixture, BoatrightRightHandSpinDriftFast) {
  ASSERT_NE(puut, nullptr);
  constexpr double kBarrelTwist = 9.375;
  constexpr uint16_t kVelocityError = 1;
  constexpr uint16_t kEnergyError = 5;
  constexpr double kMoaError = 0.1;
  constexpr double kInchError = 0.5;
  constexpr double kTimeOfFlightError = 0.01;
  constexpr size_t kSolutionLength = 14;
  const auto kInput = puut->TwistInchesPerTurn(kBarrelTwist)
                          .NoseLengthInch(kOgiveLength)
                          .TailLengthInch(kTailLength)
                          .BaseDiameterInch(kBaseDiameter)
                          .MeplatDiameterInch(kMeplatDiameter)
                          .OgiveRtR(kRtR)
                          .Build();
  EXPECT_FALSE(std::isnan(kInput.spindrift_factor));
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0,    150,  300,  600,  900,  1200, 1500,
      1800, 2100, 2400, 2700, 3000, 4500, 6000};
  const std::vector<lob::Output> kExpected = {
      {0, 3071, 5230, -2.00, 0.00, 0.000},
      {150, 2992, 4963, 0.95, 0.02, 0.049},
      {300, 2913, 4706, 2.93, 0.08, 0.100},
      {600, 2759, 4221, 3.76, 0.29, 0.206},
      {900, 2609, 3775, 0.02, 0.64, 0.318},
      {1200, 2464, 3366, -8.84, 1.15, 0.436},
      {1500, 2323, 2992, -23.42, 1.83, 0.562},
      {1800, 2187, 2652, -44.45, 2.70, 0.695},
      {2100, 2056, 2344, -72.77, 3.79, 0.836},
      {2400, 1929, 2064, -109.33, 5.13, 0.987},
      {2700, 1806, 1810, -155.23, 6.76, 1.148},
      {3000, 1687, 1579, -211.78, 8.72, 1.319},
      {4500, 1150, 734, -721.84, 26.01, 2.397},
      {6000, 949, 500, -1882.01, 62.42, 3.867}};

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
TEST_F(LobSpinTestFixture, BoatrightLeftHandSpinDrift) {
  ASSERT_NE(puut, nullptr);
  constexpr double kBarrelTwist = -11.0;
  constexpr uint16_t kVelocityError = 1;
  constexpr uint16_t kEnergyError = 5;
  constexpr double kMoaError = 0.1;
  constexpr double kInchError = 0.5;
  constexpr double kTimeOfFlightError = 0.01;
  constexpr size_t kSolutionLength = 14;
  const auto kInput = puut->TwistInchesPerTurn(kBarrelTwist)
                          .NoseLengthInch(kOgiveLength)
                          .TailLengthInch(kTailLength)
                          .BaseDiameterInch(kBaseDiameter)
                          .MeplatDiameterInch(kMeplatDiameter)
                          .OgiveRtR(kRtR)
                          .Build();
  EXPECT_FALSE(std::isnan(kInput.spindrift_factor));
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0,    150,  300,  600,  900,  1200, 1500,
      1800, 2100, 2400, 2700, 3000, 4500, 6000};
  const std::vector<lob::Output> kExpected = {
      {0, 3071, 5230, -2.00, 0.00, 0.000},
      {150, 2992, 4963, 0.95, -0.02, 0.049},
      {300, 2913, 4706, 2.93, -0.06, 0.100},
      {600, 2759, 4221, 3.76, -0.23, 0.206},
      {900, 2609, 3775, 0.02, -0.52, 0.318},
      {1200, 2464, 3366, -8.84, -0.93, 0.436},
      {1500, 2323, 2992, -23.42, -1.47, 0.562},
      {1800, 2187, 2652, -44.45, -2.17, 0.695},
      {2100, 2056, 2344, -72.77, -3.05, 0.836},
      {2400, 1929, 2064, -109.33, -4.12, 0.987},
      {2700, 1806, 1810, -155.23, -5.44, 1.148},
      {3000, 1687, 1579, -211.78, -7.02, 1.319},
      {4500, 1150, 734, -721.84, -20.93, 2.397},
      {6000, 949, 500, -1882.01, -50.22, 3.867}};

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
TEST_F(LobSpinTestFixture, BoatrightLeftHandSpinDriftFast) {
  ASSERT_NE(puut, nullptr);
  constexpr double kBarrelTwist = -9.375;
  constexpr uint16_t kVelocityError = 1;
  constexpr uint16_t kEnergyError = 5;
  constexpr double kMoaError = 0.1;
  constexpr double kInchError = 0.5;
  constexpr double kTimeOfFlightError = 0.01;
  constexpr size_t kSolutionLength = 14;
  const auto kInput = puut->TwistInchesPerTurn(kBarrelTwist)
                          .NoseLengthInch(kOgiveLength)
                          .TailLengthInch(kTailLength)
                          .BaseDiameterInch(kBaseDiameter)
                          .MeplatDiameterInch(kMeplatDiameter)
                          .OgiveRtR(kRtR)
                          .Build();
  EXPECT_FALSE(std::isnan(kInput.spindrift_factor));
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0,    150,  300,  600,  900,  1200, 1500,
      1800, 2100, 2400, 2700, 3000, 4500, 6000};
  const std::vector<lob::Output> kExpected = {
      {0, 3071, 5230, -2.00, 0.00, 0.000},
      {150, 2992, 4963, 0.95, -0.02, 0.049},
      {300, 2913, 4706, 2.93, -0.08, 0.100},
      {600, 2759, 4221, 3.76, -0.29, 0.206},
      {900, 2609, 3775, 0.02, -0.64, 0.318},
      {1200, 2464, 3366, -8.84, -1.15, 0.436},
      {1500, 2323, 2992, -23.42, -1.83, 0.562},
      {1800, 2187, 2652, -44.45, -2.70, 0.695},
      {2100, 2056, 2344, -72.77, -3.79, 0.836},
      {2400, 1929, 2064, -109.33, -5.13, 0.987},
      {2700, 1806, 1810, -155.23, -6.76, 1.148},
      {3000, 1687, 1579, -211.78, -8.72, 1.319},
      {4500, 1150, 734, -721.84, -26.01, 2.397},
      {6000, 949, 500, -1882.01, -62.42, 3.867}};

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