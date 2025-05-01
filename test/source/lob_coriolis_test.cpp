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

struct LobCoriolisTestFixture : public testing::Test {
  // Unit under test
  std::unique_ptr<lob::Builder> puut;

  LobCoriolisTestFixture() : puut(nullptr) {}

  void SetUp() override {
    ASSERT_EQ(puut, nullptr);
    puut = std::make_unique<lob::Builder>();
    ASSERT_NE(puut, nullptr);

    const double kTestBC = 0.33;
    const lob::DragFunctionT kDragFunction = lob::DragFunctionT::kG7;
    const double kTestDiameter = 0.510;
    const double kTestWeight = 661.0;
    const uint16_t kTestMuzzleVelocity = 2800;
    const double kTestZeroAngle = 5.06;
    const double kTestOpticHeight = 3.0;

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

TEST_F(LobCoriolisTestFixture, ZeroAngleSearch) {
  ASSERT_NE(puut, nullptr);
  auto input1 = puut->Build();
  const double kZeroRange = 100.0;
  auto input2 = puut->ZeroAngleMOA(std::numeric_limits<double>::quiet_NaN())
                    .ZeroDistanceYds(kZeroRange)
                    .Build();
  const double kError = 0.01;
  EXPECT_NEAR(input1.zero_angle, input2.zero_angle, kError);
}

TEST_F(LobCoriolisTestFixture, GetSpeedOfSoundFps) {
  ASSERT_NE(puut, nullptr);
  const auto kInput = puut->Build();
  const double kExpectedFps = 1116.45;
  const double kError = 0.001;
  EXPECT_NEAR(kInput.speed_of_sound, kExpectedFps, kError);
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
TEST_F(LobCoriolisTestFixture, SolveWithoutCoriolisEffect) {
  ASSERT_NE(puut, nullptr);
  constexpr uint16_t kTestStepSize = 100;
  constexpr uint16_t kVelocityError = 2;
  constexpr double kMoaError = 0.5;
  // constexpr double kInchError = 0.1;
  constexpr double kTimeOfFlightError = 0.01;
  constexpr size_t kSolutionLength = 16;
  const auto kInput = puut->Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0,    150,  300,  600,  900,  1200, 1500, 1800,
      2100, 2400, 2700, 3000, 4500, 6000, 7500, 9000};
  const std::vector<lob::Output> kExpected = {
      {0, 2800, 11496, -3.00, 0.00, 0.000},
      {150, 2729, 10919, -0.91, 0.00, 0.054},
      {300, 2659, 10366, 0.01, 0.00, 0.110},
      {600, 2522, 9323, -1.91, 0.00, 0.226},
      {900, 2388, 8363, -9.30, 0.00, 0.348},
      {1200, 2259, 7483, -22.80, 0.00, 0.477},
      {1500, 2134, 6679, -43.10, 0.00, 0.614},
      {1800, 2013, 5944, -71.04, 0.00, 0.758},
      {2100, 1896, 5273, -107.58, 0.00, 0.912},
      {2400, 1783, 4660, -153.77, 0.00, 1.075},
      {2700, 1672, 4099, -210.94, 0.00, 1.249},
      {3000, 1564, 3589, -280.55, 0.00, 1.434},
      {4500, 1090, 1741, -891.15, 0.00, 2.589},
      {6000, 940, 1295, -2208.54, 0.00, 4.090},
      {7500, 842, 1039, -4520.81, 0.00, 5.793},
      {9000, 761, 848, -8096.45, 0.00, 7.707}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  const lob::Options kOptions = {0, 0, lob::NaN(), kTestStepSize};
  lob::Solve(kInput, &kRanges, &solutions, kOptions);
  for (size_t i = 0; i < kSolutionLength; i++) {
    EXPECT_EQ(solutions.at(i).range, kExpected.at(i).range);
    EXPECT_NEAR(solutions.at(i).velocity, kExpected.at(i).velocity,
                kVelocityError);
    const double kSolutionElevationMoa =
        lob::InchToMoa(solutions.at(i).elevation, solutions.at(i).range);
    const double kExpectedElevationMoa =
        lob::InchToMoa(kExpected.at(i).elevation, kExpected.at(i).range);
    EXPECT_NEAR(kSolutionElevationMoa, kExpectedElevationMoa, kMoaError);
    /*EXPECT_NEAR(solutions.at(i).elevation, kExpected.at(i).elevation,
                kInchError);*/
    const double kSolutionDeflectionMoa =
        lob::InchToMoa(solutions.at(i).deflection, solutions.at(i).range);
    const double kExpectedDeflectionMoa =
        lob::InchToMoa(kExpected.at(i).deflection, kExpected.at(i).range);
    EXPECT_NEAR(kSolutionDeflectionMoa, kExpectedDeflectionMoa, kMoaError);
    /*EXPECT_NEAR(solutions.at(i).deflection, kExpected.at(i).deflection,
                kInchError);*/
    EXPECT_NEAR(solutions.at(i).time_of_flight, kExpected.at(i).time_of_flight,
                kTimeOfFlightError);
  }
}

TEST_F(LobCoriolisTestFixture, NorthernHemisphereDeflectionNorth) {
  ASSERT_NE(puut, nullptr);
  constexpr double kLattitude = 45.0;
  constexpr double kAzimuth = 0.0;  // North
  constexpr uint16_t kTestStepSize = 100;
  constexpr double kInchError = 0.1;
  constexpr size_t kSolutionLength = 16;
  const auto kInput1 = puut->Build();
  const auto kInput2 =
      puut->LatitudeDeg(kLattitude).AzimuthDeg(kAzimuth).Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0,    150,  300,  600,  900,  1200, 1500, 1800,
      2100, 2400, 2700, 3000, 4500, 6000, 7500, 9000};
  const std::array<double, kSolutionLength> kExpectedElevationDifference = {
      0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
      0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
  const std::array<double, kSolutionLength> kExpectedDeflectionDifference = {
      0.0,  0.01, 0.02, 0.08, 0.19, 0.34,  0.54,  0.8,
      1.11, 1.48, 1.92, 2.42, 6.14, 12.49, 22.12, 35.65};

  std::array<lob::Output, kSolutionLength> solutions1 = {};
  std::array<lob::Output, kSolutionLength> solutions2 = {};
  const lob::Options kOptions = {0, 0, lob::NaN(), kTestStepSize};
  lob::Solve(kInput1, &kRanges, &solutions1, kOptions);
  lob::Solve(kInput2, &kRanges, &solutions2, kOptions);
  for (size_t i = 0; i < kSolutionLength; i++) {
    const auto kElevationDifference =
        solutions2.at(i).elevation - solutions1.at(i).elevation;
    const auto kDeflectionDifference =
        solutions2.at(i).deflection - solutions1.at(i).deflection;
    EXPECT_NEAR(kElevationDifference, kExpectedElevationDifference.at(i),
                kInchError);
    EXPECT_NEAR(kDeflectionDifference, kExpectedDeflectionDifference.at(i),
                kInchError);
  }
}

TEST_F(LobCoriolisTestFixture, NorthernHemisphereDeflectionEast) {
  ASSERT_NE(puut, nullptr);
  constexpr double kLattitude = 45.0;
  constexpr double kAzimuth = 90.0;  // East
  constexpr uint16_t kTestStepSize = 100;
  constexpr double kInchError = 0.1;
  constexpr size_t kSolutionLength = 16;
  const auto kInput1 = puut->Build();
  const auto kInput2 =
      puut->LatitudeDeg(kLattitude).AzimuthDeg(kAzimuth).Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0,    150,  300,  600,  900,  1200, 1500, 1800,
      2100, 2400, 2700, 3000, 4500, 6000, 7500, 9000};
  const std::array<double, kSolutionLength> kExpectedElevationDifference = {
      0.0,  0.0,  0.02, 0.08, 0.19, 0.35,  0.55,  0.8,
      1.11, 1.48, 1.91, 2.41, 6.08, 12.25, 21.54, 34.29};
  const std::array<double, kSolutionLength> kExpectedDeflectionDifference = {
      0.0,  0.01, 0.02, 0.08, 0.19, 0.34,  0.54,  0.8,
      1.11, 1.48, 1.91, 2.41, 6.07, 12.24, 21.38, 33.88};

  std::array<lob::Output, kSolutionLength> solutions1 = {};
  std::array<lob::Output, kSolutionLength> solutions2 = {};
  const lob::Options kOptions = {0, 0, lob::NaN(), kTestStepSize};
  lob::Solve(kInput1, &kRanges, &solutions1, kOptions);
  lob::Solve(kInput2, &kRanges, &solutions2, kOptions);
  for (size_t i = 0; i < kSolutionLength; i++) {
    const auto kElevationDifference =
        solutions2.at(i).elevation - solutions1.at(i).elevation;
    const auto kDeflectionDifference =
        solutions2.at(i).deflection - solutions1.at(i).deflection;
    EXPECT_NEAR(kElevationDifference, kExpectedElevationDifference.at(i),
                kInchError);
    EXPECT_NEAR(kDeflectionDifference, kExpectedDeflectionDifference.at(i),
                kInchError);
  }
}

TEST_F(LobCoriolisTestFixture,
       NorthernHemisphereDeflectionEastNegativeAzimuth) {
  ASSERT_NE(puut, nullptr);
  constexpr double kLattitude = 45.0;
  constexpr double kAzimuth = -270.0;  // East
  constexpr uint16_t kTestStepSize = 100;
  constexpr double kInchError = 0.1;
  constexpr size_t kSolutionLength = 16;
  const auto kInput1 = puut->Build();
  const auto kInput2 =
      puut->LatitudeDeg(kLattitude).AzimuthDeg(kAzimuth).Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0,    150,  300,  600,  900,  1200, 1500, 1800,
      2100, 2400, 2700, 3000, 4500, 6000, 7500, 9000};
  const std::array<double, kSolutionLength> kExpectedElevationDifference = {
      0.0,  0.0,  0.02, 0.08, 0.19, 0.35,  0.55,  0.8,
      1.11, 1.48, 1.91, 2.41, 6.08, 12.25, 21.54, 34.29};
  const std::array<double, kSolutionLength> kExpectedDeflectionDifference = {
      0.0,  0.01, 0.02, 0.08, 0.19, 0.34,  0.54,  0.8,
      1.11, 1.48, 1.91, 2.41, 6.07, 12.24, 21.38, 33.88};

  std::array<lob::Output, kSolutionLength> solutions1 = {};
  std::array<lob::Output, kSolutionLength> solutions2 = {};
  const lob::Options kOptions = {0, 0, lob::NaN(), kTestStepSize};
  lob::Solve(kInput1, &kRanges, &solutions1, kOptions);
  lob::Solve(kInput2, &kRanges, &solutions2, kOptions);
  for (size_t i = 0; i < kSolutionLength; i++) {
    const auto kElevationDifference =
        solutions2.at(i).elevation - solutions1.at(i).elevation;
    const auto kDeflectionDifference =
        solutions2.at(i).deflection - solutions1.at(i).deflection;
    EXPECT_NEAR(kElevationDifference, kExpectedElevationDifference.at(i),
                kInchError);
    EXPECT_NEAR(kDeflectionDifference, kExpectedDeflectionDifference.at(i),
                kInchError);
  }
}

TEST_F(LobCoriolisTestFixture, NorthernHemisphereDeflectionSouth) {
  ASSERT_NE(puut, nullptr);
  constexpr double kLattitude = 45.0;
  constexpr double kAzimuth = 180.0;  // South
  constexpr uint16_t kTestStepSize = 100;
  constexpr double kInchError = 0.1;
  constexpr size_t kSolutionLength = 16;
  const auto kInput1 = puut->Build();
  const auto kInput2 =
      puut->LatitudeDeg(kLattitude).AzimuthDeg(kAzimuth).Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0,    150,  300,  600,  900,  1200, 1500, 1800,
      2100, 2400, 2700, 3000, 4500, 6000, 7500, 9000};
  const std::array<double, kSolutionLength> kExpectedElevationDifference = {
      0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
      0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
  const std::array<double, kSolutionLength> kExpectedDeflectionDifference = {
      0.0,  0.01, 0.02, 0.08, 0.19, 0.34,  0.54,  0.8,
      1.11, 1.47, 1.9,  2.39, 6.01, 11.99, 20.64, 32.1};

  std::array<lob::Output, kSolutionLength> solutions1 = {};
  std::array<lob::Output, kSolutionLength> solutions2 = {};
  const lob::Options kOptions = {0, 0, lob::NaN(), kTestStepSize};
  lob::Solve(kInput1, &kRanges, &solutions1, kOptions);
  lob::Solve(kInput2, &kRanges, &solutions2, kOptions);
  for (size_t i = 0; i < kSolutionLength; i++) {
    const auto kElevationDifference =
        solutions2.at(i).elevation - solutions1.at(i).elevation;
    const auto kDeflectionDifference =
        solutions2.at(i).deflection - solutions1.at(i).deflection;
    EXPECT_NEAR(kElevationDifference, kExpectedElevationDifference.at(i),
                kInchError);
    EXPECT_NEAR(kDeflectionDifference, kExpectedDeflectionDifference.at(i),
                kInchError);
  }
}

TEST_F(LobCoriolisTestFixture,
       NorthernHemisphereDeflectionSouthNegativeAzimuth) {
  ASSERT_NE(puut, nullptr);
  constexpr double kLattitude = 45.0;
  constexpr double kAzimuth = -180.0;  // South
  constexpr uint16_t kTestStepSize = 100;
  constexpr double kInchError = 0.1;
  constexpr size_t kSolutionLength = 16;
  const auto kInput1 = puut->Build();
  const auto kInput2 =
      puut->LatitudeDeg(kLattitude).AzimuthDeg(kAzimuth).Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0,    150,  300,  600,  900,  1200, 1500, 1800,
      2100, 2400, 2700, 3000, 4500, 6000, 7500, 9000};
  const std::array<double, kSolutionLength> kExpectedElevationDifference = {
      0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
      0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
  const std::array<double, kSolutionLength> kExpectedDeflectionDifference = {
      0.0,  0.01, 0.02, 0.08, 0.19, 0.34,  0.54,  0.8,
      1.11, 1.47, 1.9,  2.39, 6.01, 11.99, 20.64, 32.1};

  std::array<lob::Output, kSolutionLength> solutions1 = {};
  std::array<lob::Output, kSolutionLength> solutions2 = {};
  const lob::Options kOptions = {0, 0, lob::NaN(), kTestStepSize};
  lob::Solve(kInput1, &kRanges, &solutions1, kOptions);
  lob::Solve(kInput2, &kRanges, &solutions2, kOptions);
  for (size_t i = 0; i < kSolutionLength; i++) {
    const auto kElevationDifference =
        solutions2.at(i).elevation - solutions1.at(i).elevation;
    const auto kDeflectionDifference =
        solutions2.at(i).deflection - solutions1.at(i).deflection;
    EXPECT_NEAR(kElevationDifference, kExpectedElevationDifference.at(i),
                kInchError);
    EXPECT_NEAR(kDeflectionDifference, kExpectedDeflectionDifference.at(i),
                kInchError);
  }
}

TEST_F(LobCoriolisTestFixture, NorthernHemisphereCoriolisDeflectionWest) {
  ASSERT_NE(puut, nullptr);
  constexpr double kLattitude = 45.0;
  constexpr double kAzimuth = 270.0;  // West
  constexpr uint16_t kTestStepSize = 100;
  constexpr double kInchError = 0.1;
  constexpr size_t kSolutionLength = 16;
  const auto kInput1 = puut->Build();
  const auto kInput2 =
      puut->LatitudeDeg(kLattitude).AzimuthDeg(kAzimuth).Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0,    150,  300,  600,  900,  1200, 1500, 1800,
      2100, 2400, 2700, 3000, 4500, 6000, 7500, 9000};
  const std::array<double, kSolutionLength> kExpectedElevationDifference = {
      -0.0,  -0.01, -0.03, -0.09, -0.19, -0.34, -0.54,  -0.8,
      -1.11, -1.48, -1.90, -2.4,  -6.07, -12.3, -21.45, -34.4};
  const std::array<double, kSolutionLength> kExpectedDeflectionDifference = {
      0.0,  0.01, 0.02, 0.08, 0.19, 0.34,  0.54,  0.8,
      1.11, 1.48, 1.91, 2.41, 6.07, 12.24, 21.38, 33.88};

