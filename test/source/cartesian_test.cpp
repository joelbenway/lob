// This file is a part of lob, an exterior ballistics calculation library
// Copyright (c) 2024  Joel Benway
// Please see end of file for extended copyright information

#include "cartesian.hpp"

#include <gtest/gtest.h>

namespace testing {

TEST(CartesianTypeTests, CartesianTConstructors) {
  const double kVal2 = 5.0;
  const lob::CartesianT<double> kVelocity1;
  const lob::CartesianT<double> kVelocity2(kVal2, kVal2, kVal2);
  const lob::CartesianT<double> kVelocity3(kVal2);
  const lob::CartesianT<double> kVelocity4(kVelocity3);
  const lob::CartesianT<double> kVelocity5 = kVelocity4;
  EXPECT_DOUBLE_EQ(kVelocity1.X(), 0.0);
  EXPECT_DOUBLE_EQ(kVelocity1.Y(), 0.0);
  EXPECT_DOUBLE_EQ(kVelocity1.Z(), 0.0);
  EXPECT_DOUBLE_EQ(kVelocity2.X(), kVal2);
  EXPECT_DOUBLE_EQ(kVelocity2.Y(), kVal2);
  EXPECT_DOUBLE_EQ(kVelocity2.Z(), kVal2);
  EXPECT_DOUBLE_EQ(kVelocity3.X(), kVal2);
  EXPECT_DOUBLE_EQ(kVelocity3.Y(), kVal2);
  EXPECT_DOUBLE_EQ(kVelocity3.Z(), kVal2);
  EXPECT_DOUBLE_EQ(kVelocity4.X(), kVal2);
  EXPECT_DOUBLE_EQ(kVelocity4.Y(), kVal2);
  EXPECT_DOUBLE_EQ(kVelocity4.Z(), kVal2);
  EXPECT_DOUBLE_EQ(kVelocity5.X(), kVal2);
  EXPECT_DOUBLE_EQ(kVelocity5.Y(), kVal2);
  EXPECT_DOUBLE_EQ(kVelocity5.Z(), kVal2);
}

TEST(CartesianTypeTests, CartesianTArithmetic) {
  const double kVal1 = 1.5;
  const double kVal2 = 5.0;
  lob::CartesianT<double> velocity;
  velocity = velocity + kVal1;
  EXPECT_DOUBLE_EQ(velocity.X(), kVal1);
  EXPECT_DOUBLE_EQ(velocity.Y(), kVal1);
  EXPECT_DOUBLE_EQ(velocity.Z(), kVal1);
  velocity = velocity / kVal1;
  EXPECT_DOUBLE_EQ(velocity.X(), 1.0);
  EXPECT_DOUBLE_EQ(velocity.Y(), 1.0);
  EXPECT_DOUBLE_EQ(velocity.Z(), 1.0);
  velocity = velocity + kVal1;
  velocity = velocity * 2;
  EXPECT_DOUBLE_EQ(velocity.X(), kVal2);
  EXPECT_DOUBLE_EQ(velocity.Y(), kVal2);
  EXPECT_DOUBLE_EQ(velocity.Z(), kVal2);
  velocity = velocity - kVal1;
  EXPECT_DOUBLE_EQ(velocity.X(), kVal2 - kVal1);
  EXPECT_DOUBLE_EQ(velocity.Y(), kVal2 - kVal1);
  EXPECT_DOUBLE_EQ(velocity.Z(), kVal2 - kVal1);
  velocity = velocity + lob::CartesianT<double>(kVal1);
  EXPECT_DOUBLE_EQ(velocity.X(), kVal2);
  EXPECT_DOUBLE_EQ(velocity.Y(), kVal2);
  EXPECT_DOUBLE_EQ(velocity.Z(), kVal2);
  velocity = velocity / lob::CartesianT<double>(kVal2);
  EXPECT_DOUBLE_EQ(velocity.X(), 1.0);
  EXPECT_DOUBLE_EQ(velocity.Y(), 1.0);
  EXPECT_DOUBLE_EQ(velocity.Z(), 1.0);
  velocity = velocity * lob::CartesianT<double>(kVal2) *
             lob::CartesianT<double>(kVal2);
  EXPECT_DOUBLE_EQ(velocity.X(), kVal2 * kVal2);
  EXPECT_DOUBLE_EQ(velocity.Y(), kVal2 * kVal2);
  EXPECT_DOUBLE_EQ(velocity.Z(), kVal2 * kVal2);
  velocity.X(0.0);
  velocity.Y(kVal1);
  velocity.Z(kVal2);
  const lob::CartesianT<double> kTest(1.0, -kVal1, kVal2);
  velocity = velocity - kTest;
  EXPECT_DOUBLE_EQ(velocity.X(), -1.0);
  EXPECT_DOUBLE_EQ(velocity.Y(), 3.0);
  EXPECT_DOUBLE_EQ(velocity.Z(), 0.0);
}

TEST(CartesianTypeTests, CartesianTMagnitude) {
  const double kComponent1(0.0);
  const double kComponent2(3.0);
  const double kComponent3(4.0);
  const double kExpectedMagnitude(5.0);
  const lob::CartesianT<double> kTest1(kComponent1, kComponent2, kComponent3);
  const lob::CartesianT<double> kTest2(kComponent2, kComponent3, kComponent1);
  const lob::CartesianT<double> kTest3(kComponent3, kComponent1, kComponent2);
  EXPECT_DOUBLE_EQ(kTest1.Magnitude(), kExpectedMagnitude);
  EXPECT_DOUBLE_EQ(kTest2.Magnitude(), kExpectedMagnitude);
  EXPECT_DOUBLE_EQ(kTest3.Magnitude(), kExpectedMagnitude);
}

}  // namespace testing

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