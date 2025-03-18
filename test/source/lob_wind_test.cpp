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

struct LobWindTestFixture : public testing::Test {
  // Unit under test
  std::unique_ptr<lob::Builder> puut;

  LobWindTestFixture() : puut(nullptr) {}

  void SetUp() override {
    ASSERT_EQ(puut, nullptr);
    puut = std::make_unique<lob::Builder>();
    ASSERT_NE(puut, nullptr);

    const float kTestBC = 0.372F;
    const lob::DragFunctionT kDragFunction = lob::DragFunctionT::kG1;
    const float kTestDiameter = 0.224F;
    const float kTestWeight = 77.0F;
    const uint16_t kTestMuzzleVelocity = 2720;
    const float kTestZeroAngle = 4.78F;
    const float kTestOpticHeight = 2.5F;

    puut->BallisticCoefficentPsi(kTestBC)
        .BCDragFunction(kDragFunction)
        .BCAtmosphere(lob::AtmosphereReferenceT::kIcao)
        .DiameterInch(kTestDiameter)
        .MassGrains(kTestWeight)
        .InitialVelocityFps(kTestMuzzleVelocity)
        .ZeroAngleMOA(kTestZeroAngle)
        .OpticHeightInches(kTestOpticHeight);
  }

  void TearDown() override { puut.reset(); }
};

TEST_F(LobWindTestFixture, ZeroAngleSearch) {
  ASSERT_NE(puut, nullptr);
  auto input1 = puut->Build();
  const float kZeroRange = 100.0F;
  auto input2 = puut->ZeroAngleMOA(std::numeric_limits<double>::quiet_NaN())
                    .ZeroDistanceYds(kZeroRange)
                    .Build();
  const float kError = 0.01F;
  EXPECT_NEAR(input1.zero_angle, input2.zero_angle, kError);
}

TEST_F(LobWindTestFixture, GetSpeedOfSoundFps) {
  ASSERT_NE(puut, nullptr);
  const auto kInput = puut->Build();
  const float kExpectedFps = 1116.45F;
  const float kError = 0.001F;
  EXPECT_NEAR(kInput.speed_of_sound, kExpectedFps, kError);
}

TEST_F(LobWindTestFixture, SolveWithoutWind) {
  ASSERT_NE(puut, nullptr);
  constexpr uint16_t kTestStepSize = 100;
  constexpr uint16_t kVelocityError = 1;
  constexpr uint16_t kEnergyError = 5;
  constexpr double kMoaError = 0.1;
  constexpr double kInchError = 0.1;
  constexpr double kTimeOfFlightError = 0.01;
  constexpr size_t kSolutionLength = 12;
  const auto kInput = puut->Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0, 150, 300, 600, 900, 1200, 1500, 1800, 2100, 2400, 2700, 3000};
  const std::vector<lob::Output> kExpected = {
      {0, 2720, 1264, -2.50F, 0.00F, 0.000F},
      {150, 2597, 1152, -0.60F, 0.00F, 0.056F},
      {300, 2477, 1048, 0.01F, 0.00F, 0.116F},
      {600, 2248, 863, -3.18F, 0.00F, 0.243F},
      {900, 2030, 704, -13.26F, 0.00F, 0.383F},
      {1200, 1826, 569, -31.80F, 0.00F, 0.539F},
      {1500, 1636, 457, -60.84F, 0.00F, 0.713F},
      {1800, 1464, 366, -102.89F, 0.00F, 0.906F},
      {2100, 1313, 294, -161.25F, 0.00F, 1.123F},
      {2400, 1187, 241, -239.78F, 0.00F, 1.364F},
      {2700, 1091, 203, -343.03F, 0.00F, 1.628F},
      {3000, 1021, 178, -475.40F, 0.00F, 1.913F}};

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