  std::array<lob::Output, kSolutionLength> solutions1 = {};
  std::array<lob::Output, kSolutionLength> solutions2 = {};
  const lob::Options kOptions = {0, 0, lob::NaN(), kTestStepSize};
  lob::Solve(kInput1, &kRanges, &solutions1, kOptions);
  lob::Solve(kInput2, &kRanges, &solutions2, kOptions);
  for (size_t i = 0; i < kSolutionLength; i++) {
    const auto kElevationDifference =
        solutions2.at(i).elevation - solutions1.at(i).elevation;
    const auto kDeflectionDifference =
        solutions2.at(i).deflection - solutions1.at(i).deflection;
    EXPECT_NEAR(kElevationDifference, kExpectedElevationDifference.at(i),
                kInchError);
    EXPECT_NEAR(kDeflectionDifference, kExpectedDeflectionDifference.at(i),
                kInchError);
  }
}

TEST_F(LobCoriolisTestFixture,
       NorthernHemisphereCoriolisDeflectionWestNegativeAzimuth) {
  ASSERT_NE(puut, nullptr);
  constexpr double kLattitude = 45.0;
  constexpr double kAzimuth = -90.0;  // West
  constexpr uint16_t kTestStepSize = 100;
  constexpr double kInchError = 0.1;
  constexpr size_t kSolutionLength = 16;
  const auto kInput1 = puut->Build();
  const auto kInput2 =
      puut->LatitudeDeg(kLattitude).AzimuthDeg(kAzimuth).Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0,    150,  300,  600,  900,  1200, 1500, 1800,
      2100, 2400, 2700, 3000, 4500, 6000, 7500, 9000};
  const std::array<double, kSolutionLength> kExpectedElevationDifference = {
      -0.0,  -0.01, -0.03, -0.09, -0.19, -0.34, -0.54,  -0.8,
      -1.11, -1.48, -1.90, -2.4,  -6.07, -12.3, -21.45, -34.4};
  const std::array<double, kSolutionLength> kExpectedDeflectionDifference = {
      0.0,  0.01, 0.02, 0.08, 0.19, 0.34,  0.54,  0.8,
      1.11, 1.48, 1.91, 2.41, 6.07, 12.24, 21.38, 33.88};

  std::array<lob::Output, kSolutionLength> solutions1 = {};
  std::array<lob::Output, kSolutionLength> solutions2 = {};
  const lob::Options kOptions = {0, 0, lob::NaN(), kTestStepSize};
  lob::Solve(kInput1, &kRanges, &solutions1, kOptions);
  lob::Solve(kInput2, &kRanges, &solutions2, kOptions);
  for (size_t i = 0; i < kSolutionLength; i++) {
    const auto kElevationDifference =
        solutions2.at(i).elevation - solutions1.at(i).elevation;
    const auto kDeflectionDifference =
        solutions2.at(i).deflection - solutions1.at(i).deflection;
    EXPECT_NEAR(kElevationDifference, kExpectedElevationDifference.at(i),
                kInchError);
    EXPECT_NEAR(kDeflectionDifference, kExpectedDeflectionDifference.at(i),
                kInchError);
  }
}

