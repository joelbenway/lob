// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include "solve_angle.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

#include "cartesian.hpp"
#include "constants.hpp"
#include "eng_units.hpp"
#include "helpers.hpp"
#include "lob/lob.h"
#include "ode.hpp"
#include "solve_step.hpp"
#include "splines.hpp"

namespace tests {
namespace {

struct SolveAngleTest : public testing::Test {
  const double kTestBC = 0.436;
  const uint16_t kTestMuzzleVelocity = 3100U;
  const double kTestZeroAngle = 6.11;

  LobContext BuildContext(double bc, uint16_t mv) const {
    LobBuilder builder{};
    LobBuilderInit(&builder);
    LobBuilderBallisticCoefficientPsi(&builder, bc);
    LobBuilderInitialVelocityFps(&builder, mv);
    LobBuilderZeroAngleMOA(&builder, kTestZeroAngle);
    LobContext result{};
    LobBuilderBuild(&builder, &result);
    EXPECT_EQ(result.error, kLobErrorNone);
    LobBuilderDestroy(&builder);
    return result;
  }

  LobContext ctx{};

  void SetUp() override { ctx = BuildContext(kTestBC, kTestMuzzleVelocity); }
};

TEST_F(SolveAngleTest, FastInverseAngleZeroResidualReturnsInputAngle) {
  const lob::RadiansT kTheta(0.1);
  const lob::FeetT kResidual(0.0);
  const lob::FeetT kRange(300.0);
  const lob::RadiansT kResult =
      lob::FastInverseAngle(kTheta, kResidual, kRange);
  EXPECT_DOUBLE_EQ(kResult.Value(), kTheta.Value());
}

TEST_F(SolveAngleTest, FastInverseAngleNonPositiveRangeReturnsInputAngle) {
  const lob::RadiansT kTheta(0.1);
  const lob::FeetT kResidual(3.0);
  for (const double kRangeValue : {0.0, -300.0}) {
    const lob::FeetT kRange(kRangeValue);
    const lob::RadiansT kResult =
        lob::FastInverseAngle(kTheta, kResidual, kRange);
    EXPECT_DOUBLE_EQ(kResult.Value(), kTheta.Value());
  }
}

TEST_F(SolveAngleTest, FastInverseAngleNegativeResidualIncreasesAngle) {
  const lob::RadiansT kTheta(0.1);
  const lob::FeetT kRange(300.0);
  const lob::RadiansT kResultPos =
      lob::FastInverseAngle(kTheta, lob::FeetT(3.0), kRange);
  const lob::RadiansT kResultNeg =
      lob::FastInverseAngle(kTheta, lob::FeetT(-3.0), kRange);
  EXPECT_LT(kResultPos.Value(), kTheta.Value());
  EXPECT_GT(kResultNeg.Value(), kTheta.Value());
}

TEST_F(SolveAngleTest, FastInverseAngleMonotonicInResidual) {
  const lob::RadiansT kTheta(0.1);
  const lob::FeetT kRange(300.0);
  const lob::RadiansT kR1 =
      lob::FastInverseAngle(kTheta, lob::FeetT(1.0), kRange);
  const lob::RadiansT kR2 =
      lob::FastInverseAngle(kTheta, lob::FeetT(2.0), kRange);
  const lob::RadiansT kR3 =
      lob::FastInverseAngle(kTheta, lob::FeetT(3.0), kRange);
  EXPECT_GT(kR1.Value(), kR2.Value());
  EXPECT_GT(kR2.Value(), kR3.Value());
}

lob::FeetT FireAndMeasureImpact(const LobContext& ctx,
                                const lob::MoaT& launch_angle,
                                const lob::FeetT& range) {
  const double kAngleRad =
      lob::RadiansT(launch_angle).Value() +
      lob::RadiansT(lob::MoaT(ctx.aerodynamic_jump)).Value();
  lob::TrajectoryStateT s(
      lob::CartesianT<lob::FeetT>(lob::FeetT(0.0)),
      lob::CartesianT<lob::FpsT>(lob::FpsT(ctx.velocity) * std::cos(kAngleRad),
                                 lob::FpsT(ctx.velocity) * std::sin(kAngleRad),
                                 lob::FpsT(0.0)));
  lob::spline::CurveView curve(lob::spline::kKnots.data(), &ctx.drags[0]);
  while (s.P().X() < range) {
    if (s.V().X() <= lob::FpsT(0) || s.TOF() >= lob::SecT(ctx.max_time) ||
        s.V().Magnitude() <= lob::FpsT(ctx.minimum_speed)) {
      return lob::FeetT(lob::NaN());
    }
    lob::FastSolveStep(ctx, &s, &curve, range);
  }
  return s.P().Y() - lob::FeetT(ctx.optic_height);
}

TEST_F(SolveAngleTest, SolveAngleConvergesFromZeroSeed) {
  const lob::FeetT kRange = lob::FeetT(lob::YardT(300));
  const lob::MoaT kAngle =
      lob::FastSolveAngle(ctx, kRange, lob::FeetT(0.0), lob::RadiansT(0.0));
  ASSERT_FALSE(kAngle.IsNaN());
  EXPECT_TRUE(std::isfinite(kAngle.Value()));
  EXPECT_GT(kAngle.Value(), 0.0);
  EXPECT_NEAR(FireAndMeasureImpact(ctx, kAngle, kRange).Value(), 0.0, 0.1);
}

TEST_F(SolveAngleTest, SolveAngleConvergesFromPositiveSeed) {
  const lob::FeetT kRange = lob::FeetT(lob::YardT(300));
  const lob::MoaT kAngle = lob::FastSolveAngle(
      ctx, kRange, lob::FeetT(0.0), lob::RadiansT(lob::DegreesT(30)));
  ASSERT_FALSE(kAngle.IsNaN());
  EXPECT_TRUE(std::isfinite(kAngle.Value()));
  EXPECT_GT(kAngle.Value(), 0.0);
  EXPECT_NEAR(FireAndMeasureImpact(ctx, kAngle, kRange).Value(), 0.0, 0.1);
}

TEST_F(SolveAngleTest, SolveAngleConvergesFromNegativeSeed) {
  const lob::FeetT kRange = lob::FeetT(lob::YardT(300));
  const lob::MoaT kAngle = lob::FastSolveAngle(
      ctx, kRange, lob::FeetT(0.0), lob::RadiansT(lob::DegreesT(-30)));
  ASSERT_FALSE(kAngle.IsNaN());
  EXPECT_TRUE(std::isfinite(kAngle.Value()));
  EXPECT_GT(kAngle.Value(), 0.0);
  EXPECT_NEAR(FireAndMeasureImpact(ctx, kAngle, kRange).Value(), 0.0, 0.1);
}

TEST_F(SolveAngleTest, SolveAngleConvergesFromNearMaxAngleSeed) {
  const lob::FeetT kRange = lob::FeetT(lob::YardT(300));
  const lob::MoaT kAngle = lob::FastSolveAngle(
      ctx, kRange, lob::FeetT(0.0), lob::RadiansT(lob::DegreesT(44)));
  ASSERT_FALSE(kAngle.IsNaN());
  EXPECT_TRUE(std::isfinite(kAngle.Value()));
  EXPECT_GT(kAngle.Value(), 0.0);
  EXPECT_NEAR(FireAndMeasureImpact(ctx, kAngle, kRange).Value(), 0.0, 0.1);
}

TEST_F(SolveAngleTest, SolveAngleRoundTripImpactHeightZero) {
  const lob::FeetT kRange = lob::FeetT(lob::YardT(300));
  const lob::MoaT kLaunchAngle =
      lob::FastSolveAngle(ctx, kRange, lob::FeetT(0.0), lob::RadiansT(0.0));
  ASSERT_FALSE(kLaunchAngle.IsNaN());
  EXPECT_NEAR(FireAndMeasureImpact(ctx, kLaunchAngle, kRange).Value(), 0.0,
              0.1);
}

TEST_F(SolveAngleTest, SolveAngleRoundTripImpactHeightPositive) {
  const lob::FeetT kRange = lob::FeetT(lob::YardT(300));
  const lob::FeetT kImpactHeight(3.0);
  const lob::MoaT kLaunchAngle =
      lob::FastSolveAngle(ctx, kRange, kImpactHeight, lob::RadiansT(0.0));
  ASSERT_FALSE(kLaunchAngle.IsNaN());
  EXPECT_NEAR(FireAndMeasureImpact(ctx, kLaunchAngle, kRange).Value(),
              kImpactHeight.Value(), 0.1);
}

TEST_F(SolveAngleTest, SolveAngleRoundTripImpactHeightNegative) {
  const lob::FeetT kRange = lob::FeetT(lob::YardT(300));
  const lob::FeetT kImpactHeight(-3.0);
  const lob::MoaT kLaunchAngle =
      lob::FastSolveAngle(ctx, kRange, kImpactHeight, lob::RadiansT(0.0));
  ASSERT_FALSE(kLaunchAngle.IsNaN());
  EXPECT_NEAR(FireAndMeasureImpact(ctx, kLaunchAngle, kRange).Value(),
              kImpactHeight.Value(), 0.1);
}

TEST_F(SolveAngleTest, SolveAngleVeryShortRange) {
  const lob::FeetT kRange(10.0);
  const lob::MoaT kAngle =
      lob::FastSolveAngle(ctx, kRange, lob::FeetT(0.0), lob::RadiansT(0.0));
  EXPECT_FALSE(kAngle.IsNaN());
}

TEST_F(SolveAngleTest, SolveAngleTightTolerance) {
  const lob::FeetT kRange = lob::FeetT(lob::YardT(300));
  const lob::RadiansT kTol = lob::RadiansT(lob::MoaT(0.001));
  const lob::MoaT kAngle = lob::FastSolveAngle(ctx, kRange, lob::FeetT(0.0),
                                               lob::RadiansT(0.0), kTol);
  ASSERT_FALSE(kAngle.IsNaN());
  EXPECT_TRUE(std::isfinite(kAngle.Value()));
  EXPECT_NEAR(FireAndMeasureImpact(ctx, kAngle, kRange).Value(), 0.0, 0.01);
}

TEST_F(SolveAngleTest, SolveAngleUnreachableTargetReturnsNaN) {
  const LobContext kWeakCtx = BuildContext(0.1, 600U);
  const lob::FeetT kRange = lob::FeetT(lob::YardT(2000));
  const lob::MoaT kAngle = lob::FastSolveAngle(
      kWeakCtx, kRange, lob::FeetT(0.0), lob::RadiansT(0.0));
  EXPECT_TRUE(kAngle.IsNaN());
}

TEST_F(SolveAngleTest, SolveAngleAngleClampPastMaxAngleReturnsNaN) {
  const lob::FeetT kRange(10.0);
  const lob::MoaT kAngle = lob::FastSolveAngle(
      ctx, kRange, lob::FeetT(15.0), lob::RadiansT(lob::DegreesT(44)));
  EXPECT_TRUE(kAngle.IsNaN());
}

TEST_F(SolveAngleTest, FastInverseMatchesSolveAngle) {
  const std::array<uint32_t, 3> kRanges = {900U, 1800U, 3000U};
  const double kToleranceMoa = 0.01;
  for (const uint32_t kRangeFt : kRanges) {
    LobOutput forward{};
    ASSERT_EQ(LobSolve(&ctx, &kRangeFt, &forward, 1U), 1U);
    ASSERT_EQ(LobFastInverse(&ctx, &forward, 1U), 1U);
    const lob::MoaT kSolved = lob::FastSolveAngle(
        ctx, lob::FeetT(static_cast<double>(kRangeFt)), lob::FeetT(0.0),
        lob::RadiansT(lob::MoaT(ctx.zero_angle)));
    ASSERT_FALSE(kSolved.IsNaN());
    EXPECT_NEAR(kSolved.Value(), kTestZeroAngle + forward.elevation,
                kToleranceMoa)
        << "range=" << kRangeFt;
  }
}

TEST_F(SolveAngleTest, SolveInverseMatchesSolveAngle) {
  const std::array<uint32_t, 3> kRanges = {900U, 1800U, 3000U};
  const double kToleranceMoa = 0.01;
  for (const uint32_t kRangeFt : kRanges) {
    LobOutput inverse{};
    ASSERT_EQ(LobSolveInverse(&ctx, &kRangeFt, &inverse, 1U), 1U);
    const lob::MoaT kSolved = lob::FastSolveAngle(
        ctx, lob::FeetT(static_cast<double>(kRangeFt)), lob::FeetT(0.0),
        lob::RadiansT(lob::MoaT(ctx.zero_angle)));
    ASSERT_FALSE(kSolved.IsNaN());
    EXPECT_NEAR(kSolved.Value(), kTestZeroAngle + inverse.elevation,
                kToleranceMoa)
        << "range=" << kRangeFt;
  }
}

TEST(BuilderZeroAngleTest, MatchesSolveAngle) {
  const double kTestBC = 0.436;
  const uint16_t kTestMuzzleVelocity = 3100U;
  const double kZeroDistanceYards = 100.0;

  LobBuilder builder{};
  LobBuilderInit(&builder);
  LobBuilderBallisticCoefficientPsi(&builder, kTestBC);
  LobBuilderInitialVelocityFps(&builder, kTestMuzzleVelocity);
  LobBuilderZeroDistanceYds(&builder, kZeroDistanceYards);
  LobContext ctx{};
  LobBuilderBuild(&builder, &ctx);
  ASSERT_EQ(ctx.error, kLobErrorNone);
  LobBuilderDestroy(&builder);

  const lob::FeetT kRange = lob::FeetT(lob::YardT(kZeroDistanceYards));
  const auto kSeed = std::max(
      lob::constant::kMinAngle,
      std::min(
          lob::constant::kMaxAngle,
          lob::RadiansT(0.5 * lob::kStandardGravityFtPerSecSq * kRange.Value() /
                        (kTestMuzzleVelocity * kTestMuzzleVelocity))));
  const lob::MoaT kAngle =
      lob::FastSolveAngle(ctx, kRange, lob::FeetT(0.0), kSeed);
  ASSERT_FALSE(kAngle.IsNaN());
  EXPECT_NEAR(kAngle.Value(), ctx.zero_angle, 1.0e-6);
}

}  // namespace
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
