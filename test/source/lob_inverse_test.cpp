// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "helpers.hpp"
#include "lob/lob.hpp"

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
  const double kZeroRangeElevation = 5.0;
  const double kZeroRangeDeflection = 7.0;
  const double kInvalidElevation = 10.0;

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

  lob::Output ForwardSolve(uint32_t range_ft) const {
    lob::Output out{};
    const size_t kSize = lob::Solve(context, range_ft, &out);
    EXPECT_EQ(kSize, 1U);
    return out;
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

TEST_F(LobInverseTest, FastInverseConvertsForwardOutputInPlace) {
  lob::Output out = ForwardSolve(kRangeYd300);
  const double kElevation(out.elevation);
  const size_t kSize = lob::FastInverse(context, &out, 1U);
  EXPECT_EQ(kSize, 1U);
  EXPECT_NE(kElevation, out.elevation);
  EXPECT_TRUE(std::isfinite(out.elevation));
  EXPECT_TRUE(std::isfinite(out.deflection));
}

TEST_F(LobInverseTest, FastInverseSkipsZeroRangeOutputs) {
  lob::Output out{};
  out.range = 0;
  out.elevation = kZeroRangeElevation;
  out.deflection = kZeroRangeDeflection;
  const size_t kSize = lob::FastInverse(context, &out, 1U);
  EXPECT_EQ(kSize, 0U);
  EXPECT_DOUBLE_EQ(out.elevation, kZeroRangeElevation);
  EXPECT_DOUBLE_EQ(out.deflection, kZeroRangeDeflection);
}

TEST_F(LobInverseTest, FastInverseSkipsNonFiniteElevation) {
  lob::Output out{};
  out.range = kRangeYd300;
  out.elevation = lob::NaN();
  out.deflection = 0.0;
  const size_t kSize = lob::FastInverse(context, &out, 1U);
  EXPECT_EQ(kSize, 0U);
  EXPECT_TRUE(std::isnan(out.elevation));
}

TEST_F(LobInverseTest, FastInverseSkipsNonFiniteDeflection) {
  lob::Output out{};
  out.range = kRangeYd300;
  out.elevation = 0.0;
  out.deflection = lob::NaN();
  const size_t kSize = lob::FastInverse(context, &out, 1U);
  EXPECT_EQ(kSize, 0U);
}

TEST_F(LobInverseTest, FastInverseInvalidContextReturnsZero) {
  const lob::Context kBad =
      lob::Builder().BallisticCoefficientPsi(kTestBC).Build();
  EXPECT_NE(kBad.error, lob::ErrorT::kNone);
  lob::Output out{};
  out.range = kRangeYd300;
  out.elevation = kInvalidElevation;
  EXPECT_EQ(lob::FastInverse(kBad, &out, 1U), 0U);
}

TEST_F(LobInverseTest, FastInverseArrayOverload) {
  std::array<lob::Output, 2> outs = {ForwardSolve(kRangeYd300),
                                     ForwardSolve(kRangeYd600)};
  const size_t kSize = lob::FastInverse(context, &outs);
  EXPECT_EQ(kSize, 2U);
  EXPECT_TRUE(std::isfinite(outs[0].elevation));
  EXPECT_TRUE(std::isfinite(outs[1].elevation));
}

TEST_F(LobInverseTest, FastInverseSingleOutputOverload) {
  lob::Output out = ForwardSolve(kRangeYd300);
  const size_t kSize = lob::FastInverse(context, &out);
  EXPECT_EQ(kSize, 1U);
  EXPECT_TRUE(std::isfinite(out.elevation));
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

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
TEST_F(LobInverseTest, SolveInverseMatchesFastInverseWithJump) {
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
  for (const uint32_t kRangeFt : {kRangeYd300, kRangeYd1000, 4500U}) {
    lob::Output forward{};
    ASSERT_EQ(lob::Solve(kCtx, kRangeFt, &forward), 1U);
    ASSERT_EQ(lob::FastInverse(kCtx, &forward, 1U), 1U);
    const double kFast = forward.elevation;
    lob::Output inverse{};
    ASSERT_EQ(lob::SolveInverse(kCtx, kRangeFt, &inverse), 1U);
    EXPECT_NEAR(kFast, inverse.elevation, 0.1)
        << "range=" << kRangeFt << " jump=" << kCtx.aerodynamic_jump;
  }
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
