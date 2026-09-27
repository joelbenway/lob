// Copyright (c) 2026  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later

#include "validation_sensitivity.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <limits>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "testing.hpp"
#include "validation_io.hpp"

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
    p.InitialVelocityFps(static_cast<uint16_t>(std::llround(2800.0 + kDv)));
    const lob::Context kCtx = p.Build();
    ASSERT_EQ(kCtx.error, lob::ErrorT::kNone);
    EXPECT_EQ(kCtx.velocity,
              static_cast<uint16_t>(2800 + static_cast<int>(kDv)));
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

TEST(SensitivityIo, ArtifactRoundTripsSyntheticRows) {
  SensitivityArtifact artifact;
  artifact.provenance_lob_version = "0.13.0-test";
  artifact.provenance_git_sha = "deadbee";
  artifact.solver_config = "step_in=36,angle_tol_moa=0.01,density_path=fast";
  artifact.AddRow("velocity_fps", 1800U, "elevation_in", 10.0, 2.31, 23.1,
                  false, "genuine");
  artifact.AddRow("wind_speed_mph", 1800U, "elevation_in", 1.0, 0.0, 0.0, false,
                  "below_floor");
  const std::string kJson = artifact.ToJson();
  EXPECT_NE(kJson.find("\"input\":\"velocity_fps\""), std::string::npos);
  EXPECT_NE(kJson.find("\"status\":\"below_floor\""), std::string::npos);
  EXPECT_NE(kJson.find("\"sensitivity_coefficient_S\":null"),
            std::string::npos);
  const std::string kCsv = artifact.ToCsv();
  EXPECT_NE(kCsv.find("input,range_ft,output,h_accepted,raw_deriv,"),
            std::string::npos);
}

