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

#include "eng_units.hpp"
#include "lob/lob.hpp"
#include "testing.hpp"

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

    puut->BallisticCoefficientPsi(kTestBC)
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
  const double kZeroRange = 100.0;
  auto input2 = puut->ZeroAngleMOA(std::numeric_limits<double>::quiet_NaN())
                    .ZeroDistanceYds(kZeroRange)
                    .Build();
  const double kError = 0.01;
  EXPECT_NEAR(input1.zero_angle, input2.zero_angle, kError);
}

TEST_F(LobWindTestFixture, GetSpeedOfSoundFps) {
  ASSERT_NE(puut, nullptr);
  const auto kContext = puut->Build();
  const double kExpectedFps = 1116.45;
  const double kError = 0.001;
  EXPECT_NEAR(kContext.speed_of_sound, kExpectedFps, kError);
}

TEST_F(LobWindTestFixture, SolveWithoutWind) {
  ASSERT_NE(puut, nullptr);
  constexpr lob::FpsT kVelocityError{1};
  constexpr lob::FtLbsT kEnergyError{5};
  constexpr lob::MoaT kMoaError{0.1};
  constexpr lob::InchT kInchError{0.1};
  constexpr lob::SecT kTimeOfFlightError{0.01};
  constexpr size_t kSolutionLength = 12;
  const auto kContext = puut->Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0, 150, 300, 600, 900, 1200, 1500, 1800, 2100, 2400, 2700, 3000};
  const std::vector<lob::Output> kExpected = {
      {0, 2720, 1264, -2.50, 0.00, 0.000},
      {150, 2597, 1152, -0.60, 0.00, 0.056},
      {300, 2477, 1048, 0.00, 0.00, 0.116},
      {600, 2248, 863, -3.18, 0.00, 0.243},
      {900, 2030, 704, -13.26, 0.00, 0.383},
      {1200, 1826, 569, -31.81, 0.00, 0.539},
      {1500, 1636, 457, -60.85, 0.00, 0.713},
      {1800, 1464, 366, -102.90, 0.00, 0.906},
      {2100, 1313, 294, -161.26, 0.00, 1.123},
      {2400, 1188, 241, -239.80, 0.00, 1.364},
      {2700, 1092, 204, -343.02, 0.00, 1.628},
      {3000, 1021, 178, -475.39, 0.00, 1.913}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  const size_t kSize = lob::Solve(kContext, kRanges, &solutions);
  EXPECT_EQ(kSize, kSolutionLength);
  VerifySolutions(solutions, kExpected,
                  {kVelocityError, kEnergyError, kMoaError, kInchError,
                   kTimeOfFlightError});
}

TEST_F(LobWindTestFixture, SolveWithClockWindIII) {
  ASSERT_NE(puut, nullptr);
  const int32_t kWindSpeed = 10;
  const lob::ClockAngleT kWindHeading = lob::ClockAngleT::kIII;
  constexpr lob::FpsT kVelocityError{1};
  constexpr lob::FtLbsT kEnergyError{5};
  constexpr lob::MoaT kMoaError{0.1};
  constexpr lob::InchT kInchError{0.1};
  constexpr lob::SecT kTimeOfFlightError{0.01};
  constexpr size_t kSolutionLength = 12;
  const auto kContext =
      puut->WindSpeedMph(kWindSpeed).WindHeading(kWindHeading).Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0, 150, 300, 600, 900, 1200, 1500, 1800, 2100, 2400, 2700, 3000};
  const std::vector<lob::Output> kExpected = {
      {0, 2720, 1265, -2.50, 0.00, 0.000},
      {150, 2597, 1153, -0.60, 0.23, 0.056},
      {300, 2477, 1049, 0.01, 0.93, 0.116},
      {600, 2248, 864, -3.17, 3.91, 0.243},
      {900, 2030, 705, -13.25, 9.22, 0.383},
      {1200, 1826, 570, -31.80, 17.16, 0.539},
      {1500, 1636, 458, -60.83, 27.90, 0.713},
      {1800, 1464, 366, -102.89, 41.52, 0.907},
      {2100, 1313, 295, -161.23, 58.32, 1.123},
      {2400, 1187, 241, -239.80, 78.48, 1.364},
      {2700, 1091, 204, -343.04, 101.92, 1.628},
      {3000, 1020, 178, -475.47, 128.27, 1.913}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  const size_t kSize = lob::Solve(kContext, kRanges, &solutions);
  EXPECT_EQ(kSize, kSolutionLength);
  VerifySolutions(solutions, kExpected,
                  {kVelocityError, kEnergyError, kMoaError, kInchError,
                   kTimeOfFlightError});
}

