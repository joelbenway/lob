// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
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

    const double kTestBC = 0.372;
    const lob::DragFunctionT kDragFunction = lob::DragFunctionT::kG1;
    const double kTestDiameter = 0.224;
    const double kTestWeight = 77.0;
    const uint16_t kTestMuzzleVelocity = 2720;
    const double kTestZeroAngle = 4.78;
    const double kTestOpticHeight = 2.5;
    const uint16_t kStep = 100U;

    puut->BallisticCoefficientPsi(kTestBC)
        .BCDragFunction(kDragFunction)
        .BCAtmosphere(lob::AtmosphereReferenceT::kIcao)
        .DiameterInch(kTestDiameter)
        .MassGrains(kTestWeight)
        .InitialVelocityFps(kTestMuzzleVelocity)
        .ZeroAngleMOA(kTestZeroAngle)
        .OpticHeightInches(kTestOpticHeight)
        .StepSize(kStep);
  }

  void TearDown() override { puut.reset(); }
};

TEST_F(LobWindTestFixture, ZeroAngleSearch) {
  ASSERT_NE(puut, nullptr);
  auto input1 = puut->Build();
  const double kZeroRange = 100.0;
  auto input2 = puut->ZeroAngleMOA(std::numeric_limits<double>::quiet_NaN())
                    .ZeroDistanceYds(kZeroRange)
                    .Build();
  const double kError = 0.01;
  EXPECT_NEAR(input1.zero_angle, input2.zero_angle, kError);
}

