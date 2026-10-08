// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <nlohmann/json.hpp>
#include <random>
#include <string>
#include <vector>

#include "lob/lob.hpp"
#include "sampler.hpp"

namespace mc {

// Manifest dimension families (plan Task 2 schema). Scalar families sample
// one value per sample; wind_pair samples speed+heading jointly; profile
// samples per-station speed+heading (+height only when shear alpha != 0).
constexpr const char* kFamilyNormal = "normal";
constexpr const char* kFamilyUniform = "uniform";
constexpr const char* kFamilyTriangular = "triangular";
constexpr const char* kFamilyFixed = "fixed";
constexpr const char* kFamilyWindPair = "wind_pair";
constexpr const char* kFamilyProfile = "profile";

// Only "independent" exists: correlated sampling needs a covariance input
// that doesn't exist, so any other flag fails the manifest loudly.
constexpr const char* kCorrelationIndependent = "independent";

constexpr const char* kInputShearExponent = "shear_exponent";
constexpr const char* kInputWindProfile = "wind_profile";

constexpr std::uint16_t kDefaultStepIn = 36U;
constexpr const char* kDefaultDensityPath = "fast";
constexpr const char* kDensityFast = "fast";
constexpr const char* kDensityLapseScaledInverseTail =
    "lapse-scaled-inverse-tail";

struct WindStation {
  double range_ft = 0.0;
  double speed_mph = 0.0;
  double speed_sigma = 0.0;
  double heading_deg = 0.0;
  double heading_sigma = 0.0;
  double height_ft = 0.0;
  double height_sigma = 0.0;
};

struct Dimension {
  std::string input;
  std::string family;
  double mean = 0.0;
  double sigma = 0.0;
  double uniform_lo = 0.0;
  double uniform_hi = 0.0;
  double tri_lo = 0.0;
  double tri_mode = 0.0;
  double tri_hi = 0.0;
  double fixed_value = 0.0;
  double speed_mean = 0.0;
  double speed_sigma = 0.0;
  double heading_mean = 0.0;
  double heading_sigma = 0.0;
  std::vector<WindStation> stations;
  std::string correlation = kCorrelationIndependent;
  bool has_truncation = false;
  double trunc_lo = 0.0;
  double trunc_hi = 0.0;
  double baseline = 0.0;
};

struct Scenario {
  std::string name;
  std::string drag;
  double weight = 0.0;
};

struct CellSet {
  std::vector<Dimension> dimensions;
  std::vector<Scenario> scenarios;
};

struct SolverConfig {
  std::uint16_t step_in = kDefaultStepIn;
  std::string density_path = kDefaultDensityPath;
  std::vector<std::uint32_t> ranges;
};

struct RunManifest {
  std::string run_id;
  std::string cell;
  std::uint64_t seed = 0U;
  std::uint64_t samples = 0U;
  std::uint64_t workers = 0U;
  SolverConfig solver;
  std::vector<Dimension> dimensions;
  std::vector<Scenario> scenarios;
  std::map<std::string, CellSet> dimension_sets;
  bool synthetic_illustrative_only = false;
};

struct BranchFlags {
  bool build_failed = false;
  bool reached_all = false;
  std::size_t fall_short_index = 0U;
  // Analytic Miller-rule flag (0<|Sg|<1), not solver-reported: the solver's
  // own stop surfaces via reached_all/fall_short_index.
  bool miller_unstable = false;
  bool angle_cap_hit = false;
  // Manifest echo, not solver observation (the SolveAngle dynamic-tail
  // switch inside SolveInverse has no public per-sample signal to record).
  std::string configured_density_path = kDefaultDensityPath;
};

struct TrajectorySample {
  std::size_t index = 0U;
  std::vector<double> draws;
  std::vector<lob::Output> forward;
  std::vector<lob::Output> inverse;
  std::size_t forward_count = 0U;
  std::size_t inverse_count = 0U;
  BranchFlags flags;
  double stability = std::numeric_limits<double>::quiet_NaN();
};

// Shared worker-pool plan: threads partition the fixed logical streams,
// results assemble by sample index, never completion order. Stream count
// always equals sample count, so sample i comes from stream (i % S) with a
// per-stream Subseed(seed, stream) engine — the Task 1 scheme verbatim.
struct RunPlan {
  std::uint64_t seed = 0U;
  const lob::Builder* base = nullptr;
  const std::vector<Dimension>* dimensions = nullptr;
  const std::vector<Scenario>* scenarios = nullptr;
  const std::vector<std::uint32_t>* ranges = nullptr;
  std::uint16_t step_in = kDefaultStepIn;
  const std::string* configured_density_path = nullptr;
  std::size_t draw_width = 0U;
};

inline bool ReadDouble(const nlohmann::json& obj, const char* key, double* out,
                       std::string* error) {
  if (!obj.contains(key) || !obj.at(key).is_number()) {
    *error = std::string("manifest: dimension needs numeric '") + key + "'";
    return false;
  }
  *out = obj.at(key).get<double>();
  if (!std::isfinite(*out)) {
    *error = std::string("manifest: dimension '") + key + "' not finite";
    return false;
  }
  return true;
}

inline bool ReadTruncation(const nlohmann::json& dim, Dimension* out,
                           std::string* error) {
  if (!dim.contains("truncation")) {
    return true;
  }
  const nlohmann::json& trunc = dim.at("truncation");
  if (!trunc.is_object()) {
    *error = "manifest: 'truncation' must be an object";
    return false;
  }
  if (!ReadDouble(trunc, "lo", &out->trunc_lo, error)) {
    return false;
  }
  if (!ReadDouble(trunc, "hi", &out->trunc_hi, error)) {
    return false;
  }
  if (!(out->trunc_hi >= out->trunc_lo)) {
    *error = "manifest: truncation hi below lo";
    return false;
  }
  out->has_truncation = true;
  return true;
}

inline Truncation DrawWindow(const Dimension& dim, double default_lo,
                             double default_hi) {
  Truncation trunc;
  trunc.lo = dim.has_truncation ? dim.trunc_lo : default_lo;
  trunc.hi = dim.has_truncation ? dim.trunc_hi : default_hi;
  return trunc;
}

inline Truncation WideWindow() {
  constexpr double kBound = std::numeric_limits<double>::max();
  Truncation trunc;
  trunc.lo = -kBound;
  trunc.hi = kBound;
  return trunc;
}

// Scalar families share one draw column; the caller records the value.
inline bool DrawScalar(std::mt19937_64& engine, const Dimension& dim,
                       double* out) {
  if (dim.family == kFamilyNormal) {
    const Truncation kTrunc =
        DrawWindow(dim, -std::numeric_limits<double>::max(),
                   std::numeric_limits<double>::max());
    const Sample kSample = SampleNormal(engine, dim.mean, dim.sigma, kTrunc);
    if (!kSample.ok) {
      return false;
    }
    *out = kSample.value;
    return true;
  }
  if (dim.family == kFamilyUniform) {
    const Truncation kTrunc = DrawWindow(dim, dim.uniform_lo, dim.uniform_hi);
    const Sample kSample =
        SampleUniform(engine, dim.uniform_lo, dim.uniform_hi, kTrunc);
    if (!kSample.ok) {
      return false;
    }
    *out = kSample.value;
    return true;
  }
  if (dim.family == kFamilyTriangular) {
    const Truncation kTrunc = DrawWindow(dim, dim.tri_lo, dim.tri_hi);
    const Sample kSample =
        SampleTriangular(engine, dim.tri_lo, dim.tri_mode, dim.tri_hi, kTrunc);
    if (!kSample.ok) {
      return false;
    }
    *out = kSample.value;
    return true;
  }
  if (dim.family == kFamilyFixed) {
    *out = dim.fixed_value;
    return true;
  }
  return false;
}

inline bool DragForName(const std::string& drag, lob::DragFunctionT* out) {
  if (drag == "G1") {
    *out = lob::DragFunctionT::kG1;
    return true;
  }
  if (drag == "G2") {
    *out = lob::DragFunctionT::kG2;
    return true;
  }
  if (drag == "G5") {
    *out = lob::DragFunctionT::kG5;
    return true;
  }
  if (drag == "G6") {
    *out = lob::DragFunctionT::kG6;
    return true;
  }
  if (drag == "G7") {
    *out = lob::DragFunctionT::kG7;
    return true;
  }
  if (drag == "G8") {
    *out = lob::DragFunctionT::kG8;
    return true;
  }
  return false;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
inline bool ApplyScalar(lob::Builder* builder, const std::string& input,
                        double value) {
  constexpr double kMaxUint16 = 65535.0;
  if (input == "velocity_fps") {
    // Phase 2 integer rule: velocity snaps to whole fps via llround.
    if (!std::isfinite(value) || (value < 0.0) || (value > kMaxUint16)) {
      return false;
    }
    const auto kSnapped = static_cast<std::uint16_t>(std::llround(value));
    builder->InitialVelocityFps(kSnapped);
    return true;
  }
  if (input == "bc_psi") {
    builder->BallisticCoefficientPsi(value);
    return true;
  }
  if (input == "zero_angle_moa") {
    builder->ZeroAngleMOA(value);
    return true;
  }
  if (input == "optic_height_in") {
    builder->OpticHeightInches(value);
    return true;
  }
  if (input == "pressure_inhg") {
    builder->AirPressureInHg(value);
    return true;
  }
  if (input == "temperature_degf") {
    builder->TemperatureDegF(value);
    return true;
  }
  if (input == "humidity_pp") {
    builder->RelativeHumidityPercent(value);
    return true;
  }
  if (input == "mass_grains") {
    builder->MassGrains(value);
    return true;
  }
  if (input == "diameter_in") {
    builder->DiameterInch(value);
    return true;
  }
  if (input == "length_in") {
    builder->LengthInch(value);
    return true;
  }
  if (input == "nose_length_in") {
    builder->NoseLengthInch(value);
    return true;
  }
  if (input == "tail_length_in") {
    builder->TailLengthInch(value);
    return true;
  }
  if (input == "meplat_diameter_in") {
    builder->MeplatDiameterInch(value);
    return true;
  }
  if (input == "base_diameter_in") {
    builder->BaseDiameterInch(value);
    return true;
  }
  if (input == "ogive_rtr") {
    builder->OgiveRtR(value);
    return true;
  }
  if (input == "twist_in_per_turn") {
    builder->TwistInchesPerTurn(value);
    return true;
  }
  if (input == "azimuth_deg") {
    builder->AzimuthDeg(value);
    return true;
  }
  if (input == "latitude_deg") {
    builder->LatitudeDeg(value);
    return true;
  }
  if (input == "range_angle_deg") {
    builder->RangeAngleDeg(value);
    return true;
  }
  if (input == "altitude_of_firing_site_ft") {
    builder->AltitudeOfFiringSiteFt(value);
    return true;
  }
  if (input == "altitude_of_barometer_ft") {
    builder->AltitudeOfBarometerFt(value);
    return true;
  }
  if (input == "altitude_of_thermometer_ft") {
    builder->AltitudeOfThermometerFt(value);
    return true;
  }
  if (input == "wind_heading_deg") {
    builder->WindHeadingDeg(value);
    return true;
  }
  if (input == "shear_exponent") {
    builder->WindShearExponent(value);
    return true;
  }
  if (input == "maximum_time_s") {
    builder->MaximumTime(value);
    return true;
  }
  return false;
}

inline bool KnownScalarInput(const std::string& input) {
  lob::Builder probe;
  return ApplyScalar(&probe, input, 0.0);
}

inline double WrapHeading(double degrees) {
  constexpr double kFullCircle = 360.0;
  double wrapped = std::fmod(degrees, kFullCircle);
  if (wrapped < 0.0) {
    wrapped += kFullCircle;
  }
  return wrapped;
}

// Wind at zero baseline: speed magnitude + heading drawn jointly, heading
// from a wrapped normal around baseline reduced mod 360, never negative
// speed (truncation resample; exhaust fails the sample, counted not aborted).
inline bool ApplyWindPair(std::mt19937_64& engine, const Dimension& dim,
                          lob::Builder* builder, std::vector<double>* draws) {
  constexpr double kPositiveLo = 0.0;
  const Truncation kSpeedTrunc =
      DrawWindow(dim, kPositiveLo, std::numeric_limits<double>::max());
  const Sample kSpeed =
      SampleNormal(engine, dim.speed_mean, dim.speed_sigma, kSpeedTrunc);
  const Sample kHeading =
      SampleNormal(engine, dim.heading_mean, dim.heading_sigma, WideWindow());
  if (!kSpeed.ok || !kHeading.ok || (kSpeed.value < 0.0)) {
    return false;
  }
  const double kWrapped = WrapHeading(kHeading.value);
  draws->push_back(kSpeed.value);
  draws->push_back(kWrapped);
  builder->WindSpeedMph(kSpeed.value);
  builder->WindHeadingDeg(kWrapped);
  return true;
}

// Profile stations: per-station speed/heading draws, independent across
// stations (the correlation flag records that choice). Heights are drawn
// only when the shear exponent drawn so far is nonzero — manifests list
// shear_exponent before wind_profile — otherwise baselines stand.
inline bool ApplyProfile(std::mt19937_64& engine, const Dimension& dim,
                         double shear_alpha, lob::Builder* builder,
                         std::vector<double>* draws,
                         std::vector<lob::WindPoint>* profile) {
  constexpr double kPositiveLo = 0.0;
  const Truncation kPositiveTrunc =
      DrawWindow(dim, kPositiveLo, std::numeric_limits<double>::max());
  profile->clear();
  for (const WindStation& station : dim.stations) {
    const Sample kSpeed = SampleNormal(engine, station.speed_mph,
                                       station.speed_sigma, kPositiveTrunc);
    const Sample kHeading = SampleNormal(engine, station.heading_deg,
                                         station.heading_sigma, WideWindow());
    if (!kSpeed.ok || !kHeading.ok || (kSpeed.value < 0.0)) {
      return false;
    }
    draws->push_back(kSpeed.value);
    draws->push_back(WrapHeading(kHeading.value));
    lob::WindPoint point{};
    point.range_ft = station.range_ft;
    point.heading_deg = WrapHeading(kHeading.value);
    point.speed_mph = kSpeed.value;
    if (std::fabs(shear_alpha) > 0.0) {
      const Sample kHeight = SampleNormal(engine, station.height_ft,
                                          station.height_sigma, kPositiveTrunc);
      if (!kHeight.ok) {
        return false;
      }
      draws->push_back(kHeight.value);
      point.height_ft = kHeight.value;
    } else {
      draws->push_back(station.height_ft);
      point.height_ft = station.height_ft;
    }
    profile->push_back(point);
  }
  builder->WindProfile(profile->data(), profile->size());
  return true;
}

inline bool ParseStation(const nlohmann::json& node, WindStation* out,
                         std::string* error) {
  if (!node.is_object()) {
    *error = "manifest: profile stations must be objects";
    return false;
  }
  if (!ReadDouble(node, "range_ft", &out->range_ft, error)) {
    return false;
  }
  if (!ReadDouble(node, "speed_mph", &out->speed_mph, error)) {
    return false;
  }
  if (!ReadDouble(node, "speed_sigma", &out->speed_sigma, error)) {
    return false;
  }
  if (!ReadDouble(node, "heading_deg", &out->heading_deg, error)) {
    return false;
  }
  if (!ReadDouble(node, "heading_sigma", &out->heading_sigma, error)) {
    return false;
  }
  if (!ReadDouble(node, "height_ft", &out->height_ft, error)) {
    return false;
  }
  if (!ReadDouble(node, "height_sigma", &out->height_sigma, error)) {
    return false;
  }
  if ((out->speed_sigma < 0.0) || (out->heading_sigma < 0.0) ||
      (out->height_sigma < 0.0)) {
    *error = "manifest: station sigmas must be non-negative";
    return false;
  }
  return true;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
inline bool ParseDimension(const nlohmann::json& node, Dimension* out,
                           std::string* error) {
  if (!node.is_object()) {
    *error = "manifest: dimensions must be objects";
    return false;
  }
  if (!node.contains("input") || !node.at("input").is_string()) {
    *error = "manifest: dimension needs a string 'input'";
    return false;
  }
  if (!node.contains("family") || !node.at("family").is_string()) {
    *error = "manifest: dimension needs a string 'family'";
    return false;
  }
  out->input = node.at("input").get<std::string>();
  out->family = node.at("family").get<std::string>();
  if (!node.contains("params") || !node.at("params").is_object()) {
    *error = "manifest: dimension needs an object 'params'";
    return false;
  }
  const nlohmann::json& params = node.at("params");
  if (out->family == kFamilyNormal) {
    if (!ReadDouble(params, "mean", &out->mean, error)) {
      return false;
    }
    if (!ReadDouble(params, "sigma", &out->sigma, error)) {
      return false;
    }
    if (!(out->sigma > 0.0)) {
      *error = "manifest: normal sigma must be positive";
      return false;
    }
    if (!KnownScalarInput(out->input)) {
      *error = "manifest: unknown scalar input '" + out->input +
               "' (wind_speed_mph needs the wind_pair family)";
      return false;
    }
  } else if (out->family == kFamilyUniform) {
    if (!ReadDouble(params, "lo", &out->uniform_lo, error)) {
      return false;
    }
    if (!ReadDouble(params, "hi", &out->uniform_hi, error)) {
      return false;
    }
    if (!(out->uniform_hi > out->uniform_lo)) {
      *error = "manifest: uniform hi must exceed lo";
      return false;
    }
    if (!KnownScalarInput(out->input)) {
      *error = "manifest: unknown scalar input '" + out->input + "'";
      return false;
    }
  } else if (out->family == kFamilyTriangular) {
    if (!ReadDouble(params, "lo", &out->tri_lo, error)) {
      return false;
    }
    if (!ReadDouble(params, "mode", &out->tri_mode, error)) {
      return false;
    }
    if (!ReadDouble(params, "hi", &out->tri_hi, error)) {
      return false;
    }
    if ((out->tri_lo > out->tri_mode) || (out->tri_mode > out->tri_hi) ||
        !(out->tri_hi > out->tri_lo)) {
      *error = "manifest: triangular needs lo <= mode <= hi with hi > lo";
      return false;
    }
    if (!KnownScalarInput(out->input)) {
      *error = "manifest: unknown scalar input '" + out->input + "'";
      return false;
    }
  } else if (out->family == kFamilyFixed) {
    if (!ReadDouble(params, "value", &out->fixed_value, error)) {
      return false;
    }
    if (!KnownScalarInput(out->input)) {
      *error = "manifest: unknown scalar input '" + out->input + "'";
      return false;
    }
  } else if (out->family == kFamilyWindPair) {
    if (out->input != "wind_speed_mph") {
      *error = "manifest: wind_pair input must be 'wind_speed_mph'";
      return false;
    }
    if (!ReadDouble(params, "speed_mean", &out->speed_mean, error)) {
      return false;
    }
    if (!ReadDouble(params, "speed_sigma", &out->speed_sigma, error)) {
      return false;
    }
    if (!ReadDouble(params, "heading_mean", &out->heading_mean, error)) {
      return false;
    }
    if (!ReadDouble(params, "heading_sigma", &out->heading_sigma, error)) {
      return false;
    }
    if (!(out->speed_sigma > 0.0) || !(out->heading_sigma > 0.0)) {
      *error = "manifest: wind_pair sigmas must be positive";
      return false;
    }
  } else if (out->family == kFamilyProfile) {
    if (out->input != "wind_profile") {
      *error = "manifest: profile input must be 'wind_profile'";
      return false;
    }
    if (!params.contains("stations") || !params.at("stations").is_array() ||
        params.at("stations").empty()) {
      *error = "manifest: profile needs a non-empty 'stations' array";
      return false;
    }
    if (params.at("stations").size() > lob::kLobWindPoints) {
      *error = "manifest: profile stations exceed LOB_WIND_POINTS";
      return false;
    }
    for (const nlohmann::json& entry : params.at("stations")) {
      WindStation station;
      if (!ParseStation(entry, &station, error)) {
        return false;
      }
      out->stations.push_back(station);
    }
    if (params.contains("correlation")) {
      if (!params.at("correlation").is_string()) {
        *error = "manifest: profile 'correlation' must be a string";
        return false;
      }
      out->correlation = params.at("correlation").get<std::string>();
    }
    if (out->correlation != kCorrelationIndependent) {
      *error = "manifest: wind_profile correlation '" + out->correlation +
               "' needs a covariance matrix input that doesn't exist; use \"" +
               std::string(kCorrelationIndependent) + "\"";
      return false;
    }
  } else {
    *error = "manifest: unknown family '" + out->family + "'";
    return false;
  }
  if (!ReadTruncation(node, out, error)) {
    return false;
  }
  if (node.contains("baseline")) {
    if (!node.at("baseline").is_number()) {
      *error = "manifest: dimension 'baseline' must be numeric";
      return false;
    }
    out->baseline = node.at("baseline").get<double>();
  }
  return true;
}

inline bool ParseScenario(const nlohmann::json& node, Scenario* out,
                          std::string* error) {
  if (!node.is_object()) {
    *error = "manifest: scenarios must be objects";
    return false;
  }
  if (!node.contains("name") || !node.at("name").is_string()) {
    *error = "manifest: scenario needs a string 'name'";
    return false;
  }
  if (!node.contains("drag_function") ||
      !node.at("drag_function").is_string()) {
    *error = "manifest: scenario needs a string 'drag_function'";
    return false;
  }
  if (!node.contains("weight") || !node.at("weight").is_number()) {
    *error = "manifest: scenario needs a numeric 'weight'";
    return false;
  }
  out->name = node.at("name").get<std::string>();
  out->drag = node.at("drag_function").get<std::string>();
  out->weight = node.at("weight").get<double>();
  lob::DragFunctionT drag = lob::DragFunctionT::kG1;
  if (!DragForName(out->drag, &drag)) {
    *error = "manifest: unknown drag_function '" + out->drag + "'";
    return false;
  }
  if (!std::isfinite(out->weight) || (out->weight < 0.0)) {
    *error = "manifest: scenario weight must be finite non-negative";
    return false;
  }
  return true;
}

// Profile heights draw only when the shear exponent drawn so far is
// nonzero, so a wind_profile dimension needs a shear_exponent dimension
// earlier in the same dimension list; otherwise the run would silently fall
// back to baseline heights with identical seeds and CSV shape.
inline bool CheckDimensionOrder(const std::vector<Dimension>& dimensions,
                                std::string* error) {
  bool seen_shear = false;
  for (const Dimension& dim : dimensions) {
    if (dim.input == kInputShearExponent) {
      seen_shear = true;
    } else if (dim.input == kInputWindProfile) {
      if (!seen_shear) {
        *error =
            "manifest: wind_profile requires shear_exponent earlier in "
            "dimensions";
        return false;
      }
    }
  }
  return true;
}

inline bool ParseCellSet(const nlohmann::json& node, CellSet* out,
                         std::string* error) {
  if (!node.is_object()) {
    *error = "manifest: dimension_sets entries must be objects";
    return false;
  }
  if (!node.contains("dimensions") || !node.at("dimensions").is_array()) {
    *error = "manifest: dimension set needs a 'dimensions' array";
    return false;
  }
  for (const nlohmann::json& entry : node.at("dimensions")) {
    Dimension dim;
    if (!ParseDimension(entry, &dim, error)) {
      return false;
    }
    out->dimensions.push_back(dim);
  }
  if (!CheckDimensionOrder(out->dimensions, error)) {
    return false;
  }
  if (node.contains("scenarios")) {
    if (!node.at("scenarios").is_array()) {
      *error = "manifest: dimension set 'scenarios' must be an array";
      return false;
    }
    for (const nlohmann::json& entry : node.at("scenarios")) {
      Scenario scenario;
      if (!ParseScenario(entry, &scenario, error)) {
        return false;
      }
      out->scenarios.push_back(scenario);
    }
    if (!out->scenarios.empty()) {
      double total = 0.0;
      for (const Scenario& scenario : out->scenarios) {
        total += scenario.weight;
      }
      if (!(total > 0.0)) {
        *error = "manifest: dimension set scenario weights must sum positive";
        return false;
      }
    }
  }
  return true;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
inline bool ParseManifest(const nlohmann::json& root, RunManifest* out,
                          std::string* error) {
  if (!root.is_object()) {
    *error = "manifest: root must be an object";
    return false;
  }
  if (!root.contains("run_id") || !root.at("run_id").is_string()) {
    *error = "manifest: needs a string 'run_id'";
    return false;
  }
  if (!root.contains("cell") || !root.at("cell").is_string()) {
    *error = "manifest: needs a string 'cell'";
    return false;
  }
  out->run_id = root.at("run_id").get<std::string>();
  out->cell = root.at("cell").get<std::string>();
  for (const char* key : {"seed", "samples", "workers"}) {
    if (!root.contains(key) || !root.at(key).is_number_unsigned()) {
      *error = std::string("manifest: needs an unsigned '") + key + "'";
      return false;
    }
  }
  out->seed = root.at("seed").get<std::uint64_t>();
  out->samples = root.at("samples").get<std::uint64_t>();
  out->workers = root.at("workers").get<std::uint64_t>();
  if ((out->samples == 0U) || (out->workers == 0U)) {
    *error = "manifest: 'samples' and 'workers' must be nonzero";
    return false;
  }
  if (!root.contains("solver_config") ||
      !root.at("solver_config").is_object()) {
    *error = "manifest: needs an object 'solver_config'";
    return false;
  }
  const nlohmann::json& solver = root.at("solver_config");
  if (!solver.contains("ranges_ft") || !solver.at("ranges_ft").is_array() ||
      solver.at("ranges_ft").empty()) {
    *error = "manifest: solver_config needs a non-empty 'ranges_ft' array";
    return false;
  }
  for (const nlohmann::json& entry : solver.at("ranges_ft")) {
    if (!entry.is_number_unsigned()) {
      *error = "manifest: ranges_ft entries must be unsigned";
      return false;
    }
    const std::uint64_t kRange = entry.get<std::uint64_t>();
    constexpr std::uint64_t kMaxRange = 12000U;
    if ((kRange == 0U) || (kRange > kMaxRange)) {
      *error = "manifest: ranges_ft entries must be in (0, 12000]";
      return false;
    }
    out->solver.ranges.push_back(static_cast<std::uint32_t>(kRange));
  }
  if (solver.contains("step_in")) {
    if (!solver.at("step_in").is_number_unsigned()) {
      *error = "manifest: solver_config 'step_in' must be unsigned";
      return false;
    }
    const std::uint64_t kStep = solver.at("step_in").get<std::uint64_t>();
    constexpr std::uint64_t kMaxStep = 120U;
    if ((kStep == 0U) || (kStep > kMaxStep)) {
      *error = "manifest: solver_config 'step_in' must be in (0, 120]";
      return false;
    }
    out->solver.step_in = static_cast<std::uint16_t>(kStep);
  }
  if (solver.contains("density_path")) {
    if (!solver.at("density_path").is_string()) {
      *error = "manifest: solver_config 'density_path' must be a string";
      return false;
    }
    out->solver.density_path = solver.at("density_path").get<std::string>();
    // Opaque label, closed set: fail loudly on typos, never free-text.
    if ((out->solver.density_path != kDensityFast) &&
        (out->solver.density_path != kDensityLapseScaledInverseTail)) {
      *error =
          "manifest: solver_config 'density_path' must be 'fast' or "
          "'lapse-scaled-inverse-tail', got '" +
          out->solver.density_path + "'";
      return false;
    }
  }
  if (!root.contains("dimensions") || !root.at("dimensions").is_array()) {
    *error = "manifest: needs a 'dimensions' array";
    return false;
  }
  for (const nlohmann::json& entry : root.at("dimensions")) {
    Dimension dim;
    if (!ParseDimension(entry, &dim, error)) {
      return false;
    }
    out->dimensions.push_back(dim);
  }
  if (!CheckDimensionOrder(out->dimensions, error)) {
    return false;
  }
  if (root.contains("scenarios")) {
    if (!root.at("scenarios").is_array()) {
      *error = "manifest: 'scenarios' must be an array";
      return false;
    }
    for (const nlohmann::json& entry : root.at("scenarios")) {
      Scenario scenario;
      if (!ParseScenario(entry, &scenario, error)) {
        return false;
      }
      out->scenarios.push_back(scenario);
    }
    if (!out->scenarios.empty()) {
      double total = 0.0;
      for (const Scenario& scenario : out->scenarios) {
        total += scenario.weight;
      }
      if (!(total > 0.0)) {
        *error = "manifest: scenario weights must sum positive";
        return false;
      }
    }
  }
  if (root.contains("dimension_sets")) {
    if (!root.at("dimension_sets").is_object()) {
      *error = "manifest: 'dimension_sets' must be an object";
      return false;
    }
    for (auto it = root.at("dimension_sets").begin();
         it != root.at("dimension_sets").end(); ++it) {
      CellSet set;
      if (!ParseCellSet(it.value(), &set, error)) {
        return false;
      }
      out->dimension_sets[it.key()] = set;
    }
  }
  if (root.contains("synthetic_illustrative_only") &&
      root.at("synthetic_illustrative_only").is_boolean()) {
    out->synthetic_illustrative_only =
        root.at("synthetic_illustrative_only").get<bool>();
  }
  return true;
}

// Canonical per-cell base builders: the exact Phase 2 survey points
// (C1-ICAO from testing.hpp, C5-uniform/C8-Litz from the Pareto driver).
inline bool BaseBuilderFor(const std::string& cell, lob::Builder* out,
                           std::string* error) {
  if (cell == "C1-ICAO") {
    constexpr double kBcPsi = 0.232;
    constexpr double kDiameterInch = 0.308;
    constexpr double kMassGrains = 155.0;
    constexpr std::uint16_t kVelocityFps = 2800U;
    constexpr double kZeroAngleMoa = 3.66;
    constexpr double kOpticHeightInches = 1.5;
    out->BallisticCoefficientPsi(kBcPsi)
        .BCDragFunction(lob::DragFunctionT::kG7)
        .BCAtmosphere(lob::AtmosphereReferenceT::kIcao)
        .DiameterInch(kDiameterInch)
        .MassGrains(kMassGrains)
        .InitialVelocityFps(kVelocityFps)
        .ZeroAngleMOA(kZeroAngleMoa)
        .OpticHeightInches(kOpticHeightInches);
    return true;
  }
  if (cell == "C5-uniform") {
    constexpr double kBcPsi = 0.372;
    constexpr double kDiameterInch = 0.224;
    constexpr double kMassGrains = 77.0;
    constexpr std::uint16_t kVelocityFps = 2720U;
    constexpr double kZeroAngleMoa = 4.78;
    constexpr double kOpticHeightIn = 2.5;
    out->BallisticCoefficientPsi(kBcPsi)
        .BCDragFunction(lob::DragFunctionT::kG1)
        .DiameterInch(kDiameterInch)
        .MassGrains(kMassGrains)
        .InitialVelocityFps(kVelocityFps)
        .ZeroAngleMOA(kZeroAngleMoa)
        .OpticHeightInches(kOpticHeightIn);
    return true;
  }
  if (cell == "C8-Litz") {
    constexpr double kBcPsi = 0.436;
    constexpr std::uint16_t kVelocityFps = 3100U;
    constexpr double kZeroAngleMoa = 6.11;
    constexpr double kDiameterInch = 0.308;
    constexpr double kLengthInch = 1.215;
    constexpr double kMassGrains = 168.0;
    constexpr double kTwistIn = 10.0;
    constexpr double kWindSpeedMph = 10.0;
    out->BallisticCoefficientPsi(kBcPsi)
        .InitialVelocityFps(kVelocityFps)
        .ZeroAngleMOA(kZeroAngleMoa)
        .DiameterInch(kDiameterInch)
        .LengthInch(kLengthInch)
        .MassGrains(kMassGrains)
        .TwistInchesPerTurn(kTwistIn)
        .WindHeading(lob::ClockAngleT::kIII)
        .WindSpeedMph(kWindSpeedMph);
    return true;
  }
  *error = "manifest: unknown cell '" + cell + "'";
  return false;
}

inline bool SelectCell(const RunManifest& manifest, const std::string& cell,
                       const std::vector<Dimension>** dimensions,
                       const std::vector<Scenario>** scenarios,
                       std::string* error) {
  if ((dimensions == nullptr) || (scenarios == nullptr)) {
    *error = "internal: null out-parameters";
    return false;
  }
  if (cell == manifest.cell) {
    *dimensions = &manifest.dimensions;
    *scenarios = &manifest.scenarios;
    return true;
  }
  const auto kFound = manifest.dimension_sets.find(cell);
  if (kFound == manifest.dimension_sets.end()) {
    *error = "manifest: no dimension set for cell '" + cell + "'";
    return false;
  }
  *dimensions = &kFound->second.dimensions;
  *scenarios = &kFound->second.scenarios;
  return true;
}

// Fixed draw-column order for samples.csv: scenario index first, then one
// column per scalar draw, two per wind_pair (speed, heading), and per-station
// speed/heading/height triples for profiles. Heights always reserve columns;
// at alpha 0 the baseline value is recorded (never drawn).
inline std::vector<std::string> DrawColumnNames(
    const std::vector<Scenario>& scenarios,
    const std::vector<Dimension>& dimensions) {
  std::vector<std::string> names;
  if (!scenarios.empty()) {
    names.emplace_back("draw_scenario_index");
  }
  for (const Dimension& dim : dimensions) {
    if (dim.family == kFamilyWindPair) {
      names.emplace_back("draw_wind_speed_mph");
      names.emplace_back("draw_wind_heading_deg");
      continue;
    }
    if (dim.family == kFamilyProfile) {
      for (std::size_t idx = 0U; idx < dim.stations.size(); ++idx) {
        names.emplace_back("draw_station" + std::to_string(idx) + "_speed_mph");
        names.emplace_back("draw_station" + std::to_string(idx) +
                           "_heading_deg");
        names.emplace_back("draw_station" + std::to_string(idx) + "_height_ft");
      }
      continue;
    }
    names.emplace_back("draw_" + dim.input);
  }
  return names;
}

inline void PadDraws(TrajectorySample* sample, std::size_t width) {
  const double kMissing = std::numeric_limits<double>::quiet_NaN();
  while (sample->draws.size() < width) {
    sample->draws.push_back(kMissing);
  }
}

// Branch counters from one built context: forward reach, analytic
// miller_unstable (Sg < 1, not solver-reported), and the SolveAngle-NaN angle
// cap, which surfaces publicly as an inverse prefix-short (SolveInverse stops
// at the first uncapped range, so inverse < forward is the cap branch).
inline void SolveBranches(const lob::Context& ctx,
                          const std::vector<std::uint32_t>& ranges,
                          TrajectorySample* sample) {
  constexpr double kTumbleSg = 1.0;
  sample->stability = ctx.stability_factor;
  sample->forward.assign(ranges.size(), lob::Output{});
  sample->inverse.assign(ranges.size(), lob::Output{});
  sample->forward_count =
      lob::Solve(ctx, ranges.data(), sample->forward.data(), ranges.size());
  sample->inverse_count = lob::SolveInverse(
      ctx, ranges.data(), sample->inverse.data(), ranges.size());
  sample->flags.reached_all = (sample->forward_count == ranges.size());
  sample->flags.fall_short_index = sample->forward_count;
  // Miller-stability flag: 0.0 means "not computed" (the solver's own
  // fabs > 0 activity test), so only a computed Sg below 1.0 flags.
  const double kSg = ctx.stability_factor;
  sample->flags.miller_unstable =
      (std::fabs(kSg) > 0.0) && (std::fabs(kSg) < kTumbleSg);
  sample->flags.angle_cap_hit = (sample->inverse_count < sample->forward_count);
}

// Mutable per-sample draw state threaded through the dimension loop.
struct DrawState {
  double shear_alpha = 0.0;
  std::vector<lob::WindPoint> profile;
};

// Categorical G-curve choice as a weighted scenario per manifest weights;
// degenerate weight 1.0 on the case's own curve reproduces the base builder.
inline bool ApplyScenario(std::mt19937_64& engine,
                          const std::vector<Scenario>& scenarios,
                          lob::Builder* builder, std::vector<double>* draws) {
  std::vector<double> weights;
  weights.reserve(scenarios.size());
  for (const Scenario& scenario : scenarios) {
    weights.push_back(scenario.weight);
  }
  const std::size_t kChoice = SampleCategorical(engine, weights);
  draws->push_back(static_cast<double>(kChoice));
  lob::DragFunctionT drag = lob::DragFunctionT::kG1;
  if ((kChoice >= scenarios.size()) ||
      !DragForName(scenarios.at(kChoice).drag, &drag)) {
    return false;
  }
  builder->BCDragFunction(drag);
  return true;
}

// One manifest dimension: draw, record, and apply onto the builder clone.
// Returns false when the sample must be counted build_failed (OOR draw,
// truncation exhaust, or rejected value) — the caller pads draws and stops.
inline bool ApplyDimension(std::mt19937_64& engine, const Dimension& dim,
                           lob::Builder* builder, std::vector<double>* draws,
                           DrawState* state) {
  constexpr double kShearLo = 0.0;
  constexpr double kShearHi = 1.0;
  if (dim.family == kFamilyWindPair) {
    return ApplyWindPair(engine, dim, builder, draws);
  }
  if (dim.family == kFamilyProfile) {
    return ApplyProfile(engine, dim, state->shear_alpha, builder, draws,
                        &state->profile);
  }
  double value = 0.0;
  if (!DrawScalar(engine, dim, &value)) {
    return false;
  }
  // BC is normal around the band value, truncated positive; shear lives
  // on [0, 1] truncated (plan §12.1 rules when the manifest is silent).
  if ((dim.input == "bc_psi") && (value < 0.0)) {
    return false;
  }
  if ((dim.input == "shear_exponent") &&
      ((value < kShearLo) || (value > kShearHi))) {
    return false;
  }
  draws->push_back(value);
  if (!ApplyScalar(builder, dim.input, value)) {
    return false;
  }
  if (dim.input == "shear_exponent") {
    state->shear_alpha = value;
  }
  return true;
}

// One sample = one Build + Solve(+Inverse) through the public C++ API.
// Draw order is manifest order; the engine is the per-stream
// Subseed(seed, index) engine, so the sequence never depends on workers.
// OOR draws and build errors mark the sample build_failed (counted downstream,
// never aborting the run).
inline TrajectorySample RunSample(const RunPlan& plan, std::size_t index) {
  TrajectorySample sample;
  sample.index = index;
  sample.flags.configured_density_path = *plan.configured_density_path;
  std::mt19937_64 engine(Subseed(plan.seed, static_cast<std::uint64_t>(index)));
  lob::Builder builder = *plan.base;
  builder.StepSize(plan.step_in);
  if (!plan.scenarios->empty() &&
      !ApplyScenario(engine, *plan.scenarios, &builder, &sample.draws)) {
    sample.flags.build_failed = true;
    PadDraws(&sample, plan.draw_width);
    return sample;
  }
  DrawState state;
  for (const Dimension& dim : *plan.dimensions) {
    if (!ApplyDimension(engine, dim, &builder, &sample.draws, &state)) {
      sample.flags.build_failed = true;
      break;
    }
  }
  PadDraws(&sample, plan.draw_width);
  if (sample.flags.build_failed) {
    return sample;
  }
  // WindProfile borrows the station array: Build() runs while the profile
  // vector is still alive, so per-sample profile draws stay valid.
  const lob::Context kCtx = builder.Build();
  if (kCtx.error != lob::ErrorT::kNone) {
    sample.flags.build_failed = true;
    return sample;
  }
  SolveBranches(kCtx, *plan.ranges, &sample);
  return sample;
}

}  // namespace mc

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
