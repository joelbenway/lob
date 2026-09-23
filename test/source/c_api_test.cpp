// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include <gtest/gtest.h>

#include <array>
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
  EXPECT_EQ(LobBuilderTwistInchesPerTurn(nullptr, kDummy), nullptr);
  EXPECT_EQ(LobBuilderZeroAngleMOA(nullptr, kDummy), nullptr);
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

  EXPECT_EQ(LobFastInverse(nullptr, &out, 1U), 0U);
  EXPECT_EQ(LobFastInverse(&kCtx, nullptr, 1U), 0U);
  EXPECT_EQ(LobFastInverse(&kCtx, &out, 0U), 0U);

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
  // Wind nodes sit right after the drag table; the small-integer tail packs
  // last. Each boundary is exact, so no padding byte exists anywhere.
  EXPECT_EQ(offsetof(LobContext, wind_nodes),
            offsetof(LobContext, drags) + sizeof(LobContext::drags));
  EXPECT_EQ(offsetof(LobContext, velocity),
            offsetof(LobContext, wind_shear_exponent) + sizeof(double));
  EXPECT_EQ(sizeof(LobContext),
            offsetof(LobContext, wind_count) + sizeof(uint8_t));
}

TEST(LobCAPITest, WindProfileErrorCodesAppended) {
  // Wind codes sort alphabetically between WindHeadingOOR and ZeroAngleOOR.
  EXPECT_LT(kLobErrorWindHeadingOOR, kLobErrorWindProfileInvalid);
  EXPECT_LT(kLobErrorWindProfileInvalid, kLobErrorWindProfileNotMonotonic);
  EXPECT_LT(kLobErrorWindProfileNotMonotonic, kLobErrorWindProfileTooLong);
  EXPECT_LT(kLobErrorWindProfileTooLong, kLobErrorZeroAngleOOR);
}

TEST(LobCAPITest, WindProfileBuildZeroesUnusedTail) {
  // Build writes only wind_nodes[0..count-1]; the rest must be zeroed so
  // identically-built contexts compare equal. Fill the output with garbage
  // first to prove it.
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
      {0.0, 0.0, 7.0, std::numeric_limits<double>::quiet_NaN()},
      {1500.0, 0.0, 8.0, std::numeric_limits<double>::quiet_NaN()},
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