TEST_F(LobWindTestFixture, ZeroShearExponentMatchesUnscaledGoldens) {
  // Shear exponent 0 disables scaling exactly: the solve must reproduce the
  // pre-scaling golden table below, pinning refactor integrity against the
  // default-alpha regeneration that follows.
  ASSERT_NE(puut, nullptr);
  const int32_t kWindSpeed = 10;
  const lob::ClockAngleT kWindHeading = lob::ClockAngleT::kIII;
  constexpr lob::FpsT kVelocityError{1};
  constexpr lob::FtLbsT kEnergyError{5};
  constexpr lob::MoaT kMoaError{0.1};
  constexpr lob::InchT kInchError{0.1};
  constexpr lob::SecT kTimeOfFlightError{0.01};
  constexpr size_t kSolutionLength = 12;
  const auto kContext = puut->WindSpeedMph(kWindSpeed)
                            .WindHeading(kWindHeading)
                            .WindShearExponent(0.0)
                            .Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0, 150, 300, 600, 900, 1200, 1500, 1800, 2100, 2400, 2700, 3000};
  const std::vector<lob::Output> kExpected = {
      {0, 2720, 1264, -2.50, 0.00, 0.000},
      {150, 2597, 1152, -0.60, 0.23, 0.056},
      {300, 2477, 1048, 0.00, 0.93, 0.116},
      {600, 2248, 863, -3.18, 3.90, 0.243},
      {900, 2030, 704, -13.26, 9.20, 0.383},
      {1200, 1826, 569, -31.81, 17.22, 0.539},
      {1500, 1636, 457, -60.85, 28.37, 0.713},
      {1800, 1464, 366, -102.90, 43.09, 0.906},
      {2100, 1313, 294, -161.26, 61.81, 1.123},
      {2400, 1188, 241, -239.80, 84.75, 1.364},
      {2700, 1092, 204, -343.02, 111.82, 1.628},
      {3000, 1021, 178, -475.39, 142.53, 1.913}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  const size_t kSize = lob::Solve(kContext, kRanges, &solutions);
  EXPECT_EQ(kSize, kSolutionLength);
  VerifySolutions(solutions, kExpected,
                  {kVelocityError, kEnergyError, kMoaError, kInchError,
                   kTimeOfFlightError});
}

TEST_F(LobWindTestFixture, SolveWithClockWindIV) {
  ASSERT_NE(puut, nullptr);
  const int32_t kWindSpeed = 10;
  const lob::ClockAngleT kWindHeading = lob::ClockAngleT::kIV;
  constexpr lob::FpsT kVelocityError{1};
  constexpr lob::FtLbsT kEnergyError{5};
  constexpr lob::MoaT kMoaError{0.1};
  constexpr lob::InchT kInchError{0.1};
  constexpr lob::SecT kTimeOfFlightError{0.01};
  constexpr size_t kSolutionLength = 12;
  const auto kContext =
      puut->WindSpeedMph(kWindSpeed).WindHeading(kWindHeading).Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0, 150, 300, 600, 900, 1200, 1500, 1800, 2100, 2400, 2700, 3000};
  const std::vector<lob::Output> kExpected = {
      {0, 2720, 1265, -2.50, 0.00, 0.000},
      {150, 2596, 1152, -0.60, 0.20, 0.056},
      {300, 2476, 1048, 0.00, 0.81, 0.116},
      {600, 2245, 862, -3.18, 3.40, 0.243},
      {900, 2026, 702, -13.29, 8.02, 0.384},
      {1200, 1821, 567, -31.90, 14.93, 0.540},
      {1500, 1630, 454, -61.06, 24.29, 0.714},
      {1800, 1457, 363, -103.37, 36.17, 0.909},
      {2100, 1306, 291, -162.13, 50.82, 1.126},
      {2400, 1181, 238, -241.35, 68.41, 1.368},
      {2700, 1085, 201, -345.56, 88.85, 1.634},
      {3000, 1015, 176, -479.31, 111.80, 1.920}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  const size_t kSize = lob::Solve(kContext, kRanges, &solutions);
  EXPECT_EQ(kSize, kSolutionLength);
  VerifySolutions(solutions, kExpected,
                  {kVelocityError, kEnergyError, kMoaError, kInchError,
                   kTimeOfFlightError});
}