TEST_F(LobWindTestFixture, SolveWithClockWindIII) {
  ASSERT_NE(puut, nullptr);
  const int32_t kWindSpeed = 10;
  const lob::ClockAngleT kWindHeading = lob::ClockAngleT::kIII;
  constexpr uint16_t kTestStepSize = 100;
  constexpr uint16_t kVelocityError = 1;
  constexpr uint16_t kEnergyError = 5;
  constexpr double kMoaError = 0.1;
  constexpr double kInchError = 0.1;
  constexpr double kTimeOfFlightError = 0.01;
  constexpr size_t kSolutionLength = 12;
  const auto kInput =
      puut->WindSpeedMph(kWindSpeed).WindHeading(kWindHeading).Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0, 150, 300, 600, 900, 1200, 1500, 1800, 2100, 2400, 2700, 3000};
  const std::vector<lob::Output> kExpected = {
      {0, 2720, 1264, -2.50F, 0.00F, 0.000F},
      {150, 2597, 1152, -0.60F, 0.23F, 0.056F},
      {300, 2477, 1048, 0.01F, 0.93F, 0.116F},
      {600, 2248, 863, -3.18F, 3.90F, 0.243F},
      {900, 2030, 704, -13.26F, 9.20F, 0.383F},
      {1200, 1826, 569, -31.80F, 17.22F, 0.539F},
      {1500, 1636, 457, -60.84F, 28.37F, 0.713F},
      {1800, 1464, 366, -102.89F, 43.09F, 0.906F},
      {2100, 1313, 294, -161.25F, 61.81F, 1.123F},
      {2400, 1187, 241, -239.81F, 84.76F, 1.364F},
      {2700, 1091, 203, -343.03F, 111.83F, 1.628F},
      {3000, 1021, 178, -475.40F, 142.55F, 1.913F}};

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

TEST_F(LobWindTestFixture, SolveWithClockWindIV) {
  ASSERT_NE(puut, nullptr);
  const int32_t kWindSpeed = 10;
  const lob::ClockAngleT kWindHeading = lob::ClockAngleT::kIV;
  constexpr uint16_t kTestStepSize = 100;
  constexpr uint16_t kVelocityError = 1;
  constexpr uint16_t kEnergyError = 5;
  constexpr double kMoaError = 0.1;
  constexpr double kInchError = 0.1;
  constexpr double kTimeOfFlightError = 0.01;
  constexpr size_t kSolutionLength = 12;
  const auto kInput =
      puut->WindSpeedMph(kWindSpeed).WindHeading(kWindHeading).Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0, 150, 300, 600, 900, 1200, 1500, 1800, 2100, 2400, 2700, 3000};
  const std::vector<lob::Output> kExpected = {
      {0, 2720, 1264, -2.50F, 0.00F, 0.000F},
      {150, 2596, 1151, -0.60F, 0.20F, 0.056F},
      {300, 2476, 1047, 0.00F, 0.81F, 0.116F},
      {600, 2245, 861, -3.18F, 3.39F, 0.243F},
      {900, 2026, 701, -13.30F, 8.00F, 0.383F},
      {1200, 1820, 566, -31.91F, 14.98F, 0.540F},
      {1500, 1630, 454, -61.07F, 24.70F, 0.714F},
      {1800, 1457, 362, -103.38F, 37.55F, 0.909F},
      {2100, 1305, 291, -162.17F, 53.89F, 1.126F},
      {2400, 1180, 238, -241.47F, 73.94F, 1.369F},
      {2700, 1085, 201, -345.75F, 97.57F, 1.634F},
      {3000, 1015, 176, -479.64F, 124.35F, 1.921F}};

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

