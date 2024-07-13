// This file is a part of lob, an exterior ballistics calculation library
// Copyright (c) 2024  Joel Benway
// Please see end of file for extended copyright information

#include "lob/lob.hpp"

#include <gtest/gtest.h>

#include <cstddef>
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
               .BCAtmosphere(lob::AtmosphereReferenceT::kIcao)
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

TEST_F(LobTestFixture, Solve) {
  ASSERT_NE(puut, nullptr);
  constexpr size_t kSolutionLength = 10;
  const std::vector<lob::Lob::Solution> kExpected = {
      {100, 2774, 3073, 0.0, 0.0, 0.0, 0.0, 0.104},
      {200, 2560, 2616, -3.0, -1.4, 0.0, 0.0, 0.216},
      {300, 2355, 2214, -11.4, -3.6, 0.0, 0.0, 0.339},
      {400, 2159, 1862, -26.0, -6.2, 0.0, 0.0, 0.472},
      {500, 1974, 1555, -48.1, -9.2, 0.0, 0.0, 0.617},
      {600, 1798, 1291, -79.1, -12.6, 0.0, 0.0, 0.776},
      {700, 1634, 1066, -121.0, -16.5, 0.0, 0.0, 0.951},
      {800, 1483, 878, -175.9, -21.0, 0.0, 0.0, 1.144},
      {900, 1348, 725, -246.6, -26.2, 0.0, 0.0, 1.357},
      {1000, 1231, 606, -336.5, -32.1, 0.0, 0.0, 1.590}};
  lob::Lob::Solution solutions[kSolutionLength] = {0};
  size_t written =
      puut->Solve(static_cast<lob::Lob::Solution*>(solutions), kSolutionLength);
  EXPECT_EQ(written, kSolutionLength);
  for (size_t i = 0; i < kSolutionLength; i++) {
    EXPECT_NEAR(solutions[i].range, kExpected[i].range, 1);
    EXPECT_NEAR(solutions[i].velocity, kExpected[i].velocity, 1);
    EXPECT_NEAR(solutions[i].energy, kExpected[i].energy, 1);
    EXPECT_NEAR(solutions[i].time_of_flight, kExpected[i].time_of_flight, .001);
  }
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