// Copyright (c) 2026  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Static-only: includes internal solver headers (never add to
// LOB_TEST_SOURCES).

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <string>
#include <utility>
#include <vector>

#include "cartesian.hpp"
#include "eng_units.hpp"
#include "lob/lob.h"
#include "ode.hpp"
#include "solve_angle.hpp"
#include "solve_step.hpp"
#include "wind.hpp"

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

namespace {

inline std::vector<double> EffectDelta(
    const std::vector<double>& with_effect,
    const std::vector<double>& without_effect) {
  std::vector<double> delta(with_effect.size(), 0.0);
  for (std::size_t i = 0; i < with_effect.size(); ++i) {
    delta.at(i) = with_effect.at(i) - without_effect.at(i);
  }
  return delta;
}

inline double UTotal(
    double u_num, double granularity,
    double mc_sigma = std::numeric_limits<double>::quiet_NaN()) {
  double sum = (u_num * u_num) + (granularity * granularity);
  if (!std::isnan(mc_sigma)) {
    sum += mc_sigma * mc_sigma;
  }
  return std::sqrt(sum);
}

inline double RSig(double delta, double u_total) {
  if (!(u_total > 0.0)) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  return std::fabs(delta) / u_total;
}

inline SigBand ClassifySigBand(double r_sig) {
  if (r_sig > kSigClear) {
    return SigBand::kDistinguishable;
  }
  if (r_sig >= kSigMarginalLo) {
    return SigBand::kMarginal;
  }
  return SigBand::kIndistinguishable;
}

}  // namespace

}  // namespace tests

