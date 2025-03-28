// This file is a part of lob, an exterior ballistics calculation library
// Copyright (c) 2025  Joel Benway
// Please see end of file for extended copyright information

#include "helpers.hpp"

#include <gtest/gtest.h>

#include <limits>

namespace tests {

TEST(HelpersTest, AreEqual) {
  const int kIntA = 7;
  const int kIntB = kIntA;
  const int kIntC = kIntA + 1;
  const float kFloatA(kIntA);
  const float kFloatB(kIntB);
  const float kFloatC(kIntC);
  const double kDoubleA(kIntA);
  const double kDoubleB(kIntB);
  const double kDoubleC(kIntC);
  const double kInfinity(std::numeric_limits<double>::infinity());
  const double kNan(std::numeric_limits<double>::quiet_NaN());
  EXPECT_TRUE(lob::AreEqual(kIntA, kIntB));
  EXPECT_TRUE(lob::AreEqual(kFloatA, kFloatB));
  EXPECT_TRUE(lob::AreEqual(kDoubleA, kDoubleB));
  EXPECT_FALSE(lob::AreEqual(kIntA, kIntC));
  EXPECT_FALSE(lob::AreEqual(kFloatA, kFloatC));
  EXPECT_FALSE(lob::AreEqual(kDoubleA, kDoubleC));
  EXPECT_TRUE(lob::AreEqual(kInfinity, kInfinity));
  EXPECT_TRUE(lob::AreEqual(kNan, kNan));
}

TEST(HelpersTest, Modulo) {
  const int kIntA = 100;
  const int kIntB = 3;
  const int kIntC = 1;
  const float kFloatA(kIntA);
  const float kFloatB(kIntB);
  const float kFloatC(kIntC);
  const double kDoubleA(kIntA);
  const double kDoubleB(kIntB);
  const double kDoubleC(kIntC);
  EXPECT_EQ(lob::Modulo(kIntA, kIntB), kIntC);
  EXPECT_FLOAT_EQ(lob::Modulo(kFloatA, kFloatB), kFloatC);
  EXPECT_DOUBLE_EQ(lob::Modulo(kDoubleA, kDoubleB), kDoubleC);
  EXPECT_TRUE(std::isnan(lob::Modulo(kDoubleA, 0.0)));
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