TEST_F(LobWindTestFixture, SolveWithClockWindV) {
  ASSERT_NE(puut, nullptr);
  const int32_t kWindSpeed = 10;
  const lob::ClockAngleT kWindHeading = lob::ClockAngleT::kV;
  constexpr lob::FpsT kVelocityError{1};
  constexpr lob::FtLbsT kEnergyError{5};
  constexpr lob::MoaT kMoaError{0.1};
  constexpr lob::InchT kInchError{0.1};
  constexpr lob::SecT kTimeOfFlightError{0.01};
  constexpr size_t kSolutionLength = 12;
  const auto kContext =
      puut->WindSpeedMph(kWindSpeed).WindHeading(kWindHeading).Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0, 150, 300, 600, 900, 1200, 1500, 1800, 2100, 2400, 2700, 3000};
  const std::vector<lob::Output> kExpected = {
      {0, 2720, 1265, -2.50, 0.00, 0.000},
      {150, 2596, 1152, -0.60, 0.11, 0.056},
      {300, 2475, 1047, 0.00, 0.47, 0.116},
      {600, 2243, 860, -3.18, 1.97, 0.243},
      {900, 2023, 700, -13.31, 4.64, 0.384},
      {1200, 1817, 564, -31.98, 8.65, 0.540},
      {1500, 1626, 452, -61.23, 14.08, 0.715},
      {1800, 1452, 361, -103.72, 20.97, 0.910},
      {2100, 1301, 289, -162.79, 29.47, 1.129},
      {2400, 1176, 236, -242.50, 39.68, 1.372},
      {2700, 1081, 200, -347.43, 51.54, 1.638},
      {3000, 1012, 175, -482.15, 64.85, 1.926}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  const size_t kSize = lob::Solve(kContext, kRanges, &solutions);
  EXPECT_EQ(kSize, kSolutionLength);
  VerifySolutions(solutions, kExpected,
                  {kVelocityError, kEnergyError, kMoaError, kInchError,
                   kTimeOfFlightError});
}

TEST_F(LobWindTestFixture, SolveWithClockWindVI) {
  ASSERT_NE(puut, nullptr);
  const int32_t kWindSpeed = 10;
  const lob::ClockAngleT kWindHeading = lob::ClockAngleT::kVI;
  constexpr lob::FpsT kVelocityError{1};
  constexpr lob::FtLbsT kEnergyError{5};
  constexpr lob::MoaT kMoaError{0.1};
  constexpr lob::InchT kInchError{0.1};
  constexpr lob::SecT kTimeOfFlightError{0.01};
  constexpr size_t kSolutionLength = 12;
  const auto kContext =
      puut->WindSpeedMph(kWindSpeed).WindHeading(kWindHeading).Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0, 150, 300, 600, 900, 1200, 1500, 1800, 2100, 2400, 2700, 3000};
  const std::vector<lob::Output> kExpected = {
      {0, 2720, 1265, -2.50, 0.00, 0.000},
      {150, 2596, 1152, -0.60, -0.00, 0.056},
      {300, 2475, 1047, 0.00, -0.00, 0.116},
      {600, 2242, 860, -3.19, -0.00, 0.243},
      {900, 2022, 699, -13.32, -0.00, 0.384},
      {1200, 1816, 563, -32.00, -0.00, 0.540},
      {1500, 1624, 451, -61.29, -0.00, 0.715},
      {1800, 1451, 360, -103.85, -0.00, 0.911},
      {2100, 1299, 288, -163.03, -0.00, 1.130},
      {2400, 1174, 236, -242.93, -0.00, 1.373},
      {2700, 1080, 199, -348.12, -0.00, 1.640},
      {3000, 1011, 175, -483.19, -0.00, 1.928}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  const size_t kSize = lob::Solve(kContext, kRanges, &solutions);
  EXPECT_EQ(kSize, kSolutionLength);
  VerifySolutions(solutions, kExpected,
                  {kVelocityError, kEnergyError, kMoaError, kInchError,
                   kTimeOfFlightError});
}