TEST_F(LobWindTestFixture, SolveWithClockWindV) {
  ASSERT_NE(puut, nullptr);
  const int32_t kWindSpeed = 10;
  const lob::ClockAngleT kWindHeading = lob::ClockAngleT::kV;
  constexpr uint16_t kTestStepSize = 100;
  constexpr uint16_t kVelocityError = 1;
  constexpr uint16_t kEnergyError = 5;
  constexpr double kMoaError = 0.1;
  constexpr double kInchError = 0.1;
  constexpr double kTimeOfFlightError = 0.01;
  constexpr size_t kSolutionLength = 12;
  const auto kInput =
      puut->WindSpeedMph(kWindSpeed).WindHeading(kWindHeading).Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0, 150, 300, 600, 900, 1200, 1500, 1800, 2100, 2400, 2700, 3000};
  const std::vector<lob::Output> kExpected = {
      {0, 2720, 1264, -2.50F, 0.00F, 0.000F},
      {150, 2596, 1151, -0.60F, 0.11F, 0.056F},
      {300, 2475, 1046, 0.00F, 0.47F, 0.116F},
      {600, 2243, 859, -3.19F, 1.96F, 0.243F},
      {900, 2023, 699, -13.31F, 4.63F, 0.384F},
      {1200, 1817, 564, -31.98F, 8.68F, 0.540F},
      {1500, 1625, 451, -61.25F, 14.32F, 0.715F},
      {1800, 1451, 360, -103.75F, 21.78F, 0.910F},
      {2100, 1300, 288, -162.87F, 31.27F, 1.129F},
      {2400, 1175, 236, -242.68F, 42.91F, 1.372F},
      {2700, 1080, 199, -347.81F, 56.64F, 1.639F},
      {3000, 1011, 175, -482.80F, 72.18F, 1.927F}};

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

TEST_F(LobWindTestFixture, SolveWithClockWindVI) {
  ASSERT_NE(puut, nullptr);
  const int32_t kWindSpeed = 10;
  const lob::ClockAngleT kWindHeading = lob::ClockAngleT::kVI;
  constexpr uint16_t kTestStepSize = 100;
  constexpr uint16_t kVelocityError = 1;
  constexpr uint16_t kEnergyError = 5;
  constexpr double kMoaError = 0.1;
  constexpr double kInchError = 0.1;
  constexpr double kTimeOfFlightError = 0.01;
  constexpr size_t kSolutionLength = 12;
  const auto kInput =
      puut->WindSpeedMph(kWindSpeed).WindHeading(kWindHeading).Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0, 150, 300, 600, 900, 1200, 1500, 1800, 2100, 2400, 2700, 3000};
  const std::vector<lob::Output> kExpected = {
      {0, 2720, 1264, -2.50F, 0.00F, 0.000F},
      {150, 2596, 1151, -0.60F, -0.00F, 0.056F},
      {300, 2475, 1046, 0.00F, -0.00F, 0.116F},
      {600, 2242, 859, -3.19F, -0.00F, 0.243F},
      {900, 2022, 699, -13.32F, -0.00F, 0.384F},
      {1200, 1815, 563, -32.01F, -0.00F, 0.540F},
      {1500, 1623, 450, -61.30F, -0.00F, 0.715F},
      {1800, 1449, 359, -103.88F, -0.00F, 0.911F},
      {2100, 1298, 288, -163.13F, -0.00F, 1.130F},
      {2400, 1173, 235, -243.17F, -0.00F, 1.374F},
      {2700, 1079, 199, -348.55F, -0.00F, 1.641F},
      {3000, 1009, 174, -483.97F, -0.00F, 1.929F}};

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

