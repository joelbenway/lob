// Copyright (c) 2026  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <sstream>
#include <vector>

#include "lob/lob.hpp"
#include "testing.hpp"

namespace tests {

// Band edges are conventional-not-prescribed: order-of-magnitude reporting
// bands from spec §13.2's >>/approx/<</ language, not a project significance
// threshold. No application decision may cite them without stating its own k.
constexpr double kSigClear = 10.0;
constexpr double kSigMarginalLo = 0.1;

enum class SigBand : std::uint8_t {
  kDistinguishable,
  kMarginal,
  kIndistinguishable
};

// tests:: API is plan-mandated for Tasks 2-3; single-TU use is by design.
// NOLINTNEXTLINE(misc-use-internal-linkage)
inline std::vector<double> EffectDelta(
    const std::vector<double>& with_effect,
    const std::vector<double>& without_effect) {
  std::vector<double> delta(with_effect.size(), 0.0);
  for (std::size_t i = 0; i < with_effect.size(); ++i) {
    delta.at(i) = with_effect.at(i) - without_effect.at(i);
  }
  return delta;
}

// tests:: API is plan-mandated for Tasks 2-3; single-TU use is by design.
// NOLINTNEXTLINE(misc-use-internal-linkage)
inline double UTotal(double u_num, double granularity,
                     double mc_sigma =
                         std::numeric_limits<double>::quiet_NaN()) {
  double sum = (u_num * u_num) + (granularity * granularity);
  if (!std::isnan(mc_sigma)) {
    sum += mc_sigma * mc_sigma;
  }
  return std::sqrt(sum);
}

// tests:: API is plan-mandated for Tasks 2-3; single-TU use is by design.
// NOLINTNEXTLINE(misc-use-internal-linkage)
inline double RSig(double delta, double u_total) {
  if (!(u_total > 0.0)) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  return std::fabs(delta) / u_total;
}

// tests:: API is plan-mandated for Tasks 2-3; single-TU use is by design.
// NOLINTNEXTLINE(misc-use-internal-linkage)
inline SigBand ClassifySigBand(double r_sig) {
  if (r_sig > kSigClear) {
    return SigBand::kDistinguishable;
  }
  if (r_sig >= kSigMarginalLo) {
    return SigBand::kMarginal;
  }
  return SigBand::kIndistinguishable;
}

}  // namespace tests

TEST(SignalMath, DeltaOfKnownVectorsIsExact) {
  const std::vector<double> kWith = {10.0, -89.70};
  const std::vector<double> kWithout = {10.0, -89.73};
  const std::vector<double> kDelta = tests::EffectDelta(kWith, kWithout);
  ASSERT_EQ(kDelta.size(), 2U);
  EXPECT_DOUBLE_EQ(kDelta.at(0), 0.0);
  EXPECT_NEAR(kDelta.at(1), 0.03, 1e-12);
}

TEST(SignalMath, UTotalFollowsHypotenuse) {
  EXPECT_DOUBLE_EQ(tests::UTotal(3.0, 4.0), 5.0);
  EXPECT_DOUBLE_EQ(
      tests::UTotal(3.0, 4.0, std::numeric_limits<double>::quiet_NaN()), 5.0);
  EXPECT_DOUBLE_EQ(tests::UTotal(3.0, 4.0, 0.0), 5.0);
  EXPECT_DOUBLE_EQ(tests::UTotal(0.0, 0.0, 5.0), 5.0);
}

TEST(SignalMath, RSigGuardsZeroYardstick) {
  EXPECT_DOUBLE_EQ(tests::RSig(10.0, 5.0), 2.0);
  EXPECT_TRUE(std::isnan(tests::RSig(1.0, 0.0)));
  EXPECT_TRUE(std::isnan(tests::RSig(1.0, -1.0)));
  EXPECT_TRUE(std::isnan(tests::RSig(0.0, 0.0)));
  EXPECT_TRUE(
      std::isnan(tests::RSig(1.0, std::numeric_limits<double>::quiet_NaN())));
}

