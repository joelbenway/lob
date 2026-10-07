// Copyright (c) 2026  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "testing.hpp"

namespace tests {

struct BudgetRow {
  double c = 0.0;
  double u = 0.0;
  bool u_known = false;
  bool nonlinear = false;
  double floor_value = 0.0;
};

struct BudgetCovariance {
  std::size_t i = 0;
  std::size_t j = 0;
  double cov = 0.0;
};

struct BudgetResult {
  bool complete = false;
  double u_c = 0.0;
  std::string status;
};

namespace {

inline BudgetResult CombineBudget(const std::vector<BudgetRow>& rows,
                                  const std::vector<BudgetCovariance>& covs =
                                      std::vector<BudgetCovariance>()) {
  constexpr double kCovarianceWeight = 2.0;
  for (const BudgetRow& r : rows) {
    if (r.nonlinear) {
      const BudgetResult kOut = {
          false, std::numeric_limits<double>::quiet_NaN(), "route_to_mc"};
      return kOut;
    }
  }
  for (const BudgetRow& r : rows) {
    if (!r.u_known) {
      const BudgetResult kOut = {false,
                                 std::numeric_limits<double>::quiet_NaN(),
                                 "incomplete-missing-u"};
      return kOut;
    }
  }
  double sum = 0.0;
  for (const BudgetRow& r : rows) {
    const double kTerm = r.c * r.u;
    sum += kTerm * kTerm;
  }
  for (const BudgetCovariance& cov : covs) {
    sum += kCovarianceWeight * cov.cov;
  }
  const BudgetResult kOut = {true, std::sqrt(sum), "complete"};
  return kOut;
}

}  // namespace

TEST(BudgetMath, TwoRowCombinationMatchesHandComputation) {
  const std::vector<BudgetRow> kRows = {{2.0, 3.0, true, false, 0.0},
                                        {1.0, 4.0, true, false, 0.0}};
  const BudgetResult kR = CombineBudget(kRows);
  ASSERT_TRUE(kR.complete);
  EXPECT_NEAR(kR.u_c, 7.211102551, 1e-9);
}

TEST(BudgetMath, SingleRowCombinationIsExact) {
  const std::vector<BudgetRow> kRows = {{0.5, 2.0, true, false, 0.0}};
  const BudgetResult kR = CombineBudget(kRows);
  ASSERT_TRUE(kR.complete);
  EXPECT_DOUBLE_EQ(kR.u_c, 1.0);
}

TEST(BudgetMath, UnknownUFailsClosedWithNaN) {
  const std::vector<BudgetRow> kRows = {{2.0, 3.0, true, false, 0.0},
                                        {1.0, 0.0, false, false, 0.0}};
  const BudgetResult kR = CombineBudget(kRows);
  EXPECT_FALSE(kR.complete);
  EXPECT_TRUE(std::isnan(kR.u_c));
}

TEST(BudgetMath, NonlinearRowRoutesToMc) {
  const std::vector<BudgetRow> kRows = {{2.0, 3.0, true, true, 0.0}};
  const BudgetResult kR = CombineBudget(kRows);
  EXPECT_FALSE(kR.complete);
  EXPECT_EQ(kR.status, "route_to_mc");
}

TEST(BudgetMath, CovariancePairAddsTwiceCovInsideRoot) {
  const std::vector<BudgetRow> kRows = {{2.0, 3.0, true, false, 0.0},
                                        {1.0, 4.0, true, false, 0.0}};
  const std::vector<BudgetCovariance> kCov = {{0, 1, 6.0}};
  const BudgetResult kR = CombineBudget(kRows, kCov);
  ASSERT_TRUE(kR.complete);
  EXPECT_DOUBLE_EQ(kR.u_c, 8.0);
}

// ---- Task 2: manifest loader + template tests (fail-closed) ----

struct ManifestRow {
  std::string input;
  double c = 0.0;
  bool u_known = false;
  std::string status;
};

struct ManifestCell {
  std::string cell;
  std::string channel;
  int range_ft = 0;
  std::vector<ManifestRow> rows;
};

struct ManifestLoad {
  bool ok = false;
  std::vector<ManifestCell> cells;
  std::string error;
};

struct ManifestAssessment {
  int complete_cells = 0;
  int incomplete_cells = 0;
  std::vector<std::string> missing_rows;
};

namespace {

// TBD marker: the only honest u(x) until a human supplies evidence (§11.3).
inline const char* BudgetTbdLiteral() {
  return "TBD \u2014 human input required";
}

inline std::string ReqManifestString(const nlohmann::json& obj,
                                     const char* key) {
  const nlohmann::json& val = obj.at(key);
  if (!val.is_string()) {
    throw std::runtime_error(std::string("manifest key not a string: ") + key);
  }
  return val.get<std::string>();
}

// Throws std::runtime_error on schema violation; the caller fails closed.
void FillManifestRow(const nlohmann::json& row_node, ManifestRow* row) {
  row->input = ReqManifestString(row_node, "input");
  if (!row_node.at("sensitivity_c").is_number()) {
    throw std::runtime_error("manifest sensitivity_c must be numeric");
  }
  row->c = row_node.at("sensitivity_c").get<double>();
  (void)ReqManifestString(row_node, "sensitivity_source");
  const nlohmann::json& u = row_node.at("u");
  // ponytail: string-or-number branch; any non-numeric u fails closed.
  row->u_known = u.is_number();
  (void)ReqManifestString(row_node, "u_provenance");
  (void)ReqManifestString(row_node, "distribution");
  (void)ReqManifestString(row_node, "correlation");
  if (!row_node.at("contribution").is_null()) {
    throw std::runtime_error("manifest contribution must be null");
  }
  row->status = ReqManifestString(row_node, "status");
  if (row->status != "complete" && row->status != "incomplete-missing-u" &&
      row->status != "nonlinear-route-to-mc" &&
      row->status != "below-floor-excluded") {
    throw std::runtime_error("manifest row has unknown status");
  }
}

// Throws std::runtime_error on schema violation; the caller fails closed.
void FillManifestCell(const nlohmann::json& cell_node, ManifestCell* entry) {
  entry->cell = ReqManifestString(cell_node, "cell");
  entry->channel = ReqManifestString(cell_node, "channel");
  entry->range_ft = cell_node.at("range_ft").get<int>();
  (void)cell_node.at("epsilon_num").at("value").get<double>();
  (void)ReqManifestString(cell_node.at("epsilon_num"), "source");
  (void)cell_node.at("delta_ref").at("value");
  (void)ReqManifestString(cell_node.at("delta_ref"), "source");
  if (ReqManifestString(cell_node, "eta_ref") != "unknown") {
    throw std::runtime_error("manifest eta_ref must be \"unknown\"");
  }
  const nlohmann::json& rows = cell_node.at("rows");
  if (!rows.is_array()) {
    throw std::runtime_error("manifest rows must be an array");
  }
  for (const auto& row_node : rows) {
    ManifestRow manifest{};
    FillManifestRow(row_node, &manifest);
    entry->rows.push_back(manifest);
  }
  const nlohmann::json& excluded = cell_node.at("excluded_with_floor");
  if (!excluded.is_array()) {
    throw std::runtime_error("manifest excluded_with_floor must be array");
  }
  for (const auto& excluded_node : excluded) {
    (void)ReqManifestString(excluded_node, "input");
    (void)excluded_node.at("floor").get<double>();
  }
  const nlohmann::json& nonlinear = cell_node.at("nonlinear_route_to_mc");
  if (!nonlinear.is_array()) {
    throw std::runtime_error("manifest nonlinear_route_to_mc must be array");
  }
  for (const auto& nonlinear_node : nonlinear) {
    (void)ReqManifestString(nonlinear_node, "input");
    (void)ReqManifestString(nonlinear_node, "reason");
  }
}

// Reads <dir>/budget_template.json and validates the row schema with at()
// discipline. Never throws: schema violations return {ok=false, error}.
inline ManifestLoad LoadBudgetManifest(const std::string& dir) {
  const std::string kPath = dir + "/budget_template.json";
  const std::ifstream kIn(kPath.c_str());
  if (!kIn) {
    ManifestLoad out;
    out.ok = false;
    out.error = "budget manifest file not found: " + kPath;
    return out;
  }
  std::ostringstream raw;
  raw << kIn.rdbuf();
  try {
    const nlohmann::json kRoot = nlohmann::json::parse(raw.str());
    (void)kRoot.at("artifact_schema").get<int>();
    if (ReqManifestString(kRoot, "instructions").empty()) {
      throw std::runtime_error("manifest instructions string is empty");
    }
    const nlohmann::json& cells = kRoot.at("cells");
    if (!cells.is_array() || cells.empty()) {
      throw std::runtime_error("manifest cells must be a non-empty array");
    }
    ManifestLoad out;
    out.ok = true;
    for (const auto& cell_node : cells) {
      ManifestCell entry{};
      FillManifestCell(cell_node, &entry);
      out.cells.push_back(entry);
    }
    return out;
  } catch (const nlohmann::json::parse_error& e) {
    ManifestLoad out;
    out.ok = false;
    out.error = std::string("parse error in ") + kPath + ": " + e.what();
    return out;
  } catch (const nlohmann::json::out_of_range& e) {
    ManifestLoad out;
    out.ok = false;
    out.error = std::string("schema key missing in ") + kPath + ": " + e.what();
    return out;
  } catch (const nlohmann::json::type_error& e) {
    ManifestLoad out;
    out.ok = false;
    out.error = std::string("schema type error in ") + kPath + ": " + e.what();
    return out;
  } catch (const std::runtime_error& e) {
    ManifestLoad out;
    out.ok = false;
    out.error = std::string("manifest invalid: ") + e.what();
    return out;
  }
}

// The incomplete listing IS the Phase 4 offline deliverable: every row that
// blocks a combined number is named with its reason.
inline ManifestAssessment AssessManifest(
    const std::vector<ManifestCell>& cells) {
  ManifestAssessment out;
  out.complete_cells = 0;
  out.incomplete_cells = 0;
  for (const auto& cell : cells) {
    bool complete = !cell.rows.empty();
    for (const auto& row : cell.rows) {
      const std::string kWhere =
          cell.cell + "/" + cell.channel + "/" + row.input;
      if (!row.u_known) {
        complete = false;
        out.missing_rows.push_back(kWhere + ": missing-u (TBD)");
      } else if (row.status != "complete") {
        complete = false;
        out.missing_rows.push_back(kWhere + ": status=" + row.status);
      }
    }
    if (complete) {
      ++out.complete_cells;
    } else {
      ++out.incomplete_cells;
    }
  }
  return out;
}

}  // namespace

#ifndef LOB_MANIFESTS_DIR
#define LOB_MANIFESTS_DIR "test/validation/manifests"
#endif

// 12 cell/channel entries (4 cells x 3 forward channels x 1800 ft),
// 69 genuine-driver rows counted by hand from pareto.json (plan said ~40;
// actual count is 69 — reported, not trimmed).
const int kExpectedManifestCells = 12;
const int kExpectedIncompleteRows = 69;

TEST(BudgetManifest, TemplateLoadsOk) {
  const ManifestLoad kLoad = LoadBudgetManifest(LOB_MANIFESTS_DIR);
  ASSERT_TRUE(kLoad.ok) << kLoad.error;
  EXPECT_EQ(static_cast<int>(kLoad.cells.size()), kExpectedManifestCells);
}

TEST(BudgetManifest, TemplateIsFullyIncomplete) {
  const ManifestLoad kLoad = LoadBudgetManifest(LOB_MANIFESTS_DIR);
  ASSERT_TRUE(kLoad.ok) << kLoad.error;
  const ManifestAssessment kA = AssessManifest(kLoad.cells);
  EXPECT_EQ(kA.complete_cells, 0);
  EXPECT_EQ(kA.incomplete_cells, kExpectedManifestCells);
  EXPECT_EQ(static_cast<int>(kA.missing_rows.size()), kExpectedIncompleteRows);
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
TEST(BudgetManifest, EveryRowUFailsClosedAsTbdLiteral) {
  const ManifestLoad kLoad = LoadBudgetManifest(LOB_MANIFESTS_DIR);
  ASSERT_TRUE(kLoad.ok) << kLoad.error;
  const std::string kTbd = BudgetTbdLiteral();
  // Grep-level tripwire on the raw document: no row may carry a numeric u.
  const std::ifstream kIn(std::string(LOB_MANIFESTS_DIR) +
                          "/budget_template.json");
  ASSERT_TRUE(static_cast<bool>(kIn));
  std::ostringstream raw;
  raw << kIn.rdbuf();
  const nlohmann::json kRoot = nlohmann::json::parse(raw.str());
  int tbd_rows = 0;
  const nlohmann::json& cells = kRoot.at("cells");
  for (std::size_t ci = 0; ci < cells.size(); ++ci) {
    const nlohmann::json& rows = cells.at(ci).at("rows");
    for (std::size_t ri = 0; ri < rows.size(); ++ri) {
      const nlohmann::json& u = rows.at(ri).at("u");
      ASSERT_TRUE(u.is_string())
          << "numeric u invented at cell " << ci << " row " << ri;
      EXPECT_EQ(u.get<std::string>(), kTbd);
      ++tbd_rows;
    }
  }
  EXPECT_EQ(tbd_rows, kExpectedIncompleteRows);
}

TEST(BudgetManifest, CellAliasMapsC6ShearToFloorsCell) {
  const std::ifstream kIn(std::string(LOB_MANIFESTS_DIR) +
                          "/budget_template.json");
  ASSERT_TRUE(static_cast<bool>(kIn));
  std::ostringstream raw;
  raw << kIn.rdbuf();
  const nlohmann::json kRoot = nlohmann::json::parse(raw.str());
  const std::string kFloors =
      ReqManifestString(kRoot.at("cell_aliases").at("C6-shear"), "floors_cell");
  EXPECT_EQ(kFloors, "C6-scaled");
  bool found = false;
  const nlohmann::json& cells = kRoot.at("cells");
  for (const auto& cell_node : cells) {
    const std::string kSource =
        ReqManifestString(cell_node.at("epsilon_num"), "source");
    if (kSource.find(kFloors) != std::string::npos) {
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found);
}

// ---- Task 3: CI smoke (hermetic read-only, no solver, deterministic) ----
#ifndef LOB_VALIDATION_CASES_DIR
#define LOB_VALIDATION_CASES_DIR "test/validation/cases"
#endif

TEST(BudgetSmoke, TemplateIsIncompleteAndMathHolds) {
  // Manifests resolve under the same LOB_VALIDATION_CASES_DIR tree.
  const std::string kManifestsDir =
      std::string(LOB_VALIDATION_CASES_DIR) + "/../manifests";
  const ManifestLoad kLoad = LoadBudgetManifest(kManifestsDir);
  ASSERT_TRUE(kLoad.ok) << kLoad.error;
  const ManifestAssessment kA = AssessManifest(kLoad.cells);
  EXPECT_EQ(kA.complete_cells, 0);

  const std::vector<BudgetRow> kRows = {{2.0, 3.0, true, false, 0.0},
                                        {1.0, 4.0, true, false, 0.0}};
  const BudgetResult kR = CombineBudget(kRows);
  ASSERT_TRUE(kR.complete);
  EXPECT_NEAR(kR.u_c, std::sqrt(52.0), 1e-12);

  const std::ifstream kIn((kManifestsDir + "/budget_template.json").c_str());
  ASSERT_TRUE(static_cast<bool>(kIn));
  std::ostringstream raw;
  raw << kIn.rdbuf();
  const nlohmann::json kRoot = nlohmann::json::parse(raw.str());
  const std::string kInstructions = ReqManifestString(kRoot, "instructions");
  EXPECT_FALSE(kInstructions.empty());
  EXPECT_NE(kInstructions.find("human"), std::string::npos);
}

// ---- Task 4: assembler (always runs; envelope recomputed when stale) ----
#ifndef LOB_VALIDATION_DIR
#error "LOB_VALIDATION_DIR must be defined by CMake"
#endif
#ifndef LOB_GIT_SHA
#error "LOB_GIT_SHA must be defined by CMake"
#endif

namespace {

// Reads a JSON artifact file with try/catch parse; never throws.
inline bool BudgetReadJson(const std::string& path, nlohmann::json* out,
                           std::string* error) {
  const std::ifstream kIn(path.c_str());
  if (!kIn) {
    *error = "artifact file not found: " + path;
    return false;
  }
  std::ostringstream raw;
  raw << kIn.rdbuf();
  try {
    *out = nlohmann::json::parse(raw.str());
  } catch (const nlohmann::json::exception& e) {
    *error = std::string("parse error in ") + path + ": " + e.what();
    return false;
  }
  return true;
}

// Manifest short channel vs the floors/envelope long key.
inline std::string BudgetFloorKey(const std::string& channel) {
  if (channel == "tof_s") {
    return "time_of_flight_s";
  }
  return channel;
}

// raw_deriv lookup for a single driver list. False when the input is absent
// or non-numeric — the caller FAILs loudly (template drift breaks).
bool BudgetDriverDeriv(const nlohmann::json& drivers, const std::string& input,
                       double* deriv) {
  for (const auto& driver : drivers) {
    if (driver.at("input").get<std::string>() != input) {
      continue;
    }
    const nlohmann::json& raw = driver.at("raw_deriv");
    if (!raw.is_number()) {
      return false;
    }
    *deriv = raw.get<double>();
    return true;
  }
  return false;
}

bool BudgetRangeDeriv(const nlohmann::json& ranges, int range_ft,
                      const std::string& input, double* deriv) {
  for (const auto& range : ranges) {
    if (range.at("range_ft").get<int>() != range_ft) {
      continue;
    }
    return BudgetDriverDeriv(range.at("drivers"), input, deriv);
  }
  return false;
}

bool BudgetChannelDeriv(const nlohmann::json& channels,
                        const std::string& channel, int range_ft,
                        const std::string& input, double* deriv) {
  for (const auto& channel_node : channels) {
    if (channel_node.at("output").get<std::string>() != channel) {
      continue;
    }
    return BudgetRangeDeriv(channel_node.at("ranges"), range_ft, input, deriv);
  }
  return false;
}

// raw_deriv lookup for cell/channel@range/input in pareto.json. False when
// absent or non-numeric — the caller FAILs loudly (template drift breaks).
inline bool BudgetParetoDeriv(const nlohmann::json& pareto,
                              const std::string& cell,
                              const std::string& channel, int range_ft,
                              const std::string& input, double* deriv) {
  try {
    const nlohmann::json& cells = pareto.at("cells");
    if (!cells.is_array()) {
      return false;
    }
    for (const auto& cell_node : cells) {
      if (cell_node.at("cell").get<std::string>() != cell) {
        continue;
      }
      return BudgetChannelDeriv(cell_node.at("channels"), channel, range_ft,
                                input, deriv);
    }
  } catch (const nlohmann::json::exception&) {
    return false;
  }
  return false;
}

// floors.json value for an aliased cell (C6-shear reads C6-scaled).
inline bool BudgetFloorValue(const nlohmann::json& floors,
                             const std::string& floors_cell,
                             const std::string& channel, double* value) {
  try {
    const nlohmann::json& cells = floors.at("cells");
    if (!cells.is_array()) {
      return false;
    }
    for (const auto& cell_node : cells) {
      if (cell_node.at("cell").get<std::string>() != floors_cell) {
        continue;
      }
      const nlohmann::json& floor_value =
          cell_node.at("floors_18_9").at(BudgetFloorKey(channel));
      if (!floor_value.is_number()) {
        return false;
      }
      *value = floor_value.get<double>();
      return true;
    }
  } catch (const nlohmann::json::exception&) {
    return false;
  }
  return false;
}

// Max across FullMatrix cells of worst_residual[key] (ordered max on
// doubles — exact, no tolerance involved).
inline bool BudgetEnvelopeWorst(const nlohmann::json& envelope,
                                const std::string& channel, double* worst) {
  try {
    const std::string kKey = BudgetFloorKey(channel);
    const nlohmann::json& cells = envelope.at("cells");
    if (!cells.is_array() || cells.empty()) {
      return false;
    }
    bool seen = false;
    double peak = 0.0;
    for (const auto& cell_node : cells) {
      const nlohmann::json& worst_node =
          cell_node.at("worst_residual").at(kKey);
      if (!worst_node.is_number()) {
        return false;
      }
      const double kV = worst_node.get<double>();
      if (!seen || kV > peak) {
        peak = kV;
        seen = true;
      }
    }
    *worst = peak;
    return seen;
  } catch (const nlohmann::json::exception&) {
    return false;
  }
}

// DriftEq: combined absolute/relative check for live-recomputed values vs
// checked-in template literals. |a-b| <= atol + 1e-6*max(|a|,|b|); the fixed
// 1e-6 relative term admits ~1e-7 cross-platform libm noise with 10x margin
// while genuine staleness moves values far beyond it. atol is the channel's
// epsilon floor already in scope at the call site.
inline ::testing::AssertionResult DriftEq(double actual, double expected,
                                          double atol) {
  constexpr double kRtol = 1e-6;
  constexpr int kDoubleRoundTripDigits = 17;
  const double kDiff = std::fabs(actual - expected);
  const double kRtolTerm =
      kRtol * std::max(std::fabs(actual), std::fabs(expected));
  if (kDiff <= atol + kRtolTerm) {
    return ::testing::AssertionSuccess();
  }
  std::ostringstream os;
  os << "drift: actual=" << std::setprecision(kDoubleRoundTripDigits) << actual
     << " expected=" << expected << " abs-diff=" << kDiff << " admitted by "
     << (atol >= kRtolTerm ? "atol" : "rtol") << " term"
     << " (atol=" << atol << " rtol-term=" << kRtolTerm << ")";
  return ::testing::AssertionFailure() << os.str();
}

}  // namespace

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
TEST(BudgetAssemble, OfflineDocuments) {
  constexpr int kJsonPrecisionDigits = 17;
  constexpr std::size_t kTrailingAssumptionCount = 3;
  const std::string kManifestsDir =
      std::string(LOB_VALIDATION_CASES_DIR) + "/../manifests";
  const std::string kBaselinesDir =
      std::string(LOB_VALIDATION_CASES_DIR) + "/../baselines";
  const ManifestLoad kLoad = LoadBudgetManifest(kManifestsDir);
  ASSERT_TRUE(kLoad.ok) << kLoad.error;
  const ManifestAssessment kA = AssessManifest(kLoad.cells);
  EXPECT_EQ(kA.complete_cells, 0);
  EXPECT_EQ(kA.incomplete_cells, kExpectedManifestCells);

  nlohmann::json tmpl;
  std::string error;
  ASSERT_TRUE(
      BudgetReadJson(kManifestsDir + "/budget_template.json", &tmpl, &error))
      << error;
  nlohmann::json pareto;
  ASSERT_TRUE(BudgetReadJson(kBaselinesDir + "/pareto.json", &pareto, &error))
      << error;
  nlohmann::json floors;
  ASSERT_TRUE(BudgetReadJson(kBaselinesDir + "/floors.json", &floors, &error))
      << error;
  nlohmann::json envelope;
  // Always recomputed in-process: no test reads another test's output
  // file, so no cross-test ordering or torn-read window can exist. The
  // matrix test's envelope_report.json remains as human-readable
  // diagnostics only.
  EnvelopeWorst worst{};
  ASSERT_TRUE(TryComputeEnvelopeWorst(std::string(LOB_VALIDATION_CASES_DIR),
                                      &worst, &error))
      << error;
  nlohmann::json residual;
  residual["elevation_in"] = worst.elev_in;
  residual["elevation_moa"] = worst.elev_moa;
  residual["deflection_moa"] = worst.defl_moa;
  residual["velocity_fps"] = worst.vel;
  residual["energy_ft_lbf"] = worst.energy;
  residual["time_of_flight_s"] = worst.tof;
  nlohmann::json worst_cell;
  worst_cell["worst_residual"] = residual;
  envelope["provenance"] = {{"git_sha", LOB_GIT_SHA}};
  envelope["cells"] = nlohmann::json::array({worst_cell});

  int docs = 0;
  int emitted_uc = 0;
  const nlohmann::json& cells = tmpl.at("cells");
  ASSERT_EQ(cells.size(), kLoad.cells.size());
  for (std::size_t ci = 0; ci < cells.size(); ++ci) {
    const nlohmann::json& cell = cells.at(ci);
    const std::string kName = ReqManifestString(cell, "cell");
    const std::string kChannel = ReqManifestString(cell, "channel");
    const int kRange = cell.at("range_ft").get<int>();
    ASSERT_EQ(kName, kLoad.cells.at(ci).cell);
    ASSERT_EQ(kChannel, kLoad.cells.at(ci).channel);

    // Floors alias: C6-shear owns no floors cell; it reuses C6-scaled.
    std::string floors_cell = kName;
    if (tmpl.at("cell_aliases").count(kName) > 0) {
      floors_cell =
          ReqManifestString(tmpl.at("cell_aliases").at(kName), "floors_cell");
    }

    const double kEpsTemplate =
        cell.at("epsilon_num").at("value").get<double>();
    double eps_live = 0.0;
    ASSERT_TRUE(BudgetFloorValue(floors, floors_cell, kChannel, &eps_live))
        << "floors.json has no " << floors_cell << "/" << kChannel;
    ASSERT_DOUBLE_EQ(kEpsTemplate, eps_live)
        << "epsilon drift: " << kName << "/" << kChannel;
    const double kDeltaTemplate =
        cell.at("delta_ref").at("value").get<double>();
    double delta_live = 0.0;
    ASSERT_TRUE(BudgetEnvelopeWorst(envelope, kChannel, &delta_live))
        << "recomputed envelope has no " << kChannel;
    ASSERT_TRUE(DriftEq(delta_live, kDeltaTemplate, eps_live))
        << "delta drift: " << kName << "/" << kChannel;

    // Sensitivity cross-check: every template c must equal the live
    // pareto.json raw_deriv. Mismatch FAILs loudly — drift breaks.
    const nlohmann::json& rows = cell.at("rows");
    std::vector<BudgetRow> budget_rows;
    for (const auto& row_node : rows) {
      const std::string kInput = ReqManifestString(row_node, "input");
      ASSERT_TRUE(row_node.at("sensitivity_c").is_number())
          << "non-numeric sensitivity_c: " << kName << "/" << kChannel << "/"
          << kInput;
      const double kC = row_node.at("sensitivity_c").get<double>();
      double live = 0.0;
      ASSERT_TRUE(
          BudgetParetoDeriv(pareto, kName, kChannel, kRange, kInput, &live))
          << "pareto.json has no driver " << kName << "/" << kChannel << "@"
          << kRange << "/" << kInput;
      ASSERT_DOUBLE_EQ(kC, live)
          << "template drift: " << kName << "/" << kChannel << "/" << kInput;
      BudgetRow b{};
      b.c = kC;
      b.u = 0.0;
      b.u_known = row_node.at("u").is_number();
      b.nonlinear =
          ReqManifestString(row_node, "status") == "nonlinear-route-to-mc";
      b.floor_value = 0.0;
      budget_rows.push_back(b);
    }

    // (a) BudgetCovariance i,j are unchecked indices by design; this
    // assembler never constructs covariance pairs (no correlation evidence
    // in Phase 4 — every row is assumed-independent), so there is no i,j
    // indexing to bounds-check. CombineBudget runs on the empty cov list.
    // Empty row vectors would combine to u_c=0 — fail closed instead.
    BudgetResult res;
    if (budget_rows.empty()) {
      res.complete = false;
      res.u_c = std::numeric_limits<double>::quiet_NaN();
      res.status = "incomplete-no-genuine-drivers";
    } else {
      res = CombineBudget(budget_rows);
    }

    // (b) Zero-row cells (C6 elev/tof) name themselves here so the offline
    // deliverable names every blocked cell, not just blocked rows.
    std::vector<std::string> missing;
    if (budget_rows.empty()) {
      std::ostringstream where;
      where << kName << "/" << kChannel << ": no-genuine-drivers";
      missing.push_back(where.str());
    } else {
      for (std::size_t ri = 0; ri < rows.size(); ++ri) {
        std::ostringstream where;
        where << kName << "/" << kChannel << "/"
              << ReqManifestString(rows.at(ri), "input");
        const std::string kWhere = where.str();
        if (!budget_rows.at(ri).u_known) {
          missing.push_back(kWhere + ": missing-u (TBD)");
        } else if (ReqManifestString(rows.at(ri), "status") != "complete") {
          missing.push_back(
              kWhere + ": status=" + ReqManifestString(rows.at(ri), "status"));
        }
      }
    }
    ASSERT_FALSE(missing.empty())
        << "manifest completed without human review — see §21 item 7: " << kName
        << "/" << kChannel;

    std::vector<std::string> assumptions;
    assumptions.reserve(rows.size() + kTrailingAssumptionCount);
    for (const auto& row_node : rows) {
      assumptions.push_back(
          ReqManifestString(row_node, "input") +
          ": correlation=" + ReqManifestString(row_node, "correlation") +
          "; distribution=" + ReqManifestString(row_node, "distribution") +
          " (template default per §12.1, human must confirm); u unknown");
    }
    {
      std::ostringstream os;
      os << "epsilon_num reused from floors.json " << floors_cell
         << " floors_18_9 " << BudgetFloorKey(kChannel) << "="
         << std::setprecision(kJsonPrecisionDigits) << eps_live;
      assumptions.push_back(os.str());
    }
    {
      std::ostringstream os;
      os << "delta_ref recomputed in-process (same FullMatrix computation) "
         << BudgetFloorKey(kChannel) << "="
         << std::setprecision(kJsonPrecisionDigits) << delta_live
         << " (max across FullMatrix cells)";
      assumptions.push_back(os.str());
    }
    assumptions.emplace_back("eta_ref unknown: no eta evidence in Phase 4");

    nlohmann::json doc;
    doc["provenance"] = {{"git_sha", LOB_GIT_SHA},
                         {"generator", "BudgetAssemble.OfflineDocuments"},
                         {"gate", "none (always run)"},
                         {"cell", kName},
                         {"channel", kChannel},
                         {"range_ft", kRange}};
    doc["cell"] = kName;
    doc["channel"] = kChannel;
    doc["range_ft"] = kRange;
    doc["rows"] = nlohmann::json::array();
    for (const auto& row_node : rows) {
      nlohmann::json entry;
      entry["input"] = ReqManifestString(row_node, "input");
      entry["sensitivity_c"] = row_node.at("sensitivity_c");
      entry["sensitivity_source"] =
          ReqManifestString(row_node, "sensitivity_source");
      entry["u"] = nullptr;
      entry["u_provenance"] = ReqManifestString(row_node, "u_provenance");
      entry["distribution"] = ReqManifestString(row_node, "distribution");
      entry["correlation"] = ReqManifestString(row_node, "correlation");
      entry["contribution"] = nullptr;
      entry["status"] = ReqManifestString(row_node, "status");
      doc["rows"].push_back(entry);
    }
    doc["excluded_with_floor"] = cell.at("excluded_with_floor");
    doc["nonlinear_route_to_mc"] = cell.at("nonlinear_route_to_mc");
    doc["epsilon_num"] = cell.at("epsilon_num");
    doc["delta_ref"] = cell.at("delta_ref");
    doc["eta_ref"] = "unknown";
    doc["missing_u"] = missing;
    doc["assumptions"] = assumptions;
    doc["verdict"] = res.status;
    if (res.complete) {
      doc["u_c_or_absent"] = res.u_c;
      ++emitted_uc;
    } else {
      doc["u_c_or_absent"] = nullptr;
    }

    std::ostringstream path;
    path << LOB_VALIDATION_DIR << "/budget_" << kName << "_" << kChannel
         << ".json";
    const std::string kPath = path.str();
    std::ofstream out(kPath.c_str());
    ASSERT_TRUE(out.good()) << "cannot open " << kPath;
    out << doc.dump(2) << "\n";
    out.close();
    ++docs;
    std::cout << "BUDGET " << kName << " " << kChannel
              << " rows=" << rows.size() << " missing=" << missing.size()
              << " verdict=" << res.status << "\n";
  }
  EXPECT_EQ(docs, kExpectedManifestCells);
  ASSERT_EQ(emitted_uc, 0);
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
