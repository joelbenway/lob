// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include "helpers.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <limits>
#include <type_traits>

namespace tests {

TEST(HelpersTest, NaNdouble) {
  static_assert(std::is_same<double, decltype(lob::NaN())>::value,
                "NaN() not constexpr");

  EXPECT_TRUE(std::isnan(lob::NaN()));
}

TEST(HelpersTest, NaNfloat) { EXPECT_TRUE(std::isnan(lob::NaN<float>())); }

TEST(HelpersTest, AreEqual) {
  constexpr double kTwo = 2.0;
  static_assert(lob::AreEqual(kTwo, kTwo), "AreEqual not constexpr");
  const int kIntA = 7;
  const int kIntB = kIntA;
  const int kIntC = kIntA + 1;
  const float kFloatA = 7.0F;
  const float kFloatB = kFloatA;
  const float kFloatC = kFloatA + 1.0F;
  const double kDoubleA = 7.0;
  const double kDoubleB = kDoubleA;
  const double kDoubleC = kDoubleA + 1.0;
  const double kInfinity = std::numeric_limits<double>::infinity();
  const double kNaN = std::numeric_limits<double>::quiet_NaN();
  EXPECT_TRUE(lob::AreEqual(kIntA, kIntB));
  EXPECT_TRUE(lob::AreEqual(kFloatA, kFloatB));
  EXPECT_TRUE(lob::AreEqual(kDoubleA, kDoubleB));
  EXPECT_FALSE(lob::AreEqual(kIntA, kIntC));
  EXPECT_FALSE(lob::AreEqual(kFloatA, kFloatC));
  EXPECT_FALSE(lob::AreEqual(kDoubleA, kDoubleC));
  EXPECT_TRUE(lob::AreEqual(kInfinity, kInfinity));
  EXPECT_TRUE(lob::AreEqual(kNaN, kNaN));
}

TEST(HelpersTest, Modulo) {
  constexpr double kZero = 0.0;
  constexpr double kOne = 1.0;
  constexpr double kTwo = 2.0;
  static_assert(lob::AreEqual(lob::Modulo(kTwo, kOne), kZero),
                "Modulo not constexpr");
  const int kIntA = 100;
  const int kIntB = 3;
  const int kIntC = 1;
  const float kFloatA = 100.0F;
  const float kFloatB = 3.0F;
  const float kFloatC = 1.0F;
  const double kDoubleA = 100.0;
  const double kDoubleB = 3.0;
  const double kDoubleC = 1.0;
  EXPECT_EQ(lob::Modulo(kIntA, kIntB), kIntC);
  EXPECT_FLOAT_EQ(lob::Modulo(kFloatA, kFloatB), kFloatC);
  EXPECT_DOUBLE_EQ(lob::Modulo(kDoubleA, kDoubleB), kDoubleC);
  EXPECT_TRUE(std::isnan(lob::Modulo(kDoubleA, 0.0)));
}

TEST(HelpersTest, FloatEqualityNearZero) {
  constexpr double kZero = 0.0;
  static_assert(lob::AreFloatingPointsEqual(kZero, kZero),
                "AreFloatingPointsEqual not constexpr");

  const double kZeroClose = 1.0e-15;
  const double kZeroFar = 1.0e-13;
  EXPECT_TRUE(lob::AreFloatingPointsEqual(0.0, kZeroClose));
  EXPECT_TRUE(lob::AreFloatingPointsEqual(kZeroClose, 0.0));
  EXPECT_TRUE(lob::AreFloatingPointsEqual(-kZeroClose, 0.0));
  EXPECT_FALSE(lob::AreFloatingPointsEqual(0.0, kZeroFar));
  EXPECT_FALSE(lob::AreFloatingPointsEqual(kZeroFar, 0.0));
}

TEST(HelpersTest, LargeQuotientFmod) {
  const double kLargeA = 1.0e20;
  const double kLargeB = 3.0;
  EXPECT_DOUBLE_EQ(lob::Modulo(kLargeA, kLargeB), std::fmod(kLargeA, kLargeB));
}

TEST(HelpersTest, InfinityNanQuotientFmod) {
  const double kInfinity = std::numeric_limits<double>::infinity();
  const double kNaN = std::numeric_limits<double>::quiet_NaN();
  EXPECT_TRUE(std::isnan(lob::Modulo(kInfinity, kInfinity)));
  EXPECT_TRUE(std::isnan(lob::Modulo(kNaN, 1.0)));
}

TEST(HelpersTest, FmodSpecialCases) {
  const double kInfinity = std::numeric_limits<double>::infinity();
  const double kNaN = std::numeric_limits<double>::quiet_NaN();
  EXPECT_TRUE(std::isnan(lob::Modulo(kNaN, 5.0)));
  EXPECT_DOUBLE_EQ(lob::Modulo(3.5, kInfinity), 3.5);
  EXPECT_DOUBLE_EQ(lob::Modulo(0.0, 5.0), 0.0);
  EXPECT_DOUBLE_EQ(lob::Modulo(-10.0, 3.0), -1.0);
}

static_assert(lob::IsInf(std::numeric_limits<double>::infinity()),
              "IsInf not constexpr");

TEST(IsInfTest, PositiveInfinity) {
  const double kInfinity = std::numeric_limits<double>::infinity();
  EXPECT_TRUE(lob::IsInf(kInfinity));
  EXPECT_TRUE(lob::IsInf(-kInfinity));
}

TEST(IsInfTest, FiniteIsNotInf) {
  const double kPi = 3.14;
  EXPECT_FALSE(lob::IsInf(kPi));
  EXPECT_FALSE(lob::IsInf(0.0));
  EXPECT_FALSE(lob::IsInf(-1.0));
}

TEST(IsInfTest, NaNIsNotInf) {
  const double kNaN = std::numeric_limits<double>::quiet_NaN();
  EXPECT_FALSE(lob::IsInf(kNaN));
}

TEST(IsNanTest, NaN) {
  const double kNaN = std::numeric_limits<double>::quiet_NaN();
  EXPECT_TRUE(lob::IsNan(kNaN));
}

TEST(IsNanTest, InfinityIsNotNaN) {
  const double kInfinity = std::numeric_limits<double>::infinity();
  EXPECT_FALSE(lob::IsNan(kInfinity));
  EXPECT_FALSE(lob::IsNan(-kInfinity));
}

TEST(IsNanTest, FiniteIsNotNaN) {
  const double kPi = 3.14;
  EXPECT_FALSE(lob::IsNan(kPi));
  EXPECT_FALSE(lob::IsNan(0.0));
  EXPECT_FALSE(lob::IsNan(-1.0));
}

TEST(FabsTest, PositiveValue) {
  constexpr double kTwo = 2.0;
  static_assert(lob::AreEqual(lob::Fabs(-kTwo), kTwo), "Fabs not constexpr");

  const double kVal = 3.0;
  EXPECT_DOUBLE_EQ(lob::Fabs(kVal), kVal);
}

TEST(FabsTest, NegativeValue) {
  const double kVal = 3.0;
  EXPECT_DOUBLE_EQ(lob::Fabs(-kVal), kVal);
}

TEST(FabsTest, Zero) {
  const double kVal = 0.0;
  EXPECT_DOUBLE_EQ(lob::Fabs(kVal), kVal);
}

TEST(FabsTest, IntegerTypes) {
  const int kVal = 5;
  EXPECT_EQ(lob::Fabs(-kVal), kVal);
}

TEST(FabsTest, Infinity) {
  const double kInfinity = std::numeric_limits<double>::infinity();
  EXPECT_TRUE(std::isinf(lob::Fabs(kInfinity)));
  EXPECT_GT(lob::Fabs(kInfinity), 0);
}

TEST(FabsTest, NaN) {
  const double kNaN = std::numeric_limits<double>::quiet_NaN();
  EXPECT_TRUE(std::isnan(lob::Fabs(kNaN)));
}

TEST(FmaxTest, FirstLarger) {
  constexpr double kOne = 1.0;
  constexpr double kTwo = 2.0;
  static_assert(lob::AreEqual(lob::Fmax(kTwo, kOne), kTwo),
                "Fmax not constexpr");

  const double kFive = 5.0;
  const double kThree = 3.0;
  EXPECT_DOUBLE_EQ(lob::Fmax(kFive, kThree), kFive);
}

TEST(FmaxTest, SecondLarger) {
  const double kFive = 5.0;
  const double kThree = 3.0;
  EXPECT_DOUBLE_EQ(lob::Fmax(kThree, kFive), kFive);
}

TEST(FmaxTest, Equal) {
  const double kFour = 4.0;
  EXPECT_DOUBLE_EQ(lob::Fmax(kFour, kFour), kFour);
}

TEST(FmaxTest, NegativeValues) {
  const double kThree = 3.0;
  const double kFive = 5.0;
  EXPECT_DOUBLE_EQ(lob::Fmax(-kThree, -kFive), -kThree);
}

TEST(FmaxTest, IntegerTypes) {
  const int kTen = 10;
  const int kTwenty = 20;
  EXPECT_EQ(lob::Fmax(kTen, kTwenty), kTwenty);
}

TEST(FmaxTest, NaNFirst) {
  const double kFive = 5.0;
  const double kNaN = std::numeric_limits<double>::quiet_NaN();
  EXPECT_DOUBLE_EQ(lob::Fmax(kNaN, kFive), kFive);
}

TEST(FmaxTest, NaNSecond) {
  const double kFive = 5.0;
  const double kNaN = std::numeric_limits<double>::quiet_NaN();
  EXPECT_TRUE(std::isnan(lob::Fmax(kFive, kNaN)));
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
