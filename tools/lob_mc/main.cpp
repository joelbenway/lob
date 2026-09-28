// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "case_adapter.hpp"
#include "lob/lob.hpp"
#include "sampler.hpp"

namespace {

constexpr int kExitOk = 0;
constexpr int kExitUsage = 2;
constexpr int kExitBroken = 3;

constexpr const char* kHelpLong = "--help";
constexpr const char* kHelpShort = "-h";
constexpr const char* kSelfcheckFlag = "--selfcheck";
constexpr const char* kManifestOpt = "--manifest=";
constexpr const char* kSeedOpt = "--seed=";
constexpr const char* kSamplesOpt = "--samples=";
constexpr const char* kWorkersOpt = "--workers=";
constexpr const char* kOutDirOpt = "--out-dir=";
constexpr const char* kCellOpt = "--cell=";
constexpr const char* kDefaultOutDir = "build/validation";

constexpr std::uint64_t kSelfcheckSeed = 0x9E3779B9ULL;
constexpr std::size_t kSelfcheckSamples = 1024U;
constexpr std::size_t kSelfcheckStreams = 32U;
constexpr std::size_t kSingleWorker = 1U;
constexpr std::size_t kQuadWorkers = 4U;
constexpr double kSelfcheckMean = 0.0;
constexpr double kSelfcheckSigma = 1.0;
constexpr std::uint64_t kZeroSeed = 0U;
constexpr std::uint64_t kSplitmixZeroRecord = 0xE220A8397B1DCDAFULL;
constexpr int kDecimalBase = 10;
constexpr int kJsonIndent = 2;

struct Config {
  bool show_help = false;
  bool selfcheck = false;
  bool parse_error = false;
  bool has_manifest = false;
  bool has_seed = false;
  bool has_samples = false;
  bool has_workers = false;
  bool has_out_dir = false;
  bool has_cell = false;
  std::string manifest_path;
  std::string out_dir = kDefaultOutDir;
  std::string cell;
  std::uint64_t seed = 0U;
  std::uint64_t samples = 0U;
  std::uint64_t workers = 0U;
};

bool ParseUint64(const std::string& text, std::uint64_t* out) {
  if ((out == nullptr) || text.empty() || (text.at(0) == '-')) {
    return false;
  }
  errno = 0;
  char* end = nullptr;
  const std::uint64_t kValue = std::strtoull(text.c_str(), &end, kDecimalBase);
  if ((end == nullptr) || (*end != '\0') || (errno == ERANGE)) {
    return false;
  }
  *out = kValue;
  return true;
}

void ParseUintOption(const std::string& arg, const char* opt,
                     std::uint64_t* value, bool* has, Config* config) {
  const std::string kText = arg.substr(std::strlen(opt));
  if (!ParseUint64(kText, value)) {
    std::cerr << "lob_mc: bad " << opt << " value " << kText << '\n';
    config->parse_error = true;
    return;
  }
  *has = true;
}

Config ParseArgs(int argc, char** argv) {
  Config config;
  for (int i = 1; i < argc; ++i) {
    const std::string kArg(argv[i]);
    if ((kArg == kHelpLong) || (kArg == kHelpShort)) {
      config.show_help = true;
    } else if (kArg == kSelfcheckFlag) {
      config.selfcheck = true;
    } else if (kArg.compare(0, std::strlen(kManifestOpt), kManifestOpt) == 0) {
      config.has_manifest = true;
      config.manifest_path = kArg.substr(std::strlen(kManifestOpt));
    } else if (kArg.compare(0, std::strlen(kSeedOpt), kSeedOpt) == 0) {
      ParseUintOption(kArg, kSeedOpt, &config.seed, &config.has_seed, &config);
    } else if (kArg.compare(0, std::strlen(kSamplesOpt), kSamplesOpt) == 0) {
      ParseUintOption(kArg, kSamplesOpt, &config.samples, &config.has_samples,
                      &config);
    } else if (kArg.compare(0, std::strlen(kWorkersOpt), kWorkersOpt) == 0) {
      ParseUintOption(kArg, kWorkersOpt, &config.workers, &config.has_workers,
                      &config);
    } else if (kArg.compare(0, std::strlen(kOutDirOpt), kOutDirOpt) == 0) {
      config.has_out_dir = true;
      config.out_dir = kArg.substr(std::strlen(kOutDirOpt));
    } else if (kArg.compare(0, std::strlen(kCellOpt), kCellOpt) == 0) {
      config.has_cell = true;
      config.cell = kArg.substr(std::strlen(kCellOpt));
    } else {
      std::cerr << "lob_mc: unknown option " << kArg << '\n';
      config.parse_error = true;
    }
  }
  return config;
}

void PrintUsage(std::ostream& out) {
  out << "Usage: lob_mc --manifest=FILE [--seed=N] [--samples=N] "
         "[--workers=N] [--out-dir=DIR] [--cell=CELL]\n"
      << "       lob_mc --selfcheck\n"
      << "       lob_mc --help\n"
      << "Monte Carlo runner: samples the manifest dimensions through the\n"
      << "unchanged deterministic solver (one Build + Solve + SolveInverse\n"
      << "per sample) and writes samples.csv with per-sample branch flags.\n"
      << "--cell selects a dimension set from the manifest (default: the\n"
      << "manifest cell). Results assemble by sample index, so 1-vs-N\n"
      << "workers are byte-identical. Exit codes: 0 ok, 2 usage or bad\n"
      << "manifest, 3 every sample failed to build.\n"
      << "lob " << lob::Version() << " " << LOB_GIT_SHA << '\n';
}

// Draws the owned stream range into out by sample index. Sample i always
// comes from stream (i % num_streams), so the assembled sequence never
// depends on how streams are grouped into workers.
void RunWorkerStreams(std::uint64_t seed, std::size_t first_stream,
                      std::size_t num_owned, std::size_t num_samples,
                      std::size_t num_streams, std::vector<double>* out) {
  constexpr double kBound = std::numeric_limits<double>::max();
  constexpr std::size_t kOne = 1U;
  mc::Truncation wide;
  wide.lo = -kBound;
  wide.hi = kBound;
  for (std::size_t stream = first_stream; stream < (first_stream + num_owned);
       ++stream) {
    if (stream >= num_samples) {
      continue;
    }
    std::mt19937_64 engine(
        mc::Subseed(seed, static_cast<std::uint64_t>(stream)));
    const std::size_t kCount =
        ((num_samples - stream - kOne) / num_streams) + kOne;
    for (std::size_t k = 0U; k < kCount; ++k) {
      const std::size_t kIndex = stream + (k * num_streams);
      const mc::Sample kSample =
          mc::SampleNormal(engine, kSelfcheckMean, kSelfcheckSigma, wide);
      // Unreachable with finite draws: +-max bounds accept everything.
      // The deterministic fallback keeps the sequence defined regardless.
      double value = kSelfcheckMean;
      if (kSample.ok) {
        value = kSample.value;
      }
      out->at(kIndex) = value;
    }
  }
}

// Precondition: num_workers >= 1. Threads partition the fixed logical
// streams; results assemble by sample index, never completion order.
std::vector<double> DrawNormals(std::uint64_t seed, std::size_t num_samples,
                                std::size_t num_streams,
                                std::size_t num_workers) {
  constexpr double kZero = 0.0;
  constexpr std::size_t kOne = 1U;
  std::vector<double> values(num_samples, kZero);
  const std::size_t kBase = num_streams / num_workers;
  const std::size_t kRem = num_streams % num_workers;
  std::vector<std::thread> threads;
  std::size_t first = 0U;
  for (std::size_t slot = 0U; slot < num_workers; ++slot) {
    std::size_t count = kBase;
    if (slot < kRem) {
      count += kOne;
    }
    threads.emplace_back(RunWorkerStreams, seed, first, count, num_samples,
                         num_streams, &values);
    first += count;
  }
  for (auto& worker : threads) {
    worker.join();
  }
  return values;
}

int RunSelfcheck() {
  const std::uint64_t kRecord = mc::Splitmix64(kZeroSeed);
  std::cout << "splitmix64(0) = 0x" << std::hex << kRecord << std::dec << '\n';
  if (kRecord != kSplitmixZeroRecord) {
    std::cerr << "selfcheck: FAIL splitmix64 record mismatch\n";
    return kExitBroken;
  }
  const std::vector<double> kSingle = DrawNormals(
      kSelfcheckSeed, kSelfcheckSamples, kSelfcheckStreams, kSingleWorker);
  const std::vector<double> kQuad = DrawNormals(
      kSelfcheckSeed, kSelfcheckSamples, kSelfcheckStreams, kQuadWorkers);
  if (kSingle.size() != kQuad.size()) {
    std::cerr << "selfcheck: FAIL worker runs disagree in length\n";
    return kExitBroken;
  }
  const std::size_t kBytes = kSingle.size() * sizeof(double);
  if (std::memcmp(kSingle.data(), kQuad.data(), kBytes) != 0) {
    std::cerr << "selfcheck: FAIL 1-vs-4-worker sequences differ\n";
    return kExitBroken;
  }
  std::cout << "selfcheck: PASS " << kSelfcheckSamples
            << " normals identical across 1 and 4 workers ("
            << kSelfcheckStreams << " streams)\n";
  return kExitOk;
}

// Draws the owned stream range into out by sample index. Stream count always
// equals sample count, so sample i comes from stream (i % S) with a
// per-stream Subseed(seed, stream) engine — the selfcheck scheme verbatim.
void RunWorkerSamples(const mc::RunPlan& plan, std::size_t first_stream,
                      std::size_t num_owned,
                      std::vector<mc::TrajectorySample>* out) {
  for (std::size_t stream = first_stream; stream < (first_stream + num_owned);
       ++stream) {
    out->at(stream) = mc::RunSample(plan, stream);
  }
}

// Precondition: num_workers >= 1. Threads partition the fixed logical
// streams; results assemble by sample index, never completion order.
std::vector<mc::TrajectorySample> RunAllSamples(const mc::RunPlan& plan,
                                                std::size_t num_samples,
                                                std::size_t num_workers) {
  constexpr std::size_t kOne = 1U;
  std::vector<mc::TrajectorySample> samples(num_samples);
  const std::size_t kBase = num_samples / num_workers;
  const std::size_t kRem = num_samples % num_workers;
  std::vector<std::thread> threads;
  std::size_t first = 0U;
  for (std::size_t slot = 0U; slot < num_workers; ++slot) {
    std::size_t count = kBase;
    if (slot < kRem) {
      count += kOne;
    }
    threads.emplace_back(RunWorkerSamples, plan, first, count, &samples);
    first += count;
  }
  for (auto& worker : threads) {
    worker.join();
  }
  return samples;
}

void WriteSampleRow(std::ostream& out, const mc::TrajectorySample& sample,
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
bool WriteSamplesCsv(const std::string& path,
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

int PrintRunSummary(const mc::RunManifest& manifest, const std::string& cell,
                    std::uint64_t seed, std::uint64_t total_samples,
                    std::uint64_t workers, const std::string& csv_path,
                    const std::vector<mc::TrajectorySample>& samples) {
  std::uint64_t build_failed = 0U;
  std::uint64_t reached_all = 0U;
  std::uint64_t miller_unstable = 0U;
  std::uint64_t angle_cap_hit = 0U;
  for (const mc::TrajectorySample& sample : samples) {
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
  nlohmann::json summary;
  summary["run_id"] = manifest.run_id;
  summary["cell"] = cell;
  summary["seed"] = seed;
  summary["samples"] = total_samples;
  summary["workers"] = workers;
  summary["out_csv"] = csv_path;
  summary["synthetic_illustrative_only"] = manifest.synthetic_illustrative_only;
  summary["build_failed"] = build_failed;
  summary["reached_all"] = reached_all;
  summary["miller_unstable"] = miller_unstable;
  summary["angle_cap_hit"] = angle_cap_hit;
  summary["lob_version"] = lob::Version();
  summary["git_sha"] = LOB_GIT_SHA;
  std::cout << summary.dump(kJsonIndent) << '\n';
  if (build_failed == total_samples) {
    return kExitBroken;
  }
  return kExitOk;
}

int RunFromManifest(const Config& config) {
  std::ifstream input(config.manifest_path);
  if (!input.is_open()) {
    std::cerr << "lob_mc: cannot open manifest " << config.manifest_path
              << '\n';
    return kExitUsage;
  }
  nlohmann::json root = nlohmann::json::object();
  try {
    input >> root;
  } catch (const nlohmann::json::exception& e) {
    std::cerr << "lob_mc: bad manifest JSON: " << e.what() << '\n';
    return kExitUsage;
  }
  mc::RunManifest manifest;
  std::string error;
  if (!mc::ParseManifest(root, &manifest, &error)) {
    std::cerr << "lob_mc: " << error << '\n';
    return kExitUsage;
  }
  const std::string kCell = config.has_cell ? config.cell : manifest.cell;
  const std::uint64_t kSeed = config.has_seed ? config.seed : manifest.seed;
  const std::uint64_t kSamples =
      config.has_samples ? config.samples : manifest.samples;
  const std::uint64_t kWorkers =
      config.has_workers ? config.workers : manifest.workers;
  if ((kSamples == 0U) || (kWorkers == 0U)) {
    std::cerr << "lob_mc: samples and workers must be nonzero\n";
    return kExitUsage;
  }
  std::string out_dir = config.out_dir;
  if (!config.has_out_dir && root.contains("out_dir") &&
      root.at("out_dir").is_string()) {
    out_dir = root.at("out_dir").get<std::string>();
  }
  lob::Builder base;
  if (!mc::BaseBuilderFor(kCell, &base, &error)) {
    std::cerr << "lob_mc: " << error << '\n';
    return kExitUsage;
  }
  const std::vector<mc::Dimension>* dimensions = nullptr;
  const std::vector<mc::Scenario>* scenarios = nullptr;
  if (!mc::SelectCell(manifest, kCell, &dimensions, &scenarios, &error)) {
    std::cerr << "lob_mc: " << error << '\n';
    return kExitUsage;
  }
  const std::vector<std::string> kDrawNames =
      mc::DrawColumnNames(*scenarios, *dimensions);
  mc::RunPlan plan;
  plan.seed = kSeed;
  plan.base = &base;
  plan.dimensions = dimensions;
  plan.scenarios = scenarios;
  plan.ranges = &manifest.solver.ranges;
  plan.step_in = manifest.solver.step_in;
  plan.configured_density_path = &manifest.solver.density_path;
  plan.draw_width = kDrawNames.size();
  const auto kNumSamples = static_cast<std::size_t>(kSamples);
  const auto kNumWorkers = static_cast<std::size_t>(kWorkers);
  const std::vector<mc::TrajectorySample> kSamplesOut =
      RunAllSamples(plan, kNumSamples, kNumWorkers);
  std::string csv_path = out_dir;
  if (!csv_path.empty() && (csv_path.back() != '/')) {
    csv_path += '/';
  }
  csv_path += "samples.csv";
  if (!WriteSamplesCsv(csv_path, kDrawNames, manifest.solver.ranges,
                       kSamplesOut)) {
    std::cerr << "lob_mc: cannot write " << csv_path << '\n';
    return kExitUsage;
  }
  return PrintRunSummary(manifest, kCell, kSeed, kSamples, kWorkers, csv_path,
                         kSamplesOut);
}

}  // namespace

int main(int argc, char** argv) {
  const Config kConfig = ParseArgs(argc, argv);
  if (kConfig.parse_error) {
    PrintUsage(std::cerr);
    return kExitUsage;
  }
  if (kConfig.show_help) {
    PrintUsage(std::cout);
    return kExitOk;
  }
  if (kConfig.selfcheck) {
    if (kConfig.has_manifest || kConfig.has_seed || kConfig.has_samples ||
        kConfig.has_workers || kConfig.has_out_dir || kConfig.has_cell) {
      std::cerr << "lob_mc: --selfcheck takes no other options\n";
      return kExitUsage;
    }
    return RunSelfcheck();
  }
  if (!kConfig.has_manifest) {
    std::cerr << "lob_mc: need --manifest=FILE\n";
    PrintUsage(std::cerr);
    return kExitUsage;
  }
  try {
    return RunFromManifest(kConfig);
  } catch (const std::exception& e) {
    std::cerr << "lob_mc: run failed: " << e.what() << '\n';
    return kExitUsage;
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
