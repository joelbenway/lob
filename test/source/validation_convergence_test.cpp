// Copyright (c) 2026  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <string>

#include "lob/lob.hpp"
#include "testing.hpp"
#include "validation_io.hpp"

namespace tests {

TEST(ValidationConvergenceScaffold, C1SolvesAtDefaultStep) {
  const std::array<uint32_t, 4> kRanges = {300U, 900U, 1800U, 3000U};
  std::array<lob::Output, 4> outs{};
  const lob::Context kCtx = BuildAtStep(MakeC1IcaoBuilder(), 36U);
  ASSERT_EQ(kCtx.error, lob::ErrorT::kNone);
  EXPECT_EQ(SolveN(kCtx, kRanges, &outs), kRanges.size());
}

TEST(ValidationMath, ElevMoaDiffUsesSameRange) {
  lob::Output a{};
  lob::Output b{};
  a.range = 900U;
  b.range = 900U;
  a.elevation = 10.0;
  b.elevation = 13.0;
  EXPECT_DOUBLE_EQ(ElevInDiff(a, b), 3.0);
  EXPECT_DOUBLE_EQ(ElevMoaDiff(a, b),
                   lob::InchToMoa(13.0, 900.0) - lob::InchToMoa(10.0, 900.0));
}

TEST(ValidationMath, QuantizedChannelsFloorAtOneLsb) {
  lob::Output a{};
  lob::Output b{};
  a.range = 900U;
  b.range = 900U;
  a.velocity = 2000U;
  b.velocity = 2000U;
  a.energy = 1500U;
  b.energy = 1500U;
  EXPECT_TRUE(IsAtFloor(VelDiff(a, b), kVelFloorFps));
  EXPECT_TRUE(IsAtFloor(EnergyDiff(a, b), kEnergyFloorFtLbs));
}

TEST(ValidationMath, MonotoneAllowsEqualityOnlyAtFloor) {
  EXPECT_TRUE(DecreasesOrAtFloor(0.5, 0.3, 0.01));
  EXPECT_TRUE(DecreasesOrAtFloor(0.005, 0.005, 0.01));
  EXPECT_FALSE(DecreasesOrAtFloor(0.3, 0.5, 0.01));
  EXPECT_FALSE(DecreasesOrAtFloor(0.005, 0.02, 0.01));
}

TEST(ValidationMath, ObservedOrderSecondOrderCase) {
  EXPECT_NEAR(ObservedOrder(0.8, 0.2), 2.0, 1e-9);
  EXPECT_TRUE(std::isnan(ObservedOrder(0.8, 0.0)));
}

TEST(ValidationIo, JsonDoubleHandlesNanAndFormatsFinite) {
  EXPECT_EQ(JsonDouble(std::numeric_limits<double>::quiet_NaN()), "null");
  EXPECT_EQ(JsonDouble(0.0), "0");
  EXPECT_EQ(JsonDouble(-374.359), "-374.359");
}

TEST(ValidationIo, ArtifactSerializesProvenanceAndRungs) {
  ConvergenceArtifact artifact;
  artifact.provenance_lob_version = "0.13.0-test";
  artifact.provenance_git_sha = "deadbee";
  artifact.solver_config = "step_in=36,angle_tol_moa=0.01,density_path=fast";
  artifact.AddRung(36U, -374.0, 0.5);
  artifact.AddRung(18U, -374.2, 0.4);
  const std::string kJson = artifact.ToJson();
  EXPECT_NE(kJson.find("\"lob_version\":\"0.13.0-test\""), std::string::npos);
  EXPECT_NE(kJson.find("\"step_in\":18"), std::string::npos);
  const std::string kCsv = artifact.ToCsv();
  EXPECT_NE(kCsv.find("step_in,elevation_in"), std::string::npos);
  EXPECT_NE(kCsv.find("\n18,-374.2,0.4\n"), std::string::npos);
}

TEST(ValidationIo, JsonEscapeQuotesStrings) {
  EXPECT_EQ(JsonEscape("a\"b\\c"), "a\\\"b\\\\c");
}

namespace {
// Measured 2026-09-27, dev preset, x86_64-linux, C1-ICAO, ranges
// {300,900,1800,3000} ft. Ceilings = worst observed 18→9 delta × ~2 margin,
// rounded up to one significant figure, except deflection (worst 0.0 observed
// 2026-09-27; epsilon guard for cross-platform noise). Source of truth
// mirrored in test/validation/baselines/floors.json (C1-ICAO cell).
constexpr double kCeilElevIn_18_9 = 9e-05;    // worst 4.15814e-05 @3000ft x~2
constexpr double kCeilElevMoa_18_9 = 8e-06;   // worst 3.97148e-06 @3000ft x~2
constexpr double kCeilDeflMoa_18_9 = 1e-12;   // worst 0.0 observed 2026-09-27; epsilon guard for cross-platform noise
constexpr double kCeilTof_18_9 = 2e-07;       // worst 6.12488e-08 @3000ft x~2
}  // namespace

TEST(ValidationConvergenceC1, StepLadderDecreasesWithoutRegression) {
  const std::array<uint32_t, 4> kRanges = {300U, 900U, 1800U, 3000U};
  std::array<lob::Output, 4> outs36{};
  std::array<lob::Output, 4> outs18{};
  std::array<lob::Output, 4> outs9{};
  ASSERT_EQ(SolveN(BuildAtStep(MakeC1IcaoBuilder(), 36U), kRanges, &outs36),
            kRanges.size());
  ASSERT_EQ(SolveN(BuildAtStep(MakeC1IcaoBuilder(), 18U), kRanges, &outs18),
            kRanges.size());
  ASSERT_EQ(SolveN(BuildAtStep(MakeC1IcaoBuilder(), 9U), kRanges, &outs9),
            kRanges.size());
  for (size_t i = 0; i < kRanges.size(); ++i) {
    const double kElevCoarse = ElevInDiff(outs36[i], outs18[i]);
    const double kElevFine = ElevInDiff(outs18[i], outs9[i]);
    EXPECT_TRUE(DecreasesOrAtFloor(kElevCoarse, kElevFine, kElevFloorIn))
        << "range=" << kRanges[i];
    EXPECT_LE(kElevFine, kCeilElevIn_18_9) << "range=" << kRanges[i];
    const double kMoaCoarse = ElevMoaDiff(outs36[i], outs18[i]);
    const double kMoaFine = ElevMoaDiff(outs18[i], outs9[i]);
    EXPECT_TRUE(DecreasesOrAtFloor(kMoaCoarse, kMoaFine, kMoaFloor))
        << "range=" << kRanges[i];
    EXPECT_LE(kMoaFine, kCeilElevMoa_18_9) << "range=" << kRanges[i];
    const double kDeflCoarse = DeflMoaDiff(outs36[i], outs18[i]);
    const double kDeflFine = DeflMoaDiff(outs18[i], outs9[i]);
    EXPECT_TRUE(DecreasesOrAtFloor(kDeflCoarse, kDeflFine, kMoaFloor))
        << "range=" << kRanges[i];
    EXPECT_LE(kDeflFine, kCeilDeflMoa_18_9) << "range=" << kRanges[i];
    EXPECT_TRUE(DecreasesOrAtFloor(VelDiff(outs36[i], outs18[i]),
                                   VelDiff(outs18[i], outs9[i]), kVelFloorFps))
        << "range=" << kRanges[i];
    EXPECT_TRUE(DecreasesOrAtFloor(
        EnergyDiff(outs36[i], outs18[i]), EnergyDiff(outs18[i], outs9[i]),
        kEnergyFloorFtLbs))
        << "range=" << kRanges[i];
    const double kTofCoarse = TofDiff(outs36[i], outs18[i]);
    const double kTofFine = TofDiff(outs18[i], outs9[i]);
    EXPECT_TRUE(DecreasesOrAtFloor(kTofCoarse, kTofFine, kTofFloorSec))
        << "range=" << kRanges[i];
    EXPECT_LE(kTofFine, kCeilTof_18_9) << "range=" << kRanges[i];
  }
}

TEST(ValidationConvergenceC1, InverseLadderDecreasesWithoutRegression) {
  const std::array<uint32_t, 2> kRanges = {900U, 1800U};
  std::array<lob::Output, 2> outs36{};
  std::array<lob::Output, 2> outs18{};
  std::array<lob::Output, 2> outs9{};
  const lob::Context kCtx36 = BuildAtStep(MakeC1IcaoBuilder(), 36U);
  const lob::Context kCtx18 = BuildAtStep(MakeC1IcaoBuilder(), 18U);
  const lob::Context kCtx9 = BuildAtStep(MakeC1IcaoBuilder(), 9U);
  ASSERT_EQ(kCtx36.error, lob::ErrorT::kNone);
  ASSERT_EQ(lob::SolveInverse(kCtx36, kRanges, &outs36), kRanges.size());
  ASSERT_EQ(lob::SolveInverse(kCtx18, kRanges, &outs18), kRanges.size());
  ASSERT_EQ(lob::SolveInverse(kCtx9, kRanges, &outs9), kRanges.size());
  // Fast-branch precondition: forward drop must stay above the 1200-in
  // dynamic-branch switch or this test measures the wrong path.
  std::array<lob::Output, 2> fwd{};
  ASSERT_EQ(SolveN(kCtx36, kRanges, &fwd), kRanges.size());
  for (size_t i = 0; i < kRanges.size(); ++i) {
    EXPECT_GT(fwd[i].elevation, -1200.0) << "range=" << kRanges[i];
  }
  for (size_t i = 0; i < kRanges.size(); ++i) {
    // Inverse outputs are MOA adjustments; forward drop must stay above the
    // 1200-in dynamic-branch switch or this test measures the wrong path.
    EXPECT_TRUE(std::isfinite(outs36[i].elevation));
    const double kCoarse = std::fabs(outs36[i].elevation - outs18[i].elevation);
    const double kFine = std::fabs(outs18[i].elevation - outs9[i].elevation);
    EXPECT_TRUE(DecreasesOrAtFloor(kCoarse, kFine, kMoaFloor))
        << "range=" << kRanges[i];
    EXPECT_LE(kFine, 0.1) << "range=" << kRanges[i];
  }
}

namespace {
inline lob::Builder MakeWindBaseBuilder() {
  // Mirrors WindProfileBuildFixture::ConfiguredBuilder in
  // test/source/lob_wind_profile_test.cpp — keep in sync by review.
  lob::Builder b;
  b.BallisticCoefficientPsi(0.372)
      .BCDragFunction(lob::DragFunctionT::kG1)
      .DiameterInch(0.224)
      .MassGrains(77.0)
      .InitialVelocityFps(2720)
      .ZeroAngleMOA(4.78)
      .OpticHeightInches(2.5);
  return b;
}
}  // namespace

TEST(ValidationConvergenceWind, UniformLadderDecreasesWithoutRegression) {
  const std::array<uint32_t, 3> kRanges = {900U, 1800U, 2700U};
  std::array<lob::Output, 3> outs36{};
  std::array<lob::Output, 3> outs18{};
  std::array<lob::Output, 3> outs9{};
  auto build_uniform = [](uint16_t step) {
    lob::Builder b = MakeWindBaseBuilder();
    b.WindHeading(lob::ClockAngleT::kIII).WindSpeedMph(5.0);
    return BuildAtStep(b, step);
  };
  ASSERT_EQ(SolveN(build_uniform(36U), kRanges, &outs36), kRanges.size());
  ASSERT_EQ(SolveN(build_uniform(18U), kRanges, &outs18), kRanges.size());
  ASSERT_EQ(SolveN(build_uniform(9U), kRanges, &outs9), kRanges.size());
  for (size_t i = 0; i < kRanges.size(); ++i) {
    EXPECT_TRUE(DecreasesOrAtFloor(DeflMoaDiff(outs36[i], outs18[i]),
                                   DeflMoaDiff(outs18[i], outs9[i]), kMoaFloor))
        << "range=" << kRanges[i];
  }
}

TEST(ValidationConvergenceWind, ScaledProfileLadderDecreasesWithoutRegression) {
  const std::array<lob::WindPoint, 2> kTwoPoint = {{
      {0.0, 90.0, 5.0, std::numeric_limits<double>::quiet_NaN()},
      {1500.0, 90.0, 10.0, 6.0},
  }};
  const std::array<uint32_t, 3> kRanges = {900U, 1800U, 2700U};
  std::array<lob::Output, 3> outs36{};
  std::array<lob::Output, 3> outs18{};
  std::array<lob::Output, 3> outs9{};
  auto build_profile = [&kTwoPoint](uint16_t step) {
    lob::Builder b = MakeWindBaseBuilder();
    b.WindProfile(kTwoPoint).WindShearExponent(0.25);
    return BuildAtStep(b, step);
  };
  const lob::Context kProbe = build_profile(36U);
  ASSERT_EQ(kProbe.error, lob::ErrorT::kNone);
  ASSERT_EQ(kProbe.wind_count, 2U);
  ASSERT_EQ(SolveN(build_profile(36U), kRanges, &outs36), kRanges.size());
  ASSERT_EQ(SolveN(build_profile(18U), kRanges, &outs18), kRanges.size());
  ASSERT_EQ(SolveN(build_profile(9U), kRanges, &outs9), kRanges.size());
  for (size_t i = 0; i < kRanges.size(); ++i) {
    EXPECT_TRUE(DecreasesOrAtFloor(DeflMoaDiff(outs36[i], outs18[i]),
                                   DeflMoaDiff(outs18[i], outs9[i]), kMoaFloor))
        << "range=" << kRanges[i];
  }
}

TEST(ValidationFullLadder, C1StepLadderToOneInchWritesArtifact) {
  const char* kGate = std::getenv("LOB_FULL_LADDER");
  if (kGate == nullptr || std::string(kGate) != "1") {
    GTEST_SKIP() << "offline only: set LOB_FULL_LADDER=1";
  }
  // 1000-ft station is off-grid at the 36-in rung (1000 % 3 != 0), covering
  // the clamp path CI ranges (all multiples of 3 ft) never exercise.
  const std::array<uint32_t, 5> kRanges = {300U, 900U, 1000U, 1800U, 3000U};
  const std::array<uint16_t, 6> kRungs = {36U, 18U, 9U, 4U, 2U, 1U};
  std::array<std::array<lob::Output, 5>, 6> ladders{};
  for (size_t r = 0; r < kRungs.size(); ++r) {
    ASSERT_EQ(SolveN(BuildAtStep(MakeC1IcaoBuilder(), kRungs[r]), kRanges,
                     &ladders[r]),
              kRanges.size())
        << "rung=" << kRungs[r];
  }
  // Monotone + observed-order plausibility (Heun theory: p ≈ 2; allow
  // [1, 3] for knot/wind-joint degradation) on the 1800-ft elevation
  // channel, finest two pairs.
  const double kD1 =
      ElevInDiff(ladders[1][3], ladders[2][3]);  // 18->9
  const double kD2 =
      ElevInDiff(ladders[2][3], ladders[3][3]);  // 9->4
  const double kD3 =
      ElevInDiff(ladders[3][3], ladders[4][3]);  // 4->2
  const double kD4 =
      ElevInDiff(ladders[4][3], ladders[5][3]);  // 2->1
  EXPECT_TRUE(DecreasesOrAtFloor(kD1, kD2, kElevFloorIn));
  EXPECT_TRUE(DecreasesOrAtFloor(kD2, kD3, kElevFloorIn));
  EXPECT_TRUE(DecreasesOrAtFloor(kD3, kD4, kElevFloorIn));
  const double kOrder = ObservedOrder(kD3, kD4);
  if (!IsAtFloor(kD3, kElevFloorIn) && !IsAtFloor(kD4, kElevFloorIn)) {
    EXPECT_GE(kOrder, 1.0);
    EXPECT_LE(kOrder, 3.0);
  }
  ConvergenceArtifact artifact;
  artifact.provenance_lob_version = lob::Version();
  artifact.provenance_git_sha = LOB_GIT_SHA;
  artifact.solver_config = "step_ladder_in=36,18,9,4,2,1,angle_tol_moa=0.01,"
                           "density_path=fast,ranges_ft=300,900,1000,1800,3000";
  for (size_t r = 0; r < kRungs.size(); ++r) {
    const double kDelta =
        (r == 0) ? 0.0 : ElevInDiff(ladders[r - 1][3], ladders[r][3]);
    artifact.AddRung(kRungs[r], ladders[r][3].elevation, kDelta);
  }
  ASSERT_TRUE(artifact.WriteFiles(LOB_VALIDATION_DIR, "convergence_C1"));
}

}  // namespace tests
