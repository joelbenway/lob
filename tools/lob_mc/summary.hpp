// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <limits>
#include <map>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "case_adapter.hpp"

namespace mc {

// Batch-means batches: B = min(32, N/8), so every batch holds >= 8 samples.
// Fewer than 2 batches (N < 16) leaves SE undefined (NaN, honestly).
constexpr std::size_t kMaxBatches = 32U;
constexpr std::size_t kMinSamplesPerBatch = 8U;
// Binomial CI half-width scale (approx. 95%).
constexpr double kBinomialZ = 1.96;
// Mean-converged verdict: SE < 0.1 * sigma (E_target default).
constexpr double kConvergenceRatio = 0.1;
// Tails beyond P1/P99 are "unresolved at N" unless N reaches this count.
constexpr std::uint64_t kTailResolveSamples = 100000U;
constexpr int kArtifactSchema = 1;
constexpr std::uint64_t kGzipSuggestSamples = 10000U;
constexpr const char* kRngName = "mt19937_64";
// SolveAngle library default (source/solve_angle.hpp): the runner never
// overrides it, so provenance records it as the fixed per-run value.
constexpr double kAngleTolMoa = 0.01;
constexpr const char* kPooledWarning =
    "DO-NOT-USE for claims: pooled across branch discontinuities; use "
    "by_branch for claims, pooled for diagnostics only.";

inline void WelfordMeanSd(const std::vector<double>& values, double* mean,
                          double* sd) {
  const double kNan = std::numeric_limits<double>::quiet_NaN();
  double running_mean = 0.0;
  double m2 = 0.0;
  std::size_t count = 0U;
  for (const double kValue : values) {
    ++count;
    const double kDelta = kValue - running_mean;
    running_mean += kDelta / static_cast<double>(count);
    m2 += kDelta * (kValue - running_mean);
  }
  if (count == 0U) {
    *mean = kNan;
    *sd = kNan;
    return;
  }
  *mean = running_mean;
  if (count < 2U) {
    *sd = kNan;
    return;
  }
  *sd = std::sqrt(m2 / static_cast<double>(count - 1U));
}

inline std::size_t BatchCount(std::size_t count) {
  const std::size_t kBySize = count / kMinSamplesPerBatch;
  if (kBySize < kMaxBatches) {
    return kBySize;
  }
  return kMaxBatches;
}

// Batch-means SE: split the sequence into B contiguous batches, take the
// batch means, return sd(batch means) / sqrt(B).
inline double BatchMeansSe(const std::vector<double>& values,
                           std::size_t batches) {
  const double kNan = std::numeric_limits<double>::quiet_NaN();
  if (values.empty() || (batches < 2U)) {
    return kNan;
  }
  std::vector<double> means(batches, 0.0);
  std::vector<std::size_t> sizes(batches, 0U);
  for (std::size_t i = 0U; i < values.size(); ++i) {
    const std::size_t kBatch = (i * batches) / values.size();
    means[kBatch] += values[i];
    sizes[kBatch] += 1U;
  }
  for (std::size_t batch = 0U; batch < batches; ++batch) {
    if (sizes[batch] == 0U) {
      return kNan;
    }
    means[batch] /= static_cast<double>(sizes[batch]);
  }
  double mean = 0.0;
  double sd = 0.0;
  WelfordMeanSd(means, &mean, &sd);
  if (!std::isfinite(sd)) {
    return kNan;
  }
  return sd / std::sqrt(static_cast<double>(batches));
}

// Nearest-rank order statistic via nth_element (deterministic for a fixed
// input order): rank = ceil(f * N), clamped to [1, N].
inline double PercentileOf(const std::vector<double>& values, double fraction) {
  const double kNan = std::numeric_limits<double>::quiet_NaN();
  if (values.empty()) {
    return kNan;
  }
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

inline double BinomialHalfWidth(double prob, std::size_t count) {
  const double kNan = std::numeric_limits<double>::quiet_NaN();
  if ((count == 0U) || (prob < 0.0) || (prob > 1.0)) {
    return kNan;
  }
  return kBinomialZ *
         std::sqrt(prob * (1.0 - prob) / static_cast<double>(count));
}

inline std::string TailVerdict(std::size_t count) {
  if (static_cast<std::uint64_t>(count) >= kTailResolveSamples) {
    return "resolved at N";
  }
  return "unresolved at N";
}

inline bool MeanConverged(double se, double sd) {
  return std::isfinite(se) && (sd > 0.0) && (se < kConvergenceRatio * sd);
}

// Average (1-based) ranks with ties sharing the mean rank. Equality is the
// negated strict ordering (never ==, per the float-equal gate).
inline std::vector<double> Ranks(const std::vector<double>& values) {
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

inline double Pearson(const std::vector<double>& xs,
                      const std::vector<double>& ys) {
  const double kNan = std::numeric_limits<double>::quiet_NaN();
  if ((xs.size() != ys.size()) || (xs.size() < 2U)) {
    return kNan;
  }
  double mean_x = 0.0;
  double mean_y = 0.0;
  for (std::size_t i = 0U; i < xs.size(); ++i) {
    mean_x += xs.at(i);
    mean_y += ys.at(i);
  }
  mean_x /= static_cast<double>(xs.size());
  mean_y /= static_cast<double>(ys.size());
  double cov = 0.0;
  double var_x = 0.0;
  double var_y = 0.0;
  for (std::size_t i = 0U; i < xs.size(); ++i) {
    const double kDx = xs.at(i) - mean_x;
    const double kDy = ys.at(i) - mean_y;
    cov += kDx * kDy;
    var_x += kDx * kDx;
    var_y += kDy * kDy;
  }
  if (!(var_x > 0.0) || !(var_y > 0.0)) {
    return kNan;
  }
  return cov / std::sqrt(var_x * var_y);
}

// Spearman rank correlation over the finite (x, y) pairs; NaN when fewer
// than 3 usable pairs or either side is constant.
inline double Spearman(const std::vector<double>& xs,
                       const std::vector<double>& ys) {
  const double kNan = std::numeric_limits<double>::quiet_NaN();
  if (xs.size() != ys.size()) {
    return kNan;
  }
  std::vector<double> finite_x;
  std::vector<double> finite_y;
  finite_x.reserve(xs.size());
  finite_y.reserve(ys.size());
  for (std::size_t i = 0U; i < xs.size(); ++i) {
    if (std::isfinite(xs.at(i)) && std::isfinite(ys.at(i))) {
      finite_x.push_back(xs.at(i));
      finite_y.push_back(ys.at(i));
    }
  }
  if (finite_x.size() < 3U) {
    return kNan;
  }
  return Pearson(Ranks(finite_x), Ranks(finite_y));
}

struct SummaryStats {
  std::size_t n = 0U;
  double mean = std::numeric_limits<double>::quiet_NaN();
  double sd = std::numeric_limits<double>::quiet_NaN();
  double se = std::numeric_limits<double>::quiet_NaN();
  bool mean_converged = false;
  double min = std::numeric_limits<double>::quiet_NaN();
  double max = std::numeric_limits<double>::quiet_NaN();
  double p2_5 = std::numeric_limits<double>::quiet_NaN();
  double p2_5_hw = std::numeric_limits<double>::quiet_NaN();
  double p5 = std::numeric_limits<double>::quiet_NaN();
  double p5_hw = std::numeric_limits<double>::quiet_NaN();
  double p50 = std::numeric_limits<double>::quiet_NaN();
  double p50_hw = std::numeric_limits<double>::quiet_NaN();
  double p95 = std::numeric_limits<double>::quiet_NaN();
  double p95_hw = std::numeric_limits<double>::quiet_NaN();
  double p97_5 = std::numeric_limits<double>::quiet_NaN();
  double p97_5_hw = std::numeric_limits<double>::quiet_NaN();
  std::string tail_verdict;
};

inline SummaryStats Summarize(const std::vector<double>& values) {
  constexpr double kP025 = 0.025;
  constexpr double kP05 = 0.05;
  constexpr double kP50 = 0.5;
  constexpr double kP95 = 0.95;
  constexpr double kP975 = 0.975;
  SummaryStats out;
  out.n = values.size();
  out.tail_verdict = TailVerdict(values.size());
  if (values.empty()) {
    return out;
  }
  WelfordMeanSd(values, &out.mean, &out.sd);
  out.min = *std::min_element(values.begin(), values.end());
  out.max = *std::max_element(values.begin(), values.end());
  out.se = BatchMeansSe(values, BatchCount(values.size()));
  out.mean_converged = MeanConverged(out.se, out.sd);
  out.p2_5 = PercentileOf(values, kP025);
  out.p2_5_hw = BinomialHalfWidth(kP025, values.size());
  out.p5 = PercentileOf(values, kP05);
  out.p5_hw = BinomialHalfWidth(kP05, values.size());
  out.p50 = PercentileOf(values, kP50);
  out.p50_hw = BinomialHalfWidth(kP50, values.size());
  out.p95 = PercentileOf(values, kP95);
  out.p95_hw = BinomialHalfWidth(kP95, values.size());
  out.p97_5 = PercentileOf(values, kP975);
  out.p97_5_hw = BinomialHalfWidth(kP975, values.size());
  return out;
}

inline nlohmann::json FiniteOrNull(double value) {
  // NOTE: assigned, never braced-returned — return {value} would build a
  // one-element JSON array instead of a number.
  nlohmann::json node;
  if (std::isfinite(value)) {
    node = value;
  }
  return node;
}

inline nlohmann::json SummaryToJson(const SummaryStats& stats) {
  nlohmann::json node = nlohmann::json::object();
  node["n"] = stats.n;
  node["mean"] = FiniteOrNull(stats.mean);
  node["sd"] = FiniteOrNull(stats.sd);
  node["se"] = FiniteOrNull(stats.se);
  node["mean_converged"] = stats.mean_converged;
  node["min"] = FiniteOrNull(stats.min);
  node["max"] = FiniteOrNull(stats.max);
  node["p2_5"] = FiniteOrNull(stats.p2_5);
  node["p2_5_ci_hw"] = FiniteOrNull(stats.p2_5_hw);
  node["p5"] = FiniteOrNull(stats.p5);
  node["p5_ci_hw"] = FiniteOrNull(stats.p5_hw);
  node["p50"] = FiniteOrNull(stats.p50);
  node["p50_ci_hw"] = FiniteOrNull(stats.p50_hw);
  node["p95"] = FiniteOrNull(stats.p95);
  node["p95_ci_hw"] = FiniteOrNull(stats.p95_hw);
  node["p97_5"] = FiniteOrNull(stats.p97_5);
  node["p97_5_ci_hw"] = FiniteOrNull(stats.p97_5_hw);
  node["tail_verdict"] = stats.tail_verdict;
  return node;
}

// Branch label for one sample: "clean", or the active branch flags joined.
// Build failures (no solver outputs) label separately and never enter a
// value summary.
inline std::string BranchLabel(const BranchFlags& flags) {
  if (flags.build_failed) {
    return "build_failed";
  }
  std::string label;
  if (!flags.reached_all) {
    label += "fall_short";
  }
  if (flags.miller_unstable) {
    if (!label.empty()) {
      label += "+";
    }
    label += "miller_unstable";
  }
  if (flags.angle_cap_hit) {
    if (!label.empty()) {
      label += "+";
    }
    label += "angle_cap_hit";
  }
  if (label.empty()) {
    return "clean";
  }
  return label;
}

inline std::map<std::string, std::vector<std::size_t>> BranchGroups(
    const std::vector<TrajectorySample>& samples) {
  std::map<std::string, std::vector<std::size_t>> groups;
  for (std::size_t i = 0U; i < samples.size(); ++i) {
    groups[BranchLabel(samples.at(i).flags)].push_back(i);
  }
  return groups;
}

// Finite forward-channel values over an index subset; build-failed and
// fall-short samples contribute nothing (their prefix is simply absent).
inline std::vector<double> GroupChannelValues(
    const std::vector<TrajectorySample>& samples,
    const std::vector<std::size_t>& indices, std::size_t pos, bool elevation) {
  std::vector<double> values;
  values.reserve(indices.size());
  for (const std::size_t kIndex : indices) {
    const TrajectorySample& sample = samples.at(kIndex);
    if (sample.flags.build_failed) {
      continue;
    }
    if ((pos >= sample.forward_count) || (pos >= sample.forward.size())) {
      continue;
    }
    const double kValue = elevation ? sample.forward.at(pos).elevation
                                    : sample.forward.at(pos).deflection;
    if (std::isfinite(kValue)) {
      values.push_back(kValue);
    }
  }
  return values;
}

inline std::vector<double> ChannelValues(
    const std::vector<TrajectorySample>& samples, std::size_t pos,
    bool elevation) {
  std::vector<std::size_t> indices(samples.size(), 0U);
  for (std::size_t i = 0U; i < samples.size(); ++i) {
    indices[i] = i;
  }
  return GroupChannelValues(samples, indices, pos, elevation);
}

inline std::string ElevKey(std::uint32_t range) {
  return "elev_in_" + std::to_string(range);
}

inline std::string DeflKey(std::uint32_t range) {
  return "defl_in_" + std::to_string(range);
}

// Spearman(input draw, output channel) over samples carrying both. Pair
// counts equal the channel value count: draws are NaN-padded exactly when
// the sample failed and carries no outputs.
inline double ChannelDrawSpearman(const std::vector<TrajectorySample>& samples,
                                  std::size_t pos, bool elevation,
                                  std::size_t draw) {
  std::vector<double> xs;
  std::vector<double> ys;
  xs.reserve(samples.size());
  ys.reserve(samples.size());
  for (const TrajectorySample& sample : samples) {
    if (sample.flags.build_failed) {
      continue;
    }
    if ((pos >= sample.forward_count) || (pos >= sample.forward.size()) ||
        (draw >= sample.draws.size())) {
      continue;
    }
    const double kValue = elevation ? sample.forward.at(pos).elevation
                                    : sample.forward.at(pos).deflection;
    const double kDraw = sample.draws.at(draw);
    if (std::isfinite(kValue) && std::isfinite(kDraw)) {
      xs.push_back(kDraw);
      ys.push_back(kValue);
    }
  }
  return Spearman(xs, ys);
}

// One output channel: pooled summary (labeled across-branches iff the values
// split over branch labels) plus the per-branch summaries.
inline nlohmann::json ChannelSummaryJson(
    const std::vector<TrajectorySample>& samples,
    const std::map<std::string, std::vector<std::size_t>>& groups,
    std::size_t pos, bool elevation) {
  nlohmann::json node = nlohmann::json::object();
  nlohmann::json branches = nlohmann::json::object();
  std::size_t valued_groups = 0U;
  for (const auto& entry : groups) {
    const std::vector<double> kValues =
        GroupChannelValues(samples, entry.second, pos, elevation);
    if (kValues.empty()) {
      continue;
    }
    ++valued_groups;
    branches[entry.first] = SummaryToJson(Summarize(kValues));
  }
  const bool kSplit = valued_groups > 1U;
  node["pooled_across_branches"] = kSplit;
  if (kSplit) {
    node["warning"] = kPooledWarning;
  }
  node["pooled"] =
      SummaryToJson(Summarize(ChannelValues(samples, pos, elevation)));
  node["by_branch"] = branches;
  return node;
}

inline nlohmann::json ChannelSpearmanJson(
    const std::vector<TrajectorySample>& samples,
    const std::vector<std::string>& draw_names,
    const std::vector<double>& values, std::size_t pos, bool elevation) {
  nlohmann::json node = nlohmann::json::object();
  node["spearman_n"] = values.size();
  nlohmann::json pairs = nlohmann::json::object();
  for (std::size_t draw = 0U; draw < draw_names.size(); ++draw) {
    const double kRho = ChannelDrawSpearman(samples, pos, elevation, draw);
    if (std::isfinite(kRho)) {
      pairs[draw_names.at(draw)] = kRho;
    }
  }
  node["inputs"] = pairs;
  return node;
}

// Full mc_run_{id} document: legacy top-level counts (kept for continuity),
// the §12.3 probability readouts (p_build_fail + per-branch p_branch are
// just normalized counters), the §15.2 provenance header, per-channel
// pooled/branch-split summaries, and Spearman-vs-Pareto inputs.
inline nlohmann::json BuildRunJson(
    const RunManifest& manifest, const std::string& cell, std::uint64_t seed,
    std::uint64_t total_samples, std::uint64_t workers,
    const std::string& csv_path, const std::string& lob_version,
    const std::string& git_sha, const std::string& compiler,
    const std::string& platform, const std::vector<std::string>& draw_names,
    const std::vector<std::uint32_t>& ranges,
    const std::vector<TrajectorySample>& samples) {
  std::uint64_t build_failed = 0U;
  std::uint64_t reached_all = 0U;
  std::uint64_t miller_unstable = 0U;
  std::uint64_t angle_cap_hit = 0U;
  for (const TrajectorySample& sample : samples) {
    if (sample.flags.build_failed) {
      ++build_failed;
    }
    if (sample.flags.reached_all) {
      ++reached_all;
    }
    if (sample.flags.miller_unstable) {
      ++miller_unstable;
    }
    if (sample.flags.angle_cap_hit) {
      ++angle_cap_hit;
    }
  }
  const auto kTotal = static_cast<double>(samples.size());
  const auto kFails = static_cast<double>(build_failed);
  const double kPBuildFail = (kTotal > 0.0) ? (kFails / kTotal) : 0.0;
  const std::uint64_t kReached = reached_all + build_failed;
  const std::uint64_t kFallShort =
      (samples.size() >= kReached) ? (samples.size() - kReached) : 0U;

  nlohmann::json doc = nlohmann::json::object();
  nlohmann::json provenance = nlohmann::json::object();
  provenance["lob_version"] = lob_version;
  provenance["git_sha"] = git_sha;
  provenance["compiler"] = compiler;
  provenance["platform"] = platform;
  nlohmann::json solver = nlohmann::json::object();
  solver["step_in"] = manifest.solver.step_in;
  solver["angle_tol_moa"] = kAngleTolMoa;
  solver["density_path"] = manifest.solver.density_path;
  solver["ranges_ft"] = manifest.solver.ranges;
  provenance["solver_config"] = solver;
  provenance["seed"] = seed;
  provenance["rng"] = kRngName;
  provenance["artifact_schema"] = kArtifactSchema;
  // No created_utc: timestamps break diff-clean reruns.
  // No inputs_hash: deferred until canonical builder-JSON hashing lands.
  provenance["synthetic_illustrative_only"] =
      manifest.synthetic_illustrative_only;
  doc["provenance"] = provenance;

  doc["run_id"] = manifest.run_id;
  doc["cell"] = cell;
  doc["seed"] = seed;
  doc["samples"] = total_samples;
  doc["workers"] = workers;
  doc["out_csv"] = csv_path;
  doc["synthetic_illustrative_only"] = manifest.synthetic_illustrative_only;
  doc["build_failed"] = build_failed;
  doc["reached_all"] = reached_all;
  doc["miller_unstable"] = miller_unstable;
  doc["angle_cap_hit"] = angle_cap_hit;
  doc["lob_version"] = lob_version;
  doc["git_sha"] = git_sha;
  doc["p_build_fail"] = kPBuildFail;
  nlohmann::json branches = nlohmann::json::object();
  branches["fall_short"] =
      (kTotal > 0.0) ? (static_cast<double>(kFallShort) / kTotal) : 0.0;
  branches["miller_unstable"] =
      (kTotal > 0.0) ? (static_cast<double>(miller_unstable) / kTotal) : 0.0;
  branches["angle_cap_hit"] =
      (kTotal > 0.0) ? (static_cast<double>(angle_cap_hit) / kTotal) : 0.0;
  doc["p_branch"] = branches;

  const std::map<std::string, std::vector<std::size_t>> kGroups =
      BranchGroups(samples);
  nlohmann::json outputs = nlohmann::json::object();
  nlohmann::json spearmans = nlohmann::json::object();
  for (std::size_t pos = 0U; pos < ranges.size(); ++pos) {
    const std::string kElev = ElevKey(ranges.at(pos));
    const std::string kDefl = DeflKey(ranges.at(pos));
    outputs[kElev] =
        ChannelSummaryJson(samples, kGroups, pos, /*elevation=*/true);
    outputs[kDefl] =
        ChannelSummaryJson(samples, kGroups, pos, /*elevation=*/false);
    const std::vector<double> kElevValues =
        ChannelValues(samples, pos, /*elevation=*/true);
    const std::vector<double> kDeflValues =
        ChannelValues(samples, pos, /*elevation=*/false);
    spearmans[kElev] = ChannelSpearmanJson(samples, draw_names, kElevValues,
                                           pos, /*elevation=*/true);
    spearmans[kDefl] = ChannelSpearmanJson(samples, draw_names, kDeflValues,
                                           pos, /*elevation=*/false);
  }
  doc["outputs"] = outputs;
  doc["spearman"] = spearmans;
  return doc;
}

inline bool WriteJsonFile(const std::string& path, const nlohmann::json& doc) {
  constexpr int kJsonIndent = 2;
  std::ofstream out(path);
  if (!out.is_open()) {
    return false;
  }
  out << doc.dump(kJsonIndent) << '\n';
  out.flush();
  return static_cast<bool>(out);
}

inline void WriteSampleRow(std::ostream& out, const TrajectorySample& sample,
                           const std::vector<std::uint32_t>& ranges) {
  const double kMissing = std::numeric_limits<double>::quiet_NaN();
  constexpr std::uint32_t kMissingInt = 0U;
  out << sample.index;
  for (const double kDraw : sample.draws) {
    out << ',' << kDraw;
  }
  for (std::size_t pos = 0U; pos < ranges.size(); ++pos) {
    const bool kHit =
        (pos < sample.forward_count) && (pos < sample.forward.size());
    const double kElev = kHit ? sample.forward.at(pos).elevation : kMissing;
    const double kDefl = kHit ? sample.forward.at(pos).deflection : kMissing;
    const std::uint32_t kVel =
        kHit ? sample.forward.at(pos).velocity : kMissingInt;
    const std::uint32_t kEnergy =
        kHit ? sample.forward.at(pos).energy : kMissingInt;
    const double kTof = kHit ? sample.forward.at(pos).time_of_flight : kMissing;
    out << ',' << kElev << ',' << kDefl << ',' << kVel << ',' << kEnergy << ','
        << kTof;
  }
  const std::uint64_t kReached = sample.flags.reached_all ? 1U : 0U;
  const std::uint64_t kMiller = sample.flags.miller_unstable ? 1U : 0U;
  const std::uint64_t kCap = sample.flags.angle_cap_hit ? 1U : 0U;
  const std::uint64_t kFailed = sample.flags.build_failed ? 1U : 0U;
  out << ',' << kReached << ',' << sample.flags.fall_short_index << ','
      << kMiller << ',' << sample.stability << ',' << kCap << ',' << kFailed
      << ',' << sample.flags.configured_density_path << '\n';
}

// samples.csv is the determinism artifact: fixed header, full-precision
// doubles, per-sample rows in index order, no timestamps or pooled stats.
inline bool WriteSamplesCsv(const std::string& path,
                            const std::vector<std::string>& draw_names,
                            const std::vector<std::uint32_t>& ranges,
                            const std::vector<mc::TrajectorySample>& samples) {
  constexpr int kFullPrecision = 17;
  std::ofstream csv(path);
  if (!csv.is_open()) {
    return false;
  }
  csv << std::setprecision(kFullPrecision);
  csv << "index";
  for (const std::string& name : draw_names) {
    csv << ',' << name;
  }
  for (const std::uint32_t kRange : ranges) {
    csv << ",elev_in_" << kRange << ",defl_in_" << kRange << ",vel_fps_"
        << kRange << ",energy_ftlbs_" << kRange << ",tof_s_" << kRange;
  }
  // CSV v2: v1 columns renamed (tumble_hit -> miller_unstable, density_path
  // -> configured_density_path) and stability added; column order otherwise
  // fixed, no schema break beyond the rename.
  csv << ",reached,fall_short_index,miller_unstable,stability,angle_cap_hit,"
         "build_failed,configured_density_path\n";
  for (const mc::TrajectorySample& sample : samples) {
    WriteSampleRow(csv, sample, ranges);
  }
  csv.flush();
  return static_cast<bool>(csv);
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
