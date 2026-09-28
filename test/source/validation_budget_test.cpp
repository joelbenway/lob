// Copyright (c) 2026  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace tests {

struct BudgetRow {
  double c;
  double u;
  bool u_known;
  bool nonlinear;
  double floor_value;
};

struct BudgetCovariance {
  std::size_t i;
  std::size_t j;
  double cov;
};

struct BudgetResult {
  bool complete;
  double u_c;
  std::string status;
};

inline BudgetResult CombineBudget(
    const std::vector<BudgetRow>& rows,
    const std::vector<BudgetCovariance>& covs = std::vector<BudgetCovariance>()) {
  for (const BudgetRow& r : rows) {
    if (r.nonlinear) {
      const BudgetResult kOut = {false,
                                 std::numeric_limits<double>::quiet_NaN(),
                                 "route_to_mc"};
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
  for (const BudgetCovariance& kCov : covs) {
    sum += 2.0 * kCov.cov;
  }
  const BudgetResult kOut = {true, std::sqrt(sum), "complete"};
  return kOut;
}

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

TEST(BudgetMath, UnknownU_FailsClosedWithNaN) {
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

// TBD marker: the only honest u(x) until a human supplies evidence (§11.3).
inline const char* BudgetTbdLiteral() { return "TBD \u2014 human input required"; }

struct ManifestRow {
  std::string input;
  double c;
  bool u_known;
  std::string status;
};

struct ManifestCell {
  std::string cell;
  std::string channel;
  int range_ft;
  std::vector<ManifestRow> rows;
};

struct ManifestLoad {
  bool ok;
  std::vector<ManifestCell> cells;
  std::string error;
};

struct ManifestAssessment {
  int complete_cells;
  int incomplete_cells;
  std::vector<std::string> missing_rows;
};

inline std::string ReqManifestString(const nlohmann::json& obj,
                                     const char* key) {
  const nlohmann::json& kVal = obj.at(key);
  if (!kVal.is_string()) {
    throw std::runtime_error(std::string("manifest key not a string: ") + key);
  }
  return kVal.get<std::string>();
}

// Reads <dir>/budget_template.json and validates the row schema with at()
// discipline. Never throws: schema violations return {ok=false, error}.
inline ManifestLoad LoadBudgetManifest(const std::string& dir) {
  const std::string kPath = dir + "/budget_template.json";
  std::ifstream in(kPath.c_str());
  if (!in) {
    ManifestLoad kOut;
    kOut.ok = false;
    kOut.error = "budget manifest file not found: " + kPath;
    return kOut;
  }
  std::ostringstream raw;
  raw << in.rdbuf();
  try {
    const nlohmann::json kRoot = nlohmann::json::parse(raw.str());
    (void)kRoot.at("artifact_schema").get<int>();
    if (ReqManifestString(kRoot, "instructions").empty()) {
      throw std::runtime_error("manifest instructions string is empty");
    }
    const nlohmann::json& kCells = kRoot.at("cells");
    if (!kCells.is_array() || kCells.empty()) {
      throw std::runtime_error("manifest cells must be a non-empty array");
    }
    ManifestLoad kOut;
    kOut.ok = true;
    for (std::size_t ci = 0; ci < kCells.size(); ++ci) {
      const nlohmann::json& kCell = kCells.at(ci);
      ManifestCell kEntry;
      kEntry.cell = ReqManifestString(kCell, "cell");
      kEntry.channel = ReqManifestString(kCell, "channel");
      kEntry.range_ft = kCell.at("range_ft").get<int>();
      (void)kCell.at("epsilon_num").at("value").get<double>();
      (void)ReqManifestString(kCell.at("epsilon_num"), "source");
      (void)kCell.at("delta_ref").at("value");
      (void)ReqManifestString(kCell.at("delta_ref"), "source");
      if (ReqManifestString(kCell, "eta_ref") != "unknown") {
        throw std::runtime_error("manifest eta_ref must be \"unknown\"");
      }
      const nlohmann::json& kRows = kCell.at("rows");
      if (!kRows.is_array()) {
        throw std::runtime_error("manifest rows must be an array");
      }
      for (std::size_t ri = 0; ri < kRows.size(); ++ri) {
        const nlohmann::json& kRow = kRows.at(ri);
        ManifestRow kManifest;
        kManifest.input = ReqManifestString(kRow, "input");
        if (!kRow.at("sensitivity_c").is_number()) {
          throw std::runtime_error("manifest sensitivity_c must be numeric");
        }
        kManifest.c = kRow.at("sensitivity_c").get<double>();
        (void)ReqManifestString(kRow, "sensitivity_source");
        const nlohmann::json& kU = kRow.at("u");
        // ponytail: string-or-number branch; any non-numeric u fails closed.
        kManifest.u_known = kU.is_number();
        (void)ReqManifestString(kRow, "u_provenance");
        (void)ReqManifestString(kRow, "distribution");
        (void)ReqManifestString(kRow, "correlation");
        if (!kRow.at("contribution").is_null()) {
          throw std::runtime_error("manifest contribution must be null");
        }
        kManifest.status = ReqManifestString(kRow, "status");
        if (kManifest.status != "complete" &&
            kManifest.status != "incomplete-missing-u" &&
            kManifest.status != "nonlinear-route-to-mc" &&
            kManifest.status != "below-floor-excluded") {
          throw std::runtime_error("manifest row has unknown status");
        }
        kEntry.rows.push_back(kManifest);
      }
      const nlohmann::json& kExcluded = kCell.at("excluded_with_floor");
      if (!kExcluded.is_array()) {
        throw std::runtime_error("manifest excluded_with_floor must be array");
      }
      for (std::size_t ei = 0; ei < kExcluded.size(); ++ei) {
        (void)ReqManifestString(kExcluded.at(ei), "input");
        (void)kExcluded.at(ei).at("floor").get<double>();
      }
      const nlohmann::json& kNonlinear = kCell.at("nonlinear_route_to_mc");
      if (!kNonlinear.is_array()) {
        throw std::runtime_error("manifest nonlinear_route_to_mc must be array");
      }
      for (std::size_t ni = 0; ni < kNonlinear.size(); ++ni) {
        (void)ReqManifestString(kNonlinear.at(ni), "input");
        (void)ReqManifestString(kNonlinear.at(ni), "reason");
      }
      kOut.cells.push_back(kEntry);
    }
    return kOut;
  } catch (const nlohmann::json::parse_error& e) {
    ManifestLoad kOut;
    kOut.ok = false;
    kOut.error = std::string("parse error in ") + kPath + ": " + e.what();
    return kOut;
  } catch (const nlohmann::json::out_of_range& e) {
    ManifestLoad kOut;
    kOut.ok = false;
    kOut.error = std::string("schema key missing in ") + kPath + ": " + e.what();
    return kOut;
  } catch (const nlohmann::json::type_error& e) {
    ManifestLoad kOut;
    kOut.ok = false;
    kOut.error = std::string("schema type error in ") + kPath + ": " + e.what();
    return kOut;
  } catch (const std::runtime_error& e) {
    ManifestLoad kOut;
    kOut.ok = false;
    kOut.error = std::string("manifest invalid: ") + e.what();
    return kOut;
  }
}

// The incomplete listing IS the Phase 4 offline deliverable: every row that
// blocks a combined number is named with its reason.
inline ManifestAssessment AssessManifest(
    const std::vector<ManifestCell>& cells) {
  ManifestAssessment kOut;
  kOut.complete_cells = 0;
  kOut.incomplete_cells = 0;
  for (std::size_t ci = 0; ci < cells.size(); ++ci) {
    const ManifestCell& kCell = cells[ci];
    bool complete = !kCell.rows.empty();
    for (std::size_t ri = 0; ri < kCell.rows.size(); ++ri) {
      const ManifestRow& kRow = kCell.rows[ri];
      const std::string kWhere =
          kCell.cell + "/" + kCell.channel + "/" + kRow.input;
      if (!kRow.u_known) {
        complete = false;
        kOut.missing_rows.push_back(kWhere + ": missing-u (TBD)");
      } else if (kRow.status != "complete") {
        complete = false;
        kOut.missing_rows.push_back(kWhere + ": status=" + kRow.status);
      }
    }
    if (complete) {
      ++kOut.complete_cells;
    } else {
      ++kOut.incomplete_cells;
    }
  }
  return kOut;
}

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
  EXPECT_EQ(static_cast<int>(kA.missing_rows.size()),
            kExpectedIncompleteRows);
}

TEST(BudgetManifest, EveryRowUFailsClosedAsTbdLiteral) {
  const ManifestLoad kLoad = LoadBudgetManifest(LOB_MANIFESTS_DIR);
  ASSERT_TRUE(kLoad.ok) << kLoad.error;
  const std::string kTbd = BudgetTbdLiteral();
  // Grep-level tripwire on the raw document: no row may carry a numeric u.
  std::ifstream in(std::string(LOB_MANIFESTS_DIR) + "/budget_template.json");
  ASSERT_TRUE(static_cast<bool>(in));
  std::ostringstream raw;
  raw << in.rdbuf();
  const nlohmann::json kRoot = nlohmann::json::parse(raw.str());
  int tbd_rows = 0;
  const nlohmann::json& kCells = kRoot.at("cells");
  for (std::size_t ci = 0; ci < kCells.size(); ++ci) {
    const nlohmann::json& kRows = kCells.at(ci).at("rows");
    for (std::size_t ri = 0; ri < kRows.size(); ++ri) {
      const nlohmann::json& kU = kRows.at(ri).at("u");
      ASSERT_TRUE(kU.is_string())
          << "numeric u invented at cell " << ci << " row " << ri;
      EXPECT_EQ(kU.get<std::string>(), kTbd);
      ++tbd_rows;
    }
  }
  EXPECT_EQ(tbd_rows, kExpectedIncompleteRows);
}

TEST(BudgetManifest, CellAliasMapsC6ShearToFloorsCell) {
  std::ifstream in(std::string(LOB_MANIFESTS_DIR) + "/budget_template.json");
  ASSERT_TRUE(static_cast<bool>(in));
  std::ostringstream raw;
  raw << in.rdbuf();
  const nlohmann::json kRoot = nlohmann::json::parse(raw.str());
  const std::string kFloors =
      ReqManifestString(kRoot.at("cell_aliases").at("C6-shear"), "floors_cell");
  EXPECT_EQ(kFloors, "C6-scaled");
  bool found = false;
  const nlohmann::json& kCells = kRoot.at("cells");
  for (std::size_t ci = 0; ci < kCells.size(); ++ci) {
    const std::string kSource =
        ReqManifestString(kCells.at(ci).at("epsilon_num"), "source");
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

  std::ifstream in((kManifestsDir + "/budget_template.json").c_str());
  ASSERT_TRUE(static_cast<bool>(in));
  std::ostringstream raw;
  raw << in.rdbuf();
  const nlohmann::json kRoot = nlohmann::json::parse(raw.str());
  const std::string kInstructions = ReqManifestString(kRoot, "instructions");
  EXPECT_FALSE(kInstructions.empty());
  EXPECT_NE(kInstructions.find("human"), std::string::npos);
}

// ---- Task 4: offline assembler (env-gated, LOB_FULL_BUDGET=1) ----
#ifndef LOB_VALIDATION_DIR
#error "LOB_VALIDATION_DIR must be defined by CMake"
#endif
#ifndef LOB_GIT_SHA
#error "LOB_GIT_SHA must be defined by CMake"
#endif

inline bool BudgetAssembleGated() {
  const char* kGate = std::getenv("LOB_FULL_BUDGET");
  return kGate != nullptr && std::string(kGate) == "1";
}

// Reads a JSON artifact file with try/catch parse; never throws.
inline bool BudgetReadJson(const std::string& path, nlohmann::json* out,
                           std::string* error) {
  std::ifstream in(path.c_str());
  if (!in) {
    *error = "artifact file not found: " + path;
    return false;
  }
  std::ostringstream raw;
  raw << in.rdbuf();
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

// raw_deriv lookup for cell/channel@range/input in pareto.json. False when
// absent or non-numeric — the caller FAILs loudly (template drift breaks).
inline bool BudgetParetoDeriv(const nlohmann::json& pareto,
                              const std::string& cell,
                              const std::string& channel, int range_ft,
                              const std::string& input, double* deriv) {
  try {
    const nlohmann::json& kCells = pareto.at("cells");
    if (!kCells.is_array()) {
      return false;
    }
    for (std::size_t ci = 0; ci < kCells.size(); ++ci) {
      if (kCells.at(ci).at("cell").get<std::string>() != cell) {
        continue;
      }
      const nlohmann::json& kChannels = kCells.at(ci).at("channels");
      for (std::size_t hi = 0; hi < kChannels.size(); ++hi) {
        if (kChannels.at(hi).at("output").get<std::string>() != channel) {
          continue;
        }
        const nlohmann::json& kRanges = kChannels.at(hi).at("ranges");
        for (std::size_t ri = 0; ri < kRanges.size(); ++ri) {
          if (kRanges.at(ri).at("range_ft").get<int>() != range_ft) {
            continue;
          }
          const nlohmann::json& kDrivers = kRanges.at(ri).at("drivers");
          for (std::size_t di = 0; di < kDrivers.size(); ++di) {
            if (kDrivers.at(di).at("input").get<std::string>() != input) {
              continue;
            }
            const nlohmann::json& kRaw = kDrivers.at(di).at("raw_deriv");
            if (!kRaw.is_number()) {
              return false;
            }
            *deriv = kRaw.get<double>();
            return true;
          }
          return false;
        }
        return false;
      }
      return false;
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
    const nlohmann::json& kCells = floors.at("cells");
    if (!kCells.is_array()) {
      return false;
    }
    for (std::size_t ci = 0; ci < kCells.size(); ++ci) {
      if (kCells.at(ci).at("cell").get<std::string>() != floors_cell) {
        continue;
      }
      const nlohmann::json& kF =
          kCells.at(ci).at("floors_18_9").at(BudgetFloorKey(channel));
      if (!kF.is_number()) {
        return false;
      }
      *value = kF.get<double>();
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
    const nlohmann::json& kCells = envelope.at("cells");
    if (!kCells.is_array() || kCells.empty()) {
      return false;
    }
    bool seen = false;
    double peak = 0.0;
    for (std::size_t ci = 0; ci < kCells.size(); ++ci) {
      const nlohmann::json& kW =
          kCells.at(ci).at("worst_residual").at(kKey);
      if (!kW.is_number()) {
        return false;
      }
      const double kV = kW.get<double>();
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

TEST(BudgetAssemble, OfflineDocuments) {
  if (!BudgetAssembleGated()) {
    GTEST_SKIP() << "offline only: set LOB_FULL_BUDGET=1";
  }
  const std::string kManifestsDir =
      std::string(LOB_VALIDATION_CASES_DIR) + "/../manifests";
  const std::string kBaselinesDir =
      std::string(LOB_VALIDATION_CASES_DIR) + "/../baselines";
  const ManifestLoad kLoad = LoadBudgetManifest(kManifestsDir);
  ASSERT_TRUE(kLoad.ok) << kLoad.error;
  const ManifestAssessment kA = AssessManifest(kLoad.cells);
  EXPECT_EQ(kA.complete_cells, 0);
  EXPECT_EQ(kA.incomplete_cells, kExpectedManifestCells);

  nlohmann::json kTemplate;
  std::string kError;
  ASSERT_TRUE(BudgetReadJson(kManifestsDir + "/budget_template.json",
                             &kTemplate, &kError))
      << kError;
  nlohmann::json kPareto;
  ASSERT_TRUE(BudgetReadJson(kBaselinesDir + "/pareto.json", &kPareto,
                             &kError))
      << kError;
  nlohmann::json kFloors;
  ASSERT_TRUE(BudgetReadJson(kBaselinesDir + "/floors.json", &kFloors,
                             &kError))
      << kError;
  nlohmann::json kEnvelope;
  ASSERT_TRUE(BudgetReadJson(
      std::string(LOB_VALIDATION_DIR) + "/envelope_report.json", &kEnvelope,
      &kError))
      << kError << " (run ReferenceMatrix.FullMatrix first: LOB_FULL_MATRIX=1)";

  int docs = 0;
  int emitted_uc = 0;
  const nlohmann::json& kCells = kTemplate.at("cells");
  ASSERT_EQ(kCells.size(), kLoad.cells.size());
  for (std::size_t ci = 0; ci < kCells.size(); ++ci) {
    const nlohmann::json& kCell = kCells.at(ci);
    const std::string kName = ReqManifestString(kCell, "cell");
    const std::string kChannel = ReqManifestString(kCell, "channel");
    const int kRange = kCell.at("range_ft").get<int>();
    ASSERT_EQ(kName, kLoad.cells[ci].cell);
    ASSERT_EQ(kChannel, kLoad.cells[ci].channel);

    // Floors alias: C6-shear owns no floors cell; it reuses C6-scaled.
    std::string kFloorsCell = kName;
    if (kTemplate.at("cell_aliases").count(kName) > 0) {
      kFloorsCell = ReqManifestString(
          kTemplate.at("cell_aliases").at(kName), "floors_cell");
    }

    const double kEpsTemplate = kCell.at("epsilon_num").at("value").get<double>();
    double kEpsLive = 0.0;
    ASSERT_TRUE(BudgetFloorValue(kFloors, kFloorsCell, kChannel, &kEpsLive))
        << "floors.json has no " << kFloorsCell << "/" << kChannel;
    EXPECT_DOUBLE_EQ(kEpsTemplate, kEpsLive)
        << "epsilon drift: " << kName << "/" << kChannel;
    const double kDeltaTemplate = kCell.at("delta_ref").at("value").get<double>();
    double kDeltaLive = 0.0;
    ASSERT_TRUE(BudgetEnvelopeWorst(kEnvelope, kChannel, &kDeltaLive))
        << "envelope_report.json has no " << kChannel;
    EXPECT_DOUBLE_EQ(kDeltaTemplate, kDeltaLive)
        << "delta drift: " << kName << "/" << kChannel;

    // Sensitivity cross-check: every template c must equal the live
    // pareto.json raw_deriv. Mismatch FAILs loudly — drift breaks.
    const nlohmann::json& kRows = kCell.at("rows");
    std::vector<BudgetRow> kBudgetRows;
    for (std::size_t ri = 0; ri < kRows.size(); ++ri) {
      const nlohmann::json& kRow = kRows.at(ri);
      const std::string kInput = ReqManifestString(kRow, "input");
      ASSERT_TRUE(kRow.at("sensitivity_c").is_number())
          << "non-numeric sensitivity_c: " << kName << "/" << kChannel
          << "/" << kInput;
      const double kC = kRow.at("sensitivity_c").get<double>();
      double kLive = 0.0;
      ASSERT_TRUE(
          BudgetParetoDeriv(kPareto, kName, kChannel, kRange, kInput, &kLive))
          << "pareto.json has no driver " << kName << "/" << kChannel
          << "@" << kRange << "/" << kInput;
      ASSERT_DOUBLE_EQ(kC, kLive) << "template drift: " << kName << "/"
                                  << kChannel << "/" << kInput;
      BudgetRow kB;
      kB.c = kC;
      kB.u = 0.0;
      kB.u_known = kRow.at("u").is_number();
      kB.nonlinear =
          ReqManifestString(kRow, "status") == "nonlinear-route-to-mc";
      kB.floor_value = 0.0;
      kBudgetRows.push_back(kB);
    }

    // (a) BudgetCovariance i,j are unchecked indices by design; this
    // assembler never constructs covariance pairs (no correlation evidence
    // in Phase 4 — every row is assumed-independent), so there is no i,j
    // indexing to bounds-check. CombineBudget runs on the empty cov list.
    // Empty row vectors would combine to u_c=0 — fail closed instead.
    BudgetResult kRes;
    if (kBudgetRows.empty()) {
      kRes.complete = false;
      kRes.u_c = std::numeric_limits<double>::quiet_NaN();
      kRes.status = "incomplete-no-genuine-drivers";
    } else {
      kRes = CombineBudget(kBudgetRows);
    }

    // (b) Zero-row cells (C6 elev/tof) name themselves here so the offline
    // deliverable names every blocked cell, not just blocked rows.
    std::vector<std::string> kMissing;
    if (kBudgetRows.empty()) {
      kMissing.push_back(kName + "/" + kChannel + ": no-genuine-drivers");
    } else {
      for (std::size_t ri = 0; ri < kRows.size(); ++ri) {
        const std::string kWhere = kName + "/" + kChannel + "/" +
                                   ReqManifestString(kRows.at(ri), "input");
        if (!kBudgetRows[ri].u_known) {
          kMissing.push_back(kWhere + ": missing-u (TBD)");
        } else if (ReqManifestString(kRows.at(ri), "status") != "complete") {
          kMissing.push_back(kWhere + ": status=" +
                              ReqManifestString(kRows.at(ri), "status"));
        }
      }
    }
    ASSERT_FALSE(kMissing.empty())
        << "manifest completed without human review — see §21 item 7: "
        << kName << "/" << kChannel;

    std::vector<std::string> kAssumptions;
    for (std::size_t ri = 0; ri < kRows.size(); ++ri) {
      kAssumptions.push_back(
          ReqManifestString(kRows.at(ri), "input") + ": correlation=" +
          ReqManifestString(kRows.at(ri), "correlation") +
          "; distribution=" +
          ReqManifestString(kRows.at(ri), "distribution") +
          " (template default per §12.1, human must confirm); u unknown");
    }
    {
      std::ostringstream os;
      os << "epsilon_num reused from floors.json " << kFloorsCell
         << " floors_18_9 " << BudgetFloorKey(kChannel) << "="
         << std::setprecision(17) << kEpsLive;
      kAssumptions.push_back(os.str());
    }
    {
      std::ostringstream os;
      os << "delta_ref reused from envelope_report.json worst_residual "
         << BudgetFloorKey(kChannel) << "="
         << std::setprecision(17) << kDeltaLive
         << " (max across FullMatrix cells)";
      kAssumptions.push_back(os.str());
    }
    kAssumptions.push_back("eta_ref unknown: no eta evidence in Phase 4");

    nlohmann::json kDoc;
    kDoc["provenance"] = {{"git_sha", LOB_GIT_SHA},
                          {"generator", "BudgetAssemble.OfflineDocuments"},
                          {"gate", "LOB_FULL_BUDGET=1"},
                          {"cell", kName},
                          {"channel", kChannel},
                          {"range_ft", kRange}};
    kDoc["cell"] = kName;
    kDoc["channel"] = kChannel;
    kDoc["range_ft"] = kRange;
    kDoc["rows"] = nlohmann::json::array();
    for (std::size_t ri = 0; ri < kRows.size(); ++ri) {
      nlohmann::json kEntry;
      kEntry["input"] = ReqManifestString(kRows.at(ri), "input");
      kEntry["sensitivity_c"] = kRows.at(ri).at("sensitivity_c");
      kEntry["sensitivity_source"] =
          ReqManifestString(kRows.at(ri), "sensitivity_source");
      kEntry["u"] = nullptr;
      kEntry["u_provenance"] = ReqManifestString(kRows.at(ri), "u_provenance");
      kEntry["distribution"] = ReqManifestString(kRows.at(ri), "distribution");
      kEntry["correlation"] = ReqManifestString(kRows.at(ri), "correlation");
      kEntry["contribution"] = nullptr;
      kEntry["status"] = ReqManifestString(kRows.at(ri), "status");
      kDoc["rows"].push_back(kEntry);
    }
    kDoc["excluded_with_floor"] = kCell.at("excluded_with_floor");
    kDoc["nonlinear_route_to_mc"] = kCell.at("nonlinear_route_to_mc");
    kDoc["epsilon_num"] = kCell.at("epsilon_num");
    kDoc["delta_ref"] = kCell.at("delta_ref");
    kDoc["eta_ref"] = "unknown";
    kDoc["missing_u"] = kMissing;
    kDoc["assumptions"] = kAssumptions;
    kDoc["verdict"] = kRes.status;
    if (kRes.complete) {
      kDoc["u_c_or_absent"] = kRes.u_c;
      ++emitted_uc;
    } else {
      kDoc["u_c_or_absent"] = nullptr;
    }

    const std::string kPath = std::string(LOB_VALIDATION_DIR) + "/budget_" +
                              kName + "_" + kChannel + ".json";
    std::ofstream kOut(kPath.c_str());
    ASSERT_TRUE(kOut.good()) << "cannot open " << kPath;
    kOut << kDoc.dump(2) << "\n";
    kOut.close();
    ++docs;
    std::printf("BUDGET %s %s rows=%u missing=%u verdict=%s\n", kName.c_str(),
                kChannel.c_str(), static_cast<unsigned>(kRows.size()),
                static_cast<unsigned>(kMissing.size()), kRes.status.c_str());
  }
  EXPECT_EQ(docs, kExpectedManifestCells);
  EXPECT_EQ(emitted_uc, 0);
}

}  // namespace tests
