// Copyright (c) 2026  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "lob/lob.hpp"
#include "testing.hpp"

#ifndef LOB_VALIDATION_CASES_DIR
#error "LOB_VALIDATION_CASES_DIR must be defined by CMake"
#endif

namespace tests {

TEST(ReferenceJson, ParsesFlatCaseShape) {
  const nlohmann::json kRoot = nlohmann::json::parse(
      "{\"id\":\"ref-icao\",\"ranges_ft\":[0,150],\"expected\":[{\"range_ft\":0,"
      "\"velocity_fps\":2800,\"u_ref\":\"unknown\"}],\"status\":\"provisional\","
      "\"supersedes\":null}");
  EXPECT_EQ(kRoot.at("id").get<std::string>(), "ref-icao");
  EXPECT_DOUBLE_EQ(kRoot.at("ranges_ft").at(1).get<double>(), 150.0);
  EXPECT_DOUBLE_EQ(
      kRoot.at("expected").at(0).at("velocity_fps").get<double>(), 2800.0);
  EXPECT_EQ(kRoot.at("expected").at(0).at("u_ref").get<std::string>(),
            "unknown");
  EXPECT_TRUE(kRoot.at("supersedes").is_null());
  EXPECT_EQ(kRoot.at("status").get<std::string>(), "provisional");
}

TEST(ReferenceJson, RejectsMalformedDocuments) {
  for (const char* kBad : {"{\"a\":1,}", "{\"a\":", "{\"a\" 1}", "[1,2",
                           "{\"a\":01}", ""}) {
    EXPECT_THROW(nlohmann::json::parse(kBad), nlohmann::json::parse_error)
        << "accepted: " << kBad;
  }
}

TEST(ReferenceJson, MissingKeysThrowInsteadOfInserting) {
  const nlohmann::json kRoot = nlohmann::json::parse("{\"a\":1}");
  EXPECT_THROW(kRoot.at("zzz"), nlohmann::json::out_of_range);
  EXPECT_THROW(kRoot.at("a").get<std::string>(), nlohmann::json::type_error);
}

}  // namespace tests

// ---- Task 3: reference loader + C1-ICAO decomposition smoke ----