TEST_F(LobWindTestFixture, SolveWithClockWindVII) {
  ASSERT_NE(puut, nullptr);
  const int32_t kWindSpeed = 10;
  const lob::ClockAngleT kWindHeading = lob::ClockAngleT::kVII;
  constexpr lob::FpsT kVelocityError{1};
  constexpr lob::FtLbsT kEnergyError{5};
  constexpr lob::MoaT kMoaError{0.1};
  constexpr lob::InchT kInchError{0.1};
  constexpr lob::SecT kTimeOfFlightError{0.01};
  constexpr size_t kSolutionLength = 12;
  const auto kContext =
      puut->WindSpeedMph(kWindSpeed).WindHeading(kWindHeading).Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0, 150, 300, 600, 900, 1200, 1500, 1800, 2100, 2400, 2700, 3000};
  const std::vector<lob::Output> kExpected = {
      {0, 2720, 1265, -2.50, 0.00, 0.000},
      {150, 2596, 1152, -0.60, -0.11, 0.056},
      {300, 2475, 1047, 0.00, -0.47, 0.116},
      {600, 2243, 860, -3.18, -1.97, 0.243},
      {900, 2023, 700, -13.31, -4.64, 0.384},
      {1200, 1817, 564, -31.98, -8.65, 0.540},
      {1500, 1626, 452, -61.23, -14.08, 0.715},
      {1800, 1452, 361, -103.72, -20.97, 0.910},
      {2100, 1301, 289, -162.79, -29.47, 1.129},
      {2400, 1176, 236, -242.50, -39.68, 1.372},
      {2700, 1081, 200, -347.43, -51.54, 1.638},
      {3000, 1012, 175, -482.15, -64.85, 1.926}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  const size_t kSize = lob::Solve(kContext, kRanges, &solutions);
  EXPECT_EQ(kSize, kSolutionLength);
  VerifySolutions(solutions, kExpected,
                  {kVelocityError, kEnergyError, kMoaError, kInchError,
                   kTimeOfFlightError});
}

TEST_F(LobWindTestFixture, SolveWithClockWindVIII) {
  ASSERT_NE(puut, nullptr);
  const int32_t kWindSpeed = 10;
  const lob::ClockAngleT kWindHeading = lob::ClockAngleT::kVIII;
  constexpr lob::FpsT kVelocityError{1};
  constexpr lob::FtLbsT kEnergyError{5};
  constexpr lob::MoaT kMoaError{0.1};
  constexpr lob::InchT kInchError{0.1};
  constexpr lob::SecT kTimeOfFlightError{0.01};
  constexpr size_t kSolutionLength = 12;
  const auto kContext =
      puut->WindSpeedMph(kWindSpeed).WindHeading(kWindHeading).Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0, 150, 300, 600, 900, 1200, 1500, 1800, 2100, 2400, 2700, 3000};
  const std::vector<lob::Output> kExpected = {
      {0, 2720, 1265, -2.50, 0.00, 0.000},
      {150, 2596, 1152, -0.60, -0.20, 0.056},
      {300, 2476, 1048, 0.00, -0.81, 0.116},
      {600, 2245, 862, -3.18, -3.40, 0.243},
      {900, 2026, 702, -13.29, -8.02, 0.384},
      {1200, 1821, 567, -31.90, -14.93, 0.540},
      {1500, 1630, 454, -61.06, -24.29, 0.714},
      {1800, 1457, 363, -103.37, -36.17, 0.909},
      {2100, 1306, 291, -162.13, -50.82, 1.126},
      {2400, 1181, 238, -241.35, -68.41, 1.368},
      {2700, 1085, 201, -345.56, -88.85, 1.634},
      {3000, 1015, 176, -479.31, -111.80, 1.920}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  const size_t kSize = lob::Solve(kContext, kRanges, &solutions);
  EXPECT_EQ(kSize, kSolutionLength);
  VerifySolutions(solutions, kExpected,
                  {kVelocityError, kEnergyError, kMoaError, kInchError,
                   kTimeOfFlightError});
}

