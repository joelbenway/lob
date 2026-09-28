// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#pragma once

#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

namespace mc {

// Splitmix64 state mixer (Steele et al.): Splitmix64(x) is the output the
// stateful generator produces when its state advances from x, i.e. the mix of
// (x + kIncrement). Splitmix64(0) is therefore the canonical first output
// 0xe220a8397b1dcdaf, pinned by the --selfcheck record check.
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

// Subseed(seed, w) = Splitmix64(seed ^ (w + 1)); the +1 mixes worker 0.
inline std::uint64_t Subseed(std::uint64_t seed, std::uint64_t worker) {
  constexpr std::uint64_t kWorkerOffset = 1U;
  return Splitmix64(seed ^ (worker + kWorkerOffset));
}

// Draw-then-check truncation window: resample until the draw lands in
// [lo, hi], up to max_retries resamples, then report ok=false so the CALLER
// counts the failed sample. Never infinite-loops, never clips silently.
struct Truncation {
  static constexpr int kDefaultMaxRetries = 100;
  double lo = 0.0;
  double hi = 0.0;
  int max_retries = kDefaultMaxRetries;
};

struct Sample {
  double value = 0.0;
  bool ok = false;
};

inline bool WithinBounds(const Truncation& trunc, double value) {
  return (value >= trunc.lo) && (value <= trunc.hi);
}

inline Sample SampleNormal(std::mt19937_64& engine, double mean, double sigma,
                           const Truncation& trunc) {
  if (!(sigma > 0.0)) {
    return {};
  }
  std::normal_distribution<double> dist(mean, sigma);
  for (int attempt = 0; attempt <= trunc.max_retries; ++attempt) {
    const double kDraw = dist(engine);
    if (WithinBounds(trunc, kDraw)) {
      Sample out;
      out.value = kDraw;
      out.ok = true;
      return out;
    }
  }
  return {};
}

inline Sample SampleUniform(std::mt19937_64& engine, double lo, double hi,
                            const Truncation& trunc) {
  if (!(hi > lo)) {
    return {};
  }
  std::uniform_real_distribution<double> dist(lo, hi);
  for (int attempt = 0; attempt <= trunc.max_retries; ++attempt) {
    const double kDraw = dist(engine);
    if (WithinBounds(trunc, kDraw)) {
      Sample out;
      out.value = kDraw;
      out.ok = true;
      return out;
    }
  }
  return {};
}

// Symmetric triangular on [a, b] via the two-uniform difference method
// ((u1 + u2) / 2 peaks at 0.5); no custom PDF math. The mode c is validated
// inside [a, b] (fail-closed) and selects no asymmetry.
inline Sample SampleTriangular(std::mt19937_64& engine, double a, double c,
                               double b, const Truncation& trunc) {
  if ((a > c) || (c > b) || !(b > a)) {
    return {};
  }
  constexpr double kUnitLo = 0.0;
  constexpr double kUnitHi = 1.0;
  constexpr double kHalf = 0.5;
  std::uniform_real_distribution<double> unit(kUnitLo, kUnitHi);
  for (int attempt = 0; attempt <= trunc.max_retries; ++attempt) {
    const double kFirst = unit(engine);
    const double kSecond = unit(engine);
    const double kDraw = a + ((b - a) * ((kFirst + kSecond) * kHalf));
    if (WithinBounds(trunc, kDraw)) {
      Sample out;
      out.value = kDraw;
      out.ok = true;
      return out;
    }
  }
  return {};
}

inline Sample SampleFixed(double value) {
  Sample out;
  out.value = value;
  out.ok = true;
  return out;
}

// Weighted scenario index (G-curve weights etc.): scenario selection only,
// never continuous perturbation. Empty weights fall back to the degenerate
// single-scenario distribution (always index 0) rather than UB.
inline std::size_t SampleCategorical(std::mt19937_64& engine,
                                     const std::vector<double>& weights) {
  if (weights.empty()) {
    std::discrete_distribution<std::size_t> fallback;
    return fallback(engine);
  }
  std::discrete_distribution<std::size_t> dist(weights.begin(), weights.end());
  return dist(engine);
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
