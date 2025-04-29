// This file is a part of lob, an exterior ballistics calculation library
// Copyright (c) 2025  Joel Benway
// Please see end of file for extended copyright information

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <cstddef>
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

TEST_F(BuilderTestFixture, CopyConstructor) {
  const float kTestBC = 0.425F;
  const float kTestDiameter = 0.308F;
  const float kTestWeight = 180.0F;
  const uint16_t kTestMuzzleVelocity = 2700U;
  const float kTestZeroAngle = 3.38F;
  puut->BallisticCoefficientPsi(kTestBC)
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
  const float kTestBC = 0.425F;
  const float kTestDiameter = 0.308F;
  const float kTestWeight = 180.0F;
  const uint16_t kTestMuzzleVelocity = 2700U;
  const float kTestZeroAngle = 3.38F;
  puut->BallisticCoefficientPsi(kTestBC)
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
  const float kTestBC = 0.425F;
  const float kTestDiameter = 0.308F;
  const float kTestWeight = 180.0F;
  const uint16_t kTestMuzzleVelocity = 2700U;
  const float kTestZeroAngle = 3.38F;
  puut->BallisticCoefficientPsi(kTestBC)
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
  const float kTestBC = 0.425F;
  const float kTestDiameter = 0.308F;
  const float kTestWeight = 180.0F;
  const uint16_t kTestMuzzleVelocity = 2700U;
  const float kTestZeroAngle = 3.38F;
  puut->BallisticCoefficientPsi(kTestBC)
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
  const float kTestBC = 0.425F;
  const uint16_t kTestMuzzleVelocity = 2700U;
  const float kTestZeroAngle = 3.84F;
  const float kZeroDistance = 100.0F;
  const lob::Input kResult = puut->BallisticCoefficientPsi(kTestBC)
                                 .InitialVelocityFps(kTestMuzzleVelocity)
                                 .ZeroDistanceYds(kZeroDistance)
                                 .Build();
  EXPECT_FALSE(std::isnan(kResult.table_coefficient));
  EXPECT_FALSE(std::isnan(kResult.speed_of_sound));
  EXPECT_EQ(kResult.velocity, kTestMuzzleVelocity);
  EXPECT_NEAR(kResult.zero_angle, kTestZeroAngle, 0.01);
  EXPECT_FLOAT_EQ(kResult.gravity.y,
                  -1.0F * static_cast<float>(lob::kStandardGravityFtPerSecSq));
}

TEST_F(BuilderTestFixture, BuildInvalidVelocityInput) {
  const float kTestBC = 0.425F;
  const float kTestZeroAngle = 3.84F;
  const lob::Input kResult = puut->BallisticCoefficientPsi(kTestBC)
                                 .ZeroAngleMOA(kTestZeroAngle)
                                 .Build();
  EXPECT_TRUE(std::isnan(kResult.table_coefficient));
}

TEST_F(BuilderTestFixture, BuildInvalidBCInput) {
  const uint16_t kTestMuzzleVelocity = 2700U;
  const float kTestZeroAngle = 3.84F;
  const lob::Input kResult = puut->InitialVelocityFps(kTestMuzzleVelocity)
                                 .ZeroAngleMOA(kTestZeroAngle)
                                 .Build();
  EXPECT_TRUE(std::isnan(kResult.table_coefficient));
}

TEST_F(BuilderTestFixture, BuildInvalidZeroInput) {
  const float kTestBC = 0.425F;
  const uint16_t kTestMuzzleVelocity = 2700U;
  const lob::Input kResult = puut->BallisticCoefficientPsi(kTestBC)
                                 .InitialVelocityFps(kTestMuzzleVelocity)
                                 .Build();
  EXPECT_TRUE(std::isnan(kResult.table_coefficient));
}

