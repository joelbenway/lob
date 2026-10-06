// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#pragma once

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <fstream>
#include <limits>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "eng_units.hpp"
#include "helpers.hpp"
#include "lob/lob.hpp"

namespace tests {

struct SolutionTolerances {
  lob::FpsT velocity = lob::FpsT(lob::NaN());
  lob::FtLbsT energy = lob::FtLbsT(lob::NaN());
  lob::MoaT moa = lob::MoaT(lob::NaN());
  lob::InchT inch = lob::InchT(lob::NaN());
  lob::SecT time_of_flight = lob::SecT(lob::NaN());
  constexpr SolutionTolerances() = default;
  constexpr SolutionTolerances(lob::FpsT v, lob::FtLbsT e, lob::MoaT m,
                               lob::InchT i, lob::SecT t)
      : velocity(std::move(v)),
        energy(std::move(e)),
        moa(std::move(m)),
        inch(std::move(i)),
        time_of_flight(std::move(t)) {}
};

class OutputNearMatcher
    : public testing::MatcherInterface<
          std::tuple<const lob::Output&, const lob::Output&>> {
 public:
  explicit OutputNearMatcher(SolutionTolerances tolerances)
      : tolerances_(std::move(tolerances)) {}

  bool MatchAndExplain(std::tuple<const lob::Output&, const lob::Output&> arg,
                       testing::MatchResultListener* listener) const override {
    const auto& actual = std::get<0>(arg);
    const auto& expected = std::get<1>(arg);

    if (actual.range != expected.range) {
      *listener << "range " << actual.range << " vs " << expected.range;
      return false;
    }
    if (std::abs(static_cast<double>(actual.velocity) -
                 static_cast<double>(expected.velocity)) >
        tolerances_.velocity.Value()) {
      *listener << "velocity " << actual.velocity << " vs "
                << expected.velocity;
      return false;
    }
    if (!tolerances_.energy.IsNaN() &&
        std::abs(static_cast<double>(actual.energy) -
                 static_cast<double>(expected.energy)) >
            tolerances_.energy.Value()) {
      *listener << "energy " << actual.energy << " vs " << expected.energy;
      return false;
    }
    const double kActualElevationMoa =
        lob::InchToMoa(actual.elevation, actual.range);
    const double kExpectedElevationMoa =
        lob::InchToMoa(expected.elevation, expected.range);
    if (std::abs(kActualElevationMoa - kExpectedElevationMoa) >
        tolerances_.moa.Value()) {
      *listener << "elevation MOA " << kActualElevationMoa << " vs "
                << kExpectedElevationMoa;
      return false;
    }
    if (!tolerances_.inch.IsNaN() &&
        std::abs(actual.elevation - expected.elevation) >
            tolerances_.inch.Value()) {
      *listener << "elevation inch " << actual.elevation << " vs "
                << expected.elevation;
      return false;
    }
    const double kActualDeflectionMoa =
        lob::InchToMoa(actual.deflection, actual.range);
    const double kExpectedDeflectionMoa =
        lob::InchToMoa(expected.deflection, expected.range);
    if (std::abs(kActualDeflectionMoa - kExpectedDeflectionMoa) >
        tolerances_.moa.Value()) {
      *listener << "deflection MOA " << kActualDeflectionMoa << " vs "
                << kExpectedDeflectionMoa;
      return false;
    }
    if (!tolerances_.inch.IsNaN() &&
        std::abs(actual.deflection - expected.deflection) >
            tolerances_.inch.Value()) {
      *listener << "deflection inch " << actual.deflection << " vs "
                << expected.deflection;
      return false;
    }
    if (std::abs(actual.time_of_flight - expected.time_of_flight) >
        tolerances_.time_of_flight.Value()) {
      *listener << "time_of_flight " << actual.time_of_flight << " vs "
                << expected.time_of_flight;
      return false;
    }
    return true;
  }

  void DescribeTo(std::ostream* os) const override {
    *os << "output is within tolerance";
  }

 private:
  SolutionTolerances tolerances_;
};

inline testing::Matcher<std::tuple<const lob::Output&, const lob::Output&>>
OutputNear(SolutionTolerances tolerances) {
  return testing::MakeMatcher(new OutputNearMatcher(tolerances));
}

template <size_t N>
void VerifySolutions(const std::array<lob::Output, N>& solutions,
                     const std::vector<lob::Output>& expected,
                     const SolutionTolerances& tolerances = {}) {
  ASSERT_EQ(solutions.size(), expected.size());
  EXPECT_THAT(solutions, testing::Pointwise(OutputNear(tolerances), expected));
}

template <size_t N>
void VerifySolutionDifferences(
    const std::array<lob::Output, N>& solutions1,
    const std::array<lob::Output, N>& solutions2,
    const std::array<double, N>& expected_elevation_diff,
    const std::array<double, N>& expected_deflection_diff, double tolerance) {
  for (size_t i = 0; i < N; i++) {
    const auto kElevationDifference =
        solutions2.at(i).elevation - solutions1.at(i).elevation;
    const auto kDeflectionDifference =
        solutions2.at(i).deflection - solutions1.at(i).deflection;
    EXPECT_NEAR(kElevationDifference, expected_elevation_diff.at(i), tolerance);
    EXPECT_NEAR(kDeflectionDifference, expected_deflection_diff.at(i),
                tolerance);
  }
}

inline void SetupTestBuilder(lob::Builder& b) {
  constexpr double kTestBC = 0.425;
  constexpr double kTestDiameter = 0.308;
  constexpr double kTestWeight = 180.0;
  constexpr uint16_t kTestMuzzleVelocity = 2700U;
  constexpr double kTestZeroAngle = 3.38;
  b.BallisticCoefficientPsi(kTestBC)
      .DiameterInch(kTestDiameter)
      .MassGrains(kTestWeight)
      .InitialVelocityFps(kTestMuzzleVelocity)
      .ZeroAngleMOA(kTestZeroAngle);
}

// ---- Validation convergence helpers (Phase 1) ----
inline lob::Builder MakeC1IcaoBuilder() {
  constexpr double kC1BcPsi = 0.232;
  constexpr double kC1DiameterInch = 0.308;
  constexpr double kC1MassGrains = 155.0;
  constexpr uint16_t kC1MuzzleVelocityFps = 2800U;
  constexpr double kC1ZeroAngleMoa = 3.66;
  constexpr double kC1OpticHeightInches = 1.5;
  lob::Builder b;
  b.BallisticCoefficientPsi(kC1BcPsi)
      .BCDragFunction(lob::DragFunctionT::kG7)
      .BCAtmosphere(lob::AtmosphereReferenceT::kIcao)
      .DiameterInch(kC1DiameterInch)
      .MassGrains(kC1MassGrains)
      .InitialVelocityFps(kC1MuzzleVelocityFps)
      .ZeroAngleMOA(kC1ZeroAngleMoa)
      .OpticHeightInches(kC1OpticHeightInches);
  return b;
}

inline lob::Context BuildAtStep(lob::Builder builder, uint16_t step_in) {
  builder.StepSize(step_in);
  return builder.Build();
}

template <size_t N>
size_t SolveN(const lob::Context& ctx, const std::array<uint32_t, N>& ranges,
              std::array<lob::Output, N>* pouts) {
  return lob::Solve(ctx, ranges, pouts);
}

// ---- Validation convergence math (Phase 1) ----
constexpr double kElevFloorIn = 0.01;      // spec §8.2 floor_y
constexpr double kMoaFloor = 0.01;         // angle-tolerance granularity
constexpr double kVelFloorFps = 1.0;       // 1 LSB of U16 truncation
constexpr double kEnergyFloorFtLbs = 1.0;  // 1 LSB of U32 truncation
constexpr double kTofFloorSec = 1e-9;      // double channel, epsilon only

inline double ElevInDiff(const lob::Output& a, const lob::Output& b) {
  return std::fabs(a.elevation - b.elevation);
}

inline double ElevMoaDiff(const lob::Output& a, const lob::Output& b) {
  return std::fabs(lob::InchToMoa(a.elevation, static_cast<double>(a.range)) -
                   lob::InchToMoa(b.elevation, static_cast<double>(b.range)));
}

inline double DeflMoaDiff(const lob::Output& a, const lob::Output& b) {
  return std::fabs(lob::InchToMoa(a.deflection, static_cast<double>(a.range)) -
                   lob::InchToMoa(b.deflection, static_cast<double>(b.range)));
}

inline double VelDiff(const lob::Output& a, const lob::Output& b) {
  return std::fabs(static_cast<double>(a.velocity) -
                   static_cast<double>(b.velocity));
}

inline double EnergyDiff(const lob::Output& a, const lob::Output& b) {
  return std::fabs(static_cast<double>(a.energy) -
                   static_cast<double>(b.energy));
}

inline double TofDiff(const lob::Output& a, const lob::Output& b) {
  return std::fabs(a.time_of_flight - b.time_of_flight);
}

inline bool IsAtFloor(double delta, double floor) { return delta <= floor; }

// Passes when the finer rung does not regress: strictly decreases, or both
// rungs sit at/below the reporting floor (quantization chatter allowance).
inline bool DecreasesOrAtFloor(double coarse_delta, double fine_delta,
                               double floor) {
  if (IsAtFloor(coarse_delta, floor) && IsAtFloor(fine_delta, floor)) {
    return true;
  }
  return fine_delta < coarse_delta;
}

// Observed order p ≈ log2(|Δh| / |Δh/2|); NaN when the finer delta is zero.
inline double ObservedOrder(double coarse_delta, double fine_delta) {
  if (!(fine_delta > 0.0) || !(coarse_delta >= 0.0)) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  if (!(coarse_delta > 0.0)) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  return std::log2(coarse_delta / fine_delta);
}

// ---- Validation envelope recomputation (Phases 3-4 shared) ----
// Reference-case builders live here so ReferenceMatrix.FullMatrix and
// BudgetAssemble.OfflineDocuments run the same 6 solves from one definition.
// Keep in sync with test/source/lob_env_test.cpp and
// test/validation/cases/reference_<stem>.json.
inline lob::Builder BuildAltitude4500Case() {
  constexpr double kSiteAltitudeFt = 4500.0;
  constexpr double kTemperatureF = 59.0;
  lob::Builder b = MakeC1IcaoBuilder();
  b.AltitudeOfFiringSiteFt(kSiteAltitudeFt).TemperatureDegF(kTemperatureF);
  return b;
}

inline lob::Builder BuildHotLowPCase() {
  constexpr double kTemperatureF = 100.0;
  constexpr double kPressureInHg = 25.0;
  lob::Builder b = MakeC1IcaoBuilder();
  b.TemperatureDegF(kTemperatureF).AirPressureInHg(kPressureInHg);
  return b;
}

inline lob::Builder BuildBarometerCase() {
  constexpr double kSiteAltitudeFt = 5280.0;
  constexpr double kPressureInHg = 30.0;
  constexpr double kTemperatureF = 59.0;
  lob::Builder b = MakeC1IcaoBuilder();
  b.AltitudeOfFiringSiteFt(kSiteAltitudeFt)
      .AirPressureInHg(kPressureInHg)
      .AltitudeOfBarometerFt(0)
      .TemperatureDegF(kTemperatureF);
  return b;
}

inline lob::Builder BuildHumidCase() {
  constexpr double kPressureInHg = 29.0;
  constexpr double kTemperatureF = 75.0;
  constexpr double kHumidityPct = 80.0;
  lob::Builder b = MakeC1IcaoBuilder();
  b.AirPressureInHg(kPressureInHg)
      .TemperatureDegF(kTemperatureF)
      .RelativeHumidityPercent(kHumidityPct);
  return b;
}

inline lob::Builder BuildWeatherStationCase() {
  constexpr double kSiteAltitudeFt = 5280.0;
  constexpr double kPressureInHg = 30.0;
  constexpr double kTemperatureF = 65.0;
  constexpr double kThermoAltitudeFt = 3598.0;
  lob::Builder b = MakeC1IcaoBuilder();
  b.AltitudeOfFiringSiteFt(kSiteAltitudeFt)
      .AirPressureInHg(kPressureInHg)
      .AltitudeOfBarometerFt(0)
      .TemperatureDegF(kTemperatureF)
      .AltitudeOfThermometerFt(kThermoAltitudeFt);
  return b;
}

struct EnvelopeWorst {
  double elev_in = 0.0;
  double elev_moa = 0.0;
  double defl_moa = 0.0;
  double vel = 0.0;
  double energy = 0.0;
  double tof = 0.0;
};

namespace envelope_detail {

inline void AccumulateWorst(const lob::Output& solved, const lob::Output& ref,
                            EnvelopeWorst* worst) {
  const double kRange = static_cast<double>(solved.range);
  const double kRElevIn = std::fabs(solved.elevation - ref.elevation);
  const double kRElevMoa = std::fabs(lob::InchToMoa(solved.elevation, kRange) -
                                     lob::InchToMoa(ref.elevation, kRange));
  const double kRDeflMoa = std::fabs(lob::InchToMoa(solved.deflection, kRange) -
                                     lob::InchToMoa(ref.deflection, kRange));
  const double kRVel = std::fabs(static_cast<double>(solved.velocity) -
                                 static_cast<double>(ref.velocity));
  const double kREnergy = std::fabs(static_cast<double>(solved.energy) -
                                    static_cast<double>(ref.energy));
  const double kRTof = std::fabs(solved.time_of_flight - ref.time_of_flight);
  worst->elev_in = std::max(worst->elev_in, kRElevIn);
  worst->elev_moa = std::max(worst->elev_moa, kRElevMoa);
  worst->defl_moa = std::max(worst->defl_moa, kRDeflMoa);
  worst->vel = std::max(worst->vel, kRVel);
  worst->energy = std::max(worst->energy, kREnergy);
  worst->tof = std::max(worst->tof, kRTof);
}

inline bool WorstForCase(const std::string& cases_dir, const char* stem,
                         lob::Builder (*build)(), EnvelopeWorst* worst,
                         std::string* error) {
  constexpr std::size_t kNumRanges = 12;
  const std::string kPath = cases_dir + "/reference_" + stem + ".json";
  std::ifstream in(kPath.c_str());
  if (!in) {
    *error = "reference case file not found: " + kPath;
    return false;
  }
  std::ostringstream raw;
  raw << in.rdbuf();
  try {
    const nlohmann::json kRoot = nlohmann::json::parse(raw.str());
    const nlohmann::json& kRangesJson = kRoot.at("ranges_ft");
    const nlohmann::json& kRows = kRoot.at("expected");
    if (!kRangesJson.is_array() || !kRows.is_array() ||
        kRangesJson.size() != kNumRanges || kRows.size() != kNumRanges) {
      *error = "bad range count in " + kPath;
      return false;
    }
    std::array<uint32_t, kNumRanges> ranges = {};
    for (std::size_t i = 0; i < kNumRanges; ++i) {
      ranges.at(i) = static_cast<uint32_t>(kRangesJson.at(i).get<double>());
    }
    std::array<lob::Output, kNumRanges> outs = {};
    if (SolveN(BuildAtStep(build(), 36U), ranges, &outs) != kNumRanges) {
      *error = "solve failed for " + kPath;
      return false;
    }
    for (std::size_t i = 0; i < kNumRanges; ++i) {
      const nlohmann::json& kRow = kRows.at(i);
      lob::Output ref{};
      ref.range = ranges.at(i);
      ref.velocity =
          static_cast<uint16_t>(kRow.at("velocity_fps").get<double>());
      ref.energy =
          static_cast<uint32_t>(kRow.at("energy_ft_lbf").get<double>());
      ref.elevation = kRow.at("elevation_in").get<double>();
      ref.deflection = kRow.at("deflection_in").get<double>();
      ref.time_of_flight = kRow.at("time_of_flight_s").get<double>();
      AccumulateWorst(outs.at(i), ref, worst);
    }
  } catch (const std::exception& e) {
    *error = "envelope recompute failed for " + kPath + ": " + e.what();
    return false;
  }
  return true;
}

}  // namespace envelope_detail

// Recomputes the envelope worst-residuals in-process (6 solves at the 36-in
// rung vs the checked-in reference cases). Same inputs give same values as
// the envelope_report.json artifact; the budget test uses this when that
// file is missing or stale instead of reading a file another test writes.
inline bool TryComputeEnvelopeWorst(const std::string& cases_dir,
                                    EnvelopeWorst* out, std::string* error) {
  *out = EnvelopeWorst();
  struct CaseEntry {
    const char* stem;
    lob::Builder (*build)();
  };
  const std::array<CaseEntry, 6> kCases = {
      {{"icao", MakeC1IcaoBuilder},
       {"altitude4500", BuildAltitude4500Case},
       {"hot_lowp", BuildHotLowPCase},
       {"barometer", BuildBarometerCase},
       {"humidity", BuildHumidCase},
       {"weather_station", BuildWeatherStationCase}}};
  for (const CaseEntry& entry : kCases) {
    if (!envelope_detail::WorstForCase(cases_dir, entry.stem, entry.build, out,
                                       error)) {
      return false;
    }
  }
  return true;
}

}  // namespace tests

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
