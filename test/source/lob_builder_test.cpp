// This file is a part of lob, an exterior ballistics calculation library
// Copyright (c) 2025  Joel Benway
// Please see end of file for extended copyright information

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <memory>
#include <utility>

#include "constants.hpp"
#include "lob/lob.hpp"
#include "tables.hpp"

namespace tests {

struct BuilderTestFixture : public testing::Test {
  // Unit under test
  std::unique_ptr<lob::Builder> puut;

  BuilderTestFixture() = default;

  void SetUp() override {
    ASSERT_EQ(puut, nullptr);

    puut = std::make_unique<lob::Builder>();

    ASSERT_NE(puut, nullptr);
  }

  void TearDown() override {
    puut.reset();
    puut = nullptr;
    ASSERT_EQ(puut, nullptr);
  }
};

TEST_F(BuilderTestFixture, Constructor) { ASSERT_NE(puut, nullptr); }

TEST_F(BuilderTestFixture, Destructor) {
  // We can ensure no memory leak by testing a sequence of constructions and
  // destructions
  const int kTestIterations = 1'000;
  for (int i = 0; i < kTestIterations; i++) {
    auto* b = new lob::Builder();
    delete b;  // If there's no crash or memory leak, then the destructor works
               // fine
  }
}

TEST_F(BuilderTestFixture, CopyConstructor) {
  const float kTestBC = 0.425;
  const float kTestDiameter = 0.308;
  const float kTestWeight = 180.0;
  const uint16_t kTestMuzzleVelocity = 2700U;
  const float kTestZeroAngle = 3.38;
  puut->BallisticCoefficentPsi(kTestBC)
      .DiameterInch(kTestDiameter)
      .MassGrains(kTestWeight)
      .InitialVelocityFps(kTestMuzzleVelocity)
      .ZeroAngleMOA(kTestZeroAngle);
  lob::Builder copy = *puut;
  const lob::Input kVal1 = puut->Build();
  EXPECT_FLOAT_EQ(kVal1.velocity, kTestMuzzleVelocity);
  const lob::Input kVal2 = copy.Build();
  EXPECT_FLOAT_EQ(kVal2.velocity, kTestMuzzleVelocity);
}

TEST_F(BuilderTestFixture, MoveConstructor) {
  const float kTestBC = 0.425;
  const float kTestDiameter = 0.308;
  const float kTestWeight = 180.0;
  const uint16_t kTestMuzzleVelocity = 2700U;
  const float kTestZeroAngle = 3.38;
  puut->BallisticCoefficentPsi(kTestBC)
      .DiameterInch(kTestDiameter)
      .MassGrains(kTestWeight)
      .InitialVelocityFps(kTestMuzzleVelocity)
      .ZeroAngleMOA(kTestZeroAngle);
  lob::Builder moved = std::move(*puut);
  const lob::Input kVal = moved.Build();
  EXPECT_EQ(kVal.velocity, kTestMuzzleVelocity);
}

// Test for copy assignment operator
TEST_F(BuilderTestFixture, CopyAssignmentOperator) {
  const float kTestBC = 0.425;
  const float kTestDiameter = 0.308;
  const float kTestWeight = 180.0;
  const uint16_t kTestMuzzleVelocity = 2700U;
  const float kTestZeroAngle = 3.38;
  puut->BallisticCoefficentPsi(kTestBC)
      .DiameterInch(kTestDiameter)
      .MassGrains(kTestWeight)
      .InitialVelocityFps(kTestMuzzleVelocity)
      .ZeroAngleMOA(kTestZeroAngle);
  lob::Builder copy;
  copy = *puut;
  const lob::Input kVal1 = puut->Build();
  EXPECT_FLOAT_EQ(kVal1.velocity, kTestMuzzleVelocity);
  const lob::Input kVal2 = copy.Build();
  EXPECT_FLOAT_EQ(kVal2.velocity, kTestMuzzleVelocity);
}

// Test for move assignment operator
TEST_F(BuilderTestFixture, MoveAssignmentOperator) {
  const float kTestBC = 0.425;
  const float kTestDiameter = 0.308;
  const float kTestWeight = 180.0;
  const uint16_t kTestMuzzleVelocity = 2700U;
  const float kTestZeroAngle = 3.38;
  puut->BallisticCoefficentPsi(kTestBC)
      .DiameterInch(kTestDiameter)
      .MassGrains(kTestWeight)
      .InitialVelocityFps(kTestMuzzleVelocity)
      .ZeroAngleMOA(kTestZeroAngle);
  lob::Builder moved;
  moved = std::move(*puut);
  const lob::Input kVal = moved.Build();
  EXPECT_EQ(kVal.velocity, kTestMuzzleVelocity);
}

TEST_F(BuilderTestFixture, BuildMinimalInput) {
  const float kTestBC = 0.425;
  const float kTestDiameter = 0.308;
  const float kTestWeight = 180.0;
  const uint16_t kTestMuzzleVelocity = 2700U;
  const float kTestZeroAngle = 3.84;
  const float kZeroDistance = 100;
  const lob::Input kResult = puut->BallisticCoefficentPsi(kTestBC)
                                 .BCAtmosphere(lob::AtmosphereReferenceT::kIcao)
                                 .DiameterInch(kTestDiameter)
                                 .MassGrains(kTestWeight)
                                 .InitialVelocityFps(kTestMuzzleVelocity)
                                 // .ZeroAngleMOA(kTestZeroAngle)
                                 .ZeroDistanceYds(kZeroDistance)
                                 .Build();
  EXPECT_EQ(kResult.drags.front(), lob::kG1Drags.front());
  EXPECT_EQ(kResult.drags.back(), lob::kG1Drags.back());
  EXPECT_NE(kResult.speed_of_sound, lob::kNaN);
  EXPECT_EQ(kResult.velocity, kTestMuzzleVelocity);
  EXPECT_NEAR(kResult.zero_angle, kTestZeroAngle, 0.01);
  EXPECT_FLOAT_EQ(kResult.gravity.y, -1.0F * lob::kStandardGravity);

  const uint32_t kOutSize = 12;
  const uint32_t kMaxRange = 6000;
  const std::array<uint32_t, kOutSize> kRanges = {
      0, 150, 300, 600, 900, 1200, 1500, 1800, 2100, 2400, 2700, 3000};
  std::array<lob::Output, kOutSize> out = {};
  lob::Solve(kResult, &kRanges, &out);
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