TEST_F(LobCoriolisTestFixture, SouthernHemisphereCoriolisDeflectionNorth) {
  ASSERT_NE(puut, nullptr);
  constexpr double kLattitude = -45.0;
  constexpr double kAzimuth = 0.0;  // North
  constexpr uint16_t kTestStepSize = 100;
  constexpr double kInchError = 0.1;
  constexpr size_t kSolutionLength = 16;
  const auto kInput1 = puut->Build();
  const auto kInput2 =
      puut->LatitudeDeg(kLattitude).AzimuthDeg(kAzimuth).Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0,    150,  300,  600,  900,  1200, 1500, 1800,
      2100, 2400, 2700, 3000, 4500, 6000, 7500, 9000};
  const std::array<double, kSolutionLength> kExpectedElevationDifference = {
      0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
      0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
  const std::array<double, kSolutionLength> kExpectedDeflectionDifference = {
      0.0,   -0.01, -0.02, -0.08, -0.19, -0.34,  -0.54,  -0.8,
      -1.11, -1.47, -1.9,  -2.39, -6.01, -11.99, -20.64, -32.1};

  std::array<lob::Output, kSolutionLength> solutions1 = {};
  std::array<lob::Output, kSolutionLength> solutions2 = {};
  const lob::Options kOptions = {0, 0, lob::NaN(), kTestStepSize};
  lob::Solve(kInput1, &kRanges, &solutions1, kOptions);
  lob::Solve(kInput2, &kRanges, &solutions2, kOptions);
  for (size_t i = 0; i < kSolutionLength; i++) {
    const auto kElevationDifference =
        solutions2.at(i).elevation - solutions1.at(i).elevation;
    const auto kDeflectionDifference =
        solutions2.at(i).deflection - solutions1.at(i).deflection;
    EXPECT_NEAR(kElevationDifference, kExpectedElevationDifference.at(i),
                kInchError);
    EXPECT_NEAR(kDeflectionDifference, kExpectedDeflectionDifference.at(i),
                kInchError);
  }
}

