// Copyright (c) 2026  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <string>

#include "testing.hpp"
#include "validation_sensitivity.hpp"

namespace tests {

TEST(SensitivityMath, CentralDifferenceQuadraticIsExact) {
  auto f = [](double x) { return x * x; };
  const DiffResult kR = CentralDifference(f, 3.0, 0.5);
  EXPECT_NEAR(kR.deriv, 6.0, 1e-9);
  EXPECT_DOUBLE_EQ(kR.f_plus, 12.25);
  EXPECT_DOUBLE_EQ(kR.f_minus, 6.25);
}

TEST(SensitivityMath, ConstantFunctionHasZeroDerivative) {
  auto f = [](double) { return 5.0; };
  const DiffResult kR = CentralDifference(f, 3.0, 0.5);
  EXPECT_DOUBLE_EQ(kR.deriv, 0.0);
  EXPECT_FALSE(GenuineCheck(0.0, 0.01, 1, 1));
}

TEST(SensitivityMath, SnapHRespectsIntegerGrid) {
  EXPECT_DOUBLE_EQ(SnapH(10.4, 1.0), 10.0);
  EXPECT_DOUBLE_EQ(SnapH(0.3, 1.0), 1.0);
  EXPECT_DOUBLE_EQ(SnapH(2.5, 0.0), 2.5);
}

TEST(SensitivityMath, SelectHAcceptsLinearFunction) {
  auto f = [](double x) { return 2.0 * x + 1.0; };
  const HSelection kS = SelectH(f, 3.0, 0.5, 0.0);
  ASSERT_TRUE(kS.ok);
  EXPECT_NEAR(kS.deriv, 2.0, 1e-9);
  EXPECT_DOUBLE_EQ(kS.h, 0.5);
}

TEST(SensitivityMath, SelectHFlagsSharpNonlinearity) {
  auto f = [](double x) { return x * x * x; };
  const HSelection kS = SelectH(f, 1.0, 0.5, 0.0);
  EXPECT_FALSE(kS.ok);
}

TEST(SensitivityMath, GenuineNeedsMarginAndSignAgreement) {
  EXPECT_TRUE(GenuineCheck(0.5, 0.01, 1, 1));
  EXPECT_FALSE(GenuineCheck(0.05, 0.01, 1, 1));
  EXPECT_FALSE(GenuineCheck(0.5, 0.01, 1, -1));
}

TEST(SensitivityMath, WrapDeltaNormalizesCrossWarrant) {
  EXPECT_DOUBLE_EQ(WrapDelta180(1.0, 359.0), 2.0);
  EXPECT_DOUBLE_EQ(WrapDelta180(359.0, 1.0), -2.0);
  EXPECT_DOUBLE_EQ(WrapDelta180(10.0, 20.0), -10.0);
}

TEST(SensitivityMath, CannedTableHasExpectedEntries) {
  EXPECT_STREQ(kCannedTable[0].name, "velocity_fps");
  EXPECT_DOUBLE_EQ(kCannedTable[0].h_canned, 10.0);
  EXPECT_DOUBLE_EQ(kCannedTable[0].quantum, 1.0);
}

namespace {
// Reuses the C1-ICAO point from tests::MakeC1IcaoBuilder (testing.hpp).
// Channel convention for all Phase 2 work: extractors are tiny lambdas at
// call sites over const lob::Output&, e.g.
//   [](const lob::Output& o) { return o.elevation; }
// for forward inches, lob::InchToMoa(o.elevation, o.range) for MOA channels.
template <size_t N>
lob::Output SolveChannelAt(const lob::Context& ctx,
                           const std::array<uint32_t, N>& ranges, size_t idx) {
  std::array<lob::Output, N> outs{};
  const size_t kSolved = SolveN(ctx, ranges, &outs);
  EXPECT_EQ(kSolved, N);
  return outs[idx];
}
}  // namespace

TEST(SensitivityPlumbing, VelocityApplierBuildsCleanContexts) {
  const std::array<uint32_t, 2> kRanges = {900U, 1800U};
  lob::Builder base = MakeC1IcaoBuilder();
  for (const double kDv : {-10.0, 0.0, 10.0}) {
    lob::Builder p = base;
    p.InitialVelocityFps(
        static_cast<uint16_t>(std::llround(2800.0 + kDv)));
    const lob::Context kCtx = p.Build();
    ASSERT_EQ(kCtx.error, lob::ErrorT::kNone);
    EXPECT_EQ(kCtx.velocity, static_cast<uint16_t>(2800 + static_cast<int>(kDv)));
  }
  const lob::Context kBase = base.Build();
  const double kE0 = SolveChannelAt(kBase, kRanges, 1).elevation;
  lob::Builder p = base;
  p.InitialVelocityFps(2810U);
  const double kE1 = SolveChannelAt(p.Build(), kRanges, 1).elevation;
  EXPECT_GT(std::fabs(kE1 - kE0), 1.0);
}

TEST(SensitivityPlumbing, WindApplierBuildsCleanContexts) {
  lob::Builder base = MakeC1IcaoBuilder();
  for (const double kDw : {-1.0, 1.0}) {
    lob::Builder p = base;
    p.WindHeadingDeg(90.0).WindSpeedMph(kDw > 0.0 ? kDw : -kDw);
    // NOTE: speed is magnitude-only; direction comes from heading. Negative
    // perturbation at zero baseline is expressed as heading 270 vs 90, see
    // Task 3 smoke test — this sanity check only proves clean builds.
    const lob::Context kCtx = p.Build();
    ASSERT_EQ(kCtx.error, lob::ErrorT::kNone);
  }
}

}  // namespace tests
