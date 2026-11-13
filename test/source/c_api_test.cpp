// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>

#include "lob/lob.h"

namespace tests {

namespace {

void ExpectWindTailZeroed(const LobContext& ctx) {
  const LobWindNode* nodes = &ctx.wind_nodes[0];
  for (size_t i = ctx.wind_count; i < LOB_WIND_POINTS; ++i) {
    EXPECT_EQ(nodes[i].range_ft, 0U);
    EXPECT_DOUBLE_EQ(nodes[i].x_fps, 0.0);
    EXPECT_DOUBLE_EQ(nodes[i].y_fps, 0.0);
    EXPECT_DOUBLE_EQ(nodes[i].z_fps, 0.0);
  }
}

}  // namespace

TEST(LobCAPITest, BuilderNullptrReturnsNullptr) {
  const uint8_t kDummy = 4U;
  EXPECT_EQ(LobBuilderBallisticCoefficientPsi(nullptr, kDummy), nullptr);
  EXPECT_EQ(LobBuilderBCAtmosphere(nullptr, kDummy), nullptr);
  EXPECT_EQ(LobBuilderBCDragFunction(nullptr, kDummy), nullptr);
  EXPECT_EQ(LobBuilderDiameterInch(nullptr, kDummy), nullptr);
  EXPECT_EQ(LobBuilderMeplatDiameterInch(nullptr, kDummy), nullptr);
  EXPECT_EQ(LobBuilderBaseDiameterInch(nullptr, kDummy), nullptr);
  EXPECT_EQ(LobBuilderLengthInch(nullptr, kDummy), nullptr);
  EXPECT_EQ(LobBuilderNoseLengthInch(nullptr, kDummy), nullptr);
  EXPECT_EQ(LobBuilderTailLengthInch(nullptr, kDummy), nullptr);
  EXPECT_EQ(LobBuilderOgiveRtR(nullptr, kDummy), nullptr);

  const size_t kSize = 3;
  const std::array<float, kSize> kMachs = {kDummy, kDummy, kDummy};
  const std::array<float, kSize> kDrags = {kDummy, kDummy, kDummy};

  EXPECT_EQ(
      LobBuilderSplineFitTable(nullptr, kMachs.data(), kDrags.data(), kSize),
      nullptr);
  LobBuilder builder;
  EXPECT_EQ(LobBuilderSplineFitTable(&builder, nullptr, kDrags.data(), kSize),
            &builder);
  EXPECT_EQ(LobBuilderSplineFitTable(&builder, kMachs.data(), nullptr, kSize),
            &builder);

  const std::array<float, kSize> kFps = {kDummy, kDummy, kDummy};

  EXPECT_EQ(
      LobBuilderBCVelocityBands(nullptr, kFps.data(), kDrags.data(), kSize),
      nullptr);
  EXPECT_EQ(LobBuilderBCVelocityBands(&builder, nullptr, kDrags.data(), kSize),
            &builder);
  EXPECT_EQ(LobBuilderBCVelocityBands(&builder, kFps.data(), nullptr, kSize),
            &builder);

  const std::array<float, LOB_SPLINE_SEGMENTS * 4> kDummyCoefs = {};
  EXPECT_EQ(LobBuilderSplineCoefficients(nullptr, kDummyCoefs.data()), nullptr);
  EXPECT_EQ(LobBuilderSplineCoefficients(nullptr, nullptr), nullptr);

  EXPECT_EQ(LobBuilderMassGrains(nullptr, kDummy), nullptr);
  EXPECT_EQ(LobBuilderInitialVelocityFps(nullptr, kDummy), nullptr);
  EXPECT_EQ(LobBuilderOpticHeightInches(nullptr, kDummy), nullptr);
  EXPECT_EQ(LobBuilderOpticLateralOffsetInches(nullptr, kDummy), nullptr);
  EXPECT_EQ(LobBuilderTwistInchesPerTurn(nullptr, kDummy), nullptr);
  EXPECT_EQ(LobBuilderZeroAngleMOA(nullptr, kDummy), nullptr);
  EXPECT_EQ(LobBuilderZeroAzimuthMOA(nullptr, kDummy), nullptr);
  EXPECT_EQ(LobBuilderZeroDistanceYds(nullptr, kDummy), nullptr);
  EXPECT_EQ(LobBuilderZeroImpactHeightInches(nullptr, kDummy), nullptr);
  EXPECT_EQ(LobBuilderAltitudeOfFiringSiteFt(nullptr, kDummy), nullptr);
  EXPECT_EQ(LobBuilderAirPressureInHg(nullptr, kDummy), nullptr);
  EXPECT_EQ(LobBuilderAltitudeOfBarometerFt(nullptr, kDummy), nullptr);
  EXPECT_EQ(LobBuilderTemperatureDegF(nullptr, kDummy), nullptr);
  EXPECT_EQ(LobBuilderAltitudeOfThermometerFt(nullptr, kDummy), nullptr);
  EXPECT_EQ(LobBuilderRelativeHumidityPercent(nullptr, kDummy), nullptr);
  EXPECT_EQ(LobBuilderWindHeading(nullptr, kLobClockAngleXII), nullptr);
  EXPECT_EQ(LobBuilderWindHeadingDeg(nullptr, kDummy), nullptr);
  EXPECT_EQ(LobBuilderWindSpeedFps(nullptr, kDummy), nullptr);
  EXPECT_EQ(LobBuilderWindSpeedMph(nullptr, kDummy), nullptr);

  const std::array<LobWindPoint, 2> kWindPts{};
  EXPECT_EQ(LobBuilderWindProfile(nullptr, kWindPts.data(), kWindPts.size()),
            nullptr);
  EXPECT_EQ(LobBuilderWindProfile(&builder, nullptr, kWindPts.size()),
            &builder);
  EXPECT_EQ(LobBuilderWindShearExponent(nullptr, kDummy), nullptr);
  EXPECT_EQ(LobBuilderAzimuthDeg(nullptr, kDummy), nullptr);
  EXPECT_EQ(LobBuilderLatitudeDeg(nullptr, kDummy), nullptr);
  EXPECT_EQ(LobBuilderRangeAngleDeg(nullptr, kDummy), nullptr);
  EXPECT_EQ(LobBuilderMinimumSpeed(nullptr, kDummy), nullptr);
  EXPECT_EQ(LobBuilderMinimumEnergy(nullptr, kDummy), nullptr);
  EXPECT_EQ(LobBuilderMaximumTime(nullptr, kDummy), nullptr);
  EXPECT_EQ(LobBuilderStepSize(nullptr, kDummy), nullptr);
  EXPECT_EQ(LobBuilderReset(nullptr), nullptr);
  LobBuilderInit(nullptr);
  LobBuilderBuild(nullptr, nullptr);
  LobContext ctx{};
  LobBuilderBuild(nullptr, &ctx);
}

TEST(LobCAPITest, SolveFunctionsNullptrReturnsZero) {
  const uint32_t kRange = 300U;
  LobOutput out{};
  const LobContext kCtx{};
  EXPECT_EQ(LobSolve(nullptr, &kRange, &out, 1U), 0U);
  EXPECT_EQ(LobSolve(&kCtx, nullptr, &out, 1U), 0U);
  EXPECT_EQ(LobSolve(&kCtx, &kRange, nullptr, 1U), 0U);
  EXPECT_EQ(LobSolve(&kCtx, &kRange, &out, 0U), 0U);

  EXPECT_EQ(LobSolveELR(nullptr, &kRange, &out, 1U), 0U);
  EXPECT_EQ(LobSolveELR(&kCtx, nullptr, &out, 1U), 0U);
  EXPECT_EQ(LobSolveELR(&kCtx, &kRange, nullptr, 1U), 0U);
  EXPECT_EQ(LobSolveELR(&kCtx, &kRange, &out, 0U), 0U);

  EXPECT_EQ(LobSolveInverse(nullptr, &kRange, &out, 1U), 0U);
  EXPECT_EQ(LobSolveInverse(&kCtx, nullptr, &out, 1U), 0U);
  EXPECT_EQ(LobSolveInverse(&kCtx, &kRange, nullptr, 1U), 0U);
  EXPECT_EQ(LobSolveInverse(&kCtx, &kRange, &out, 0U), 0U);
}

TEST(LobCAPITest, BuilderDestroyNullptrIsNoOp) { LobBuilderDestroy(nullptr); }

TEST(LobCAPITest, BuilderCopyNullptrIsNoOp) {
  LobBuilder builder;
  LobBuilderInit(&builder);
  LobBuilderCopy(nullptr, &builder);
  LobBuilder dst;
  LobBuilderInit(&dst);
  LobBuilderCopy(&dst, nullptr);
  LobBuilderCopy(nullptr, nullptr);
}

TEST(LobCAPITest, WindProfileCapacityConstant) {
  EXPECT_EQ(LOB_WIND_POINTS, 8);
}

TEST(LobCAPITest, WindNodeLayout) {
  EXPECT_EQ(sizeof(LobWindNode), 4 * sizeof(double));
  EXPECT_EQ(offsetof(LobWindNode, range_ft), 0U);
  EXPECT_EQ(offsetof(LobWindNode, x_fps), sizeof(double));
  EXPECT_EQ(offsetof(LobWindNode, y_fps), 2 * sizeof(double));
  EXPECT_EQ(offsetof(LobWindNode, z_fps), 3 * sizeof(double));
}

TEST(LobCAPITest, WindContextPacksWithoutWaste) {
  EXPECT_EQ(offsetof(LobContext, wind_nodes),
            offsetof(LobContext, drags) + sizeof(LobContext::drags));
  EXPECT_EQ(offsetof(LobContext, velocity),
            offsetof(LobContext, wind_shear_exponent) + sizeof(double));
  EXPECT_EQ(sizeof(LobContext),
            offsetof(LobContext, wind_count) + sizeof(uint8_t));
}

TEST(LobCAPITest, WindProfileErrorCodesAppended) {
  EXPECT_LT(kLobErrorWindHeadingOOR, kLobErrorWindProfileInvalid);
  EXPECT_LT(kLobErrorWindProfileInvalid, kLobErrorWindProfileNotMonotonic);
  EXPECT_LT(kLobErrorWindProfileNotMonotonic, kLobErrorWindProfileTooLong);
  EXPECT_LT(kLobErrorWindProfileTooLong, kLobErrorZeroAngleOOR);
}

TEST(LobCAPITest, WindProfileBuildZeroesUnusedTail) {
  const double kBcPsi = 0.372;
  const uint16_t kVelocityFps = 2720U;
  const double kZeroAngleMoa = 4.78;
  const int kGarbageByte = 0xAB;
  LobBuilder builder;
  LobBuilderInit(&builder);
  LobBuilderBallisticCoefficientPsi(&builder, kBcPsi);
  LobBuilderInitialVelocityFps(&builder, kVelocityFps);
  LobBuilderZeroAngleMOA(&builder, kZeroAngleMoa);
  const std::array<LobWindPoint, 2> kPts = {{
      {0.0, 90.0, 5.0, std::numeric_limits<double>::quiet_NaN()},
      {1500.0, 90.0, 6.0, std::numeric_limits<double>::quiet_NaN()},
  }};
  LobBuilderWindProfile(&builder, kPts.data(), kPts.size());
  LobContext ctx;
  std::memset(&ctx, kGarbageByte, sizeof(ctx));
  LobBuilderBuild(&builder, &ctx);
  LobBuilderDestroy(&builder);
  ASSERT_EQ(ctx.error, kLobErrorNone);
  ASSERT_EQ(ctx.wind_count, 2U);
  ExpectWindTailZeroed(ctx);
}

TEST(CApiTest, OpticLateralOffsetEntryPoints) {
  constexpr double kOffsetInches = 1.5;
  constexpr double kAzimuthMoa = -2.5;
  LobBuilder builder{};
  LobBuilderInit(&builder);
  EXPECT_NE(LobBuilderOpticLateralOffsetInches(&builder, kOffsetInches),
            nullptr);
  EXPECT_NE(LobBuilderZeroAzimuthMOA(&builder, kAzimuthMoa), nullptr);
  LobBuilderDestroy(&builder);
}

TEST(CApiTest, LobBuilderBuildFullyInitializesPresultAndPreventsStaleReuse) {
  constexpr double kDiameter = 0.308;
  constexpr double kLength = 1.25;
  constexpr double kMassGrains = 175.0;
  constexpr double kTwist = 10.0;
  constexpr uint16_t kVelocity = 2600U;
  constexpr double kZeroDistanceYds = 100.0;
  constexpr double kBc = 0.505;

  LobBuilder b1{};
  LobBuilderInit(&b1);
  LobBuilderDiameterInch(&b1, kDiameter);
  LobBuilderLengthInch(&b1, kLength);
  LobBuilderMassGrains(&b1, kMassGrains);
  LobBuilderTwistInchesPerTurn(&b1, kTwist);
  LobBuilderInitialVelocityFps(&b1, kVelocity);
  LobBuilderZeroDistanceYds(&b1, kZeroDistanceYds);
  LobBuilderBallisticCoefficientPsi(&b1, kBc);

  LobContext ctx{};
  LobBuilderBuild(&b1, &ctx);
  LobBuilderDestroy(&b1);

  ASSERT_EQ(ctx.error, kLobErrorNone);
  EXPECT_FALSE(std::isnan(ctx.stability_factor));
  EXPECT_GT(ctx.stability_factor, 1.0);

  // Build 2 reusing the SAME ctx with a builder that has no twist/diameter/mass
  // and temperature 0 deg F (speed of sound ~1052 fps)
  LobBuilder b2{};
  LobBuilderInit(&b2);
  LobBuilderTemperatureDegF(&b2, 0.0);
  LobBuilderInitialVelocityFps(&b2, kVelocity);
  LobBuilderZeroDistanceYds(&b2, kZeroDistanceYds);
  LobBuilderBallisticCoefficientPsi(&b2, kBc);

  LobBuilderBuild(&b2, &ctx);
  LobBuilderDestroy(&b2);

  ASSERT_EQ(ctx.error, kLobErrorNone);
  EXPECT_TRUE(std::isnan(ctx.stability_factor));
  EXPECT_NEAR(ctx.speed_of_sound, 1052.0, 5.0);

  // Build 3 reusing the SAME ctx with an invalid BC
  LobBuilder b3{};
  LobBuilderInit(&b3);
  LobBuilderInitialVelocityFps(&b3, kVelocity);
  LobBuilderZeroDistanceYds(&b3, kZeroDistanceYds);
  LobBuilderBallisticCoefficientPsi(&b3, -1.0);

  LobBuilderBuild(&b3, &ctx);
  LobBuilderDestroy(&b3);

  EXPECT_EQ(ctx.error, kLobErrorBallisticCoefficientOOR);
  EXPECT_TRUE(std::isnan(ctx.drag_coeff));
  EXPECT_FLOAT_EQ(ctx.drags[0], 0.0F);
  EXPECT_NEAR(ctx.speed_of_sound, 1116.45, 0.1);  // Reset from b2's 1052 to b3's 1116.45

  // Build 4 reusing the SAME ctx with early environment failure
  LobBuilder b4{};
  LobBuilderInit(&b4);
  LobBuilderAirPressureInHg(&b4, -1.0);

  LobBuilderBuild(&b4, &ctx);
  LobBuilderDestroy(&b4);

  EXPECT_EQ(ctx.error, kLobErrorAirPressureOOR);
  EXPECT_TRUE(std::isnan(ctx.speed_of_sound));
  EXPECT_TRUE(std::isnan(ctx.drag_coeff));
  EXPECT_FLOAT_EQ(ctx.drags[0], 0.0F);
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
