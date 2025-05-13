// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include "boatright.hpp"

#include <gtest/gtest.h>

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
  const double kRTR(0.900);
  const double kExpected(2.5441);
  const lob::CaliberT kActual = CalculateFullNoseLength(kLN, kDM, kRTR);
  EXPECT_NEAR(kActual.Value(), kExpected, 1E-4);
}

TEST(BoatrightTests, CalculateRelativeDensity) {
  // Test data from Sample Calculations of Calculating Aerodynamic Jump for
  // Firing Point Conditions – Boatright & Ruiz – rev. June/2018
  using lob::boatright::CalculateRelativeDensity;
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

TEST(BoatrightTests, CalculateCoefficientOfLift) {
  // Test data from Sample Calculations of Calculating Aerodynamic Jump for
  // Firing Point Conditions – Boatright & Ruiz – rev. June/2018
  using lob::boatright::CalculateCoefficientOfLift;
  const lob::CaliberT kLN(2.240);
  const lob::CaliberT kDM(0.211);
  const double kRTR(0.900);
  const lob::MachT kVelocity(2800 / lob::kIsaSeaLevelSpeedOfSoundFps);
  const double kExpected(2.807);
  const double kActual = CalculateCoefficientOfLift(kLN, kDM, kRTR, kVelocity);
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
  // believe is the result of an improperly calculated gyroscopic rates.
  const double kExpected(-0.424);
  const lob::MoaT kActual = lob::CalculateBRAerodynamicJump(
      kD, kDM, kDB, kL, kLN, kLBT, kRTR, kMass, kV, kSg, kTwist, kZwind,
      kAirDensity, kSos, kBcG7, kCDref);
  EXPECT_NEAR(kActual.Value(), kExpected, 1E-3);
}

TEST(BoatrightTests, CalculateKV) {
  const lob::FpsT kInitialVelocity(2600.07);
  const lob::FpsT kTargetVelocity(1340);
  const double kExpected = -0.66287;
  const double kError = 1E-5;
  const double kResult =
      lob::boatright::CalculateKV(kInitialVelocity, kTargetVelocity);
  EXPECT_NEAR(kResult, kExpected, kError);
}

TEST(BoatrightTests, CalculateKOmega) {
  const lob::InchT kDiameter(0.308);
  const lob::SecT kTimeToVelcityTarget(1.43);
  const double kExpected = -1.64845 + 0.66287;  // -0.98558;
  const double kError = 1E-5;
  const double kResult =
      lob::boatright::CalculateKOmega(kDiameter, kTimeToVelcityTarget);
  EXPECT_NEAR(kResult, kExpected, kError);
}

TEST(BoatrightTests, CalculateYawOfRepose) {
  const lob::FpsT kInitialVelocity(2600.07);
  const lob::InchPerTwistT kTwist(11.5);
  const double kIyOverIx = 9.0376;
  const double kR = 5.5808;
  const double kV = -0.66287;
  const double kOmega = -0.98558;

  const auto kResult = lob::boatright::CalculateYawOfRepose(
      kInitialVelocity, kTwist, kIyOverIx, kR, kOmega, kV);
  const double kExpected = 0.0006909;
  const double kError = 1E-7;
  EXPECT_NEAR(kResult.Value(), kExpected, kError);
}

TEST(BoatrightTests, CalculatePotentialDragForce) {
  const lob::InchT kDiameter(0.308);
  const lob::LbsPerCuFtT kAirDensity(0.0764742);
  const lob::FpsT kTargetVelocity(1340);
  const auto kActual = lob::boatright::CalculatePotentialDragForce(
      kDiameter, kAirDensity, kTargetVelocity);
  const double kExpected = 1.1041;
  const double kError = 1E-4;
  EXPECT_NEAR(kActual, kExpected, kError);
}

TEST(BoatrightTests, CalculateCoefficentOfLiftAtT) {
  const double kCL0(2.6759);
  const lob::FpsT kInitialVelocity(2600.07);
  const lob::SecT kTimeToTargetVelocity(1.430);
  const auto kActual = lob::boatright::CalculateCoefficentOfLiftAtT(
      kCL0, kInitialVelocity, kTimeToTargetVelocity);
  const double kExpected = 1.8463;
  const double kError = 1E-4;
  EXPECT_NEAR(kActual, kExpected, kError);
}

TEST(BoatrightTests, CalculateSpinDriftScaleFactor) {
  const double kPotentialDragForce(1.1041);
  const lob::RadiansT kYawOfReposeT(0.0006909);
  const double kCLT(1.8463);
  const lob::GrainT kMass(175.16);
  const double kActual = lob::boatright::CalculateSpinDriftScaleFactor(
      kPotentialDragForce, kYawOfReposeT, kCLT, kMass);
  const double kExpected = 0.02185;
  const double kError = 1E-5;
  EXPECT_NEAR(kActual, kExpected, kError);
}

TEST(BoatrightTests, CalculateSpinDrift) {
  const double kScaleFactor(0.02185);
  const lob::InchT kDrop(435.3450);
  const auto kActual = lob::boatright::CalculateSpinDrift(kScaleFactor, kDrop);
  const double kExpected = 9.5123;
  const double kError = 1E-4;
  EXPECT_NEAR(kActual.Value(), kExpected, kError);
}

TEST(BoatrightTests, CalculateBRSpinDriftFactor) {
  const lob::InchT kD(0.308);
  const lob::InchT kDM(0.2175 * kD.Value());
  const lob::InchT kDB(0.8000 * kD.Value());
  const lob::InchT kL(4.4 * kD.Value());
  const lob::InchT kLN(2.45 * kD.Value());
  const lob::InchT kLBT(0.6000 * kD.Value());
  const double kRTR(1.000);
  const lob::GrainT kMass(175.16);
  const lob::FpsT kV(2600.17);
  const double kSg(1.94);
  const lob::InchPerTwistT kTwist(11.5);
  const lob::LbsPerCuFtT kAirDensity(0.0764742);
  const lob::SecT kSupersonicTof(1.43);

  const auto kActual = lob::CalculateBRSpinDriftFactor(
      kD, kDM, kDB, kL, kLN, kLBT, kRTR, kMass, kV, kSg, kTwist, kAirDensity,
      kSupersonicTof);
  const double kExpected = 0.02185;
  const double kError = 1E-3;
  EXPECT_NEAR(kActual, kExpected, kError);
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