TEST_F(LobWindTestFixture, SolveWithClockWindVII) {
  ASSERT_NE(puut, nullptr);
  const int32_t kWindSpeed = 10;
  const lob::ClockAngleT kWindHeading = lob::ClockAngleT::kVII;
  constexpr uint16_t kTestStepSize = 100;
  constexpr uint16_t kVelocityError = 1;
  constexpr uint16_t kEnergyError = 5;
  constexpr double kMoaError = 0.1;
  constexpr double kInchError = 0.1;
  constexpr double kTimeOfFlightError = 0.01;
  constexpr size_t kSolutionLength = 12;
  const auto kInput =
      puut->WindSpeedMph(kWindSpeed).WindHeading(kWindHeading).Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0, 150, 300, 600, 900, 1200, 1500, 1800, 2100, 2400, 2700, 3000};
  const std::vector<lob::Output> kExpected = {
      {0, 2720, 1264, -2.50F, 0.00F, 0.000F},
      {150, 2596, 1151, -0.60F, -0.11F, 0.056F},
      {300, 2475, 1046, 0.00F, -0.47F, 0.116F},
      {600, 2243, 859, -3.19F, -1.96F, 0.243F},
      {900, 2023, 699, -13.31F, -4.63F, 0.384F},
      {1200, 1817, 564, -31.98F, -8.68F, 0.540F},
      {1500, 1625, 451, -61.25F, -14.32F, 0.715F},
      {1800, 1451, 360, -103.75F, -21.78F, 0.910F},
      {2100, 1300, 288, -162.87F, -31.27F, 1.129F},
      {2400, 1175, 236, -242.68F, -42.91F, 1.372F},
      {2700, 1080, 199, -347.81F, -56.64F, 1.639F},
      {3000, 1011, 175, -482.80F, -72.18F, 1.927F}};

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

TEST_F(LobWindTestFixture, SolveWithClockWindVIII) {
  ASSERT_NE(puut, nullptr);
  const int32_t kWindSpeed = 10;
  const lob::ClockAngleT kWindHeading = lob::ClockAngleT::kVIII;
  constexpr uint16_t kTestStepSize = 100;
  constexpr uint16_t kVelocityError = 1;
  constexpr uint16_t kEnergyError = 5;
  constexpr double kMoaError = 0.1;
  constexpr double kInchError = 0.1;
  constexpr double kTimeOfFlightError = 0.01;
  constexpr size_t kSolutionLength = 12;
  const auto kInput =
      puut->WindSpeedMph(kWindSpeed).WindHeading(kWindHeading).Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0, 150, 300, 600, 900, 1200, 1500, 1800, 2100, 2400, 2700, 3000};
  const std::vector<lob::Output> kExpected = {
      {0, 2720, 1264, -2.50F, 0.00F, 0.000F},
      {150, 2596, 1151, -0.60F, -0.20F, 0.056F},
      {300, 2476, 1047, 0.00F, -0.81F, 0.116F},
      {600, 2245, 861, -3.18F, -3.39F, 0.243F},
      {900, 2026, 701, -13.30F, -8.00F, 0.383F},
      {1200, 1820, 566, -31.91F, -14.98F, 0.540F},
      {1500, 1630, 454, -61.07F, -24.70F, 0.714F},
      {1800, 1457, 362, -103.38F, -37.55F, 0.909F},
      {2100, 1305, 291, -162.17F, -53.89F, 1.126F},
      {2400, 1180, 238, -241.47F, -73.94F, 1.369F},
      {2700, 1085, 201, -345.75F, -97.57F, 1.634F},
      {3000, 1015, 176, -479.64F, -124.35F, 1.921F}};

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

