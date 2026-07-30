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
  constexpr auto kA = lob::NaN();
  EXPECT_TRUE((std::is_same<const double, decltype(kA)>::value));
  EXPECT_TRUE(std::isnan(kA));
}

TEST(HelpersTest, NaNfloat) {
  constexpr auto kA = lob::NaN<float>();
  EXPECT_TRUE((std::is_same<const float, decltype(kA)>::value));
  EXPECT_TRUE(std::isnan(kA));
}

TEST(HelpersTest, AreEqual) {
  constexpr int kIntA = 7;
  constexpr int kIntB = kIntA;
  constexpr int kIntC = kIntA + 1;
  constexpr float kFloatA(kIntA);
  constexpr float kFloatB(kIntB);
  constexpr float kFloatC(kIntC);
  constexpr double kDoubleA(kIntA);
  constexpr double kDoubleB(kIntB);
  constexpr double kDoubleC(kIntC);
  constexpr double kInfinity(std::numeric_limits<double>::infinity());
  constexpr double kNaN(std::numeric_limits<double>::quiet_NaN());
  EXPECT_TRUE((lob::AreEqual(kIntA, kIntB)));
  EXPECT_TRUE((lob::AreEqual(kFloatA, kFloatB)));
  EXPECT_TRUE((lob::AreEqual(kDoubleA, kDoubleB)));
  EXPECT_FALSE((lob::AreEqual(kIntA, kIntC)));
  EXPECT_FALSE((lob::AreEqual(kFloatA, kFloatC)));
  EXPECT_FALSE((lob::AreEqual(kDoubleA, kDoubleC)));
  EXPECT_TRUE((lob::AreEqual(kInfinity, kInfinity)));
  EXPECT_TRUE((lob::AreEqual(kNaN, kNaN)));
}

TEST(HelpersTest, Modulo) {
  constexpr int kIntA = 100;
  constexpr int kIntB = 3;
  constexpr int kIntC = 1;
  constexpr float kFloatA(kIntA);
  constexpr float kFloatB(kIntB);
  constexpr float kFloatC(kIntC);
  constexpr double kDoubleA(kIntA);
  constexpr double kDoubleB(kIntB);
  constexpr double kDoubleC(kIntC);
  EXPECT_EQ((lob::Modulo(kIntA, kIntB)), kIntC);
  EXPECT_FLOAT_EQ((lob::Modulo(kFloatA, kFloatB)), kFloatC);
  EXPECT_DOUBLE_EQ((lob::Modulo(kDoubleA, kDoubleB)), kDoubleC);
  EXPECT_TRUE(std::isnan(lob::Modulo(kDoubleA, 0.0)));
}

TEST(HelpersTest, FloatEqualityNearZero) {
  const auto kZeroClose = lob::AreFloatingPointsEqual(0.0, 1e-15);
  const auto kSymClose = lob::AreFloatingPointsEqual(1e-15, 0.0);
  const auto kNegClose = lob::AreFloatingPointsEqual(-1e-15, 0.0);
  const auto kZeroFar = lob::AreFloatingPointsEqual(0.0, 1e-13);
  const auto kSymFar = lob::AreFloatingPointsEqual(1e-13, 0.0);
  EXPECT_TRUE(kZeroClose);
  EXPECT_TRUE(kSymClose);
  EXPECT_TRUE(kNegClose);
  EXPECT_FALSE(kZeroFar);
  EXPECT_FALSE(kSymFar);
}

TEST(HelpersTest, LargeQuotientFmod) {
  constexpr double kLargeA = 1.0e20;
  constexpr double kLargeB = 3.0;
  const auto kActual = lob::Modulo(kLargeA, kLargeB);
  const double kExpected = lob::Fmod(kLargeA, kLargeB);
  EXPECT_DOUBLE_EQ(kActual, kExpected);
}

TEST(HelpersTest, SmallQuotientConstexpr) {
  const auto kActual = lob::Modulo(100.0, 3.0);
  EXPECT_DOUBLE_EQ(kActual, 1.0);
}

TEST(HelpersTest, InfinityNanQuotientFmod) {
  constexpr auto kInfinity = std::numeric_limits<double>::infinity();
  constexpr auto kNaN = std::numeric_limits<double>::quiet_NaN();
  const auto kInfMod = lob::Modulo(kInfinity, kInfinity);
  const auto kNanMod = lob::Modulo(kNaN, 1.0);
  EXPECT_TRUE(std::isnan(kInfMod));
  EXPECT_TRUE(std::isnan(kNanMod));
}

