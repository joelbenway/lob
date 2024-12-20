// This file is a part of lob, an exterior ballistics calculation library
// Copyright (c) 2024  Joel Benway
// Please see end of file for extended copyright information

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>

#include "lob/lob.hpp"

namespace tests {

struct LobCoriolisTestFixture : public testing::Test {
  // Unit under test
  std::unique_ptr<lob::Lob> puut;

  LobCoriolisTestFixture() : puut(nullptr) {}

  void SetUp() override {
    puut.reset();
    ASSERT_EQ(puut, nullptr);

    const double kTestBC = 0.200;
    const lob::DragFunctionT kDragFunction = lob::DragFunctionT::kG7;
    const double kTestDiameter = 0.308;
    const double kTestWeight = 147.0;
    const double kTestMuzzleVelocity = 2810.0;
    const double kTestZeroAngle = 3.66;
    const double kTestOpticHeight = 1.5;
    const double kTestTargetDistance = 2000.0;

    const uint16_t kStepSize = 100;

    puut = lob::Lob::Builder()
               .BallisticCoefficentPsi(kTestBC)
               .BCDragFunction(kDragFunction)
               .BCAtmosphere(lob::AtmosphereReferenceT::kIcao)
               .DiameterInch(kTestDiameter)
               .MassGrains(kTestWeight)
               .InitialVelocityFps(kTestMuzzleVelocity)
               .ZeroAngleMOA(kTestZeroAngle)
               .OpticHeightInches(kTestOpticHeight)
               .TargetDistanceYds(kTestTargetDistance)
               .SolverStepSizeUsec(kStepSize)
               .Build();

    ASSERT_NE(puut, nullptr);
  }