namespace {
// ---- Phase 2 Task 5: offline full-Pareto driver (env-gated) ----
// Survey builders mirrored exactly from ValidationFloorSurvey.SurveyCells in
// test/source/validation_convergence_test.cpp:
// - C5-uniform: wind-base builder + WindHeading(kIII) + 5 mph.
// - C6-scaled: wind-base builder + two-point 90-degree profile + shear 0.25.
// - C8-Litz: BC 0.436 / 3100 fps / zero 6.11 MOA / dia 0.308 / len 1.215 /
//   mass 168 gr / twist 10.0 + kIII 10 mph.
// Canned response definition: |f(x+h) - f(x-h)|, the full symmetric swing
// at the accepted h; one-sided |f(x+h) - f(x)| for boundary inputs
// (humidity_pp at the 0% boundary, shear_exponent per brief). LOB_FULL_PARETO
// gates everything; no file I/O unless gated.

enum ParetoKind {
  kParetoVelocity,
  kParetoBc,
  kParetoZero,
  kParetoOptic,
  kParetoPressure,
  kParetoTemp,
  kParetoHumidity,
  kParetoMass,
  kParetoDiameter,
  kParetoLength,
  kParetoTwist,
  kParetoWindSpeed,
  kParetoWindHeading,
  kParetoShear,
  kParetoHeight2
};

struct ParetoInput {
  const char* name;
  ParetoKind kind;
  double x0;
  double h_seed;
  double quantum;
  bool one_sided;  // forward-only evals (humidity at 0%, shear per brief)
};

// Channel ids: 0 elevation_in, 1 elevation_moa, 2 deflection_moa,
// 3 velocity_fps, 4 tof_s. Inverse MOA (C1 only) uses channel id 5 with
// SolveInverse outputs, where .elevation is the MOA adjustment.
struct ParetoFloors {
  double elev_in;
  double elev_moa;
  double defl_moa;
  double vel_fps;
  double tof_s;
  double inverse_moa;
};

inline lob::Builder ParetoWindBaseBuilder() {
  // Mirrors MakeWindBaseBuilder in validation_convergence_test.cpp.
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

inline lob::Builder ParetoC8LitzBuilder() {
  // Mirrors the C8-Litz survey builder in validation_convergence_test.cpp.
  lob::Builder b;
  b.BallisticCoefficientPsi(0.436)
      .InitialVelocityFps(3100U)
      .ZeroAngleMOA(6.11)
      .DiameterInch(0.308)
      .LengthInch(1.215)
      .MassGrains(168.0)
      .TwistInchesPerTurn(10.0)
      .WindHeading(lob::ClockAngleT::kIII)
      .WindSpeedMph(10.0);
  return b;
}

inline double ParetoFloorFor(const ParetoFloors& floors, int channel) {
  switch (channel) {
    case 0:
      return floors.elev_in;
    case 1:
      return floors.elev_moa;
    case 2:
      return floors.defl_moa;
    case 3:
      return floors.vel_fps;
    case 4:
      return floors.tof_s;
    default:
      return floors.inverse_moa;
  }
}

inline const char* ParetoChannelName(int channel) {
  switch (channel) {
    case 0:
      return "elevation_in";
    case 1:
      return "elevation_moa";
    case 2:
      return "deflection_moa";
    case 3:
      return "velocity_fps";
    case 4:
      return "tof_s";
    default:
      return "inverse_moa";
  }
}

inline double ParetoExtract(const lob::Output& o, int channel) {
  const double kRangeFt = static_cast<double>(o.range);
  switch (channel) {
    case 0:
      return o.elevation;
    case 1:
      return lob::InchToMoa(o.elevation, kRangeFt);
    case 2:
      return lob::InchToMoa(o.deflection, kRangeFt);
    case 3:
      return static_cast<double>(o.velocity);
    case 4:
      return o.time_of_flight;
    default:
      return o.elevation;  // inverse: .elevation is the MOA adjustment
  }
}

// Applies absolute value v for one input kind onto builder copy b.
// zero_wind selects the C1 zero-baseline wind_speed mapping: +h is heading
// 90 at h mph, -h is heading 270 at h mph, never a negative speed.
// profile supplies the C6 two-point stations for the height_ft tweak.
inline void ParetoApply(lob::Builder* b, ParetoKind kind, double v,
                        bool zero_wind) {
  switch (kind) {
    case kParetoVelocity:
      b->InitialVelocityFps(static_cast<uint16_t>(std::llround(v)));
      break;
    case kParetoBc:
      b->BallisticCoefficientPsi(v);
      break;
    case kParetoZero:
      b->ZeroAngleMOA(v);
      break;
    case kParetoOptic:
      b->OpticHeightInches(v);
      break;
    case kParetoPressure:
      b->AirPressureInHg(v);
      break;
    case kParetoTemp:
      b->TemperatureDegF(v);
      break;
    case kParetoHumidity:
      b->RelativeHumidityPercent(v);
      break;
    case kParetoMass:
      b->MassGrains(v);
      break;
    case kParetoDiameter:
      b->DiameterInch(v);
      break;
    case kParetoLength:
      b->LengthInch(v);
      break;
    case kParetoTwist:
      b->TwistInchesPerTurn(v);
      break;
    case kParetoWindSpeed:
      if (zero_wind) {
        if (v > 0.0) {
          b->WindHeadingDeg(90.0);
          b->WindSpeedMph(v);
        } else if (v < 0.0) {
          b->WindHeadingDeg(270.0);
          b->WindSpeedMph(-v);
        } else {
          b->WindSpeedMph(0.0);
        }
      } else {
        b->WindSpeedMph(v);
      }
      break;
    case kParetoWindHeading:
      b->WindHeadingDeg(v);
      break;
    case kParetoShear:
      b->WindShearExponent(v);
      break;
    case kParetoHeight2:
      // Unreachable by construction: height tweaks go through
      // ParetoApplyProfile at the call sites, because WindProfile borrows
      // the station array and it must outlive Build().
      break;
  }
}

// Tweaks station-2 height in a caller-owned array and points the builder at
// it. The array MUST stay alive through Build() (WindProfile borrows).
inline void ParetoApplyProfile(lob::Builder* b,
                               std::array<lob::WindPoint, 2>* pts, double v) {
  (*pts)[1].height_ft = v;
  b->WindProfile(pts->data(), pts->size());
}

// Builds + solves one perturbed point over all ranges. Returns false (with
// ADD_FAILURE context from the caller) on build error, short solve, or
// non-finite outputs.
inline bool ParetoEval(lob::Builder base, ParetoKind kind, double v,
                       bool zero_wind,
                       const std::array<lob::WindPoint, 2>& profile,
                       const std::vector<uint32_t>& ranges, bool inverse,
                       std::vector<lob::Output>* outs) {
  // WindProfile borrows the station array: the tweaked copy lives in this
  // scope through Build().
  std::array<lob::WindPoint, 2> pts = profile;
  if (kind == kParetoHeight2) {
    ParetoApplyProfile(&base, &pts, v);
  } else {
    ParetoApply(&base, kind, v, zero_wind);
  }
  const lob::Context kCtx = base.Build();
  if (kCtx.error != lob::ErrorT::kNone) {
    return false;
  }
  outs->assign(ranges.size(), lob::Output());
  size_t solved = 0;
  if (inverse) {
    solved = lob::SolveInverse(kCtx, ranges.data(), outs->data(), outs->size());
  } else {
    solved = lob::Solve(kCtx, ranges.data(), outs->data(), outs->size());
  }
  if (solved != outs->size()) {
    return false;
  }
  for (size_t i = 0; i < outs->size(); ++i) {
    if (!std::isfinite((*outs)[i].elevation) ||
        !std::isfinite((*outs)[i].deflection) ||
        !std::isfinite((*outs)[i].time_of_flight)) {
      return false;
    }
  }
  return true;
}

// One-sided analog of SelectH for boundary inputs (humidity_pp at 0%,
// shear_exponent per brief): forward differences at h, 2h, h/2 with the
// same 20% relative-spread acceptance rule.
template <typename F>
HSelection ParetoForwardSelectH(F f, double x, double h) {
  const double kF0 = f(x);
  const double kD1 = (f(x + h) - kF0) / h;
  const double kD2 = (f(x + 2.0 * h) - kF0) / (2.0 * h);
  const double kD3 = (f(x + 0.5 * h) - kF0) / (0.5 * h);
  const double kMean = (kD1 + kD2 + kD3) / 3.0;
  HSelection s;
  s.h = h;
  s.deriv = kD1;
  if (!(std::fabs(kMean) > 0.0)) {
    s.rel_spread = 0.0;
    s.ok = true;
    return s;
  }
  const double kS12 = std::fabs(kD2 - kD1) / std::fabs(kMean);
  const double kS13 = std::fabs(kD3 - kD1) / std::fabs(kMean);
  s.rel_spread = (kS12 > kS13) ? kS12 : kS13;
  s.ok = s.rel_spread <= 0.2;
  return s;
}

struct ParetoInteraction {
  std::string a;
  std::string b;
  std::string output;
  std::vector<uint32_t> ranges;
  std::vector<double> residues;  // per range
};

// Per-case driver: OVAT SelectH over every (input, channel, range),
// top-3 interaction pairs per channel with corner residues, artifact +
// interactions file writes. Caches solves by absolute input value so the
// per-(channel, range) SelectH calls share the 6 rung evaluations.
inline void RunParetoCase(const char* cell, const std::vector<uint32_t>& ranges,
                          lob::Builder base,
                          const std::vector<ParetoInput>& inputs,
                          const ParetoFloors& floors, const char* stem,
                          const char* solver_config, bool zero_wind,
                          const std::array<lob::WindPoint, 2>& profile,
                          const std::vector<uint32_t>& inverse_ranges) {
  SensitivityArtifact artifact;
  artifact.provenance_lob_version = lob::Version();
  artifact.provenance_git_sha = LOB_GIT_SHA;
  artifact.solver_config = solver_config;

  std::vector<lob::Output> base_outs;
  {
    const lob::Context kBaseCtx = base.Build();
    ASSERT_EQ(kBaseCtx.error, lob::ErrorT::kNone);
    base_outs.assign(ranges.size(), lob::Output());
    ASSERT_EQ(
        lob::Solve(kBaseCtx, ranges.data(), base_outs.data(), base_outs.size()),
        base_outs.size());
  }

  std::vector<lob::Output> base_inv;
  if (!inverse_ranges.empty()) {
    // Fast-branch precondition (InverseLadder pattern): forward drop must
    // stay above the 1200-in dynamic-branch switch at the inverse ranges.
    for (size_t i = 0; i < ranges.size(); ++i) {
      for (size_t j = 0; j < inverse_ranges.size(); ++j) {
        if (ranges[i] == inverse_ranges[j]) {
          EXPECT_GT(base_outs[i].elevation, -1200.0)
              << "range=" << ranges[i] << " leaves the Fast branch";
        }
      }
    }
    const lob::Context kBaseCtx = base.Build();
    ASSERT_EQ(kBaseCtx.error, lob::ErrorT::kNone);
    base_inv.assign(inverse_ranges.size(), lob::Output());
    ASSERT_EQ(lob::SolveInverse(kBaseCtx, inverse_ranges.data(),
                                base_inv.data(), base_inv.size()),
              base_inv.size());
  }

  // channels: 0..4 forward always; 5 inverse when requested.
  std::vector<int> channels;
  channels.push_back(0);
  channels.push_back(1);
  channels.push_back(2);
  channels.push_back(3);
  channels.push_back(4);
  if (!inverse_ranges.empty()) {
    channels.push_back(5);
  }

  // canned[input][channel_slot][range_slot], h_used[input] (accepted h at the
  // longest range, representative for the interaction corners).
  std::vector<std::vector<std::vector<double> > > canned(
      inputs.size(), std::vector<std::vector<double> >(channels.size()));
  std::vector<double> h_used(inputs.size(), 0.0);
  // per-input solve caches survive into the interaction pass (single-perturb
  // +h values + joint-corner reuse of the same appliers).
  std::vector<std::map<double, std::vector<lob::Output> > > fwd_caches(
      inputs.size());
  std::vector<std::map<double, std::vector<lob::Output> > > inv_caches(
      inputs.size());

  for (size_t n = 0; n < inputs.size(); ++n) {
    const ParetoInput kIn = inputs[n];
    std::map<double, std::vector<lob::Output> >& fwd_cache = fwd_caches[n];
    std::map<double, std::vector<lob::Output> >& inv_cache = inv_caches[n];
    for (size_t c = 0; c < channels.size(); ++c) {
      const int kCh = channels[c];
      const bool kInv = (kCh == 5);
      const std::vector<uint32_t>& kRanges = kInv ? inverse_ranges : ranges;
      std::map<double, std::vector<lob::Output> >& cache =
          kInv ? inv_cache : fwd_cache;
      // Memoized scalar field: SelectH's 6 rung evaluations are solved once
      // per input value and shared across all channels/ranges.
      auto eval_vec = [&](double v) -> const std::vector<lob::Output>& {
        const typename std::map<double,
                                std::vector<lob::Output> >::const_iterator kIt =
            cache.find(v);
        if (kIt != cache.end()) {
          return kIt->second;
        }
        std::vector<lob::Output> outs;
        const bool kOk = ParetoEval(base, kIn.kind, v,
                                    zero_wind && kIn.kind == kParetoWindSpeed,
                                    profile, kRanges, kInv, &outs);
        EXPECT_TRUE(kOk) << cell << " input=" << kIn.name << " v=" << v;
        if (!kOk) {
          outs.assign(kRanges.size(), lob::Output());
        }
        cache[v] = outs;
        return cache.find(v)->second;
      };
      // Pre-warm the rung set so the SelectH lambdas below never miss (the
      // quantum-1.0 velocity seed never collides, so no rung growth occurs;
      // on a miss eval_vec still solves, so correctness never depends on
      // this list being exhaustive).
      if (kIn.one_sided) {
        eval_vec(kIn.x0);
        eval_vec(kIn.x0 + 0.5 * kIn.h_seed);
        eval_vec(kIn.x0 + kIn.h_seed);
        eval_vec(kIn.x0 + 2.0 * kIn.h_seed);
      } else {
        const double kHs = SnapH(kIn.h_seed, kIn.quantum);
        eval_vec(kIn.x0 + kHs);
        eval_vec(kIn.x0 - kHs);
        eval_vec(kIn.x0 + 2.0 * kHs);
        eval_vec(kIn.x0 - 2.0 * kHs);
        eval_vec(kIn.x0 + 0.5 * kHs);
        eval_vec(kIn.x0 - 0.5 * kHs);
      }
      const double kFloor = ParetoFloorFor(floors, kCh);
      canned[n][c].assign(kRanges.size(), 0.0);
      for (size_t r = 0; r < kRanges.size(); ++r) {
        auto scalar = [&](double v) -> double {
          return ParetoExtract(eval_vec(v)[r], kCh);
        };
        HSelection sel;
        if (kIn.one_sided) {
          sel = ParetoForwardSelectH(scalar, kIn.x0, kIn.h_seed);
        } else {
          sel = SelectH(scalar, kIn.x0, kIn.h_seed, kIn.quantum);
        }
        const double kHacc = sel.h;
        double fp = 0.0;
        double fm = 0.0;
        double f2p = 0.0;
        double f2m = 0.0;
        if (kIn.one_sided) {
          fp = scalar(kIn.x0 + kHacc);
          fm = scalar(kIn.x0);
          f2p = scalar(kIn.x0 + 2.0 * kHacc);
          f2m = fm;
        } else {
          fp = scalar(kIn.x0 + kHacc);
          fm = scalar(kIn.x0 - kHacc);
          f2p = scalar(kIn.x0 + 2.0 * kHacc);
          f2m = scalar(kIn.x0 - 2.0 * kHacc);
        }
        EXPECT_TRUE(std::isfinite(fp));
        EXPECT_TRUE(std::isfinite(fm));
        EXPECT_TRUE(std::isfinite(sel.deriv));
        const double kCanned = std::fabs(fp - fm);
        const int kSignH = (fp > fm) ? 1 : -1;
        const int kSign2H = (f2p > f2m) ? 1 : -1;
        const bool kGenuine = GenuineCheck(kCanned, kFloor, kSignH, kSign2H);
        const bool kNonlinear = !sel.ok;
        canned[n][c][r] = kCanned;
        artifact.AddRow(kIn.name, kRanges[r], ParetoChannelName(kCh), kHacc,
                        sel.deriv, kCanned, kNonlinear,
                        kGenuine ? "genuine" : "below_floor");
        if (r + 1 == kRanges.size()) {
          h_used[n] = kHacc;
        }
        std::printf(
            "PARETO %s %s range=%u input=%s h=%.10g canned=%.10g "
            "status=%s nonlinear=%d\n",
            cell, ParetoChannelName(kCh), kRanges[r], kIn.name, kHacc, kCanned,
            kGenuine ? "genuine" : "below_floor", kNonlinear ? 1 : 0);
      }
    }
  }

  // Interactions: rank input pairs per channel by max-over-ranges summed
  // canned response, take the top 3, evaluate the 4 corners f(x+/-h, z+/-h)
  // (one-sided members use {+h, 0} instead of {+h, -h}), and record the
  // residue f(x+h,z+h) - f(x+h) - f(z+h) + f(x) per range.
  std::vector<ParetoInteraction> interactions;
  for (size_t c = 0; c < channels.size(); ++c) {
    const int kCh = channels[c];
    const bool kInv = (kCh == 5);
    const std::vector<uint32_t>& kRanges = kInv ? inverse_ranges : ranges;
    const std::vector<lob::Output>& kBase = kInv ? base_inv : base_outs;
    std::vector<std::pair<double, std::pair<size_t, size_t> > > scored;
    for (size_t a = 0; a < inputs.size(); ++a) {
      for (size_t b = a + 1; b < inputs.size(); ++b) {
        double best = 0.0;
        for (size_t r = 0; r < kRanges.size(); ++r) {
          const double kSum = canned[a][c][r] + canned[b][c][r];
          if (kSum > best) {
            best = kSum;
          }
        }
        scored.push_back(std::make_pair(best, std::make_pair(a, b)));
      }
    }
    std::sort(scored.begin(), scored.end());
    std::reverse(scored.begin(), scored.end());
    size_t n_pairs = 3;
    if (scored.size() < n_pairs) {
      n_pairs = scored.size();
    }
    for (size_t p = 0; p < n_pairs; ++p) {
      const size_t kA = scored[p].second.first;
      const size_t kB = scored[p].second.second;
      const ParetoInput kInA = inputs[kA];
      const ParetoInput kInB = inputs[kB];
      const double kDminusA = kInA.one_sided ? 0.0 : -h_used[kA];
      const double kDminusB = kInB.one_sided ? 0.0 : -h_used[kB];
      // Corner order: (++, +-, -+, --) with the one-sided substitution above.
      const double kDeltas[4][2] = {{h_used[kA], h_used[kB]},
                                    {h_used[kA], kDminusB},
                                    {kDminusA, h_used[kB]},
                                    {kDminusA, kDminusB}};
      std::vector<std::vector<lob::Output> > corners(4);
      for (size_t k = 0; k < 4; ++k) {
        lob::Builder b = base;
        // Caller-owned profile borrow (see ParetoEval).
        std::array<lob::WindPoint, 2> pts = profile;
        if (kInA.kind == kParetoHeight2) {
          ParetoApplyProfile(&b, &pts, kInA.x0 + kDeltas[k][0]);
        } else {
          ParetoApply(&b, kInA.kind, kInA.x0 + kDeltas[k][0],
                      zero_wind && kInA.kind == kParetoWindSpeed);
        }
        if (kInB.kind == kParetoHeight2) {
          ParetoApplyProfile(&b, &pts, kInB.x0 + kDeltas[k][1]);
        } else {
          ParetoApply(&b, kInB.kind, kInB.x0 + kDeltas[k][1],
                      zero_wind && kInB.kind == kParetoWindSpeed);
        }
        const lob::Context kCtx = b.Build();
        ASSERT_EQ(kCtx.error, lob::ErrorT::kNone);
        corners[k].assign(kRanges.size(), lob::Output());
        size_t solved = 0;
        if (kInv) {
          solved = lob::SolveInverse(kCtx, kRanges.data(), corners[k].data(),
                                     corners[k].size());
        } else {
          solved = lob::Solve(kCtx, kRanges.data(), corners[k].data(),
                              corners[k].size());
        }
        ASSERT_EQ(solved, corners[k].size());
      }
      ParetoInteraction rec;
      rec.a = kInA.name;
      rec.b = kInB.name;
      rec.output = ParetoChannelName(kCh);
      rec.ranges = kRanges;
      rec.residues.assign(kRanges.size(), 0.0);
      const std::map<double, std::vector<lob::Output> >& kCacheA =
          kInv ? inv_caches[kA] : fwd_caches[kA];
      const std::map<double, std::vector<lob::Output> >& kCacheB =
          kInv ? inv_caches[kB] : fwd_caches[kB];
      const double kVA = kInA.x0 + h_used[kA];
      const double kVB = kInB.x0 + h_used[kB];
      ASSERT_TRUE(kCacheA.find(kVA) != kCacheA.end());
      ASSERT_TRUE(kCacheB.find(kVB) != kCacheB.end());
      for (size_t r = 0; r < kRanges.size(); ++r) {
        const double kFpp = ParetoExtract(corners[0][r], kCh);
        const double kFp = ParetoExtract(kCacheA.find(kVA)->second[r], kCh);
        const double kFz = ParetoExtract(kCacheB.find(kVB)->second[r], kCh);
        const double kF0 = ParetoExtract(kBase[r], kCh);
        const double kRes = kFpp - kFp - kFz + kF0;
        EXPECT_TRUE(std::isfinite(kRes));
        rec.residues[r] = kRes;
        std::printf("INTERACTION %s %s pair=%s,%s range=%u residue=%.10g\n",
                    cell, ParetoChannelName(kCh), kInA.name, kInB.name,
                    kRanges[r], kRes);
      }
      interactions.push_back(rec);
    }
  }

  ASSERT_TRUE(artifact.WriteFiles(LOB_VALIDATION_DIR, stem))
      << "artifact write failed for " << cell;
  {
    // Sibling interactions file: SensitivityArtifact is frozen (Tasks 1-4
    // review-approved), so the plan's interactions[] array lands here for
    // the pareto.json transcription.
    std::ostringstream os;
    os << "{\"provenance\":{\"lob_version\":\""
       << JsonEscape(artifact.provenance_lob_version) << "\",\"git_sha\":\""
       << JsonEscape(artifact.provenance_git_sha) << "\"},\"solver_config\":\""
       << JsonEscape(artifact.solver_config) << "\",\"interactions\":[";
    for (size_t i = 0; i < interactions.size(); ++i) {
      if (i > 0) {
        os << ",";
      }
      const ParetoInteraction& kRec = interactions[i];
      os << "{\"pair\":[\"" << JsonEscape(kRec.a) << "\",\""
         << JsonEscape(kRec.b) << "\"],\"output\":\"" << JsonEscape(kRec.output)
         << "\",\"residues\":[";
      for (size_t r = 0; r < kRec.ranges.size(); ++r) {
        if (r > 0) {
          os << ",";
        }
        os << "{\"range_ft\":" << kRec.ranges[r]
           << ",\"residue\":" << JsonDouble(kRec.residues[r]) << "}";
      }
      os << "]}";
    }
    os << "]}";
    const std::string kPath =
        std::string(LOB_VALIDATION_DIR) + "/" + stem + "_interactions.json";
    std::ofstream out(kPath.c_str());
    ASSERT_TRUE(out.is_open()) << "cannot open " << kPath;
    out << os.str();
    out.close();
  }
  EXPECT_GT(artifact.rows.size(), 0U);
}

inline bool ParetoEnvGated() {
  const char* kGate = std::getenv("LOB_FULL_PARETO");
  return kGate != nullptr && std::string(kGate) == "1";
}

}  // namespace

TEST(SensitivityFullPareto, C1) {
  if (!ParetoEnvGated()) {
    GTEST_SKIP() << "offline only: set LOB_FULL_PARETO=1";
  }
  // Noise floors mirror test/validation/baselines/floors.json cell "C1-ICAO"
  // floors_18_9. Deflection floor is exactly 0.0 (the 1e-12 entry is the
  // ceilings guard, not the noise model). velocity_fps floor is 1.0, the
  // numeric form of the "1 LSB (U16 truncation)" entry. inverse_moa reuses
  // the elevation_moa floor: no inverse-channel floor cell exists for C1.
  const ParetoFloors kFloors = {4.15814e-05, 3.97148e-06, 0.0,
                                1.0,         6.12488e-08, 3.97148e-06};
  const std::vector<uint32_t> kRanges = {300U, 900U, 1800U, 3000U};
  const std::vector<uint32_t> kInvRanges = {900U, 1800U};
  // length_in borrows the C8 1.215 baseline: C1 sets no length (NaN) and no
  // twist, so the spin path stays off and both +/-h evals set an explicit
  // length; expect below_floor (inert without twist).
  const std::vector<ParetoInput> kInputs = {
      {"velocity_fps", kParetoVelocity, 2800.0, 10.0, 1.0, false},
      {"bc_psi", kParetoBc, 0.232, 0.00232, 0.0, false},
      {"zero_angle_moa", kParetoZero, 3.66, 0.05, 0.0, false},
      {"optic_height_in", kParetoOptic, 1.5, 0.1, 0.0, false},
      {"pressure_inhg", kParetoPressure, 29.92, 0.1, 0.0, false},
      {"temperature_degf", kParetoTemp, 59.0, 2.0, 0.0, false},
      {"humidity_pp", kParetoHumidity, 0.0, 5.0, 0.0, true},
      {"mass_grains", kParetoMass, 155.0, 1.0, 0.0, false},
      {"diameter_in", kParetoDiameter, 0.308, 0.002, 0.0, false},
      {"length_in", kParetoLength, 1.215, 0.002, 0.0, false},
      {"wind_speed_mph", kParetoWindSpeed, 0.0, 1.0, 0.0, false},
      {"wind_heading_deg", kParetoWindHeading, 0.0, 2.0, 0.0, false},
  };
  const std::array<lob::WindPoint, 2> kProfile = {};
  RunParetoCase("C1-ICAO", kRanges, MakeC1IcaoBuilder(), kInputs, kFloors,
                "sensitivity_C1",
                "step_in=36,angle_tol_moa=0.01,density_path=fast,"
                "ranges_ft=300,900,1800,3000,canned=|f(x+h)-f(x-h)| full "
                "symmetric swing at accepted h; one-sided |f(x+h)-f(x)| for "
                "humidity_pp; C1 wind_speed uses heading-pair (90@+h vs "
                "270@+h); inverse_moa at 900,1800 via SolveInverse",
                true, kProfile, kInvRanges);
}

TEST(SensitivityFullPareto, C5) {
  if (!ParetoEnvGated()) {
    GTEST_SKIP() << "offline only: set LOB_FULL_PARETO=1";
  }
  // Floors mirror floors.json cell "C5-uniform" floors_18_9; velocity floor
  // 1.0 = "1 LSB (U16 truncation)".
  const ParetoFloors kFloors = {5.342835971e-05, 5.669994664e-06,
                                7.577461467e-07, 1.0,
                                8.11391121e-08,  0.0};
  const std::vector<uint32_t> kRanges = {900U, 1800U, 2700U};
  lob::Builder base = ParetoWindBaseBuilder();
  base.WindHeading(lob::ClockAngleT::kIII).WindSpeedMph(5.0);
  const std::vector<ParetoInput> kInputs = {
      {"velocity_fps", kParetoVelocity, 2720.0, 10.0, 1.0, false},
      {"bc_psi", kParetoBc, 0.372, 0.00372, 0.0, false},
      {"zero_angle_moa", kParetoZero, 4.78, 0.05, 0.0, false},
      {"optic_height_in", kParetoOptic, 2.5, 0.1, 0.0, false},
      {"pressure_inhg", kParetoPressure, 29.92, 0.1, 0.0, false},
      {"temperature_degf", kParetoTemp, 59.0, 2.0, 0.0, false},
      {"humidity_pp", kParetoHumidity, 0.0, 5.0, 0.0, true},
      {"mass_grains", kParetoMass, 77.0, 1.0, 0.0, false},
      {"diameter_in", kParetoDiameter, 0.224, 0.002, 0.0, false},
      {"length_in", kParetoLength, 1.215, 0.002, 0.0, false},
      {"wind_speed_mph", kParetoWindSpeed, 5.0, 1.0, 0.0, false},
      {"wind_heading_deg", kParetoWindHeading, 90.0, 2.0, 0.0, false},
  };
  const std::array<lob::WindPoint, 2> kProfile = {};
  RunParetoCase("C5-uniform", kRanges, base, kInputs, kFloors, "sensitivity_C5",
                "step_in=36,angle_tol_moa=0.01,density_path=fast,"
                "ranges_ft=900,1800,2700,canned=|f(x+h)-f(x-h)| full "
                "symmetric swing at accepted h; one-sided |f(x+h)-f(x)| for "
                "humidity_pp; base is the exact C5-uniform survey builder",
                false, kProfile, std::vector<uint32_t>());
}

TEST(SensitivityFullPareto, C8) {
  if (!ParetoEnvGated()) {
    GTEST_SKIP() << "offline only: set LOB_FULL_PARETO=1";
  }
  // Floors mirror floors.json cell "C8-Litz" floors_18_9; velocity floor
  // 1.0 = "1 LSB (U16 truncation)".
  const ParetoFloors kFloors = {2.799664696e-05, 2.971096993e-06,
                                9.767386047e-07, 1.0,
                                4.926922315e-08, 0.0};
  const std::vector<uint32_t> kRanges = {900U, 1800U, 2700U};
  const std::vector<ParetoInput> kInputs = {
      {"velocity_fps", kParetoVelocity, 3100.0, 10.0, 1.0, false},
      {"bc_psi", kParetoBc, 0.436, 0.00436, 0.0, false},
      {"zero_angle_moa", kParetoZero, 6.11, 0.05, 0.0, false},
      {"optic_height_in", kParetoOptic, 1.5, 0.1, 0.0, false},
      {"pressure_inhg", kParetoPressure, 29.92, 0.1, 0.0, false},
      {"temperature_degf", kParetoTemp, 59.0, 2.0, 0.0, false},
      {"humidity_pp", kParetoHumidity, 0.0, 5.0, 0.0, true},
      {"mass_grains", kParetoMass, 168.0, 1.0, 0.0, false},
      {"diameter_in", kParetoDiameter, 0.308, 0.002, 0.0, false},
      {"length_in", kParetoLength, 1.215, 0.002, 0.0, false},
      {"twist_in_per_turn", kParetoTwist, 10.0, 0.5, 0.0, false},
      {"wind_speed_mph", kParetoWindSpeed, 10.0, 1.0, 0.0, false},
      {"wind_heading_deg", kParetoWindHeading, 90.0, 2.0, 0.0, false},
  };
  const std::array<lob::WindPoint, 2> kProfile = {};
  RunParetoCase("C8-Litz", kRanges, ParetoC8LitzBuilder(), kInputs, kFloors,
                "sensitivity_C8",
                "step_in=36,angle_tol_moa=0.01,density_path=fast,"
                "ranges_ft=900,1800,2700,canned=|f(x+h)-f(x-h)| full "
                "symmetric swing at accepted h; one-sided |f(x+h)-f(x)| for "
                "humidity_pp; base is the exact C8-Litz survey builder",
                false, kProfile, std::vector<uint32_t>());
}

TEST(SensitivityFullPareto, C6Shear) {
  if (!ParetoEnvGated()) {
    GTEST_SKIP() << "offline only: set LOB_FULL_PARETO=1";
  }
  // Shear extension on the exact C6-scaled survey builder (wind-base +
  // two-point 90-degree profile + shear 0.25). Heights pruned everywhere
  // else per the section 9.1 inert-at-alpha-0 rule; only station-2 height
  // is perturbed here. Floors mirror floors.json cell "C6-scaled".
  const ParetoFloors kFloors = {5.361285491e-05, 5.689573904e-06,
                                1.477174983e-06, 1.0,
                                8.137269125e-08, 0.0};
  const std::vector<uint32_t> kRanges = {900U, 1800U, 2700U};
  const std::array<lob::WindPoint, 2> kTwoPoint = {{
      {0.0, 90.0, 5.0, std::numeric_limits<double>::quiet_NaN()},
      {1500.0, 90.0, 10.0, 6.0},
  }};
  lob::Builder base = ParetoWindBaseBuilder();
  base.WindProfile(kTwoPoint).WindShearExponent(0.25);
  const std::vector<ParetoInput> kInputs = {
      {"shear_exponent", kParetoShear, 0.25, 0.02, 0.0, true},
      {"height_ft", kParetoHeight2, 6.0, 1.0, 0.0, false},
  };
  RunParetoCase(
      "C6-shear", kRanges, base, kInputs, kFloors, "sensitivity_C6shear",
      "step_in=36,angle_tol_moa=0.01,density_path=fast,"
      "wind_shear_exponent=0.25,ranges_ft=900,1800,2700,canned=|f(x+h)"
      "-f(x-h)| full symmetric swing at accepted h; one-sided "
      "|f(x+h)-f(x)| for shear_exponent; base is the exact "
      "C6-scaled survey builder",
      false, kTwoPoint, std::vector<uint32_t>());
}

}  // namespace tests
