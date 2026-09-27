// Copyright (c) 2026  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <array>
#include <cmath>
#include <cstdint>
#include <fstream>
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