  void TearDown() override {
    puut.reset();

    ASSERT_EQ(puut, nullptr);
  }
};

TEST_F(LobCoriolisTestFixture, ZeroAngleSearch) {
  ASSERT_NE(puut, nullptr);
  const double kZeroRange = 100;
  auto puut2 = lob::Lob::Builder(*puut)
                   .ZeroAngleMOA(std::numeric_limits<double>::quiet_NaN())
                   .ZeroDistanceYds(kZeroRange)
                   .Build();
  ASSERT_NE(puut2, nullptr);
  const float kError = 0.1F;
  EXPECT_NEAR(puut->GetZeroAngleMOA(), puut2->GetZeroAngleMOA(), kError);
  puut2.reset();
}

TEST_F(LobCoriolisTestFixture, CompareCoriolisDeflectionNorth) {
  ASSERT_NE(puut, nullptr);
  const size_t kSolutionLength = 5;
  // NOLINTBEGIN c-style array, decay array into pointer
  const uint16_t kRanges[] = {0, 500, 1000, 1500, 2000};
  lob::Lob::Solution solutions1[kSolutionLength] = {0};
  size_t written = puut->Solve(solutions1, kRanges, kSolutionLength);
  // NOLINTEND
  EXPECT_EQ(written, kSolutionLength);

  const double kLatitude = 45.0;
  const double kAzimuth = 0;  // North
  const double kError = 0.25;

  auto puut2 = lob::Lob::Builder(*puut)
                   .LatitudeDeg(kLatitude)
                   .AzimuthDeg(kAzimuth)
                   .Build();

  ASSERT_NE(puut2, nullptr);
  // NOLINTBEGIN c-style array, decay array into pointer
  lob::Lob::Solution solutions2[kSolutionLength] = {0};
  written = puut2->Solve(solutions2, kRanges, kSolutionLength);
  EXPECT_EQ(written, kSolutionLength);
  const double kExpectedElevationDeflection[] = {0, 0, 0, 0, 0};
  const double kExpectedWindageDeflection[] = {0, 0.6, 2.8, 7.8, 16.4};
  // NOLINTEND
  for (size_t i = 0; i < kSolutionLength; i++) {
    const double kElevationDeflection =
        solutions2[i].elevation_distance - solutions1[i].elevation_distance;
    const double kWindageDeflection =
        solutions2[i].windage_distance - solutions1[i].windage_distance;
    EXPECT_NEAR(kElevationDeflection, kExpectedElevationDeflection[i], kError);
    EXPECT_NEAR(kWindageDeflection, kExpectedWindageDeflection[i], kError);
  }
}

TEST_F(LobCoriolisTestFixture, CompareCoriolisDeflectionEast) {
  ASSERT_NE(puut, nullptr);
  const size_t kSolutionLength = 5;
  // NOLINTBEGIN c-style array, decay array into pointer
  const uint16_t kRanges[] = {0, 500, 1000, 1500, 2000};
  lob::Lob::Solution solutions1[kSolutionLength] = {0};
  size_t written = puut->Solve(solutions1, kRanges, kSolutionLength);
  // NOLINTEND
  EXPECT_EQ(written, kSolutionLength);

  const double kLatitude = 45.0;
  const double kAzimuth = 90;  // East
  const double kError = 0.25;

  auto puut2 = lob::Lob::Builder(*puut)
                   .LatitudeDeg(kLatitude)
                   .AzimuthDeg(kAzimuth)
                   .Build();

  ASSERT_NE(puut2, nullptr);
  // NOLINTBEGIN c-style array, decay array into pointer
  lob::Lob::Solution solutions2[kSolutionLength] = {0};
  written = puut2->Solve(solutions2, kRanges, kSolutionLength);
  EXPECT_EQ(written, kSolutionLength);
  const double kExpectedElevationDeflection[] = {0, .6, 2.8, 7.6, 16};
  const double kExpectedWindageDeflection[] = {0, 0.6, 2.8, 7.6, 15.8};
  // NOLINTEND
  for (size_t i = 0; i < kSolutionLength; i++) {
    const double kElevationDeflection =
        solutions2[i].elevation_distance - solutions1[i].elevation_distance;
    const double kWindageDeflection =
        solutions2[i].windage_distance - solutions1[i].windage_distance;
    EXPECT_NEAR(kElevationDeflection, kExpectedElevationDeflection[i], kError);
    EXPECT_NEAR(kWindageDeflection, kExpectedWindageDeflection[i], kError);
  }
}

TEST_F(LobCoriolisTestFixture, CompareCoriolisDeflectionSouth) {
  ASSERT_NE(puut, nullptr);
  const size_t kSolutionLength = 5;
  // NOLINTBEGIN c-style array, decay array into pointer
  const uint16_t kRanges[] = {0, 500, 1000, 1500, 2000};
  lob::Lob::Solution solutions1[kSolutionLength] = {0};
  size_t written = puut->Solve(solutions1, kRanges, kSolutionLength);
  // NOLINTEND
  EXPECT_EQ(written, kSolutionLength);

  const double kLatitude = 45.0;
  const double kAzimuth = 180;  // South
  const double kError = 0.25;

  auto puut2 = lob::Lob::Builder(*puut)
                   .LatitudeDeg(kLatitude)
                   .AzimuthDeg(kAzimuth)
                   .Build();

  ASSERT_NE(puut2, nullptr);
  // NOLINTBEGIN c-style array, decay array into pointer
  lob::Lob::Solution solutions2[kSolutionLength] = {0};
  written = puut2->Solve(solutions2, kRanges, kSolutionLength);
  EXPECT_EQ(written, kSolutionLength);
  const double kExpectedElevationDeflection[] = {0, 0, 0, 0, 0};
  const double kExpectedWindageDeflection[] = {0, 0.6, 2.8, 7.5, 15.2};
  // NOLINTEND
  for (size_t i = 0; i < kSolutionLength; i++) {
    const double kElevationDeflection =
        solutions2[i].elevation_distance - solutions1[i].elevation_distance;
    const double kWindageDeflection =
        solutions2[i].windage_distance - solutions1[i].windage_distance;
    EXPECT_NEAR(kElevationDeflection, kExpectedElevationDeflection[i], kError);
    EXPECT_NEAR(kWindageDeflection, kExpectedWindageDeflection[i], kError);
  }
}

TEST_F(LobCoriolisTestFixture, CompareCoriolisDeflectionWest) {
  ASSERT_NE(puut, nullptr);
  const size_t kSolutionLength = 5;
  // NOLINTBEGIN c-style array, decay array into pointer
  const uint16_t kRanges[] = {0, 500, 1000, 1500, 2000};
  lob::Lob::Solution solutions1[kSolutionLength] = {0};
  size_t written = puut->Solve(solutions1, kRanges, kSolutionLength);
  // NOLINTEND
  EXPECT_EQ(written, kSolutionLength);

  const double kLatitude = 45.0;
  const double kAzimuth = 270;  // West
  const double kError = 0.25;

  auto puut2 = lob::Lob::Builder(*puut)
                   .LatitudeDeg(kLatitude)
                   .AzimuthDeg(kAzimuth)
                   .Build();

  ASSERT_NE(puut2, nullptr);
  // NOLINTBEGIN c-style array, decay array into pointer
  lob::Lob::Solution solutions2[kSolutionLength] = {0};
  written = puut2->Solve(solutions2, kRanges, kSolutionLength);
  EXPECT_EQ(written, kSolutionLength);
  const double kExpectedElevationDeflection[] = {0, -0.6, -2.8, -7.6, -15.8};
  const double kExpectedWindageDeflection[] = {0, 0.6, 2.8, 7.6, 15.8};
  // NOLINTEND
  for (size_t i = 0; i < kSolutionLength; i++) {
    const double kElevationDeflection =
        solutions2[i].elevation_distance - solutions1[i].elevation_distance;
    const double kWindageDeflection =
        solutions2[i].windage_distance - solutions1[i].windage_distance;
    EXPECT_NEAR(kElevationDeflection, kExpectedElevationDeflection[i], kError);
    EXPECT_NEAR(kWindageDeflection, kExpectedWindageDeflection[i], kError);
  }
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