TEST(SignalMath, BandsAtTenAndTenth) {
  EXPECT_EQ(tests::ClassifySigBand(11.0), tests::SigBand::kDistinguishable);
  EXPECT_EQ(tests::ClassifySigBand(1.0), tests::SigBand::kMarginal);
  EXPECT_EQ(tests::ClassifySigBand(0.01),
            tests::SigBand::kIndistinguishable);
  EXPECT_EQ(tests::ClassifySigBand(10.0), tests::SigBand::kMarginal);
  EXPECT_EQ(tests::ClassifySigBand(0.1), tests::SigBand::kMarginal);
  EXPECT_EQ(tests::ClassifySigBand(std::numeric_limits<double>::quiet_NaN()),
            tests::SigBand::kIndistinguishable);
}

namespace {

// Band name for RECORD lines: physics outcomes are logged with their band,
// never asserted (record-don't-tripwire).
inline const char* SigBandName(tests::SigBand band) {
  switch (band) {
    case tests::SigBand::kDistinguishable:
      return "distinguishable";
    case tests::SigBand::kMarginal:
      return "marginal";
    case tests::SigBand::kIndistinguishable:
    default:
      return "indistinguishable";
  }
}

// RECORD-don't-tripwire logger: formats Δ + R_sig + band as a SCOPED_TRACE
// so the physics outcome is recorded without tripping the gate.
inline void RecordSignal(const char* label, uint32_t range_ft, double delta,
                         double u_num, double granularity) {
  const double kR = tests::RSig(delta, tests::UTotal(u_num, granularity));
  std::ostringstream msg;
  msg << label << " range_ft=" << range_ft << " delta=" << delta
      << " r_sig=" << kR << " band=" << SigBandName(tests::ClassifySigBand(kR));
  SCOPED_TRACE(msg.str());
}

// C9-tail-style context: mirrors the C9-dynamic-tail survey builder in
// test/source/validation_convergence_test.cpp. Default 36-in step.
inline lob::Context BuildTailContext() {
  constexpr double kBcPsi = 0.436;
  constexpr uint16_t kVelocityFps = 3100U;
  constexpr double kZeroAngleMoa = 6.11;
  return lob::Builder()
      .BallisticCoefficientPsi(kBcPsi)
      .InitialVelocityFps(kVelocityFps)
      .ZeroAngleMOA(kZeroAngleMoa)
      .Build();
}

// Path-engagement precondition (Phase 1 Task 5 pattern): fail loud unless
// the long-tail drop sits on the lapse-scaled branch.
inline void ExpectTailDropEngaged(const std::array<lob::Output, 3>& fwd,
                                  const std::array<uint32_t, 3>& ranges) {
  for (size_t i = 0; i < ranges.size(); ++i) {
    EXPECT_LT(fwd.at(i).elevation, -1200.0) << "range=" << ranges.at(i);
  }
}

// Forward-Solve vs SolveInverse-derived elevation outcome. Mechanism (both
// solves complete) is asserted by the caller; the outcome here is RECORDED,
// never tripwired.
inline void RecordLapseVsInverse(const std::array<lob::Output, 3>& fwd,
                                 const std::array<lob::Output, 3>& inv,
                                 const std::array<uint32_t, 3>& ranges) {
  // Yardstick cross-link: u_num is floors.json C9-dynamic-tail
  // floors_18_9.elevation_moa; granularity is testing.hpp kMoaFloor.
  constexpr double kUNumMoa = 6.535115801e-06;
  for (size_t i = 0; i < ranges.size(); ++i) {
    ASSERT_TRUE(std::isfinite(fwd.at(i).elevation));
    ASSERT_TRUE(std::isfinite(inv.at(i).elevation));
    RecordSignal("lapse", ranges.at(i),
                 fwd.at(i).elevation - inv.at(i).elevation, kUNumMoa,
                 tests::kMoaFloor);
  }
}

// C8-Litz survey builder mirrored exactly — see ValidationFloorSurvey in
// test/source/validation_convergence_test.cpp. Default 36-in step.
inline lob::Context BuildLitzContext(double wind_mph) {
  constexpr double kBcPsi = 0.436;
  constexpr uint16_t kVelocityFps = 3100U;
  constexpr double kZeroAngleMoa = 6.11;
  constexpr double kDiameterInch = 0.308;
  constexpr double kLengthInch = 1.215;
  constexpr double kMassGrains = 168.0;
  constexpr double kTwistIn = 10.0;
  lob::Builder b;
  b.BallisticCoefficientPsi(kBcPsi)
      .InitialVelocityFps(kVelocityFps)
      .ZeroAngleMOA(kZeroAngleMoa)
      .DiameterInch(kDiameterInch)
      .LengthInch(kLengthInch)
      .MassGrains(kMassGrains)
      .TwistInchesPerTurn(kTwistIn)
      .WindHeading(lob::ClockAngleT::kIII)
      .WindSpeedMph(wind_mph);
  return b.Build();
}

// C5-uniform wind-base builder: mirrors MakeWindBaseBuilder in
// test/source/validation_convergence_test.cpp. Default 36-in step.
inline lob::Builder BuildC5WindBase() {
  constexpr double kBcPsi = 0.372;
  constexpr double kDiameterInch = 0.224;
  constexpr double kMassGrains = 77.0;
  constexpr int kVelocityFps = 2720;
  constexpr double kZeroAngleMoa = 4.78;
  constexpr double kOpticHeightIn = 2.5;
  lob::Builder b;
  b.BallisticCoefficientPsi(kBcPsi)
      .BCDragFunction(lob::DragFunctionT::kG1)
      .DiameterInch(kDiameterInch)
      .MassGrains(kMassGrains)
      .InitialVelocityFps(kVelocityFps)
      .ZeroAngleMOA(kZeroAngleMoa)
      .OpticHeightInches(kOpticHeightIn);
  return b;
}

inline lob::Context BuildC5Profile() {
  // One-station 5 mph crosswind profile; the array must outlive Build()
  // (WindProfile borrows) — Build copies synchronously below, so it does.
  const std::array<lob::WindPoint, 1> kOne = {{
      {0.0, 90.0, 5.0, std::numeric_limits<double>::quiet_NaN()},
  }};
  lob::Builder b = BuildC5WindBase();
  return b.WindProfile(kOne).Build();
}

inline lob::Context BuildC5Uniform() {
  // WindHeadingDeg(90.0), not kIII: this is the node-proven pair
  // (WindProfileBuildFixture.SinglePointEqualsUniform in
  // test/source/lob_wind_profile_test.cpp); kIII stores 2π rad whose sine
  // is 1 ulp off zero.
  constexpr double kHeadingDeg = 90.0;
  constexpr double kWindMph = 5.0;
  lob::Builder b = BuildC5WindBase();
  return b.WindHeadingDeg(kHeadingDeg).WindSpeedMph(kWindMph).Build();
}

// Bit-identity oracle core (effect 15): every range must agree to the last
// ulp on the wind-affected channels.
inline void ExpectIdentityAtAllRanges(
    const std::array<lob::Output, 3>& profile_outs,
    const std::array<lob::Output, 3>& uniform_outs) {
  for (size_t i = 0; i < profile_outs.size(); ++i) {
    EXPECT_DOUBLE_EQ(profile_outs.at(i).elevation,
                     uniform_outs.at(i).elevation);
    EXPECT_DOUBLE_EQ(profile_outs.at(i).deflection,
                     uniform_outs.at(i).deflection);
  }
}

}  // namespace