TEST_F(LobCoriolisTestFixture, SouthernHemisphereDeflectionEast) {
  ASSERT_NE(puut, nullptr);
  constexpr double kLattitude = -45.0;
  constexpr double kAzimuth = 90.0;  // East
  constexpr uint16_t kTestStepSize = 100;
  constexpr double kInchError = 0.1;
  constexpr size_t kSolutionLength = 16;
  const auto kInput1 = puut->Build();
  const auto kInput2 =
      puut->LatitudeDeg(kLattitude).AzimuthDeg(kAzimuth).Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0,    150,  300,  600,  900,  1200, 1500, 1800,
      2100, 2400, 2700, 3000, 4500, 6000, 7500, 9000};
  const std::array<double, kSolutionLength> kExpectedElevationDifference = {
      0.0,  0.0,  0.02, 0.08, 0.19, 0.35,  0.55,  0.8,
      1.11, 1.48, 1.91, 2.41, 6.08, 12.25, 21.54, 34.29};
  const std::array<double, kSolutionLength> kExpectedDeflectionDifference = {
      0.0,   -0.01, -0.02, -0.08, -0.19, -0.34,  -0.54,  -0.8,
      -1.11, -1.48, -1.91, -2.41, -6.07, -12.24, -21.38, -33.88};

  std::array<lob::Output, kSolutionLength> solutions1 = {};
  std::array<lob::Output, kSolutionLength> solutions2 = {};
  const lob::Options kOptions = {0, 0, lob::NaN(), kTestStepSize};
  lob::Solve(kInput1, &kRanges, &solutions1, kOptions);
  lob::Solve(kInput2, &kRanges, &solutions2, kOptions);
  for (size_t i = 0; i < kSolutionLength; i++) {
    const auto kElevationDifference =
        solutions2.at(i).elevation - solutions1.at(i).elevation;
    const auto kDeflectionDifference =
        solutions2.at(i).deflection - solutions1.at(i).deflection;
    EXPECT_NEAR(kElevationDifference, kExpectedElevationDifference.at(i),
                kInchError);
    EXPECT_NEAR(kDeflectionDifference, kExpectedDeflectionDifference.at(i),
                kInchError);
  }
}

