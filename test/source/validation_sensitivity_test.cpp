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
  // Measured 0.859 in (drop∝t² predicts ≈0.65); bound 0.5 keeps channel
  // consistency with the Task 3 smoke tests at ~12,000× the noise floor.
  EXPECT_GT(std::fabs(kE1 - kE0), 0.5);
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

TEST(SensitivitySmoke, C1VelocityWindPareto) {
  // Noise floors mirror test/validation/baselines/floors.json cell "C1-ICAO"
  // floors_18_9 (elevation_in 4.15814e-05, elevation_moa 3.97148e-06,
  // time_of_flight_s 6.12488e-08). Deflection floor is exactly 0.0 — the
  // 1e-12 entry is ceilings_18_9, a CI guard, not the noise model; with floor
  // 0.0 any nonzero response is genuine per spec section 9.4, so sign
  // agreement + finiteness carry the signal.
  constexpr double kNoiseElevIn = 4.15814e-05;
  constexpr double kNoiseDeflMoa = 0.0;
  const std::array<uint32_t, 4> kRanges = {300U, 900U, 1800U, 3000U};
  constexpr size_t kIdx1800 = 2;
  constexpr double kV0 = 2800.0;
  auto elev_at_vel = [&](double v) {
    lob::Builder p = MakeC1IcaoBuilder();
    p.InitialVelocityFps(static_cast<uint16_t>(std::llround(v)));
    return SolveChannelAt(p.Build(), kRanges, kIdx1800).elevation;
  };
  const HSelection kSelV = SelectH(elev_at_vel, kV0, 10.0, 1.0);
  ASSERT_TRUE(kSelV.ok);
  const double kHv2 = SnapH(2.0 * kSelV.h, 1.0);
  const DiffResult kDv = CentralDifference(elev_at_vel, kV0, kSelV.h);
  const DiffResult kDv2 = CentralDifference(elev_at_vel, kV0, kHv2);
  const double kRespV = std::fabs(kDv.f_plus - kDv.f_minus);
  const int kSignV = (kDv.f_plus > kDv.f_minus) ? 1 : -1;
  const int kSignV2 = (kDv2.f_plus > kDv2.f_minus) ? 1 : -1;
  EXPECT_EQ(kSignV, kSignV2);
  EXPECT_TRUE(GenuineCheck(kRespV, kNoiseElevIn, kSignV, kSignV2));
  EXPECT_GT(kRespV, 1.0);
  auto out_at_wind = [&](double heading, double speed) {
    lob::Builder p = MakeC1IcaoBuilder();
    p.WindHeadingDeg(heading).WindSpeedMph(speed);
    return SolveChannelAt(p.Build(), kRanges, kIdx1800);
  };
  const lob::Output kWPlus = out_at_wind(90.0, 1.0);
  const lob::Output kWMinus = out_at_wind(270.0, 1.0);
  const lob::Output kWPlus2 = out_at_wind(90.0, 2.0);
  const lob::Output kWMinus2 = out_at_wind(270.0, 2.0);
  const double kDp =
      lob::InchToMoa(kWPlus.deflection, static_cast<double>(kWPlus.range));
  const double kDm =
      lob::InchToMoa(kWMinus.deflection, static_cast<double>(kWMinus.range));
  const double kDp2 =
      lob::InchToMoa(kWPlus2.deflection, static_cast<double>(kWPlus2.range));
  const double kDm2 =
      lob::InchToMoa(kWMinus2.deflection, static_cast<double>(kWMinus2.range));
  ASSERT_TRUE(std::isfinite(kDp));
  ASSERT_TRUE(std::isfinite(kDm));
  ASSERT_TRUE(std::isfinite(kDp2));
  ASSERT_TRUE(std::isfinite(kDm2));
  const double kRespWDefl = std::fabs(kDp - kDm);
  const double kRespWDefl2 = std::fabs(kDp2 - kDm2);
  const int kSignW = (kDp > kDm) ? 1 : -1;
  const int kSignW2 = (kDp2 > kDm2) ? 1 : -1;
  EXPECT_EQ(kSignW, kSignW2);
  EXPECT_TRUE(GenuineCheck(kRespWDefl, kNoiseDeflMoa, kSignW, kSignW2));
  EXPECT_TRUE(GenuineCheck(kRespWDefl2, kNoiseDeflMoa, kSignW, kSignW2));
  const double kRespWElev = std::fabs(kWPlus.elevation - kWMinus.elevation);
  const double kDen = kRespV + kRespWElev;
  ASSERT_GT(kDen, 0.0);
  const double kShareV = kRespV / kDen;
  const double kShareW = kRespWElev / kDen;
  EXPECT_GT(kShareV, 0.9);
  EXPECT_LT(std::fabs(kShareV + kShareW - 1.0), 1e-9);
  // C1 BC is 0.232, so the offline BC canned step is 0.00232 (1%), NOT the
  // table's 0.00425 — Task 5 scales per case. This smoke test does not
  // perturb BC.
}

}  // namespace tests