TEST(SqrtTest, PerfectSquares) {
  const double kSqrt4 = lob::Sqrt(4.0);
  const double kSqrt9 = lob::Sqrt(9.0);
  const double kSqrt16 = lob::Sqrt(16.0);
  const double kSqrt25 = lob::Sqrt(25.0);
  const double kSqrt100 = lob::Sqrt(100.0);
  EXPECT_DOUBLE_EQ(kSqrt4, 2.0);
  EXPECT_DOUBLE_EQ(kSqrt9, 3.0);
  EXPECT_DOUBLE_EQ(kSqrt16, 4.0);
  EXPECT_DOUBLE_EQ(kSqrt25, 5.0);
  EXPECT_DOUBLE_EQ(kSqrt100, 10.0);
}

TEST(SqrtTest, SqrtTwo) {
  const auto kSqrt2 = lob::Sqrt(2.0);
  EXPECT_NEAR(kSqrt2, std::sqrt(2.0), 1e-14);
}

TEST(SqrtTest, SqrtOfZero) {
  const auto kVal = lob::Sqrt(0.0);
  EXPECT_DOUBLE_EQ(kVal, 0.0);
}

TEST(SqrtTest, SqrtOfNegativeIsNaN) {
  const auto kVal = lob::Sqrt(-1.0);
  EXPECT_TRUE(std::isnan(kVal));
}

TEST(SqrtTest, SqrtOfInfinity) {
  constexpr auto kInfinity = std::numeric_limits<double>::infinity();
  const auto kVal = lob::Sqrt(kInfinity);
  EXPECT_DOUBLE_EQ(kVal, kInfinity);
}

TEST(SqrtTest, ConstexprSqrt) {
  const auto kVal = lob::Sqrt(2.0);
  EXPECT_GT(kVal, 1.4);
  EXPECT_LT(kVal, 1.5);
}

TEST(FabsTest, PositiveValue) {
  constexpr auto kVal = lob::Fabs(3.0);
  EXPECT_DOUBLE_EQ(kVal, 3.0);
}

TEST(FabsTest, NegativeValue) {
  constexpr auto kVal = lob::Fabs(-3.0);
  EXPECT_DOUBLE_EQ(kVal, 3.0);
}

TEST(FabsTest, Zero) {
  constexpr auto kVal = lob::Fabs(0.0);
  EXPECT_DOUBLE_EQ(kVal, 0.0);
}

TEST(FabsTest, IntegerTypes) {
  constexpr auto kVal = lob::Fabs(-5);
  EXPECT_EQ(kVal, 5);
}

TEST(FabsTest, Infinity) {
  constexpr auto kInfinity = std::numeric_limits<double>::infinity();
  constexpr auto kVal = lob::Fabs(kInfinity);
  EXPECT_TRUE(std::isinf(kVal));
  EXPECT_GT(kVal, 0);
}

TEST(FabsTest, NaN) {
  constexpr auto kVal = lob::Fabs(std::numeric_limits<double>::quiet_NaN());
  EXPECT_TRUE(std::isnan(kVal));
}

TEST(FmaxTest, FirstLarger) {
  constexpr auto kVal = lob::Fmax(5.0, 3.0);
  EXPECT_DOUBLE_EQ(kVal, 5.0);
}

TEST(FmaxTest, SecondLarger) {
  constexpr auto kVal = lob::Fmax(3.0, 5.0);
  EXPECT_DOUBLE_EQ(kVal, 5.0);
}

TEST(FmaxTest, Equal) {
  constexpr auto kVal = lob::Fmax(4.0, 4.0);
  EXPECT_DOUBLE_EQ(kVal, 4.0);
}

TEST(FmaxTest, NegativeValues) {
  constexpr auto kVal = lob::Fmax(-3.0, -5.0);
  EXPECT_DOUBLE_EQ(kVal, -3.0);
}

TEST(FmaxTest, IntegerTypes) {
  constexpr auto kVal = lob::Fmax(10, 20);
  EXPECT_EQ(kVal, 20);
}

TEST(FmaxTest, NaNFirst) {
  constexpr auto kNaN = std::numeric_limits<double>::quiet_NaN();
  constexpr auto kVal = lob::Fmax(kNaN, 5.0);
  EXPECT_DOUBLE_EQ(kVal, 5.0);
}

TEST(FmaxTest, NaNSecond) {
  constexpr auto kNaN = std::numeric_limits<double>::quiet_NaN();
  constexpr auto kVal = lob::Fmax(5.0, kNaN);
  EXPECT_TRUE(std::isnan(kVal));
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