namespace tests {

// Reads <dir>/reference_<stem>.json and returns the parsed document.
// Throws std::runtime_error (path + cause) on missing files, parse errors,
// or schema violations — callers FAIL with the message, never fall back to
// embedded literals.
inline nlohmann::json LoadReferenceCase(const std::string& dir,
                                        const std::string& stem) {
  const std::string kPath = dir + "/reference_" + stem + ".json";
  std::ifstream in(kPath.c_str());
  if (!in) {
    throw std::runtime_error("reference case file not found: " + kPath);
  }
  std::ostringstream raw;
  raw << in.rdbuf();
  try {
    nlohmann::json root = nlohmann::json::parse(raw.str());
    root.at("artifact_schema");
    root.at("id");
    root.at("builder");
    root.at("solver_config");
    root.at("ranges_ft");
    root.at("reporting_granularity");
    root.at("expected");
    root.at("envelope_tags");
    root.at("status");
    root.at("supersedes");
    // Typed probes so wrong-typed values throw type_error with path context.
    (void)root.at("builder").at("ballistic_coefficient_psi").get<double>();
    (void)root.at("solver_config").at("step_in").get<double>();
    (void)root.at("id").get<std::string>();
    (void)root.at("expected").at(0).at("velocity_fps").get<double>();
    return root;
  } catch (const nlohmann::json::parse_error& e) {
    throw std::runtime_error("parse error in " + kPath + ": " + e.what());
  } catch (const nlohmann::json::out_of_range& e) {
    throw std::runtime_error("schema key missing in " + kPath + ": " +
                             e.what());
  } catch (const nlohmann::json::type_error& e) {
    throw std::runtime_error("schema type error in " + kPath + ": " +
                             e.what());
  }
}

// The C1 fixture IS the ICAO case: byte-for-byte the same setter list.
inline lob::Builder BuildIcaoCase() { return MakeC1IcaoBuilder(); }

TEST(ReferenceDecomposition, C1Icao) {
  nlohmann::json root;
  try {
    root = LoadReferenceCase(LOB_VALIDATION_CASES_DIR, "icao");
  } catch (const std::exception& e) {
    FAIL() << e.what();
  }

  // Builder mirrors the fixture setter-for-setter (field-by-field pin).
  const nlohmann::json kBuilder = root.at("builder");
  EXPECT_TRUE(kBuilder.at("ballistic_coefficient_psi").is_number());
  EXPECT_DOUBLE_EQ(kBuilder.at("ballistic_coefficient_psi").get<double>(),
                   0.232);
  EXPECT_TRUE(kBuilder.at("bc_drag_function").is_string());
  EXPECT_EQ(kBuilder.at("bc_drag_function").get<std::string>(), "G7");
  EXPECT_TRUE(kBuilder.at("bc_atmosphere").is_string());
  EXPECT_EQ(kBuilder.at("bc_atmosphere").get<std::string>(), "ICAO");
  EXPECT_TRUE(kBuilder.at("diameter_in").is_number());
  EXPECT_DOUBLE_EQ(kBuilder.at("diameter_in").get<double>(), 0.308);
  EXPECT_TRUE(kBuilder.at("mass_grains").is_number());
  EXPECT_DOUBLE_EQ(kBuilder.at("mass_grains").get<double>(), 155.0);
  EXPECT_TRUE(kBuilder.at("initial_velocity_fps").is_number());
  EXPECT_DOUBLE_EQ(kBuilder.at("initial_velocity_fps").get<double>(), 2800.0);
  EXPECT_TRUE(kBuilder.at("zero_angle_moa").is_number());
  EXPECT_DOUBLE_EQ(kBuilder.at("zero_angle_moa").get<double>(), 3.66);
  EXPECT_TRUE(kBuilder.at("optic_height_in").is_number());
  EXPECT_DOUBLE_EQ(kBuilder.at("optic_height_in").get<double>(), 1.5);

  const nlohmann::json kSolver = root.at("solver_config");
  EXPECT_TRUE(kSolver.at("step_in").is_number());
  EXPECT_DOUBLE_EQ(kSolver.at("step_in").get<double>(), 36.0);
  EXPECT_TRUE(kSolver.at("angle_tol_moa").is_number());
  EXPECT_DOUBLE_EQ(kSolver.at("angle_tol_moa").get<double>(), 0.01);
  EXPECT_TRUE(kSolver.at("density_path").is_string());
  EXPECT_EQ(kSolver.at("density_path").get<std::string>(), "fast");

  const nlohmann::json kGran = root.at("reporting_granularity");
  EXPECT_TRUE(kGran.at("velocity_fps").is_number());
  EXPECT_DOUBLE_EQ(kGran.at("velocity_fps").get<double>(), 1.0);
  EXPECT_TRUE(kGran.at("energy_ft_lbf").is_number());
  EXPECT_DOUBLE_EQ(kGran.at("energy_ft_lbf").get<double>(), 5.0);
  EXPECT_TRUE(kGran.at("elevation_moa").is_number());
  EXPECT_DOUBLE_EQ(kGran.at("elevation_moa").get<double>(), 0.1);
  EXPECT_TRUE(kGran.at("time_of_flight_s").is_number());
  EXPECT_DOUBLE_EQ(kGran.at("time_of_flight_s").get<double>(), 0.01);

  EXPECT_TRUE(root.at("id").is_string());
  EXPECT_EQ(root.at("id").get<std::string>(), "ref-icao");
  EXPECT_TRUE(root.at("status").is_string());
  EXPECT_EQ(root.at("status").get<std::string>(), "provisional");
  EXPECT_TRUE(root.at("supersedes").is_null());

  const std::vector<std::string> kExpectedTags = {
      "range-0-300yd", "range-300-1000yd", "regime-supersonic",
      "regime-transonic-tail", "wind-calm", "spin-off", "density-fast",
      "drag-g7-single-bc", "atm-isa"};
  const nlohmann::json kTags = root.at("envelope_tags");
  EXPECT_TRUE(kTags.is_array());
  ASSERT_EQ(kTags.size(), kExpectedTags.size());
  for (size_t i = 0; i < kExpectedTags.size(); ++i) {
    EXPECT_TRUE(kTags.at(i).is_string());
    EXPECT_EQ(kTags.at(i).get<std::string>(), kExpectedTags[i]) << "tag " << i;
  }

  constexpr size_t kNumRanges = 12;
  const std::array<uint32_t, kNumRanges> kRanges = {
      0U, 150U, 300U, 600U, 900U, 1200U, 1500U, 1800U, 2100U, 2400U, 2700U,
      3000U};
  const nlohmann::json kRangesJson = root.at("ranges_ft");
  EXPECT_TRUE(kRangesJson.is_array());
  ASSERT_EQ(kRangesJson.size(), kRanges.size());
  for (size_t i = 0; i < kRanges.size(); ++i) {
    EXPECT_TRUE(kRangesJson.at(i).is_number());
    EXPECT_DOUBLE_EQ(kRangesJson.at(i).get<double>(),
                      static_cast<double>(kRanges.at(i)))
        << "range " << i;
  }

  // Range-0 muzzle row compared like any other row below.
  const nlohmann::json kRows = root.at("expected");
  EXPECT_TRUE(kRows.is_array());
  ASSERT_EQ(kRows.size(), kRanges.size());
  std::vector<lob::Output> refs;
  refs.reserve(kRanges.size());
  for (size_t i = 0; i < kRanges.size(); ++i) {
    const nlohmann::json& kRow = kRows.at(i);
    EXPECT_TRUE(kRow.at("u_ref").is_string());
    EXPECT_EQ(kRow.at("u_ref").get<std::string>(), "unknown") << "row " << i;
    lob::Output out{};
    EXPECT_TRUE(kRow.at("range_ft").is_number());
    out.range = static_cast<uint32_t>(kRow.at("range_ft").get<double>());
    EXPECT_EQ(out.range, kRanges.at(i)) << "row " << i;
    EXPECT_TRUE(kRow.at("velocity_fps").is_number());
    out.velocity =
        static_cast<uint16_t>(kRow.at("velocity_fps").get<double>());
    EXPECT_TRUE(kRow.at("energy_ft_lbf").is_number());
    out.energy = static_cast<uint32_t>(kRow.at("energy_ft_lbf").get<double>());
    EXPECT_TRUE(kRow.at("elevation_in").is_number());
    out.elevation = kRow.at("elevation_in").get<double>();
    EXPECT_TRUE(kRow.at("deflection_in").is_number());
    out.deflection = kRow.at("deflection_in").get<double>();
    EXPECT_TRUE(kRow.at("time_of_flight_s").is_number());
    out.time_of_flight = kRow.at("time_of_flight_s").get<double>();
    refs.push_back(out);
  }

  const lob::Context kCtx = BuildAtStep(BuildIcaoCase(), 36U);
  ASSERT_EQ(kCtx.error, lob::ErrorT::kNone);
  std::array<lob::Output, kNumRanges> solutions = {};
  EXPECT_EQ(SolveN(kCtx, kRanges, &solutions), kRanges.size());

  // Loader round-trip: JSON numbers reproduce the C++-literal outcome.
  EXPECT_TRUE(kGran.at("velocity_fps").is_number());
  EXPECT_TRUE(kGran.at("energy_ft_lbf").is_number());
  EXPECT_TRUE(kGran.at("elevation_moa").is_number());
  EXPECT_TRUE(kGran.at("time_of_flight_s").is_number());
  VerifySolutions(solutions, refs,
                  {lob::FpsT(kGran.at("velocity_fps").get<double>()),
                   lob::FtLbsT(kGran.at("energy_ft_lbf").get<double>()),
                   lob::MoaT(kGran.at("elevation_moa").get<double>()),
                   lob::InchT(lob::NaN()),
                   lob::SecT(kGran.at("time_of_flight_s").get<double>())});

  // Decomposition on elevation: r vs the C1 18->9 floor
  // (test/validation/baselines/floors.json cell C1-ICAO floors_18_9).
  constexpr double kC1ElevFloorIn = 4.15814e-05;
  EXPECT_TRUE(kGran.at("elevation_moa").is_number());
  const double kMoaGran = kGran.at("elevation_moa").get<double>();
  for (size_t i = 0; i < kRanges.size(); ++i) {
    SCOPED_TRACE(testing::Message() << "range_ft=" << kRanges.at(i));
    const double kRIn =
        std::fabs(solutions.at(i).elevation - refs.at(i).elevation);
    const double kMoaRes = std::fabs(
        lob::InchToMoa(solutions.at(i).elevation,
                       static_cast<double>(kRanges.at(i))) -
        lob::InchToMoa(refs.at(i).elevation,
                       static_cast<double>(kRanges.at(i))));
    EXPECT_LE(kMoaRes, kMoaGran);
    const double kDelta = kRIn - kC1ElevFloorIn;
    if (!(kDelta >= 0.0)) {
      SCOPED_TRACE("consistent-within-resolution");
    } else {
      SCOPED_TRACE("residual-is-model-or-reference");
    }
  }
}

}  // namespace tests

