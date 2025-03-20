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

    const float kTestBC = 0.310F;
    const lob::DragFunctionT kDragFunction = lob::DragFunctionT::kG7;
    const float kTestDiameter = 0.338F;
    const float kTestWeight = 250.0F;
    const float kBulletLength = 1.457F;
    const uint16_t kTestMuzzleVelocity = 3071;
    const float kTestZeroAngle = 6.53F;
    const float kTestOpticHeight = 2.0F;

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
  const float kExpectedFps = 1116.45F;
  const float kError = 0.001F;
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
      {0, 3071, 5230, -2.00F, 0.00F, 0.000F},
      {150, 2992, 4965, 0.95F, 0.00F, 0.049F},
      {300, 2914, 4709, 2.93F, 0.00F, 0.100F},
      {600, 2761, 4227, 3.76F, 0.00F, 0.206F},
      {900, 2612, 3783, 0.03F, 0.00F, 0.318F},
      {1200, 2467, 3376, -8.80F, 0.00F, 0.436F},
      {1500, 2327, 3004, -23.35F, 0.00F, 0.561F},
      {1800, 2192, 2665, -44.32F, 0.00F, 0.694F},
      {2100, 2062, 2357, -72.53F, 0.00F, 0.835F},
      {2400, 1936, 2078, -108.94F, 0.00F, 0.985F},
      {2700, 1814, 1824, -154.62F, 0.00F, 1.145F},
      {3000, 1695, 1593, -210.93F, 0.00F, 1.316F},
      {4500, 1159, 745, -717.03F, 0.00F, 2.388F},
      {6000, 952, 503, -1865.85F, 0.00F, 3.851F}};

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
  constexpr float kBarrelTwist = 11.0F;
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
      {0, 3071, 5230, -2.00F, 0.00F, 0.000F},
      {150, 2992, 4965, 0.95F, 0.02F, 0.049F},
      {300, 2914, 4709, 2.93F, 0.06F, 0.100F},
      {600, 2761, 4227, 3.76F, 0.24F, 0.206F},
      {900, 2612, 3783, 0.03F, 0.53F, 0.318F},
      {1200, 2467, 3376, -8.80F, 0.94F, 0.436F},
      {1500, 2327, 3004, -23.35F, 1.49F, 0.561F},
      {1800, 2192, 2665, -44.32F, 2.20F, 0.694F},
      {2100, 2062, 2357, -72.53F, 3.09F, 0.835F},
      {2400, 1936, 2078, -108.94F, 4.19F, 0.985F},
      {2700, 1814, 1824, -154.62F, 5.51F, 1.145F},
      {3000, 1695, 1593, -210.93F, 7.12F, 1.316F},
      {4500, 1159, 745, -717.03F, 21.15F, 2.388F},
      {6000, 952, 503, -1865.85F, 50.74F, 3.851F}};

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
  constexpr float kBarrelTwist = 9.375F;
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
      {0, 3071, 5230, -2.00F, 0.00F, 0.000F},
      {150, 2992, 4965, 0.95F, 0.02F, 0.049F},
      {300, 2914, 4709, 2.93F, 0.08F, 0.100F},
      {600, 2761, 4227, 3.76F, 0.30F, 0.206F},
      {900, 2612, 3783, 0.03F, 0.66F, 0.318F},
      {1200, 2467, 3376, -8.80F, 1.17F, 0.436F},
      {1500, 2327, 3004, -23.35F, 1.86F, 0.561F},
      {1800, 2192, 2665, -44.32F, 2.74F, 0.694F},
      {2100, 2062, 2357, -72.53F, 3.85F, 0.835F},
      {2400, 1936, 2078, -108.94F, 5.21F, 0.985F},
      {2700, 1814, 1824, -154.62F, 6.87F, 1.145F},
      {3000, 1695, 1593, -210.93F, 8.86F, 1.316F},
      {4500, 1159, 745, -717.03F, 26.34F, 2.388F},
      {6000, 952, 503, -1865.85F, 63.19F, 3.851F}};

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
  constexpr float kBarrelTwist = -11.0F;
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
      {0, 3071, 5230, -2.00F, 0.00F, 0.000F},
      {150, 2992, 4965, 0.95F, -0.02F, 0.049F},
      {300, 2914, 4709, 2.93F, -0.06F, 0.100F},
      {600, 2761, 4227, 3.76F, -0.24F, 0.206F},
      {900, 2612, 3783, 0.03F, -0.53F, 0.318F},
      {1200, 2467, 3376, -8.80F, -0.94F, 0.436F},
      {1500, 2327, 3004, -23.35F, -1.49F, 0.561F},
      {1800, 2192, 2665, -44.32F, -2.20F, 0.694F},
      {2100, 2062, 2357, -72.53F, -3.09F, 0.835F},
      {2400, 1936, 2078, -108.94F, -4.19F, 0.985F},
      {2700, 1814, 1824, -154.62F, -5.51F, 1.145F},
      {3000, 1695, 1593, -210.93F, -7.12F, 1.316F},
      {4500, 1159, 745, -717.03F, -21.15F, 2.388F},
      {6000, 952, 503, -1865.85F, -50.74F, 3.851F}};

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
  constexpr float kBarrelTwist = -9.375F;
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
      {0, 3071, 5230, -2.00F, 0.00F, 0.000F},
      {150, 2992, 4965, 0.95F, -0.02F, 0.049F},
      {300, 2914, 4709, 2.93F, -0.08F, 0.100F},
      {600, 2761, 4227, 3.76F, -0.30F, 0.206F},
      {900, 2612, 3783, 0.03F, -0.66F, 0.318F},
      {1200, 2467, 3376, -8.80F, -1.17F, 0.436F},
      {1500, 2327, 3004, -23.35F, -1.86F, 0.561F},
      {1800, 2192, 2665, -44.32F, -2.74F, 0.694F},
      {2100, 2062, 2357, -72.53F, -3.85F, 0.835F},
      {2400, 1936, 2078, -108.94F, -5.21F, 0.985F},
      {2700, 1814, 1824, -154.62F, -6.87F, 1.145F},
      {3000, 1695, 1593, -210.93F, -8.86F, 1.316F},
      {4500, 1159, 745, -717.03F, -26.34F, 2.388F},
      {6000, 952, 503, -1865.85F, -63.19F, 3.851F}};

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
  constexpr float kBarrelTwist = 11.0F;
  constexpr float kWind = 15.0F;
  constexpr lob::ClockAngleT kWindHeading = lob::ClockAngleT::kIX;
  constexpr uint16_t kTestStepSize = 100;
  constexpr uint16_t kVelocityError = 1;
  constexpr uint16_t kEnergyError = 5;
  constexpr double kMoaError = 0.1;
  constexpr double kInchError = 0.5;
  constexpr double kTimeOfFlightError = 0.01;
  constexpr double kAerodynamicJump = 0.660887;
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
      {0, 3071, 5230, -2.00F, 0.00F, 0.000F},
      {150, 2992, 4965, 1.30F, -0.15F, 0.049F},
      {300, 2914, 4709, 3.62F, -0.62F, 0.100F},
      {600, 2761, 4227, 5.15F, -2.58F, 0.206F},
      {900, 2612, 3783, 2.10F, -5.99F, 0.318F},
      {1200, 2467, 3376, -6.04F, -10.99F, 0.436F},
      {1500, 2327, 3004, -19.90F, -17.70F, 0.561F},
      {1800, 2192, 2665, -40.17F, -26.27F, 0.694F},
      {2100, 2062, 2357, -67.69F, -36.84F, 0.835F},
      {2400, 1936, 2078, -103.41F, -49.61F, 0.985F},
      {2700, 1814, 1824, -148.40F, -64.76F, 1.145F},
      {3000, 1695, 1593, -204.01F, -82.56F, 1.316F},
      {4500, 1159, 745, -706.66F, -222.33F, 2.388F},
      {6000, 952, 503, -1852.03F, -450.26F, 3.851F}};

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
  constexpr float kBarrelTwist = 11.0F;
  constexpr float kWind = 15.0F;
  constexpr lob::ClockAngleT kWindHeading = lob::ClockAngleT::kIII;
  constexpr uint16_t kTestStepSize = 100;
  constexpr uint16_t kVelocityError = 1;
  constexpr uint16_t kEnergyError = 5;
  constexpr double kMoaError = 0.1;
  constexpr double kInchError = 0.5;
  constexpr double kTimeOfFlightError = 0.01;
  constexpr double kAerodynamicJump = -0.660887;
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
      {0, 3071, 5230, -2.00F, 0.00F, 0.000F},
      {150, 2992, 4965, 0.61F, 0.19F, 0.049F},
      {300, 2914, 4709, 2.24F, 0.75F, 0.100F},
      {600, 2761, 4227, 2.38F, 3.06F, 0.206F},
      {900, 2612, 3783, -2.04F, 7.05F, 0.318F},
      {1200, 2467, 3376, -11.57F, 12.87F, 0.436F},
      {1500, 2327, 3004, -26.81F, 20.69F, 0.561F},
      {1800, 2192, 2665, -48.47F, 30.67F, 0.694F},
      {2100, 2062, 2357, -77.37F, 43.03F, 0.835F},
      {2400, 1936, 2078, -114.47F, 57.98F, 0.985F},
      {2700, 1814, 1824, -160.84F, 75.79F, 1.145F},
      {3000, 1695, 1593, -217.84F, 96.79F, 1.316F},
      {4500, 1159, 745, -727.39F, 264.63F, 2.388F},
      {6000, 952, 503, -1879.67F, 551.74F, 3.851F}};

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