TEST_F(LobWindTestFixture, SolveWithClockWindIX) {
  ASSERT_NE(puut, nullptr);
  const int32_t kWindSpeed = 10;
  const lob::ClockAngleT kWindHeading = lob::ClockAngleT::kIX;
  constexpr uint16_t kTestStepSize = 100;
  constexpr uint16_t kVelocityError = 1;
  constexpr uint16_t kEnergyError = 5;
  constexpr double kMoaError = 0.1;
  constexpr double kInchError = 0.1;
  constexpr double kTimeOfFlightError = 0.01;
  constexpr size_t kSolutionLength = 12;
  const auto kInput =
      puut->WindSpeedMph(kWindSpeed).WindHeading(kWindHeading).Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0, 150, 300, 600, 900, 1200, 1500, 1800, 2100, 2400, 2700, 3000};
  const std::vector<lob::Output> kExpected = {
      {0, 2720, 1264, -2.50F, 0.00F, 0.000F},
      {150, 2597, 1152, -0.60F, -0.23F, 0.056F},
      {300, 2477, 1048, 0.01F, -0.93F, 0.116F},
      {600, 2248, 863, -3.18F, -3.90F, 0.243F},
      {900, 2030, 704, -13.26F, -9.20F, 0.383F},
      {1200, 1826, 569, -31.80F, -17.22F, 0.539F},
      {1500, 1636, 457, -60.84F, -28.37F, 0.713F},
      {1800, 1464, 366, -102.89F, -43.09F, 0.906F},
      {2100, 1313, 294, -161.25F, -61.81F, 1.123F},
      {2400, 1187, 241, -239.81F, -84.76F, 1.364F},
      {2700, 1091, 203, -343.03F, -111.83F, 1.628F},
      {3000, 1021, 178, -475.40F, -142.55F, 1.913F}};

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

TEST_F(LobWindTestFixture, SolveWithClockWindX) {
  ASSERT_NE(puut, nullptr);
  const int32_t kWindSpeed = 10;
  const lob::ClockAngleT kWindHeading = lob::ClockAngleT::kX;
  constexpr uint16_t kTestStepSize = 100;
  constexpr uint16_t kVelocityError = 1;
  constexpr uint16_t kEnergyError = 5;
  constexpr double kMoaError = 0.1;
  constexpr double kInchError = 0.1;
  constexpr double kTimeOfFlightError = 0.01;
  constexpr size_t kSolutionLength = 12;
  const auto kInput =
      puut->WindSpeedMph(kWindSpeed).WindHeading(kWindHeading).Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0, 150, 300, 600, 900, 1200, 1500, 1800, 2100, 2400, 2700, 3000};
  const std::vector<lob::Output> kExpected = {
      {0, 2720, 1264, -2.50F, 0.00F, 0.000F},
      {150, 2598, 1153, -0.60F, -0.20F, 0.056F},
      {300, 2479, 1049, 0.01F, -0.80F, 0.116F},
      {600, 2250, 865, -3.17F, -3.36F, 0.242F},
      {900, 2034, 707, -13.22F, -7.93F, 0.383F},
      {1200, 1831, 573, -31.71F, -14.84F, 0.538F},
      {1500, 1642, 461, -60.61F, -24.43F, 0.711F},
      {1800, 1471, 370, -102.41F, -37.09F, 0.904F},
      {2100, 1320, 298, -160.30F, -53.16F, 1.120F},
      {2400, 1195, 244, -238.17F, -72.87F, 1.359F},
      {2700, 1098, 206, -340.32F, -96.13F, 1.622F},
      {3000, 1026, 180, -471.25F, -122.56F, 1.905F}};

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

TEST_F(LobWindTestFixture, SolveWithClockWindXI) {
  ASSERT_NE(puut, nullptr);
  const int32_t kWindSpeed = 10;
  const lob::ClockAngleT kWindHeading = lob::ClockAngleT::kXI;
  constexpr uint16_t kTestStepSize = 100;
  constexpr uint16_t kVelocityError = 1;
  constexpr uint16_t kEnergyError = 5;
  constexpr double kMoaError = 0.1;
  constexpr double kInchError = 0.1;
  constexpr double kTimeOfFlightError = 0.01;
  constexpr size_t kSolutionLength = 12;
  const auto kInput =
      puut->WindSpeedMph(kWindSpeed).WindHeading(kWindHeading).Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0, 150, 300, 600, 900, 1200, 1500, 1800, 2100, 2400, 2700, 3000};
  const std::vector<lob::Output> kExpected = {
      {0, 2720, 1264, -2.50F, 0.00F, 0.000F},
      {150, 2598, 1153, -0.60F, -0.11F, 0.056F},
      {300, 2480, 1050, 0.01F, -0.46F, 0.116F},
      {600, 2252, 866, -3.16F, -1.93F, 0.242F},
      {900, 2037, 709, -13.20F, -4.57F, 0.382F},
      {1200, 1835, 575, -31.64F, -8.54F, 0.538F},
      {1500, 1647, 463, -60.43F, -14.05F, 0.710F},
      {1800, 1476, 372, -102.05F, -21.32F, 0.903F},
      {2100, 1326, 300, -159.63F, -30.54F, 1.117F},
      {2400, 1200, 246, -236.97F, -41.85F, 1.356F},
      {2700, 1103, 208, -338.35F, -55.20F, 1.617F},
      {3000, 1031, 181, -468.21F, -70.38F, 1.899F}};

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

