// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>

#include "lob/lob.h"

namespace tests {

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
