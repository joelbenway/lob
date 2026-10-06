// Copyright (c) 2026  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <fstream>
#include <iomanip>
#include <limits>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "lob/lob.hpp"
#include "testing.hpp"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#ifndef LOB_VALIDATION_CASES_DIR
#error "LOB_VALIDATION_CASES_DIR must be defined by CMake"
#endif

namespace tests {

TEST(ReferenceJson, ParsesFlatCaseShape) {
  const nlohmann::json kRoot = nlohmann::json::parse(
      "{\"id\":\"ref-icao\",\"ranges_ft\":[0,150],\"expected\":[{\"range_ft\":"
      "0,"
      "\"velocity_fps\":2800,\"u_ref\":\"unknown\"}],\"status\":"
      "\"provisional\","
      "\"supersedes\":null}");
  EXPECT_EQ(kRoot.at("id").get<std::string>(), "ref-icao");
  EXPECT_DOUBLE_EQ(kRoot.at("ranges_ft").at(1).get<double>(), 150.0);
  EXPECT_DOUBLE_EQ(kRoot.at("expected").at(0).at("velocity_fps").get<double>(),
                   2800.0);
  EXPECT_EQ(kRoot.at("expected").at(0).at("u_ref").get<std::string>(),
            "unknown");
  EXPECT_TRUE(kRoot.at("supersedes").is_null());
  EXPECT_EQ(kRoot.at("status").get<std::string>(), "provisional");
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
TEST(ReferenceJson, RejectsMalformedDocuments) {
  for (const char* bad :
       {"{\"a\":1,}", "{\"a\":", "{\"a\" 1}", "[1,2", "{\"a\":01}", ""}) {
    EXPECT_THROW((void)nlohmann::json::parse(bad), nlohmann::json::parse_error)
        << "accepted: " << bad;
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
namespace {

// Reads <dir>/reference_<stem>.json and returns the parsed document.
// Throws std::runtime_error (path + cause) on missing files, parse errors,
// or schema violations — callers FAIL with the message, never fall back to
// embedded literals.
inline nlohmann::json LoadReferenceCase(const std::string& dir,
                                        const std::string& stem) {
  const std::string kPath = dir + "/reference_" + stem + ".json";
  const std::ifstream kIn(kPath.c_str());
  if (!kIn) {
    throw std::runtime_error("reference case file not found: " + kPath);
  }
  std::ostringstream raw;
  raw << kIn.rdbuf();
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
    throw std::runtime_error("schema type error in " + kPath + ": " + e.what());
  }
}

// The C1 fixture IS the ICAO case: byte-for-byte the same setter list.
inline lob::Builder BuildIcaoCase() { return MakeC1IcaoBuilder(); }

}  // namespace

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
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
      "range-0-300yd",         "range-300-1000yd",  "regime-supersonic",
      "regime-transonic-tail", "wind-calm",         "spin-off",
      "density-fast",          "drag-g7-single-bc", "atm-isa"};
  const nlohmann::json kTags = root.at("envelope_tags");
  EXPECT_TRUE(kTags.is_array());
  ASSERT_EQ(kTags.size(), kExpectedTags.size());
  for (size_t i = 0; i < kExpectedTags.size(); ++i) {
    EXPECT_TRUE(kTags.at(i).is_string());
    EXPECT_EQ(kTags.at(i).get<std::string>(), kExpectedTags[i]) << "tag " << i;
  }

  constexpr size_t kNumRanges = 12;
  const std::array<uint32_t, kNumRanges> kRanges = {0U,    150U,  300U,  600U,
                                                    900U,  1200U, 1500U, 1800U,
                                                    2100U, 2400U, 2700U, 3000U};
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
    const nlohmann::json& row = kRows.at(i);
    EXPECT_TRUE(row.at("u_ref").is_string());
    EXPECT_EQ(row.at("u_ref").get<std::string>(), "unknown") << "row " << i;
    lob::Output out{};
    EXPECT_TRUE(row.at("range_ft").is_number());
    out.range = static_cast<uint32_t>(row.at("range_ft").get<double>());
    EXPECT_EQ(out.range, kRanges.at(i)) << "row " << i;
    EXPECT_TRUE(row.at("velocity_fps").is_number());
    out.velocity = static_cast<uint16_t>(row.at("velocity_fps").get<double>());
    EXPECT_TRUE(row.at("energy_ft_lbf").is_number());
    out.energy = static_cast<uint32_t>(row.at("energy_ft_lbf").get<double>());
    EXPECT_TRUE(row.at("elevation_in").is_number());
    out.elevation = row.at("elevation_in").get<double>();
    EXPECT_TRUE(row.at("deflection_in").is_number());
    out.deflection = row.at("deflection_in").get<double>();
    EXPECT_TRUE(row.at("time_of_flight_s").is_number());
    out.time_of_flight = row.at("time_of_flight_s").get<double>();
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
                   lob::InchT(std::numeric_limits<double>::quiet_NaN()),
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
    const double kMoaRes =
        std::fabs(lob::InchToMoa(solutions.at(i).elevation,
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
namespace {

namespace reference_matrix {

// C1 floor family reused as eps_num for every case. Cross-case floor reuse is
// conservative: the C1-ICAO floors_18_9 cell in
// test/validation/baselines/floors.json is the only measured set; per-case
// floors arrive with envelope expansion (ledger note).
constexpr double kEpsElevIn = 4.15814e-05;
constexpr double kEpsElevMoa = 3.97148e-06;
constexpr double kEpsDeflMoa = 0.0;
constexpr double kEpsTofSec = 6.12488e-08;
constexpr double kEpsVelFps = 1.0;       // 1 LSB of U16 truncation
constexpr double kEpsEnergyFtLbs = 1.0;  // 1 LSB of U32 truncation

inline double ReqDouble(const nlohmann::json& obj, const char* key) {
  const nlohmann::json& val = obj.at(key);
  if (!val.is_number()) {
    throw std::runtime_error(std::string("expected number at key: ") + key);
  }
  return val.get<double>();
}

inline std::string ReqString(const nlohmann::json& obj, const char* key) {
  const nlohmann::json& val = obj.at(key);
  if (!val.is_string()) {
    throw std::runtime_error(std::string("expected string at key: ") + key);
  }
  return val.get<std::string>();
}

struct Worst {
  double elev_in = 0.0;
  double elev_moa = 0.0;
  double defl_moa = 0.0;
  double vel = 0.0;
  double energy = 0.0;
  double tof = 0.0;
};

inline std::string Fmt(double value) {
  constexpr int kPrecisionDigits = 10;
  std::ostringstream out;
  out << std::setprecision(kPrecisionDigits) << value;
  return out.str();
}

}  // namespace reference_matrix

// Reference-case builders (BuildAltitude4500Case et al.) now live in
// testing.hpp, shared with BudgetAssemble.OfflineDocuments.

}  // namespace

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
TEST(ReferenceMatrix, FullMatrix) {
  struct MatrixCase {
    const char* stem;
    const char* id;
    lob::Builder (*build)();
  };
  constexpr size_t kNumCases = 6;
  const std::array<MatrixCase, kNumCases> kCases = {
      {{"icao", "ref-icao", BuildIcaoCase},
       {"altitude4500", "ref-altitude4500", BuildAltitude4500Case},
       {"hot_lowp", "ref-hot-lowp", BuildHotLowPCase},
       {"barometer", "ref-barometer", BuildBarometerCase},
       {"humidity", "ref-humidity", BuildHumidCase},
       {"weather_station", "ref-weather-station", BuildWeatherStationCase}}};

  constexpr size_t kNumRanges = 12;
  std::string csv;
  csv +=
      "case_id,range_ft,r_elev_in,r_elev_moa,r_defl_moa,r_vel_fps,r_energy_"
      "ftlbf,r_tof_s\n";

  std::vector<std::string> case_ids;
  std::vector<std::vector<std::string> > case_tags;
  std::vector<reference_matrix::Worst> case_worst;

  for (const MatrixCase& matrix_case : kCases) {
    SCOPED_TRACE(testing::Message() << "case=" << matrix_case.stem);
    nlohmann::json root;
    try {
      root = LoadReferenceCase(LOB_VALIDATION_CASES_DIR, matrix_case.stem);
    } catch (const std::exception& e) {
      FAIL() << e.what();
    }
    EXPECT_EQ(reference_matrix::ReqString(root, "id"), matrix_case.id);
    EXPECT_EQ(reference_matrix::ReqString(root, "status"), "provisional");

    const nlohmann::json kTags = root.at("envelope_tags");
    ASSERT_TRUE(kTags.is_array());
    std::vector<std::string> tags;
    for (const auto& tag : kTags) {
      ASSERT_TRUE(tag.is_string());
      tags.push_back(tag.get<std::string>());
    }

    const nlohmann::json kRangesJson = root.at("ranges_ft");
    ASSERT_TRUE(kRangesJson.is_array());
    ASSERT_EQ(kRangesJson.size(), kNumRanges);
    std::array<uint32_t, kNumRanges> ranges = {};
    for (size_t i = 0; i < kNumRanges; ++i) {
      ASSERT_TRUE(kRangesJson.at(i).is_number());
      ranges.at(i) = static_cast<uint32_t>(kRangesJson.at(i).get<double>());
    }

    const nlohmann::json kRows = root.at("expected");
    ASSERT_TRUE(kRows.is_array());
    ASSERT_EQ(kRows.size(), kNumRanges);
    const nlohmann::json kGran = root.at("reporting_granularity");
    const double kVelGran = reference_matrix::ReqDouble(kGran, "velocity_fps");
    const double kEnergyGran =
        reference_matrix::ReqDouble(kGran, "energy_ft_lbf");
    const double kMoaGran = reference_matrix::ReqDouble(kGran, "elevation_moa");
    const double kTofGran =
        reference_matrix::ReqDouble(kGran, "time_of_flight_s");
    std::vector<lob::Output> refs;
    refs.reserve(kNumRanges);
    for (size_t i = 0; i < kNumRanges; ++i) {
      const nlohmann::json& row = kRows.at(i);
      EXPECT_EQ(reference_matrix::ReqString(row, "u_ref"), "unknown");
      lob::Output out{};
      out.range =
          static_cast<uint32_t>(reference_matrix::ReqDouble(row, "range_ft"));
      EXPECT_EQ(out.range, ranges.at(i));
      out.velocity = static_cast<uint16_t>(
          reference_matrix::ReqDouble(row, "velocity_fps"));
      out.energy = static_cast<uint32_t>(
          reference_matrix::ReqDouble(row, "energy_ft_lbf"));
      out.elevation = reference_matrix::ReqDouble(row, "elevation_in");
      out.deflection = reference_matrix::ReqDouble(row, "deflection_in");
      out.time_of_flight = reference_matrix::ReqDouble(row, "time_of_flight_s");
      refs.push_back(out);
    }

    // Ladder shape: solve at 36/18/9; deltas are sanity-logged, not gated.
    std::array<lob::Output, kNumRanges> outs36 = {};
    std::array<lob::Output, kNumRanges> outs18 = {};
    std::array<lob::Output, kNumRanges> outs9 = {};
    ASSERT_EQ(SolveN(BuildAtStep(matrix_case.build(), 36U), ranges, &outs36),
              kNumRanges);
    ASSERT_EQ(SolveN(BuildAtStep(matrix_case.build(), 18U), ranges, &outs18),
              kNumRanges);
    ASSERT_EQ(SolveN(BuildAtStep(matrix_case.build(), 9U), ranges, &outs9),
              kNumRanges);
    double ladder_36_18 = 0.0;
    double ladder_18_9 = 0.0;
    for (size_t i = 0; i < kNumRanges; ++i) {
      const double kD1 =
          std::fabs(outs36.at(i).elevation - outs18.at(i).elevation);
      const double kD2 =
          std::fabs(outs18.at(i).elevation - outs9.at(i).elevation);
      ladder_36_18 = std::max(ladder_36_18, kD1);
      ladder_18_9 = std::max(ladder_18_9, kD2);
    }
    RecordProperty(std::string("ladder.") + matrix_case.stem + ".elev_in_36_18",
                   ladder_36_18);
    RecordProperty(std::string("ladder.") + matrix_case.stem + ".elev_in_18_9",
                   ladder_18_9);

    // Loader round-trip: JSON numbers reproduce the C++-literal outcome.
    VerifySolutions(
        outs36, refs,
        {lob::FpsT(kVelGran), lob::FtLbsT(kEnergyGran), lob::MoaT(kMoaGran),
         lob::InchT(std::numeric_limits<double>::quiet_NaN()),
         lob::SecT(kTofGran)});

    // Decomposition at the 36-in rung vs expected, C1 floors as eps_num.
    reference_matrix::Worst worst;
    reference_matrix::Worst worst_delta;
    for (size_t i = 0; i < kNumRanges; ++i) {
      SCOPED_TRACE(testing::Message() << "range_ft=" << ranges.at(i));
      const auto kRange = static_cast<double>(ranges.at(i));
      const double kRElevIn =
          std::fabs(outs36.at(i).elevation - refs.at(i).elevation);
      const double kRElevMoa =
          std::fabs(lob::InchToMoa(outs36.at(i).elevation, kRange) -
                    lob::InchToMoa(refs.at(i).elevation, kRange));
      const double kRDeflMoa =
          std::fabs(lob::InchToMoa(outs36.at(i).deflection, kRange) -
                    lob::InchToMoa(refs.at(i).deflection, kRange));
      const double kRVel =
          std::fabs(static_cast<double>(outs36.at(i).velocity) -
                    static_cast<double>(refs.at(i).velocity));
      const double kREnergy =
          std::fabs(static_cast<double>(outs36.at(i).energy) -
                    static_cast<double>(refs.at(i).energy));
      const double kRTof =
          std::fabs(outs36.at(i).time_of_flight - refs.at(i).time_of_flight);
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
      worst.elev_in = std::max(worst.elev_in, kRElevIn);
      worst.elev_moa = std::max(worst.elev_moa, kRElevMoa);
      worst.defl_moa = std::max(worst.defl_moa, kRDeflMoa);
      worst.vel = std::max(worst.vel, kRVel);
      worst.energy = std::max(worst.energy, kREnergy);
      worst.tof = std::max(worst.tof, kRTof);
      worst_delta.elev_in = std::max(worst_delta.elev_in, kDElevIn);
      worst_delta.elev_moa = std::max(worst_delta.elev_moa, kDElevMoa);
      worst_delta.defl_moa = std::max(worst_delta.defl_moa, kDDeflMoa);
      worst_delta.vel = std::max(worst_delta.vel, kDVel);
      worst_delta.energy = std::max(worst_delta.energy, kDEnergy);
      worst_delta.tof = std::max(worst_delta.tof, kDTof);
      csv += matrix_case.id + std::string(",") + reference_matrix::Fmt(kRange) +
             "," + reference_matrix::Fmt(kRElevIn) + "," +
             reference_matrix::Fmt(kRElevMoa) + "," +
             reference_matrix::Fmt(kRDeflMoa) + "," +
             reference_matrix::Fmt(kRVel) + "," +
             reference_matrix::Fmt(kREnergy) + "," +
             reference_matrix::Fmt(kRTof) + "\n";
    }
    case_ids.emplace_back(matrix_case.id);
    case_tags.push_back(tags);
    case_worst.push_back(worst);
    RecordProperty(std::string("delta.") + matrix_case.stem + ".elev_in_worst",
                   worst_delta.elev_in);
    RecordProperty(std::string("delta.") + matrix_case.stem + ".elev_moa_worst",
                   worst_delta.elev_moa);
    RecordProperty(std::string("delta.") + matrix_case.stem + ".tof_worst",
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
    icao_ranges.at(i) = static_cast<uint32_t>(kIcaoRanges.at(i).get<double>());
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
    const auto kRange = static_cast<double>(icao_ranges.at(i));
    const double kDIn =
        std::fabs(g7_outs.at(i).elevation - g1_outs.at(i).elevation);
    const double kDMoa =
        std::fabs(lob::InchToMoa(g7_outs.at(i).elevation, kRange) -
                  lob::InchToMoa(g1_outs.at(i).elevation, kRange));
    struct_in = std::max(struct_in, kDIn);
    struct_moa = std::max(struct_moa, kDMoa);
  }
  EXPECT_GT(struct_in, 0.0);
  RecordProperty("structural.g7_vs_g1.delta_elev_in_max", struct_in);
  RecordProperty("structural.g7_vs_g1.delta_elev_moa_max", struct_moa);

  // Coverage matrix: one cell per envelope tag, worst residual per channel
  // over the covering cases. Every Phase 3 cell is provisional (u_ref
  // unknown, single borrowed trajectory per cell).
  std::vector<std::string> tag_order;
  for (const auto& tags : case_tags) {
    for (const auto& tag : tags) {
      bool seen = false;
      for (const auto& ordered : tag_order) {
        if (ordered == tag) {
          seen = true;
          break;
        }
      }
      if (!seen) {
        tag_order.push_back(tag);
      }
    }
  }
  nlohmann::json cells = nlohmann::json::array();
  for (const auto& tag : tag_order) {
    reference_matrix::Worst worst;
    nlohmann::json covering = nlohmann::json::array();
    for (size_t case_idx = 0; case_idx < case_ids.size(); ++case_idx) {
      bool covers = false;
      for (const auto& case_tag : case_tags.at(case_idx)) {
        if (case_tag == tag) {
          covers = true;
          break;
        }
      }
      if (!covers) {
        continue;
      }
      covering.push_back(case_ids.at(case_idx));
      const reference_matrix::Worst& worst_case = case_worst.at(case_idx);
      worst.elev_in = std::max(worst.elev_in, worst_case.elev_in);
      worst.elev_moa = std::max(worst.elev_moa, worst_case.elev_moa);
      worst.defl_moa = std::max(worst.defl_moa, worst_case.defl_moa);
      worst.vel = std::max(worst.vel, worst_case.vel);
      worst.energy = std::max(worst.energy, worst_case.energy);
      worst.tof = std::max(worst.tof, worst_case.tof);
    }
    nlohmann::json residual;
    residual["elevation_in"] = worst.elev_in;
    residual["elevation_moa"] = worst.elev_moa;
    residual["deflection_moa"] = worst.defl_moa;
    residual["velocity_fps"] = worst.vel;
    residual["energy_ft_lbf"] = worst.energy;
    residual["time_of_flight_s"] = worst.tof;
    nlohmann::json cell;
    cell["tags"] = nlohmann::json::array({tag});
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
  constexpr size_t kNumStructuralNa = 4;
  const std::array<const char*, kNumStructuralNa> kNaChecks = {
      {"density-lapse", "coriolis", "spin-drift", "aerodynamic-jump"}};
  const std::array<const char*, kNumStructuralNa> kNaReasons = {
      {"Fast-only corpus: reference cases carry density_path fast, no "
       "lapse-scaled rung to compare against",
       "corpus builders set no latitude/azimuth inputs; calm-calm deflection "
       "is the only observed channel",
       "corpus tags spin-off on all six cases; no spin-on trajectory borrowed",
       "corpus builders set no jump inputs; deflection is calm throughout"}};
  for (size_t i = 0; i < kNumStructuralNa; ++i) {
    nlohmann::json entry;
    entry["check"] = kNaChecks.at(i);
    entry["status"] = "not_applicable";
    entry["reason"] = kNaReasons.at(i);
    structural.push_back(entry);
  }

  constexpr int kStepCoarseIn = 36;
  constexpr int kStepMidIn = 18;
  constexpr int kStepFineIn = 9;
  nlohmann::json report;
  report["provenance"] = {
      {"lob_version", lob::Version()},
      {"git_sha", LOB_GIT_SHA},
      {"generator", "ReferenceMatrix.FullMatrix"},
      {"gate", "LOB_FULL_MATRIX=1"},
      {"step_ladder_in", {kStepCoarseIn, kStepMidIn, kStepFineIn}}};
  report["cells"] = cells;
  report["unclaimed"] = {"range-1000yd-plus",  "wind-profile*", "spin-*",
                         "density-lapse-tail", "drag-bands",    "drag-custom",
                         "field-radar"};
  report["structural"] = structural;

  const std::string kDir = LOB_VALIDATION_DIR;
  const std::string kTarget = kDir + "/envelope_report.json";
  // Atomic publish (temp+rename): readers either see the old or the new
  // file, never a torn write. Tmp suffix is pid-unique; only this test
  // writes, so no writer-writer collision.
#ifdef _WIN32
  const std::string kTmp =
      kTarget + ".tmp" + std::to_string(::GetCurrentProcessId());
#else
  const std::string kTmp = kTarget + ".tmp" + std::to_string(::getpid());
#endif
  {
    std::ofstream out(kTmp.c_str());
    ASSERT_TRUE(out.good()) << "cannot open " << kTmp;
    out << report.dump(2) << "\n";
    out.close();
    ASSERT_TRUE(out.good()) << "write failed " << kTmp;
  }
#ifdef _WIN32
  ASSERT_TRUE(::MoveFileExA(kTmp.c_str(), kTarget.c_str(),
                            MOVEFILE_REPLACE_EXISTING) != 0)
      << "rename " << kTmp;
#else
  ASSERT_EQ(std::rename(kTmp.c_str(), kTarget.c_str()), 0) << "rename " << kTmp;
#endif
  {
    std::ofstream out((kDir + "/reference_matrix.csv").c_str());
    ASSERT_TRUE(out.good());
    out << csv;
  }
}

}  // namespace tests