TEST_F(LobWindTestFixture, SolveWithClockWindXII) {
  ASSERT_NE(puut, nullptr);
  const int32_t kWindSpeed = 10;
  const lob::ClockAngleT kWindHeading = lob::ClockAngleT::kXII;
  constexpr uint16_t kTestStepSize = 100;
  constexpr uint16_t kVelocityError = 1;
  constexpr uint16_t kEnergyError = 5;
  constexpr double kMoaError = 0.1;
  constexpr double kInchError = 0.1;
  constexpr double kTimeOfFlightError = 0.01;
  constexpr size_t kSolutionLength = 12;
  const auto kInput =
      puut->WindSpeedMph(kWindSpeed).WindHeading(kWindHeading).Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0, 150, 300, 600, 900, 1200, 1500, 1800, 2100, 2400, 2700, 3000};
  const std::vector<lob::Output> kExpected = {
      {0, 2720, 1264, -2.50F, 0.00F, 0.000F},
      {150, 2598, 1153, -0.60F, 0.00F, 0.056F},
      {300, 2480, 1051, 0.01F, 0.00F, 0.116F},
      {600, 2253, 867, -3.16F, 0.00F, 0.242F},
      {900, 2038, 709, -13.19F, 0.00F, 0.382F},
      {1200, 1836, 576, -31.62F, 0.00F, 0.537F},
      {1500, 1649, 464, -60.38F, 0.00F, 0.710F},
      {1800, 1478, 373, -101.92F, 0.00F, 0.902F},
      {2100, 1328, 301, -159.41F, 0.00F, 1.117F},
      {2400, 1202, 247, -236.53F, 0.00F, 1.354F},
      {2700, 1105, 208, -337.64F, 0.00F, 1.615F},
      {3000, 1032, 182, -467.08F, 0.00F, 1.897F}};

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

TEST_F(LobWindTestFixture, SolveWithClockWindI) {
  ASSERT_NE(puut, nullptr);
  const int32_t kWindSpeed = 10;
  const lob::ClockAngleT kWindHeading = lob::ClockAngleT::kI;
  constexpr uint16_t kTestStepSize = 100;
  constexpr uint16_t kVelocityError = 1;
  constexpr uint16_t kEnergyError = 5;
  constexpr double kMoaError = 0.1;
  constexpr double kInchError = 0.1;
  constexpr double kTimeOfFlightError = 0.01;
  constexpr size_t kSolutionLength = 12;
  const auto kInput =
      puut->WindSpeedMph(kWindSpeed).WindHeading(kWindHeading).Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0, 150, 300, 600, 900, 1200, 1500, 1800, 2100, 2400, 2700, 3000};
  const std::vector<lob::Output> kExpected = {
      {0, 2720, 1264, -2.50F, 0.00F, 0.000F},
      {150, 2598, 1153, -0.60F, 0.11F, 0.056F},
      {300, 2480, 1050, 0.01F, 0.46F, 0.116F},
      {600, 2252, 866, -3.16F, 1.93F, 0.242F},
      {900, 2037, 709, -13.20F, 4.57F, 0.382F},
      {1200, 1835, 575, -31.64F, 8.54F, 0.538F},
      {1500, 1647, 463, -60.43F, 14.05F, 0.710F},
      {1800, 1476, 372, -102.05F, 21.32F, 0.903F},
      {2100, 1326, 300, -159.63F, 30.54F, 1.117F},
      {2400, 1200, 246, -236.97F, 41.85F, 1.356F},
      {2700, 1103, 208, -338.35F, 55.20F, 1.617F},
      {3000, 1031, 181, -468.21F, 70.38F, 1.899F}};

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

