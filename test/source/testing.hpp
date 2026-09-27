// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#pragma once

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
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
  lob::Builder b;
  b.BallisticCoefficientPsi(0.232)
      .BCDragFunction(lob::DragFunctionT::kG7)
      .BCAtmosphere(lob::AtmosphereReferenceT::kIcao)
      .DiameterInch(0.308)
      .MassGrains(155.0)
      .InitialVelocityFps(2800)
      .ZeroAngleMOA(3.66)
      .OpticHeightInches(1.5);
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

}  // namespace tests
