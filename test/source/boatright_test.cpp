// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include "boatright.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>

#include "constants.hpp"
#include "eng_units.hpp"
#include "tables.hpp"

namespace tests {

TEST(BoatrightTests, CalculateDynamicPressure) {
  // Test data from Sample Calculations of Calculating Aerodynamic Jump for
  // Firing Point Conditions – Boatright & Ruiz – rev. June/2018
  using lob::boatright::CalculateDynamicPressure;
  const lob::LbsPerCuFtT kAirDensity(0.0764742);
  const lob::FpsT kVelocity(2800);
  const lob::PsiT kExpected(64.704);
  const auto kActual = CalculateDynamicPressure(kAirDensity, kVelocity);
  EXPECT_NEAR(kActual.Value(), kExpected.Value(), 1E-3);
}

TEST(BoatrightTests, CalculateFullNoseLength) {
  // Test data from Sample Calculations of Calculating Aerodynamic Jump for
  // Firing Point Conditions – Boatright & Ruiz – rev. June/2018
  using lob::boatright::CalculateFullNoseLength;
  const lob::CaliberT kLN(2.240);
  const lob::CaliberT kDM(0.211);
  const lob::CaliberT kRT =
      lob::boatright::CalculateRadiusOfTangentOgive(kLN, kDM);
  const double kRTR(0.900);
  const double kExpected(2.5441);
  const lob::CaliberT kActual = CalculateFullNoseLength(kLN, kDM, kRT, kRTR);
  EXPECT_NEAR(kActual.Value(), kExpected, 1E-4);
}

TEST(BoatrightTests, CalculateOgiveVolume155SMK) {
  using lob::boatright::CalculateFrustrumVolume;
  using lob::boatright::CalculateOgiveVolume;
  const lob::InchT kD(0.308);
  const lob::InchT kLN(0.678);
  const lob::InchT kDM(0.068);
  const lob::InchT kL(1.131);
  const lob::InchT kLBT(0.180);
  const lob::InchT kDB(0.250);
  const double kRTR(0.910);
  const lob::CaliberT kRT = lob::boatright::CalculateRadiusOfTangentOgive(
      lob::CaliberT(kLN, kD.Inverse()), lob::CaliberT(kDM, kD.Inverse()));
  const lob::CaliberT kLFN = lob::boatright::CalculateFullNoseLength(
      lob::CaliberT(kLN, kD.Inverse()), lob::CaliberT(kDM, kD.Inverse()), kRT,
      kRTR);
  const lob::InchT kR = lob::InchT(kRT / kRTR, kD);
  const double kBodyVolume =
      (std::pow(kD / 2, 2) * lob::kPi * (kL - kLN - kLBT)).Value();
  const double kTailVolume = CalculateFrustrumVolume(kD, kDB, kLBT);
  const double kExpected(0.061 - kBodyVolume - kTailVolume);
  const auto kActual = CalculateOgiveVolume(kD, kLN, lob::InchT(kLFN, kD), kR);
  EXPECT_NEAR(kActual, kExpected, 1E-3);
}

TEST(BoatrightTests, CalculateOgiveVolume180JLK) {
  using lob::boatright::CalculateFrustrumVolume;
  using lob::boatright::CalculateOgiveVolume;
  const lob::InchT kD(lob::MmT(7));
  const lob::InchT kLN(0.772);
  const lob::InchT kDM(0.075);
  const lob::InchT kL(1.518);
  const lob::InchT kLBT(0.220);
  const lob::InchT kDB(0.224);
  const double kRTR(0.587);  // Calculated from R of 5.146"
  const lob::CaliberT kRT = lob::boatright::CalculateRadiusOfTangentOgive(
      lob::CaliberT(kLN, kD.Inverse()), lob::CaliberT(kDM, kD.Inverse()));
  const lob::CaliberT kLFN = lob::boatright::CalculateFullNoseLength(
      lob::CaliberT(kLN, kD.Inverse()), lob::CaliberT(kDM, kD.Inverse()), kRT,
      kRTR);
  const lob::InchT kR = lob::InchT(kRT / kRTR, kD);
  const double kBodyVolume =
      (std::pow(kD / 2, 2) * lob::kPi * (kL - kLN - kLBT)).Value();
  const double kTailVolume = CalculateFrustrumVolume(kD, kDB, kLBT);
  const double kExpected(0.068 - kBodyVolume - kTailVolume);
  const auto kActual = CalculateOgiveVolume(kD, kLN, lob::InchT(kLFN, kD), kR);
  EXPECT_NEAR(kActual, kExpected, 1E-3);
}

TEST(BoatrightTests, CalculateOgiveVolumeTangent) {
  using lob::boatright::CalculateFrustrumVolume;
  using lob::boatright::CalculateOgiveVolume;
  const lob::InchT kD(.243);
  const lob::InchT kLN(0.573);
  const lob::InchT kDM(0.070);
  const double kRTR(0.999);
  const lob::CaliberT kRT = lob::boatright::CalculateRadiusOfTangentOgive(
      lob::CaliberT(kLN, kD.Inverse()), lob::CaliberT(kDM, kD.Inverse()));
  const lob::CaliberT kLFN = lob::boatright::CalculateFullNoseLength(
      lob::CaliberT(kLN, kD.Inverse()), lob::CaliberT(kDM, kD.Inverse()), kRT,
      kRTR);
  const lob::InchT kR = lob::InchT(kRT / kRTR, kD);
  EXPECT_NEAR(kR.Value(), 1.944, 1E-3);
  const double kExpected(0.016583);
  const auto kActual = CalculateOgiveVolume(kD, kLN, lob::InchT(kLFN, kD), kR);
  EXPECT_NEAR(kActual, kExpected, 1E-3);
}

TEST(BoatrightTests, CalculateOgiveVolumeSecant) {
  using lob::boatright::CalculateFrustrumVolume;
  using lob::boatright::CalculateOgiveVolume;
  const lob::InchT kD(.510);
  const lob::InchT kLN(1.250);
  const lob::InchT kDM(0.050);
  const double kRTR(0.574);
  const lob::CaliberT kRT = lob::boatright::CalculateRadiusOfTangentOgive(
      lob::CaliberT(kLN, kD.Inverse()), lob::CaliberT(kDM, kD.Inverse()));
  const lob::CaliberT kLFN = lob::boatright::CalculateFullNoseLength(
      lob::CaliberT(kLN, kD.Inverse()), lob::CaliberT(kDM, kD.Inverse()), kRT,
      kRTR);
  const lob::InchT kR = lob::InchT(kRT / kRTR, kD);
  EXPECT_NEAR(kR.Value(), 6.12, 1E-2);
  const double kExpected(0.121879);
  const auto kActual = CalculateOgiveVolume(kD, kLN, lob::InchT(kLFN, kD), kR);
  EXPECT_NEAR(kActual, kExpected, 1E-3);
}

TEST(BoatrightTests, CalculateAverageDensity) {
  // Test data from Sample Calculations of Calculating Aerodynamic Jump for
  // Firing Point Conditions – Boatright & Ruiz – rev. June/2018
  using lob::boatright::CalculateAverageDensity;
  const lob::InchT kD(0.308);
  const lob::CaliberT kL(3.945);
  const lob::CaliberT kDM(0.211);
  const lob::CaliberT kLN(2.240);
  const lob::CaliberT kDB(0.786);
  const lob::CaliberT kLBT(0.455);
  const double kRTR(0.900);
  const lob::GrainT kMass(168);
  const lob::CaliberT kRT =
      lob::boatright::CalculateRadiusOfTangentOgive(kLN, kDM);
  const lob::CaliberT kLFN =
      lob::boatright::CalculateFullNoseLength(kLN, kDM, kRT, kRTR);
  const lob::CaliberT kRO = kRT / kRTR;
  // The expected value is an estimate for this category of bullet.
  const double kExpected(2750);
  const double kError(kExpected * 0.10);
  const double kActual =
      CalculateAverageDensity(kD, kL, kLN, kLFN, kRO, kDB, kLBT, kMass);
  EXPECT_NEAR(kActual, kExpected, kError);
}

TEST(BoatrightTests, CalculateCoefficientOfLift) {
  // Test data from Sample Calculations of Calculating Aerodynamic Jump for
  // Firing Point Conditions – Boatright & Ruiz – rev. June/2018
  using lob::boatright::CalculateCoefficientOfLift;
  const lob::CaliberT kLN(2.240);
  const lob::CaliberT kDM(0.211);
  const double kRTR(0.900);
  const lob::CaliberT kRT =
      lob::boatright::CalculateRadiusOfTangentOgive(kLN, kDM);
  const lob::CaliberT kLFN =
      lob::boatright::CalculateFullNoseLength(kLN, kDM, kRT, kRTR);
  const lob::MachT kVelocity(2800 / lob::kIsaSeaLevelSpeedOfSoundFps);
  const double kExpected(2.807);
  const double kActual = CalculateCoefficientOfLift(kLFN, kVelocity);
  EXPECT_NEAR(kActual, kExpected, 1E-3);
}

TEST(BoatrightTests, CalculateInertialRatio) {
  // Test data from Sample Calculations of Calculating Aerodynamic Jump for
  // Firing Point Conditions – Boatright & Ruiz – rev. June/2018
  using lob::boatright::CalculateInertialRatio;
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

TEST(BoatrightTests, CalculateSpinRate) {
  // Test data from Sample Calculations of Calculating Aerodynamic Jump for
  // Firing Point Conditions – Boatright & Ruiz – rev. June/2018
  using lob::boatright::CalculateSpinRate;
  const lob::FpsT kVelocity(2800);
  const lob::InchPerTwistT kTwistRate(12);
  const double kExpected(2800);
  const lob::HzT kActual = CalculateSpinRate(kVelocity, kTwistRate);
  EXPECT_NEAR(kActual.Value(), kExpected, 1E-3);
}

TEST(BoatrightTests, CalculateAspectRatio) {
  // Test data from Sample Calculations of Calculating Aerodynamic Jump for
  // Firing Point Conditions – Boatright & Ruiz – rev. June/2018
  using lob::boatright::CalculateAspectRatio;
  const lob::CaliberT kL(3.945);
  const lob::CaliberT kLFN(2.5441);
  const lob::CaliberT kLBT(0.455);
  const lob::CaliberT kDB(0.786);
  const double kExpected(2.1840);
  const double kActual = CalculateAspectRatio(kL, kLFN, kLBT, kDB);
  EXPECT_NEAR(kActual, kExpected, 1E-4);
}

TEST(BoatrightTests, CalculateYawDragCoefficient) {
  // Test data from Sample Calculations of Calculating Aerodynamic Jump for
  // Firing Point Conditions – Boatright & Ruiz – rev. June/2018
  using lob::boatright::CalculateYawDragCoefficient;
  const lob::MachT kVelocity(2800 / lob::kIsaSeaLevelSpeedOfSoundFps);
  const double kCL(2.807);
  const double kAR(2.1840);
  const double kExpected(4.4212);
  const double kActual = CalculateYawDragCoefficient(kVelocity, kCL, kAR);
  EXPECT_NEAR(kActual, kExpected, 1E-4);
}

TEST(BoatrightTests, CalculateEpicyclicRatio) {
  // Test data from Sample Calculations of Calculating Aerodynamic Jump for
  // Firing Point Conditions – Boatright & Ruiz – rev. June/2018
  using lob::boatright::CalculateEpicyclicRatio;
  const double kSg = 1.74;
  const double kExpected(4.75);
  const double kActual = CalculateEpicyclicRatio(kSg);
  EXPECT_NEAR(kActual, kExpected, 1E-2);
}

TEST(BoatrightTests, CalculateNutationCyclesNeeded) {
  // Test data from Sample Calculations of Calculating Aerodynamic Jump for
  // Firing Point Conditions – Boatright & Ruiz – rev. June/2018
  using lob::boatright::CalculateNutationCyclesNeeded;
  const double kR = 4.75;
  const double kExpected(1);
  const double kActual = CalculateNutationCyclesNeeded(kR);
  EXPECT_NEAR(kActual, kExpected, 1E-2);
}

TEST(BoatrightTests, CalculateGyroscopicRateSum) {
  // Test data from Sample Calculations of Calculating Aerodynamic Jump for
  // Firing Point Conditions – Boatright & Ruiz – rev. June/2018
  using lob::boatright::CalculateGyroscopicRateSum;
  const lob::HzT kP(2800);
  const double kIyPerIx = 7.5482;
  // I believe there is an error in the paper that listed this result as 394 Hz.
  // For subsequent tests I'll use the published values as test inputs but all
  // results downstream of this error are incorrect.
  const double kExpected(371);
  const lob::HzT kActual = CalculateGyroscopicRateSum(kP, kIyPerIx);
  EXPECT_NEAR(kActual.Value(), kExpected, .25);
}

TEST(BoatrightTests, CalculateGyroscopicRateF2) {
  // Test data from Sample Calculations of Calculating Aerodynamic Jump for
  // Firing Point Conditions – Boatright & Ruiz – rev. June/2018
  using lob::boatright::CalculateGyroscopicRateF2;
  const lob::HzT kF1F2Sum(394);
  const double kR = 4.75;
  const double kExpected(68.5);
  const lob::HzT kActual = CalculateGyroscopicRateF2(kF1F2Sum, kR);
  EXPECT_NEAR(kActual.Value(), kExpected, .25);
}

TEST(BoatrightTests, CalculateFirstNutationPeriod) {
  // Test data from Sample Calculations of Calculating Aerodynamic Jump for
  // Firing Point Conditions – Boatright & Ruiz – rev. June/2018
  using lob::boatright::CalculateFirstNutationPeriod;
  const lob::HzT kF1F2Sum(394);
  const lob::HzT kF2(68.5);
  const lob::HzT kF1(kF1F2Sum - kF2);
  const double kExpected(3.891E-3);
  const lob::SecT kActual = CalculateFirstNutationPeriod(kF1, kF2);
  EXPECT_NEAR(kActual.Value(), kExpected, 1E-3);
}

TEST(BoatrightTests, CalculateCrosswindAngleGamma) {
  // Test data from Sample Calculations of Calculating Aerodynamic Jump for
  // Firing Point Conditions – Boatright & Ruiz – rev. June/2018
  using lob::boatright::CalculateCrosswindAngleGamma;
  const lob::FpsT kZWind(14.67);
  const lob::FpsT kVelocity(2800.0);
  const double kExpected(5.239E-3);
  const double kActual = CalculateCrosswindAngleGamma(kZWind, kVelocity);
  EXPECT_NEAR(kActual, kExpected, 1E-3);
}

TEST(BoatrightTests, CalculateZeroYawDragCoefficientOfDrag) {
  // Test data from Sample Calculations of Calculating Aerodynamic Jump for
  // Firing Point Conditions – Boatright & Ruiz – rev. June/2018
  using lob::boatright::CalculateZeroYawDragCoefficientOfDrag;
  const double kCDref = 0.270;
  const lob::GrainT kWt(168);
  const lob::InchT kD(0.308);
  const lob::PmsiT kBcG7(0.223);
  const double kExpected(0.3063);
  const double kActual =
      CalculateZeroYawDragCoefficientOfDrag(kCDref, kWt, kD, kBcG7);
  EXPECT_NEAR(kActual, kExpected, 1E-4);
}

TEST(BoatrightTests, CalculateYawDragAdjustment) {
  // Test data from Sample Calculations of Calculating Aerodynamic Jump for
  // Firing Point Conditions – Boatright & Ruiz – rev. June/2018
  using lob::boatright::CalculateYawDragAdjustment;
  const double kGamma = -5.239E-3;
  const double kR = 4.75;
  const double kCDa = 4.4212;
  const double kCD0 = 0.3063;
  const double kExpected(0.3065 - kCD0);
  const double kActual = CalculateYawDragAdjustment(kGamma, kR, kCDa);
  EXPECT_NEAR(kActual, kExpected, 1E-4);
}

TEST(BoatrightTests, CalculateVerticalPitch) {
  // Test data from Sample Calculations of Calculating Aerodynamic Jump for
  // Firing Point Conditions – Boatright & Ruiz – rev. June/2018
  using lob::boatright::CalculateVerticalPitch;
  const double kGamma = -5.239E-3;
  const double kR = 4.75;
  const double kN = 1;
  const double kExpected(-4.1799E-3);
  const double kActual = CalculateVerticalPitch(kGamma, kR, kN);
  EXPECT_NEAR(kActual, kExpected, 1E-4);
}

TEST(BoatrightTests, CalculateVerticalImpulse) {
  // Test data from Sample Calculations of Calculating Aerodynamic Jump for
  // Firing Point Conditions – Boatright & Ruiz – rev. June/2018
  using lob::boatright::CalculateVerticalImpulse;
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

TEST(BoatrightTests, CalculateMagnitudeOfMomentum) {
  // Test data from Sample Calculations of Calculating Aerodynamic Jump for
  // Firing Point Conditions – Boatright & Ruiz – rev. June/2018
  using lob::boatright::CalculateMagnitudeOfMomentum;
  const lob::GrainT kMass(168);
  const lob::FpsT kVelocity(2800);
  const double kExpected = 2.0886;
  const double kActual = CalculateMagnitudeOfMomentum(kMass, kVelocity);
  EXPECT_NEAR(kActual, kExpected, 1E-4);
}

TEST(BoatrightTests, CalculateBRAerodynamicJump) {
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
  // believe is the result of an improperly calculated gyroscopic rates as well
  // as an estimated rather than calculated average density.
  const double kExpected(-0.463);
  const lob::MoaT kActual = lob::CalculateBRAerodynamicJump(
      kD, kDM, kDB, kL, kLN, kLBT, kRTR, kMass, kV, kSg, kTwist, kZwind,
      kAirDensity, kSos, kBcG7, kCDref);
  EXPECT_NEAR(kActual.Value(), kExpected, 1E-3);
}

TEST(BoatrightTests, CalculateKV) {
  // Test data from Sample Calculations of Calculating Yaw of Repose and Spin
  // Drift - Boatright & Ruiz - Rev September/2018
  const lob::FpsT kInitialVelocity(2600.07);
  const lob::FpsT kTargetVelocity(1340);
  const double kExpected = -0.66287;
  const double kError = 1E-5;
  const double kResult =
      lob::boatright::CalculateKV(kInitialVelocity, kTargetVelocity);
  EXPECT_NEAR(kResult, kExpected, kError);
}

TEST(BoatrightTests, CalculateKOmega) {
  // Test data from Sample Calculations of Calculating Yaw of Repose and Spin
  // Drift - Boatright & Ruiz - Rev September/2018
  const lob::InchT kDiameter(0.308);
  const lob::SecT kTimeToVelcityTarget(1.43);
  const double kExpected = -1.64845 + 0.66287;  // -0.98558;
  const double kError = 1E-5;
  const double kResult =
      lob::boatright::CalculateKOmega(kDiameter, kTimeToVelcityTarget);
  EXPECT_NEAR(kResult, kExpected, kError);
}

struct BRTestFire {
  double diameter;
  double length;
  double ogive_length;
  double meplat_diameter;
  double tail_length;
  double base_diameter;
  double ogive_rtr;
  double mass;
  double rt;
  double lfn;
  double density;
  double iy_per_ix;
  uint16_t velocity;
  double g7_bc;
  double cl0;
  double supersonic_time;
  double clt;
  double twist;
  double sg;
  double f2;
  double komega_kv;
  double beta_r_t;
  double potential_dragf;
  double scf;
  double drop_1000;
  double sd_1000;
};

namespace {
// Test data from Sample Calculations of Calculating Yaw of Repose and Spin
// Drift - Boatright & Ruiz - Rev September/2018
const BRTestFire kInternational{
    0.308,  3.98,   2.26,    0.25,     0.510,  0.7645,  0.900,
    168.0,  6.9976, 2.6392,  2750.0,   7.7748, 2800U,   0.218,
    3.1015, 1.2723, 1.9120,  12.0,     1.74,   62.6387, -1.61385,
    0.4572, 1.1041, 0.01561, 436.0450, 6.8061};
const BRTestFire kM118LR{0.308,    4.4,     2.45,     0.2175, 0.6,    0.8,
                         1.0,      175.16,  7.8666,   2.7598, 2600.0, 9.0376,
                         2600U,    0.2720,  2.6759,   1.43,   1.8463, 11.5,
                         1.94,     45.6182, -1.64845, 0.6909, 1.1041, 0.02185,
                         435.3450, 9.5111};
const BRTestFire kULDSB{0.3002,   5.4368,  2.8368,   0.1000, 0.7012, 0.8420,
                        0.500,    173.0,   9.1666,   3.0690, 2128.0, 13.4975,
                        3200U,    0.3220,  2.5670,   2.1070, 1.7528, 8.25,
                        1.5940,   67.1674, -2.32837, 0.5954, 1.0489, 0.01719,
                        250.0250, 4.2983};
const BRTestFire kBergerTactical{
    0.308,  4.1169, 2.3701,  0.1948,   0.6331, 0.8409,  0.900,
    175.0,  7.1779, 2.6632,  2750.0,   8.1466, 2660U,   0.2580,
    2.8145, 1.3972, 1.8913,  10.0,     2.24,   50.1486, -1.64866,
    0.6144, 1.1041, 0.02009, 428.4970, 8.6095};
const BRTestFire kBergerLRBT{
    0.308,  4.3929, 2.5747,  0.2013,   0.5844, 0.8182,  0.950,
    185.0,  8.4993, 2.8897,  2750.0,   9.1976, 2630U,   0.2830,
    2.6189, 1.5030, 1.8248,  10.0,     1.91,   53.1437, -1.71021,
    0.6098, 1.1041, 0.01820, 414.8350, 7.5496};
}  // namespace

struct SpinDriftParameterizedFixture
    : public ::testing::TestWithParam<BRTestFire> {
  void SetUp() override {}
};

TEST_P(SpinDriftParameterizedFixture, CalculateRadiusOfTangentOgive) {
  const BRTestFire kShot = GetParam();
  const lob::CaliberT kLN(kShot.ogive_length);
  const lob::CaliberT kDM(kShot.meplat_diameter);
  const lob::CaliberT kRT =
      lob::boatright::CalculateRadiusOfTangentOgive(kLN, kDM);
  EXPECT_NEAR(kRT.Value(), kShot.rt, 1E-3);
}

TEST_P(SpinDriftParameterizedFixture, CalculateFullNoseLength) {
  const BRTestFire kShot = GetParam();
  const lob::CaliberT kLN(kShot.ogive_length);
  const lob::CaliberT kDM(kShot.meplat_diameter);
  const double kRTR(kShot.ogive_rtr);
  const lob::CaliberT kRT =
      lob::boatright::CalculateRadiusOfTangentOgive(kLN, kDM);
  const lob::CaliberT kLFN =
      lob::boatright::CalculateFullNoseLength(kLN, kDM, kRT, kRTR);
  EXPECT_NEAR(kLFN.Value(), kShot.lfn, 1E-3);
}

TEST_P(SpinDriftParameterizedFixture, CalculateInertialRatio) {
  const BRTestFire kShot = GetParam();
  const lob::InchT kD(kShot.diameter);
  const lob::CaliberT kL(kShot.length);
  const lob::CaliberT kLN(kShot.ogive_length);
  const lob::CaliberT kLFN(kShot.lfn);
  const lob::GrainT kMass(kShot.mass);
  const lob::CaliberT kDM(kShot.meplat_diameter);
  const lob::CaliberT kDB(kShot.base_diameter);
  const lob::CaliberT kLBT(kShot.tail_length);
  const double kRho = kShot.density;
  const double kIyPerIx =
      lob::boatright::CalculateInertialRatio(kD, kL, kLN, kLFN, kMass, kRho);
  ASSERT_NEAR(kIyPerIx, kShot.iy_per_ix, 1E-3);
}

TEST_P(SpinDriftParameterizedFixture, CalculateAverageDensity) {
  const BRTestFire kShot = GetParam();
  const lob::InchT kD(kShot.diameter);
  const lob::CaliberT kL(kShot.length);
  const lob::CaliberT kLN(kShot.ogive_length);
  const lob::CaliberT kLFN(kShot.lfn);
  const lob::CaliberT kR(kShot.rt / kShot.ogive_rtr);
  const lob::CaliberT kDB(kShot.base_diameter);
  const lob::CaliberT kLBT(kShot.tail_length);
  const lob::GrainT kMass(kShot.mass);
  const double kRho = lob::boatright::CalculateAverageDensity(
      kD, kL, kLN, kLFN, kR, kDB, kLBT, kMass);
  ASSERT_NEAR(kRho, kShot.density, kShot.density * 0.15);
}

TEST_P(SpinDriftParameterizedFixture, CalculateKVPlusOmega) {
  const BRTestFire kShot = GetParam();
  const lob::InchT kD(kShot.diameter);
  const lob::SecT kSST(kShot.supersonic_time);
  const lob::FpsT kVelocity(kShot.velocity);
  const lob::FpsT kTarget(1340);
  const double kV = lob::boatright::CalculateKV(kVelocity, kTarget);
  const double kOmega = lob::boatright::CalculateKOmega(kD, kSST);
  ASSERT_NEAR(kV + kOmega, kShot.komega_kv, 1E-3);
}

TEST_P(SpinDriftParameterizedFixture, CalculatePotentialDragForce) {
  const BRTestFire kShot = GetParam();
  const lob::InchT kD(kShot.diameter);
  const lob::LbsPerCuFtT kAirDensity(0.0764742);
  const lob::FpsT kTarget(1340);
  const double kQTS =
      lob::boatright::CalculatePotentialDragForce(kD, kAirDensity, kTarget);
  ASSERT_NEAR(kQTS, kShot.potential_dragf, 1E-3);
}

TEST_P(SpinDriftParameterizedFixture, CalculateYawOfRepose) {
  const BRTestFire kShot = GetParam();
  const lob::FpsT kVelocity(kShot.velocity);
  const lob::FpsT kTarget(1340);
  const lob::InchPerTwistT kTwist(kShot.twist);
  const double kIyOverIx = kShot.iy_per_ix;
  const double kR = lob::boatright::CalculateEpicyclicRatio(kShot.sg);
  const double kV = lob::boatright::CalculateKV(kVelocity, kTarget);
  const double kOmega = kShot.komega_kv - kV;
  const lob::RadiansT kBetaROfT = lob::boatright::CalculateYawOfRepose(
      kVelocity, kTwist, kIyOverIx, kR, kOmega, kV);
  ASSERT_NEAR(kBetaROfT.Value(), kShot.beta_r_t / 1E3, 1E-3);
}

TEST_P(SpinDriftParameterizedFixture, CalculateCoefficientOfLift) {
  const BRTestFire kShot = GetParam();
  const lob::CaliberT kLFN(kShot.lfn);
  const lob::MachT kVelocity(kShot.velocity / 1116.45);
  const double kBoatTailAdjustmentFactor = std::sqrt(0.2720 / kShot.g7_bc);
  const double kCL0 =
      lob::boatright::CalculateCoefficientOfLift(kLFN, kVelocity) *
      kBoatTailAdjustmentFactor;
  ASSERT_NEAR(kCL0, kShot.cl0, 0.1);
}

TEST_P(SpinDriftParameterizedFixture, CalculateCoefficientOfLiftAtT) {
  const BRTestFire kShot = GetParam();
  const double kCL0(kShot.cl0);
  const lob::FpsT kVelocity(kShot.velocity);
  const lob::SecT kSST(kShot.supersonic_time);
  const double kCLT =
      lob::boatright::CalculateCoefficientOfLiftAtT(kCL0, kVelocity, kSST);
  ASSERT_NEAR(kCLT, kShot.clt, 1E-3);
}

TEST_P(SpinDriftParameterizedFixture, CalculateSpinDriftScaleFactor) {
  const BRTestFire kShot = GetParam();
  const double kQTS(kShot.potential_dragf);
  const lob::RadiansT kBetaROfT(kShot.beta_r_t / 1E3);
  const double kCLT(kShot.clt);
  const lob::GrainT kMass(kShot.mass);
  const double kScF = lob::boatright::CalculateSpinDriftScaleFactor(
      kQTS, kBetaROfT, kCLT, kMass);
  ASSERT_NEAR(kScF, kShot.scf, 1E-3);
}

TEST_P(SpinDriftParameterizedFixture, CalculateSpinDrift) {
  const BRTestFire kShot = GetParam();
  const double kScF(kShot.scf);
  const lob::InchT kDrop(kShot.drop_1000);
  const lob::InchT kSD = lob::boatright::CalculateSpinDrift(kScF, kDrop);
  ASSERT_NEAR(kSD.Value(), kShot.sd_1000, 1E-2);
}

INSTANTIATE_TEST_SUITE_P(BoatrightTests, SpinDriftParameterizedFixture,
                         ::testing::Values(kInternational, kM118LR, kULDSB,
                                           kBergerTactical, kBergerLRBT));

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