TEST_F(LobWindTestFixture, SolveWithClockWindIX) {
  ASSERT_NE(puut, nullptr);
  const int32_t kWindSpeed = 10;
  const lob::ClockAngleT kWindHeading = lob::ClockAngleT::kIX;
  constexpr lob::FpsT kVelocityError{1};
  constexpr lob::FtLbsT kEnergyError{5};
  constexpr lob::MoaT kMoaError{0.1};
  constexpr lob::InchT kInchError{0.1};
  constexpr lob::SecT kTimeOfFlightError{0.01};
  constexpr size_t kSolutionLength = 12;
  const auto kContext =
      puut->WindSpeedMph(kWindSpeed).WindHeading(kWindHeading).Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0, 150, 300, 600, 900, 1200, 1500, 1800, 2100, 2400, 2700, 3000};
  const std::vector<lob::Output> kExpected = {
      {0, 2720, 1265, -2.50, 0.00, 0.000},
      {150, 2597, 1153, -0.60, -0.23, 0.056},
      {300, 2477, 1049, 0.01, -0.93, 0.116},
      {600, 2248, 864, -3.17, -3.91, 0.243},
      {900, 2030, 705, -13.25, -9.22, 0.383},
      {1200, 1826, 570, -31.80, -17.16, 0.539},
      {1500, 1636, 458, -60.83, -27.90, 0.713},
      {1800, 1464, 366, -102.89, -41.52, 0.907},
      {2100, 1313, 295, -161.23, -58.32, 1.123},
      {2400, 1187, 241, -239.80, -78.48, 1.364},
      {2700, 1091, 204, -343.04, -101.92, 1.628},
      {3000, 1020, 178, -475.47, -128.27, 1.913}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  const size_t kSize = lob::Solve(kContext, kRanges, &solutions);
  EXPECT_EQ(kSize, kSolutionLength);
  VerifySolutions(solutions, kExpected,
                  {kVelocityError, kEnergyError, kMoaError, kInchError,
                   kTimeOfFlightError});
}

TEST_F(LobWindTestFixture, SolveWithClockWindX) {
  ASSERT_NE(puut, nullptr);
  const int32_t kWindSpeed = 10;
  const lob::ClockAngleT kWindHeading = lob::ClockAngleT::kX;
  constexpr lob::FpsT kVelocityError{1};
  constexpr lob::FtLbsT kEnergyError{5};
  constexpr lob::MoaT kMoaError{0.1};
  constexpr lob::InchT kInchError{0.1};
  constexpr lob::SecT kTimeOfFlightError{0.01};
  constexpr size_t kSolutionLength = 12;
  const auto kContext =
      puut->WindSpeedMph(kWindSpeed).WindHeading(kWindHeading).Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0, 150, 300, 600, 900, 1200, 1500, 1800, 2100, 2400, 2700, 3000};
  const std::vector<lob::Output> kExpected = {
      {0, 2720, 1265, -2.50, 0.00, 0.000},
      {150, 2598, 1154, -0.60, -0.20, 0.056},
      {300, 2479, 1050, 0.01, -0.80, 0.116},
      {600, 2250, 866, -3.16, -3.37, 0.243},
      {900, 2034, 707, -13.22, -7.95, 0.383},
      {1200, 1831, 573, -31.71, -14.79, 0.538},
      {1500, 1642, 461, -60.60, -24.03, 0.711},
      {1800, 1470, 370, -102.42, -35.75, 0.905},
      {2100, 1319, 298, -160.35, -50.19, 1.120},
      {2400, 1194, 244, -238.27, -67.53, 1.360},
      {2700, 1097, 206, -340.55, -87.69, 1.622},
      {3000, 1025, 180, -471.68, -110.37, 1.906}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  const size_t kSize = lob::Solve(kContext, kRanges, &solutions);
  EXPECT_EQ(kSize, kSolutionLength);
  VerifySolutions(solutions, kExpected,
                  {kVelocityError, kEnergyError, kMoaError, kInchError,
                   kTimeOfFlightError});
}

