// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include "calc.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <vector>

#include "constants.hpp"
#include "eng_units.hpp"

namespace tests {

TEST(CalcTests, CalculateTemperatureAtAltitude) {
  // Test data from page 167 of Modern Exterior Ballistics - McCoy
  const std::vector<uint16_t> kAltitudesFt = {
      0,    500,  1000, 1500,  2000,  3000,  4000,  5000,  6000,
      7000, 8000, 9000, 10000, 15000, 20000, 25000, 30000, 35000};
  const std::vector<double> kExpectedResultsDegF = {
      59.0, 57.2, 55.4, 53.7, 51.9, 48.3,  44.7,  41.2,  37.6,
      34.0, 30.5, 26.9, 23.4, 5.5,  -12.3, -30.0, -47.8, -65.6};
  const double kError = 0.25;

  for (uint32_t i = 0; i < kAltitudesFt.size(); i++) {
    EXPECT_NEAR(
        kExpectedResultsDegF.at(i),
        lob::CalculateTemperatureAtAltitude(lob::FeetT(kAltitudesFt.at(i)),
                                            lob::DegFT(lob::kIsaSeaLevelDegF))
            .Value(),
        kError);
  }
}

TEST(CalcTests, CalculateTemperatureAtAltitudeMcCoy) {
  // Test data from page 167 of Modern Exterior Ballistics - McCoy
  const std::vector<uint16_t> kAltitudesFt = {
      0,    500,  1000, 1500,  2000,  3000,  4000,  5000,  6000,
      7000, 8000, 9000, 10000, 15000, 20000, 25000, 30000, 35000};
  const std::vector<double> kExpectedResultsDegF = {
      59.0, 57.2, 55.4, 53.7, 51.9, 48.3,  44.7,  41.2,  37.6,
      34.0, 30.5, 26.9, 23.4, 5.5,  -12.3, -30.0, -47.8, -65.6};
  // McCoy formula does not seem quite as accurate as using ISA Lapse rate with
  // this test data yet is slower and more complicated.
  const double kError = 0.33;

  for (uint32_t i = 0; i < kAltitudesFt.size(); i++) {
    EXPECT_NEAR(
        kExpectedResultsDegF.at(i),
        lob::CalculateTemperatureAtAltitudeMcCoy(
            lob::FeetT(kAltitudesFt.at(i)), lob::DegFT(lob::kIsaSeaLevelDegF))
            .Value(),
        kError);
  }
}

TEST(CalcTests, BarometricFormula) {
  // Test data from page 167 of Modern Exterior Ballistics - McCoy
  const std::vector<uint16_t> kAltitudesFt = {
      0,    500,  1000, 1500,  2000,  3000,  4000,  5000,  6000,
      7000, 8000, 9000, 10000, 15000, 20000, 25000, 30000, 35000};
  const std::vector<double> kExpectedResultsInHg = {
      29.92, 29.38, 28.86, 28.33, 27.82, 26.82, 25.84, 24.90, 23.98,
      23.09, 22.23, 21.39, 20.58, 16.89, 13.76, 11.12, 8.90,  7.06};
  const double kError = 0.025;
  for (uint32_t i = 0; i < kAltitudesFt.size(); i++) {
    EXPECT_NEAR(
        kExpectedResultsInHg.at(i),
        lob::BarometricFormula(lob::FeetT(kAltitudesFt.at(i)),
                               lob::InHgT(lob::kIsaSeaLevelPressureInHg),
                               lob::DegFT(lob::kIsaSeaLevelDegF))
            .Value(),
        kError);
  }
}

TEST(CalcTests, BarometricFormulaNegative) {
  constexpr int16_t kAltitude = -1000;
  constexpr double kExpectedResult = 31.02;
  const double kError = 0.025;
  EXPECT_NEAR(lob::BarometricFormula(lob::FeetT(kAltitude),
                                     lob::InHgT(lob::kIsaSeaLevelPressureInHg),
                                     lob::DegFT(lob::kIsaSeaLevelDegF))
                  .Value(),
              kExpectedResult, kError);
}

TEST(CalcTests, CalculateAirDensityAtAltitude) {
  // Test data from page 167 of Modern Exterior Ballistics - McCoy
  const std::vector<uint16_t> kAltitudesFt = {
      0,    500,  1000, 1500,  2000,  3000,  4000,  5000,  6000,
      7000, 8000, 9000, 10000, 15000, 20000, 25000, 30000, 35000};
  const double kP0 = lob::kIsaSeaLevelAirDensityLbsPerCuFt;
  const std::vector<double> kExpectedResultsLbsPerCuFt = {
      1.0 * kP0,  .985 * kP0, .971 * kP0, .957 * kP0, .943 * kP0, .915 * kP0,
      .888 * kP0, .862 * kP0, .836 * kP0, .811 * kP0, .786 * kP0, .761 * kP0,
      .739 * kP0, .629 * kP0, .533 * kP0, .449 * kP0, .375 * kP0, .310 * kP0};
  const double kError = 0.1;
  for (uint32_t i = 0; i < kAltitudesFt.size(); i++) {
    EXPECT_NEAR(kExpectedResultsLbsPerCuFt.at(i),
                lob::CalculateAirDensityAtAltitude(
                    lob::FeetT(kAltitudesFt.at(i)),
                    lob::LbsPerCuFtT(lob::kIsaSeaLevelAirDensityLbsPerCuFt))
                    .Value(),
                kError);
  }
}

TEST(CalcTests, CalculateSpeedOfSoundInAir) {
  // Test data from page 169 of Modern Exterior Ballistics - McCoy
  const std::vector<uint8_t> kTempsDegF = {0, 32, 59, 70, 100, 130};
  const std::vector<double> kExpectedResultsFps = {1051.0, 1087.0, 1116.45,
                                                   1128.2, 1159.7, 1190.4};
  const double kError = 0.1;
  for (uint32_t i = 0; i < kTempsDegF.size(); i++) {
    EXPECT_NEAR(
        kExpectedResultsFps.at(i),
        lob::CalculateSpeedOfSoundInAir(lob::DegFT(kTempsDegF.at(i))).Value(),
        kError);
  }
}

TEST(CalcTests, CalculateWaterVaporSaturationPressure) {
  // Test data from paper: A Simple Accurate Formula for Calculating
  // Saturation Vapor Pressure of Water and Ice - Huang.
  const std::vector<lob::DegFT> kTemps = {
      lob::DegCT(-100), lob::DegCT(-80), lob::DegCT(-60), lob::DegCT(-40),
      lob::DegCT(-20),  lob::DegCT(0),   lob::DegCT(0.1), lob::DegCT(20),
      lob::DegCT(40),   lob::DegCT(60),  lob::DegCT(80),  lob::DegCT(100)};

  const std::vector<lob::InHgT> kExpectedResults = {
      lob::PaT(0.0014049), lob::PaT(0.054773), lob::PaT(1.0813),
      lob::PaT(12.8412),   lob::PaT(103.239),  lob::PaT(611.153),
      lob::PaT(611.655),   lob::PaT(2339.32),  lob::PaT(7384.94),
      lob::PaT(19946.4),   lob::PaT(47414.5),  lob::PaT(101418.0)};

  const double kError = 0.1;
  for (uint32_t i = 0; i < kTemps.size(); i++) {
    EXPECT_NEAR(
        kExpectedResults.at(i).Value(),
        lob::CalculateWaterVaporSaturationPressure(kTemps.at(i)).Value(),
        kError);
  }
}

TEST(CalcTests, CalculateAirDensityRatio) {
  // Test data from page 168 of Modern Exterior Ballistics - McCoy
  const std::vector<double> kPressuresInHg = {
      29.92, 29.38, 28.86, 28.33, 27.82, 26.82, 25.84, 24.90, 23.98,
      23.09, 22.23, 21.39, 20.58, 16.89, 13.76, 11.12, 8.90,  7.06};
  const std::vector<double> kTemperaturesDegF = {
      59.0, 57.2, 55.4, 53.7, 51.9, 48.3,  44.7,  41.2,  37.6,
      34.0, 30.5, 26.9, 23.4, 5.5,  -12.3, -30.0, -47.8, -65.6};
  const std::vector<double> kExpectedResults = {
      1.0,  .985, .971, .957, .943, .915, .888, .862, .836,
      .811, .786, .761, .739, .629, .533, .449, .375, .310};
  const double kError = 0.1;
  for (uint32_t i = 0; i < kPressuresInHg.size(); i++) {
    EXPECT_NEAR(
        kExpectedResults.at(i),
        lob::CalculateAirDensityRatio(lob::InHgT(kPressuresInHg.at(i)),
                                      lob::DegFT(kTemperaturesDegF.at(i))),
        kError);
  }
}

TEST(CalcTests, CalculateAirDensityRatioHumidityCorrection) {
  // Test data from page 169 of Modern Exterior Ballistics - McCoy
  const std::vector<uint8_t> kTempsDegF = {0, 32, 59, 70, 100, 130};
  const std::vector<double> kUncorrectedDensities = {1.128, 1.055, 1.0,
                                                     .979,  .927,  .880};
  const std::vector<uint8_t> kRelativeHumidities = {0, 50, 78, 100};
  const std::vector<double> kExpectedResults = {
      1.128, 1.128, 1.128, 1.128, 1.055, 1.054, 1.053, 1.053,
      1.000, .997,  .995,  .994,  .979,  .975,  .972,  .970,
      .927,  .915,  .909,  .904,  .880,  .854,  .840,  .829};
  const double kError = 0.1;
  for (uint32_t i = 0; i < kTempsDegF.size(); i++) {
    for (uint32_t j = 0; j < kRelativeHumidities.size(); j++) {
      EXPECT_NEAR(kExpectedResults.at((i * kRelativeHumidities.size()) + j),
                  kUncorrectedDensities.at(i) *
                      lob::CalculateAirDensityRatioHumidityCorrection(
                          kRelativeHumidities.at(j),
                          lob::CalculateWaterVaporSaturationPressure(
                              lob::DegFT(kTempsDegF.at(i)))),
                  kError);
    }
  }
}

TEST(CalcTests, CalculateSpeedOfSoundHumidityCorrection) {
  // Test data from page 169 of Modern Exterior Ballistics - McCoy
  const std::vector<uint8_t> kTempsDegF = {0, 32, 59, 70, 100, 130};
  const std::vector<double> kUncorrectedSpeedOfSoundFps = {
      1051.0, 1087.0, 1116.45, 1128.2, 1159.7, 1190.4};
  const std::vector<uint8_t> kRelativeHumidities = {0, 50, 78, 100};
  const std::vector<double> kExpectedResultsFps = {
      1051.0,  1051.2, 1051.2, 1051.3, 1087.0, 1087.5, 1087.7, 1087.9,
      1116.45, 1117.8, 1118.5, 1119.1, 1128.2, 1130.2, 1131.3, 1132.1,
      1159.7,  1165.0, 1167.9, 1170.2, 1190.4, 1203.0, 1210.1, 1215.7};
  const double kError = 1.0;
  for (uint32_t i = 0; i < kTempsDegF.size(); i++) {
    for (uint32_t j = 0; j < kRelativeHumidities.size(); j++) {
      EXPECT_NEAR(kExpectedResultsFps.at((i * kRelativeHumidities.size()) + j),
                  kUncorrectedSpeedOfSoundFps.at(i) *
                      lob::CalculateSpeedOfSoundHumidityCorrection(
                          kRelativeHumidities.at(j),
                          lob::CalculateWaterVaporSaturationPressure(
                              lob::DegFT(kTempsDegF.at(i)))),
                  kError);
    }
  }
}

TEST(CalcTests, CalculateCdCoefficient) {
  // Test data from Ball M1911 round
  const lob::PmsiT kBC(0.162);
  const lob::InchT kDiameter(0.452);
  const lob::LbsT kMass(lob::GrainT(230));
  const lob::LbsPerCuFtT kAirDensity(0.0765);
  const double kCdCoeff1 = CalculateCdCoefficient(kAirDensity, kBC);
  const double kCdcoeff2 =
      kAirDensity.Value() *
      lob::SqFtT(CalculateProjectileReferenceArea(kDiameter)).Value() /
      (2 * kMass.Value());
  EXPECT_NEAR(kCdCoeff1, kCdcoeff2, 1E-5);
}

TEST(CalcTests, CalculateMillerTwistRuleStabilityFactor) {
  // Test data from Sample Calculations section of A New Rule for Estimating
  // Rifling Twist - Miller
  const auto kTestBulletDiameter = lob::InchT(.243);
  const auto kTestBulletMass = lob::GrainT(70.0);
  const auto kTestBulletLength = lob::InchT(.83);
  const auto kTestTwistRate = lob::InchPerTwistT(14.0);
  const auto kTestMuzzleVelocity = lob::FpsT(3350.0);
  const double kExpectedStabilityFactor = 1.083;

  auto result = CalculateMillerTwistRuleStabilityFactor(
      kTestBulletDiameter, kTestBulletMass, kTestBulletLength, kTestTwistRate,
      kTestMuzzleVelocity);

  const double kError = .01;
  EXPECT_NEAR(result, kExpectedStabilityFactor, kError);
}

TEST(CalcTests, CalculateMillerTwistRuleCorrectionFactor) {
  // Test data from Sample Calculations section of A New Rule for Estimating
  // Rifling Twist - Miller
  const auto kTestPressure = lob::InHgT(lob::kIsaSeaLevelPressureInHg);
  const auto kTestTemperature = lob::DegFT(-10.0);
  const double kExpectedCorrectionFactor = 0.8671;

  auto result = lob::CalculateMillerTwistRuleCorrectionFactor(kTestPressure,
                                                              kTestTemperature);
  const double kError = .01;
  EXPECT_NEAR(result, kExpectedCorrectionFactor, kError);
}

TEST(CalcTests, CalculateProjectileReferenceArea) {
  EXPECT_NEAR(CalculateProjectileReferenceArea(lob::InchT(0.308)).Value(),
              0.074506, 1E-3);
}

TEST(CalcTests, CalculateKineticEnergy) {
  EXPECT_NEAR(CalculateKineticEnergy(lob::FpsT(3000), lob::GrainT(180)).Value(),
              3596.5, 0.1);
  const double kNaN = std::numeric_limits<double>::quiet_NaN();
  EXPECT_DOUBLE_EQ(
      CalculateKineticEnergy(lob::FpsT(kNaN), lob::GrainT(kNaN)).Value(), 0.0);
}

TEST(CalcTests, CalculateVelocityFromKineticEnergy) {
  const auto kVelocity = lob::FpsT(3'000);
  const auto kMass = lob::GrainT(180.0);
  const auto kEnergy = CalculateKineticEnergy(kVelocity, kMass);
  const auto kResult = lob::CalculateVelocityFromKineticEnergy(kEnergy, kMass);
  EXPECT_DOUBLE_EQ(kResult.Value(), kVelocity.Value());
}

TEST(CalcTests, CalculateSectionalDensity) {
  EXPECT_NEAR(
      CalculateSectionalDensity(lob::InchT(.224), lob::GrainT(77)).Value(),
      0.219, 1E-3);
  EXPECT_NEAR(
      CalculateSectionalDensity(lob::InchT(.308), lob::GrainT(168)).Value(),
      0.253, 1E-3);
  EXPECT_NEAR(
      CalculateSectionalDensity(lob::InchT(.375), lob::GrainT(270)).Value(),
      0.274, 1E-3);
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