TEST(SignalSmoke, LapsePathConverged) {
  // Hermetic in-memory solves at the default 36-in step.
  const std::array<uint32_t, 3> kRanges = {6000U, 7500U, 9000U};
  const lob::Context kCtx = BuildTailContext();
  ASSERT_EQ(kCtx.error, lob::ErrorT::kNone);
  std::array<lob::Output, 3> fwd{};
  ASSERT_EQ(lob::Solve(kCtx, kRanges, &fwd), kRanges.size());
  ExpectTailDropEngaged(fwd, kRanges);
  std::array<lob::Output, 3> inv{};
  ASSERT_EQ(lob::SolveInverse(kCtx, kRanges, &inv), kRanges.size());
  // FastInverse converts the forward outputs in place
  // (SolveInverseMatchesFastInverseWithJump pattern in
  // test/source/lob_inverse_test.cpp).
  ASSERT_EQ(lob::FastInverse(kCtx, &fwd), kRanges.size());
  RecordLapseVsInverse(fwd, inv, kRanges);
}

TEST(SignalSmoke, JumpIsLive) {
  // Hermetic in-memory solves at the default 36-in step.
  const std::array<uint32_t, 3> kRanges = {900U, 1800U, 2700U};
  constexpr double kWindMph = 10.0;
  // Yardstick cross-link: u_num is floors.json C8-Litz
  // floors_18_9.elevation_in; granularity is testing.hpp kElevFloorIn.
  constexpr double kUNumIn = 2.799664696e-05;
  const lob::Context kWindy = BuildLitzContext(kWindMph);
  ASSERT_EQ(kWindy.error, lob::ErrorT::kNone);
  // Tripwire: the Litz jump path must be live — fail loud if it goes quiet.
  EXPECT_TRUE(std::fabs(kWindy.aerodynamic_jump) > 0.0);
  const lob::Context kCalm = BuildLitzContext(0.0);
  ASSERT_EQ(kCalm.error, lob::ErrorT::kNone);
  // Zero crosswind ⇒ zero jump by the BuildLitzAerodynamicJump early-out.
  EXPECT_DOUBLE_EQ(kCalm.aerodynamic_jump, 0.0);
  std::array<lob::Output, 3> windy_outs{};
  std::array<lob::Output, 3> calm_outs{};
  ASSERT_EQ(lob::Solve(kWindy, kRanges, &windy_outs), kRanges.size());
  ASSERT_EQ(lob::Solve(kCalm, kRanges, &calm_outs), kRanges.size());
  for (size_t i = 0; i < kRanges.size(); ++i) {
    // Outcome RECORDED, never tripwired.
    RecordSignal("jump", kRanges.at(i),
                 windy_outs.at(i).elevation - calm_outs.at(i).elevation,
                 kUNumIn, tests::kElevFloorIn);
  }
}

TEST(SignalSmoke, SinglePointEqualsUniform) {
  // Effect 15 bit-identity oracle (THIS test trips): the same 5 mph
  // crosswind on the C5-uniform wind base, set once via a one-station
  // profile and once via uniform setters. Hermetic in-memory solves at the
  // default 36-in step.
  const lob::Context kProfile = BuildC5Profile();
  const lob::Context kUniform = BuildC5Uniform();
  ASSERT_EQ(kProfile.error, lob::ErrorT::kNone);
  ASSERT_EQ(kUniform.error, lob::ErrorT::kNone);
  const std::array<uint32_t, 3> kRanges = {900U, 1800U, 2700U};
  std::array<lob::Output, 3> profile_outs{};
  std::array<lob::Output, 3> uniform_outs{};
  ASSERT_EQ(lob::Solve(kProfile, kRanges, &profile_outs), kRanges.size());
  ASSERT_EQ(lob::Solve(kUniform, kRanges, &uniform_outs), kRanges.size());
  ExpectIdentityAtAllRanges(profile_outs, uniform_outs);
}
