// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "eng_units.hpp"
#include "lob/lob.h"
#include "lob/lob.hpp"
#include "solve_angle.hpp"

namespace tests {

struct LobInverseTest : public testing::Test {
  const double kTestBC = 0.436;
  const uint16_t kTestMuzzleVelocity = 3100U;
  const double kTestZeroAngle = 6.11;
  const uint32_t kRangeYd100 = 300U;
  const uint32_t kRangeYd300 = 900U;
  const uint32_t kRangeYd600 = 1800U;
  const uint32_t kRangeYd1000 = 3000U;
  const double kWindSpeed = 5.0;

  lob::Context MakeTestContext(double bc, uint16_t mv) const {
    lob::Context result = lob::Builder()
                              .BallisticCoefficientPsi(bc)
                              .InitialVelocityFps(mv)
                              .ZeroAngleMOA(kTestZeroAngle)
                              .WindHeading(lob::ClockAngleT::kIII)
                              .WindSpeedMph(kWindSpeed)
                              .Build();
    EXPECT_EQ(result.error, lob::ErrorT::kNone);
    return result;
  }

  lob::Context context{};

  void SetUp() override {
    context = MakeTestContext(kTestBC, kTestMuzzleVelocity);
  }
};

TEST_F(LobInverseTest, SolveInverseSingleRangeReturnsOne) {
  lob::Output out{};
  const size_t kSize = lob::SolveInverse(context, kRangeYd300, &out);
  EXPECT_EQ(kSize, 1U);
  EXPECT_TRUE(std::isfinite(out.elevation));
  EXPECT_TRUE(std::isfinite(out.deflection));
}

TEST_F(LobInverseTest, SolveInverseSingleRangeMoaScale) {
  lob::Output out{};
  ASSERT_EQ(lob::SolveInverse(context, kRangeYd300, &out), 1U);
  EXPECT_NEAR(out.elevation, 0.0, 1.0);
}

TEST_F(LobInverseTest, SolveInverseAngleLimitedToQuadrant) {
  lob::Output out{};
  ASSERT_EQ(lob::SolveInverse(context, kRangeYd1000, &out), 1U);
  EXPECT_GT(out.elevation, 0.0);
  EXPECT_LT(std::fabs(out.elevation), 45.0 * 60.0);
}

TEST_F(LobInverseTest, SolveInverseArrayReturnsCount) {
  const std::array<uint32_t, 3> kRanges = {kRangeYd300, kRangeYd600,
                                           kRangeYd1000};
  std::array<lob::Output, 3> outs{};
  const size_t kSize = lob::SolveInverse(context, kRanges, &outs);
  EXPECT_EQ(kSize, kRanges.size());
}

TEST_F(LobInverseTest, SolveInverseAdjustmentGrowsWithRange) {
  const std::array<uint32_t, 3> kRanges = {kRangeYd300, kRangeYd600,
                                           kRangeYd1000};
  std::array<lob::Output, 3> outs{};
  ASSERT_EQ(lob::SolveInverse(context, kRanges, &outs), kRanges.size());
  EXPECT_GT(outs[2].elevation, outs[1].elevation);
  EXPECT_GT(outs[1].elevation, outs[0].elevation);
  EXPECT_TRUE(std::isfinite(outs[0].elevation));
  EXPECT_TRUE(std::isfinite(outs[1].elevation));
  EXPECT_TRUE(std::isfinite(outs[2].elevation));
}

TEST_F(LobInverseTest, SolveInverseRawPointerOverload) {
  const std::array<uint32_t, 2> kRanges = {kRangeYd300, kRangeYd600};
  std::array<lob::Output, 2> outs{};
  const size_t kSize =
      lob::SolveInverse(context, kRanges.data(), outs.data(), kRanges.size());
  EXPECT_EQ(kSize, kRanges.size());
}

TEST_F(LobInverseTest, SolveInverseInvalidContextReturnsZero) {
  const lob::Context kBad{};
  lob::Output out{};
  EXPECT_EQ(lob::SolveInverse(kBad, kRangeYd300, &out), 0U);
}

TEST_F(LobInverseTest, SolveInverseNonMonotonicRangesReturnsZero) {
  const std::array<uint32_t, 2> kRanges = {kRangeYd300, kRangeYd100};
  std::array<lob::Output, 2> outs{};
  EXPECT_EQ(lob::SolveInverse(context, kRanges, &outs), 0U);
}

TEST_F(LobInverseTest, SolveInverseFlipsSignsOfElevationAndDeflection) {
  lob::Output forward{};
  lob::Output inverse{};
  ASSERT_EQ(lob::Solve(context, kRangeYd300, &forward), 1U);
  ASSERT_EQ(lob::SolveInverse(context, kRangeYd300, &inverse), 1U);
  EXPECT_NE(std::signbit(forward.elevation), std::signbit(inverse.elevation));
  EXPECT_NE(std::signbit(forward.deflection), std::signbit(inverse.deflection));
}

TEST_F(LobInverseTest, SolveInverseCombinesForwardSolveAndOneStepCorrection) {
  lob::Output forward{};
  ASSERT_EQ(lob::Solve(context, kRangeYd300, &forward), 1U);
  lob::Output inverse{};
  ASSERT_EQ(lob::SolveInverse(context, kRangeYd300, &inverse), 1U);
  const lob::RadiansT kTheta =
      lob::RadiansT(lob::MoaT(context.zero_angle + context.aerodynamic_jump));
  const double kExpected =
      lob::MoaT(lob::FastInverseAngle(kTheta,
                                      lob::FeetT(lob::InchT(forward.elevation)),
                                      lob::FeetT(forward.range)) -
                kTheta)
          .Value();
  EXPECT_DOUBLE_EQ(inverse.elevation, kExpected);
}

TEST(LobELRInverseTest, SolveInverseELRRejectsInvalidArguments) {
  const uint32_t kRange = 300U;
  LobOutput out{};
  const LobContext kBad{};
  EXPECT_EQ(LobSolveInverseELR(nullptr, &kRange, &out, 1U), 0U);
  EXPECT_EQ(LobSolveInverseELR(&kBad, nullptr, &out, 1U), 0U);
  EXPECT_EQ(LobSolveInverseELR(&kBad, &kRange, nullptr, 1U), 0U);
  EXPECT_EQ(LobSolveInverseELR(&kBad, &kRange, &out, 0U), 0U);
}

TEST(LobELRInverseTest, SolveInverseELRZeroRangeReturnsZeroAdjustment) {
  const double kTestBC = 0.436;
  const uint16_t kTestMuzzleVelocity = 3100U;
  const double kTestZeroAngle = 6.11;
  LobBuilder builder{};
  LobBuilderInit(&builder);
  LobBuilderBallisticCoefficientPsi(&builder, kTestBC);
  LobBuilderInitialVelocityFps(&builder, kTestMuzzleVelocity);
  LobBuilderZeroAngleMOA(&builder, kTestZeroAngle);
  LobContext ctx{};
  LobBuilderBuild(&builder, &ctx);
  ASSERT_EQ(ctx.error, kLobErrorNone);
  LobBuilderDestroy(&builder);
  const std::array<uint32_t, 2> kRanges = {0U, 900U};
  std::array<LobOutput, 2> outs{};
  ASSERT_EQ(LobSolveInverseELR(&ctx, kRanges.data(), outs.data(), outs.size()),
            kRanges.size());
  EXPECT_DOUBLE_EQ(outs.at(0).elevation, 0.0);
  EXPECT_DOUBLE_EQ(outs.at(0).deflection, 0.0);
  EXPECT_TRUE(std::isfinite(outs.at(1).elevation));
}

TEST(LobELRInverseTest, CheapInverseMatchesELRToAngleToleranceAt1000Yd) {
  const double kTestBC = 0.436;
  const uint16_t kTestMuzzleVelocity = 3100U;
  const double kTestZeroAngle = 6.11;
  constexpr double kMaxDifferenceMoa = 0.01;
  LobBuilder builder{};
  LobBuilderInit(&builder);
  LobBuilderBallisticCoefficientPsi(&builder, kTestBC);
  LobBuilderInitialVelocityFps(&builder, kTestMuzzleVelocity);
  LobBuilderZeroAngleMOA(&builder, kTestZeroAngle);
  LobContext ctx{};
  LobBuilderBuild(&builder, &ctx);
  ASSERT_EQ(ctx.error, kLobErrorNone);
  LobBuilderDestroy(&builder);
  const std::array<uint32_t, 3> kRanges = {900U, 1800U, 3000U};
  std::array<LobOutput, 3> cheap{};
  ASSERT_EQ(LobSolveInverse(&ctx, kRanges.data(), cheap.data(), cheap.size()),
            cheap.size());
  std::array<LobOutput, 3> elr{};
  ASSERT_EQ(LobSolveInverseELR(&ctx, kRanges.data(), elr.data(), elr.size()),
            elr.size());
  for (size_t i = 0; i < kRanges.size(); ++i) {
    EXPECT_LE(std::fabs(cheap.at(i).elevation - elr.at(i).elevation),
              kMaxDifferenceMoa)
        << "range=" << kRanges.at(i);
  }
}

TEST(LobELRInverseTest, CheapInverseAgreesWithELRTo2000Yd) {
  const double kTestBC = 0.436;
  const uint16_t kTestMuzzleVelocity = 3100U;
  const double kTestZeroAngle = 6.11;
  constexpr double kMaxDifferenceMoa = 0.15;
  LobBuilder builder{};
  LobBuilderInit(&builder);
  LobBuilderBallisticCoefficientPsi(&builder, kTestBC);
  LobBuilderInitialVelocityFps(&builder, kTestMuzzleVelocity);
  LobBuilderZeroAngleMOA(&builder, kTestZeroAngle);
  LobContext ctx{};
  LobBuilderBuild(&builder, &ctx);
  ASSERT_EQ(ctx.error, kLobErrorNone);
  LobBuilderDestroy(&builder);
  constexpr size_t kNumRanges = 5;
  const std::array<uint32_t, kNumRanges> kRanges = {900U, 1800U, 3000U, 4500U,
                                                    6000U};
  std::array<LobOutput, kNumRanges> cheap{};
  ASSERT_EQ(LobSolveInverse(&ctx, kRanges.data(), cheap.data(), cheap.size()),
            cheap.size());
  std::array<LobOutput, kNumRanges> elr{};
  ASSERT_EQ(LobSolveInverseELR(&ctx, kRanges.data(), elr.data(), elr.size()),
            elr.size());
  for (size_t i = 0; i < kRanges.size(); ++i) {
    EXPECT_LE(std::fabs(cheap.at(i).elevation - elr.at(i).elevation),
              kMaxDifferenceMoa)
        << "range=" << kRanges.at(i);
  }
}

TEST(LobELRInverseTest, SolveInverseELRStopsWhenRefinementFails) {
  const double kTestBC = 0.436;
  const uint16_t kTestMuzzleVelocity = 3100U;
  const double kTestZeroAngle = 6.11;
  LobBuilder builder{};
  LobBuilderInit(&builder);
  LobBuilderBallisticCoefficientPsi(&builder, kTestBC);
  LobBuilderInitialVelocityFps(&builder, kTestMuzzleVelocity);
  LobBuilderZeroAngleMOA(&builder, kTestZeroAngle);
  LobContext ctx{};
  LobBuilderBuild(&builder, &ctx);
  ASSERT_EQ(ctx.error, kLobErrorNone);
  LobBuilderDestroy(&builder);
  // Forward flight reaches all five, so the cheap answer converts all five
  // while ELR refinement stops where the angle search fails.
  constexpr size_t kNumRanges = 5;
  constexpr size_t kNumRefined = 4;
  const std::array<uint32_t, kNumRanges> kRanges = {3000U, 6000U, 9000U, 12000U,
                                                    15000U};
  std::array<LobOutput, kNumRanges> cheap{};
  ASSERT_EQ(LobSolveInverse(&ctx, kRanges.data(), cheap.data(), cheap.size()),
            cheap.size());
  std::array<LobOutput, kNumRanges> elr{};
  EXPECT_EQ(LobSolveInverseELR(&ctx, kRanges.data(), elr.data(), elr.size()),
            kNumRefined);
  for (size_t i = 0; i < kNumRefined; ++i) {
    EXPECT_TRUE(std::isfinite(elr.at(i).elevation));
  }
}

TEST(LobELRInverseTest, SolveInverseELRStopsAtUnreachableRange) {
  const double kWeakBC = 0.05;
  const uint16_t kWeakVelocityFps = 500U;
  const double kWeakZeroAngleMoa = 6.11;
  const uint32_t kUnreachableRangeFt = 4000U;
  LobBuilder builder{};
  LobBuilderInit(&builder);
  LobBuilderBallisticCoefficientPsi(&builder, kWeakBC);
  LobBuilderInitialVelocityFps(&builder, kWeakVelocityFps);
  LobBuilderZeroAngleMOA(&builder, kWeakZeroAngleMoa);
  LobContext ctx{};
  LobBuilderBuild(&builder, &ctx);
  ASSERT_EQ(ctx.error, kLobErrorNone);
  LobBuilderDestroy(&builder);
  LobOutput out{};
  ASSERT_EQ(LobSolve(&ctx, &kUnreachableRangeFt, &out, 1U), 1U);
  ASSERT_LT(out.range, kUnreachableRangeFt);
  EXPECT_EQ(LobSolveInverseELR(&ctx, &kUnreachableRangeFt, &out, 1U), 0U);
}

TEST(LobELRInverseTest, SolveInverseELRStopsAtFirstUnreachableInArray) {
  const double kWeakBC = 0.05;
  const uint16_t kWeakVelocityFps = 500U;
  const double kWeakZeroAngleMoa = 6.11;
  LobBuilder builder{};
  LobBuilderInit(&builder);
  LobBuilderBallisticCoefficientPsi(&builder, kWeakBC);
  LobBuilderInitialVelocityFps(&builder, kWeakVelocityFps);
  LobBuilderZeroAngleMOA(&builder, kWeakZeroAngleMoa);
  LobContext ctx{};
  LobBuilderBuild(&builder, &ctx);
  ASSERT_EQ(ctx.error, kLobErrorNone);
  LobBuilderDestroy(&builder);
  const std::array<uint32_t, 3> kRanges = {300U, 900U, 4000U};
  std::array<LobOutput, 3> outs{};
  const size_t kSize =
      LobSolveInverseELR(&ctx, kRanges.data(), outs.data(), outs.size());
  EXPECT_EQ(kSize, 2U);
  EXPECT_TRUE(std::isfinite(outs.at(0).elevation));
  EXPECT_TRUE(std::isfinite(outs.at(1).elevation));
}

TEST(LobELRInverseTest, SolveInverseELRWrappersLinkAndSolve) {
  const lob::Context kCtx = lob::Builder()
                                .BallisticCoefficientPsi(0.436)
                                .InitialVelocityFps(3100U)
                                .ZeroAngleMOA(6.11)
                                .Build();
  ASSERT_EQ(kCtx.error, lob::ErrorT::kNone);
  const std::array<uint32_t, 2> kRanges = {900U, 1800U};
  std::array<lob::Output, 2> array_outs{};
  lob::Output pointer_out{};
  lob::Output single_out{};
  EXPECT_EQ(lob::SolveInverseELR(kCtx, kRanges.data(), &pointer_out, 1U), 1U);
  EXPECT_EQ(lob::SolveInverseELR(kCtx, kRanges, &array_outs), kRanges.size());
  EXPECT_EQ(lob::SolveInverseELR(kCtx, kRanges.at(0), &single_out), 1U);
}

TEST(LobELRInverseTest, SolveInverseELRStartsFromCheapInverseAndRefines) {
  const double kTestBC = 0.436;
  const uint16_t kTestMuzzleVelocity = 3100U;
  const double kTestZeroAngle = 6.11;
  LobBuilder builder{};
  LobBuilderInit(&builder);
  LobBuilderBallisticCoefficientPsi(&builder, kTestBC);
  LobBuilderInitialVelocityFps(&builder, kTestMuzzleVelocity);
  LobBuilderZeroAngleMOA(&builder, kTestZeroAngle);
  LobContext ctx{};
  LobBuilderBuild(&builder, &ctx);
  ASSERT_EQ(ctx.error, kLobErrorNone);
  LobBuilderDestroy(&builder);
  const std::array<uint32_t, 2> kRanges = {900U, 1800U};
  std::array<LobOutput, 2> cheap{};
  std::array<LobOutput, 2> elr{};
  ASSERT_EQ(LobSolveInverse(&ctx, kRanges.data(), cheap.data(), kRanges.size()),
            kRanges.size());
  ASSERT_EQ(
      LobSolveInverseELR(&ctx, kRanges.data(), elr.data(), kRanges.size()),
      kRanges.size());
  for (size_t i = 0; i < kRanges.size(); ++i) {
    // Cheap and ELR answers agree where the lapse correction is small;
    // the ELR-vs-iterative cross-check lives in solve_angle_test.cpp
    // (static-only, since it calls the internal angle solver directly).
    EXPECT_NEAR(cheap.at(i).elevation, elr.at(i).elevation, 0.1)
        << "range=" << kRanges.at(i);
  }
}

TEST_F(LobInverseTest, SolveInverseIgnoresDropThreshold) {
  const lob::Context kCtx = lob::Builder()
                                .BallisticCoefficientPsi(kTestBC)
                                .InitialVelocityFps(kTestMuzzleVelocity)
                                .ZeroAngleMOA(kTestZeroAngle)
                                .Build();
  ASSERT_EQ(kCtx.error, lob::ErrorT::kNone);
  const std::array<uint32_t, 1> kRanges = {7500U};
  std::array<lob::Output, 1> forward{};
  ASSERT_EQ(lob::Solve(kCtx, kRanges, &forward), kRanges.size());
  ASSERT_LT(forward.at(0).elevation, -1200.0);
  std::array<lob::Output, 1> inverse{};
  ASSERT_EQ(lob::SolveInverse(kCtx, kRanges, &inverse), kRanges.size());
  const lob::RadiansT kTheta =
      lob::RadiansT(lob::MoaT(kCtx.zero_angle + kCtx.aerodynamic_jump));
  const double kExpected =
      lob::MoaT(lob::FastInverseAngle(
                    kTheta, lob::FeetT(lob::InchT(forward.at(0).elevation)),
                    lob::FeetT(forward.at(0).range)) -
                kTheta)
          .Value();
  EXPECT_DOUBLE_EQ(inverse.at(0).elevation, kExpected);
}

TEST_F(LobInverseTest, SolveInverseZeroRangeReturnsZeroAdjustment) {
  const std::array<uint32_t, 2> kRanges = {0U, kRangeYd300};
  std::array<lob::Output, 2> outs{};
  const size_t kSize = lob::SolveInverse(context, kRanges, &outs);
  EXPECT_EQ(kSize, 2U);
  EXPECT_DOUBLE_EQ(outs[0].elevation, 0.0);
  EXPECT_DOUBLE_EQ(outs[0].deflection, 0.0);
  EXPECT_TRUE(std::isfinite(outs[1].elevation));
}

TEST_F(LobInverseTest, SolveInverseStopsAtUnreachableRange) {
  const lob::Context kWeak = lob::Builder()
                                 .BallisticCoefficientPsi(0.05)
                                 .InitialVelocityFps(500U)
                                 .ZeroAngleMOA(kTestZeroAngle)
                                 .Build();
  ASSERT_EQ(kWeak.error, lob::ErrorT::kNone);
  const uint32_t kRange = 4000U;
  lob::Output out{};
  ASSERT_EQ(lob::Solve(kWeak, kRange, &out), 1U);
  ASSERT_LT(out.range, kRange);
  EXPECT_EQ(lob::SolveInverse(kWeak, kRange, &out), 0U);
}

TEST_F(LobInverseTest, SolveInverseStopsAtFirstUnreachableInArray) {
  const lob::Context kWeak = lob::Builder()
                                 .BallisticCoefficientPsi(0.05)
                                 .InitialVelocityFps(500U)
                                 .ZeroAngleMOA(kTestZeroAngle)
                                 .Build();
  ASSERT_EQ(kWeak.error, lob::ErrorT::kNone);
  const std::array<uint32_t, 3> kRanges = {300U, 900U, 4000U};
  std::array<lob::Output, 3> outs{};
  const size_t kSize = lob::SolveInverse(kWeak, kRanges, &outs);
  EXPECT_EQ(kSize, 2U);
  EXPECT_TRUE(std::isfinite(outs[0].elevation));
  EXPECT_TRUE(std::isfinite(outs[1].elevation));
}

TEST_F(LobInverseTest, SolveInverseMatchesGeometricWarpWithJump) {
  const lob::Context kCtx = lob::Builder()
                                .BallisticCoefficientPsi(kTestBC)
                                .InitialVelocityFps(kTestMuzzleVelocity)
                                .ZeroAngleMOA(kTestZeroAngle)
                                .DiameterInch(0.308)
                                .LengthInch(1.215)
                                .MassGrains(168.0)
                                .TwistInchesPerTurn(10.0)
                                .WindHeading(lob::ClockAngleT::kIII)
                                .WindSpeedMph(10.0)
                                .Build();
  ASSERT_EQ(kCtx.error, lob::ErrorT::kNone);
  ASSERT_NE(kCtx.aerodynamic_jump, 0.0);
  const uint32_t kRangeFt = 4500U;
  lob::Output forward{};
  ASSERT_EQ(lob::Solve(kCtx, kRangeFt, &forward), 1U);
  lob::Output inverse{};
  ASSERT_EQ(lob::SolveInverse(kCtx, kRangeFt, &inverse), 1U);
  const lob::RadiansT kTheta =
      lob::RadiansT(lob::MoaT(kCtx.zero_angle + kCtx.aerodynamic_jump));
  const double kExpected =
      lob::MoaT(lob::FastInverseAngle(kTheta,
                                      lob::FeetT(lob::InchT(forward.elevation)),
                                      lob::FeetT(forward.range)) -
                kTheta)
          .Value();
  EXPECT_DOUBLE_EQ(inverse.elevation, kExpected);
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