TEST_F(LobWindTestFixture, GetSpeedOfSoundFps) {
  ASSERT_NE(puut, nullptr);
  const auto kInput = puut->Build();
  const double kExpectedFps = 1116.45;
  const double kError = 0.001;
  EXPECT_NEAR(kInput.speed_of_sound, kExpectedFps, kError);
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
TEST_F(LobWindTestFixture, SolveWithoutWind) {
  ASSERT_NE(puut, nullptr);
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
      {0, 2720, 1264, -2.50, 0.00, 0.000},
      {150, 2597, 1152, -0.60, 0.00, 0.056},
      {300, 2477, 1048, 0.01, 0.00, 0.116},
      {600, 2248, 863, -3.18, 0.00, 0.243},
      {900, 2030, 704, -13.26, 0.00, 0.383},
      {1200, 1826, 569, -31.80, 0.00, 0.539},
      {1500, 1636, 457, -60.84, 0.00, 0.713},
      {1800, 1464, 366, -102.89, 0.00, 0.906},
      {2100, 1313, 294, -161.25, 0.00, 1.123},
      {2400, 1187, 241, -239.78, 0.00, 1.364},
      {2700, 1091, 203, -343.03, 0.00, 1.628},
      {3000, 1021, 178, -475.40, 0.00, 1.913}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  lob::Solve(kInput, kRanges, solutions);
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
TEST_F(LobWindTestFixture, SolveWithClockWindIII) {
  ASSERT_NE(puut, nullptr);
  const int32_t kWindSpeed = 10;
  const lob::ClockAngleT kWindHeading = lob::ClockAngleT::kIII;
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
      {0, 2720, 1264, -2.50, 0.00, 0.000},
      {150, 2597, 1152, -0.60, 0.23, 0.056},
      {300, 2477, 1048, 0.01, 0.93, 0.116},
      {600, 2248, 863, -3.18, 3.90, 0.243},
      {900, 2030, 704, -13.26, 9.20, 0.383},
      {1200, 1826, 569, -31.80, 17.22, 0.539},
      {1500, 1636, 457, -60.84, 28.37, 0.713},
      {1800, 1464, 366, -102.89, 43.09, 0.906},
      {2100, 1313, 294, -161.25, 61.81, 1.123},
      {2400, 1187, 241, -239.81, 84.76, 1.364},
      {2700, 1091, 203, -343.03, 111.83, 1.628},
      {3000, 1021, 178, -475.40, 142.55, 1.913}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  lob::Solve(kInput, kRanges, solutions);
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
TEST_F(LobWindTestFixture, SolveWithClockWindIV) {
  ASSERT_NE(puut, nullptr);
  const int32_t kWindSpeed = 10;
  const lob::ClockAngleT kWindHeading = lob::ClockAngleT::kIV;
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
      {0, 2720, 1264, -2.50, 0.00, 0.000},
      {150, 2596, 1151, -0.60, 0.20, 0.056},
      {300, 2476, 1047, 0.00, 0.81, 0.116},
      {600, 2245, 861, -3.18, 3.39, 0.243},
      {900, 2026, 701, -13.30, 8.00, 0.383},
      {1200, 1820, 566, -31.91, 14.98, 0.540},
      {1500, 1630, 454, -61.07, 24.70, 0.714},
      {1800, 1457, 362, -103.38, 37.55, 0.909},
      {2100, 1305, 291, -162.17, 53.89, 1.126},
      {2400, 1180, 238, -241.47, 73.94, 1.369},
      {2700, 1085, 201, -345.75, 97.57, 1.634},
      {3000, 1015, 176, -479.64, 124.35, 1.921}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  lob::Solve(kInput, kRanges, solutions);
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
TEST_F(LobWindTestFixture, SolveWithClockWindV) {
  ASSERT_NE(puut, nullptr);
  const int32_t kWindSpeed = 10;
  const lob::ClockAngleT kWindHeading = lob::ClockAngleT::kV;
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
      {0, 2720, 1264, -2.50, 0.00, 0.000},
      {150, 2596, 1151, -0.60, 0.11, 0.056},
      {300, 2475, 1046, 0.00, 0.47, 0.116},
      {600, 2243, 859, -3.19, 1.96, 0.243},
      {900, 2023, 699, -13.31, 4.63, 0.384},
      {1200, 1817, 564, -31.98, 8.68, 0.540},
      {1500, 1625, 451, -61.25, 14.32, 0.715},
      {1800, 1451, 360, -103.75, 21.78, 0.910},
      {2100, 1300, 288, -162.87, 31.27, 1.129},
      {2400, 1175, 236, -242.68, 42.91, 1.372},
      {2700, 1080, 199, -347.81, 56.64, 1.639},
      {3000, 1011, 175, -482.80, 72.18, 1.927}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  lob::Solve(kInput, kRanges, solutions);
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
TEST_F(LobWindTestFixture, SolveWithClockWindVI) {
  ASSERT_NE(puut, nullptr);
  const int32_t kWindSpeed = 10;
  const lob::ClockAngleT kWindHeading = lob::ClockAngleT::kVI;
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
      {0, 2720, 1264, -2.50, 0.00, 0.000},
      {150, 2596, 1151, -0.60, -0.00, 0.056},
      {300, 2475, 1046, 0.00, -0.00, 0.116},
      {600, 2242, 859, -3.19, -0.00, 0.243},
      {900, 2022, 699, -13.32, -0.00, 0.384},
      {1200, 1815, 563, -32.01, -0.00, 0.540},
      {1500, 1623, 450, -61.30, -0.00, 0.715},
      {1800, 1449, 359, -103.88, -0.00, 0.911},
      {2100, 1298, 288, -163.13, -0.00, 1.130},
      {2400, 1173, 235, -243.17, -0.00, 1.374},
      {2700, 1079, 199, -348.55, -0.00, 1.641},
      {3000, 1009, 174, -483.97, -0.00, 1.929}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  lob::Solve(kInput, kRanges, solutions);
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
TEST_F(LobWindTestFixture, SolveWithClockWindVII) {
  ASSERT_NE(puut, nullptr);
  const int32_t kWindSpeed = 10;
  const lob::ClockAngleT kWindHeading = lob::ClockAngleT::kVII;
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
      {0, 2720, 1264, -2.50, 0.00, 0.000},
      {150, 2596, 1151, -0.60, -0.11, 0.056},
      {300, 2475, 1046, 0.00, -0.47, 0.116},
      {600, 2243, 859, -3.19, -1.96, 0.243},
      {900, 2023, 699, -13.31, -4.63, 0.384},
      {1200, 1817, 564, -31.98, -8.68, 0.540},
      {1500, 1625, 451, -61.25, -14.32, 0.715},
      {1800, 1451, 360, -103.75, -21.78, 0.910},
      {2100, 1300, 288, -162.87, -31.27, 1.129},
      {2400, 1175, 236, -242.68, -42.91, 1.372},
      {2700, 1080, 199, -347.81, -56.64, 1.639},
      {3000, 1011, 175, -482.80, -72.18, 1.927}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  lob::Solve(kInput, kRanges, solutions);
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
TEST_F(LobWindTestFixture, SolveWithClockWindVIII) {
  ASSERT_NE(puut, nullptr);
  const int32_t kWindSpeed = 10;
  const lob::ClockAngleT kWindHeading = lob::ClockAngleT::kVIII;
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
      {0, 2720, 1264, -2.50, 0.00, 0.000},
      {150, 2596, 1151, -0.60, -0.20, 0.056},
      {300, 2476, 1047, 0.00, -0.81, 0.116},
      {600, 2245, 861, -3.18, -3.39, 0.243},
      {900, 2026, 701, -13.30, -8.00, 0.383},
      {1200, 1820, 566, -31.91, -14.98, 0.540},
      {1500, 1630, 454, -61.07, -24.70, 0.714},
      {1800, 1457, 362, -103.38, -37.55, 0.909},
      {2100, 1305, 291, -162.17, -53.89, 1.126},
      {2400, 1180, 238, -241.47, -73.94, 1.369},
      {2700, 1085, 201, -345.75, -97.57, 1.634},
      {3000, 1015, 176, -479.64, -124.35, 1.921}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  lob::Solve(kInput, kRanges, solutions);
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
TEST_F(LobWindTestFixture, SolveWithClockWindIX) {
  ASSERT_NE(puut, nullptr);
  const int32_t kWindSpeed = 10;
  const lob::ClockAngleT kWindHeading = lob::ClockAngleT::kIX;
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
      {0, 2720, 1264, -2.50, 0.00, 0.000},
      {150, 2597, 1152, -0.60, -0.23, 0.056},
      {300, 2477, 1048, 0.01, -0.93, 0.116},
      {600, 2248, 863, -3.18, -3.90, 0.243},
      {900, 2030, 704, -13.26, -9.20, 0.383},
      {1200, 1826, 569, -31.80, -17.22, 0.539},
      {1500, 1636, 457, -60.84, -28.37, 0.713},
      {1800, 1464, 366, -102.89, -43.09, 0.906},
      {2100, 1313, 294, -161.25, -61.81, 1.123},
      {2400, 1187, 241, -239.81, -84.76, 1.364},
      {2700, 1091, 203, -343.03, -111.83, 1.628},
      {3000, 1021, 178, -475.40, -142.55, 1.913}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  lob::Solve(kInput, kRanges, solutions);
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
TEST_F(LobWindTestFixture, SolveWithClockWindX) {
  ASSERT_NE(puut, nullptr);
  const int32_t kWindSpeed = 10;
  const lob::ClockAngleT kWindHeading = lob::ClockAngleT::kX;
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
      {0, 2720, 1264, -2.50, 0.00, 0.000},
      {150, 2598, 1153, -0.60, -0.20, 0.056},
      {300, 2479, 1049, 0.01, -0.80, 0.116},
      {600, 2250, 865, -3.17, -3.36, 0.242},
      {900, 2034, 707, -13.22, -7.93, 0.383},
      {1200, 1831, 573, -31.71, -14.84, 0.538},
      {1500, 1642, 461, -60.61, -24.43, 0.711},
      {1800, 1471, 370, -102.41, -37.09, 0.904},
      {2100, 1320, 298, -160.30, -53.16, 1.120},
      {2400, 1195, 244, -238.17, -72.87, 1.359},
      {2700, 1098, 206, -340.32, -96.13, 1.622},
      {3000, 1026, 180, -471.25, -122.56, 1.905}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  lob::Solve(kInput, kRanges, solutions);
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
TEST_F(LobWindTestFixture, SolveWithClockWindXI) {
  ASSERT_NE(puut, nullptr);
  const int32_t kWindSpeed = 10;
  const lob::ClockAngleT kWindHeading = lob::ClockAngleT::kXI;
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
      {0, 2720, 1264, -2.50, 0.00, 0.000},
      {150, 2598, 1153, -0.60, -0.11, 0.056},
      {300, 2480, 1050, 0.01, -0.46, 0.116},
      {600, 2252, 866, -3.16, -1.93, 0.242},
      {900, 2037, 709, -13.20, -4.57, 0.382},
      {1200, 1835, 575, -31.64, -8.54, 0.538},
      {1500, 1647, 463, -60.43, -14.05, 0.710},
      {1800, 1476, 372, -102.05, -21.32, 0.903},
      {2100, 1326, 300, -159.63, -30.54, 1.117},
      {2400, 1200, 246, -236.97, -41.85, 1.356},
      {2700, 1103, 208, -338.35, -55.20, 1.617},
      {3000, 1031, 181, -468.21, -70.38, 1.899}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  lob::Solve(kInput, kRanges, solutions);
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
TEST_F(LobWindTestFixture, SolveWithClockWindXII) {
  ASSERT_NE(puut, nullptr);
  const int32_t kWindSpeed = 10;
  const lob::ClockAngleT kWindHeading = lob::ClockAngleT::kXII;
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
      {0, 2720, 1264, -2.50, 0.00, 0.000},
      {150, 2598, 1153, -0.60, 0.00, 0.056},
      {300, 2480, 1051, 0.01, 0.00, 0.116},
      {600, 2253, 867, -3.16, 0.00, 0.242},
      {900, 2038, 709, -13.19, 0.00, 0.382},
      {1200, 1836, 576, -31.62, 0.00, 0.537},
      {1500, 1649, 464, -60.38, 0.00, 0.710},
      {1800, 1478, 373, -101.92, 0.00, 0.902},
      {2100, 1328, 301, -159.41, 0.00, 1.117},
      {2400, 1202, 247, -236.53, 0.00, 1.354},
      {2700, 1105, 208, -337.64, 0.00, 1.615},
      {3000, 1032, 182, -467.08, 0.00, 1.897}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  lob::Solve(kInput, kRanges, solutions);
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
TEST_F(LobWindTestFixture, SolveWithClockWindI) {
  ASSERT_NE(puut, nullptr);
  const int32_t kWindSpeed = 10;
  const lob::ClockAngleT kWindHeading = lob::ClockAngleT::kI;
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
      {0, 2720, 1264, -2.50, 0.00, 0.000},
      {150, 2598, 1153, -0.60, 0.11, 0.056},
      {300, 2480, 1050, 0.01, 0.46, 0.116},
      {600, 2252, 866, -3.16, 1.93, 0.242},
      {900, 2037, 709, -13.20, 4.57, 0.382},
      {1200, 1835, 575, -31.64, 8.54, 0.538},
      {1500, 1647, 463, -60.43, 14.05, 0.710},
      {1800, 1476, 372, -102.05, 21.32, 0.903},
      {2100, 1326, 300, -159.63, 30.54, 1.117},
      {2400, 1200, 246, -236.97, 41.85, 1.356},
      {2700, 1103, 208, -338.35, 55.20, 1.617},
      {3000, 1031, 181, -468.21, 70.38, 1.899}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  lob::Solve(kInput, kRanges, solutions);
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
TEST_F(LobWindTestFixture, SolveWithClockWindII) {
  ASSERT_NE(puut, nullptr);
  const int32_t kWindSpeed = 10;
  const lob::ClockAngleT kWindHeading = lob::ClockAngleT::kII;
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
      {0, 2720, 1264, -2.50, 0.00, 0.000},
      {150, 2598, 1153, -0.60, 0.20, 0.056},
      {300, 2479, 1049, 0.01, 0.80, 0.116},
      {600, 2250, 865, -3.17, 3.36, 0.242},
      {900, 2034, 707, -13.22, 7.93, 0.383},
      {1200, 1831, 573, -31.71, 14.84, 0.538},
      {1500, 1642, 461, -60.61, 24.43, 0.711},
      {1800, 1471, 370, -102.41, 37.09, 0.904},
      {2100, 1320, 298, -160.30, 53.16, 1.120},
      {2400, 1195, 244, -238.17, 72.87, 1.359},
      {2700, 1098, 206, -340.32, 96.13, 1.622},
      {3000, 1026, 180, -471.25, 122.56, 1.905}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  lob::Solve(kInput, kRanges, solutions);
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
TEST_F(LobWindTestFixture, SolveWithAngleWind150) {
  ASSERT_NE(puut, nullptr);
  const int32_t kWindSpeed = 20;
  const double kWindHeading = 150;
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
      {0, 2720, 1264, -2.50, 0.00, 0.000},
      {150, 2595, 1150, -0.60, 0.23, 0.056},
      {300, 2473, 1044, 0.00, 0.94, 0.116},
      {600, 2238, 856, -3.20, 3.94, 0.243},
      {900, 2016, 694, -13.38, 9.34, 0.384},
      {1200, 1808, 558, -32.15, 17.51, 0.541},
      {1500, 1614, 445, -61.66, 28.91, 0.717},
      {1800, 1439, 354, -104.63, 44.03, 0.914},
      {2100, 1286, 283, -164.57, 63.30, 1.135},
      {2400, 1162, 231, -245.69, 86.93, 1.381},
      {2700, 1069, 195, -352.75, 114.74, 1.651},
      {3000, 1001, 171, -490.37, 146.17, 1.941}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  lob::Solve(kInput, kRanges, solutions);
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
TEST_F(LobWindTestFixture, SolveWithAngleWindNegativeMagnitude) {
  ASSERT_NE(puut, nullptr);
  const int32_t kWindSpeed = -20;
  const double kWindHeading = 330;
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
      {0, 2720, 1264, -2.50, 0.00, 0.000},
      {150, 2595, 1150, -0.60, 0.23, 0.056},
      {300, 2473, 1044, 0.00, 0.94, 0.116},
      {600, 2238, 856, -3.20, 3.94, 0.243},
      {900, 2016, 694, -13.38, 9.34, 0.384},
      {1200, 1808, 558, -32.15, 17.51, 0.541},
      {1500, 1614, 445, -61.66, 28.91, 0.717},
      {1800, 1439, 354, -104.63, 44.03, 0.914},
      {2100, 1286, 283, -164.57, 63.30, 1.135},
      {2400, 1162, 231, -245.69, 86.93, 1.381},
      {2700, 1069, 195, -352.75, 114.74, 1.651},
      {3000, 1001, 171, -490.37, 146.17, 1.941}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  lob::Solve(kInput, kRanges, solutions);
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
TEST_F(LobWindTestFixture, SolveWithAngleWindNegativeAngle) {
  ASSERT_NE(puut, nullptr);
  const int32_t kWindSpeed = 20;
  const double kWindHeading = -210;
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
      {0, 2720, 1264, -2.50, 0.00, 0.000},
      {150, 2595, 1150, -0.60, 0.23, 0.056},
      {300, 2473, 1044, 0.00, 0.94, 0.116},
      {600, 2238, 856, -3.20, 3.94, 0.243},
      {900, 2016, 694, -13.38, 9.34, 0.384},
      {1200, 1808, 558, -32.15, 17.51, 0.541},
      {1500, 1614, 445, -61.66, 28.91, 0.717},
      {1800, 1439, 354, -104.63, 44.03, 0.914},
      {2100, 1286, 283, -164.57, 63.30, 1.135},
      {2400, 1162, 231, -245.69, 86.93, 1.381},
      {2700, 1069, 195, -352.75, 114.74, 1.651},
      {3000, 1001, 171, -490.37, 146.17, 1.941}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  lob::Solve(kInput, kRanges, solutions);
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