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

    const float kTestBC = 0.33F;
    const lob::DragFunctionT kDragFunction = lob::DragFunctionT::kG7;
    const float kTestDiameter = 0.510F;
    const float kTestWeight = 661.0F;
    const uint16_t kTestMuzzleVelocity = 2800;
    const float kTestZeroAngle = 5.06F;
    const float kTestOpticHeight = 3.0F;

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

TEST_F(LobCoriolisTestFixture, ZeroAngleSearch) {
  ASSERT_NE(puut, nullptr);
  auto input1 = puut->Build();
  const float kZeroRange = 100.0F;
  auto input2 = puut->ZeroAngleMOA(std::numeric_limits<double>::quiet_NaN())
                    .ZeroDistanceYds(kZeroRange)
                    .Build();
  const float kError = 0.01F;
  EXPECT_NEAR(input1.zero_angle, input2.zero_angle, kError);
}

TEST_F(LobCoriolisTestFixture, GetSpeedOfSoundFps) {
  ASSERT_NE(puut, nullptr);
  const auto kInput = puut->Build();
  const float kExpectedFps = 1116.45F;
  const float kError = 0.001F;
  EXPECT_NEAR(kInput.speed_of_sound, kExpectedFps, kError);
}

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
      {0, 2800, 11496, -3.00F, 0.00F, 0.000F},
      {150, 2729, 10919, -0.91F, 0.00F, 0.054F},
      {300, 2659, 10366, 0.01F, 0.00F, 0.110F},
      {600, 2522, 9323, -1.91F, 0.00F, 0.226F},
      {900, 2388, 8363, -9.30F, 0.00F, 0.348F},
      {1200, 2259, 7483, -22.80F, 0.00F, 0.477F},
      {1500, 2134, 6679, -43.10F, 0.00F, 0.614F},
      {1800, 2013, 5944, -71.04F, 0.00F, 0.758F},
      {2100, 1896, 5273, -107.58F, 0.00F, 0.912F},
      {2400, 1783, 4660, -153.77F, 0.00F, 1.075F},
      {2700, 1672, 4099, -210.94F, 0.00F, 1.249F},
      {3000, 1564, 3589, -280.55F, 0.00F, 1.434F},
      {4500, 1090, 1741, -891.15F, 0.00F, 2.589F},
      {6000, 940, 1295, -2208.54F, 0.00F, 4.090F},
      {7500, 842, 1039, -4520.81F, 0.00F, 5.793F},
      {9000, 761, 848, -8096.45F, 0.00F, 7.707F}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  const lob::Options kOptions = {0, 0, lob::kNaN, kTestStepSize};
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
  constexpr float kLattitude = 45.0F;
  constexpr float kAzimuth = 0.0F;  // North
  constexpr uint16_t kTestStepSize = 100;
  constexpr double kInchError = 0.1;
  constexpr size_t kSolutionLength = 16;
  const auto kInput1 = puut->Build();
  const auto kInput2 =
      puut->LatitudeDeg(kLattitude).AzimuthDeg(kAzimuth).Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0,    150,  300,  600,  900,  1200, 1500, 1800,
      2100, 2400, 2700, 3000, 4500, 6000, 7500, 9000};
  const std::array<float, kSolutionLength> kExpectedElevationDifference = {
      0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F,
      0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F};
  const std::array<float, kSolutionLength> kExpectedDeflectionDifference = {
      0.0F,  0.01F, 0.02F, 0.08F, 0.19F, 0.34F,  0.54F,  0.8F,
      1.11F, 1.48F, 1.92F, 2.42F, 6.14F, 12.49F, 22.12F, 35.65F};

  std::array<lob::Output, kSolutionLength> solutions1 = {};
  std::array<lob::Output, kSolutionLength> solutions2 = {};
  const lob::Options kOptions = {0, 0, lob::kNaN, kTestStepSize};
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
  constexpr float kLattitude = 45.0F;
  constexpr float kAzimuth = 90.0F;  // East
  constexpr uint16_t kTestStepSize = 100;
  constexpr double kInchError = 0.1;
  constexpr size_t kSolutionLength = 16;
  const auto kInput1 = puut->Build();
  const auto kInput2 =
      puut->LatitudeDeg(kLattitude).AzimuthDeg(kAzimuth).Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0,    150,  300,  600,  900,  1200, 1500, 1800,
      2100, 2400, 2700, 3000, 4500, 6000, 7500, 9000};
  const std::array<float, kSolutionLength> kExpectedElevationDifference = {
      0.0F,  0.0F,  0.02F, 0.08F, 0.19F, 0.35F,  0.55F,  0.8F,
      1.11F, 1.48F, 1.91F, 2.41F, 6.08F, 12.25F, 21.54F, 34.29F};
  const std::array<float, kSolutionLength> kExpectedDeflectionDifference = {
      0.0F,  0.01F, 0.02F, 0.08F, 0.19F, 0.34F,  0.54F,  0.8F,
      1.11F, 1.48F, 1.91F, 2.41F, 6.07F, 12.24F, 21.38F, 33.88F};

  std::array<lob::Output, kSolutionLength> solutions1 = {};
  std::array<lob::Output, kSolutionLength> solutions2 = {};
  const lob::Options kOptions = {0, 0, lob::kNaN, kTestStepSize};
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
  constexpr float kLattitude = 45.0F;
  constexpr float kAzimuth = -270.0F;  // East
  constexpr uint16_t kTestStepSize = 100;
  constexpr double kInchError = 0.1;
  constexpr size_t kSolutionLength = 16;
  const auto kInput1 = puut->Build();
  const auto kInput2 =
      puut->LatitudeDeg(kLattitude).AzimuthDeg(kAzimuth).Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0,    150,  300,  600,  900,  1200, 1500, 1800,
      2100, 2400, 2700, 3000, 4500, 6000, 7500, 9000};
  const std::array<float, kSolutionLength> kExpectedElevationDifference = {
      0.0F,  0.0F,  0.02F, 0.08F, 0.19F, 0.35F,  0.55F,  0.8F,
      1.11F, 1.48F, 1.91F, 2.41F, 6.08F, 12.25F, 21.54F, 34.29F};
  const std::array<float, kSolutionLength> kExpectedDeflectionDifference = {
      0.0F,  0.01F, 0.02F, 0.08F, 0.19F, 0.34F,  0.54F,  0.8F,
      1.11F, 1.48F, 1.91F, 2.41F, 6.07F, 12.24F, 21.38F, 33.88F};

  std::array<lob::Output, kSolutionLength> solutions1 = {};
  std::array<lob::Output, kSolutionLength> solutions2 = {};
  const lob::Options kOptions = {0, 0, lob::kNaN, kTestStepSize};
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
  constexpr float kLattitude = 45.0F;
  constexpr float kAzimuth = 180.0F;  // South
  constexpr uint16_t kTestStepSize = 100;
  constexpr double kInchError = 0.1;
  constexpr size_t kSolutionLength = 16;
  const auto kInput1 = puut->Build();
  const auto kInput2 =
      puut->LatitudeDeg(kLattitude).AzimuthDeg(kAzimuth).Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0,    150,  300,  600,  900,  1200, 1500, 1800,
      2100, 2400, 2700, 3000, 4500, 6000, 7500, 9000};
  const std::array<float, kSolutionLength> kExpectedElevationDifference = {
      0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F,
      0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F};
  const std::array<float, kSolutionLength> kExpectedDeflectionDifference = {
      0.0F,  0.01F, 0.02F, 0.08F, 0.19F, 0.34F,  0.54F,  0.8F,
      1.11F, 1.47F, 1.9F,  2.39F, 6.01F, 11.99F, 20.64F, 32.1F};

  std::array<lob::Output, kSolutionLength> solutions1 = {};
  std::array<lob::Output, kSolutionLength> solutions2 = {};
  const lob::Options kOptions = {0, 0, lob::kNaN, kTestStepSize};
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
  constexpr float kLattitude = 45.0F;
  constexpr float kAzimuth = -180.0F;  // South
  constexpr uint16_t kTestStepSize = 100;
  constexpr double kInchError = 0.1;
  constexpr size_t kSolutionLength = 16;
  const auto kInput1 = puut->Build();
  const auto kInput2 =
      puut->LatitudeDeg(kLattitude).AzimuthDeg(kAzimuth).Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0,    150,  300,  600,  900,  1200, 1500, 1800,
      2100, 2400, 2700, 3000, 4500, 6000, 7500, 9000};
  const std::array<float, kSolutionLength> kExpectedElevationDifference = {
      0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F,
      0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F};
  const std::array<float, kSolutionLength> kExpectedDeflectionDifference = {
      0.0F,  0.01F, 0.02F, 0.08F, 0.19F, 0.34F,  0.54F,  0.8F,
      1.11F, 1.47F, 1.9F,  2.39F, 6.01F, 11.99F, 20.64F, 32.1F};

  std::array<lob::Output, kSolutionLength> solutions1 = {};
  std::array<lob::Output, kSolutionLength> solutions2 = {};
  const lob::Options kOptions = {0, 0, lob::kNaN, kTestStepSize};
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
  constexpr float kLattitude = 45.0F;
  constexpr float kAzimuth = 270.0F;  // West
  constexpr uint16_t kTestStepSize = 100;
  constexpr double kInchError = 0.1;
  constexpr size_t kSolutionLength = 16;
  const auto kInput1 = puut->Build();
  const auto kInput2 =
      puut->LatitudeDeg(kLattitude).AzimuthDeg(kAzimuth).Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0,    150,  300,  600,  900,  1200, 1500, 1800,
      2100, 2400, 2700, 3000, 4500, 6000, 7500, 9000};
  const std::array<float, kSolutionLength> kExpectedElevationDifference = {
      -0.0F,  -0.01F, -0.03F, -0.09F, -0.19F, -0.34F, -0.54F,  -0.8F,
      -1.11F, -1.48F, -1.90F, -2.4F,  -6.07F, -12.3F, -21.45F, -34.4F};
  const std::array<float, kSolutionLength> kExpectedDeflectionDifference = {
      0.0F,  0.01F, 0.02F, 0.08F, 0.19F, 0.34F,  0.54F,  0.8F,
      1.11F, 1.48F, 1.91F, 2.41F, 6.07F, 12.24F, 21.38F, 33.88F};

  std::array<lob::Output, kSolutionLength> solutions1 = {};
  std::array<lob::Output, kSolutionLength> solutions2 = {};
  const lob::Options kOptions = {0, 0, lob::kNaN, kTestStepSize};
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
  constexpr float kLattitude = 45.0F;
  constexpr float kAzimuth = -90.0F;  // West
  constexpr uint16_t kTestStepSize = 100;
  constexpr double kInchError = 0.1;
  constexpr size_t kSolutionLength = 16;
  const auto kInput1 = puut->Build();
  const auto kInput2 =
      puut->LatitudeDeg(kLattitude).AzimuthDeg(kAzimuth).Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0,    150,  300,  600,  900,  1200, 1500, 1800,
      2100, 2400, 2700, 3000, 4500, 6000, 7500, 9000};
  const std::array<float, kSolutionLength> kExpectedElevationDifference = {
      -0.0F,  -0.01F, -0.03F, -0.09F, -0.19F, -0.34F, -0.54F,  -0.8F,
      -1.11F, -1.48F, -1.90F, -2.4F,  -6.07F, -12.3F, -21.45F, -34.4F};
  const std::array<float, kSolutionLength> kExpectedDeflectionDifference = {
      0.0F,  0.01F, 0.02F, 0.08F, 0.19F, 0.34F,  0.54F,  0.8F,
      1.11F, 1.48F, 1.91F, 2.41F, 6.07F, 12.24F, 21.38F, 33.88F};

  std::array<lob::Output, kSolutionLength> solutions1 = {};
  std::array<lob::Output, kSolutionLength> solutions2 = {};
  const lob::Options kOptions = {0, 0, lob::kNaN, kTestStepSize};
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
  constexpr float kLattitude = -45.0;
  constexpr float kAzimuth = 0.0F;  // North
  constexpr uint16_t kTestStepSize = 100;
  constexpr double kInchError = 0.1;
  constexpr size_t kSolutionLength = 16;
  const auto kInput1 = puut->Build();
  const auto kInput2 =
      puut->LatitudeDeg(kLattitude).AzimuthDeg(kAzimuth).Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0,    150,  300,  600,  900,  1200, 1500, 1800,
      2100, 2400, 2700, 3000, 4500, 6000, 7500, 9000};
  const std::array<float, kSolutionLength> kExpectedElevationDifference = {
      0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F,
      0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F};
  const std::array<float, kSolutionLength> kExpectedDeflectionDifference = {
      0.0F,   -0.01F, -0.02F, -0.08F, -0.19F, -0.34F,  -0.54F,  -0.8F,
      -1.11F, -1.47F, -1.9F,  -2.39F, -6.01F, -11.99F, -20.64F, -32.1F};

  std::array<lob::Output, kSolutionLength> solutions1 = {};
  std::array<lob::Output, kSolutionLength> solutions2 = {};
  const lob::Options kOptions = {0, 0, lob::kNaN, kTestStepSize};
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
  constexpr float kLattitude = -45.0;
  constexpr float kAzimuth = 90.0F;  // East
  constexpr uint16_t kTestStepSize = 100;
  constexpr double kInchError = 0.1;
  constexpr size_t kSolutionLength = 16;
  const auto kInput1 = puut->Build();
  const auto kInput2 =
      puut->LatitudeDeg(kLattitude).AzimuthDeg(kAzimuth).Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0,    150,  300,  600,  900,  1200, 1500, 1800,
      2100, 2400, 2700, 3000, 4500, 6000, 7500, 9000};
  const std::array<float, kSolutionLength> kExpectedElevationDifference = {
      0.0F,  0.0F,  0.02F, 0.08F, 0.19F, 0.35F,  0.55F,  0.8F,
      1.11F, 1.48F, 1.91F, 2.41F, 6.08F, 12.25F, 21.54F, 34.29F};
  const std::array<float, kSolutionLength> kExpectedDeflectionDifference = {
      0.0F,   -0.01F, -0.02F, -0.08F, -0.19F, -0.34F,  -0.54F,  -0.8F,
      -1.11F, -1.48F, -1.91F, -2.41F, -6.07F, -12.24F, -21.38F, -33.88F};

  std::array<lob::Output, kSolutionLength> solutions1 = {};
  std::array<lob::Output, kSolutionLength> solutions2 = {};
  const lob::Options kOptions = {0, 0, lob::kNaN, kTestStepSize};
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
  constexpr float kLattitude = -45.0;
  constexpr float kAzimuth = 180.0F;  // South
  constexpr uint16_t kTestStepSize = 100;
  constexpr double kInchError = 0.1;
  constexpr size_t kSolutionLength = 16;
  const auto kInput1 = puut->Build();
  const auto kInput2 =
      puut->LatitudeDeg(kLattitude).AzimuthDeg(kAzimuth).Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0,    150,  300,  600,  900,  1200, 1500, 1800,
      2100, 2400, 2700, 3000, 4500, 6000, 7500, 9000};
  const std::array<float, kSolutionLength> kExpectedElevationDifference = {
      0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F,
      0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F};
  const std::array<float, kSolutionLength> kExpectedDeflectionDifference = {
      0.0F,   -0.01F, -0.02F, -0.08F, -0.19F, -0.34F,  -0.54F,  -0.8F,
      -1.11F, -1.48F, -1.92F, -2.42F, -6.14F, -12.49F, -22.12F, -35.65F};

  std::array<lob::Output, kSolutionLength> solutions1 = {};
  std::array<lob::Output, kSolutionLength> solutions2 = {};
  const lob::Options kOptions = {0, 0, lob::kNaN, kTestStepSize};
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
  constexpr float kLattitude = -45.0;
  constexpr float kAzimuth = 270.0F;  // West
  constexpr uint16_t kTestStepSize = 100;
  constexpr double kInchError = 0.1;
  constexpr size_t kSolutionLength = 16;
  const auto kInput1 = puut->Build();
  const auto kInput2 =
      puut->LatitudeDeg(kLattitude).AzimuthDeg(kAzimuth).Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0,    150,  300,  600,  900,  1200, 1500, 1800,
      2100, 2400, 2700, 3000, 4500, 6000, 7500, 9000};
  const std::array<float, kSolutionLength> kExpectedElevationDifference = {
      0.0F,   -0.01F, -0.03F, -0.09F, -0.19F, -0.34F, -0.54F,  -0.8F,
      -1.11F, -1.48F, -1.90F, -2.4F,  -6.07F, -12.3F, -21.45F, -34.4F};
  const std::array<float, kSolutionLength> kExpectedDeflectionDifference = {
      0.0F,   -0.01F, -0.02F, -0.08F, -0.19F, -0.34F,  -0.54F,  -0.8F,
      -1.11F, -1.48F, -1.91F, -2.41F, -6.07F, -12.24F, -21.38F, -33.88F};

  std::array<lob::Output, kSolutionLength> solutions1 = {};
  std::array<lob::Output, kSolutionLength> solutions2 = {};
  const lob::Options kOptions = {0, 0, lob::kNaN, kTestStepSize};
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