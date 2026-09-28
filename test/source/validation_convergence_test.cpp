// Copyright (c) 2026  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
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
  constexpr uint32_t kRangeFt = 900U;
  constexpr double kElevLowIn = 10.0;
  constexpr double kElevHighIn = 13.0;
  lob::Output a{};
  lob::Output b{};
  a.range = kRangeFt;
  b.range = kRangeFt;
  a.elevation = kElevLowIn;
  b.elevation = kElevHighIn;
  EXPECT_DOUBLE_EQ(ElevInDiff(a, b), 3.0);
  EXPECT_DOUBLE_EQ(ElevMoaDiff(a, b),
                   lob::InchToMoa(13.0, 900.0) - lob::InchToMoa(10.0, 900.0));
}

TEST(ValidationMath, QuantizedChannelsFloorAtOneLsb) {
  constexpr uint32_t kRangeFt = 900U;
  constexpr uint16_t kVelocityFps = 2000U;
  constexpr uint32_t kEnergyFtLbs = 1500U;
  lob::Output a{};
  lob::Output b{};
  a.range = kRangeFt;
  b.range = kRangeFt;
  a.velocity = kVelocityFps;
  b.velocity = kVelocityFps;
  a.energy = kEnergyFtLbs;
  b.energy = kEnergyFtLbs;
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
  constexpr uint32_t kStepCoarseIn = 36U;
  constexpr uint32_t kStepFineIn = 18U;
  constexpr double kElevCoarseIn = -374.0;
  constexpr double kElevFineIn = -374.2;
  constexpr double kDeltaCoarseIn = 0.5;
  constexpr double kDeltaFineIn = 0.4;
  ConvergenceArtifact artifact;
  artifact.provenance_lob_version = "0.13.0-test";
  artifact.provenance_git_sha = "deadbee";
  artifact.solver_config = "step_in=36,angle_tol_moa=0.01,density_path=fast";
  artifact.AddRung(kStepCoarseIn, kElevCoarseIn, kDeltaCoarseIn);
  artifact.AddRung(kStepFineIn, kElevFineIn, kDeltaFineIn);
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
constexpr double kCeilElevIn18To9 = 9e-05;   // worst 4.15814e-05 @3000ft x~2
constexpr double kCeilElevMoa18To9 = 8e-06;  // worst 3.97148e-06 @3000ft x~2
constexpr double kCeilDeflMoa18To9 =
    1e-12;  // worst 0.0 observed 2026-09-27; epsilon guard for cross-platform
            // noise
constexpr double kCeilTof18To9 = 2e-07;  // worst 6.12488e-08 @3000ft x~2
// Print precision for machine-readable SURVEY lines: %.10g semantics.
constexpr int kSurveyPrecisionDigits = 10;
}  // namespace

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
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
    const double kElevCoarse = ElevInDiff(outs36.at(i), outs18.at(i));
    const double kElevFine = ElevInDiff(outs18.at(i), outs9.at(i));
    EXPECT_TRUE(DecreasesOrAtFloor(kElevCoarse, kElevFine, kElevFloorIn))
        << "range=" << kRanges.at(i);
    EXPECT_LE(kElevFine, kCeilElevIn18To9) << "range=" << kRanges.at(i);
    const double kMoaCoarse = ElevMoaDiff(outs36.at(i), outs18.at(i));
    const double kMoaFine = ElevMoaDiff(outs18.at(i), outs9.at(i));
    EXPECT_TRUE(DecreasesOrAtFloor(kMoaCoarse, kMoaFine, kMoaFloor))
        << "range=" << kRanges.at(i);
    EXPECT_LE(kMoaFine, kCeilElevMoa18To9) << "range=" << kRanges.at(i);
    const double kDeflCoarse = DeflMoaDiff(outs36.at(i), outs18.at(i));
    const double kDeflFine = DeflMoaDiff(outs18.at(i), outs9.at(i));
    EXPECT_TRUE(DecreasesOrAtFloor(kDeflCoarse, kDeflFine, kMoaFloor))
        << "range=" << kRanges.at(i);
    EXPECT_LE(kDeflFine, kCeilDeflMoa18To9) << "range=" << kRanges.at(i);
    EXPECT_TRUE(DecreasesOrAtFloor(VelDiff(outs36.at(i), outs18.at(i)),
                                   VelDiff(outs18.at(i), outs9.at(i)),
                                   kVelFloorFps))
        << "range=" << kRanges.at(i);
    EXPECT_TRUE(DecreasesOrAtFloor(EnergyDiff(outs36.at(i), outs18.at(i)),
                                   EnergyDiff(outs18.at(i), outs9.at(i)),
                                   kEnergyFloorFtLbs))
        << "range=" << kRanges.at(i);
    const double kTofCoarse = TofDiff(outs36.at(i), outs18.at(i));
    const double kTofFine = TofDiff(outs18.at(i), outs9.at(i));
    EXPECT_TRUE(DecreasesOrAtFloor(kTofCoarse, kTofFine, kTofFloorSec))
        << "range=" << kRanges.at(i);
    EXPECT_LE(kTofFine, kCeilTof18To9) << "range=" << kRanges.at(i);
  }
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
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
    EXPECT_GT(fwd.at(i).elevation, -1200.0) << "range=" << kRanges.at(i);
  }
  for (size_t i = 0; i < kRanges.size(); ++i) {
    // Inverse outputs are MOA adjustments; forward drop must stay above the
    // 1200-in dynamic-branch switch or this test measures the wrong path.
    EXPECT_TRUE(std::isfinite(outs36.at(i).elevation));
    const double kCoarse =
        std::fabs(outs36.at(i).elevation - outs18.at(i).elevation);
    const double kFine =
        std::fabs(outs18.at(i).elevation - outs9.at(i).elevation);
    EXPECT_TRUE(DecreasesOrAtFloor(kCoarse, kFine, kMoaFloor))
        << "range=" << kRanges.at(i);
    EXPECT_LE(kFine, 0.1) << "range=" << kRanges.at(i);
  }
}

