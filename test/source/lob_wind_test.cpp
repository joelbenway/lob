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
      {300, 2477, 1049, 0.01, 0.94, 0.116},
      {600, 2248, 864, -3.17, 3.97, 0.243},
      {900, 2030, 705, -13.25, 9.34, 0.383},
      {1200, 1826, 570, -31.80, 17.43, 0.539},
      {1500, 1636, 458, -60.83, 28.65, 0.713},
      {1800, 1464, 366, -102.89, 43.45, 0.907},
      {2100, 1313, 295, -161.23, 62.24, 1.123},
      {2400, 1187, 241, -239.80, 85.27, 1.364},
      {2700, 1091, 204, -343.04, 112.43, 1.628},
      {3000, 1020, 178, -475.47, 143.24, 1.913}};

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
      {300, 2476, 1048, 0.00, 0.82, 0.116},
      {600, 2245, 862, -3.18, 3.45, 0.243},
      {900, 2026, 702, -13.29, 8.13, 0.384},
      {1200, 1821, 567, -31.90, 15.17, 0.540},
      {1500, 1630, 454, -61.07, 24.95, 0.714},
      {1800, 1457, 363, -103.39, 37.86, 0.909},
      {2100, 1305, 291, -162.18, 54.27, 1.127},
      {2400, 1180, 238, -241.48, 74.39, 1.369},
      {2700, 1085, 201, -345.81, 98.09, 1.635},
      {3000, 1015, 176, -479.74, 124.95, 1.921}};

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
      {150, 2596, 1152, -0.60, 0.12, 0.056},
      {300, 2475, 1047, 0.00, 0.47, 0.116},
      {600, 2243, 860, -3.18, 2.00, 0.243},
      {900, 2023, 700, -13.31, 4.71, 0.384},
      {1200, 1817, 564, -31.98, 8.79, 0.540},
      {1500, 1625, 451, -61.24, 14.46, 0.715},
      {1800, 1451, 360, -103.76, 21.96, 0.910},
      {2100, 1299, 289, -162.88, 31.49, 1.129},
      {2400, 1174, 236, -242.72, 43.18, 1.372},
      {2700, 1080, 199, -347.86, 56.94, 1.639},
      {3000, 1010, 175, -482.90, 72.53, 1.927}};

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
      {600, 2242, 859, -3.19, -0.00, 0.243},
      {900, 2022, 699, -13.32, -0.00, 0.384},
      {1200, 1815, 563, -32.01, -0.00, 0.540},
      {1500, 1623, 450, -61.31, -0.00, 0.715},
      {1800, 1449, 359, -103.89, -0.00, 0.911},
      {2100, 1297, 288, -163.14, -0.00, 1.130},
      {2400, 1172, 235, -243.18, -0.00, 1.374},
      {2700, 1078, 199, -348.61, -0.00, 1.641},
      {3000, 1009, 174, -484.06, -0.00, 1.929}};

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
      {150, 2596, 1152, -0.60, -0.12, 0.056},
      {300, 2475, 1047, 0.00, -0.47, 0.116},
      {600, 2243, 860, -3.18, -2.00, 0.243},
      {900, 2023, 700, -13.31, -4.71, 0.384},
      {1200, 1817, 564, -31.98, -8.79, 0.540},
      {1500, 1625, 451, -61.24, -14.46, 0.715},
      {1800, 1451, 360, -103.76, -21.96, 0.910},
      {2100, 1299, 289, -162.88, -31.49, 1.129},
      {2400, 1174, 236, -242.72, -43.18, 1.372},
      {2700, 1080, 199, -347.86, -56.94, 1.639},
      {3000, 1010, 175, -482.90, -72.53, 1.927}};

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
      {300, 2476, 1048, 0.00, -0.82, 0.116},
      {600, 2245, 862, -3.18, -3.45, 0.243},
      {900, 2026, 702, -13.29, -8.13, 0.384},
      {1200, 1821, 567, -31.90, -15.17, 0.540},
      {1500, 1630, 454, -61.07, -24.95, 0.714},
      {1800, 1457, 363, -103.39, -37.86, 0.909},
      {2100, 1305, 291, -162.18, -54.27, 1.127},
      {2400, 1180, 238, -241.48, -74.39, 1.369},
      {2700, 1085, 201, -345.81, -98.09, 1.635},
      {3000, 1015, 176, -479.74, -124.95, 1.921}};

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
      {300, 2477, 1049, 0.01, -0.94, 0.116},
      {600, 2248, 864, -3.17, -3.97, 0.243},
      {900, 2030, 705, -13.25, -9.34, 0.383},
      {1200, 1826, 570, -31.80, -17.43, 0.539},
      {1500, 1636, 458, -60.83, -28.65, 0.713},
      {1800, 1464, 366, -102.89, -43.45, 0.907},
      {2100, 1313, 295, -161.23, -62.24, 1.123},
      {2400, 1187, 241, -239.80, -85.27, 1.364},
      {2700, 1091, 204, -343.04, -112.43, 1.628},
      {3000, 1020, 178, -475.47, -143.24, 1.913}};

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
      {300, 2479, 1050, 0.01, -0.81, 0.116},
      {600, 2250, 866, -3.16, -3.42, 0.243},
      {900, 2034, 707, -13.22, -8.06, 0.383},
      {1200, 1831, 573, -31.71, -15.02, 0.538},
      {1500, 1642, 461, -60.59, -24.68, 0.711},
      {1800, 1471, 370, -102.40, -37.40, 0.904},
      {2100, 1320, 298, -160.30, -53.54, 1.120},
      {2400, 1195, 244, -238.15, -73.31, 1.359},
      {2700, 1098, 206, -340.32, -96.64, 1.622},
      {3000, 1026, 180, -471.26, -123.14, 1.905}};

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
      {300, 2480, 1051, 0.01, -0.47, 0.116},
      {600, 2252, 867, -3.16, -1.97, 0.242},
      {900, 2037, 709, -13.20, -4.64, 0.383},
      {1200, 1835, 575, -31.63, -8.64, 0.538},
      {1500, 1647, 464, -60.42, -14.19, 0.710},
      {1800, 1476, 373, -102.04, -21.49, 0.903},
      {2100, 1326, 301, -159.62, -30.75, 1.117},
      {2400, 1200, 246, -236.95, -42.10, 1.356},
      {2700, 1103, 208, -338.35, -55.49, 1.617},
      {3000, 1030, 181, -468.22, -70.71, 1.899}};

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
      {300, 2480, 1052, 0.01, 0.00, 0.116},
      {600, 2253, 868, -3.16, 0.00, 0.242},
      {900, 2038, 710, -13.19, 0.00, 0.382},
      {1200, 1836, 576, -31.61, 0.00, 0.538},
      {1500, 1649, 465, -60.36, 0.00, 0.710},
      {1800, 1478, 374, -101.91, 0.00, 0.902},
      {2100, 1328, 301, -159.38, 0.00, 1.117},
      {2400, 1202, 247, -236.52, 0.00, 1.354},
      {2700, 1104, 208, -337.63, 0.00, 1.615},
      {3000, 1032, 182, -467.12, 0.00, 1.897}};

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
      {300, 2480, 1051, 0.01, 0.47, 0.116},
      {600, 2252, 867, -3.16, 1.97, 0.242},
      {900, 2037, 709, -13.20, 4.64, 0.383},
      {1200, 1835, 575, -31.63, 8.64, 0.538},
      {1500, 1647, 464, -60.42, 14.19, 0.710},
      {1800, 1476, 373, -102.04, 21.49, 0.903},
      {2100, 1326, 301, -159.62, 30.75, 1.117},
      {2400, 1200, 246, -236.95, 42.10, 1.356},
      {2700, 1103, 208, -338.35, 55.49, 1.617},
      {3000, 1030, 181, -468.22, 70.71, 1.899}};

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
      {300, 2479, 1050, 0.01, 0.81, 0.116},
      {600, 2250, 866, -3.16, 3.42, 0.243},
      {900, 2034, 707, -13.22, 8.06, 0.383},
      {1200, 1831, 573, -31.71, 15.02, 0.538},
      {1500, 1642, 461, -60.59, 24.68, 0.711},
      {1800, 1471, 370, -102.40, 37.40, 0.904},
      {2100, 1320, 298, -160.30, 53.54, 1.120},
      {2400, 1195, 244, -238.15, 73.31, 1.359},
      {2700, 1098, 206, -340.32, 96.64, 1.622},
      {3000, 1026, 180, -471.26, 123.14, 1.905}};

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
      {300, 2473, 1045, 0.00, 0.95, 0.116},
      {600, 2238, 856, -3.20, 4.02, 0.243},
      {900, 2016, 695, -13.37, 9.48, 0.384},
      {1200, 1807, 558, -32.16, 17.73, 0.542},
      {1500, 1614, 445, -61.66, 29.21, 0.717},
      {1800, 1439, 354, -104.64, 44.40, 0.914},
      {2100, 1286, 283, -164.58, 63.74, 1.135},
      {2400, 1162, 231, -245.73, 87.46, 1.381},
      {2700, 1069, 195, -352.82, 115.36, 1.651},
      {3000, 1001, 171, -490.53, 146.89, 1.942}};

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
      {300, 2473, 1045, 0.00, 0.95, 0.116},
      {600, 2238, 856, -3.20, 4.02, 0.243},
      {900, 2016, 695, -13.37, 9.48, 0.384},
      {1200, 1807, 558, -32.16, 17.73, 0.542},
      {1500, 1614, 445, -61.66, 29.21, 0.717},
      {1800, 1439, 354, -104.64, 44.40, 0.914},
      {2100, 1286, 283, -164.58, 63.74, 1.135},
      {2400, 1162, 231, -245.73, 87.46, 1.381},
      {2700, 1069, 195, -352.82, 115.36, 1.651},
      {3000, 1001, 171, -490.53, 146.89, 1.942}};

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
      {300, 2473, 1045, 0.00, 0.95, 0.116},
      {600, 2238, 856, -3.20, 4.02, 0.243},
      {900, 2016, 695, -13.37, 9.48, 0.384},
      {1200, 1807, 558, -32.16, 17.73, 0.542},
      {1500, 1614, 445, -61.66, 29.21, 0.717},
      {1800, 1439, 354, -104.64, 44.40, 0.914},
      {2100, 1286, 283, -164.58, 63.74, 1.135},
      {2400, 1162, 231, -245.73, 87.46, 1.381},
      {2700, 1069, 195, -352.82, 115.36, 1.651},
      {3000, 1001, 171, -490.53, 146.89, 1.942}};

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