// ---- Task 4: offline full-matrix driver + coverage matrix ----

namespace tests {
namespace reference_matrix {

// C1 floor family reused as eps_num for every case. Cross-case floor reuse is
// conservative: the C1-ICAO floors_18_9 cell in
// test/validation/baselines/floors.json is the only measured set; per-case
// floors arrive with envelope expansion (ledger note).
constexpr double kEpsElevIn = 4.15814e-05;
constexpr double kEpsElevMoa = 3.97148e-06;
constexpr double kEpsDeflMoa = 0.0;
constexpr double kEpsTofSec = 6.12488e-08;
constexpr double kEpsVelFps = 1.0;      // 1 LSB of U16 truncation
constexpr double kEpsEnergyFtLbs = 1.0;  // 1 LSB of U32 truncation

inline double ReqDouble(const nlohmann::json& obj, const char* key) {
  const nlohmann::json& kVal = obj.at(key);
  if (!kVal.is_number()) {
    throw std::runtime_error(std::string("expected number at key: ") + key);
  }
  return kVal.get<double>();
}

inline std::string ReqString(const nlohmann::json& obj, const char* key) {
  const nlohmann::json& kVal = obj.at(key);
  if (!kVal.is_string()) {
    throw std::runtime_error(std::string("expected string at key: ") + key);
  }
  return kVal.get<std::string>();
}

struct Worst {
  double elev_in = 0.0;
  double elev_moa = 0.0;
  double defl_moa = 0.0;
  double vel = 0.0;
  double energy = 0.0;
  double tof = 0.0;
};

inline std::string Fmt(double v) {
  char buf[32];
  std::snprintf(buf, sizeof(buf), "%.10g", v);
  return std::string(buf);
}

}  // namespace reference_matrix

// Altitude case: base C1 setter list + firing-site/temperature diff.
// Keep in sync with LobEnvTestFixture::SolveWithAltitude4500ft
// (test/source/lob_env_test.cpp) and the builder block of
// test/validation/cases/reference_altitude4500.json.
inline lob::Builder BuildAltitude4500Case() {
  lob::Builder b = MakeC1IcaoBuilder();
  b.AltitudeOfFiringSiteFt(4500).TemperatureDegF(59);
  return b;
}

// Hot / low-pressure case: base list + temperature/air-pressure diff.
// Keep in sync with LobEnvTestFixture::SolveWithTempAndAirPressure and
// test/validation/cases/reference_hot_lowp.json.
inline lob::Builder BuildHotLowPCase() {
  lob::Builder b = MakeC1IcaoBuilder();
  b.TemperatureDegF(100).AirPressureInHg(25);
  return b;
}

// Barometer-offset case: base list + firing-site/pressure/barometer diff.
// Keep in sync with LobEnvTestFixture::SolveWithBarometricPressure and
// test/validation/cases/reference_barometer.json.
inline lob::Builder BuildBarometerCase() {
  lob::Builder b = MakeC1IcaoBuilder();
  b.AltitudeOfFiringSiteFt(5280)
      .AirPressureInHg(30)
      .AltitudeOfBarometerFt(0)
      .TemperatureDegF(59);
  return b;
}

// Humid case: base list + pressure/temperature/humidity diff.
// Keep in sync with LobEnvTestFixture::SolveWithPressureTempHumidity and
// test/validation/cases/reference_humidity.json.
inline lob::Builder BuildHumidCase() {
  lob::Builder b = MakeC1IcaoBuilder();
  b.AirPressureInHg(29).TemperatureDegF(75).RelativeHumidityPercent(80);
  return b;
}

// Weather-station case: base list + full station diff.
// Keep in sync with LobEnvTestFixture::SolveWithWeatherStationData and
// test/validation/cases/reference_weather_station.json.
inline lob::Builder BuildWeatherStationCase() {
  lob::Builder b = MakeC1IcaoBuilder();
  b.AltitudeOfFiringSiteFt(5280)
      .AirPressureInHg(30)
      .AltitudeOfBarometerFt(0)
      .TemperatureDegF(65)
      .AltitudeOfThermometerFt(3598);
  return b;
}

TEST(ReferenceMatrix, FullMatrix) {
  const char* kGate = std::getenv("LOB_FULL_MATRIX");
  if (kGate == nullptr || std::string(kGate) != "1") {
    GTEST_SKIP() << "offline only: set LOB_FULL_MATRIX=1";
  }

  struct MatrixCase {
    const char* stem;
    const char* id;
    lob::Builder (*build)();
  };
  const MatrixCase kCases[] = {
      {"icao", "ref-icao", BuildIcaoCase},
      {"altitude4500", "ref-altitude4500", BuildAltitude4500Case},
      {"hot_lowp", "ref-hot-lowp", BuildHotLowPCase},
      {"barometer", "ref-barometer", BuildBarometerCase},
      {"humidity", "ref-humidity", BuildHumidCase},
      {"weather_station", "ref-weather-station", BuildWeatherStationCase},
  };

  constexpr size_t kNumRanges = 12;
  std::string csv;
  csv +=
      "case_id,range_ft,r_elev_in,r_elev_moa,r_defl_moa,r_vel_fps,r_energy_"
      "ftlbf,r_tof_s\n";

  std::vector<std::string> case_ids;
  std::vector<std::vector<std::string> > case_tags;
  std::vector<reference_matrix::Worst> case_worst;

  for (size_t c = 0; c < 6; ++c) {
    SCOPED_TRACE(testing::Message() << "case=" << kCases[c].stem);
    nlohmann::json root;
    try {
      root = LoadReferenceCase(LOB_VALIDATION_CASES_DIR, kCases[c].stem);
    } catch (const std::exception& e) {
      FAIL() << e.what();
    }
    EXPECT_EQ(reference_matrix::ReqString(root, "id"), kCases[c].id);
    EXPECT_EQ(reference_matrix::ReqString(root, "status"), "provisional");

    const nlohmann::json kTags = root.at("envelope_tags");
    ASSERT_TRUE(kTags.is_array());
    std::vector<std::string> tags;
    for (size_t t = 0; t < kTags.size(); ++t) {
      ASSERT_TRUE(kTags.at(t).is_string());
      tags.push_back(kTags.at(t).get<std::string>());
    }

    const nlohmann::json kRangesJson = root.at("ranges_ft");
    ASSERT_TRUE(kRangesJson.is_array());
    ASSERT_EQ(kRangesJson.size(), kNumRanges);
    std::array<uint32_t, kNumRanges> ranges = {};
    for (size_t i = 0; i < kNumRanges; ++i) {
      ASSERT_TRUE(kRangesJson.at(i).is_number());
      ranges.at(i) =
          static_cast<uint32_t>(kRangesJson.at(i).get<double>());
    }

    const nlohmann::json kRows = root.at("expected");
    ASSERT_TRUE(kRows.is_array());
    ASSERT_EQ(kRows.size(), kNumRanges);
    const nlohmann::json kGran = root.at("reporting_granularity");
    const double kVelGran = reference_matrix::ReqDouble(kGran, "velocity_fps");
    const double kEnergyGran =
        reference_matrix::ReqDouble(kGran, "energy_ft_lbf");
    const double kMoaGran =
        reference_matrix::ReqDouble(kGran, "elevation_moa");
    const double kTofGran =
        reference_matrix::ReqDouble(kGran, "time_of_flight_s");
    std::vector<lob::Output> refs;
    refs.reserve(kNumRanges);
    for (size_t i = 0; i < kNumRanges; ++i) {
      const nlohmann::json& kRow = kRows.at(i);
      EXPECT_EQ(reference_matrix::ReqString(kRow, "u_ref"), "unknown");
      lob::Output out{};
      out.range = static_cast<uint32_t>(
          reference_matrix::ReqDouble(kRow, "range_ft"));
      EXPECT_EQ(out.range, ranges.at(i));
      out.velocity = static_cast<uint16_t>(
          reference_matrix::ReqDouble(kRow, "velocity_fps"));
      out.energy = static_cast<uint32_t>(
          reference_matrix::ReqDouble(kRow, "energy_ft_lbf"));
      out.elevation = reference_matrix::ReqDouble(kRow, "elevation_in");
      out.deflection = reference_matrix::ReqDouble(kRow, "deflection_in");
      out.time_of_flight =
          reference_matrix::ReqDouble(kRow, "time_of_flight_s");
      refs.push_back(out);
    }

    // Ladder shape: solve at 36/18/9; deltas are sanity-logged, not gated.
    std::array<lob::Output, kNumRanges> outs36 = {};
    std::array<lob::Output, kNumRanges> outs18 = {};
    std::array<lob::Output, kNumRanges> outs9 = {};
    ASSERT_EQ(SolveN(BuildAtStep(kCases[c].build(), 36U), ranges, &outs36),
              kNumRanges);
    ASSERT_EQ(SolveN(BuildAtStep(kCases[c].build(), 18U), ranges, &outs18),
              kNumRanges);
    ASSERT_EQ(SolveN(BuildAtStep(kCases[c].build(), 9U), ranges, &outs9),
              kNumRanges);
    double ladder_36_18 = 0.0;
    double ladder_18_9 = 0.0;
    for (size_t i = 0; i < kNumRanges; ++i) {
      const double kD1 =
          std::fabs(outs36.at(i).elevation - outs18.at(i).elevation);
      const double kD2 =
          std::fabs(outs18.at(i).elevation - outs9.at(i).elevation);
      if (kD1 > ladder_36_18) ladder_36_18 = kD1;
      if (kD2 > ladder_18_9) ladder_18_9 = kD2;
    }
    RecordProperty(
        std::string("ladder.") + kCases[c].stem + ".elev_in_36_18",
        ladder_36_18);
    RecordProperty(
        std::string("ladder.") + kCases[c].stem + ".elev_in_18_9",
        ladder_18_9);

    // Loader round-trip: JSON numbers reproduce the C++-literal outcome.
    VerifySolutions(outs36, refs,
                    {lob::FpsT(kVelGran), lob::FtLbsT(kEnergyGran),
                     lob::MoaT(kMoaGran), lob::InchT(lob::NaN()),
                     lob::SecT(kTofGran)});

    // Decomposition at the 36-in rung vs expected, C1 floors as eps_num.
    reference_matrix::Worst worst;
    reference_matrix::Worst worst_delta;
    for (size_t i = 0; i < kNumRanges; ++i) {
      SCOPED_TRACE(testing::Message() << "range_ft=" << ranges.at(i));
      const double kRange = static_cast<double>(ranges.at(i));
      const double kRElevIn =
          std::fabs(outs36.at(i).elevation - refs.at(i).elevation);
      const double kRElevMoa = std::fabs(
          lob::InchToMoa(outs36.at(i).elevation, kRange) -
          lob::InchToMoa(refs.at(i).elevation, kRange));
      const double kRDeflMoa = std::fabs(
          lob::InchToMoa(outs36.at(i).deflection, kRange) -
          lob::InchToMoa(refs.at(i).deflection, kRange));
      const double kRVel = std::fabs(static_cast<double>(outs36.at(i).velocity) -
                                     static_cast<double>(refs.at(i).velocity));
      const double kREnergy = std::fabs(
          static_cast<double>(outs36.at(i).energy) -
          static_cast<double>(refs.at(i).energy));
      const double kRTof = std::fabs(outs36.at(i).time_of_flight -
                                     refs.at(i).time_of_flight);
      EXPECT_LE(kRElevMoa, kMoaGran);
      EXPECT_LE(kRDeflMoa, kMoaGran);
      EXPECT_LE(kRVel, kVelGran);
      EXPECT_LE(kREnergy, kEnergyGran);
      EXPECT_LE(kRTof, kTofGran);
      // Delta vs the floor family (negative = consistent-with-resolution;
      // positive = residual-is-model-or-reference); logged, never gated.
      const double kDElevIn = kRElevIn - reference_matrix::kEpsElevIn;
      const double kDElevMoa = kRElevMoa - reference_matrix::kEpsElevMoa;
      const double kDDeflMoa = kRDeflMoa - reference_matrix::kEpsDeflMoa;
      const double kDVel = kRVel - reference_matrix::kEpsVelFps;
      const double kDEnergy = kREnergy - reference_matrix::kEpsEnergyFtLbs;
      const double kDTof = kRTof - reference_matrix::kEpsTofSec;
      if (!(kDElevIn >= 0.0)) {
        SCOPED_TRACE("elev: consistent-within-resolution");
      } else {
        SCOPED_TRACE("elev: residual-is-model-or-reference");
      }
      if (kRElevIn > worst.elev_in) worst.elev_in = kRElevIn;
      if (kRElevMoa > worst.elev_moa) worst.elev_moa = kRElevMoa;
      if (kRDeflMoa > worst.defl_moa) worst.defl_moa = kRDeflMoa;
      if (kRVel > worst.vel) worst.vel = kRVel;
      if (kREnergy > worst.energy) worst.energy = kREnergy;
      if (kRTof > worst.tof) worst.tof = kRTof;
      if (kDElevIn > worst_delta.elev_in) worst_delta.elev_in = kDElevIn;
      if (kDElevMoa > worst_delta.elev_moa) worst_delta.elev_moa = kDElevMoa;
      if (kDDeflMoa > worst_delta.defl_moa) worst_delta.defl_moa = kDDeflMoa;
      if (kDVel > worst_delta.vel) worst_delta.vel = kDVel;
      if (kDEnergy > worst_delta.energy) worst_delta.energy = kDEnergy;
      if (kDTof > worst_delta.tof) worst_delta.tof = kDTof;
      csv += kCases[c].id + std::string(",") + reference_matrix::Fmt(kRange) +
             "," + reference_matrix::Fmt(kRElevIn) + "," +
             reference_matrix::Fmt(kRElevMoa) + "," +
             reference_matrix::Fmt(kRDeflMoa) + "," +
             reference_matrix::Fmt(kRVel) + "," +
             reference_matrix::Fmt(kREnergy) + "," +
             reference_matrix::Fmt(kRTof) + "\n";
    }
    case_ids.push_back(kCases[c].id);
    case_tags.push_back(tags);
    case_worst.push_back(worst);
    RecordProperty(
        std::string("delta.") + kCases[c].stem + ".elev_in_worst",
        worst_delta.elev_in);
    RecordProperty(
        std::string("delta.") + kCases[c].stem + ".elev_moa_worst",
        worst_delta.elev_moa);
    RecordProperty(std::string("delta.") + kCases[c].stem + ".tof_worst",
                   worst_delta.tof);
  }

  // Model-form spot (spec section 10.5): G7-vs-G1 scenario swap on ICAO.
  nlohmann::json icao_root;
  try {
    icao_root = LoadReferenceCase(LOB_VALIDATION_CASES_DIR, "icao");
  } catch (const std::exception& e) {
    FAIL() << e.what();
  }
  const nlohmann::json kIcaoRanges = icao_root.at("ranges_ft");
  std::array<uint32_t, kNumRanges> icao_ranges = {};
  for (size_t i = 0; i < kNumRanges; ++i) {
    ASSERT_TRUE(kIcaoRanges.at(i).is_number());
    icao_ranges.at(i) =
        static_cast<uint32_t>(kIcaoRanges.at(i).get<double>());
  }
  lob::Builder g1_builder = BuildIcaoCase();
  g1_builder.BCDragFunction(lob::DragFunctionT::kG1);
  std::array<lob::Output, kNumRanges> g7_outs = {};
  std::array<lob::Output, kNumRanges> g1_outs = {};
  ASSERT_EQ(SolveN(BuildAtStep(BuildIcaoCase(), 36U), icao_ranges, &g7_outs),
            kNumRanges);
  ASSERT_EQ(SolveN(BuildAtStep(g1_builder, 36U), icao_ranges, &g1_outs),
            kNumRanges);
  double struct_in = 0.0;
  double struct_moa = 0.0;
  for (size_t i = 0; i < kNumRanges; ++i) {
    const double kRange = static_cast<double>(icao_ranges.at(i));
    const double kDIn =
        std::fabs(g7_outs.at(i).elevation - g1_outs.at(i).elevation);
    const double kDMoa = std::fabs(
        lob::InchToMoa(g7_outs.at(i).elevation, kRange) -
        lob::InchToMoa(g1_outs.at(i).elevation, kRange));
    if (kDIn > struct_in) struct_in = kDIn;
    if (kDMoa > struct_moa) struct_moa = kDMoa;
  }
  EXPECT_GT(struct_in, 0.0);
  RecordProperty("structural.g7_vs_g1.delta_elev_in_max", struct_in);
  RecordProperty("structural.g7_vs_g1.delta_elev_moa_max", struct_moa);

  // Coverage matrix: one cell per envelope tag, worst residual per channel
  // over the covering cases. Every Phase 3 cell is provisional (u_ref
  // unknown, single borrowed trajectory per cell).
  std::vector<std::string> tag_order;
  for (size_t c = 0; c < case_tags.size(); ++c) {
    for (size_t t = 0; t < case_tags.at(c).size(); ++t) {
      bool seen = false;
      for (size_t u = 0; u < tag_order.size(); ++u) {
        if (tag_order.at(u) == case_tags.at(c).at(t)) {
          seen = true;
          break;
        }
      }
      if (!seen) tag_order.push_back(case_tags.at(c).at(t));
    }
  }
  nlohmann::json cells = nlohmann::json::array();
  for (size_t u = 0; u < tag_order.size(); ++u) {
    reference_matrix::Worst worst;
    nlohmann::json covering = nlohmann::json::array();
    for (size_t c = 0; c < case_ids.size(); ++c) {
      bool covers = false;
      for (size_t t = 0; t < case_tags.at(c).size(); ++t) {
        if (case_tags.at(c).at(t) == tag_order.at(u)) {
          covers = true;
          break;
        }
      }
      if (!covers) continue;
      covering.push_back(case_ids.at(c));
      const reference_matrix::Worst& kW = case_worst.at(c);
      if (kW.elev_in > worst.elev_in) worst.elev_in = kW.elev_in;
      if (kW.elev_moa > worst.elev_moa) worst.elev_moa = kW.elev_moa;
      if (kW.defl_moa > worst.defl_moa) worst.defl_moa = kW.defl_moa;
      if (kW.vel > worst.vel) worst.vel = kW.vel;
      if (kW.energy > worst.energy) worst.energy = kW.energy;
      if (kW.tof > worst.tof) worst.tof = kW.tof;
    }
    nlohmann::json residual;
    residual["elevation_in"] = worst.elev_in;
    residual["elevation_moa"] = worst.elev_moa;
    residual["deflection_moa"] = worst.defl_moa;
    residual["velocity_fps"] = worst.vel;
    residual["energy_ft_lbf"] = worst.energy;
    residual["time_of_flight_s"] = worst.tof;
    nlohmann::json cell;
    cell["tags"] = nlohmann::json::array({tag_order.at(u)});
    cell["cases"] = covering;
    cell["worst_residual"] = residual;
    cell["verdict"] = "provisional";
    cells.push_back(cell);
  }

  nlohmann::json structural = nlohmann::json::array();
  {
    nlohmann::json entry;
    entry["check"] = "g7-vs-g1";
    entry["case"] = "ref-icao";
    entry["delta_elev_in_max"] = struct_in;
    entry["delta_elev_moa_max"] = struct_moa;
    entry["status"] = "measured";
    structural.push_back(entry);
  }
  const char* kNaChecks[] = {"density-lapse", "coriolis", "spin-drift",
                             "aerodynamic-jump"};
  const char* kNaReasons[] = {
      "Fast-only corpus: reference cases carry density_path fast, no "
      "lapse-scaled rung to compare against",
      "corpus builders set no latitude/azimuth inputs; calm-calm deflection "
      "is the only observed channel",
      "corpus tags spin-off on all six cases; no spin-on trajectory borrowed",
      "corpus builders set no jump inputs; deflection is calm throughout"};
  for (size_t i = 0; i < 4; ++i) {
    nlohmann::json entry;
    entry["check"] = kNaChecks[i];
    entry["status"] = "not_applicable";
    entry["reason"] = kNaReasons[i];
    structural.push_back(entry);
  }

  nlohmann::json report;
  report["provenance"] = {{"lob_version", lob::Version()},
                          {"git_sha", LOB_GIT_SHA},
                          {"generator", "ReferenceMatrix.FullMatrix"},
                          {"gate", "LOB_FULL_MATRIX=1"},
                          {"step_ladder_in", {36, 18, 9}}};
  report["cells"] = cells;
  report["unclaimed"] = {"range-1000yd-plus", "wind-profile*", "spin-*",
                         "density-lapse-tail", "drag-bands", "drag-custom",
                         "field-radar"};
  report["structural"] = structural;

  const std::string kDir = LOB_VALIDATION_DIR;
  {
    std::ofstream out((kDir + "/envelope_report.json").c_str());
    ASSERT_TRUE(out.good());
    out << report.dump(2) << "\n";
  }
  {
    std::ofstream out((kDir + "/reference_matrix.csv").c_str());
    ASSERT_TRUE(out.good());
    out << csv;
  }
}

}  // namespace tests
