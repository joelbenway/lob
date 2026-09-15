// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include "solve_step.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

#include "cartesian.hpp"
#include "constants.hpp"
#include "eng_units.hpp"
#include "lob/lob.h"
#include "ode.hpp"
#include "splines.hpp"

namespace tests {
namespace {

constexpr double kTestBC = 0.436;
constexpr uint16_t kTestMuzzleVelocity = 3100U;
constexpr double kTestZeroAngle = 6.11;
constexpr double kWindSpeedMph = 10.0;
constexpr double kGrassRoughnessFt = 0.1;
constexpr double kBoreHeightFt = 1.0;
constexpr double kInclineDeg = 15.0;
constexpr double kQueryWindZFps = 10.0;
constexpr double kQuerySpeedFps = 2000.0;

LobContext BuildContext(uint16_t step_size_in) {
  LobBuilder builder{};
  LobBuilderInit(&builder);
  LobBuilderBallisticCoefficientPsi(&builder, kTestBC);
  LobBuilderInitialVelocityFps(&builder, kTestMuzzleVelocity);
  LobBuilderZeroAngleMOA(&builder, kTestZeroAngle);
  LobBuilderStepSize(&builder, step_size_in);
  LobContext ctx{};
  LobBuilderBuild(&builder, &ctx);
  EXPECT_EQ(ctx.error, kLobErrorNone);
  LobBuilderDestroy(&builder);
  return ctx;
}

size_t CountStepsTo(const LobContext& ctx, lob::FeetT target) {
  const double kAngle = lob::RadiansT(lob::MoaT(ctx.zero_angle)).Value();
  lob::TrajectoryStateT s(
      lob::CartesianT<lob::FeetT>(lob::FeetT(0.0)),
      lob::CartesianT<lob::FpsT>(lob::FpsT(ctx.velocity) * std::cos(kAngle),
                                 lob::FpsT(ctx.velocity) * std::sin(kAngle),
                                 lob::FpsT(0.0)));
  lob::spline::CurveView curve(lob::spline::kKnots.data(), &ctx.drags[0]);
  size_t steps = 0;
  // Tolerance absorbs floating point rounding in the integrator (noise is
  // ~1e-13 ft); any real step-size error is at least 1 inch.
  constexpr double kRoundingToleranceFt = 1e-6;
  while (s.P().X() < target - lob::FeetT(kRoundingToleranceFt)) {
    lob::FastSolveStep(ctx, &s, &curve, target);
    ++steps;
  }
  return steps;
}

using lob::CartesianT;
using lob::FeetT;
using lob::FpsT;
using lob::TrajectoryStateT;

LobContext MakeWindQueryCtx() {
  LobContext ctx{};
  ctx.gravity.x = 0.0;
  ctx.gravity.y = -lob::kStandardGravityFtPerSecSq;
  ctx.wind_nodes[0].range_ft = 0U;
  ctx.wind_nodes[0].x_fps = 0.0F;
  ctx.wind_nodes[0].y_fps = 0.0F;
  ctx.wind_nodes[0].z_fps = static_cast<float>(kQueryWindZFps);
  ctx.wind_count = 1;
  ctx.wind_roughness_ft = std::numeric_limits<double>::quiet_NaN();
  ctx.wind_muzzle_height_ft = std::numeric_limits<double>::quiet_NaN();
  return ctx;
}

TrajectoryStateT MakeStateAt(double x_ft, double y_ft) {
  return {CartesianT<FeetT>(FeetT(x_ft), FeetT(y_ft), FeetT(0.0)),
          CartesianT<FpsT>(FpsT(kQuerySpeedFps), FpsT(0.0), FpsT(0.0))};
}

// Builds a uniform-wind LobContext through the C API, so query tests can
// hold the C type directly instead of casting out of lob::Context.
LobContext BuildUniformWindCtx(
    double speed_mph, double range_angle_deg,
    double roughness_ft = std::numeric_limits<double>::quiet_NaN(),
    double bore_height_ft = std::numeric_limits<double>::quiet_NaN()) {
  LobBuilder builder;
  LobBuilderInit(&builder);
  LobBuilderBallisticCoefficientPsi(&builder, kTestBC);
  LobBuilderInitialVelocityFps(&builder, kTestMuzzleVelocity);
  LobBuilderZeroAngleMOA(&builder, kTestZeroAngle);
  LobBuilderWindHeading(&builder, kLobClockAngleIII);
  LobBuilderWindSpeedMph(&builder, speed_mph);
  LobBuilderRangeAngleDeg(&builder, range_angle_deg);
  if (!std::isnan(roughness_ft)) {
    LobBuilderWindRoughnessLengthFt(&builder, roughness_ft);
    LobBuilderHeightOfBoreAboveGroundFt(&builder, bore_height_ft);
  }
  LobContext ctx{};
  LobBuilderBuild(&builder, &ctx);
  LobBuilderDestroy(&builder);
  return ctx;
}

const LobWindNode kLerpLow{1000U, 0.0F, 0.0F, 10.0F};
const LobWindNode kLerpHigh{2000U, 0.0F, 0.0F, 30.0F};
}  // namespace

TEST(SolveStepTests, TwelveInchStepOneYardSolveTakesThreeSteps) {
  const LobContext kCtx = BuildContext(12U);
  const lob::FeetT kTarget = lob::YardT(1);

  EXPECT_EQ(CountStepsTo(kCtx, kTarget), 3U);
}

TEST(SolveStepTests, OneHundredEightyInchStepTenYardSolveTakesTwoSteps) {
  const LobContext kCtx = BuildContext(180U);
  const lob::FeetT kTarget = lob::FeetT(lob::YardT(10));

  EXPECT_EQ(CountStepsTo(kCtx, kTarget), 2U);
}

TEST(SolveStepTests, ComputeStepTargetBehindReturnsStepSize) {
  const LobContext kCtx = BuildContext(36U);  // 1 yard step
  const double kAngle = lob::RadiansT(lob::MoaT(kCtx.zero_angle)).Value();
  lob::TrajectoryStateT s(
      lob::CartesianT<lob::FeetT>(lob::FeetT(100.0)),
      lob::CartesianT<lob::FpsT>(lob::FpsT(kCtx.velocity) * std::cos(kAngle),
                                 lob::FpsT(kCtx.velocity) * std::sin(kAngle),
                                 lob::FpsT(0.0)));
  lob::spline::CurveView curve(lob::spline::kKnots.data(), &kCtx.drags[0]);
  const lob::FeetT kTargetBehind = lob::FeetT(50.0);  // behind s.P().X()=100
  const lob::FeetT kBeforeX = s.P().X();
  lob::FastSolveStep(kCtx, &s, &curve, kTargetBehind);
  EXPECT_GT(s.P().X().Value(), kBeforeX.Value());

  lob::TrajectoryStateT s2(
      lob::CartesianT<lob::FeetT>(lob::FeetT(100.0)),
      lob::CartesianT<lob::FpsT>(lob::FpsT(kCtx.velocity) * std::cos(kAngle),
                                 lob::FpsT(kCtx.velocity) * std::sin(kAngle),
                                 lob::FpsT(0.0)));
  const lob::FeetT kBeforeX2 = s2.P().X();
  lob::SolveStep(kCtx, &s2, &curve, kTargetBehind);
  EXPECT_GT(s2.P().X().Value(), kBeforeX2.Value());
}

TEST(SolveStepTests, SolveStepClampsNegativeVx) {
  constexpr uint16_t kLowVelocityFps = 600U;
  constexpr double kHeadwindMph = 100.0;
  constexpr double kHeadwindDeg = 180.0;
  constexpr double kFarTargetFt = 1000.0;
  LobBuilder builder{};
  LobBuilderInit(&builder);
  LobBuilderBallisticCoefficientPsi(&builder, kTestBC);
  LobBuilderInitialVelocityFps(&builder, kLowVelocityFps);
  LobBuilderZeroAngleMOA(&builder, kTestZeroAngle);
  LobBuilderWindSpeedMph(&builder, kHeadwindMph);
  LobBuilderWindHeadingDeg(&builder, kHeadwindDeg);
  LobContext ctx{};
  LobBuilderBuild(&builder, &ctx);
  ASSERT_EQ(ctx.error, kLobErrorNone);
  LobBuilderDestroy(&builder);

  lob::TrajectoryStateT s(lob::CartesianT<lob::FeetT>(lob::FeetT(0.0)),
                          lob::CartesianT<lob::FpsT>(
                              lob::FpsT(1.0), lob::FpsT(0.0), lob::FpsT(0.0)));
  lob::spline::CurveView curve(lob::spline::kKnots.data(), &ctx.drags[0]);
  lob::SolveStep(ctx, &s, &curve, lob::FeetT(kFarTargetFt));
  EXPECT_EQ(s.V().X().Value(), 0.0);
  EXPECT_EQ(s.V().Y().Value(), 0.0);
  EXPECT_EQ(s.V().Z().Value(), 0.0);
}

TEST(WindProfileQuery, UniformFlatIsHistoricalPath) {
  const LobContext kCtx = MakeWindQueryCtx();
  const CartesianT<FpsT> kW = lob::GetWind(kCtx, MakeStateAt(500.0, 0.0));
  EXPECT_DOUBLE_EQ(kW.X().Value(), 0.0);
  EXPECT_DOUBLE_EQ(kW.Y().Value(), 0.0);
  EXPECT_DOUBLE_EQ(kW.Z().Value(), 10.0);
}

TEST(WindProfileQuery, LerpsMidpointAndClampsEnds) {
  LobContext ctx = MakeWindQueryCtx();  // nodes[0] is the muzzle node
  ctx.wind_count = 3;
  ctx.wind_nodes[1] = kLerpLow;
  ctx.wind_nodes[2] = kLerpHigh;
  EXPECT_DOUBLE_EQ(lob::GetWind(ctx, MakeStateAt(1500.0, 0.0)).Z().Value(),
                   20.0);
  EXPECT_DOUBLE_EQ(lob::GetWind(ctx, MakeStateAt(1000.0, 0.0)).Z().Value(),
                   10.0);
  EXPECT_DOUBLE_EQ(lob::GetWind(ctx, MakeStateAt(5000.0, 0.0)).Z().Value(),
                   30.0);
  EXPECT_DOUBLE_EQ(lob::GetWind(ctx, MakeStateAt(-10.0, 0.0)).Z().Value(),
                   10.0);
}

TEST(WindProfileQuery, AltitudeScalesAboutMuzzleReference) {
  // z0 = grass roughness, muzzle at bore height: at true height 9 ft,
  // z_agl = 10, S = 2 exactly.
  LobContext ctx = MakeWindQueryCtx();
  ctx.wind_roughness_ft = kGrassRoughnessFt;
  ctx.wind_muzzle_height_ft = kBoreHeightFt;
  const CartesianT<FpsT> kAtMuzzle = lob::GetWind(ctx, MakeStateAt(0.0, 0.0));
  EXPECT_NEAR(kAtMuzzle.Z().Value(), 10.0, 1E-9);  // S = 1 at muzzle
  const CartesianT<FpsT> kHigh = lob::GetWind(ctx, MakeStateAt(0.0, 9.0));
  EXPECT_NEAR(kHigh.Z().Value(), 20.0, 1E-9);  // S = ln(100)/ln(10) = 2
}

TEST(WindProfileQuery, SubBoreFloorKeepsLogDefined) {
  LobContext ctx = MakeWindQueryCtx();
  ctx.wind_roughness_ft = kGrassRoughnessFt;
  ctx.wind_muzzle_height_ft = kBoreHeightFt;
  const CartesianT<FpsT> kW = lob::GetWind(ctx, MakeStateAt(2500.0, -40.0));
  EXPECT_TRUE(std::isfinite(kW.X().Value()));
  EXPECT_TRUE(std::isfinite(kW.Y().Value()));
  EXPECT_TRUE(std::isfinite(kW.Z().Value()));
  EXPECT_GE(kW.Z().Value(), 0.0);
}

TEST(WindProfileQuery, CrosswindResolvesIdenticallyWithIncline) {
  // Spec section 9: the wind query resolves pure crosswind identically;
  // pitch cannot touch pure-crosswind output.
  const LobContext kFlatCtx = BuildUniformWindCtx(kWindSpeedMph, 0.0);
  const LobContext kHillCtx = BuildUniformWindCtx(kWindSpeedMph, kInclineDeg);
  const std::array<double, 3> kXs = {100.0, 900.0, 2700.0};
  for (const double kX : kXs) {
    const CartesianT<FpsT> kFlatW =
        lob::GetWind(kFlatCtx, MakeStateAt(kX, 0.0));
    const CartesianT<FpsT> kHillW =
        lob::GetWind(kHillCtx, MakeStateAt(kX, 0.0));
    // ponytail: kIII heading is 2π rad; libm sin leaves ~3.6e-15 in
    // wind.x which pitch then scales — same 1e-12 tolerance as
    // SinglePointEqualsUniform. The crosswind (Z) component is bit-identical.
    EXPECT_NEAR(kFlatW.X().Value(), kHillW.X().Value(), 1e-12);
    EXPECT_NEAR(kFlatW.Y().Value(), kHillW.Y().Value(), 1e-12);
    EXPECT_DOUBLE_EQ(kFlatW.Z().Value(), kHillW.Z().Value());
  }
}

TEST(WindProfileQuery, InclinedScalingUsesTrueVertical) {
  // Regression: altitude scaling must resolve height through the gravity
  // vector, never frame-Y. At 15° incline and state (1500, 0), frame-Y says
  // height 0 (S = 1) while true height is x*sin(15°) ≈ 388 ft (S ≈ 3.59).
  const LobContext kCtx = BuildUniformWindCtx(kWindSpeedMph, kInclineDeg,
                                              kGrassRoughnessFt, kBoreHeightFt);
  const CartesianT<FpsT> kW = lob::GetWind(kCtx, MakeStateAt(1500.0, 0.0));
  const double kG = std::sqrt((kCtx.gravity.x * kCtx.gravity.x) +
                              (kCtx.gravity.y * kCtx.gravity.y));
  const double kH = -((1500.0 * kCtx.gravity.x) / kG);
  const double kS = std::log((kH + kBoreHeightFt) / kGrassRoughnessFt) /
                    std::log(kBoreHeightFt / kGrassRoughnessFt);
  // Guard against a vacuous test: the gravity-vector answer must differ
  // decisively from the frame-Y answer (S = 1). The expectation derives
  // from the stored float node, so quantization cannot falsely fail it.
  EXPECT_GT(std::abs(kS - 1.0), 2.0);
  EXPECT_NEAR(kW.Z().Value(),
              static_cast<double>(kCtx.wind_nodes[0].z_fps) * kS, 1e-9);
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
