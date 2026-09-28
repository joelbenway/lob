// Copyright (c) 2026  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include <cmath>
#include <cstddef>
#include <limits>
#include <string>
#include <vector>

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

}  // namespace tests