namespace {
inline lob::Builder MakeWindBaseBuilder() {
  // Mirrors WindProfileBuildFixture::ConfiguredBuilder in
  // test/source/lob_wind_profile_test.cpp — keep in sync by review.
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
}  // namespace

TEST(ValidationConvergenceWind, UniformLadderDecreasesWithoutRegression) {
  const std::array<uint32_t, 3> kRanges = {900U, 1800U, 2700U};
  std::array<lob::Output, 3> outs36{};
  std::array<lob::Output, 3> outs18{};
  std::array<lob::Output, 3> outs9{};
  auto build_uniform = [](uint16_t step) {
    constexpr double kWindSpeedMph = 5.0;
    lob::Builder b = MakeWindBaseBuilder();
    b.WindHeading(lob::ClockAngleT::kIII).WindSpeedMph(kWindSpeedMph);
    return BuildAtStep(b, step);
  };
  ASSERT_EQ(SolveN(build_uniform(36U), kRanges, &outs36), kRanges.size());
  ASSERT_EQ(SolveN(build_uniform(18U), kRanges, &outs18), kRanges.size());
  ASSERT_EQ(SolveN(build_uniform(9U), kRanges, &outs9), kRanges.size());
  for (size_t i = 0; i < kRanges.size(); ++i) {
    EXPECT_TRUE(DecreasesOrAtFloor(DeflMoaDiff(outs36.at(i), outs18.at(i)),
                                   DeflMoaDiff(outs18.at(i), outs9.at(i)),
                                   kMoaFloor))
        << "range=" << kRanges.at(i);
  }
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
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
    constexpr double kShearExponent = 0.25;
    lob::Builder b = MakeWindBaseBuilder();
    b.WindProfile(kTwoPoint).WindShearExponent(kShearExponent);
    return BuildAtStep(b, step);
  };
  const lob::Context kProbe = build_profile(36U);
  ASSERT_EQ(kProbe.error, lob::ErrorT::kNone);
  ASSERT_EQ(kProbe.wind_count, 2U);
  ASSERT_EQ(SolveN(build_profile(36U), kRanges, &outs36), kRanges.size());
  ASSERT_EQ(SolveN(build_profile(18U), kRanges, &outs18), kRanges.size());
  ASSERT_EQ(SolveN(build_profile(9U), kRanges, &outs9), kRanges.size());
  for (size_t i = 0; i < kRanges.size(); ++i) {
    EXPECT_TRUE(DecreasesOrAtFloor(DeflMoaDiff(outs36.at(i), outs18.at(i)),
                                   DeflMoaDiff(outs18.at(i), outs9.at(i)),
                                   kMoaFloor))
        << "range=" << kRanges.at(i);
  }
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
TEST(ValidationFullLadder, C1StepLadderToOneInchWritesArtifact) {
  // single-threaded gtest; env gates select offline drivers
  // NOLINTNEXTLINE(concurrency-mt-unsafe)
  const char* gate = std::getenv("LOB_FULL_LADDER");
  if (gate == nullptr || std::string(gate) != "1") {
    GTEST_SKIP() << "offline only: set LOB_FULL_LADDER=1";
  }
  // 1000-ft station is off-grid at the 36-in rung (1000 % 3 != 0), covering
  // the clamp path CI ranges (all multiples of 3 ft) never exercise.
  constexpr size_t kNumRanges = 5;
  constexpr size_t kNumRungs = 6;
  const std::array<uint32_t, kNumRanges> kRanges = {300U, 900U, 1000U, 1800U,
                                                    3000U};
  const std::array<uint16_t, kNumRungs> kRungs = {36U, 18U, 9U, 4U, 2U, 1U};
  std::array<std::array<lob::Output, kNumRanges>, kNumRungs> ladders{};
  for (size_t ri = 0; ri < kRungs.size(); ++ri) {
    ASSERT_EQ(SolveN(BuildAtStep(MakeC1IcaoBuilder(), kRungs.at(ri)), kRanges,
                     &ladders.at(ri)),
              kRanges.size())
        << "rung=" << kRungs.at(ri);
  }
  // Monotone + observed-order plausibility (Heun theory: p ≈ 2; allow
  // [1, 3] for knot/wind-joint degradation) on the 1800-ft elevation
  // channel, finest two pairs.
  const double kD1 = ElevInDiff(ladders[1][3], ladders[2][3]);  // 18->9
  const double kD2 = ElevInDiff(ladders[2][3], ladders[3][3]);  // 9->4
  const double kD3 = ElevInDiff(ladders[3][3], ladders[4][3]);  // 4->2
  const double kD4 = ElevInDiff(ladders[4][3], ladders[5][3]);  // 2->1
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
  artifact.solver_config =
      "step_ladder_in=36,18,9,4,2,1,angle_tol_moa=0.01,"
      "density_path=fast,ranges_ft=300,900,1000,1800,3000";
  for (size_t ri = 0; ri < kRungs.size(); ++ri) {
    const double kDelta =
        (ri == 0) ? 0.0
                  : ElevInDiff(ladders.at(ri - 1).at(3), ladders.at(ri).at(3));
    artifact.AddRung(kRungs.at(ri), ladders.at(ri).at(3).elevation, kDelta);
  }
  ASSERT_TRUE(artifact.WriteFiles(LOB_VALIDATION_DIR, "convergence_C1"));
}

// Documented re-measurement procedure for test/validation/baselines/floors.json
// cells C5-uniform, C6-scaled, C8-Litz, C9-dynamic-tail. Hermetic: prints
// machine-readable SURVEY lines to stdout, no file I/O. Transcription into
// floors.json is by hand. Offline only: set LOB_FLOOR_SURVEY=1.
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
TEST(ValidationFloorSurvey, SurveyCells) {
  // single-threaded gtest; env gates select offline drivers
  // NOLINTNEXTLINE(concurrency-mt-unsafe)
  const char* gate = std::getenv("LOB_FLOOR_SURVEY");
  if (gate == nullptr || std::string(gate) != "1") {
    GTEST_SKIP() << "offline only: set LOB_FLOOR_SURVEY=1";
  }

  // C5-uniform: wind-base builder + kIII 5 mph, ranges {900, 1800, 2700} ft.
  {
    const std::array<uint32_t, 3> kRanges = {900U, 1800U, 2700U};
    std::array<lob::Output, 3> outs36{};
    std::array<lob::Output, 3> outs18{};
    std::array<lob::Output, 3> outs9{};
    auto build_uniform = [](uint16_t step) {
      constexpr double kWindSpeedMph = 5.0;
      lob::Builder b = MakeWindBaseBuilder();
      b.WindHeading(lob::ClockAngleT::kIII).WindSpeedMph(kWindSpeedMph);
      return BuildAtStep(b, step);
    };
    ASSERT_EQ(SolveN(build_uniform(36U), kRanges, &outs36), kRanges.size());
    ASSERT_EQ(SolveN(build_uniform(18U), kRanges, &outs18), kRanges.size());
    ASSERT_EQ(SolveN(build_uniform(9U), kRanges, &outs9), kRanges.size());
    double w_elev_in = 0.0;
    double w_elev_moa = 0.0;
    double w_defl_moa = 0.0;
    double w_tof = 0.0;
    double w_vel = 0.0;
    double w_energy = 0.0;
    for (size_t i = 0; i < kRanges.size(); ++i) {
      const double kElevIn = ElevInDiff(outs18.at(i), outs9.at(i));
      w_elev_in = std::max(w_elev_in, kElevIn);
      const double kElevMoa = ElevMoaDiff(outs18.at(i), outs9.at(i));
      w_elev_moa = std::max(w_elev_moa, kElevMoa);
      const double kDeflMoa = DeflMoaDiff(outs18.at(i), outs9.at(i));
      w_defl_moa = std::max(w_defl_moa, kDeflMoa);
      const double kTof = TofDiff(outs18.at(i), outs9.at(i));
      w_tof = std::max(w_tof, kTof);
      const double kVel = VelDiff(outs18.at(i), outs9.at(i));
      w_vel = std::max(w_vel, kVel);
      const double kEnergy = EnergyDiff(outs18.at(i), outs9.at(i));
      w_energy = std::max(w_energy, kEnergy);
    }
    std::cout << "SURVEY C5-uniform elevation_in worst18_9="
              << std::setprecision(kSurveyPrecisionDigits) << w_elev_in << "\n";
    std::cout << "SURVEY C5-uniform elevation_moa worst18_9="
              << std::setprecision(kSurveyPrecisionDigits) << w_elev_moa
              << "\n";
    std::cout << "SURVEY C5-uniform deflection_moa worst18_9="
              << std::setprecision(kSurveyPrecisionDigits) << w_defl_moa
              << "\n";
    std::cout << "SURVEY C5-uniform time_of_flight_s worst18_9="
              << std::setprecision(kSurveyPrecisionDigits) << w_tof << "\n";
    std::cout << "SURVEY C5-uniform velocity_fps worst18_9="
              << std::setprecision(kSurveyPrecisionDigits) << w_vel << "\n";
    std::cout << "SURVEY C5-uniform energy_ft_lbf worst18_9="
              << std::setprecision(kSurveyPrecisionDigits) << w_energy << "\n";
  }

  // C6-scaled: wind-base + two-point 90-degree profile with shear 0.25.
  {
    const std::array<lob::WindPoint, 2> kTwoPoint = {{
        {0.0, 90.0, 5.0, std::numeric_limits<double>::quiet_NaN()},
        {1500.0, 90.0, 10.0, 6.0},
    }};
    const std::array<uint32_t, 3> kRanges = {900U, 1800U, 2700U};
    std::array<lob::Output, 3> outs36{};
    std::array<lob::Output, 3> outs18{};
    std::array<lob::Output, 3> outs9{};
    auto build_profile = [&kTwoPoint](uint16_t step) {
      constexpr double kShearExponent = 0.25;
      lob::Builder b = MakeWindBaseBuilder();
      b.WindProfile(kTwoPoint).WindShearExponent(kShearExponent);
      return BuildAtStep(b, step);
    };
    const lob::Context kProbe = build_profile(36U);
    ASSERT_EQ(kProbe.error, lob::ErrorT::kNone);
    ASSERT_EQ(kProbe.wind_count, 2U);
    ASSERT_EQ(SolveN(build_profile(36U), kRanges, &outs36), kRanges.size());
    ASSERT_EQ(SolveN(build_profile(18U), kRanges, &outs18), kRanges.size());
    ASSERT_EQ(SolveN(build_profile(9U), kRanges, &outs9), kRanges.size());
    double w_elev_in = 0.0;
    double w_elev_moa = 0.0;
    double w_defl_moa = 0.0;
    double w_tof = 0.0;
    double w_vel = 0.0;
    double w_energy = 0.0;
    for (size_t i = 0; i < kRanges.size(); ++i) {
      const double kElevIn = ElevInDiff(outs18.at(i), outs9.at(i));
      w_elev_in = std::max(w_elev_in, kElevIn);
      const double kElevMoa = ElevMoaDiff(outs18.at(i), outs9.at(i));
      w_elev_moa = std::max(w_elev_moa, kElevMoa);
      const double kDeflMoa = DeflMoaDiff(outs18.at(i), outs9.at(i));
      w_defl_moa = std::max(w_defl_moa, kDeflMoa);
      const double kTof = TofDiff(outs18.at(i), outs9.at(i));
      w_tof = std::max(w_tof, kTof);
      const double kVel = VelDiff(outs18.at(i), outs9.at(i));
      w_vel = std::max(w_vel, kVel);
      const double kEnergy = EnergyDiff(outs18.at(i), outs9.at(i));
      w_energy = std::max(w_energy, kEnergy);
    }
    std::cout << "SURVEY C6-scaled elevation_in worst18_9="
              << std::setprecision(kSurveyPrecisionDigits) << w_elev_in << "\n";
    std::cout << "SURVEY C6-scaled elevation_moa worst18_9="
              << std::setprecision(kSurveyPrecisionDigits) << w_elev_moa
              << "\n";
    std::cout << "SURVEY C6-scaled deflection_moa worst18_9="
              << std::setprecision(kSurveyPrecisionDigits) << w_defl_moa
              << "\n";
    std::cout << "SURVEY C6-scaled time_of_flight_s worst18_9="
              << std::setprecision(kSurveyPrecisionDigits) << w_tof << "\n";
    std::cout << "SURVEY C6-scaled velocity_fps worst18_9="
              << std::setprecision(kSurveyPrecisionDigits) << w_vel << "\n";
    std::cout << "SURVEY C6-scaled energy_ft_lbf worst18_9="
              << std::setprecision(kSurveyPrecisionDigits) << w_energy << "\n";
  }

  // C8-Litz: spin inputs taking the Litz (not Boatright) path. Mirrors the
  // jump context in test/source/lob_inverse_test.cpp
  // SolveInverseMatchesFastInverseWithJump.
  {
    const std::array<uint32_t, 3> kRanges = {900U, 1800U, 2700U};
    std::array<lob::Output, 3> outs36{};
    std::array<lob::Output, 3> outs18{};
    std::array<lob::Output, 3> outs9{};
    auto build_litz = [](uint16_t step) {
      constexpr double kBcPsi = 0.436;
      constexpr uint16_t kVelocityFps = 3100U;
      constexpr double kZeroAngleMoa = 6.11;
      constexpr double kDiameterInch = 0.308;
      constexpr double kLengthInch = 1.215;
      constexpr double kMassGrains = 168.0;
      constexpr double kTwistIn = 10.0;
      constexpr double kWindSpeedMph = 10.0;
      lob::Builder b;
      b.BallisticCoefficientPsi(kBcPsi)
          .InitialVelocityFps(kVelocityFps)
          .ZeroAngleMOA(kZeroAngleMoa)
          .DiameterInch(kDiameterInch)
          .LengthInch(kLengthInch)
          .MassGrains(kMassGrains)
          .TwistInchesPerTurn(kTwistIn)
          .WindHeading(lob::ClockAngleT::kIII)
          .WindSpeedMph(kWindSpeedMph);
      return BuildAtStep(b, step);
    };
    const lob::Context kProbe = build_litz(36U);
    ASSERT_EQ(kProbe.error, lob::ErrorT::kNone);
    // Litz path is active when Boatright leaves spindrift_factor NaN while
    // Miller stability is finite and the crosswind jump is nonzero.
    EXPECT_TRUE(std::isnan(kProbe.spindrift_factor));
    EXPECT_TRUE(std::isfinite(kProbe.stability_factor));
    EXPECT_TRUE(std::fabs(kProbe.stability_factor) > 0.0);
    EXPECT_TRUE(std::fabs(kProbe.aerodynamic_jump) > 0.0);
    std::cout << "SURVEY C8-Litz branch spindrift_isnan="
              << (std::isnan(kProbe.spindrift_factor) ? 1 : 0)
              << " stability=" << std::setprecision(kSurveyPrecisionDigits)
              << kProbe.stability_factor
              << " jump_moa=" << kProbe.aerodynamic_jump << "\n";
    ASSERT_EQ(SolveN(build_litz(36U), kRanges, &outs36), kRanges.size());
    ASSERT_EQ(SolveN(build_litz(18U), kRanges, &outs18), kRanges.size());
    ASSERT_EQ(SolveN(build_litz(9U), kRanges, &outs9), kRanges.size());
    double w_elev_in = 0.0;
    double w_elev_moa = 0.0;
    double w_defl_moa = 0.0;
    double w_tof = 0.0;
    double w_vel = 0.0;
    double w_energy = 0.0;
    for (size_t i = 0; i < kRanges.size(); ++i) {
      const double kElevIn = ElevInDiff(outs18.at(i), outs9.at(i));
      w_elev_in = std::max(w_elev_in, kElevIn);
      const double kElevMoa = ElevMoaDiff(outs18.at(i), outs9.at(i));
      w_elev_moa = std::max(w_elev_moa, kElevMoa);
      const double kDeflMoa = DeflMoaDiff(outs18.at(i), outs9.at(i));
      w_defl_moa = std::max(w_defl_moa, kDeflMoa);
      const double kTof = TofDiff(outs18.at(i), outs9.at(i));
      w_tof = std::max(w_tof, kTof);
      const double kVel = VelDiff(outs18.at(i), outs9.at(i));
      w_vel = std::max(w_vel, kVel);
      const double kEnergy = EnergyDiff(outs18.at(i), outs9.at(i));
      w_energy = std::max(w_energy, kEnergy);
    }
    std::cout << "SURVEY C8-Litz elevation_in worst18_9="
              << std::setprecision(kSurveyPrecisionDigits) << w_elev_in << "\n";
    std::cout << "SURVEY C8-Litz elevation_moa worst18_9="
              << std::setprecision(kSurveyPrecisionDigits) << w_elev_moa
              << "\n";
    std::cout << "SURVEY C8-Litz deflection_moa worst18_9="
              << std::setprecision(kSurveyPrecisionDigits) << w_defl_moa
              << "\n";
    std::cout << "SURVEY C8-Litz time_of_flight_s worst18_9="
              << std::setprecision(kSurveyPrecisionDigits) << w_tof << "\n";
    std::cout << "SURVEY C8-Litz velocity_fps worst18_9="
              << std::setprecision(kSurveyPrecisionDigits) << w_vel << "\n";
    std::cout << "SURVEY C8-Litz energy_ft_lbf worst18_9="
              << std::setprecision(kSurveyPrecisionDigits) << w_energy << "\n";
  }

  // C9-dynamic-tail: lapse-scaled SolveAngle path via SolveInverse at
  // {6000, 7500, 9000} ft. Precedent: solve_angle_test.cpp BC 0.436,
  // 3100 fps, zero 6.11 MOA. Fail loud unless forward drop < -1200 in.
  {
    const std::array<uint32_t, 3> kRanges = {6000U, 7500U, 9000U};
    auto build_tail = [](uint16_t step) {
      constexpr double kBcPsi = 0.436;
      constexpr uint16_t kVelocityFps = 3100U;
      constexpr double kZeroAngleMoa = 6.11;
      lob::Builder b;
      b.BallisticCoefficientPsi(kBcPsi)
          .InitialVelocityFps(kVelocityFps)
          .ZeroAngleMOA(kZeroAngleMoa);
      return BuildAtStep(b, step);
    };
    std::array<lob::Output, 3> fwd{};
    ASSERT_EQ(SolveN(build_tail(36U), kRanges, &fwd), kRanges.size());
    for (size_t i = 0; i < kRanges.size(); ++i) {
      std::cout << "SURVEY C9-dynamic-tail branch range_ft=" << kRanges.at(i)
                << " forward_drop_in="
                << std::setprecision(kSurveyPrecisionDigits)
                << fwd.at(i).elevation << "\n";
      EXPECT_LT(fwd.at(i).elevation, -1200.0) << "range=" << kRanges.at(i);
    }
    std::array<lob::Output, 3> outs36{};
    std::array<lob::Output, 3> outs18{};
    std::array<lob::Output, 3> outs9{};
    ASSERT_EQ(lob::SolveInverse(build_tail(36U), kRanges, &outs36),
              kRanges.size());
    ASSERT_EQ(lob::SolveInverse(build_tail(18U), kRanges, &outs18),
              kRanges.size());
    ASSERT_EQ(lob::SolveInverse(build_tail(9U), kRanges, &outs9),
              kRanges.size());
    double w_elev_moa = 0.0;
    double w_defl_moa = 0.0;
    double w_tof = 0.0;
    double w_vel = 0.0;
    double w_energy = 0.0;
    for (size_t i = 0; i < kRanges.size(); ++i) {
      EXPECT_TRUE(std::isfinite(outs36.at(i).elevation));
      EXPECT_TRUE(std::isfinite(outs18.at(i).elevation));
      EXPECT_TRUE(std::isfinite(outs9.at(i).elevation));
      const double kElevMoa =
          std::fabs(outs18.at(i).elevation - outs9.at(i).elevation);
      w_elev_moa = std::max(w_elev_moa, kElevMoa);
      const double kDeflMoa =
          std::fabs(outs18.at(i).deflection - outs9.at(i).deflection);
      w_defl_moa = std::max(w_defl_moa, kDeflMoa);
      const double kTof = TofDiff(outs18.at(i), outs9.at(i));
      w_tof = std::max(w_tof, kTof);
      const double kVel = VelDiff(outs18.at(i), outs9.at(i));
      w_vel = std::max(w_vel, kVel);
      const double kEnergy = EnergyDiff(outs18.at(i), outs9.at(i));
      w_energy = std::max(w_energy, kEnergy);
    }
    std::cout << "SURVEY C9-dynamic-tail elevation_moa worst18_9="
              << std::setprecision(kSurveyPrecisionDigits) << w_elev_moa
              << "\n";
    std::cout << "SURVEY C9-dynamic-tail deflection_moa worst18_9="
              << std::setprecision(kSurveyPrecisionDigits) << w_defl_moa
              << "\n";
    std::cout << "SURVEY C9-dynamic-tail time_of_flight_s worst18_9="
              << std::setprecision(kSurveyPrecisionDigits) << w_tof << "\n";
    std::cout << "SURVEY C9-dynamic-tail velocity_fps worst18_9="
              << std::setprecision(kSurveyPrecisionDigits) << w_vel << "\n";
    std::cout << "SURVEY C9-dynamic-tail energy_ft_lbf worst18_9="
              << std::setprecision(kSurveyPrecisionDigits) << w_energy << "\n";
  }
}

}  // namespace tests