TEST_F(LobCoriolisTestFixture, SouthernHemisphereCoriolisDeflectionSouth) {
  ASSERT_NE(puut, nullptr);
  constexpr double kLattitude = -45.0;
  constexpr double kAzimuth = 180.0;  // South
  constexpr uint16_t kTestStepSize = 100;
  constexpr double kInchError = 0.1;
  constexpr size_t kSolutionLength = 16;
  const auto kInput1 = puut->Build();
  const auto kInput2 =
      puut->LatitudeDeg(kLattitude).AzimuthDeg(kAzimuth).Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0,    150,  300,  600,  900,  1200, 1500, 1800,
      2100, 2400, 2700, 3000, 4500, 6000, 7500, 9000};
  const std::array<double, kSolutionLength> kExpectedElevationDifference = {
      0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
      0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
  const std::array<double, kSolutionLength> kExpectedDeflectionDifference = {
      0.0,   -0.01, -0.02, -0.08, -0.19, -0.34,  -0.54,  -0.8,
      -1.11, -1.48, -1.92, -2.42, -6.14, -12.49, -22.12, -35.65};

  std::array<lob::Output, kSolutionLength> solutions1 = {};
  std::array<lob::Output, kSolutionLength> solutions2 = {};
  const lob::Options kOptions = {0, 0, lob::NaN(), kTestStepSize};
  lob::Solve(kInput1, &kRanges, &solutions1, kOptions);
  lob::Solve(kInput2, &kRanges, &solutions2, kOptions);
  for (size_t i = 0; i < kSolutionLength; i++) {
    const auto kElevationDifference =
        solutions2.at(i).elevation - solutions1.at(i).elevation;
    const auto kDeflectionDifference =
        solutions2.at(i).deflection - solutions1.at(i).deflection;
    EXPECT_NEAR(kElevationDifference, kExpectedElevationDifference.at(i),
                kInchError);
    EXPECT_NEAR(kDeflectionDifference, kExpectedDeflectionDifference.at(i),
                kInchError);
  }
}

