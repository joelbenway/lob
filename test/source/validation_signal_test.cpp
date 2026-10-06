// Copyright (c) 2026  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <sstream>
#include <string>
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
  EXPECT_EQ(tests::ClassifySigBand(0.01), tests::SigBand::kIndistinguishable);
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
// ---- Task 3: offline effect-matrix driver (always run) ----
#ifndef LOB_VALIDATION_DIR
#error "LOB_VALIDATION_DIR must be defined by CMake"
#endif
#ifndef LOB_GIT_SHA
#error "LOB_GIT_SHA must be defined by CMake"
#endif
#ifndef LOB_VALIDATION_CASES_DIR
#error "LOB_VALIDATION_CASES_DIR must be defined by CMake"
#endif

namespace {

// Yardstick u_num values: floors.json floors_18_9, hardcoded with cross-links
// at each use (Task 1 pattern). C7 has no floors cell — it aliases C1-ICAO
// (same solver path).
constexpr double kUNumC1ElevIn = 4.15814e-05;
constexpr double kUNumC1DeflMoa = 0.0;  // 0.0 observed 2026-09-27
constexpr double kUNumC8ElevIn = 2.799664696e-05;
constexpr double kUNumC8DeflMoa = 9.767386047e-07;
constexpr double kUNumC6ElevIn = 5.361285491e-05;
constexpr double kUNumC6DeflMoa = 1.477174983e-06;
constexpr double kUNumC5ElevIn = 5.342835971e-05;
constexpr double kUNumC5DeflMoa = 7.577461467e-07;

// MC sigmas live in an optional flat file the MC stage may drop next to the
// other validation artifacts: {"elevation_in": s, "deflection_moa": s}.
// Absent or unparsable -> every row honestly carries mc_sigma_absent; the
// driver never fails for missing MC.
inline nlohmann::json LoadMcSigmas() {
  nlohmann::json mc;
  const std::ifstream kIn(
      (std::string(LOB_VALIDATION_DIR) + "/mc_sigma.json").c_str());
  if (!kIn) {
    return mc;
  }
  std::ostringstream raw;
  raw << kIn.rdbuf();
  try {
    mc = nlohmann::json::parse(raw.str());
  } catch (const nlohmann::json::exception&) {
    mc = nlohmann::json();
  }
  if (!mc.is_object()) {
    mc = nlohmann::json();
  }
  return mc;
}

inline bool McSigmaFor(const nlohmann::json& mc, const std::string& channel,
                       double* sigma) {
  try {
    if (!mc.is_object() || mc.count(channel) == 0) {
      return false;
    }
    if (!mc.at(channel).is_number()) {
      return false;
    }
    *sigma = mc.at(channel).get<double>();
    return true;
  } catch (const nlohmann::json::exception&) {
    return false;
  }
}

// One yardstick-carrying row: R_sig is resolution-relative (u_num +
// granularity [+ MC sigma where nonlinear and available]), never a full
// prediction-uncertainty significance (u_c: null everywhere in Phase 6).
inline nlohmann::json MakeSigRow(uint32_t range_ft, const std::string& channel,
                                 double delta, double u_num, double granularity,
                                 const nlohmann::json& mc,
                                 const std::string& u_num_source) {
  double mc_sigma = std::numeric_limits<double>::quiet_NaN();
  McSigmaFor(mc, channel, &mc_sigma);
  const double kUTotal = tests::UTotal(u_num, granularity, mc_sigma);
  const double kR = tests::RSig(delta, kUTotal);
  nlohmann::json row;
  row["range_ft"] = range_ft;
  row["channel"] = channel;
  row["delta"] = delta;
  row["u_total"] = kUTotal;
  if (std::isnan(kR)) {
    row["r_sig"] = nullptr;
    row["band"] = "unknown-zero-yardstick";
  } else {
    row["r_sig"] = kR;
    row["band"] = SigBandName(tests::ClassifySigBand(kR));
  }
  nlohmann::json yardstick;
  yardstick["u_num_source"] = u_num_source;
  yardstick["granularity"] = granularity;
  if (McSigmaFor(mc, channel, &mc_sigma)) {
    yardstick["mc_sigma_or_absent"] = mc_sigma;
  } else {
    yardstick["mc_sigma_or_absent"] = "mc_sigma_absent";
  }
  yardstick["u_c"] = nullptr;
  row["yardstick"] = yardstick;
  return row;
}

inline void WriteSignalDoc(const std::string& dir, const std::string& stem,
                           const std::string& effect, const std::string& cell,
                           const std::string& isolation,
                           const std::string& expectation,
                           const nlohmann::json& rows,
                           const nlohmann::json& extra) {
  nlohmann::json doc;
  doc["provenance"] = {{"git_sha", LOB_GIT_SHA},
                       {"generator", "SignalMatrix.OfflineEffectMatrix"},
                       {"gate", "LOB_FULL_SIGNAL=1"}};
  doc["effect"] = effect;
  doc["case"] = cell;
  doc["isolation"] = isolation;
  doc["expectation"] = expectation;
  doc["rows"] = rows;
  for (auto it = extra.begin(); it != extra.end(); ++it) {
    doc[it.key()] = it.value();
  }
  std::ofstream out((dir + "/" + stem).c_str());
  EXPECT_TRUE(out.good()) << "cannot open " << dir << "/" << stem;
  out << doc.dump(2) << "\n";
  out.close();
}

template <size_t N>
inline std::vector<double> ElevInVec(const std::array<lob::Output, N>& outs) {
  std::vector<double> vals;
  vals.reserve(N);
  for (size_t i = 0; i < N; ++i) {
    vals.push_back(outs.at(i).elevation);
  }
  return vals;
}

template <size_t N>
inline std::vector<double> DeflMoaVec(const std::array<lob::Output, N>& outs) {
  std::vector<double> vals;
  vals.reserve(N);
  for (size_t i = 0; i < N; ++i) {
    vals.push_back(lob::InchToMoa(outs.at(i).deflection,
                                  static_cast<double>(outs.at(i).range)));
  }
  return vals;
}

// C1 at an explicit step (effect 1 control rungs).
inline lob::Context BuildC1AtStep(uint16_t step_in) {
  return tests::BuildAtStep(tests::MakeC1IcaoBuilder(), step_in);
}

// C1 with G1 instead of G7 (effect 8 screaming-signal control).
inline lob::Context BuildC1G1() {
  lob::Builder b = tests::MakeC1IcaoBuilder();
  return b.BCDragFunction(lob::DragFunctionT::kG1).Build();
}

// C1 single-BC vs velocity bands 0.20-0.40 (effect 7: band spread large by
// construction, mirroring the BCVelocityBands builder-test shape).
inline lob::Context BuildC1Bands() {
  const std::array<float, 3> kFps = {2000.0F, 2500.0F, 3000.0F};
  const std::array<float, 3> kBcs = {0.20F, 0.30F, 0.40F};
  lob::Builder b = tests::MakeC1IcaoBuilder();
  return b.BCVelocityBands(kFps, kBcs).Build();
}

// C1 with a compact custom G7-faithful table (effect 9 baseline). Mach/Cd
// pairs transcribed from source/tables.hpp kMachs/kG7Drags grid points
// (indices 0,10,18,23,26,30,34,41,51,61,71,78,86); spans 0..5 so no
// extrapolation padding applies. PUBLIC MachVsDragTable route — never direct
// drags[] edits (builder contract).
constexpr size_t kSplineTableSize = 13;
constexpr size_t kSplineTransonicBegin = 2;  // Mach 0.8 entry
constexpr size_t kSplineTransonicEnd = 6;    // Mach 1.2 entry
constexpr float kSplineShiftCd = 5e-3F;      // §8.6 perturbation
inline void SplineBaseTable(std::array<float, kSplineTableSize>* machs,
                            std::array<float, kSplineTableSize>* drags) {
  const std::array<float, kSplineTableSize> kMach = {
      {0.0F, 0.5F, 0.8F, 0.925F, 1.0F, 1.1F, 1.2F, 1.5F, 2.0F, 2.5F, 3.0F, 3.7F,
       5.0F}};
  const std::array<float, kSplineTableSize> kDrag = {
      {0.11980F, 0.11940F, 0.12420F, 0.16600F, 0.38030F, 0.40140F, 0.38840F,
       0.34400F, 0.29800F, 0.26700F, 0.24240F, 0.20600F, 0.16180F}};
  *machs = kMach;
  *drags = kDrag;
}

inline lob::Context BuildC1Spline(bool shifted) {
  std::array<float, kSplineTableSize> machs{};
  std::array<float, kSplineTableSize> drags{};
  SplineBaseTable(&machs, &drags);
  if (shifted) {
    // +/-5e-3 Cd shift on the transonic band (0.8-1.2) — the §8.6 mapping.
    for (size_t idx = kSplineTransonicBegin; idx <= kSplineTransonicEnd;
         ++idx) {
      drags.at(idx) += kSplineShiftCd;
    }
  }
  lob::Builder b = tests::MakeC1IcaoBuilder();
  return b.BallisticCoefficientPsi(std::numeric_limits<double>::quiet_NaN())
      .MachVsDragTable(machs, drags)
      .Build();
}

// C7-style coriolis: C1 geometry plus lat 45° and a firing azimuth; omitting
// both is the off state (ISOLATED — lat/az only feed the coriolis path).
inline lob::Context BuildC7(double azimuth_deg, bool with_coriolis) {
  constexpr double kLatitudeDeg = 45.0;
  lob::Builder b = tests::MakeC1IcaoBuilder();
  if (with_coriolis) {
    b.LatitudeDeg(kLatitudeDeg).AzimuthDeg(azimuth_deg);
  }
  return b.Build();
}

// C8 base: mirrors BuildLitzContext above (the C8-Litz survey geometry);
// twist on/off selects the Litz path; wind selects the jump path.
inline lob::Context BuildC8(double wind_mph, bool with_twist) {
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
      .MassGrains(kMassGrains);
  if (with_twist) {
    b.TwistInchesPerTurn(kTwistIn);
  }
  return b.WindHeading(lob::ClockAngleT::kIII).WindSpeedMph(wind_mph).Build();
}

// C8 Boatright variant: same base plus nose/tail geometry (values from the
// spin-drift fixture in test/source/lob_spin_drift_test.cpp), taking the
// Boatright (non-NaN spindrift_factor) path.
inline lob::Context BuildC8Boatright(double wind_mph) {
  constexpr double kBcPsi = 0.436;
  constexpr uint16_t kVelocityFps = 3100U;
  constexpr double kZeroAngleMoa = 6.11;
  constexpr double kDiameterInch = 0.308;
  constexpr double kLengthInch = 1.215;
  constexpr double kMassGrains = 168.0;
  constexpr double kTwistIn = 10.0;
  constexpr double kNoseLengthInch = 0.748;
  constexpr double kTailLengthInch = 0.257;
  constexpr double kBaseDiameterInch = 0.276;
  constexpr double kMeplatDiameterInch = 0.069;
  constexpr double kOgiveRtR = 0.99;
  lob::Builder b;
  return b.BallisticCoefficientPsi(kBcPsi)
      .InitialVelocityFps(kVelocityFps)
      .ZeroAngleMOA(kZeroAngleMoa)
      .DiameterInch(kDiameterInch)
      .LengthInch(kLengthInch)
      .MassGrains(kMassGrains)
      .TwistInchesPerTurn(kTwistIn)
      .NoseLengthInch(kNoseLengthInch)
      .TailLengthInch(kTailLengthInch)
      .BaseDiameterInch(kBaseDiameterInch)
      .MeplatDiameterInch(kMeplatDiameterInch)
      .OgiveRtR(kOgiveRtR)
      .WindHeading(lob::ClockAngleT::kIII)
      .WindSpeedMph(wind_mph)
      .Build();
}

// C6 profile station set: measured heights (first station NaN height, as in
// the convergence C6) vs all-NaN heights (effect 12); shear exponent and
// incline select effects 10/11/13.
inline lob::Context BuildC6(double shear_exponent, double incline_deg,
                            bool nan_heights) {
  constexpr double kBcPsi = 0.372;
  constexpr double kDiameterInch = 0.224;
  constexpr double kMassGrains = 77.0;
  constexpr int kVelocityFps = 2720;
  constexpr double kZeroAngleMoa = 4.78;
  constexpr double kOpticHeightIn = 2.5;
  constexpr double kCrosswindHeadingDeg = 90.0;
  constexpr double kMuzzleWindMph = 5.0;
  constexpr double kAloftWindMph = 10.0;
  constexpr double kAloftRangeFt = 1500.0;
  constexpr double kAloftHeightFt = 6.0;
  const double kSecondHeight =
      nan_heights ? std::numeric_limits<double>::quiet_NaN() : kAloftHeightFt;
  const std::array<lob::WindPoint, 2> kTwoPoint = {{
      {0.0, kCrosswindHeadingDeg, kMuzzleWindMph,
       std::numeric_limits<double>::quiet_NaN()},
      {kAloftRangeFt, kCrosswindHeadingDeg, kAloftWindMph, kSecondHeight},
  }};
  lob::Builder b;
  return b.BallisticCoefficientPsi(kBcPsi)
      .BCDragFunction(lob::DragFunctionT::kG1)
      .DiameterInch(kDiameterInch)
      .MassGrains(kMassGrains)
      .InitialVelocityFps(kVelocityFps)
      .ZeroAngleMOA(kZeroAngleMoa)
      .OpticHeightInches(kOpticHeightIn)
      .WindProfile(kTwoPoint)
      .WindShearExponent(shear_exponent)
      .RangeAngleDeg(incline_deg)
      .Build();
}

inline lob::Context BuildC6Uniform() {
  constexpr double kBcPsi = 0.372;
  constexpr double kDiameterInch = 0.224;
  constexpr double kMassGrains = 77.0;
  constexpr int kVelocityFps = 2720;
  constexpr double kZeroAngleMoa = 4.78;
  constexpr double kOpticHeightIn = 2.5;
  constexpr double kCrosswindHeadingDeg = 90.0;
  constexpr double kMuzzleWindMph = 5.0;
  lob::Builder b;
  return b.BallisticCoefficientPsi(kBcPsi)
      .BCDragFunction(lob::DragFunctionT::kG1)
      .DiameterInch(kDiameterInch)
      .MassGrains(kMassGrains)
      .InitialVelocityFps(kVelocityFps)
      .ZeroAngleMOA(kZeroAngleMoa)
      .OpticHeightInches(kOpticHeightIn)
      .WindHeadingDeg(kCrosswindHeadingDeg)
      .WindSpeedMph(kMuzzleWindMph)
      .Build();
}

// C6c high-arc: mortar zero angle with a scaling profile (mirrors
// CeilingClampCompletesHighArcSolve in test/source/lob_wind_profile_test.cpp
// — queries above the 300-ft band clamp to it).
inline lob::Context BuildC6cMortar() {
  constexpr double kBcPsi = 0.372;
  constexpr double kDiameterInch = 0.224;
  constexpr double kMassGrains = 77.0;
  constexpr int kVelocityFps = 2720;
  constexpr double kMortarZeroMoa = 500.0;
  constexpr double kOpticHeightIn = 2.5;
  constexpr double kCrosswindHeadingDeg = 90.0;
  constexpr double kWindMph = 10.0;
  constexpr double kFarRangeFt = 3000.0;
  constexpr double kShearExponent = 0.25;
  const std::array<lob::WindPoint, 2> kPts = {{
      {0.0, kCrosswindHeadingDeg, kWindMph,
       std::numeric_limits<double>::quiet_NaN()},
      {kFarRangeFt, kCrosswindHeadingDeg, kWindMph,
       std::numeric_limits<double>::quiet_NaN()},
  }};
  lob::Builder b;
  b.BallisticCoefficientPsi(kBcPsi)
      .BCDragFunction(lob::DragFunctionT::kG1)
      .DiameterInch(kDiameterInch)
      .MassGrains(kMassGrains)
      .InitialVelocityFps(kVelocityFps)
      .ZeroAngleMOA(kMortarZeroMoa)
      .OpticHeightInches(kOpticHeightIn);
  return b.WindProfile(kPts).WindShearExponent(kShearExponent).Build();
}

// Byte-compare helper for the duplicate-vector check: dumps the "expected"
// array of a reference case file. True when both files parsed.
inline bool DumpExpected(const std::string& path, std::string* dump) {
  const std::ifstream kIn(path.c_str());
  if (!kIn) {
    return false;
  }
  std::ostringstream raw;
  raw << kIn.rdbuf();
  try {
    const nlohmann::json kRoot = nlohmann::json::parse(raw.str());
    *dump = kRoot.at("expected").dump();
    return true;
  } catch (const nlohmann::json::exception&) {
    return false;
  }
}

}  // namespace

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
TEST(SignalMatrix, OfflineEffectMatrix) {
  const std::string kDir = LOB_VALIDATION_DIR;
  const nlohmann::json kMc = LoadMcSigmas();
  const nlohmann::json kNoExtra;

  // Effect 1 (ISOLATED control): 36-in vs 9-in step on C1 — expect <<1
  // everywhere (proves the harness sees "nothing" correctly).
  {
    const std::array<uint32_t, 4> kRanges = {300U, 900U, 1800U, 3000U};
    std::array<lob::Output, 4> coarse{};
    std::array<lob::Output, 4> fine{};
    ASSERT_EQ(lob::Solve(BuildC1AtStep(36U), kRanges, &coarse), kRanges.size());
    ASSERT_EQ(lob::Solve(BuildC1AtStep(9U), kRanges, &fine), kRanges.size());
    const std::vector<double> kElevDelta =
        tests::EffectDelta(ElevInVec(fine), ElevInVec(coarse));
    const std::vector<double> kDeflDelta =
        tests::EffectDelta(DeflMoaVec(fine), DeflMoaVec(coarse));
    // Yardstick cross-link: floors.json C1-ICAO floors_18_9 (deflection 0.0
    // observed — granularity carries that yardstick); granularity from
    // testing.hpp kElevFloorIn/kMoaFloor.
    nlohmann::json rows = nlohmann::json::array();
    for (size_t i = 0; i < kRanges.size(); ++i) {
      rows.push_back(
          MakeSigRow(kRanges.at(i), "elevation_in", kElevDelta.at(i),
                     kUNumC1ElevIn, tests::kElevFloorIn, kMc,
                     "floors.json C1-ICAO floors_18_9.elevation_in"));
      rows.push_back(MakeSigRow(kRanges.at(i), "deflection_moa",
                                kDeflDelta.at(i), kUNumC1DeflMoa,
                                tests::kMoaFloor, kMc,
                                "floors.json C1-ICAO floors_18_9."
                                "deflection_moa (=0.0 observed)"));
    }
    WriteSignalDoc(kDir, "signal_step-36-to-9_C1.json", "step-36-to-9", "C1",
                   "ISOLATED", "R_sig << 1 everywhere (harness control)", rows,
                   kNoExtra);
  }

  // Effect 4 (ISOLATED): coriolis on/off, C7-style lat 45° x N/S/E/W.
  // Expect >>1 on deflection, ~1-or-below on elevation. Yardstick: C1-ICAO
  // (same solver path; no C7 floors cell exists).
  {
    const std::array<uint32_t, 4> kRanges = {300U, 900U, 1800U, 3000U};
    const std::array<double, 4> kAzimuths = {0.0, 90.0, 180.0, 270.0};
    const std::array<const char*, 4> kNames = {"N", "E", "S", "W"};
    nlohmann::json rows = nlohmann::json::array();
    for (size_t az = 0; az < kAzimuths.size(); ++az) {
      std::array<lob::Output, 4> on{};
      std::array<lob::Output, 4> off{};
      ASSERT_EQ(lob::Solve(BuildC7(kAzimuths.at(az), true), kRanges, &on),
                kRanges.size())
          << "az=" << kNames.at(az);
      ASSERT_EQ(lob::Solve(BuildC7(kAzimuths.at(az), false), kRanges, &off),
                kRanges.size())
          << "az=" << kNames.at(az);
      const std::vector<double> kElevDelta =
          tests::EffectDelta(ElevInVec(on), ElevInVec(off));
      const std::vector<double> kDeflDelta =
          tests::EffectDelta(DeflMoaVec(on), DeflMoaVec(off));
      for (size_t i = 0; i < kRanges.size(); ++i) {
        nlohmann::json elev = MakeSigRow(
            kRanges.at(i), "elevation_in", kElevDelta.at(i), kUNumC1ElevIn,
            tests::kElevFloorIn, kMc,
            "floors.json C1-ICAO floors_18_9.elevation_in (C7 yardstick "
            "alias: same solver path)");
        elev["direction"] = kNames.at(az);
        rows.push_back(elev);
        nlohmann::json defl = MakeSigRow(
            kRanges.at(i), "deflection_moa", kDeflDelta.at(i), kUNumC1DeflMoa,
            tests::kMoaFloor, kMc,
            "floors.json C1-ICAO floors_18_9.deflection_moa (=0.0 observed; "
            "C7 yardstick alias: same solver path)");
        defl["direction"] = kNames.at(az);
        rows.push_back(defl);
      }
    }
    WriteSignalDoc(kDir, "signal_coriolis-on-off_C7.json", "coriolis-on-off",
                   "C7", "ISOLATED",
                   "R_sig >> 1 on deflection, ~1-or-below on elevation", rows,
                   kNoExtra);
  }

  // Effect 5 (JOINT x2): Litz-vs-off and Boatright-vs-Litz on C8 geometry.
  // The twist input enables drift+jump together by construction — reported
  // JOINT, never isolated. Litz-vs-off expect >>1 deflection (twist share
  // genuine); Boatright-vs-Litz has NO expectation (first gap measurement).
  {
    const std::array<uint32_t, 3> kRanges = {900U, 1800U, 2700U};
    const lob::Context kLitz = BuildC8(10.0, /*with_twist=*/true);
    ASSERT_EQ(kLitz.error, lob::ErrorT::kNone);
    EXPECT_TRUE(std::isnan(kLitz.spindrift_factor));
    const lob::Context kOff = BuildC8(10.0, /*with_twist=*/false);
    ASSERT_EQ(kOff.error, lob::ErrorT::kNone);
    const lob::Context kBoatright = BuildC8Boatright(10.0);
    ASSERT_EQ(kBoatright.error, lob::ErrorT::kNone);
    EXPECT_FALSE(std::isnan(kBoatright.spindrift_factor));
    std::array<lob::Output, 3> litz{};
    std::array<lob::Output, 3> off{};
    std::array<lob::Output, 3> boat{};
    ASSERT_EQ(lob::Solve(kLitz, kRanges, &litz), kRanges.size());
    ASSERT_EQ(lob::Solve(kOff, kRanges, &off), kRanges.size());
    ASSERT_EQ(lob::Solve(kBoatright, kRanges, &boat), kRanges.size());
    // Yardstick cross-link: floors.json C8-Litz floors_18_9.
    nlohmann::json rows_litz = nlohmann::json::array();
    nlohmann::json rows_boat = nlohmann::json::array();
    const std::vector<double> kLitzElev =
        tests::EffectDelta(ElevInVec(litz), ElevInVec(off));
    const std::vector<double> kLitzDefl =
        tests::EffectDelta(DeflMoaVec(litz), DeflMoaVec(off));
    const std::vector<double> kBoatElev =
        tests::EffectDelta(ElevInVec(boat), ElevInVec(litz));
    const std::vector<double> kBoatDefl =
        tests::EffectDelta(DeflMoaVec(boat), DeflMoaVec(litz));
    for (size_t i = 0; i < kRanges.size(); ++i) {
      rows_litz.push_back(MakeSigRow(kRanges.at(i), "elevation_in",
                                     kLitzElev.at(i), kUNumC8ElevIn,
                                     tests::kElevFloorIn, kMc,
                                     "floors.json C8-Litz "
                                     "floors_18_9.elevation_in"));
      rows_litz.push_back(MakeSigRow(kRanges.at(i), "deflection_moa",
                                     kLitzDefl.at(i), kUNumC8DeflMoa,
                                     tests::kMoaFloor, kMc,
                                     "floors.json C8-Litz "
                                     "floors_18_9.deflection_moa"));
      rows_boat.push_back(MakeSigRow(kRanges.at(i), "elevation_in",
                                     kBoatElev.at(i), kUNumC8ElevIn,
                                     tests::kElevFloorIn, kMc,
                                     "floors.json C8-Litz "
                                     "floors_18_9.elevation_in"));
      rows_boat.push_back(MakeSigRow(kRanges.at(i), "deflection_moa",
                                     kBoatDefl.at(i), kUNumC8DeflMoa,
                                     tests::kMoaFloor, kMc,
                                     "floors.json C8-Litz "
                                     "floors_18_9.deflection_moa"));
    }
    WriteSignalDoc(kDir, "signal_spin-litz-vs-off_C8.json", "spin-litz-vs-off",
                   "C8", "JOINT",
                   "R_sig >> 1 on deflection (twist share genuine)", rows_litz,
                   kNoExtra);
    WriteSignalDoc(kDir, "signal_spin-boatright-vs-litz_C8.json",
                   "spin-boatright-vs-litz", "C8", "JOINT",
                   "no expectation (first gap measurement — report only)",
                   rows_boat, kNoExtra);
  }

  // Effect 6 (JOINT): jump on/off — C8 crosswind vs wind-zeroed. Zero
  // crosswind => zero jump by the BuildLitzAerodynamicJump early-out.
  // Expect >>1 on elevation (jump -0.48 MOA measured).
  {
    const std::array<uint32_t, 3> kRanges = {900U, 1800U, 2700U};
    const lob::Context kWindy = BuildC8(10.0, /*with_twist=*/true);
    ASSERT_EQ(kWindy.error, lob::ErrorT::kNone);
    EXPECT_TRUE(std::fabs(kWindy.aerodynamic_jump) > 0.0);
    const lob::Context kCalm = BuildC8(0.0, /*with_twist=*/true);
    ASSERT_EQ(kCalm.error, lob::ErrorT::kNone);
    EXPECT_DOUBLE_EQ(kCalm.aerodynamic_jump, 0.0);
    std::array<lob::Output, 3> windy{};
    std::array<lob::Output, 3> calm{};
    ASSERT_EQ(lob::Solve(kWindy, kRanges, &windy), kRanges.size());
    ASSERT_EQ(lob::Solve(kCalm, kRanges, &calm), kRanges.size());
    const std::vector<double> kElevDelta =
        tests::EffectDelta(ElevInVec(windy), ElevInVec(calm));
    const std::vector<double> kDeflDelta =
        tests::EffectDelta(DeflMoaVec(windy), DeflMoaVec(calm));
    nlohmann::json rows = nlohmann::json::array();
    for (size_t i = 0; i < kRanges.size(); ++i) {
      rows.push_back(
          MakeSigRow(kRanges.at(i), "elevation_in", kElevDelta.at(i),
                     kUNumC8ElevIn, tests::kElevFloorIn, kMc,
                     "floors.json C8-Litz floors_18_9.elevation_in"));
      rows.push_back(MakeSigRow(kRanges.at(i), "deflection_moa",
                                kDeflDelta.at(i), kUNumC8DeflMoa,
                                tests::kMoaFloor, kMc,
                                "floors.json C8-Litz "
                                "floors_18_9.deflection_moa"));
    }
    WriteSignalDoc(kDir, "signal_jump-on-off_C8.json", "jump-on-off", "C8",
                   "JOINT", "R_sig >> 1 on elevation (jump is live)", rows,
                   kNoExtra);
  }

  // Effect 7 (ISOLATED): single-BC vs velocity bands on C1. Expect >>1
  // somewhere in 0-3000 ft (band spread 0.20-0.40 large by construction).
  {
    const std::array<uint32_t, 4> kRanges = {300U, 900U, 1800U, 3000U};
    const lob::Context kSingle = tests::MakeC1IcaoBuilder().Build();
    ASSERT_EQ(kSingle.error, lob::ErrorT::kNone);
    const lob::Context kBands = BuildC1Bands();
    ASSERT_EQ(kBands.error, lob::ErrorT::kNone);
    std::array<lob::Output, 4> single{};
    std::array<lob::Output, 4> bands{};
    ASSERT_EQ(lob::Solve(kSingle, kRanges, &single), kRanges.size());
    ASSERT_EQ(lob::Solve(kBands, kRanges, &bands), kRanges.size());
    const std::vector<double> kElevDelta =
        tests::EffectDelta(ElevInVec(bands), ElevInVec(single));
    const std::vector<double> kDeflDelta =
        tests::EffectDelta(DeflMoaVec(bands), DeflMoaVec(single));
    nlohmann::json rows = nlohmann::json::array();
    for (size_t i = 0; i < kRanges.size(); ++i) {
      rows.push_back(MakeSigRow(kRanges.at(i), "elevation_in", kElevDelta.at(i),
                                kUNumC1ElevIn, tests::kElevFloorIn, kMc,
                                "floors.json C1-ICAO floors_18_9.elevation_in "
                                "(same solver path as bands)"));
      rows.push_back(MakeSigRow(kRanges.at(i), "deflection_moa",
                                kDeflDelta.at(i), kUNumC1DeflMoa,
                                tests::kMoaFloor, kMc,
                                "floors.json C1-ICAO floors_18_9."
                                "deflection_moa (=0.0 observed)"));
    }
    WriteSignalDoc(kDir, "signal_single-bc-vs-bands_C1.json",
                   "single-bc-vs-bands", "C1", "ISOLATED",
                   "R_sig >> 1 somewhere in 0-3000 ft", rows, kNoExtra);
  }

  // Effect 8 (ISOLATED): G7 vs G1 on the C1 builder — re-derived through
  // the harness as the screaming-signal control (Phase 3: 401 in).
  {
    const std::array<uint32_t, 4> kRanges = {300U, 900U, 1800U, 3000U};
    const lob::Context kG7 = tests::MakeC1IcaoBuilder().Build();
    ASSERT_EQ(kG7.error, lob::ErrorT::kNone);
    const lob::Context kG1 = BuildC1G1();
    ASSERT_EQ(kG1.error, lob::ErrorT::kNone);
    std::array<lob::Output, 4> g7{};
    std::array<lob::Output, 4> g1{};
    ASSERT_EQ(lob::Solve(kG7, kRanges, &g7), kRanges.size());
    ASSERT_EQ(lob::Solve(kG1, kRanges, &g1), kRanges.size());
    const std::vector<double> kElevDelta =
        tests::EffectDelta(ElevInVec(g1), ElevInVec(g7));
    const std::vector<double> kDeflDelta =
        tests::EffectDelta(DeflMoaVec(g1), DeflMoaVec(g7));
    nlohmann::json rows = nlohmann::json::array();
    for (size_t i = 0; i < kRanges.size(); ++i) {
      rows.push_back(
          MakeSigRow(kRanges.at(i), "elevation_in", kElevDelta.at(i),
                     kUNumC1ElevIn, tests::kElevFloorIn, kMc,
                     "floors.json C1-ICAO floors_18_9.elevation_in"));
      rows.push_back(MakeSigRow(kRanges.at(i), "deflection_moa",
                                kDeflDelta.at(i), kUNumC1DeflMoa,
                                tests::kMoaFloor, kMc,
                                "floors.json C1-ICAO floors_18_9."
                                "deflection_moa (=0.0 observed)"));
    }
    WriteSignalDoc(kDir, "signal_g7-vs-g1_C1.json", "g7-vs-g1", "C1",
                   "ISOLATED", "R_sig >> 1 (screaming-signal control)", rows,
                   kNoExtra);
  }

  // Effect 9 (ISOLATED): custom-table +/-5e-3 Cd on the transonic band.
  // CLOSES the §8.6 deferral — either outcome is a finding (<<1: table
  // resolution comfortably below granularity; ~1: earns a budget row).
  {
    const std::array<uint32_t, 4> kRanges = {300U, 900U, 1800U, 3000U};
    const lob::Context kBase = BuildC1Spline(/*shifted=*/false);
    ASSERT_EQ(kBase.error, lob::ErrorT::kNone);
    const lob::Context kShifted = BuildC1Spline(/*shifted=*/true);
    ASSERT_EQ(kShifted.error, lob::ErrorT::kNone);
    std::array<lob::Output, 4> base{};
    std::array<lob::Output, 4> shifted{};
    ASSERT_EQ(lob::Solve(kBase, kRanges, &base), kRanges.size());
    ASSERT_EQ(lob::Solve(kShifted, kRanges, &shifted), kRanges.size());
    const std::vector<double> kElevDelta =
        tests::EffectDelta(ElevInVec(shifted), ElevInVec(base));
    const std::vector<double> kDeflDelta =
        tests::EffectDelta(DeflMoaVec(shifted), DeflMoaVec(base));
    nlohmann::json rows = nlohmann::json::array();
    for (size_t i = 0; i < kRanges.size(); ++i) {
      rows.push_back(
          MakeSigRow(kRanges.at(i), "elevation_in", kElevDelta.at(i),
                     kUNumC1ElevIn, tests::kElevFloorIn, kMc,
                     "floors.json C1-ICAO floors_18_9.elevation_in"));
      rows.push_back(MakeSigRow(kRanges.at(i), "deflection_moa",
                                kDeflDelta.at(i), kUNumC1DeflMoa,
                                tests::kMoaFloor, kMc,
                                "floors.json C1-ICAO floors_18_9."
                                "deflection_moa (=0.0 observed)"));
    }
    WriteSignalDoc(kDir, "signal_spline-pm-5e-3_C1.json", "spline-pm-5e-3",
                   "C1", "ISOLATED",
                   "R_sig << 1 or ~1 (closes §8.6 spline mapping)", rows,
                   kNoExtra);
  }

  // Effects 10-12 (ISOLATED, C6 stations): profile-vs-uniform at alpha=0
  // (expect <<1 elevation, small deflection); shear 0 vs 0.25 (expect >>1
  // deflection, <<1 elsewhere); measured-vs-NaN heights (quantify only).
  // Yardstick cross-link: floors.json C6-scaled floors_18_9.
  {
    const std::array<uint32_t, 3> kRanges = {900U, 1800U, 2700U};
    const lob::Context kProfA0 = BuildC6(0.0, 0.0, /*nan_heights=*/false);
    ASSERT_EQ(kProfA0.error, lob::ErrorT::kNone);
    const lob::Context kUniform = BuildC6Uniform();
    ASSERT_EQ(kUniform.error, lob::ErrorT::kNone);
    const lob::Context kProfA025 = BuildC6(0.25, 0.0, /*nan_heights=*/false);
    ASSERT_EQ(kProfA025.error, lob::ErrorT::kNone);
    const lob::Context kProfNaN = BuildC6(0.25, 0.0, /*nan_heights=*/true);
    ASSERT_EQ(kProfNaN.error, lob::ErrorT::kNone);
    std::array<lob::Output, 3> prof_a0{};
    std::array<lob::Output, 3> uniform{};
    std::array<lob::Output, 3> prof_a025{};
    std::array<lob::Output, 3> prof_nan{};
    ASSERT_EQ(lob::Solve(kProfA0, kRanges, &prof_a0), kRanges.size());
    ASSERT_EQ(lob::Solve(kUniform, kRanges, &uniform), kRanges.size());
    ASSERT_EQ(lob::Solve(kProfA025, kRanges, &prof_a025), kRanges.size());
    ASSERT_EQ(lob::Solve(kProfNaN, kRanges, &prof_nan), kRanges.size());
    const std::vector<double> kE10Elev =
        tests::EffectDelta(ElevInVec(prof_a0), ElevInVec(uniform));
    const std::vector<double> kE10Defl =
        tests::EffectDelta(DeflMoaVec(prof_a0), DeflMoaVec(uniform));
    const std::vector<double> kE11Elev =
        tests::EffectDelta(ElevInVec(prof_a025), ElevInVec(prof_a0));
    const std::vector<double> kE11Defl =
        tests::EffectDelta(DeflMoaVec(prof_a025), DeflMoaVec(prof_a0));
    const std::vector<double> kE12Elev =
        tests::EffectDelta(ElevInVec(prof_nan), ElevInVec(prof_a025));
    const std::vector<double> kE12Defl =
        tests::EffectDelta(DeflMoaVec(prof_nan), DeflMoaVec(prof_a025));
    nlohmann::json rows_e10 = nlohmann::json::array();
    nlohmann::json rows_e11 = nlohmann::json::array();
    nlohmann::json rows_e12 = nlohmann::json::array();
    for (size_t i = 0; i < kRanges.size(); ++i) {
      rows_e10.push_back(MakeSigRow(kRanges.at(i), "elevation_in",
                                    kE10Elev.at(i), kUNumC6ElevIn,
                                    tests::kElevFloorIn, kMc,
                                    "floors.json C6-scaled "
                                    "floors_18_9.elevation_in"));
      rows_e10.push_back(MakeSigRow(kRanges.at(i), "deflection_moa",
                                    kE10Defl.at(i), kUNumC6DeflMoa,
                                    tests::kMoaFloor, kMc,
                                    "floors.json C6-scaled "
                                    "floors_18_9.deflection_moa"));
      rows_e11.push_back(MakeSigRow(kRanges.at(i), "elevation_in",
                                    kE11Elev.at(i), kUNumC6ElevIn,
                                    tests::kElevFloorIn, kMc,
                                    "floors.json C6-scaled "
                                    "floors_18_9.elevation_in"));
      rows_e11.push_back(MakeSigRow(kRanges.at(i), "deflection_moa",
                                    kE11Defl.at(i), kUNumC6DeflMoa,
                                    tests::kMoaFloor, kMc,
                                    "floors.json C6-scaled "
                                    "floors_18_9.deflection_moa"));
      rows_e12.push_back(MakeSigRow(kRanges.at(i), "elevation_in",
                                    kE12Elev.at(i), kUNumC6ElevIn,
                                    tests::kElevFloorIn, kMc,
                                    "floors.json C6-scaled "
                                    "floors_18_9.elevation_in"));
      rows_e12.push_back(MakeSigRow(kRanges.at(i), "deflection_moa",
                                    kE12Defl.at(i), kUNumC6DeflMoa,
                                    tests::kMoaFloor, kMc,
                                    "floors.json C6-scaled "
                                    "floors_18_9.deflection_moa"));
    }
    WriteSignalDoc(kDir, "signal_profile-vs-uniform_C6.json",
                   "profile-vs-uniform", "C6", "ISOLATED",
                   "R_sig << 1 elevation, small deflection", rows_e10,
                   kNoExtra);
    WriteSignalDoc(kDir, "signal_shear-off-on_C6.json", "shear-off-on", "C6",
                   "ISOLATED", "R_sig >> 1 deflection, << 1 elsewhere",
                   rows_e11, kNoExtra);
    WriteSignalDoc(kDir, "signal_measured-vs-nan-heights_C6.json",
                   "measured-vs-nan-heights", "C6", "ISOLATED",
                   "quantify only (no direction predicted)", rows_e12,
                   kNoExtra);
  }

  // Effect 13 (BOUND): tilted-vs-flat datum on C6b. No flat-h solver path
  // exists, so the flat-vs-15° comparison (CrosswindBlindToInclineAtSolve
  // pattern in test/source/lob_wind_profile_test.cpp) is reported as an
  // UPPER BOUND on datum sensitivity — labeled bound, never effect.
  {
    const std::array<uint32_t, 3> kRanges = {900U, 1800U, 2700U};
    const lob::Context kFlat = BuildC6(0.25, 0.0, /*nan_heights=*/false);
    ASSERT_EQ(kFlat.error, lob::ErrorT::kNone);
    const lob::Context kHill = BuildC6(0.25, 15.0, /*nan_heights=*/false);
    ASSERT_EQ(kHill.error, lob::ErrorT::kNone);
    std::array<lob::Output, 3> flat{};
    std::array<lob::Output, 3> hill{};
    ASSERT_EQ(lob::Solve(kFlat, kRanges, &flat), kRanges.size());
    ASSERT_EQ(lob::Solve(kHill, kRanges, &hill), kRanges.size());
    const std::vector<double> kElevDelta =
        tests::EffectDelta(ElevInVec(hill), ElevInVec(flat));
    const std::vector<double> kDeflDelta =
        tests::EffectDelta(DeflMoaVec(hill), DeflMoaVec(flat));
    nlohmann::json rows = nlohmann::json::array();
    for (size_t i = 0; i < kRanges.size(); ++i) {
      rows.push_back(MakeSigRow(kRanges.at(i), "elevation_in", kElevDelta.at(i),
                                kUNumC6ElevIn, tests::kElevFloorIn, kMc,
                                "floors.json C6-scaled floors_18_9."
                                "elevation_in (bound yardstick)"));
      rows.push_back(MakeSigRow(kRanges.at(i), "deflection_moa",
                                kDeflDelta.at(i), kUNumC6DeflMoa,
                                tests::kMoaFloor, kMc,
                                "floors.json C6-scaled floors_18_9."
                                "deflection_moa (bound yardstick)"));
    }
    WriteSignalDoc(kDir, "signal_tilted-vs-flat-datum_C6b.json",
                   "tilted-vs-flat-datum", "C6b", "BOUND",
                   "UPPER BOUND on datum sensitivity (bound, not effect)", rows,
                   kNoExtra);
  }

  // Effect 14 (BOUND): ceiling clamp on C6c — completion + sign oracle only
  // (a guardrail has no quantity, so no R_sig).
  {
    const std::array<uint32_t, 3> kRanges = {900U, 1800U, 2700U};
    const lob::Context kCtx = BuildC6cMortar();
    ASSERT_EQ(kCtx.error, lob::ErrorT::kNone);
    std::array<lob::Output, 3> outs{};
    ASSERT_EQ(lob::Solve(kCtx, kRanges, &outs), kRanges.size());
    nlohmann::json rows = nlohmann::json::array();
    for (size_t i = 0; i < kRanges.size(); ++i) {
      ASSERT_TRUE(std::isfinite(outs.at(i).deflection));
      EXPECT_GT(outs.at(i).deflection, 0.0) << "range=" << kRanges.at(i);
      nlohmann::json row;
      row["range_ft"] = kRanges.at(i);
      row["channel"] = "completion";
      row["completed"] = true;
      row["deflection_in"] = outs.at(i).deflection;
      row["sign"] = "positive";
      row["r_sig"] = "not-applicable-guardrail";
      row["yardstick"] = {{"u_num_source",
                           "floors.json C6-scaled (completion oracle — no "
                           "R_sig; a guardrail has no quantity)"},
                          {"granularity", tests::kElevFloorIn},
                          {"mc_sigma_or_absent", "mc_sigma_absent"},
                          {"u_c", nullptr}};
      rows.push_back(row);
    }
    WriteSignalDoc(kDir, "signal_ceiling-clamp_C6c.json", "ceiling-clamp",
                   "C6c", "BOUND", "completion + sign oracle only", rows,
                   kNoExtra);
  }

  // Effect 15 (ISOLATED): single-point profile vs uniform setters on C5 —
  // BIT-IDENTITY oracle (EXPECT_DOUBLE_EQ trips; the JSON records the zeros).
  {
    const std::array<uint32_t, 3> kRanges = {900U, 1800U, 2700U};
    const lob::Context kProfile = BuildC5Profile();
    const lob::Context kUniform = BuildC5Uniform();
    ASSERT_EQ(kProfile.error, lob::ErrorT::kNone);
    ASSERT_EQ(kUniform.error, lob::ErrorT::kNone);
    std::array<lob::Output, 3> profile_outs{};
    std::array<lob::Output, 3> uniform_outs{};
    ASSERT_EQ(lob::Solve(kProfile, kRanges, &profile_outs), kRanges.size());
    ASSERT_EQ(lob::Solve(kUniform, kRanges, &uniform_outs), kRanges.size());
    ExpectIdentityAtAllRanges(profile_outs, uniform_outs);
    const std::vector<double> kElevDelta =
        tests::EffectDelta(ElevInVec(profile_outs), ElevInVec(uniform_outs));
    const std::vector<double> kDeflDelta =
        tests::EffectDelta(DeflMoaVec(profile_outs), DeflMoaVec(uniform_outs));
    nlohmann::json rows = nlohmann::json::array();
    for (size_t i = 0; i < kRanges.size(); ++i) {
      rows.push_back(MakeSigRow(kRanges.at(i), "elevation_in", kElevDelta.at(i),
                                kUNumC5ElevIn, tests::kElevFloorIn, kMc,
                                "floors.json C5-uniform "
                                "floors_18_9.elevation_in"));
      rows.push_back(MakeSigRow(kRanges.at(i), "deflection_moa",
                                kDeflDelta.at(i), kUNumC5DeflMoa,
                                tests::kMoaFloor, kMc,
                                "floors.json C5-uniform "
                                "floors_18_9.deflection_moa"));
    }
    const nlohmann::json kExtra = {{"oracle", "bit-identity-exact"}};
    WriteSignalDoc(kDir, "signal_single-point-vs-uniform_C5.json",
                   "single-point-vs-uniform", "C5", "ISOLATED",
                   "BIT-IDENTITY (exact double equality)", rows, kExtra);
  }

  // Duplicate-vector check: byte-compare barometer vs weather-station
  // expected[] (vectors untouched — flagged in claims limitations only).
  {
    const std::string kCasesDir = LOB_VALIDATION_CASES_DIR;
    std::string baro;
    std::string station;
    ASSERT_TRUE(DumpExpected(kCasesDir + "/reference_barometer.json", &baro));
    ASSERT_TRUE(
        DumpExpected(kCasesDir + "/reference_weather_station.json", &station));
    std::cout << "DUPLICATE_CHECK barometer_vs_weather-station equal="
              << (baro == station ? 1 : 0) << " bytes=" << baro.size() << "/"
              << station.size() << "\n";
  }
}

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
