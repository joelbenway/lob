// Copyright (c) 2026  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <random>
#include <vector>

#include "lob/lob.hpp"
#include "testing.hpp"

namespace {

// 0xC10CA1 is a hex pun on the C1 cell. The sampler below is self-contained
// (mt19937_64 + Splitmix64 subseeds, NO tools/ headers — the no-coupling
// rule): it proves the seeded-sampling method, while the tool proves scale.
// Both pin the same RNG + subseed constants; the Task 3 review diffs them.
constexpr std::uint64_t kSeed = 0xC10CA1ULL;
constexpr std::size_t kSamples = 256U;
constexpr double kVelocityMeanFps = 2800.0;
constexpr double kVelocitySigmaFps = 10.0;
constexpr double kWindSigmaMph = 2.0;
constexpr double kWindHeadingDeg = 90.0;
constexpr std::uint16_t kStepIn = 36U;
constexpr double kThreeSigma = 3.0;
constexpr double kMinSpearman = 0.9;
constexpr double kP05 = 0.05;
constexpr double kP50 = 0.5;
constexpr double kP95 = 0.95;

inline std::uint64_t Splitmix64(std::uint64_t state) {
  constexpr std::uint64_t kIncrement = 0x9E3779B97F4A7C15ULL;
  constexpr std::uint64_t kMultiplierA = 0xBF58476D1CE4E5B9ULL;
  constexpr std::uint64_t kMultiplierB = 0x94D049BB133111EBULL;
  constexpr std::uint64_t kShiftA = 30U;
  constexpr std::uint64_t kShiftB = 27U;
  constexpr std::uint64_t kShiftC = 31U;
  const std::uint64_t kMixed = state + kIncrement;
  const std::uint64_t kZedA = (kMixed ^ (kMixed >> kShiftA)) * kMultiplierA;
  const std::uint64_t kZedB = (kZedA ^ (kZedA >> kShiftB)) * kMultiplierB;
  return kZedB ^ (kZedB >> kShiftC);
}

inline std::uint64_t Subseed(std::uint64_t seed, std::uint64_t stream) {
  constexpr std::uint64_t kWorkerOffset = 1U;
  return Splitmix64(seed ^ (stream + kWorkerOffset));
}

inline double MeanOf(const std::vector<double>& values) {
  double sum = 0.0;
  for (const double kValue : values) {
    sum += kValue;
  }
  return sum / static_cast<double>(values.size());
}

inline double SdOf(const std::vector<double>& values, double mean) {
  double sum_sq = 0.0;
  for (const double kValue : values) {
    const double kDev = kValue - mean;
    sum_sq += kDev * kDev;
  }
  return std::sqrt(sum_sq / static_cast<double>(values.size() - 1U));
}

// Nearest-rank percentile via nth_element (deterministic for fixed input).
inline double PercentileOf(const std::vector<double>& values, double fraction) {
  const std::size_t kCount = values.size();
  const auto kRank = static_cast<std::size_t>(
      std::ceil(fraction * static_cast<double>(kCount)));
  const std::size_t kClamped = (kRank > kCount) ? kCount : kRank;
  const std::size_t kIndex = (kClamped > 0U) ? (kClamped - 1U) : 0U;
  std::vector<double> scratch = values;
  std::nth_element(scratch.begin(),
                   scratch.begin() + static_cast<std::ptrdiff_t>(kIndex),
                   scratch.end());
  return scratch.at(kIndex);
}

inline std::vector<double> RanksOf(const std::vector<double>& values) {
  const std::size_t kCount = values.size();
  std::vector<std::size_t> order(kCount, 0U);
  for (std::size_t i = 0U; i < kCount; ++i) {
    order[i] = i;
  }
  std::sort(order.begin(), order.end(),
            [&values](std::size_t left, std::size_t right) {
              return values.at(left) < values.at(right);
            });
  std::vector<double> ranks(kCount, 0.0);
  std::size_t pos = 0U;
  while (pos < kCount) {
    std::size_t end = pos + 1U;
    while ((end < kCount) &&
           !(values.at(order.at(end)) < values.at(order.at(pos))) &&
           !(values.at(order.at(pos)) < values.at(order.at(end)))) {
      ++end;
    }
    const double kAverage =
        (static_cast<double>(pos + 1U) + static_cast<double>(end)) * 0.5;
    for (std::size_t k = pos; k < end; ++k) {
      ranks[order[k]] = kAverage;
    }
    pos = end;
  }
  return ranks;
}

inline double SpearmanOf(const std::vector<double>& xs,
                         const std::vector<double>& ys) {
  const std::vector<double> kRankX = RanksOf(xs);
  const std::vector<double> kRankY = RanksOf(ys);
  const double kMeanX = MeanOf(kRankX);
  const double kMeanY = MeanOf(kRankY);
  double cov = 0.0;
  double var_x = 0.0;
  double var_y = 0.0;
  for (std::size_t i = 0U; i < xs.size(); ++i) {
    const double kDx = kRankX.at(i) - kMeanX;
    const double kDy = kRankY.at(i) - kMeanY;
    cov += kDx * kDy;
    var_x += kDx * kDx;
    var_y += kDy * kDy;
  }
  return cov / std::sqrt(var_x * var_y);
}

struct SmokeSamples {
  std::vector<double> velocities;
  std::vector<double> elev_near;
  std::vector<double> elev_far;
  std::vector<double> defl_near;
  std::vector<double> defl_far;
  std::size_t reached = 0U;
};

using SmokeRanges = std::array<std::uint32_t, 2>;

inline SmokeSamples CollectSmokeSamples(const lob::Builder& base,
                                        const SmokeRanges& ranges) {
  SmokeSamples out;
  out.velocities.reserve(kSamples);
  out.elev_near.reserve(kSamples);
  out.elev_far.reserve(kSamples);
  out.defl_near.reserve(kSamples);
  out.defl_far.reserve(kSamples);
  std::normal_distribution<double> vel_dist(kVelocityMeanFps,
                                            kVelocitySigmaFps);
  std::normal_distribution<double> wind_dist(0.0, kWindSigmaMph);
  for (std::size_t i = 0U; i < kSamples; ++i) {
    std::mt19937_64 engine(Subseed(kSeed, static_cast<std::uint64_t>(i)));
    // Phase 2 integer rule: velocity snaps to whole fps via llround.
    const auto kVel =
        static_cast<std::uint16_t>(std::llround(vel_dist(engine)));
    // Half-normal-ish wind: positive-only speed at fixed heading 90.
    const double kWind = std::fabs(wind_dist(engine));
    lob::Builder builder = base;
    builder.InitialVelocityFps(kVel);
    builder.WindSpeedMph(kWind);
    builder.WindHeadingDeg(kWindHeadingDeg);
    const lob::Context kCtx = builder.Build();
    if (kCtx.error != lob::ErrorT::kNone) {
      continue;
    }
    std::array<lob::Output, 2> outs = {};
    if (lob::Solve(kCtx, ranges, &outs) != ranges.size()) {
      continue;
    }
    ++out.reached;
    out.velocities.push_back(static_cast<double>(kVel));
    out.elev_near.push_back(outs.at(0).elevation);
    out.elev_far.push_back(outs.at(1).elevation);
    out.defl_near.push_back(outs.at(0).deflection);
    out.defl_far.push_back(outs.at(1).deflection);
  }
  return out;
}

// Integrity, not accuracy: sampled elevation means sit within 3 pilot-SE of
// the deterministic baseline, proving sampling didn't break the solver path.
inline void CheckBaselineAgreement(const SmokeSamples& samples,
                                   const std::array<lob::Output, 2>& baseline) {
  const double kMeanNear = MeanOf(samples.elev_near);
  const double kMeanFar = MeanOf(samples.elev_far);
  const double kSeNear = SdOf(samples.elev_near, kMeanNear) /
                         std::sqrt(static_cast<double>(kSamples));
  const double kSeFar = SdOf(samples.elev_far, kMeanFar) /
                        std::sqrt(static_cast<double>(kSamples));
  EXPECT_NEAR(kMeanNear, baseline.at(0).elevation, kThreeSigma * kSeNear);
  EXPECT_NEAR(kMeanFar, baseline.at(1).elevation, kThreeSigma * kSeFar);
}

inline void CheckSummaryShape(const SmokeSamples& samples) {
  const double kMeanNear = MeanOf(samples.elev_near);
  nlohmann::json summary;
  summary["seed"] = kSeed;
  summary["n"] = samples.elev_near.size();
  summary["mean"] = kMeanNear;
  summary["sd"] = SdOf(samples.elev_near, kMeanNear);
  summary["P5"] = PercentileOf(samples.elev_near, kP05);
  summary["P50"] = PercentileOf(samples.elev_near, kP50);
  summary["P95"] = PercentileOf(samples.elev_near, kP95);
  for (const char* key : {"mean", "sd", "n", "seed", "P5", "P50", "P95"}) {
    EXPECT_TRUE(summary.contains(key)) << key;
  }
}

// Velocity dominates elevation (Phase 2 agreement): |rho| > 0.9 with a
// consistent positive sign (faster flies flatter) at both ranges.
inline void CheckVelocityDominance(const SmokeSamples& samples) {
  const double kRhoNear = SpearmanOf(samples.velocities, samples.elev_near);
  const double kRhoFar = SpearmanOf(samples.velocities, samples.elev_far);
  EXPECT_GT(std::fabs(kRhoNear), kMinSpearman) << "rho900=" << kRhoNear;
  EXPECT_GT(std::fabs(kRhoFar), kMinSpearman) << "rho1800=" << kRhoFar;
  EXPECT_GT(kRhoNear, 0.0);
  EXPECT_GT(kRhoFar, 0.0);
}

// Directional integrity: positive-only wind at heading 90 gives a positive
// deflection mean, proving the wind path is live, not zeroed.
inline void CheckDeflectionSign(const SmokeSamples& samples) {
  const double kMeanNear = MeanOf(samples.defl_near);
  const double kMeanFar = MeanOf(samples.defl_far);
  const double kSeNear = SdOf(samples.defl_near, kMeanNear) /
                         std::sqrt(static_cast<double>(kSamples));
  const double kSeFar = SdOf(samples.defl_far, kMeanFar) /
                        std::sqrt(static_cast<double>(kSamples));
  EXPECT_GT(kMeanNear, 0.0);
  EXPECT_GT(kMeanFar, 0.0);
  EXPECT_GT(kMeanNear, kThreeSigma * kSeNear);
  EXPECT_GT(kMeanFar, kThreeSigma * kSeFar);
}

}  // namespace

TEST(MonteCarloSmoke, FixedSeedIntegrity) {
  lob::Builder base = tests::MakeC1IcaoBuilder();
  base.StepSize(kStepIn);
  constexpr SmokeRanges kRanges = {{900U, 1800U}};

  // Deterministic baseline: calm C1-ICAO through the public Solve.
  const lob::Context kBaseCtx = base.Build();
  ASSERT_EQ(kBaseCtx.error, lob::ErrorT::kNone);
  std::array<lob::Output, 2> baseline = {};
  ASSERT_EQ(lob::Solve(kBaseCtx, kRanges, &baseline), kRanges.size());

  const SmokeSamples kSamplesOut = CollectSmokeSamples(base, kRanges);
  // Zero build failures on this benign box.
  EXPECT_EQ(kSamplesOut.reached, kSamples);
  ASSERT_EQ(kSamplesOut.velocities.size(), kSamples);

  CheckBaselineAgreement(kSamplesOut, baseline);
  CheckSummaryShape(kSamplesOut);
  CheckVelocityDominance(kSamplesOut);
  CheckDeflectionSign(kSamplesOut);
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