TEST_F(LobCoriolisTestFixture, SouthernHemisphereCoriolisDeflectionWest) {
  ASSERT_NE(puut, nullptr);
  constexpr double kLattitude = -45.0;
  constexpr double kAzimuth = 270.0;  // West
  constexpr uint16_t kTestStepSize = 100;
  constexpr double kInchError = 0.1;
  constexpr size_t kSolutionLength = 16;
  const auto kInput1 = puut->Build();
  const auto kInput2 =
      puut->LatitudeDeg(kLattitude).AzimuthDeg(kAzimuth).Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0,    150,  300,  600,  900,  1200, 1500, 1800,
      2100, 2400, 2700, 3000, 4500, 6000, 7500, 9000};
  const std::array<double, kSolutionLength> kExpectedElevationDifference = {
      0.0,   -0.01, -0.03, -0.09, -0.19, -0.34, -0.54,  -0.8,
      -1.11, -1.48, -1.90, -2.4,  -6.07, -12.3, -21.45, -34.4};
  const std::array<double, kSolutionLength> kExpectedDeflectionDifference = {
      0.0,   -0.01, -0.02, -0.08, -0.19, -0.34,  -0.54,  -0.8,
      -1.11, -1.48, -1.91, -2.41, -6.07, -12.24, -21.38, -33.88};

  std::array<lob::Output, kSolutionLength> solutions1 = {};
  std::array<lob::Output, kSolutionLength> solutions2 = {};
  const lob::Options kOptions = {0, 0, lob::NaN(), kTestStepSize};
  lob::Solve(kInput1, &kRanges, &solutions1, kOptions);
  lob::Solve(kInput2, &kRanges, &solutions2, kOptions);
  for (size_t i = 0; i < kSolutionLength; i++) {
    const auto kElevationDifference =
        solutions2.at(i).elevation - solutions1.at(i).elevation;
    const auto kDeflectionDifference =
        solutions2.at(i).deflection - solutions1.at(i).deflection;
    EXPECT_NEAR(kElevationDifference, kExpectedElevationDifference.at(i),
                kInchError);
    EXPECT_NEAR(kDeflectionDifference, kExpectedDeflectionDifference.at(i),
                kInchError);
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