TEST_F(BuilderTestFixture, BuildG1UsingCustomTable) {
  const float kTestBC = 1.0F;
  const uint16_t kTestMuzzleVelocity = 2500U;
  const float kTestZeroAngle = 5.59F;
  std::array<float, lob::kTableSize> machs = {};
  std::array<float, lob::kTableSize> drags = {};

  const lob::Input kResult1 =
      puut->BallisticCoefficientPsi(kTestBC)
          .BCDragFunction(lob::DragFunctionT::kG1)
          .BCAtmosphere(lob::AtmosphereReferenceT::kIcao)
          .InitialVelocityFps(kTestMuzzleVelocity)
          .ZeroAngleMOA(kTestZeroAngle)
          .Build();

  for (size_t i = 0; i < lob::kTableSize; i++) {
    machs.at(i) = static_cast<float>(lob::kMachs.at(i)) / lob::kTableScale;
    drags.at(i) = static_cast<float>(lob::kG1Drags.at(i)) / lob::kTableScale;
  }

  const lob::Input kResult2 = puut->BallisticCoefficientPsi(lob::NaN())
                                  .MachVsDragTable(machs, drags)
                                  .Build();

  for (size_t i = 0; i < lob::kTableSize; i++) {
    EXPECT_EQ(kResult1.drags.at(i), kResult2.drags.at(i));
  }
}

TEST_F(BuilderTestFixture, BuildG2UsingCustomTable) {
  const float kTestBC = 1.0F;
  const uint16_t kTestMuzzleVelocity = 2500U;
  const float kTestZeroAngle = 5.0F;
  std::array<float, lob::kTableSize> machs = {};
  std::array<float, lob::kTableSize> drags = {};

  const lob::Input kResult1 =
      puut->BallisticCoefficientPsi(kTestBC)
          .BCDragFunction(lob::DragFunctionT::kG2)
          .BCAtmosphere(lob::AtmosphereReferenceT::kIcao)
          .InitialVelocityFps(kTestMuzzleVelocity)
          .ZeroAngleMOA(kTestZeroAngle)
          .Build();

  for (size_t i = 0; i < lob::kTableSize; i++) {
    machs.at(i) = static_cast<float>(lob::kMachs.at(i)) / lob::kTableScale;
    drags.at(i) = static_cast<float>(lob::kG2Drags.at(i)) / lob::kTableScale;
  }

  const lob::Input kResult2 = puut->BallisticCoefficientPsi(lob::NaN())
                                  .MachVsDragTable(machs, drags)
                                  .Build();

  for (size_t i = 0; i < lob::kTableSize; i++) {
    EXPECT_EQ(kResult1.drags.at(i), kResult2.drags.at(i));
  }
}

TEST_F(BuilderTestFixture, BuildG5UsingCustomTable) {
  const float kTestBC = 1.0F;
  const uint16_t kTestMuzzleVelocity = 2500U;
  const float kTestZeroAngle = 5.0F;
  std::array<float, lob::kTableSize> machs = {};
  std::array<float, lob::kTableSize> drags = {};

  const lob::Input kResult1 =
      puut->BallisticCoefficientPsi(kTestBC)
          .BCDragFunction(lob::DragFunctionT::kG5)
          .BCAtmosphere(lob::AtmosphereReferenceT::kIcao)
          .InitialVelocityFps(kTestMuzzleVelocity)
          .ZeroAngleMOA(kTestZeroAngle)
          .Build();

  for (size_t i = 0; i < lob::kTableSize; i++) {
    machs.at(i) = static_cast<float>(lob::kMachs.at(i)) / lob::kTableScale;
    drags.at(i) = static_cast<float>(lob::kG5Drags.at(i)) / lob::kTableScale;
  }

  const lob::Input kResult2 = puut->BallisticCoefficientPsi(lob::NaN())
                                  .MachVsDragTable(machs, drags)
                                  .Build();

  for (size_t i = 0; i < lob::kTableSize; i++) {
    EXPECT_EQ(kResult1.drags.at(i), kResult2.drags.at(i));
  }
}

