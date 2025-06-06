// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include "litz.hpp"

#include <gtest/gtest.h>

#include <limits>

#include "eng_units.hpp"

namespace tests {

TEST(LitzTests, CalculateBallisticCoefficient) {
  const lob::GrainT kMass(155);
  const lob::InchT kDiameter(0.308);
  const double kG7FormFactor = 1.05;
  const double kExpected(0.222);
  const double kError(0.001);
  const double kActual =
      lob::litz::CalculateBallisticCoefficient(kMass, kDiameter, kG7FormFactor)
          .Value();
  EXPECT_NEAR(kActual, kExpected, kError);
}

TEST(LitzTests, CalculateGyroscopicSpinDrift) {
  const double kStabilityFactor = 1.83;
  const lob::SecT kTimeOfFlight1(0.7);
  const lob::SecT kTimeOfFlight2(1.75);
  const double kExpectedInches1 = 1.97;
  const double kExpectedInches2 = 10.54;
  const double kError = 0.1;
  const lob::InchT kActualInches1 =
      lob::litz::CalculateGyroscopicSpinDrift(kStabilityFactor, kTimeOfFlight1);
  const lob::InchT kActualInches2 =
      lob::litz::CalculateGyroscopicSpinDrift(kStabilityFactor, kTimeOfFlight2);
  EXPECT_NEAR(kExpectedInches1, kActualInches1.Value(), kError);
  EXPECT_NEAR(kExpectedInches2, kActualInches2.Value(), kError);
  const double kNaN = std::numeric_limits<double>::quiet_NaN();
  const lob::InchT kActualInches3 =
      lob::litz::CalculateGyroscopicSpinDrift(kNaN, kTimeOfFlight1);
  EXPECT_DOUBLE_EQ(kActualInches3.Value(), 0.0);
}

TEST(LitzTests, CalculateAerodynamicJump) {
  const double kError = 0.001;
  const double kSg = 1.74;
  const auto kCal = lob::InchT(0.308);
  const auto kLength = lob::InchT(3.945) * kCal;
  const lob::MphT kCrosswind(10.0);
  const lob::MoaT kExpectedResults(-0.400);
  const lob::MoaT kActualResult =
      lob::litz::CalculateAerodynamicJump(kSg, kCal, kLength, kCrosswind);
  EXPECT_NEAR(kActualResult.Value(), kExpectedResults.Value(), kError);
}

TEST(LitzTests, CalculateG7FormFactorPrediction) {
  const lob::InchT kD(0.284);
  const lob::CaliberT kLN(lob::InchT(0.763), kD.Inverse());
  const double kRTR(0.57);
  const lob::CaliberT kDM(lob::InchT(0.064), kD.Inverse());
  const lob::CaliberT kLBT(lob::InchT(0.200), kD.Inverse());
  const lob::DegreesT kBoattailAngle(8.4);
  const lob::CaliberT kDB(lob::InchT(0.225), kD.Inverse());
  const double kActual1 = lob::litz::CalculateG7FormFactorPrediction(
      kD, kLN, kRTR, kDM, kLBT, kBoattailAngle);
  const double kActual2 =
      lob::litz::CalculateG7FormFactorPrediction(kD, kLN, kRTR, kDM, kLBT, kDB);
  const double kExpected(0.926);
  const double kError(.05 * kExpected);
  EXPECT_NEAR(kActual1, kActual2, 1E-3);
  EXPECT_NEAR(kActual1, kExpected, kError);
  EXPECT_NEAR(kActual2, kExpected, kError);
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