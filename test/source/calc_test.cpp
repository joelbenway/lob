// This file is a part of lob, an exterior ballistics calculation library
// Copyright (c) 2025  Joel Benway
// Please see end of file for extended copyright information

#include "calc.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <vector>

#include "constants.hpp"
#include "eng_units.hpp"
#include "tables.hpp"

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
    EXPECT_NEAR(
        kExpectedResultsLbsPerCuFt.at(i),
        lob::CalculateAirDensityAtAltitude(lob::FeetT(kAltitudesFt.at(i)))
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

TEST(CalcTests, CalcualteAirDensityRatio) {
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
        lob::CalcualteAirDensityRatio(lob::InHgT(kPressuresInHg.at(i)),
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

TEST(CalcTests, CalculateLitzGyroscopicSpinDrift) {
  const double kStabilityFactor = 1.83;
  const lob::SecT kTimeOfFlight1(0.7);
  const lob::SecT kTimeOfFlight2(1.75);
  const double kExpectedInches1 = 1.97;
  const double kExpectedInches2 = 10.54;
  const double kError = 0.1;
  const lob::InchT kActualInches1 =
      lob::CalculateLitzGyroscopicSpinDrift(kStabilityFactor, kTimeOfFlight1);
  const lob::InchT kActualInches2 =
      lob::CalculateLitzGyroscopicSpinDrift(kStabilityFactor, kTimeOfFlight2);
  EXPECT_NEAR(kExpectedInches1, kActualInches1.Value(), kError);
  EXPECT_NEAR(kExpectedInches2, kActualInches2.Value(), kError);
  const double kNaN = std::numeric_limits<double>::quiet_NaN();
  const lob::InchT kActualInches3 =
      lob::CalculateLitzGyroscopicSpinDrift(kNaN, kTimeOfFlight1);
  EXPECT_DOUBLE_EQ(kActualInches3.Value(), 0.0);
}

TEST(CalcTests, CalculateLitzAerodynamicJump) {
  const double kError = 0.001;
  const double kSg = 1.74;
  const auto kCal = lob::InchT(0.308);
  const auto kLength = lob::InchT(3.945) * kCal;
  const lob::MphT kCrosswind(10.0);
  const lob::MoaT kExpectedResults(-0.400);
  const lob::MoaT kActualResult =
      lob::CalculateLitzAerodynamicJump(kSg, kCal, kLength, kCrosswind);
  EXPECT_NEAR(kActualResult.Value(), kExpectedResults.Value(), kError);
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

TEST(CalcTests, CalculateDynamicPressure) {
  // Test data from Sample Calculations of Calculating Aerodynamic Jump for
  // Firing Point Conditions – Boatright & Ruiz – rev. June/2018
  using lob::cwaj::CalculateDynamicPressure;
  const lob::LbsPerCuFtT kAirDensity(0.0764742);
  const lob::FpsT kVelocity(2800);
  const lob::PsiT kExpected(64.704);
  const auto kActual = CalculateDynamicPressure(kAirDensity, kVelocity);
  EXPECT_NEAR(kActual.Value(), kExpected.Value(), 1E-3);
}

TEST(CalcTests, CalculateFullNoseLength) {
  // Test data from Sample Calculations of Calculating Aerodynamic Jump for
  // Firing Point Conditions – Boatright & Ruiz – rev. June/2018
  using lob::cwaj::CalculateFullNoseLength;
  const lob::CaliberT kLN(2.240);
  const lob::CaliberT kDM(0.211);
  const double kRTR(0.900);
  const double kExpected(2.5441);
  const lob::CaliberT kActual = CalculateFullNoseLength(kLN, kDM, kRTR);
  EXPECT_NEAR(kActual.Value(), kExpected, 1E-4);
}

TEST(CalcTests, CalculateRelativeDensity) {
  // Test data from Sample Calculations of Calculating Aerodynamic Jump for
  // Firing Point Conditions – Boatright & Ruiz – rev. June/2018
  using lob::cwaj::CalculateRelativeDensity;
  const lob::InchT kD(0.308);
  const lob::InchT kL(3.945 * kD.Value());
  const lob::InchT kDM(0.211 * kD.Value());
  const lob::InchT kLN(2.240 * kD.Value());
  const lob::InchT kDB(0.786 * kD.Value());
  const lob::InchT kLBT(0.455 * kD.Value());
  const lob::GrainT kMass(168);
  // The expected value is an estimate for this category of bullet.
  const double kExpected(2750);
  const double kError(100);
  const double kActual =
      CalculateRelativeDensity(kD, kL, kDM, kLN, kDB, kLBT, kMass);
  EXPECT_NEAR(kActual, kExpected, kError);
}

TEST(CalcTests, CalculateCoefficientOfLift) {
  // Test data from Sample Calculations of Calculating Aerodynamic Jump for
  // Firing Point Conditions – Boatright & Ruiz – rev. June/2018
  using lob::cwaj::CalculateCoefficientOfLift;
  const lob::CaliberT kLN(2.240);
  const lob::CaliberT kDM(0.211);
  const double kRTR(0.900);
  const lob::MachT kVelocity(2800 / lob::kIsaSeaLevelSpeedOfSoundFps);
  const double kExpected(2.807);
  const double kActual = CalculateCoefficientOfLift(kLN, kDM, kRTR, kVelocity);
  EXPECT_NEAR(kActual, kExpected, 1E-3);
}

TEST(CalcTests, CalculateInertialRatio) {
  // Test data from Sample Calculations of Calculating Aerodynamic Jump for
  // Firing Point Conditions – Boatright & Ruiz – rev. June/2018
  using lob::cwaj::CalculateInertialRatio;
  const lob::InchT kCaliber(0.308);
  const lob::CaliberT kL(3.945);
  const lob::CaliberT kLN(2.240);
  const lob::CaliberT kLFN(2.5441);
  const lob::GrainT kMass(168);
  const double kRho(2750);
  const double kExpected(7.5482);
  const double kActual =
      CalculateInertialRatio(kCaliber, kL, kLN, kLFN, kMass, kRho);
  EXPECT_NEAR(kActual, kExpected, 1E-4);
}

TEST(CalcTests, CalculateSpinRate) {
  // Test data from Sample Calculations of Calculating Aerodynamic Jump for
  // Firing Point Conditions – Boatright & Ruiz – rev. June/2018
  using lob::cwaj::CalculateSpinRate;
  const lob::FpsT kVelocity(2800);
  const lob::InchPerTwistT kTwistRate(12);
  const double kExpected(2800);
  const lob::HzT kActual = CalculateSpinRate(kVelocity, kTwistRate);
  EXPECT_NEAR(kActual.Value(), kExpected, 1E-3);
}

TEST(CalcTests, CalculateAspectRatio) {
  // Test data from Sample Calculations of Calculating Aerodynamic Jump for
  // Firing Point Conditions – Boatright & Ruiz – rev. June/2018
  using lob::cwaj::CalculateAspectRatio;
  const lob::CaliberT kL(3.945);
  const lob::CaliberT kLFN(2.5441);
  const lob::CaliberT kLBT(0.455);
  const lob::CaliberT kDB(0.786);
  const double kExpected(2.1840);
  const double kActual = CalculateAspectRatio(kL, kLFN, kLBT, kDB);
  EXPECT_NEAR(kActual, kExpected, 1E-4);
}

TEST(CalcTests, CalculateYawDragCoefficient) {
  // Test data from Sample Calculations of Calculating Aerodynamic Jump for
  // Firing Point Conditions – Boatright & Ruiz – rev. June/2018
  using lob::cwaj::CalculateYawDragCoefficient;
  const lob::MachT kVelocity(2800 / lob::kIsaSeaLevelSpeedOfSoundFps);
  const double kCL(2.807);
  const double kAR(2.1840);
  const double kExpected(4.4212);
  const double kActual = CalculateYawDragCoefficient(kVelocity, kCL, kAR);
  EXPECT_NEAR(kActual, kExpected, 1E-4);
}

TEST(CalcTests, CalculateEpicyclicRatio) {
  // Test data from Sample Calculations of Calculating Aerodynamic Jump for
  // Firing Point Conditions – Boatright & Ruiz – rev. June/2018
  using lob::cwaj::CalculateEpicyclicRatio;
  const double kSg = 1.74;
  const double kExpected(4.75);
  const double kActual = CalculateEpicyclicRatio(kSg);
  EXPECT_NEAR(kActual, kExpected, 1E-2);
}

TEST(CalcTests, CalculateNutationCyclesNeeded) {
  // Test data from Sample Calculations of Calculating Aerodynamic Jump for
  // Firing Point Conditions – Boatright & Ruiz – rev. June/2018
  using lob::cwaj::CalculateNutationCyclesNeeded;
  const double kR = 4.75;
  const double kExpected(1);
  const double kActual = CalculateNutationCyclesNeeded(kR);
  EXPECT_NEAR(kActual, kExpected, 1E-2);
}

TEST(CalcTests, CalculateGyroscopicRateSum) {
  // Test data from Sample Calculations of Calculating Aerodynamic Jump for
  // Firing Point Conditions – Boatright & Ruiz – rev. June/2018
  using lob::cwaj::CalculateGyroscopicRateSum;
  const lob::HzT kP(2800);
  const double kIyPerIx = 7.5482;
  // I believe there is an error in the paper that listed this result as 394 Hz.
  // For subsequent tests I'll use the published values as test inputs but all
  // results downstream of this error are incorrect.
  const double kExpected(371);
  const lob::HzT kActual = CalculateGyroscopicRateSum(kP, kIyPerIx);
  EXPECT_NEAR(kActual.Value(), kExpected, .25);
}

TEST(CalcTests, CalculateGyroscopicRateF2) {
  // Test data from Sample Calculations of Calculating Aerodynamic Jump for
  // Firing Point Conditions – Boatright & Ruiz – rev. June/2018
  using lob::cwaj::CalculateGyroscopicRateF2;
  const lob::HzT kF1F2Sum(394);
  const double kR = 4.75;
  const double kExpected(68.5);
  const lob::HzT kActual = CalculateGyroscopicRateF2(kF1F2Sum, kR);
  EXPECT_NEAR(kActual.Value(), kExpected, .25);
}

TEST(CalcTests, CalculateFirstNutationPeriod) {
  // Test data from Sample Calculations of Calculating Aerodynamic Jump for
  // Firing Point Conditions – Boatright & Ruiz – rev. June/2018
  using lob::cwaj::CalculateFirstNutationPeriod;
  const lob::HzT kF1F2Sum(394);
  const lob::HzT kF2(68.5);
  const lob::HzT kF1(kF1F2Sum - kF2);
  const double kExpected(3.891E-3);
  const lob::SecT kActual = CalculateFirstNutationPeriod(kF1, kF2);
  EXPECT_NEAR(kActual.Value(), kExpected, 1E-3);
}

TEST(CalcTests, CalculateCrosswindAngleGamma) {
  // Test data from Sample Calculations of Calculating Aerodynamic Jump for
  // Firing Point Conditions – Boatright & Ruiz – rev. June/2018
  using lob::cwaj::CalculateCrosswindAngleGamma;
  const lob::FpsT kZWind(14.67);
  const lob::FpsT kVelocity(2800.0);
  const double kExpected(5.239E-3);
  const double kActual = CalculateCrosswindAngleGamma(kZWind, kVelocity);
  EXPECT_NEAR(kActual, kExpected, 1E-3);
}

TEST(CalcTests, CalculateZeroYawDragCoefficientOfDrag) {
  // Test data from Sample Calculations of Calculating Aerodynamic Jump for
  // Firing Point Conditions – Boatright & Ruiz – rev. June/2018
  using lob::cwaj::CalculateZeroYawDragCoefficientOfDrag;
  const double kCDref = 0.270;
  const lob::GrainT kWt(168);
  const lob::InchT kD(0.308);
  const lob::PmsiT kBcG7(0.223);
  const double kExpected(0.3063);
  const double kActual =
      CalculateZeroYawDragCoefficientOfDrag(kCDref, kWt, kD, kBcG7);
  EXPECT_NEAR(kActual, kExpected, 1E-4);
}

TEST(CalcTests, CalculateYawDragAdjustment) {
  // Test data from Sample Calculations of Calculating Aerodynamic Jump for
  // Firing Point Conditions – Boatright & Ruiz – rev. June/2018
  using lob::cwaj::CalculateYawDragAdjustment;
  const double kGamma = -5.239E-3;
  const double kR = 4.75;
  const double kCDa = 4.4212;
  const double kCD0 = 0.3063;
  const double kExpected(0.3065 - kCD0);
  const double kActual = CalculateYawDragAdjustment(kGamma, kR, kCDa);
  EXPECT_NEAR(kActual, kExpected, 1E-4);
}

TEST(CalcTests, CalculateVerticalPitch) {
  // Test data from Sample Calculations of Calculating Aerodynamic Jump for
  // Firing Point Conditions – Boatright & Ruiz – rev. June/2018
  using lob::cwaj::CalculateVerticalPitch;
  const double kGamma = -5.239E-3;
  const double kR = 4.75;
  const double kN = 1;
  const double kExpected(-4.1799E-3);
  const double kActual = CalculateVerticalPitch(kGamma, kR, kN);
  EXPECT_NEAR(kActual, kExpected, 1E-4);
}

TEST(CalcTests, CalculateVerticalImpulse) {
  // Test data from Sample Calculations of Calculating Aerodynamic Jump for
  // Firing Point Conditions – Boatright & Ruiz – rev. June/2018
  using lob::cwaj::CalculateVerticalImpulse;
  const lob::InchPerTwistT kTwist(12);
  const uint16_t kN = 1U;
  const lob::SecT kTn(3.891E-3);
  const lob::PsiT kQ(64.704);
  const lob::SqInT kS(0.074506);
  const double kCL = 2.807;
  const double kCD = 0.3065;
  const double kPitch = -4.1799E-3;
  const double kExpected = -0.00024413;
  const double kActual =
      CalculateVerticalImpulse(kTwist, kN, kTn, kQ, kS, kCL, kCD, kPitch);
  EXPECT_NEAR(kActual, kExpected, 1E-7);
}

TEST(CalcTests, CalculateMagnitudeOfMomentum) {
  // Test data from Sample Calculations of Calculating Aerodynamic Jump for
  // Firing Point Conditions – Boatright & Ruiz – rev. June/2018
  using lob::cwaj::CalculateMagnitudeOfMomentum;
  const lob::GrainT kMass(168);
  const lob::FpsT kVelocity(2800);
  const double kExpected = 2.0886;
  const double kActual = CalculateMagnitudeOfMomentum(kMass, kVelocity);
  EXPECT_NEAR(kActual, kExpected, 1E-4);
}

TEST(CalcTests, CalculateBRAerodynamicJump) {
  // Test data from Sample Calculations of Calculating Aerodynamic Jump for
  // Firing Point Conditions – Boatright & Ruiz – rev. June/2018
  const lob::InchT kD(0.308);
  const lob::InchT kDM(0.211 * kD.Value());
  const lob::InchT kDB(0.786 * kD.Value());
  const lob::InchT kL(3.945 * kD.Value());
  const lob::InchT kLN(2.240 * kD.Value());
  const lob::InchT kLBT(0.455 * kD.Value());
  const double kRTR(.900);
  const lob::PmsiT kBcG7(0.223);
  const lob::GrainT kMass(168.0);
  const lob::FpsT kV(2800);
  const double kSg(1.74);
  const lob::InchPerTwistT kTwist(12);
  const lob::MphT kZwind(10);
  const lob::LbsPerCuFtT kAirDensity(0.0764742);
  const lob::FpsT kSos(1116.45);
  const double kCDref =
      lob::LobLerp(lob::kMachs, lob::kG7Drags, lob::MachT(kV, kSos.Inverse()));
  // Reference paper sample calculation publishes a result of -0.402 which I
  // believe is the result of an improperly calculated gyroscopic rates.
  const double kExpected(-0.424);
  const lob::MoaT kActual = lob::CalculateBRAerodynamicJump(
      kD, kDM, kDB, kL, kLN, kLBT, kRTR, kMass, kV, kSg, kTwist, kZwind,
      kAirDensity, kSos, kBcG7, kCDref);
  EXPECT_NEAR(kActual.Value(), kExpected, 1E-3);
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