TEST_F(BuilderTestFixture, BuildG6UsingCustomTable) {
  const float kTestBC = 1.0F;
  const uint16_t kTestMuzzleVelocity = 2500U;
  const float kTestZeroAngle = 5.0F;
  std::array<float, lob::kTableSize> machs = {};
  std::array<float, lob::kTableSize> drags = {};

  const lob::Input kResult1 =
      puut->BallisticCoefficientPsi(kTestBC)
          .BCDragFunction(lob::DragFunctionT::kG6)
          .BCAtmosphere(lob::AtmosphereReferenceT::kIcao)
          .InitialVelocityFps(kTestMuzzleVelocity)
          .ZeroAngleMOA(kTestZeroAngle)
          .Build();

  for (size_t i = 0; i < lob::kTableSize; i++) {
    machs.at(i) = static_cast<float>(lob::kMachs.at(i)) / lob::kTableScale;
    drags.at(i) = static_cast<float>(lob::kG6Drags.at(i)) / lob::kTableScale;
  }

  const lob::Input kResult2 = puut->BallisticCoefficientPsi(lob::NaN())
                                  .MachVsDragTable(machs, drags)
                                  .Build();

  for (size_t i = 0; i < lob::kTableSize; i++) {
    EXPECT_EQ(kResult1.drags.at(i), kResult2.drags.at(i));
  }
}

TEST_F(BuilderTestFixture, BuildG7UsingCustomTable) {
  const float kTestBC = 1.0F;
  const uint16_t kTestMuzzleVelocity = 2500U;
  const float kTestZeroAngle = 5.0F;
  std::array<float, lob::kTableSize> machs = {};
  std::array<float, lob::kTableSize> drags = {};

  const lob::Input kResult1 =
      puut->BallisticCoefficientPsi(kTestBC)
          .BCDragFunction(lob::DragFunctionT::kG7)
          .BCAtmosphere(lob::AtmosphereReferenceT::kIcao)
          .InitialVelocityFps(kTestMuzzleVelocity)
          .ZeroAngleMOA(kTestZeroAngle)
          .Build();

  for (size_t i = 0; i < lob::kTableSize; i++) {
    machs.at(i) = static_cast<float>(lob::kMachs.at(i)) / lob::kTableScale;
    drags.at(i) = static_cast<float>(lob::kG7Drags.at(i)) / lob::kTableScale;
  }

  const lob::Input kResult2 = puut->BallisticCoefficientPsi(lob::NaN())
                                  .MachVsDragTable(machs, drags)
                                  .Build();

  for (size_t i = 0; i < lob::kTableSize; i++) {
    EXPECT_EQ(kResult1.drags.at(i), kResult2.drags.at(i));
  }
}

TEST_F(BuilderTestFixture, BuildG8UsingCustomTable) {
  const float kTestBC = 1.0F;
  const uint16_t kTestMuzzleVelocity = 2500U;
  const float kTestZeroAngle = 5.0F;
  std::array<float, lob::kTableSize> machs = {};
  std::array<float, lob::kTableSize> drags = {};

  const lob::Input kResult1 =
      puut->BallisticCoefficientPsi(kTestBC)
          .BCDragFunction(lob::DragFunctionT::kG8)
          .BCAtmosphere(lob::AtmosphereReferenceT::kIcao)
          .InitialVelocityFps(kTestMuzzleVelocity)
          .ZeroAngleMOA(kTestZeroAngle)
          .Build();

  for (size_t i = 0; i < lob::kTableSize; i++) {
    machs.at(i) = static_cast<float>(lob::kMachs.at(i)) / lob::kTableScale;
    drags.at(i) = static_cast<float>(lob::kG8Drags.at(i)) / lob::kTableScale;
  }

  const lob::Input kResult2 = puut->BallisticCoefficientPsi(lob::NaN())
                                  .MachVsDragTable(machs, drags)
                                  .Build();

  for (size_t i = 0; i < lob::kTableSize; i++) {
    EXPECT_EQ(kResult1.drags.at(i), kResult2.drags.at(i));
  }
}

TEST_F(BuilderTestFixture, JackOConnorZero) {
  const float kSierraGameKingBC = 0.436F;
  const uint16_t kM70MuzzleVelocity = 3100U;
  const float kZeroYardage = 100.0F;
  const float kZeroHeight = 3.0F;
  const float kExpectedZeroAngle = 6.11F;
  const float kError = 0.01F;
  const lob::Input kJack =
      puut->BallisticCoefficientPsi(kSierraGameKingBC)
          .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
          .InitialVelocityFps(kM70MuzzleVelocity)
          .ZeroDistanceYds(kZeroYardage)
          .ZeroImpactHeightInches(kZeroHeight)
          .Build();
  EXPECT_NEAR(kJack.zero_angle, kExpectedZeroAngle, kError);
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