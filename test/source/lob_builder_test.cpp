// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <functional>
#include <utility>

#include "constants.hpp"
#include "eng_units.hpp"
#include "helpers.hpp"
#include "test_helpers.hpp"
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

struct BuilderErrorTestParam {
  const char* name;
  std::function<lob::Input(lob::Builder&)> build_fn;
  lob::ErrorT expected_error;
};

class BuilderErrorTestFixture
    : public BuilderTestFixture,
      public testing::WithParamInterface<BuilderErrorTestParam> {};

struct CustomTableTestParam {
  const char* name;
  lob::DragFunctionT drag_function;
  const uint16_t* drag_table;
  double zero_angle;
};

class CustomTableTestFixture
    : public BuilderTestFixture,
      public testing::WithParamInterface<CustomTableTestParam> {};

TEST_F(BuilderTestFixture, Constructor) { ASSERT_NE(puut, nullptr); }

TEST_F(BuilderTestFixture, CopyConstructor) {
  SetupTestBuilder(*puut);
  lob::Builder copy = *puut;
  const lob::Input kVal1 = puut->Build();
  EXPECT_DOUBLE_EQ(kVal1.velocity, 2700U);
  const lob::Input kVal2 = copy.Build();
  EXPECT_DOUBLE_EQ(kVal2.velocity, 2700U);
}

TEST_F(BuilderTestFixture, MoveConstructor) {
  SetupTestBuilder(*puut);
  lob::Builder moved = std::move(*puut);
  const lob::Input kVal = moved.Build();
  EXPECT_EQ(kVal.velocity, 2700U);
}

TEST_F(BuilderTestFixture, CopyAssignmentOperator) {
  SetupTestBuilder(*puut);
  lob::Builder copy;
  copy = *puut;
  const lob::Input kVal1 = puut->Build();
  EXPECT_DOUBLE_EQ(kVal1.velocity, 2700U);
  const lob::Input kVal2 = copy.Build();
  EXPECT_DOUBLE_EQ(kVal2.velocity, 2700U);
}