TEST_F(LobWindTestFixture, SolveWithClockWindII) {
  ASSERT_NE(puut, nullptr);
  const int32_t kWindSpeed = 10;
  const lob::ClockAngleT kWindHeading = lob::ClockAngleT::kII;
  constexpr uint16_t kTestStepSize = 100;
  constexpr uint16_t kVelocityError = 1;
  constexpr uint16_t kEnergyError = 5;
  constexpr double kMoaError = 0.1;
  constexpr double kInchError = 0.1;
  constexpr double kTimeOfFlightError = 0.01;
  constexpr size_t kSolutionLength = 12;
  const auto kInput =
      puut->WindSpeedMph(kWindSpeed).WindHeading(kWindHeading).Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0, 150, 300, 600, 900, 1200, 1500, 1800, 2100, 2400, 2700, 3000};
  const std::vector<lob::Output> kExpected = {
      {0, 2720, 1264, -2.50F, 0.00F, 0.000F},
      {150, 2598, 1153, -0.60F, 0.20F, 0.056F},
      {300, 2479, 1049, 0.01F, 0.80F, 0.116F},
      {600, 2250, 865, -3.17F, 3.36F, 0.242F},
      {900, 2034, 707, -13.22F, 7.93F, 0.383F},
      {1200, 1831, 573, -31.71F, 14.84F, 0.538F},
      {1500, 1642, 461, -60.61F, 24.43F, 0.711F},
      {1800, 1471, 370, -102.41F, 37.09F, 0.904F},
      {2100, 1320, 298, -160.30F, 53.16F, 1.120F},
      {2400, 1195, 244, -238.17F, 72.87F, 1.359F},
      {2700, 1098, 206, -340.32F, 96.13F, 1.622F},
      {3000, 1026, 180, -471.25F, 122.56F, 1.905F}};

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

TEST_F(LobWindTestFixture, SolveWithAngleWind150) {
  ASSERT_NE(puut, nullptr);
  const int32_t kWindSpeed = 20;
  const float kWindHeading = 150;
  constexpr uint16_t kTestStepSize = 100;
  constexpr uint16_t kVelocityError = 1;
  constexpr uint16_t kEnergyError = 5;
  constexpr double kMoaError = 0.1;
  constexpr double kInchError = 0.1;
  constexpr double kTimeOfFlightError = 0.01;
  constexpr size_t kSolutionLength = 12;
  const auto kInput =
      puut->WindSpeedMph(kWindSpeed).WindHeadingDeg(kWindHeading).Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0, 150, 300, 600, 900, 1200, 1500, 1800, 2100, 2400, 2700, 3000};
  const std::vector<lob::Output> kExpected = {
      {0, 2720, 1264, -2.50F, 0.00F, 0.000F},
      {150, 2595, 1150, -0.60F, 0.23F, 0.056F},
      {300, 2473, 1044, 0.00F, 0.94F, 0.116F},
      {600, 2238, 856, -3.20F, 3.94F, 0.243F},
      {900, 2016, 694, -13.38F, 9.34F, 0.384F},
      {1200, 1808, 558, -32.15F, 17.51F, 0.541F},
      {1500, 1614, 445, -61.66F, 28.91F, 0.717F},
      {1800, 1439, 354, -104.63F, 44.03F, 0.914F},
      {2100, 1286, 283, -164.57F, 63.30F, 1.135F},
      {2400, 1162, 231, -245.69F, 86.93F, 1.381F},
      {2700, 1069, 195, -352.75F, 114.74F, 1.651F},
      {3000, 1001, 171, -490.37F, 146.17F, 1.941F}};

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