TEST_F(LobWindTestFixture, SolveWithClockWindXI) {
  ASSERT_NE(puut, nullptr);
  const int32_t kWindSpeed = 10;
  const lob::ClockAngleT kWindHeading = lob::ClockAngleT::kXI;
  constexpr lob::FpsT kVelocityError{1};
  constexpr lob::FtLbsT kEnergyError{5};
  constexpr lob::MoaT kMoaError{0.1};
  constexpr lob::InchT kInchError{0.1};
  constexpr lob::SecT kTimeOfFlightError{0.01};
  constexpr size_t kSolutionLength = 12;
  const auto kContext =
      puut->WindSpeedMph(kWindSpeed).WindHeading(kWindHeading).Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0, 150, 300, 600, 900, 1200, 1500, 1800, 2100, 2400, 2700, 3000};
  const std::vector<lob::Output> kExpected = {
      {0, 2720, 1265, -2.50, 0.00, 0.000},
      {150, 2598, 1154, -0.60, -0.11, 0.056},
      {300, 2480, 1051, 0.01, -0.46, 0.116},
      {600, 2252, 867, -3.16, -1.94, 0.242},
      {900, 2037, 709, -13.20, -4.58, 0.383},
      {1200, 1835, 575, -31.64, -8.51, 0.538},
      {1500, 1646, 463, -60.43, -13.82, 0.710},
      {1800, 1475, 372, -102.07, -20.56, 0.903},
      {2100, 1324, 300, -159.71, -28.85, 1.118},
      {2400, 1198, 246, -237.16, -38.80, 1.356},
      {2700, 1101, 207, -338.75, -50.38, 1.618},
      {3000, 1029, 181, -468.93, -63.42, 1.901}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  const size_t kSize = lob::Solve(kContext, kRanges, &solutions);
  EXPECT_EQ(kSize, kSolutionLength);
  VerifySolutions(solutions, kExpected,
                  {kVelocityError, kEnergyError, kMoaError, kInchError,
                   kTimeOfFlightError});
}

TEST_F(LobWindTestFixture, SolveWithClockWindXII) {
  ASSERT_NE(puut, nullptr);
  const int32_t kWindSpeed = 10;
  const lob::ClockAngleT kWindHeading = lob::ClockAngleT::kXII;
  constexpr lob::FpsT kVelocityError{1};
  constexpr lob::FtLbsT kEnergyError{5};
  constexpr lob::MoaT kMoaError{0.1};
  constexpr lob::InchT kInchError{0.1};
  constexpr lob::SecT kTimeOfFlightError{0.01};
  constexpr size_t kSolutionLength = 12;
  const auto kContext =
      puut->WindSpeedMph(kWindSpeed).WindHeading(kWindHeading).Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0, 150, 300, 600, 900, 1200, 1500, 1800, 2100, 2400, 2700, 3000};
  const std::vector<lob::Output> kExpected = {
      {0, 2720, 1265, -2.50, 0.00, 0.000},
      {150, 2598, 1154, -0.60, 0.00, 0.056},
      {300, 2480, 1051, 0.01, 0.00, 0.116},
      {600, 2253, 868, -3.16, 0.00, 0.242},
      {900, 2038, 710, -13.19, 0.00, 0.382},
      {1200, 1836, 576, -31.61, 0.00, 0.538},
      {1500, 1648, 464, -60.37, 0.00, 0.710},
      {1800, 1477, 373, -101.95, 0.00, 0.902},
      {2100, 1326, 301, -159.48, 0.00, 1.117},
      {2400, 1200, 246, -236.76, 0.00, 1.355},
      {2700, 1102, 208, -338.10, 0.00, 1.617},
      {3000, 1030, 181, -467.93, 0.00, 1.899}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  const size_t kSize = lob::Solve(kContext, kRanges, &solutions);
  EXPECT_EQ(kSize, kSolutionLength);
  VerifySolutions(solutions, kExpected,
                  {kVelocityError, kEnergyError, kMoaError, kInchError,
                   kTimeOfFlightError});
}

TEST_F(LobWindTestFixture, SolveWithClockWindI) {
  ASSERT_NE(puut, nullptr);
  const int32_t kWindSpeed = 10;
  const lob::ClockAngleT kWindHeading = lob::ClockAngleT::kI;
  constexpr lob::FpsT kVelocityError{1};
  constexpr lob::FtLbsT kEnergyError{5};
  constexpr lob::MoaT kMoaError{0.1};
  constexpr lob::InchT kInchError{0.1};
  constexpr lob::SecT kTimeOfFlightError{0.01};
  constexpr size_t kSolutionLength = 12;
  const auto kContext =
      puut->WindSpeedMph(kWindSpeed).WindHeading(kWindHeading).Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0, 150, 300, 600, 900, 1200, 1500, 1800, 2100, 2400, 2700, 3000};
  const std::vector<lob::Output> kExpected = {
      {0, 2720, 1265, -2.50, 0.00, 0.000},
      {150, 2598, 1154, -0.60, 0.11, 0.056},
      {300, 2480, 1051, 0.01, 0.46, 0.116},
      {600, 2252, 867, -3.16, 1.94, 0.242},
      {900, 2037, 709, -13.20, 4.58, 0.383},
      {1200, 1835, 575, -31.64, 8.51, 0.538},
      {1500, 1646, 463, -60.43, 13.82, 0.710},
      {1800, 1475, 372, -102.07, 20.56, 0.903},
      {2100, 1324, 300, -159.71, 28.85, 1.118},
      {2400, 1198, 246, -237.16, 38.80, 1.356},
      {2700, 1101, 207, -338.75, 50.38, 1.618},
      {3000, 1029, 181, -468.93, 63.42, 1.901}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  const size_t kSize = lob::Solve(kContext, kRanges, &solutions);
  EXPECT_EQ(kSize, kSolutionLength);
  VerifySolutions(solutions, kExpected,
                  {kVelocityError, kEnergyError, kMoaError, kInchError,
                   kTimeOfFlightError});
}

