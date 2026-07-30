// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <utility>

#include "constants.hpp"
#include "eng_units.hpp"
#include "helpers.hpp"
#include "lob/lob.h"
#include "lob/lob.hpp"
#include "splines.hpp"
#include "tables.hpp"
#include "test_helpers.hpp"

namespace tests {

struct BuilderTestFixture : public testing::Test {
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
  std::function<lob::Context(lob::Builder&)> build_fn;
  lob::ErrorT expected_error;
};

class BuilderErrorTestFixture
    : public BuilderTestFixture,
      public testing::WithParamInterface<BuilderErrorTestParam> {};

struct CustomTableTestParam {
  const char* name;
  lob::DragFunctionT drag_function;
  const float* drag_table;
  double zero_angle;
};

class CustomTableTestFixture
    : public BuilderTestFixture,
      public testing::WithParamInterface<CustomTableTestParam> {};

TEST_F(BuilderTestFixture, Constructor) { ASSERT_NE(puut, nullptr); }

TEST_F(BuilderTestFixture, CopyConstructor) {
  SetupTestBuilder(*puut);
  lob::Builder copy = *puut;
  const lob::Context kVal1 = puut->Build();
  EXPECT_DOUBLE_EQ(kVal1.velocity, 2700U);
  const lob::Context kVal2 = copy.Build();
  EXPECT_DOUBLE_EQ(kVal2.velocity, 2700U);
}

TEST_F(BuilderTestFixture, MoveConstructor) {
  SetupTestBuilder(*puut);
  lob::Builder moved = std::move(*puut);
  const lob::Context kVal = moved.Build();
  EXPECT_EQ(kVal.velocity, 2700U);
}

TEST_F(BuilderTestFixture, CopyAssignmentOperator) {
  SetupTestBuilder(*puut);
  lob::Builder copy;
  copy = *puut;
  const lob::Context kVal1 = puut->Build();
  EXPECT_DOUBLE_EQ(kVal1.velocity, 2700U);
  const lob::Context kVal2 = copy.Build();
  EXPECT_DOUBLE_EQ(kVal2.velocity, 2700U);
}

TEST_F(BuilderTestFixture, MoveAssignmentOperator) {
  SetupTestBuilder(*puut);
  lob::Builder moved;
  moved = std::move(*puut);
  const lob::Context kVal = moved.Build();
  EXPECT_EQ(kVal.velocity, 2700U);
}

TEST_F(BuilderTestFixture, BuildMinimalInput) {
  const double kTestBC = 0.425;
  const uint16_t kTestMuzzleVelocity = 2700U;
  const double kTestZeroAngle = 3.84;
  const double kZeroDistance = 100.0;
  const lob::Context kResult = puut->BallisticCoefficientPsi(kTestBC)
                                   .InitialVelocityFps(kTestMuzzleVelocity)
                                   .ZeroDistanceYds(kZeroDistance)
                                   .Build();
  EXPECT_FALSE(std::isnan(kResult.speed_of_sound));
  EXPECT_EQ(kResult.velocity, kTestMuzzleVelocity);
  EXPECT_NEAR(kResult.zero_angle, kTestZeroAngle, 0.01);
  EXPECT_DOUBLE_EQ(kResult.gravity.y, -1.0 * lob::kStandardGravityFtPerSecSq);
}

TEST_F(BuilderTestFixture, BuildMissingVelocityInput) {
  const double kTestBC = 0.425;
  const double kTestZeroAngle = 3.84;
  const lob::Context kResult = puut->BallisticCoefficientPsi(kTestBC)
                                   .ZeroAngleMOA(kTestZeroAngle)
                                   .Build();
  EXPECT_EQ(kResult.error, lob::ErrorT::kInitialVelocityRequired);
}

TEST_F(BuilderTestFixture, BuildMissingBCInput) {
  const uint16_t kTestMuzzleVelocity = 2700U;
  const double kTestZeroAngle = 3.84;
  const lob::Context kResult = puut->InitialVelocityFps(kTestMuzzleVelocity)
                                   .ZeroAngleMOA(kTestZeroAngle)
                                   .Build();
  EXPECT_EQ(kResult.error, lob::ErrorT::kBallisticCoefficientRequired);
}

TEST_F(BuilderTestFixture, BuildMissingZeroInput) {
  const double kTestBC = 0.425;
  const uint16_t kTestMuzzleVelocity = 2700U;
  const lob::Context kResult = puut->BallisticCoefficientPsi(kTestBC)
                                   .InitialVelocityFps(kTestMuzzleVelocity)
                                   .Build();
  EXPECT_EQ(kResult.error, lob::ErrorT::kZeroDataRequired);
}

TEST_F(BuilderTestFixture, InvalidDragFunctionIsG1) {
  const double kTestBC = 1.0;
  const uint16_t kTestMuzzleVelocity = 2500U;
  const double kTestZeroAngle = 5.59;
  // NOLINTNEXTLINE (clang-analyzer-optin.core.EnumCastOutOfRange)
  const auto kInvalidDragFunction = static_cast<lob::DragFunctionT>(0xFF);
  const lob::Context kResult = puut->BallisticCoefficientPsi(kTestBC)
                                   .BCDragFunction(kInvalidDragFunction)
                                   .InitialVelocityFps(kTestMuzzleVelocity)
                                   .ZeroAngleMOA(kTestZeroAngle)
                                   .Build();

  EXPECT_EQ(kResult.error, lob::ErrorT::kNone);
  EXPECT_THAT(kResult.drags, testing::ElementsAreArray(lob::spline::kG1Coefs));
}

TEST_P(CustomTableTestFixture, CustomTableMatchesDragFunction) {
  const auto& param = GetParam();
  const double kTestBC = 1.0;
  const uint16_t kTestMuzzleVelocity = 2500U;
  std::array<float, lob::dragtable::kTableSize> machs = {};
  std::array<float, lob::dragtable::kTableSize> drags = {};

  const lob::Context kResult1 =
      puut->BallisticCoefficientPsi(kTestBC)
          .BCDragFunction(param.drag_function)
          .BCAtmosphere(lob::AtmosphereReferenceT::kIcao)
          .InitialVelocityFps(kTestMuzzleVelocity)
          .ZeroAngleMOA(param.zero_angle)
          .Build();

  for (size_t i = 0; i < lob::dragtable::kTableSize; i++) {
    machs.at(i) = lob::dragtable::kMachs.at(i);
    drags.at(i) = param.drag_table[i];
  }

  const lob::Context kResult2 =
      puut->Reset()
          .BCAtmosphere(lob::AtmosphereReferenceT::kIcao)
          .InitialVelocityFps(kTestMuzzleVelocity)
          .ZeroAngleMOA(param.zero_angle)
          .BallisticCoefficientPsi(lob::NaN())
          .MachVsDragTable(machs, drags)
          .Build();

  ASSERT_EQ(kResult1.error, kLobErrorNone);
  ASSERT_EQ(kResult2.error, kLobErrorNone);
  for (size_t i = 0; i < kResult1.drags.size(); i++) {
    EXPECT_FLOAT_EQ(kResult1.drags.at(i), kResult2.drags.at(i));
  }
}

namespace {
constexpr double kSierraGameKingBC = 0.436;
constexpr uint16_t kM70MuzzleVelocity = 3100U;
constexpr double kM70TwistRate = 10.0;
constexpr double kJackOConnorZeroYardage = 100.0;
constexpr double kJackOConnorZeroHeight = 3.0;
constexpr double kG1ZeroAngle = 5.59;
constexpr double kDefaultZeroAngle = 5.0;
constexpr double kAzimuthOORLatitude = 45.0;
constexpr double kLatitudeOORLatitude = 91.0;
constexpr double kNoslerAccubondMass = 130.0;
constexpr double kNoslerAccubondDiameter = 0.277;
constexpr double kNoslerAccubondLength = 1.234;
constexpr double kNoslerAccubondNoseLength = 0.705;
constexpr double kNoslerAccubondTailLength = 0.070;
constexpr double kNoslerAccubondBaseDiameter = 0.245;
constexpr double kNoslerAccubondMeplatDiameter = 0.0;
constexpr double kNoslerAccubondOgiveRtR = 0.88;
constexpr double kTransonicTimeoutBC = 1.0e6;
}  // namespace

INSTANTIATE_TEST_SUITE_P(
    CustomTableTests, CustomTableTestFixture,
    testing::Values(CustomTableTestParam{"G1", lob::DragFunctionT::kG1,
                                         lob::dragtable::kG1Drags.data(),
                                         kG1ZeroAngle},
                    CustomTableTestParam{"G2", lob::DragFunctionT::kG2,
                                         lob::dragtable::kG2Drags.data(),
                                         kDefaultZeroAngle},
                    CustomTableTestParam{"G5", lob::DragFunctionT::kG5,
                                         lob::dragtable::kG5Drags.data(),
                                         kDefaultZeroAngle},
                    CustomTableTestParam{"G6", lob::DragFunctionT::kG6,
                                         lob::dragtable::kG6Drags.data(),
                                         kDefaultZeroAngle},
                    CustomTableTestParam{"G7", lob::DragFunctionT::kG7,
                                         lob::dragtable::kG7Drags.data(),
                                         kDefaultZeroAngle},
                    CustomTableTestParam{"G8", lob::DragFunctionT::kG8,
                                         lob::dragtable::kG8Drags.data(),
                                         kDefaultZeroAngle}),
    [](const auto& test_param) { return test_param.param.name; });

TEST_F(BuilderTestFixture, JackOConnorZero) {
  const double kExpectedZeroAngle = 6.11;
  const double kError = 0.01;
  const lob::Context kJack =
      puut->BallisticCoefficientPsi(kSierraGameKingBC)
          .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
          .InitialVelocityFps(kM70MuzzleVelocity)
          .ZeroDistanceYds(kJackOConnorZeroYardage)
          .ZeroImpactHeightInches(kJackOConnorZeroHeight)
          .Build();
  EXPECT_NEAR(kJack.zero_angle, kExpectedZeroAngle, kError);
}

TEST_F(BuilderTestFixture, ResetWorks) {
  const lob::Context kResult1 =
      puut->BallisticCoefficientPsi(kSierraGameKingBC)
          .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
          .InitialVelocityFps(kM70MuzzleVelocity)
          .ZeroDistanceYds(kJackOConnorZeroYardage)
          .ZeroImpactHeightInches(kJackOConnorZeroHeight)
          .Build();
  EXPECT_EQ(kResult1.error, lob::ErrorT::kNone);
  const lob::Context kResult2 = puut->Reset().Build();
  EXPECT_TRUE(kResult2.error != lob::ErrorT::kNone);
}

TEST_P(BuilderErrorTestFixture, BuilderReturnsCorrectError) {
  const auto& param = GetParam();
  const lob::Context kResult = param.build_fn(*puut);
  EXPECT_EQ(kResult.error, param.expected_error);
}

INSTANTIATE_TEST_SUITE_P(
    BuilderErrorTests, BuilderErrorTestFixture,
    testing::Values(
        BuilderErrorTestParam{
            "AirPressureOOR",
            [](lob::Builder& b) {
              return b.BallisticCoefficientPsi(kSierraGameKingBC)
                  .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
                  .InitialVelocityFps(kM70MuzzleVelocity)
                  .ZeroDistanceYds(kJackOConnorZeroYardage)
                  .ZeroImpactHeightInches(kJackOConnorZeroHeight)
                  .AirPressureInHg(-1.0)
                  .Build();
            },
            lob::ErrorT::kAirPressureOOR},
        BuilderErrorTestParam{
            "FiringSiteAltitudeOOR",
            [](lob::Builder& b) {
              return b.BallisticCoefficientPsi(kSierraGameKingBC)
                  .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
                  .InitialVelocityFps(kM70MuzzleVelocity)
                  .ZeroDistanceYds(kJackOConnorZeroYardage)
                  .ZeroImpactHeightInches(kJackOConnorZeroHeight)
                  .AltitudeOfFiringSiteFt(lob::kIsaStratosphereAltitudeFt + 1)
                  .Build();
            },
            lob::ErrorT::kAltitudeOfFiringSiteOOR},
        BuilderErrorTestParam{
            "BarometerAltitudeOOR",
            [](lob::Builder& b) {
              return b.BallisticCoefficientPsi(kSierraGameKingBC)
                  .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
                  .InitialVelocityFps(kM70MuzzleVelocity)
                  .ZeroDistanceYds(kJackOConnorZeroYardage)
                  .ZeroImpactHeightInches(kJackOConnorZeroHeight)
                  .AltitudeOfFiringSiteFt(0.0)
                  .AltitudeOfBarometerFt(lob::kIsaStratosphereAltitudeFt + 1)
                  .Build();
            },
            lob::ErrorT::kAltitudeOfBarometerOOR},
        BuilderErrorTestParam{
            "ThermometerAltitudeOOR",
            [](lob::Builder& b) {
              return b.BallisticCoefficientPsi(kSierraGameKingBC)
                  .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
                  .InitialVelocityFps(kM70MuzzleVelocity)
                  .ZeroDistanceYds(kJackOConnorZeroYardage)
                  .ZeroImpactHeightInches(kJackOConnorZeroHeight)
                  .AltitudeOfFiringSiteFt(0.0)
                  .AltitudeOfThermometerFt(lob::kIsaStratosphereAltitudeFt + 1)
                  .Build();
            },
            lob::ErrorT::kAltitudeOfThermometerOOR},
        BuilderErrorTestParam{
            "AzimuthOOR",
            [](lob::Builder& b) {
              return b.BallisticCoefficientPsi(kSierraGameKingBC)
                  .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
                  .InitialVelocityFps(kM70MuzzleVelocity)
                  .ZeroDistanceYds(kJackOConnorZeroYardage)
                  .ZeroImpactHeightInches(kJackOConnorZeroHeight)
                  .AzimuthDeg(lob::kDegreesPerTurn + 1)
                  .LatitudeDeg(kAzimuthOORLatitude)
                  .Build();
            },
            lob::ErrorT::kAzimuthOOR},
        BuilderErrorTestParam{
            "BallisticCoefficientOOR",
            [](lob::Builder& b) {
              return b.BallisticCoefficientPsi(-kSierraGameKingBC)
                  .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
                  .InitialVelocityFps(kM70MuzzleVelocity)
                  .ZeroDistanceYds(kJackOConnorZeroYardage)
                  .ZeroImpactHeightInches(kJackOConnorZeroHeight)
                  .Build();
            },
            lob::ErrorT::kBallisticCoefficientOOR},
        BuilderErrorTestParam{
            "BaseDiameterOOR",
            [](lob::Builder& b) {
              return b.BallisticCoefficientPsi(kSierraGameKingBC)
                  .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
                  .InitialVelocityFps(kM70MuzzleVelocity)
                  .ZeroDistanceYds(kJackOConnorZeroYardage)
                  .ZeroImpactHeightInches(kJackOConnorZeroHeight)
                  .BaseDiameterInch(-1.0)
                  .Build();
            },
            lob::ErrorT::kBaseDiameterOOR},
        BuilderErrorTestParam{
            "DiameterOOR",
            [](lob::Builder& b) {
              return b.BallisticCoefficientPsi(kSierraGameKingBC)
                  .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
                  .InitialVelocityFps(kM70MuzzleVelocity)
                  .ZeroDistanceYds(kJackOConnorZeroYardage)
                  .ZeroImpactHeightInches(kJackOConnorZeroHeight)
                  .DiameterInch(-1.0)
                  .Build();
            },
            lob::ErrorT::kDiameterOOR},
        BuilderErrorTestParam{
            "HumidityOOR",
            [](lob::Builder& b) {
              return b.BallisticCoefficientPsi(kSierraGameKingBC)
                  .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
                  .InitialVelocityFps(kM70MuzzleVelocity)
                  .ZeroDistanceYds(kJackOConnorZeroYardage)
                  .ZeroImpactHeightInches(kJackOConnorZeroHeight)
                  .RelativeHumidityPercent(-1.0)
                  .Build();
            },
            lob::ErrorT::kHumidityOOR},
        BuilderErrorTestParam{
            "InitialVelocityRequired",
            [](lob::Builder& b) {
              return b.BallisticCoefficientPsi(kSierraGameKingBC)
                  .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
                  .InitialVelocityFps(0)
                  .ZeroDistanceYds(kJackOConnorZeroYardage)
                  .ZeroImpactHeightInches(kJackOConnorZeroHeight)
                  .Build();
            },
            lob::ErrorT::kInitialVelocityRequired},
        BuilderErrorTestParam{
            "LatitudeOOR",
            [](lob::Builder& b) {
              return b.BallisticCoefficientPsi(kSierraGameKingBC)
                  .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
                  .InitialVelocityFps(kM70MuzzleVelocity)
                  .ZeroDistanceYds(kJackOConnorZeroYardage)
                  .ZeroImpactHeightInches(kJackOConnorZeroHeight)
                  .AzimuthDeg(0)
                  .LatitudeDeg(kLatitudeOORLatitude)
                  .Build();
            },
            lob::ErrorT::kLatitudeOOR},
        BuilderErrorTestParam{
            "LengthOOR",
            [](lob::Builder& b) {
              return b.BallisticCoefficientPsi(kSierraGameKingBC)
                  .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
                  .InitialVelocityFps(kM70MuzzleVelocity)
                  .ZeroDistanceYds(kJackOConnorZeroYardage)
                  .ZeroImpactHeightInches(kJackOConnorZeroHeight)
                  .LengthInch(-1.0)
                  .Build();
            },
            lob::ErrorT::kLengthOOR},
        BuilderErrorTestParam{
            "MassOOR",
            [](lob::Builder& b) {
              return b.BallisticCoefficientPsi(kSierraGameKingBC)
                  .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
                  .InitialVelocityFps(kM70MuzzleVelocity)
                  .ZeroDistanceYds(kJackOConnorZeroYardage)
                  .ZeroImpactHeightInches(kJackOConnorZeroHeight)
                  .MassGrains(-1.0)
                  .Build();
            },
            lob::ErrorT::kMassOOR},
        BuilderErrorTestParam{
            "MaximumTimeOOR",
            [](lob::Builder& b) {
              return b.BallisticCoefficientPsi(kSierraGameKingBC)
                  .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
                  .InitialVelocityFps(kM70MuzzleVelocity)
                  .ZeroDistanceYds(kJackOConnorZeroYardage)
                  .ZeroImpactHeightInches(kJackOConnorZeroHeight)
                  .MaximumTime(-1.0)
                  .Build();
            },
            lob::ErrorT::kMaximumTimeOOR},
        BuilderErrorTestParam{
            "MeplatDiameterOOR",
            [](lob::Builder& b) {
              return b.BallisticCoefficientPsi(kSierraGameKingBC)
                  .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
                  .InitialVelocityFps(kM70MuzzleVelocity)
                  .ZeroDistanceYds(kJackOConnorZeroYardage)
                  .ZeroImpactHeightInches(kJackOConnorZeroHeight)
                  .MeplatDiameterInch(-1.0)
                  .Build();
            },
            lob::ErrorT::kMeplatDiameterOOR},
        BuilderErrorTestParam{
            "NoseLengthOOR",
            [](lob::Builder& b) {
              return b.BallisticCoefficientPsi(kSierraGameKingBC)
                  .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
                  .InitialVelocityFps(kM70MuzzleVelocity)
                  .ZeroDistanceYds(kJackOConnorZeroYardage)
                  .ZeroImpactHeightInches(kJackOConnorZeroHeight)
                  .NoseLengthInch(-1)
                  .Build();
            },
            lob::ErrorT::kNoseLengthOOR},
        BuilderErrorTestParam{
            "OgiveRtROOR",
            [](lob::Builder& b) {
              return b.BallisticCoefficientPsi(kSierraGameKingBC)
                  .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
                  .InitialVelocityFps(kM70MuzzleVelocity)
                  .ZeroDistanceYds(kJackOConnorZeroYardage)
                  .ZeroImpactHeightInches(kJackOConnorZeroHeight)
                  .OgiveRtR(-1)
                  .Build();
            },
            lob::ErrorT::kOgiveRtROOR},
        BuilderErrorTestParam{
            "RangeAngleOOR",
            [](lob::Builder& b) {
              return b.BallisticCoefficientPsi(kSierraGameKingBC)
                  .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
                  .InitialVelocityFps(kM70MuzzleVelocity)
                  .ZeroDistanceYds(kJackOConnorZeroYardage)
                  .ZeroImpactHeightInches(kJackOConnorZeroHeight)
                  .RangeAngleDeg(90)
                  .Build();
            },
            lob::ErrorT::kRangeAngleOOR},
        BuilderErrorTestParam{
            "TailLengthOOR",
            [](lob::Builder& b) {
              return b.BallisticCoefficientPsi(kSierraGameKingBC)
                  .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
                  .InitialVelocityFps(kM70MuzzleVelocity)
                  .ZeroDistanceYds(kJackOConnorZeroYardage)
                  .ZeroImpactHeightInches(kJackOConnorZeroHeight)
                  .TailLengthInch(-1.0)
                  .Build();
            },
            lob::ErrorT::kTailLengthOOR},
        BuilderErrorTestParam{
            "WindHeadingOOR",
            [](lob::Builder& b) {
              return b.BallisticCoefficientPsi(kSierraGameKingBC)
                  .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
                  .InitialVelocityFps(kM70MuzzleVelocity)
                  .ZeroDistanceYds(kJackOConnorZeroYardage)
                  .ZeroImpactHeightInches(kJackOConnorZeroHeight)
                  .WindHeadingDeg(lob::kDegreesPerTurn * 3)
                  .WindSpeedMph(10)
                  .Build();
            },
            lob::ErrorT::kWindHeadingOOR},
        BuilderErrorTestParam{
            "ZeroAngleOOR",
            [](lob::Builder& b) {
              return b.BallisticCoefficientPsi(kSierraGameKingBC)
                  .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
                  .InitialVelocityFps(kM70MuzzleVelocity)
                  .ZeroDistanceYds(kJackOConnorZeroYardage)
                  .ZeroImpactHeightInches(kJackOConnorZeroHeight)
                  .ZeroAngleMOA(lob::MoaT(lob::DegreesT(46)).Value())
                  .Build();
            },
            lob::ErrorT::kZeroAngleOOR},
        BuilderErrorTestParam{
            "ZeroDistanceOOR",
            [](lob::Builder& b) {
              return b.BallisticCoefficientPsi(kSierraGameKingBC)
                  .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
                  .InitialVelocityFps(kM70MuzzleVelocity)
                  .ZeroDistanceYds(-kJackOConnorZeroYardage)
                  .ZeroImpactHeightInches(kJackOConnorZeroHeight)
                  .Build();
            },
            lob::ErrorT::kZeroDistanceOOR},
        BuilderErrorTestParam{
            "TransonicTimeout",
            [](lob::Builder& b) {
              return b.BallisticCoefficientPsi(kTransonicTimeoutBC)
                  .BCAtmosphere(lob::AtmosphereReferenceT::kIcao)
                  .DiameterInch(kNoslerAccubondDiameter)
                  .LengthInch(kNoslerAccubondLength)
                  .MassGrains(kNoslerAccubondMass)
                  .InitialVelocityFps(kM70MuzzleVelocity)
                  .ZeroAngleMOA(kG1ZeroAngle)
                  .TwistInchesPerTurn(kM70TwistRate)
                  .NoseLengthInch(kNoslerAccubondNoseLength)
                  .TailLengthInch(kNoslerAccubondTailLength)
                  .BaseDiameterInch(kNoslerAccubondBaseDiameter)
                  .MeplatDiameterInch(kNoslerAccubondMeplatDiameter)
                  .OgiveRtR(kNoslerAccubondOgiveRtR)
                  .Build();
            },
            lob::ErrorT::kInternalError},
        BuilderErrorTestParam{"ZeroAngleTimeout",
                              [](lob::Builder& b) {
                                return b
                                    .BallisticCoefficientPsi(kSierraGameKingBC)
                                    .InitialVelocityFps(1)
                                    .ZeroDistanceYds(10000)
                                    .Build();
                              },
                              lob::ErrorT::kInternalError}),
    [](const auto& test_param) { return test_param.param.name; });

TEST_F(BuilderTestFixture, MachVsDragTableBadParamsIgnored) {
  const lob::Context kResult =
      puut->BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
          .InitialVelocityFps(kM70MuzzleVelocity)
          .ZeroDistanceYds(kJackOConnorZeroYardage)
          .ZeroImpactHeightInches(kJackOConnorZeroHeight)
          .MachVsDragTable(nullptr, nullptr, 0)
          .Build();
  EXPECT_EQ(kResult.error, lob::ErrorT::kBallisticCoefficientRequired);
}

TEST_F(BuilderTestFixture, MachVsDragTableCoverageTooNarrow) {
  const std::array<float, 2> kMachs = {0.5F, 5.0F};
  const std::array<float, 2> kDrags = {0.0F, 1.0F};
  const lob::Context kResult =
      puut->BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
          .InitialVelocityFps(kM70MuzzleVelocity)
          .ZeroDistanceYds(kJackOConnorZeroYardage)
          .ZeroImpactHeightInches(kJackOConnorZeroHeight)
          .MachVsDragTable(kMachs, kDrags)
          .Build();
  EXPECT_EQ(kResult.error, lob::ErrorT::kMachDragTableTooNarrow);
}

TEST_F(BuilderTestFixture, MachVsDragTableCoverageTooShort) {
  const std::array<float, 1> kMachs = {0.5F};
  const std::array<float, 1> kDrags = {0.0F};
  const lob::Context kResult =
      puut->BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
          .InitialVelocityFps(kM70MuzzleVelocity)
          .ZeroDistanceYds(kJackOConnorZeroYardage)
          .ZeroImpactHeightInches(kJackOConnorZeroHeight)
          .MachVsDragTable(kMachs, kDrags)
          .Build();
  EXPECT_EQ(kResult.error, lob::ErrorT::kMachDragTableTooShort);
}

TEST_F(BuilderTestFixture, MachVsDragTableCoverageNonMonotonic) {
  const std::array<float, 3> kMachs = {0.0F, 5.0F, 5.0F};
  const std::array<float, 3> kDrags = {0.0F, 0.5F, 1.0F};
  const lob::Context kResult =
      puut->BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
          .InitialVelocityFps(kM70MuzzleVelocity)
          .ZeroDistanceYds(kJackOConnorZeroYardage)
          .ZeroImpactHeightInches(kJackOConnorZeroHeight)
          .MachVsDragTable(kMachs, kDrags)
          .Build();
  EXPECT_EQ(kResult.error, lob::ErrorT::kMachDragTableNotMonotonic);
}

TEST_F(BuilderTestFixture, MachVsDragTableCoverageNegativeMachs) {
  const std::array<float, 2> kMachs = {-1.0F, 5.0F};
  const std::array<float, 2> kDrags = {0.5F, 1.0F};
  const lob::Context kResult =
      puut->BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
          .InitialVelocityFps(kM70MuzzleVelocity)
          .ZeroDistanceYds(kJackOConnorZeroYardage)
          .ZeroImpactHeightInches(kJackOConnorZeroHeight)
          .MachVsDragTable(kMachs, kDrags)
          .Build();
  EXPECT_EQ(kResult.error, lob::ErrorT::kMachDragTableInvalid);
}

TEST_F(BuilderTestFixture, MachVsDragTableCoverageNegativeDrag) {
  const std::array<float, 2> kMachs = {0.0F, 5.0F};
  const std::array<float, 2> kDrags = {0.5F, -1.0F};
  const lob::Context kResult =
      puut->BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
          .InitialVelocityFps(kM70MuzzleVelocity)
          .ZeroDistanceYds(kJackOConnorZeroYardage)
          .ZeroImpactHeightInches(kJackOConnorZeroHeight)
          .MachVsDragTable(kMachs, kDrags)
          .Build();
  EXPECT_EQ(kResult.error, lob::ErrorT::kMachDragTableInvalid);
}

TEST_F(BuilderTestFixture, MachVsDragTableCoverageNaNMachs) {
  const std::array<float, 2> kMachs = {lob::NaN<float>(), 5.0F};
  const std::array<float, 2> kDrags = {0.5F, 1.0F};
  const lob::Context kResult =
      puut->BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
          .InitialVelocityFps(kM70MuzzleVelocity)
          .ZeroDistanceYds(kJackOConnorZeroYardage)
          .ZeroImpactHeightInches(kJackOConnorZeroHeight)
          .MachVsDragTable(kMachs, kDrags)
          .Build();
  EXPECT_EQ(kResult.error, lob::ErrorT::kMachDragTableInvalid);
}

TEST_F(BuilderTestFixture, MachVsDragTableCoverageNaNDrag) {
  const std::array<float, 2> kMachs = {0.0F, 5.0F};
  const std::array<float, 2> kDrags = {0.5F, lob::NaN<float>()};
  const lob::Context kResult =
      puut->BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
          .InitialVelocityFps(kM70MuzzleVelocity)
          .ZeroDistanceYds(kJackOConnorZeroYardage)
          .ZeroImpactHeightInches(kJackOConnorZeroHeight)
          .MachVsDragTable(kMachs, kDrags)
          .Build();
  EXPECT_EQ(kResult.error, lob::ErrorT::kMachDragTableInvalid);
}

TEST_F(BuilderTestFixture, RangeAngleDeg) {
  const double kError = 1E-6;
  const lob::Context kResult =
      puut->BallisticCoefficientPsi(0.400)
          .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
          .InitialVelocityFps(3000U)
          .ZeroAngleMOA(5.0)
          .RangeAngleDeg(-5.0)
          .Build();
  const double kGravityFpsps = -32.1740;
  const double kExpectedGravityX =
      kGravityFpsps * std::sin(lob::RadiansT(lob::DegreesT(-5.0)).Value());
  const double kExpectedGravityY =
      kGravityFpsps * std::cos(lob::RadiansT(lob::DegreesT(-5.0)).Value());
  EXPECT_NEAR(kResult.gravity.x, kExpectedGravityX, kError);
  EXPECT_NEAR(kResult.gravity.y, kExpectedGravityY, kError);
}

TEST_F(BuilderTestFixture, WindSpeedsAreEquivalent) {
  const double kError = 1E-6;
  const lob::Context kResult1 =
      puut->BallisticCoefficientPsi(0.400)
          .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
          .InitialVelocityFps(3000U)
          .ZeroAngleMOA(5.0)
          .WindHeadingDeg(45)
          .WindSpeedMph(10)
          .Build();

  const lob::Context kResult2 =
      puut->BallisticCoefficientPsi(0.400)
          .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
          .InitialVelocityFps(3000U)
          .ZeroAngleMOA(5.0)
          .WindHeadingDeg(45)
          .WindSpeedFps(14.6666667)
          .Build();
  EXPECT_NEAR(kResult1.wind.x, kResult2.wind.x, kError);
  EXPECT_NEAR(kResult1.wind.z, kResult2.wind.z, kError);
}

TEST_F(BuilderTestFixture, ReadmeMinimalExampleProducesExpectedOutput) {
  const lob::Context kCtx = lob::Builder()
                                .BallisticCoefficientPsi(0.425)
                                .InitialVelocityFps(2700)
                                .ZeroDistanceYds(100.0)
                                .Build();
  ASSERT_EQ(kCtx.error, lob::ErrorT::kNone);

  constexpr size_t kNumRanges = 7;
  const std::array<uint32_t, kNumRanges> kRanges = {0U,    300U,  600U, 900U,
                                                    1200U, 1500U, 1800U};
  std::array<lob::Output, kNumRanges> out = {};
  const size_t kCount =
      lob::Solve(kCtx, kRanges.data(), out.data(), kNumRanges);
  ASSERT_EQ(kCount, kNumRanges);

  const std::array<double, kNumRanges> kExpected = {
      -1.50, 0.00, -4.15, -15.01, -33.91, -62.51, -102.87};
  for (size_t i = 0; i < kNumRanges; i++) {
    EXPECT_NEAR(out.at(i).elevation, kExpected.at(i), 0.005)
        << "range=" << kRanges.at(i);
  }
}

TEST_F(BuilderTestFixture, ReadmeExampleIsValid) {
  const lob::Context kSolverInput =
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
          .LatitudeDeg(43.04)
          .AzimuthDeg(180.0)
          .Build();
  EXPECT_EQ(kSolverInput.error, lob::ErrorT::kNone);
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