TEST_F(LobWindTestFixture, SolveWithAngleWindNegativeMagnitude) {
  ASSERT_NE(puut, nullptr);
  const int32_t kWindSpeed = -20;
  const float kWindHeading = 330;
  constexpr uint16_t kTestStepSize = 100;
  constexpr uint16_t kVelocityError = 1;
  constexpr uint16_t kEnergyError = 5;
  constexpr double kMoaError = 0.1;
  constexpr double kInchError = 0.1;
  constexpr double kTimeOfFlightError = 0.01;
  constexpr size_t kSolutionLength = 12;
  const auto kInput =
      puut->WindSpeedMph(kWindSpeed).WindHeadingDeg(kWindHeading).Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0, 150, 300, 600, 900, 1200, 1500, 1800, 2100, 2400, 2700, 3000};
  const std::vector<lob::Output> kExpected = {
      {0, 2720, 1264, -2.50F, 0.00F, 0.000F},
      {150, 2595, 1150, -0.60F, 0.23F, 0.056F},
      {300, 2473, 1044, 0.00F, 0.94F, 0.116F},
      {600, 2238, 856, -3.20F, 3.94F, 0.243F},
      {900, 2016, 694, -13.38F, 9.34F, 0.384F},
      {1200, 1808, 558, -32.15F, 17.51F, 0.541F},
      {1500, 1614, 445, -61.66F, 28.91F, 0.717F},
      {1800, 1439, 354, -104.63F, 44.03F, 0.914F},
      {2100, 1286, 283, -164.57F, 63.30F, 1.135F},
      {2400, 1162, 231, -245.69F, 86.93F, 1.381F},
      {2700, 1069, 195, -352.75F, 114.74F, 1.651F},
      {3000, 1001, 171, -490.37F, 146.17F, 1.941F}};

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

TEST_F(LobWindTestFixture, SolveWithAngleWindNegativeAngle) {
  ASSERT_NE(puut, nullptr);
  const int32_t kWindSpeed = 20;
  const float kWindHeading = -210;
  constexpr uint16_t kTestStepSize = 100;
  constexpr uint16_t kVelocityError = 1;
  constexpr uint16_t kEnergyError = 5;
  constexpr double kMoaError = 0.1;
  constexpr double kInchError = 0.1;
  constexpr double kTimeOfFlightError = 0.01;
  constexpr size_t kSolutionLength = 12;
  const auto kInput =
      puut->WindSpeedMph(kWindSpeed).WindHeadingDeg(kWindHeading).Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0, 150, 300, 600, 900, 1200, 1500, 1800, 2100, 2400, 2700, 3000};
  const std::vector<lob::Output> kExpected = {
      {0, 2720, 1264, -2.50F, 0.00F, 0.000F},
      {150, 2595, 1150, -0.60F, 0.23F, 0.056F},
      {300, 2473, 1044, 0.00F, 0.94F, 0.116F},
      {600, 2238, 856, -3.20F, 3.94F, 0.243F},
      {900, 2016, 694, -13.38F, 9.34F, 0.384F},
      {1200, 1808, 558, -32.15F, 17.51F, 0.541F},
      {1500, 1614, 445, -61.66F, 28.91F, 0.717F},
      {1800, 1439, 354, -104.63F, 44.03F, 0.914F},
      {2100, 1286, 283, -164.57F, 63.30F, 1.135F},
      {2400, 1162, 231, -245.69F, 86.93F, 1.381F},
      {2700, 1069, 195, -352.75F, 114.74F, 1.651F},
      {3000, 1001, 171, -490.37F, 146.17F, 1.941F}};

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