// This file is a part of lob, an exterior ballistics calculation library
// Copyright (c) 2025  Joel Benway
// Please see end of file for extended copyright information

#include <gtest/gtest.h>

#include <array>
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

  void SetUp() override {
    ASSERT_EQ(puut, nullptr);
    puut = std::make_unique<lob::Builder>();
    ASSERT_NE(puut, nullptr);

    const float kTestBC = 0.310;
    const lob::DragFunctionT kDragFunction = lob::DragFunctionT::kG7;
    const float kTestDiameter = 0.338;
    const float kTestWeight = 250.0;
    const float kBulletLength = 1.457;
    const uint16_t kTestMuzzleVelocity = 3071;
    const float kTestZeroAngle = 6.53;  // 3.74;
    const float kTestOpticHeight = 2;

    puut->BallisticCoefficentPsi(kTestBC)
        .BCDragFunction(kDragFunction)
        .BCAtmosphere(lob::AtmosphereReferenceT::kIcao)
        .DiameterInch(kTestDiameter)
        .MassGrains(kTestWeight)
        .LengthInch(kBulletLength)
        .InitialVelocityFps(kTestMuzzleVelocity)
        .ZeroAngleMOA(kTestZeroAngle)
        .OpticHeightInches(kTestOpticHeight);
  }

  void TearDown() override { puut.reset(); }
};

TEST_F(LobSpinTestFixture, ZeroAngleSearch) {
  ASSERT_NE(puut, nullptr);
  auto input1 = puut->Build();
  const float kZeroRange = 300.0F;
  auto input2 = puut->ZeroAngleMOA(std::numeric_limits<double>::quiet_NaN())
                    .ZeroDistanceYds(kZeroRange)
                    .Build();
  const float kError = 0.01F;
  EXPECT_NEAR(input1.zero_angle, input2.zero_angle, kError);
}

TEST_F(LobSpinTestFixture, GetSpeedOfSoundFps) {
  ASSERT_NE(puut, nullptr);
  const auto kInput = puut->Build();
  const float kExpectedFps = 1116.45;
  const float kError = 0.001;
  EXPECT_NEAR(kInput.speed_of_sound, kExpectedFps, kError);
}