TEST_F(LobWindTestFixture, SolveWithClockWindII) {
  ASSERT_NE(puut, nullptr);
  const int32_t kWindSpeed = 10;
  const lob::ClockAngleT kWindHeading = lob::ClockAngleT::kII;
  constexpr lob::FpsT kVelocityError{1};
  constexpr lob::FtLbsT kEnergyError{5};
  constexpr lob::MoaT kMoaError{0.1};
  constexpr lob::InchT kInchError{0.1};
  constexpr lob::SecT kTimeOfFlightError{0.01};
  constexpr size_t kSolutionLength = 12;
  const auto kContext =
      puut->WindSpeedMph(kWindSpeed).WindHeading(kWindHeading).Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0, 150, 300, 600, 900, 1200, 1500, 1800, 2100, 2400, 2700, 3000};
  const std::vector<lob::Output> kExpected = {
      {0, 2720, 1265, -2.50, 0.00, 0.000},
      {150, 2598, 1154, -0.60, 0.20, 0.056},
      {300, 2479, 1050, 0.01, 0.80, 0.116},
      {600, 2250, 866, -3.16, 3.37, 0.243},
      {900, 2034, 707, -13.22, 7.95, 0.383},
      {1200, 1831, 573, -31.71, 14.79, 0.538},
      {1500, 1642, 461, -60.60, 24.03, 0.711},
      {1800, 1470, 370, -102.42, 35.75, 0.905},
      {2100, 1319, 298, -160.35, 50.19, 1.120},
      {2400, 1194, 244, -238.27, 67.53, 1.360},
      {2700, 1097, 206, -340.55, 87.69, 1.622},
      {3000, 1025, 180, -471.68, 110.37, 1.906}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  const size_t kSize = lob::Solve(kContext, kRanges, &solutions);
  EXPECT_EQ(kSize, kSolutionLength);
  VerifySolutions(solutions, kExpected,
                  {kVelocityError, kEnergyError, kMoaError, kInchError,
                   kTimeOfFlightError});
}

TEST_F(LobWindTestFixture, SolveWithAngleWind150) {
  ASSERT_NE(puut, nullptr);
  const int32_t kWindSpeed = 20;
  const double kWindHeading = 150;
  constexpr lob::FpsT kVelocityError{1};
  constexpr lob::FtLbsT kEnergyError{5};
  constexpr lob::MoaT kMoaError{0.1};
  constexpr lob::InchT kInchError{0.1};
  constexpr lob::SecT kTimeOfFlightError{0.01};
  constexpr size_t kSolutionLength = 12;
  const auto kContext =
      puut->WindSpeedMph(kWindSpeed).WindHeadingDeg(kWindHeading).Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0, 150, 300, 600, 900, 1200, 1500, 1800, 2100, 2400, 2700, 3000};
  const std::vector<lob::Output> kExpected = {
      {0, 2720, 1265, -2.50, 0.00, 0.000},
      {150, 2595, 1151, -0.60, 0.23, 0.056},
      {300, 2473, 1045, 0.00, 0.94, 0.116},
      {600, 2238, 856, -3.20, 3.96, 0.243},
      {900, 2016, 695, -13.37, 9.35, 0.384},
      {1200, 1808, 559, -32.15, 17.44, 0.542},
      {1500, 1615, 446, -61.64, 28.41, 0.717},
      {1800, 1441, 355, -104.57, 42.36, 0.914},
      {2100, 1289, 284, -164.38, 59.59, 1.134},
      {2400, 1165, 232, -245.28, 80.27, 1.380},
      {2700, 1072, 196, -351.93, 104.26, 1.649},
      {3000, 1004, 172, -488.99, 131.14, 1.939}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  const size_t kSize = lob::Solve(kContext, kRanges, &solutions);
  EXPECT_EQ(kSize, kSolutionLength);
  VerifySolutions(solutions, kExpected,
                  {kVelocityError, kEnergyError, kMoaError, kInchError,
                   kTimeOfFlightError});
}

