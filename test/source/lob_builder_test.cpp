// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>

#include "constants.hpp"
#include "eng_units.hpp"
#include "lob/lob.hpp"
#include "tables.hpp"

namespace tests {

struct BuilderTestFixture : public testing::Test {
  // Unit under test
  std::unique_ptr<lob::Builder> puut;

  BuilderTestFixture() = default;

  void SetUp() override {
    ASSERT_EQ(puut, nullptr);

    puut = std::make_unique<lob::Builder>();

    ASSERT_NE(puut, nullptr);
  }

  void TearDown() override {
    puut.reset();
    puut = nullptr;
    ASSERT_EQ(puut, nullptr);
  }
};

TEST_F(BuilderTestFixture, Constructor) { ASSERT_NE(puut, nullptr); }

TEST_F(BuilderTestFixture, CopyConstructor) {
  const double kTestBC = 0.425;
  const double kTestDiameter = 0.308;
  const double kTestWeight = 180.0;
  const uint16_t kTestMuzzleVelocity = 2700U;
  const double kTestZeroAngle = 3.38;
  puut->BallisticCoefficientPsi(kTestBC)
      .DiameterInch(kTestDiameter)
      .MassGrains(kTestWeight)
      .InitialVelocityFps(kTestMuzzleVelocity)
      .ZeroAngleMOA(kTestZeroAngle);
  lob::Builder copy = *puut;
  const lob::Input kVal1 = puut->Build();
  EXPECT_DOUBLE_EQ(kVal1.velocity, kTestMuzzleVelocity);
  const lob::Input kVal2 = copy.Build();
  EXPECT_DOUBLE_EQ(kVal2.velocity, kTestMuzzleVelocity);
}

TEST_F(BuilderTestFixture, MoveConstructor) {
  const double kTestBC = 0.425;
  const double kTestDiameter = 0.308;
  const double kTestWeight = 180.0;
  const uint16_t kTestMuzzleVelocity = 2700U;
  const double kTestZeroAngle = 3.38;
  puut->BallisticCoefficientPsi(kTestBC)
      .DiameterInch(kTestDiameter)
      .MassGrains(kTestWeight)
      .InitialVelocityFps(kTestMuzzleVelocity)
      .ZeroAngleMOA(kTestZeroAngle);
  lob::Builder moved = std::move(*puut);
  const lob::Input kVal = moved.Build();
  EXPECT_EQ(kVal.velocity, kTestMuzzleVelocity);
}

// Test for copy assignment operator
TEST_F(BuilderTestFixture, CopyAssignmentOperator) {
  const double kTestBC = 0.425;
  const double kTestDiameter = 0.308;
  const double kTestWeight = 180.0;
  const uint16_t kTestMuzzleVelocity = 2700U;
  const double kTestZeroAngle = 3.38;
  puut->BallisticCoefficientPsi(kTestBC)
      .DiameterInch(kTestDiameter)
      .MassGrains(kTestWeight)
      .InitialVelocityFps(kTestMuzzleVelocity)
      .ZeroAngleMOA(kTestZeroAngle);
  lob::Builder copy;
  copy = *puut;
  const lob::Input kVal1 = puut->Build();
  EXPECT_DOUBLE_EQ(kVal1.velocity, kTestMuzzleVelocity);
  const lob::Input kVal2 = copy.Build();
  EXPECT_DOUBLE_EQ(kVal2.velocity, kTestMuzzleVelocity);
}

// Test for move assignment operator
TEST_F(BuilderTestFixture, MoveAssignmentOperator) {
  const double kTestBC = 0.425;
  const double kTestDiameter = 0.308;
  const double kTestWeight = 180.0;
  const uint16_t kTestMuzzleVelocity = 2700U;
  const double kTestZeroAngle = 3.38;
  puut->BallisticCoefficientPsi(kTestBC)
      .DiameterInch(kTestDiameter)
      .MassGrains(kTestWeight)
      .InitialVelocityFps(kTestMuzzleVelocity)
      .ZeroAngleMOA(kTestZeroAngle);
  lob::Builder moved;
  moved = std::move(*puut);
  const lob::Input kVal = moved.Build();
  EXPECT_EQ(kVal.velocity, kTestMuzzleVelocity);
}

TEST_F(BuilderTestFixture, BuildMinimalInput) {
  const double kTestBC = 0.425;
  const uint16_t kTestMuzzleVelocity = 2700U;
  const double kTestZeroAngle = 3.84;
  const double kZeroDistance = 100.0;
  const lob::Input kResult = puut->BallisticCoefficientPsi(kTestBC)
                                 .InitialVelocityFps(kTestMuzzleVelocity)
                                 .ZeroDistanceYds(kZeroDistance)
                                 .Build();
  EXPECT_FALSE(std::isnan(kResult.table_coefficient));
  EXPECT_FALSE(std::isnan(kResult.speed_of_sound));
  EXPECT_EQ(kResult.velocity, kTestMuzzleVelocity);
  EXPECT_NEAR(kResult.zero_angle, kTestZeroAngle, 0.01);
  EXPECT_DOUBLE_EQ(kResult.gravity.y, -1.0 * lob::kStandardGravityFtPerSecSq);
}

TEST_F(BuilderTestFixture, BuildMissingVelocityInput) {
  const double kTestBC = 0.425;
  const double kTestZeroAngle = 3.84;
  const lob::Input kResult = puut->BallisticCoefficientPsi(kTestBC)
                                 .ZeroAngleMOA(kTestZeroAngle)
                                 .Build();
  EXPECT_EQ(kResult.error, lob::ErrorT::kInitialVelocityRequired);
}

TEST_F(BuilderTestFixture, BuildMissingBCInput) {
  const uint16_t kTestMuzzleVelocity = 2700U;
  const double kTestZeroAngle = 3.84;
  const lob::Input kResult = puut->InitialVelocityFps(kTestMuzzleVelocity)
                                 .ZeroAngleMOA(kTestZeroAngle)
                                 .Build();
  EXPECT_EQ(kResult.error, lob::ErrorT::kBallisticCoefficientRequired);
}

TEST_F(BuilderTestFixture, BuildMissingZeroInput) {
  const double kTestBC = 0.425;
  const uint16_t kTestMuzzleVelocity = 2700U;
  const lob::Input kResult = puut->BallisticCoefficientPsi(kTestBC)
                                 .InitialVelocityFps(kTestMuzzleVelocity)
                                 .Build();
  EXPECT_EQ(kResult.error, lob::ErrorT::kZeroDataRequired);
}

TEST_F(BuilderTestFixture, BuildG1UsingCustomTable) {
  const double kTestBC = 1.0;
  const uint16_t kTestMuzzleVelocity = 2500U;
  const double kTestZeroAngle = 5.59;
  std::array<float, lob::kTableSize> machs = {};
  std::array<float, lob::kTableSize> drags = {};

  const lob::Input kResult1 =
      puut->BallisticCoefficientPsi(kTestBC)
          .BCDragFunction(lob::DragFunctionT::kG1)
          .BCAtmosphere(lob::AtmosphereReferenceT::kIcao)
          .InitialVelocityFps(kTestMuzzleVelocity)
          .ZeroAngleMOA(kTestZeroAngle)
          .Build();

  for (size_t i = 0; i < lob::kTableSize; i++) {
    machs.at(i) = static_cast<float>(lob::kMachs.at(i)) / lob::kTableScale;
    drags.at(i) = static_cast<float>(lob::kG1Drags.at(i)) / lob::kTableScale;
  }

  const lob::Input kResult2 = puut->BallisticCoefficientPsi(lob::NaN())
                                  .MachVsDragTable(machs, drags)
                                  .Build();

  for (size_t i = 0; i < lob::kTableSize; i++) {
    EXPECT_EQ(kResult1.drags.at(i), kResult2.drags.at(i));
  }
}

TEST_F(BuilderTestFixture, BuildG2UsingCustomTable) {
  const double kTestBC = 1.0;
  const uint16_t kTestMuzzleVelocity = 2500U;
  const double kTestZeroAngle = 5.0;
  std::array<float, lob::kTableSize> machs = {};
  std::array<float, lob::kTableSize> drags = {};

  const lob::Input kResult1 =
      puut->BallisticCoefficientPsi(kTestBC)
          .BCDragFunction(lob::DragFunctionT::kG2)
          .BCAtmosphere(lob::AtmosphereReferenceT::kIcao)
          .InitialVelocityFps(kTestMuzzleVelocity)
          .ZeroAngleMOA(kTestZeroAngle)
          .Build();

  for (size_t i = 0; i < lob::kTableSize; i++) {
    machs.at(i) = static_cast<float>(lob::kMachs.at(i)) / lob::kTableScale;
    drags.at(i) = static_cast<float>(lob::kG2Drags.at(i)) / lob::kTableScale;
  }

  const lob::Input kResult2 = puut->BallisticCoefficientPsi(lob::NaN())
                                  .MachVsDragTable(machs, drags)
                                  .Build();

  for (size_t i = 0; i < lob::kTableSize; i++) {
    EXPECT_EQ(kResult1.drags.at(i), kResult2.drags.at(i));
  }
}

TEST_F(BuilderTestFixture, BuildG5UsingCustomTable) {
  const double kTestBC = 1.0;
  const uint16_t kTestMuzzleVelocity = 2500U;
  const double kTestZeroAngle = 5.0;
  std::array<float, lob::kTableSize> machs = {};
  std::array<float, lob::kTableSize> drags = {};

  const lob::Input kResult1 =
      puut->BallisticCoefficientPsi(kTestBC)
          .BCDragFunction(lob::DragFunctionT::kG5)
          .BCAtmosphere(lob::AtmosphereReferenceT::kIcao)
          .InitialVelocityFps(kTestMuzzleVelocity)
          .ZeroAngleMOA(kTestZeroAngle)
          .Build();

  for (size_t i = 0; i < lob::kTableSize; i++) {
    machs.at(i) = static_cast<float>(lob::kMachs.at(i)) / lob::kTableScale;
    drags.at(i) = static_cast<float>(lob::kG5Drags.at(i)) / lob::kTableScale;
  }

  const lob::Input kResult2 = puut->BallisticCoefficientPsi(lob::NaN())
                                  .MachVsDragTable(machs, drags)
                                  .Build();

  for (size_t i = 0; i < lob::kTableSize; i++) {
    EXPECT_EQ(kResult1.drags.at(i), kResult2.drags.at(i));
  }
}

TEST_F(BuilderTestFixture, BuildG6UsingCustomTable) {
  const double kTestBC = 1.0;
  const uint16_t kTestMuzzleVelocity = 2500U;
  const double kTestZeroAngle = 5.0;
  std::array<float, lob::kTableSize> machs = {};
  std::array<float, lob::kTableSize> drags = {};

  const lob::Input kResult1 =
      puut->BallisticCoefficientPsi(kTestBC)
          .BCDragFunction(lob::DragFunctionT::kG6)
          .BCAtmosphere(lob::AtmosphereReferenceT::kIcao)
          .InitialVelocityFps(kTestMuzzleVelocity)
          .ZeroAngleMOA(kTestZeroAngle)
          .Build();

  for (size_t i = 0; i < lob::kTableSize; i++) {
    machs.at(i) = static_cast<float>(lob::kMachs.at(i)) / lob::kTableScale;
    drags.at(i) = static_cast<float>(lob::kG6Drags.at(i)) / lob::kTableScale;
  }

  const lob::Input kResult2 = puut->BallisticCoefficientPsi(lob::NaN())
                                  .MachVsDragTable(machs, drags)
                                  .Build();

  for (size_t i = 0; i < lob::kTableSize; i++) {
    EXPECT_EQ(kResult1.drags.at(i), kResult2.drags.at(i));
  }
}

TEST_F(BuilderTestFixture, BuildG7UsingCustomTable) {
  const double kTestBC = 1.0;
  const uint16_t kTestMuzzleVelocity = 2500U;
  const double kTestZeroAngle = 5.0;
  std::array<float, lob::kTableSize> machs = {};
  std::array<float, lob::kTableSize> drags = {};

  const lob::Input kResult1 =
      puut->BallisticCoefficientPsi(kTestBC)
          .BCDragFunction(lob::DragFunctionT::kG7)
          .BCAtmosphere(lob::AtmosphereReferenceT::kIcao)
          .InitialVelocityFps(kTestMuzzleVelocity)
          .ZeroAngleMOA(kTestZeroAngle)
          .Build();

  for (size_t i = 0; i < lob::kTableSize; i++) {
    machs.at(i) = static_cast<float>(lob::kMachs.at(i)) / lob::kTableScale;
    drags.at(i) = static_cast<float>(lob::kG7Drags.at(i)) / lob::kTableScale;
  }

  const lob::Input kResult2 = puut->BallisticCoefficientPsi(lob::NaN())
                                  .MachVsDragTable(machs, drags)
                                  .Build();

  for (size_t i = 0; i < lob::kTableSize; i++) {
    EXPECT_EQ(kResult1.drags.at(i), kResult2.drags.at(i));
  }
}

TEST_F(BuilderTestFixture, BuildG8UsingCustomTable) {
  const double kTestBC = 1.0;
  const uint16_t kTestMuzzleVelocity = 2500U;
  const double kTestZeroAngle = 5.0;
  std::array<float, lob::kTableSize> machs = {};
  std::array<float, lob::kTableSize> drags = {};

  const lob::Input kResult1 =
      puut->BallisticCoefficientPsi(kTestBC)
          .BCDragFunction(lob::DragFunctionT::kG8)
          .BCAtmosphere(lob::AtmosphereReferenceT::kIcao)
          .InitialVelocityFps(kTestMuzzleVelocity)
          .ZeroAngleMOA(kTestZeroAngle)
          .Build();

  for (size_t i = 0; i < lob::kTableSize; i++) {
    machs.at(i) = static_cast<float>(lob::kMachs.at(i)) / lob::kTableScale;
    drags.at(i) = static_cast<float>(lob::kG8Drags.at(i)) / lob::kTableScale;
  }

  const lob::Input kResult2 = puut->BallisticCoefficientPsi(lob::NaN())
                                  .MachVsDragTable(machs, drags)
                                  .Build();

  for (size_t i = 0; i < lob::kTableSize; i++) {
    EXPECT_EQ(kResult1.drags.at(i), kResult2.drags.at(i));
  }
}

TEST_F(BuilderTestFixture, JackOConnorZero) {
  const double kSierraGameKingBC = 0.436;
  const uint16_t kM70MuzzleVelocity = 3100U;
  const double kZeroYardage = 100.0;
  const double kZeroHeight = 3.0;
  const double kExpectedZeroAngle = 6.11;
  const double kError = 0.01;
  const lob::Input kJack =
      puut->BallisticCoefficientPsi(kSierraGameKingBC)
          .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
          .InitialVelocityFps(kM70MuzzleVelocity)
          .ZeroDistanceYds(kZeroYardage)
          .ZeroImpactHeightInches(kZeroHeight)
          .Build();
  EXPECT_NEAR(kJack.zero_angle, kExpectedZeroAngle, kError);
}

TEST_F(BuilderTestFixture, ResetWorks) {
  const double kSierraGameKingBC = 0.436;
  const uint16_t kM70MuzzleVelocity = 3100U;
  const double kZeroYardage = 100.0;
  const double kZeroHeight = 3.0;
  const lob::Input kJack =
      puut->BallisticCoefficientPsi(kSierraGameKingBC)
          .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
          .InitialVelocityFps(kM70MuzzleVelocity)
          .ZeroDistanceYds(kZeroYardage)
          .ZeroImpactHeightInches(kZeroHeight)
          .Build();
  EXPECT_EQ(kJack.error, lob::ErrorT::kNone);
  const lob::Input kReset = puut->Reset().Build();
  EXPECT_TRUE(kReset.error != lob::ErrorT::kNone);
}

TEST_F(BuilderTestFixture, AirPressureError) {
  const double kSierraGameKingBC = 0.436;
  const uint16_t kM70MuzzleVelocity = 3100U;
  const double kZeroYardage = 100.0;
  const double kZeroHeight = 3.0;
  const lob::Input kJack =
      puut->BallisticCoefficientPsi(kSierraGameKingBC)
          .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
          .InitialVelocityFps(kM70MuzzleVelocity)
          .ZeroDistanceYds(kZeroYardage)
          .ZeroImpactHeightInches(kZeroHeight)
          .AirPressureInHg(-1.0)
          .Build();
  EXPECT_EQ(kJack.error, lob::ErrorT::kAirPressureOOR);
}

TEST_F(BuilderTestFixture, FiringSiteAltitudeError) {
  const double kSierraGameKingBC = 0.436;
  const uint16_t kM70MuzzleVelocity = 3100U;
  const double kZeroYardage = 100.0;
  const double kZeroHeight = 3.0;
  const lob::Input kJack =
      puut->BallisticCoefficientPsi(kSierraGameKingBC)
          .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
          .InitialVelocityFps(kM70MuzzleVelocity)
          .ZeroDistanceYds(kZeroYardage)
          .ZeroImpactHeightInches(kZeroHeight)
          .AltitudeOfFiringSiteFt(lob::kIsaStratosphereAltitudeFt + 1)
          .Build();
  EXPECT_EQ(kJack.error, lob::ErrorT::kAltitudeOfFiringSiteOOR);
}

TEST_F(BuilderTestFixture, BarometerAltitudeError) {
  const double kSierraGameKingBC = 0.436;
  const uint16_t kM70MuzzleVelocity = 3100U;
  const double kAltitude = 0.0;
  const double kZeroYardage = 100.0;
  const double kZeroHeight = 3.0;
  const lob::Input kJack =
      puut->BallisticCoefficientPsi(kSierraGameKingBC)
          .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
          .InitialVelocityFps(kM70MuzzleVelocity)
          .ZeroDistanceYds(kZeroYardage)
          .ZeroImpactHeightInches(kZeroHeight)
          .AltitudeOfFiringSiteFt(kAltitude)
          .AltitudeOfBarometerFt(lob::kIsaStratosphereAltitudeFt + 1)
          .Build();
  EXPECT_EQ(kJack.error, lob::ErrorT::kAltitudeOfBarometerOOR);
}

TEST_F(BuilderTestFixture, ThermometerAltitudeError) {
  const double kSierraGameKingBC = 0.436;
  const uint16_t kM70MuzzleVelocity = 3100U;
  const double kAltitude = 0.0;
  const double kZeroYardage = 100.0;
  const double kZeroHeight = 3.0;
  const lob::Input kJack =
      puut->BallisticCoefficientPsi(kSierraGameKingBC)
          .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
          .InitialVelocityFps(kM70MuzzleVelocity)
          .ZeroDistanceYds(kZeroYardage)
          .ZeroImpactHeightInches(kZeroHeight)
          .AltitudeOfFiringSiteFt(kAltitude)
          .AltitudeOfThermometerFt(lob::kIsaStratosphereAltitudeFt + 1)
          .Build();
  EXPECT_EQ(kJack.error, lob::ErrorT::kAltitudeOfThermometerOOR);
}

TEST_F(BuilderTestFixture, AzimuthError) {
  const double kSierraGameKingBC = 0.436;
  const uint16_t kM70MuzzleVelocity = 3100U;
  const double kLatitude = 45.0;
  const double kZeroYardage = 100.0;
  const double kZeroHeight = 3.0;
  const lob::Input kJack =
      puut->BallisticCoefficientPsi(kSierraGameKingBC)
          .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
          .InitialVelocityFps(kM70MuzzleVelocity)
          .ZeroDistanceYds(kZeroYardage)
          .ZeroImpactHeightInches(kZeroHeight)
          .AzimuthDeg(lob::kDegreesPerTurn + 1)
          .LatitudeDeg(kLatitude)
          .Build();
  EXPECT_EQ(kJack.error, lob::ErrorT::kAzimuthOOR);
}

TEST_F(BuilderTestFixture, BallisticCoefficientError) {
  const double kSierraGameKingBC = 0.436;
  const uint16_t kM70MuzzleVelocity = 3100U;
  const double kZeroYardage = 100.0;
  const double kZeroHeight = 3.0;
  const lob::Input kJack =
      puut->BallisticCoefficientPsi(-kSierraGameKingBC)
          .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
          .InitialVelocityFps(kM70MuzzleVelocity)
          .ZeroDistanceYds(kZeroYardage)
          .ZeroImpactHeightInches(kZeroHeight)
          .Build();
  EXPECT_EQ(kJack.error, lob::ErrorT::kBallisticCoefficientOOR);
}

TEST_F(BuilderTestFixture, BaseDiameterError) {
  const double kSierraGameKingBC = 0.436;
  const uint16_t kM70MuzzleVelocity = 3100U;
  const double kZeroYardage = 100.0;
  const double kZeroHeight = 3.0;
  const lob::Input kJack =
      puut->BallisticCoefficientPsi(kSierraGameKingBC)
          .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
          .InitialVelocityFps(kM70MuzzleVelocity)
          .ZeroDistanceYds(kZeroYardage)
          .ZeroImpactHeightInches(kZeroHeight)
          .BaseDiameterInch(-1.0)
          .Build();
  EXPECT_EQ(kJack.error, lob::ErrorT::kBaseDiameterOOR);
}

TEST_F(BuilderTestFixture, DiameterError) {
  const double kSierraGameKingBC = 0.436;
  const uint16_t kM70MuzzleVelocity = 3100U;
  const double kZeroYardage = 100.0;
  const double kZeroHeight = 3.0;
  const lob::Input kJack =
      puut->BallisticCoefficientPsi(kSierraGameKingBC)
          .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
          .InitialVelocityFps(kM70MuzzleVelocity)
          .ZeroDistanceYds(kZeroYardage)
          .ZeroImpactHeightInches(kZeroHeight)
          .DiameterInch(-1.0)
          .Build();
  EXPECT_EQ(kJack.error, lob::ErrorT::kDiameterOOR);
}

TEST_F(BuilderTestFixture, HumidityError) {
  const double kSierraGameKingBC = 0.436;
  const uint16_t kM70MuzzleVelocity = 3100U;
  const double kZeroYardage = 100.0;
  const double kZeroHeight = 3.0;
  const lob::Input kJack =
      puut->BallisticCoefficientPsi(kSierraGameKingBC)
          .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
          .InitialVelocityFps(kM70MuzzleVelocity)
          .ZeroDistanceYds(kZeroYardage)
          .ZeroImpactHeightInches(kZeroHeight)
          .RelativeHumidityPercent(-1.0)
          .Build();
  EXPECT_EQ(kJack.error, lob::ErrorT::kHumidityOOR);
}

TEST_F(BuilderTestFixture, InitialVelocity) {
  const double kSierraGameKingBC = 0.436;
  const double kZeroYardage = 100.0;
  const double kZeroHeight = 3.0;
  const lob::Input kJack =
      puut->BallisticCoefficientPsi(kSierraGameKingBC)
          .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
          .InitialVelocityFps(0)
          .ZeroDistanceYds(kZeroYardage)
          .ZeroImpactHeightInches(kZeroHeight)
          .Build();
  EXPECT_EQ(kJack.error, lob::ErrorT::kInitialVelocityRequired);
}

TEST_F(BuilderTestFixture, LatitudeError) {
  const double kSierraGameKingBC = 0.436;
  const uint16_t kM70MuzzleVelocity = 3100U;
  const double kAzimuth = 0;
  const double kZeroYardage = 100.0;
  const double kZeroHeight = 3.0;
  const lob::Input kJack =
      puut->BallisticCoefficientPsi(kSierraGameKingBC)
          .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
          .InitialVelocityFps(kM70MuzzleVelocity)
          .ZeroDistanceYds(kZeroYardage)
          .ZeroImpactHeightInches(kZeroHeight)
          .AzimuthDeg(kAzimuth)
          .LatitudeDeg(91.0)
          .Build();
  EXPECT_EQ(kJack.error, lob::ErrorT::kLatitudeOOR);
}

TEST_F(BuilderTestFixture, LengthError) {
  const double kSierraGameKingBC = 0.436;
  const uint16_t kM70MuzzleVelocity = 3100U;
  const double kZeroYardage = 100.0;
  const double kZeroHeight = 3.0;
  const lob::Input kJack =
      puut->BallisticCoefficientPsi(kSierraGameKingBC)
          .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
          .InitialVelocityFps(kM70MuzzleVelocity)
          .ZeroDistanceYds(kZeroYardage)
          .ZeroImpactHeightInches(kZeroHeight)
          .LengthInch(-1.0)
          .Build();
  EXPECT_EQ(kJack.error, lob::ErrorT::kLengthOOR);
}

TEST_F(BuilderTestFixture, MachVsDragTableBadParamsIgnored) {
  const uint16_t kM70MuzzleVelocity = 3100U;
  const double kZeroYardage = 100.0;
  const double kZeroHeight = 3.0;
  const lob::Input kJack =
      puut->BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
          .InitialVelocityFps(kM70MuzzleVelocity)
          .ZeroDistanceYds(kZeroYardage)
          .ZeroImpactHeightInches(kZeroHeight)
          .MachVsDragTable(nullptr, nullptr, 0)
          .Build();
  EXPECT_EQ(kJack.error, lob::ErrorT::kBallisticCoefficientRequired);
}

TEST_F(BuilderTestFixture, MassError) {
  const double kSierraGameKingBC = 0.436;
  const uint16_t kM70MuzzleVelocity = 3100U;
  const double kZeroYardage = 100.0;
  const double kZeroHeight = 3.0;
  const lob::Input kJack =
      puut->BallisticCoefficientPsi(kSierraGameKingBC)
          .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
          .InitialVelocityFps(kM70MuzzleVelocity)
          .ZeroDistanceYds(kZeroYardage)
          .ZeroImpactHeightInches(kZeroHeight)
          .MassGrains(-1.0)
          .Build();
  EXPECT_EQ(kJack.error, lob::ErrorT::kMassOOR);
}

TEST_F(BuilderTestFixture, MaximumTimeError) {
  const double kSierraGameKingBC = 0.436;
  const uint16_t kM70MuzzleVelocity = 3100U;
  const double kZeroYardage = 100.0;
  const double kZeroHeight = 3.0;
  const lob::Input kJack =
      puut->BallisticCoefficientPsi(kSierraGameKingBC)
          .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
          .InitialVelocityFps(kM70MuzzleVelocity)
          .ZeroDistanceYds(kZeroYardage)
          .ZeroImpactHeightInches(kZeroHeight)
          .MaximumTime(-1.0)
          .Build();
  EXPECT_EQ(kJack.error, lob::ErrorT::kMaximumTimeOOR);
}

TEST_F(BuilderTestFixture, MeplatDiameterError) {
  const double kSierraGameKingBC = 0.436;
  const uint16_t kM70MuzzleVelocity = 3100U;
  const double kZeroYardage = 100.0;
  const double kZeroHeight = 3.0;
  const lob::Input kJack =
      puut->BallisticCoefficientPsi(kSierraGameKingBC)
          .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
          .InitialVelocityFps(kM70MuzzleVelocity)
          .ZeroDistanceYds(kZeroYardage)
          .ZeroImpactHeightInches(kZeroHeight)
          .MeplatDiameterInch(-1.0)
          .Build();
  EXPECT_EQ(kJack.error, lob::ErrorT::kMeplatDiameterOOR);
}

TEST_F(BuilderTestFixture, NoseLengthError) {
  const double kSierraGameKingBC = 0.436;
  const uint16_t kM70MuzzleVelocity = 3100U;
  const double kZeroYardage = 100.0;
  const double kZeroHeight = 3.0;
  const lob::Input kJack =
      puut->BallisticCoefficientPsi(kSierraGameKingBC)
          .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
          .InitialVelocityFps(kM70MuzzleVelocity)
          .ZeroDistanceYds(kZeroYardage)
          .ZeroImpactHeightInches(kZeroHeight)
          .NoseLengthInch(-1)
          .Build();
  EXPECT_EQ(kJack.error, lob::ErrorT::kNoseLengthOOR);
}

TEST_F(BuilderTestFixture, OgiveRtRError) {
  const double kSierraGameKingBC = 0.436;
  const uint16_t kM70MuzzleVelocity = 3100U;
  const double kZeroYardage = 100.0;
  const double kZeroHeight = 3.0;
  const lob::Input kJack =
      puut->BallisticCoefficientPsi(kSierraGameKingBC)
          .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
          .InitialVelocityFps(kM70MuzzleVelocity)
          .ZeroDistanceYds(kZeroYardage)
          .ZeroImpactHeightInches(kZeroHeight)
          .OgiveRtR(-1)
          .Build();
  EXPECT_EQ(kJack.error, lob::ErrorT::kOgiveRtROOR);
}

TEST_F(BuilderTestFixture, RangeAngleError) {
  const double kSierraGameKingBC = 0.436;
  const uint16_t kM70MuzzleVelocity = 3100U;
  const double kZeroYardage = 100.0;
  const double kZeroHeight = 3.0;
  const lob::Input kJack =
      puut->BallisticCoefficientPsi(kSierraGameKingBC)
          .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
          .InitialVelocityFps(kM70MuzzleVelocity)
          .ZeroDistanceYds(kZeroYardage)
          .ZeroImpactHeightInches(kZeroHeight)
          .RangeAngleDeg(90)
          .Build();
  EXPECT_EQ(kJack.error, lob::ErrorT::kRangeAngleOOR);
}

TEST_F(BuilderTestFixture, TailLengthError) {
  const double kSierraGameKingBC = 0.436;
  const uint16_t kM70MuzzleVelocity = 3100U;
  const double kZeroYardage = 100.0;
  const double kZeroHeight = 3.0;
  const lob::Input kJack =
      puut->BallisticCoefficientPsi(kSierraGameKingBC)
          .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
          .InitialVelocityFps(kM70MuzzleVelocity)
          .ZeroDistanceYds(kZeroYardage)
          .ZeroImpactHeightInches(kZeroHeight)
          .TailLengthInch(-1.0)
          .Build();
  EXPECT_EQ(kJack.error, lob::ErrorT::kTailLengthOOR);
}

TEST_F(BuilderTestFixture, WindHeadingError) {
  const double kSierraGameKingBC = 0.436;
  const uint16_t kM70MuzzleVelocity = 3100U;
  const uint16_t kWindSpeed = 10;
  const double kZeroYardage = 100.0;
  const double kZeroHeight = 3.0;
  const lob::Input kJack =
      puut->BallisticCoefficientPsi(kSierraGameKingBC)
          .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
          .InitialVelocityFps(kM70MuzzleVelocity)
          .ZeroDistanceYds(kZeroYardage)
          .ZeroImpactHeightInches(kZeroHeight)
          .WindHeadingDeg(lob::kDegreesPerTurn * 3)
          .WindSpeedMph(kWindSpeed)
          .Build();
  EXPECT_EQ(kJack.error, lob::ErrorT::kWindHeadingOOR);
}

TEST_F(BuilderTestFixture, ZeroAngleError) {
  const double kSierraGameKingBC = 0.436;
  const uint16_t kM70MuzzleVelocity = 3100U;
  const double kZeroYardage = 100.0;
  const double kZeroHeight = 3.0;
  const lob::Input kJack =
      puut->BallisticCoefficientPsi(kSierraGameKingBC)
          .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
          .InitialVelocityFps(kM70MuzzleVelocity)
          .ZeroDistanceYds(kZeroYardage)
          .ZeroImpactHeightInches(kZeroHeight)
          .ZeroAngleMOA(lob::MoaT(lob::DegreesT(46)).Value())
          .Build();
  EXPECT_EQ(kJack.error, lob::ErrorT::kZeroAngleOOR);
}

TEST_F(BuilderTestFixture, ZeroDistanceError) {
  const double kSierraGameKingBC = 0.436;
  const uint16_t kM70MuzzleVelocity = 3100U;
  const double kZeroYardage = 100.0;
  const double kZeroHeight = 3.0;
  const lob::Input kJack =
      puut->BallisticCoefficientPsi(kSierraGameKingBC)
          .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
          .InitialVelocityFps(kM70MuzzleVelocity)
          .ZeroDistanceYds(-kZeroYardage)
          .ZeroImpactHeightInches(kZeroHeight)
          .Build();
  EXPECT_EQ(kJack.error, lob::ErrorT::kZeroDistanceOOR);
}

TEST_F(BuilderTestFixture, RangeAngleDeg) {
  const double kBc = 0.400;
  const uint16_t kVelocity = 3000U;
  const double kZeroAngle = 5.0;
  const double kRangeAngle = -5.0;
  const double kError = 1E-6;
  const lob::Input kResult =
      puut->BallisticCoefficientPsi(kBc)
          .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
          .InitialVelocityFps(kVelocity)
          .ZeroAngleMOA(kZeroAngle)
          .RangeAngleDeg(kRangeAngle)
          .Build();
  const double kGravityFpsps = -32.1740;
  const double kExpectedGravityX =
      kGravityFpsps *
      std::sin(lob::RadiansT(lob::DegreesT(kRangeAngle)).Value());
  const double kExpectedGravityY =
      kGravityFpsps *
      std::cos(lob::RadiansT(lob::DegreesT(kRangeAngle)).Value());
  EXPECT_NEAR(kResult.gravity.x, kExpectedGravityX, kError);
  EXPECT_NEAR(kResult.gravity.y, kExpectedGravityY, kError);
}

TEST_F(BuilderTestFixture, WindSpeedsAreEquivalent) {
  const double kBc = 0.400;
  const uint16_t kVelocity = 3000U;
  const double kZeroAngle = 5.0;
  const double kError = 1E-6;
  const lob::Input kResult1 =
      puut->BallisticCoefficientPsi(kBc)
          .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
          .InitialVelocityFps(kVelocity)
          .ZeroAngleMOA(kZeroAngle)
          .WindHeadingDeg(45)
          .WindSpeedMph(10)
          .Build();

  const lob::Input kResult2 =
      puut->BallisticCoefficientPsi(kBc)
          .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
          .InitialVelocityFps(kVelocity)
          .ZeroAngleMOA(kZeroAngle)
          .WindHeadingDeg(45)
          .WindSpeedFps(14.6666667)
          .Build();
  EXPECT_NEAR(kResult1.wind.x, kResult2.wind.x, kError);
  EXPECT_NEAR(kResult1.wind.z, kResult2.wind.z, kError);
}

TEST_F(BuilderTestFixture, ReadmeExampleIsValid) {
  const lob::Input kSolverInput =
      lob::Builder()
          .BallisticCoefficientPsi(0.214)
          .BCAtmosphere(lob::AtmosphereReferenceT::kIcao)
          .BCDragFunction(lob::DragFunctionT::kG7)
          .DiameterInch(0.308)
          .LengthInch(1.131)
          .MassGrains(155.0)
          .InitialVelocityFps(2800)
          .ZeroAngleMOA(4.62)
          .OpticHeightInches(2.5)
          .TwistInchesPerTurn(10.0)
          .AirPressureInHg(30.3)
          .TemperatureDegF(63.1)
          .RelativeHumidityPercent(77.0)
          .WindHeading(lob::ClockAngleT::kIII)
          .WindSpeedMph(5.0)
          .LatitudeDeg(43.04)  // hello from Milwaukee!
          .AzimuthDeg(180.0)
          .StepSize(100)
          .Build();
  EXPECT_FALSE(std::isnan(kSolverInput.table_coefficient));
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