TEST_F(LobSpinTestFixture, SolveWithoutSpin) {
  ASSERT_NE(puut, nullptr);
  constexpr uint16_t kTestStepSize = 100;
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
      {0, 3071, 5230, -2, 0, 0},
      {150, 2992, 4965, 0.95, 0, 0.049},
      {300, 2914, 4709, 2.93, 0, 0.1},
      {600, 2761, 4227, 3.76, 0, 0.206},
      {900, 2612, 3783, 0.03, 0, 0.318},
      {1200, 2467, 3376, -8.8, 0, 0.436},
      {1500, 2327, 3004, -23.35, 0, 0.561},
      {1800, 2192, 2665, -44.32, 0, 0.694},
      {2100, 2062, 2357, -72.53, 0, 0.835},
      {2400, 1936, 2078, -108.94, 0, 0.985},
      {2700, 1814, 1824, -154.62, 0, 1.145},
      {3000, 1695, 1593, -210.93, 0, 1.316},
      {4500, 1159, 745, -717.03, 0, 2.388},
      {6000, 952, 503, -1865.85, 0, 3.851}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  const lob::Options kOptions = {0, 0, lob::kNaN, kTestStepSize};
  lob::Solve(kInput, &kRanges, &solutions, kOptions);
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

TEST_F(LobSpinTestFixture, RightHandSpinDrift) {
  ASSERT_NE(puut, nullptr);
  constexpr float kBarrelTwist = 11;
  constexpr uint16_t kTestStepSize = 100;
  constexpr uint16_t kVelocityError = 1;
  constexpr uint16_t kEnergyError = 5;
  constexpr double kMoaError = 0.1;
  constexpr double kInchError = 0.5;
  constexpr double kTimeOfFlightError = 0.01;
  constexpr size_t kSolutionLength = 14;
  const auto kInput = puut->TwistInchesPerTurn(kBarrelTwist).Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0,    150,  300,  600,  900,  1200, 1500,
      1800, 2100, 2400, 2700, 3000, 4500, 6000};
  const std::vector<lob::Output> kExpected = {
      {0, 3071, 5230, -2, 0, 0},
      {150, 2992, 4965, 0.95, 0.02, 0.049},
      {300, 2914, 4709, 2.93, 0.06, 0.1},
      {600, 2761, 4227, 3.76, 0.24, 0.206},
      {900, 2612, 3783, 0.03, 0.53, 0.318},
      {1200, 2467, 3376, -8.8, 0.94, 0.436},
      {1500, 2327, 3004, -23.35, 1.49, 0.561},
      {1800, 2192, 2665, -44.32, 2.2, 0.694},
      {2100, 2062, 2357, -72.53, 3.09, 0.835},
      {2400, 1936, 2078, -108.94, 4.19, 0.985},
      {2700, 1814, 1824, -154.62, 5.51, 1.145},
      {3000, 1695, 1593, -210.93, 7.12, 1.316},
      {4500, 1159, 745, -717.03, 21.15, 2.388},
      {6000, 952, 503, -1865.85, 50.74, 3.851}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  const lob::Options kOptions = {0, 0, lob::kNaN, kTestStepSize};
  lob::Solve(kInput, &kRanges, &solutions, kOptions);
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

TEST_F(LobSpinTestFixture, RightHandSpinDriftFast) {
  ASSERT_NE(puut, nullptr);
  constexpr float kBarrelTwist = 9.375;
  constexpr uint16_t kTestStepSize = 100;
  constexpr uint16_t kVelocityError = 1;
  constexpr uint16_t kEnergyError = 5;
  constexpr double kMoaError = 0.1;
  constexpr double kInchError = 0.5;
  constexpr double kTimeOfFlightError = 0.01;
  constexpr size_t kSolutionLength = 14;
  const auto kInput = puut->TwistInchesPerTurn(kBarrelTwist).Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0,    150,  300,  600,  900,  1200, 1500,
      1800, 2100, 2400, 2700, 3000, 4500, 6000};
  const std::vector<lob::Output> kExpected = {
      {0, 3071, 5230, -2, 0, 0},
      {150, 2992, 4965, 0.95, 0.02, 0.049},
      {300, 2914, 4709, 2.93, 0.08, 0.1},
      {600, 2761, 4227, 3.76, 0.3, 0.206},
      {900, 2612, 3783, 0.03, 0.66, 0.318},
      {1200, 2467, 3376, -8.8, 1.17, 0.436},
      {1500, 2327, 3004, -23.35, 1.86, 0.561},
      {1800, 2192, 2665, -44.32, 2.74, 0.694},
      {2100, 2062, 2357, -72.53, 3.85, 0.835},
      {2400, 1936, 2078, -108.94, 5.21, 0.985},
      {2700, 1814, 1824, -154.62, 6.87, 1.145},
      {3000, 1695, 1593, -210.93, 8.86, 1.316},
      {4500, 1159, 745, -717.03, 26.34, 2.388},
      {6000, 952, 503, -1865.85, 63.19, 3.851}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  const lob::Options kOptions = {0, 0, lob::kNaN, kTestStepSize};
  lob::Solve(kInput, &kRanges, &solutions, kOptions);
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

TEST_F(LobSpinTestFixture, LeftHandSpinDrift) {
  ASSERT_NE(puut, nullptr);
  constexpr float kBarrelTwist = -11;
  constexpr uint16_t kTestStepSize = 100;
  constexpr uint16_t kVelocityError = 1;
  constexpr uint16_t kEnergyError = 5;
  constexpr double kMoaError = 0.1;
  constexpr double kInchError = 0.5;
  constexpr double kTimeOfFlightError = 0.01;
  constexpr size_t kSolutionLength = 14;
  const auto kInput = puut->TwistInchesPerTurn(kBarrelTwist).Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0,    150,  300,  600,  900,  1200, 1500,
      1800, 2100, 2400, 2700, 3000, 4500, 6000};
  const std::vector<lob::Output> kExpected = {
      {0, 3071, 5230, -2, 0, 0},
      {150, 2992, 4965, 0.95, -0.02, 0.049},
      {300, 2914, 4709, 2.93, -0.06, 0.1},
      {600, 2761, 4227, 3.76, -0.24, 0.206},
      {900, 2612, 3783, 0.03, -0.53, 0.318},
      {1200, 2467, 3376, -8.8, -0.94, 0.436},
      {1500, 2327, 3004, -23.35, -1.49, 0.561},
      {1800, 2192, 2665, -44.32, -2.2, 0.694},
      {2100, 2062, 2357, -72.53, -3.09, 0.835},
      {2400, 1936, 2078, -108.94, -4.19, 0.985},
      {2700, 1814, 1824, -154.62, -5.51, 1.145},
      {3000, 1695, 1593, -210.93, -7.12, 1.316},
      {4500, 1159, 745, -717.03, -21.15, 2.388},
      {6000, 952, 503, -1865.85, -50.74, 3.851}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  const lob::Options kOptions = {0, 0, lob::kNaN, kTestStepSize};
  lob::Solve(kInput, &kRanges, &solutions, kOptions);
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

TEST_F(LobSpinTestFixture, LeftHandSpinDriftFast) {
  ASSERT_NE(puut, nullptr);
  constexpr float kBarrelTwist = -9.375;
  constexpr uint16_t kTestStepSize = 100;
  constexpr uint16_t kVelocityError = 1;
  constexpr uint16_t kEnergyError = 5;
  constexpr double kMoaError = 0.1;
  constexpr double kInchError = 0.5;
  constexpr double kTimeOfFlightError = 0.01;
  constexpr size_t kSolutionLength = 14;
  const auto kInput = puut->TwistInchesPerTurn(kBarrelTwist).Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0,    150,  300,  600,  900,  1200, 1500,
      1800, 2100, 2400, 2700, 3000, 4500, 6000};
  const std::vector<lob::Output> kExpected = {
      {0, 3071, 5230, -2, 0, 0},
      {150, 2992, 4965, 0.95, -0.02, 0.049},
      {300, 2914, 4709, 2.93, -0.08, 0.1},
      {600, 2761, 4227, 3.76, -0.3, 0.206},
      {900, 2612, 3783, 0.03, -0.66, 0.318},
      {1200, 2467, 3376, -8.8, -1.17, 0.436},
      {1500, 2327, 3004, -23.35, -1.86, 0.561},
      {1800, 2192, 2665, -44.32, -2.74, 0.694},
      {2100, 2062, 2357, -72.53, -3.85, 0.835},
      {2400, 1936, 2078, -108.94, -5.21, 0.985},
      {2700, 1814, 1824, -154.62, -6.87, 1.145},
      {3000, 1695, 1593, -210.93, -8.86, 1.316},
      {4500, 1159, 745, -717.03, -26.34, 2.388},
      {6000, 952, 503, -1865.85, -63.19, 3.851}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  const lob::Options kOptions = {0, 0, lob::kNaN, kTestStepSize};
  lob::Solve(kInput, &kRanges, &solutions, kOptions);
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

TEST_F(LobSpinTestFixture, RightHandSpinDriftWithLitzAeroJump1) {
  ASSERT_NE(puut, nullptr);
  constexpr float kBarrelTwist = 11;
  constexpr float kWind = 15;
  constexpr lob::ClockAngleT kWindHeading = lob::ClockAngleT::kIX;
  constexpr uint16_t kTestStepSize = 100;
  constexpr uint16_t kVelocityError = 1;
  constexpr uint16_t kEnergyError = 5;
  constexpr double kMoaError = 0.1;
  constexpr double kInchError = 0.5;
  constexpr double kTimeOfFlightError = 0.01;
  constexpr size_t kSolutionLength = 14;
  const auto kInput = puut->TwistInchesPerTurn(kBarrelTwist)
                          .WindSpeedMph(kWind)
                          .WindHeading(kWindHeading)
                          .Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0,    150,  300,  600,  900,  1200, 1500,
      1800, 2100, 2400, 2700, 3000, 4500, 6000};
  const std::vector<lob::Output> kExpected = {
      {0, 3071, 5230, -2, 0, 0},
      {150, 2992, 4965, 1.3, -0.15, 0.049},
      {300, 2914, 4709, 3.62, -0.62, 0.1},
      {600, 2761, 4227, 5.15, -2.58, 0.206},
      {900, 2612, 3783, 2.1, -5.99, 0.318},
      {1200, 2467, 3376, -6.04, -10.99, 0.436},
      {1500, 2327, 3004, -19.9, -17.7, 0.561},
      {1800, 2192, 2665, -40.17, -26.27, 0.694},
      {2100, 2062, 2357, -67.69, -36.84, 0.835},
      {2400, 1936, 2078, -103.41, -49.61, 0.985},
      {2700, 1814, 1824, -148.4, -64.76, 1.145},
      {3000, 1695, 1593, -204.01, -82.56, 1.316},
      {4500, 1159, 745, -706.66, -222.33, 2.388},
      {6000, 952, 503, -1852.03, -450.26, 3.851}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  const lob::Options kOptions = {0, 0, lob::kNaN, kTestStepSize};
  lob::Solve(kInput, &kRanges, &solutions, kOptions);
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

TEST_F(LobSpinTestFixture, RightHandSpinDriftWithLitzAeroJump2) {
  ASSERT_NE(puut, nullptr);
  constexpr float kBarrelTwist = 11;
  constexpr float kWind = 15;
  constexpr lob::ClockAngleT kWindHeading = lob::ClockAngleT::kIII;
  constexpr uint16_t kTestStepSize = 100;
  constexpr uint16_t kVelocityError = 1;
  constexpr uint16_t kEnergyError = 5;
  constexpr double kMoaError = 0.1;
  constexpr double kInchError = 0.5;
  constexpr double kTimeOfFlightError = 0.01;
  constexpr size_t kSolutionLength = 14;
  const auto kInput = puut->TwistInchesPerTurn(kBarrelTwist)
                          .WindSpeedMph(kWind)
                          .WindHeading(kWindHeading)
                          .Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0,    150,  300,  600,  900,  1200, 1500,
      1800, 2100, 2400, 2700, 3000, 4500, 6000};
  const std::vector<lob::Output> kExpected = {
      {0, 3071, 5230, -2, 0, 0},
      {150, 2992, 4965, 0.61, 0.19, 0.049},
      {300, 2914, 4709, 2.24, 0.75, 0.1},
      {600, 2761, 4227, 2.38, 3.06, 0.206},
      {900, 2612, 3783, -2.04, 7.05, 0.318},
      {1200, 2467, 3376, -11.57, 12.87, 0.436},
      {1500, 2327, 3004, -26.81, 20.69, 0.561},
      {1800, 2192, 2665, -48.47, 30.67, 0.694},
      {2100, 2062, 2357, -77.37, 43.03, 0.835},
      {2400, 1936, 2078, -114.47, 57.98, 0.985},
      {2700, 1814, 1824, -160.84, 75.79, 1.145},
      {3000, 1695, 1593, -217.84, 96.79, 1.316},
      {4500, 1159, 745, -727.39, 264.63, 2.388},
      {6000, 952, 503, -1879.67, 551.74, 3.851}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  const lob::Options kOptions = {0, 0, lob::kNaN, kTestStepSize};
  lob::Solve(kInput, &kRanges, &solutions, kOptions);
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