TEST_F(LobWindTestFixture, SolveWithAngleWindNegativeMagnitude) {
  ASSERT_NE(puut, nullptr);
  const int32_t kWindSpeed = -20;
  const double kWindHeading = 330;
  constexpr lob::FpsT kVelocityError{1};
  constexpr lob::FtLbsT kEnergyError{5};
  constexpr lob::MoaT kMoaError{0.1};
  constexpr lob::InchT kInchError{0.1};
  constexpr lob::SecT kTimeOfFlightError{0.01};
  constexpr size_t kSolutionLength = 12;
  const auto kContext =
      puut->WindSpeedMph(kWindSpeed).WindHeadingDeg(kWindHeading).Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0, 150, 300, 600, 900, 1200, 1500, 1800, 2100, 2400, 2700, 3000};
  const std::vector<lob::Output> kExpected = {
      {0, 2720, 1265, -2.50, 0.00, 0.000},
      {150, 2595, 1151, -0.60, 0.23, 0.056},
      {300, 2473, 1045, 0.00, 0.94, 0.116},
      {600, 2238, 856, -3.20, 3.96, 0.243},
      {900, 2016, 695, -13.37, 9.35, 0.384},
      {1200, 1808, 559, -32.15, 17.44, 0.542},
      {1500, 1615, 446, -61.64, 28.41, 0.717},
      {1800, 1441, 355, -104.57, 42.36, 0.914},
      {2100, 1289, 284, -164.38, 59.59, 1.134},
      {2400, 1165, 232, -245.28, 80.27, 1.380},
      {2700, 1072, 196, -351.93, 104.26, 1.649},
      {3000, 1004, 172, -488.99, 131.14, 1.939}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  const size_t kSize = lob::Solve(kContext, kRanges, &solutions);
  EXPECT_EQ(kSize, kSolutionLength);
  VerifySolutions(solutions, kExpected,
                  {kVelocityError, kEnergyError, kMoaError, kInchError,
                   kTimeOfFlightError});
}

TEST_F(LobWindTestFixture, SolveWithAngleWindNegativeAngle) {
  ASSERT_NE(puut, nullptr);
  const int32_t kWindSpeed = 20;
  const double kWindHeading = -210;
  constexpr lob::FpsT kVelocityError{1};
  constexpr lob::FtLbsT kEnergyError{5};
  constexpr lob::MoaT kMoaError{0.1};
  constexpr lob::InchT kInchError{0.1};
  constexpr lob::SecT kTimeOfFlightError{0.01};
  constexpr size_t kSolutionLength = 12;
  const auto kContext =
      puut->WindSpeedMph(kWindSpeed).WindHeadingDeg(kWindHeading).Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0, 150, 300, 600, 900, 1200, 1500, 1800, 2100, 2400, 2700, 3000};
  const std::vector<lob::Output> kExpected = {
      {0, 2720, 1265, -2.50, 0.00, 0.000},
      {150, 2595, 1151, -0.60, 0.23, 0.056},
      {300, 2473, 1045, 0.00, 0.94, 0.116},
      {600, 2238, 856, -3.20, 3.96, 0.243},
      {900, 2016, 695, -13.37, 9.35, 0.384},
      {1200, 1808, 559, -32.15, 17.44, 0.542},
      {1500, 1615, 446, -61.64, 28.41, 0.717},
      {1800, 1441, 355, -104.57, 42.36, 0.914},
      {2100, 1289, 284, -164.38, 59.59, 1.134},
      {2400, 1165, 232, -245.28, 80.27, 1.380},
      {2700, 1072, 196, -351.93, 104.26, 1.649},
      {3000, 1004, 172, -488.99, 131.14, 1.939}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  const size_t kSize = lob::Solve(kContext, kRanges, &solutions);
  EXPECT_EQ(kSize, kSolutionLength);
  VerifySolutions(solutions, kExpected,
                  {kVelocityError, kEnergyError, kMoaError, kInchError,
                   kTimeOfFlightError});
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