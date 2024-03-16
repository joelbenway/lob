// This file is a part of lob, an exterior ballistics calculation library
// Copyright (c) 2024  Joel Benway
// Please see end of file for extended copyright information

#include "lob/lob.hpp"

#include <gtest/gtest.h>

#include <memory>

namespace testing {

struct LobTestFixture : public Test {
  friend class lob::Lob;

  // Unit under test
  std::unique_ptr<lob::Lob> puut;

  LobTestFixture() : puut(nullptr) {}

  void SetUp() override {
    if (puut != nullptr) {
      puut = nullptr;
    }

    const double kTestBC = 0.425;
    const double kTestDiameter = 0.308;
    const double kTestWeight = 180.0;
    const double kTestMuzzleVelocity = 3000.0;
    const double kTestZero = 100.0;
    const double kTestOpticHeight = 1.5;
    const double kTestTargetDistance = 1000.0;

    puut = lob::Lob::Builder()
               .BallisticCoefficentPsi(kTestBC)
               .DiameterInch(kTestDiameter)
               .MassGrains(kTestWeight)
               .InitialVelocityFps(kTestMuzzleVelocity)
               .ZeroDistanceYds(kTestZero)
               .OpticHeightInches(kTestOpticHeight)
               .TargetDistanceYds(kTestTargetDistance)
               .Build();
  }

  void TearDown() override {
    if (puut == nullptr) {
      return;
    }

    puut = nullptr;
  }
};

TEST_F(LobTestFixture, ZeroAngleSearch) {
  ASSERT_NE(puut, nullptr);
  const float kExpectedZA = 3.38F;
  const float kZA = puut->GetZeroAngleMOA();
  const float kError = 0.1F;
  EXPECT_NEAR(kZA, kExpectedZA, kError);
}

TEST(LobTests, PassingTest) { EXPECT_TRUE(true); }

TEST(LobTests, AnotherPassingTest) { EXPECT_EQ(6 * 7, 42); }

TEST(LobTests, OneLastPassingTest) { EXPECT_GT(43, 42); }

int Main() {
  InitGoogleTest();
  return RUN_ALL_TESTS();
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