TEST_F(BuilderTestFixture, MoveAssignmentOperator) {
  SetupTestBuilder(*puut);
  lob::Builder moved;
  moved = std::move(*puut);
  const lob::Input kVal = moved.Build();
  EXPECT_EQ(kVal.velocity, 2700U);
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

TEST_F(BuilderTestFixture, InvalidDragFunctionIsG1) {
  const double kTestBC = 1.0;
  const uint16_t kTestMuzzleVelocity = 2500U;
  const double kTestZeroAngle = 5.59;
  const auto kInvalidDragFunction = static_cast<lob::DragFunctionT>(0xFF);
  const lob::Input kResult = puut->BallisticCoefficientPsi(kTestBC)
                                 .BCDragFunction(kInvalidDragFunction)
                                 .InitialVelocityFps(kTestMuzzleVelocity)
                                 .ZeroAngleMOA(kTestZeroAngle)
                                 .Build();

  EXPECT_THAT(kResult.drags, testing::ElementsAreArray(lob::kG1Drags));
}

TEST_P(CustomTableTestFixture, CustomTableMatchesDragFunction) {
  const auto& param = GetParam();
  const double kTestBC = 1.0;
  const uint16_t kTestMuzzleVelocity = 2500U;
  std::array<float, lob::kTableSize> machs = {};
  std::array<float, lob::kTableSize> drags = {};

  const lob::Input kResult1 =
      puut->BallisticCoefficientPsi(kTestBC)
          .BCDragFunction(param.drag_function)
          .BCAtmosphere(lob::AtmosphereReferenceT::kIcao)
          .InitialVelocityFps(kTestMuzzleVelocity)
          .ZeroAngleMOA(param.zero_angle)
          .Build();

  for (size_t i = 0; i < lob::kTableSize; i++) {
    machs.at(i) = static_cast<float>(lob::kMachs.at(i)) / lob::kTableScale;
    drags.at(i) = static_cast<float>(param.drag_table[i]) / lob::kTableScale;
  }

  const lob::Input kResult2 = puut->BallisticCoefficientPsi(lob::NaN())
                                  .MachVsDragTable(machs, drags)
                                  .Build();

  EXPECT_THAT(kResult1.drags, testing::ElementsAreArray(kResult2.drags));
}

INSTANTIATE_TEST_SUITE_P(
    CustomTableTests, CustomTableTestFixture,
    testing::Values(
        CustomTableTestParam{"G1", lob::DragFunctionT::kG1,
                             lob::kG1Drags.data(), 5.59},
        CustomTableTestParam{"G2", lob::DragFunctionT::kG2,
                             lob::kG2Drags.data(), 5.0},
        CustomTableTestParam{"G5", lob::DragFunctionT::kG5,
                             lob::kG5Drags.data(), 5.0},
        CustomTableTestParam{"G6", lob::DragFunctionT::kG6,
                             lob::kG6Drags.data(), 5.0},
        CustomTableTestParam{"G7", lob::DragFunctionT::kG7,
                             lob::kG7Drags.data(), 5.0},
        CustomTableTestParam{"G8", lob::DragFunctionT::kG8,
                             lob::kG8Drags.data(), 5.0}),
    [](const auto& info) { return info.param.name; });

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

TEST_P(BuilderErrorTestFixture, BuilderReturnsCorrectError) {
  const auto& param = GetParam();
  const lob::Input kJack = param.build_fn(*puut);
  EXPECT_EQ(kJack.error, param.expected_error);
}

INSTANTIATE_TEST_SUITE_P(
    BuilderErrorTests, BuilderErrorTestFixture,
    testing::Values(
        BuilderErrorTestParam{
            "AirPressureOOR",
            [](lob::Builder& b) {
              return b.BallisticCoefficientPsi(0.436)
                  .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
                  .InitialVelocityFps(3100U)
                  .ZeroDistanceYds(100.0)
                  .ZeroImpactHeightInches(3.0)
                  .AirPressureInHg(-1.0)
                  .Build();
            },
            lob::ErrorT::kAirPressureOOR},
        BuilderErrorTestParam{
            "FiringSiteAltitudeOOR",
            [](lob::Builder& b) {
              return b.BallisticCoefficientPsi(0.436)
                  .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
                  .InitialVelocityFps(3100U)
                  .ZeroDistanceYds(100.0)
                  .ZeroImpactHeightInches(3.0)
                  .AltitudeOfFiringSiteFt(
                      lob::kIsaStratosphereAltitudeFt + 1)
                  .Build();
            },
            lob::ErrorT::kAltitudeOfFiringSiteOOR},
        BuilderErrorTestParam{
            "BarometerAltitudeOOR",
            [](lob::Builder& b) {
              return b.BallisticCoefficientPsi(0.436)
                  .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
                  .InitialVelocityFps(3100U)
                  .ZeroDistanceYds(100.0)
                  .ZeroImpactHeightInches(3.0)
                  .AltitudeOfFiringSiteFt(0.0)
                  .AltitudeOfBarometerFt(
                      lob::kIsaStratosphereAltitudeFt + 1)
                  .Build();
            },
            lob::ErrorT::kAltitudeOfBarometerOOR},
        BuilderErrorTestParam{
            "ThermometerAltitudeOOR",
            [](lob::Builder& b) {
              return b.BallisticCoefficientPsi(0.436)
                  .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
                  .InitialVelocityFps(3100U)
                  .ZeroDistanceYds(100.0)
                  .ZeroImpactHeightInches(3.0)
                  .AltitudeOfFiringSiteFt(0.0)
                  .AltitudeOfThermometerFt(
                      lob::kIsaStratosphereAltitudeFt + 1)
                  .Build();
            },
            lob::ErrorT::kAltitudeOfThermometerOOR},
        BuilderErrorTestParam{
            "AzimuthOOR",
            [](lob::Builder& b) {
              return b.BallisticCoefficientPsi(0.436)
                  .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
                  .InitialVelocityFps(3100U)
                  .ZeroDistanceYds(100.0)
                  .ZeroImpactHeightInches(3.0)
                  .AzimuthDeg(lob::kDegreesPerTurn + 1)
                  .LatitudeDeg(45.0)
                  .Build();
            },
            lob::ErrorT::kAzimuthOOR},
        BuilderErrorTestParam{
            "BallisticCoefficientOOR",
            [](lob::Builder& b) {
              return b.BallisticCoefficientPsi(-0.436)
                  .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
                  .InitialVelocityFps(3100U)
                  .ZeroDistanceYds(100.0)
                  .ZeroImpactHeightInches(3.0)
                  .Build();
            },
            lob::ErrorT::kBallisticCoefficientOOR},
        BuilderErrorTestParam{
            "BaseDiameterOOR",
            [](lob::Builder& b) {
              return b.BallisticCoefficientPsi(0.436)
                  .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
                  .InitialVelocityFps(3100U)
                  .ZeroDistanceYds(100.0)
                  .ZeroImpactHeightInches(3.0)
                  .BaseDiameterInch(-1.0)
                  .Build();
            },
            lob::ErrorT::kBaseDiameterOOR},
        BuilderErrorTestParam{
            "DiameterOOR",
            [](lob::Builder& b) {
              return b.BallisticCoefficientPsi(0.436)
                  .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
                  .InitialVelocityFps(3100U)
                  .ZeroDistanceYds(100.0)
                  .ZeroImpactHeightInches(3.0)
                  .DiameterInch(-1.0)
                  .Build();
            },
            lob::ErrorT::kDiameterOOR},
        BuilderErrorTestParam{
            "HumidityOOR",
            [](lob::Builder& b) {
              return b.BallisticCoefficientPsi(0.436)
                  .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
                  .InitialVelocityFps(3100U)
                  .ZeroDistanceYds(100.0)
                  .ZeroImpactHeightInches(3.0)
                  .RelativeHumidityPercent(-1.0)
                  .Build();
            },
            lob::ErrorT::kHumidityOOR},
        BuilderErrorTestParam{
            "InitialVelocityRequired",
            [](lob::Builder& b) {
              return b.BallisticCoefficientPsi(0.436)
                  .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
                  .InitialVelocityFps(0)
                  .ZeroDistanceYds(100.0)
                  .ZeroImpactHeightInches(3.0)
                  .Build();
            },
            lob::ErrorT::kInitialVelocityRequired},
        BuilderErrorTestParam{
            "LatitudeOOR",
            [](lob::Builder& b) {
              return b.BallisticCoefficientPsi(0.436)
                  .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
                  .InitialVelocityFps(3100U)
                  .ZeroDistanceYds(100.0)
                  .ZeroImpactHeightInches(3.0)
                  .AzimuthDeg(0)
                  .LatitudeDeg(91.0)
                  .Build();
            },
            lob::ErrorT::kLatitudeOOR},
        BuilderErrorTestParam{
            "LengthOOR",
            [](lob::Builder& b) {
              return b.BallisticCoefficientPsi(0.436)
                  .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
                  .InitialVelocityFps(3100U)
                  .ZeroDistanceYds(100.0)
                  .ZeroImpactHeightInches(3.0)
                  .LengthInch(-1.0)
                  .Build();
            },
            lob::ErrorT::kLengthOOR},
        BuilderErrorTestParam{
            "MassOOR",
            [](lob::Builder& b) {
              return b.BallisticCoefficientPsi(0.436)
                  .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
                  .InitialVelocityFps(3100U)
                  .ZeroDistanceYds(100.0)
                  .ZeroImpactHeightInches(3.0)
                  .MassGrains(-1.0)
                  .Build();
            },
            lob::ErrorT::kMassOOR},
        BuilderErrorTestParam{
            "MaximumTimeOOR",
            [](lob::Builder& b) {
              return b.BallisticCoefficientPsi(0.436)
                  .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
                  .InitialVelocityFps(3100U)
                  .ZeroDistanceYds(100.0)
                  .ZeroImpactHeightInches(3.0)
                  .MaximumTime(-1.0)
                  .Build();
            },
            lob::ErrorT::kMaximumTimeOOR},
        BuilderErrorTestParam{
            "MeplatDiameterOOR",
            [](lob::Builder& b) {
              return b.BallisticCoefficientPsi(0.436)
                  .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
                  .InitialVelocityFps(3100U)
                  .ZeroDistanceYds(100.0)
                  .ZeroImpactHeightInches(3.0)
                  .MeplatDiameterInch(-1.0)
                  .Build();
            },
            lob::ErrorT::kMeplatDiameterOOR},
        BuilderErrorTestParam{
            "NoseLengthOOR",
            [](lob::Builder& b) {
              return b.BallisticCoefficientPsi(0.436)
                  .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
                  .InitialVelocityFps(3100U)
                  .ZeroDistanceYds(100.0)
                  .ZeroImpactHeightInches(3.0)
                  .NoseLengthInch(-1)
                  .Build();
            },
            lob::ErrorT::kNoseLengthOOR},
        BuilderErrorTestParam{
            "OgiveRtROOR",
            [](lob::Builder& b) {
              return b.BallisticCoefficientPsi(0.436)
                  .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
                  .InitialVelocityFps(3100U)
                  .ZeroDistanceYds(100.0)
                  .ZeroImpactHeightInches(3.0)
                  .OgiveRtR(-1)
                  .Build();
            },
            lob::ErrorT::kOgiveRtROOR},
        BuilderErrorTestParam{
            "RangeAngleOOR",
            [](lob::Builder& b) {
              return b.BallisticCoefficientPsi(0.436)
                  .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
                  .InitialVelocityFps(3100U)
                  .ZeroDistanceYds(100.0)
                  .ZeroImpactHeightInches(3.0)
                  .RangeAngleDeg(90)
                  .Build();
            },
            lob::ErrorT::kRangeAngleOOR},
        BuilderErrorTestParam{
            "TailLengthOOR",
            [](lob::Builder& b) {
              return b.BallisticCoefficientPsi(0.436)
                  .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
                  .InitialVelocityFps(3100U)
                  .ZeroDistanceYds(100.0)
                  .ZeroImpactHeightInches(3.0)
                  .TailLengthInch(-1.0)
                  .Build();
            },
            lob::ErrorT::kTailLengthOOR},
        BuilderErrorTestParam{
            "WindHeadingOOR",
            [](lob::Builder& b) {
              return b.BallisticCoefficientPsi(0.436)
                  .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
                  .InitialVelocityFps(3100U)
                  .ZeroDistanceYds(100.0)
                  .ZeroImpactHeightInches(3.0)
                  .WindHeadingDeg(lob::kDegreesPerTurn * 3)
                  .WindSpeedMph(10)
                  .Build();
            },
            lob::ErrorT::kWindHeadingOOR},
        BuilderErrorTestParam{
            "ZeroAngleOOR",
            [](lob::Builder& b) {
              return b.BallisticCoefficientPsi(0.436)
                  .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
                  .InitialVelocityFps(3100U)
                  .ZeroDistanceYds(100.0)
                  .ZeroImpactHeightInches(3.0)
                  .ZeroAngleMOA(
                      lob::MoaT(lob::DegreesT(46)).Value())
                  .Build();
            },
            lob::ErrorT::kZeroAngleOOR},
        BuilderErrorTestParam{
            "ZeroDistanceOOR",
            [](lob::Builder& b) {
              return b.BallisticCoefficientPsi(0.436)
                  .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
                  .InitialVelocityFps(3100U)
                  .ZeroDistanceYds(-100.0)
                  .ZeroImpactHeightInches(3.0)
                  .Build();
            },
            lob::ErrorT::kZeroDistanceOOR}),
    [](const auto& info) { return info.param.name; });

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