namespace {

inline const char* ProbeSigBandName(tests::SigBand band) {
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

// C1-ICAO point via the C API used by test/source/solve_angle_test.cpp —
// same values as tests::MakeC1IcaoBuilder() (testing.hpp).
inline LobContext BuildProbeC1() {
  constexpr double kBcPsi = 0.232;
  constexpr double kDiameterInch = 0.308;
  constexpr double kMassGrains = 155.0;
  constexpr uint16_t kMuzzleVelocityFps = 2800U;
  constexpr double kZeroAngleMoa = 3.66;
  constexpr double kOpticHeightInches = 1.5;
  LobBuilder builder{};
  LobBuilderInit(&builder);
  LobBuilderBallisticCoefficientPsi(&builder, kBcPsi);
  LobBuilderBCDragFunction(&builder, kLobDragFunctionG7);
  LobBuilderBCAtmosphere(&builder, kLobAtmosphereReferenceIcao);
  LobBuilderDiameterInch(&builder, kDiameterInch);
  LobBuilderMassGrains(&builder, kMassGrains);
  LobBuilderInitialVelocityFps(&builder, kMuzzleVelocityFps);
  LobBuilderZeroAngleMOA(&builder, kZeroAngleMoa);
  LobBuilderOpticHeightInches(&builder, kOpticHeightInches);
  LobContext ctx{};
  LobBuilderBuild(&builder, &ctx);
  LobBuilderDestroy(&builder);
  return ctx;
}

// C9-dynamic-tail point: same values as the C9-tail context in
// test/source/validation_signal_test.cpp (BC 0.436, 3100 fps, 6.11 MOA).
inline LobContext BuildProbeC9Tail() {
  constexpr double kBcPsi = 0.436;
  constexpr uint16_t kVelocityFps = 3100U;
  constexpr double kZeroAngleMoa = 6.11;
  LobBuilder builder{};
  LobBuilderInit(&builder);
  LobBuilderBallisticCoefficientPsi(&builder, kBcPsi);
  LobBuilderInitialVelocityFps(&builder, kVelocityFps);
  LobBuilderZeroAngleMOA(&builder, kZeroAngleMoa);
  LobContext ctx{};
  LobBuilderBuild(&builder, &ctx);
  LobBuilderDestroy(&builder);
  return ctx;
}

// C6-scaled wind point: same values as MakeWindBaseBuilder() + the two-point
// 90-degree profile with shear 0.25 in
// test/source/validation_convergence_test.cpp. incline_deg bakes the range
// pitch into the nodes (0.0 = flat datum).
inline LobContext BuildProbeC6(double incline_deg) {
  constexpr double kBcPsi = 0.372;
  constexpr double kDiameterInch = 0.224;
  constexpr double kMassGrains = 77.0;
  constexpr uint16_t kVelocityFps = 2720U;
  constexpr double kZeroAngleMoa = 4.78;
  constexpr double kOpticHeightIn = 2.5;
  constexpr double kShearExponent = 0.25;
  const std::array<LobWindPoint, 2> kTwoPoint = {{
      {0.0, 90.0, 5.0, std::numeric_limits<double>::quiet_NaN()},
      {1500.0, 90.0, 10.0, 6.0},
  }};
  LobBuilder builder{};
  LobBuilderInit(&builder);
  LobBuilderBallisticCoefficientPsi(&builder, kBcPsi);
  LobBuilderBCDragFunction(&builder, kLobDragFunctionG1);
  LobBuilderDiameterInch(&builder, kDiameterInch);
  LobBuilderMassGrains(&builder, kMassGrains);
  LobBuilderInitialVelocityFps(&builder, kVelocityFps);
  LobBuilderZeroAngleMOA(&builder, kZeroAngleMoa);
  LobBuilderOpticHeightInches(&builder, kOpticHeightIn);
  LobBuilderWindProfile(&builder, kTwoPoint.data(), kTwoPoint.size());
  LobBuilderWindShearExponent(&builder, kShearExponent);
  LobBuilderRangeAngleDeg(&builder, incline_deg);
  LobContext ctx{};
  LobBuilderBuild(&builder, &ctx);
  LobBuilderDestroy(&builder);
  return ctx;
}

// Trajectory state at (downrange_ft, height_ft) with a cruise-like velocity
// (GetWind reads position only; velocity is carried for API completeness).
inline lob::TrajectoryStateT ProbeState(double downrange_ft, double height_ft) {
  constexpr double kCruiseFps = 2500.0;
  return {lob::CartesianT<lob::FeetT>(lob::FeetT(downrange_ft),
                                      lob::FeetT(height_ft), lob::FeetT(0.0)),
          lob::CartesianT<lob::FpsT>(lob::FpsT(kCruiseFps), lob::FpsT(0.0),
                                     lob::FpsT(0.0))};
}

// Wind-vector magnitude helper (crosswind lives on the lateral axis, so no
// single Cartesian component carries every fixture — compare magnitudes).
inline double ProbeWindMag(const lob::CartesianT<lob::FpsT>& wind) {
  const double kX = wind.X().Value();
  const double kY = wind.Y().Value();
  const double kZ = wind.Z().Value();
  return std::sqrt((kX * kX) + (kY * kY) + (kZ * kZ));
}

// Exact LobInchToMoa math via the allowed eng_units header (no public-header
// include needed): MOA = IPHY at range_ft hundred-yards.
inline double ProbeInchToMoa(double inches, double range_ft) {
  constexpr double kHundredYardsInFeet = 300.0;
  return lob::MoaT(lob::IphyT(inches / (range_ft / kHundredYardsInFeet)))
      .Value();
}

inline bool ProbesOfflineGated() {
  // single-threaded gtest; env gates select offline drivers
  // NOLINTNEXTLINE(concurrency-mt-unsafe)
  const char* gate = std::getenv("LOB_FULL_SIGNAL");
  return gate != nullptr && std::string(gate) == "1";
}

}  // namespace

// Effect 2 (STATIC): SolveAngle/FastSolveAngle tolerance 0.01→0.001 on
// C1 @900 ft. Structural asserts (both paths complete, tightening lands
// within granularity); the R_sig verdict is RECORDED, never tripwired.
TEST(SignalProbes, AngleToleranceTightening) {
  const LobContext kCtx = BuildProbeC1();
  ASSERT_EQ(kCtx.error, kLobErrorNone);
  const lob::FeetT kRange(900.0);
  const lob::MoaT kSolveDefault =
      lob::SolveAngle(kCtx, kRange, lob::FeetT(0.0), lob::RadiansT(0.0));
  const lob::MoaT kSolveTight =
      lob::SolveAngle(kCtx, kRange, lob::FeetT(0.0), lob::RadiansT(0.0),
                      lob::RadiansT(lob::MoaT(0.001)));
  const lob::MoaT kFastDefault =
      lob::FastSolveAngle(kCtx, kRange, lob::FeetT(0.0), lob::RadiansT(0.0));
  const lob::MoaT kFastTight =
      lob::FastSolveAngle(kCtx, kRange, lob::FeetT(0.0), lob::RadiansT(0.0),
                          lob::RadiansT(lob::MoaT(0.001)));
  ASSERT_FALSE(kSolveDefault.IsNaN());
  ASSERT_FALSE(kSolveTight.IsNaN());
  ASSERT_FALSE(kFastDefault.IsNaN());
  ASSERT_FALSE(kFastTight.IsNaN());
  EXPECT_LE(std::fabs((kSolveTight - kSolveDefault).Value()), 0.02);
  EXPECT_LE(std::fabs((kFastTight - kFastDefault).Value()), 0.02);
  // Yardstick cross-link: u_num is floors.json C1-ICAO
  // floors_18_9.elevation_moa; granularity is the 0.01 MOA angle tolerance.
  constexpr double kUNumMoa = 3.97148e-06;
  constexpr double kGranMoa = 0.01;
  const double kSolveR =
      tests::RSig(std::fabs((kSolveTight - kSolveDefault).Value()),
                  tests::UTotal(kUNumMoa, kGranMoa));
  const double kFastR =
      tests::RSig(std::fabs((kFastTight - kFastDefault).Value()),
                  tests::UTotal(kUNumMoa, kGranMoa));
  std::cout << "PROBE angle-tol solve r_sig=" << kSolveR
            << " band=" << ProbeSigBandName(tests::ClassifySigBand(kSolveR))
            << "\n";
  std::cout << "PROBE angle-tol fast r_sig=" << kFastR
            << " band=" << ProbeSigBandName(tests::ClassifySigBand(kFastR))
            << "\n";
}

// Effect 3 (STATIC): FastSolveStep vs SolveStep trajectories from an
// identical C9 initial state. Both steppers fire to each range from the same
// seed; the per-range residual Δ is RECORDED (small-but-nonzero expected —
// the path gate exists because stepping matters at range).
TEST(SignalProbes, LapseSteppingDivergence) {
  const LobContext kCtx = BuildProbeC9Tail();
  ASSERT_EQ(kCtx.error, kLobErrorNone);
  const std::array<uint32_t, 3> kRanges = {6000U, 7500U, 9000U};
  constexpr double kSeedMoa = 6.11;
  std::vector<double> fast_resid;
  std::vector<double> solve_resid;
  for (const uint32_t kRangeFt : kRanges) {
    const lob::FeetT kRange(static_cast<double>(kRangeFt));
    const lob::FeetT kFast = lob::detail::FireToTarget(
        kCtx, kRange, lob::FeetT(0.0), lob::RadiansT(lob::MoaT(kSeedMoa)),
        lob::FastSolveStep);
    const lob::FeetT kSolve = lob::detail::FireToTarget(
        kCtx, kRange, lob::FeetT(0.0), lob::RadiansT(lob::MoaT(kSeedMoa)),
        lob::SolveStep);
    ASSERT_TRUE(std::isfinite(kFast.Value())) << "range=" << kRangeFt;
    ASSERT_TRUE(std::isfinite(kSolve.Value())) << "range=" << kRangeFt;
    fast_resid.push_back(kFast.Value());
    solve_resid.push_back(kSolve.Value());
  }
  const std::vector<double> kDelta =
      tests::EffectDelta(fast_resid, solve_resid);
  ASSERT_EQ(kDelta.size(), kRanges.size());
  size_t range_idx = 0;
  for (const uint32_t kRangeFt : kRanges) {
    std::cout << "PROBE lapse-step range_ft=" << kRangeFt
              << " delta_in=" << kDelta.at(range_idx) << "\n";
    ++range_idx;
  }
}

// Effect 13 bound input (STATIC): GetWind height-response at the same
// downrange × Y sweep, flat datum vs 15° incline. Characterization: both
// curves finite; incline differs from flat (pitch is baked into the nodes).
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
TEST(SignalProbes, GetWindHeightResponse) {
  const LobContext kFlat = BuildProbeC6(0.0);
  ASSERT_EQ(kFlat.error, kLobErrorNone);
  const LobContext kHill = BuildProbeC6(15.0);
  ASSERT_EQ(kHill.error, kLobErrorNone);
  const std::array<double, 2> kDownrange = {900.0, 1800.0};
  const std::array<double, 6> kHeights = {1.0,   50.0,  150.0,
                                          299.0, 500.0, 5000.0};
  constexpr double kWindEpsFps = 1e-9;
  bool hill_differs = false;
  for (const double kDownrangeFt : kDownrange) {
    for (const double kHeightFt : kHeights) {
      const lob::TrajectoryStateT kState = ProbeState(kDownrangeFt, kHeightFt);
      const lob::CartesianT<lob::FpsT> kFlatWind = lob::GetWind(kFlat, kState);
      const lob::CartesianT<lob::FpsT> kHillWind = lob::GetWind(kHill, kState);
      const double kFlatMag = ProbeWindMag(kFlatWind);
      const double kHillMag = ProbeWindMag(kHillWind);
      ASSERT_TRUE(std::isfinite(kFlatMag));
      ASSERT_TRUE(std::isfinite(kHillMag));
      if (std::fabs(kHillMag - kFlatMag) > kWindEpsFps) {
        hill_differs = true;
      }
      std::cout << "PROBE getwind x_ft=" << kDownrangeFt
                << " y_ft=" << kHeightFt << " flat_fps=" << kFlatMag
                << " hill_fps=" << kHillMag << "\n";
    }
  }
  EXPECT_TRUE(hill_differs);
}

// Effect 14 (STATIC): GetWind at above-300-ft-equivalent states clamps to
// the 300-ft band — wind stops growing. Exact clamp: all heights ≥300 at the
// same downrange return bitwise-identical winds.
TEST(SignalProbes, CeilingClampHolds) {
  const LobContext kCtx = BuildProbeC6(0.0);
  ASSERT_EQ(kCtx.error, kLobErrorNone);
  constexpr double kDownrangeFt = 900.0;
  const double kAt300 =
      ProbeWindMag(lob::GetWind(kCtx, ProbeState(kDownrangeFt, 300.0)));
  const std::array<double, 3> kAbove = {301.0, 1000.0, 5000.0};
  for (const double kHeightFt : kAbove) {
    const double kWind =
        ProbeWindMag(lob::GetWind(kCtx, ProbeState(kDownrangeFt, kHeightFt)));
    EXPECT_DOUBLE_EQ(kWind, kAt300) << "y_ft=" << kHeightFt;
  }
  // Guard against a vacuous clamp: below-band winds must still grow with
  // height, so the clamp above is a real ceiling, not a dead scaling path.
  const double kAt50 =
      ProbeWindMag(lob::GetWind(kCtx, ProbeState(kDownrangeFt, 50.0)));
  EXPECT_GT(kAt300, kAt50);
}

// Offline writer for the two STATIC effects (no public knob exists, so the
// LOB driver cannot produce them). Env-gated: LOB_FULL_SIGNAL=1 writes
// signal_angle-tol_C1.json + signal_lapse-fast-vs-solve_C9.json; CI path
// performs no file I/O.
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
TEST(SignalProbesOffline, WritesStaticEffects) {
  if (!ProbesOfflineGated()) {
    GTEST_SKIP() << "offline only: set LOB_FULL_SIGNAL=1";
  }
  const std::string kDir = LOB_VALIDATION_DIR;

  // Effect 2: tolerance R_sig on the MOA channel.
  {
    const LobContext kCtx = BuildProbeC1();
    ASSERT_EQ(kCtx.error, kLobErrorNone);
    constexpr uint32_t kRangeFt = 900U;
    const lob::FeetT kRange(static_cast<double>(kRangeFt));
    const lob::MoaT kSolveDefault =
        lob::SolveAngle(kCtx, kRange, lob::FeetT(0.0), lob::RadiansT(0.0));
    const lob::MoaT kSolveTight =
        lob::SolveAngle(kCtx, kRange, lob::FeetT(0.0), lob::RadiansT(0.0),
                        lob::RadiansT(lob::MoaT(0.001)));
    const lob::MoaT kFastDefault =
        lob::FastSolveAngle(kCtx, kRange, lob::FeetT(0.0), lob::RadiansT(0.0));
    const lob::MoaT kFastTight =
        lob::FastSolveAngle(kCtx, kRange, lob::FeetT(0.0), lob::RadiansT(0.0),
                            lob::RadiansT(lob::MoaT(0.001)));
    ASSERT_FALSE(kSolveDefault.IsNaN());
    ASSERT_FALSE(kSolveTight.IsNaN());
    ASSERT_FALSE(kFastDefault.IsNaN());
    ASSERT_FALSE(kFastTight.IsNaN());
    constexpr double kUNumMoa = 3.97148e-06;
    constexpr double kGranMoa = 0.01;
    const double kUTotal = tests::UTotal(kUNumMoa, kGranMoa);
    nlohmann::json doc;
    doc["provenance"] = {
        {"git_sha", LOB_GIT_SHA},
        {"generator", "SignalProbesOffline.WritesStaticEffects"},
        {"gate", "LOB_FULL_SIGNAL=1"}};
    doc["effect"] = "angle-tol";
    doc["case"] = "C1";
    doc["isolation"] = "ISOLATED";
    doc["expectation"] = "R_sig << 1 (tolerance tightening below resolution)";
    doc["rows"] = nlohmann::json::array();
    const std::array<std::pair<const char*, double>, 2> kPaths = {{
        {"solve", std::fabs((kSolveTight - kSolveDefault).Value())},
        {"fast", std::fabs((kFastTight - kFastDefault).Value())},
    }};
    for (const auto& path : kPaths) {
      const double kR = tests::RSig(path.second, kUTotal);
      nlohmann::json row;
      row["range_ft"] = kRangeFt;
      row["path"] = path.first;
      row["channel"] = "elevation_moa";
      row["delta"] = path.second;
      row["u_total"] = kUTotal;
      row["r_sig"] = kR;
      row["band"] = ProbeSigBandName(tests::ClassifySigBand(kR));
      row["yardstick"] = {
          {"u_num_source", "floors.json C1-ICAO floors_18_9.elevation_moa"},
          {"granularity", kGranMoa},
          {"mc_sigma_or_absent", "mc_sigma_absent"},
          {"u_c", nullptr}};
      doc["rows"].push_back(row);
    }
    std::ofstream out((kDir + "/signal_angle-tol_C1.json").c_str());
    ASSERT_TRUE(out.good());
    out << doc.dump(2) << "\n";
  }

  // Effect 3: lapse-stepping per-range Δ (inches + MOA); R_sig on MOA.
  {
    const LobContext kCtx = BuildProbeC9Tail();
    ASSERT_EQ(kCtx.error, kLobErrorNone);
    const std::array<uint32_t, 3> kRanges = {6000U, 7500U, 9000U};
    constexpr double kSeedMoa = 6.11;
    constexpr double kUNumMoa = 6.535115801e-06;
    constexpr double kGranMoa = 0.01;
    const double kUTotal = tests::UTotal(kUNumMoa, kGranMoa);
    nlohmann::json doc;
    doc["provenance"] = {
        {"git_sha", LOB_GIT_SHA},
        {"generator", "SignalProbesOffline.WritesStaticEffects"},
        {"gate", "LOB_FULL_SIGNAL=1"}};
    doc["effect"] = "lapse-fast-vs-solve";
    doc["case"] = "C9";
    doc["isolation"] = "ISOLATED";
    doc["expectation"] = "small-but-nonzero per-range delta (path matters)";
    doc["rows"] = nlohmann::json::array();
    for (const uint32_t kRangeFt : kRanges) {
      const lob::FeetT kRange(static_cast<double>(kRangeFt));
      const lob::FeetT kFast = lob::detail::FireToTarget(
          kCtx, kRange, lob::FeetT(0.0), lob::RadiansT(lob::MoaT(kSeedMoa)),
          lob::FastSolveStep);
      const lob::FeetT kSolve = lob::detail::FireToTarget(
          kCtx, kRange, lob::FeetT(0.0), lob::RadiansT(lob::MoaT(kSeedMoa)),
          lob::SolveStep);
      ASSERT_TRUE(std::isfinite(kFast.Value()));
      ASSERT_TRUE(std::isfinite(kSolve.Value()));
      const double kDeltaIn = kFast.Value() - kSolve.Value();
      const double kDeltaMoa =
          ProbeInchToMoa(kDeltaIn, static_cast<double>(kRangeFt));
      const double kR = tests::RSig(std::fabs(kDeltaMoa), kUTotal);
      nlohmann::json row;
      row["range_ft"] = kRangeFt;
      row["channel"] = "elevation_moa";
      row["delta_in"] = kDeltaIn;
      row["delta"] = kDeltaMoa;
      row["u_total"] = kUTotal;
      row["r_sig"] = kR;
      row["band"] = ProbeSigBandName(tests::ClassifySigBand(kR));
      row["yardstick"] = {{"u_num_source",
                           "floors.json C9-dynamic-tail "
                           "floors_18_9.elevation_moa"},
                          {"granularity", kGranMoa},
                          {"mc_sigma_or_absent", "mc_sigma_absent"},
                          {"u_c", nullptr}};
      doc["rows"].push_back(row);
    }
    std::ofstream out((kDir + "/signal_lapse-fast-vs-solve_C9.json").c_str());
    ASSERT_TRUE(out.good());
    out << doc.dump(2) << "\n";
  }
}
