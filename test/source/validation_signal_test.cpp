// Copyright (c) 2026  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include <cmath>
#include <cstddef>
#include <limits>
#include <vector>

namespace tests {

// Band edges are conventional-not-prescribed: order-of-magnitude reporting
// bands from spec §13.2's >>/approx/<</ language, not a project significance
// threshold. No application decision may cite them without stating its own k.
constexpr double kSigClear = 10.0;
constexpr double kSigMarginalLo = 0.1;

enum class SigBand { kDistinguishable, kMarginal, kIndistinguishable };

inline std::vector<double> EffectDelta(
    const std::vector<double>& with_effect,
    const std::vector<double>& without_effect) {
  std::vector<double> delta(with_effect.size(), 0.0);
  for (std::size_t i = 0; i < with_effect.size(); ++i) {
    delta.at(i) = with_effect.at(i) - without_effect.at(i);
  }
  return delta;
}

inline double UTotal(double u_num, double granularity,
                     double mc_sigma =
                         std::numeric_limits<double>::quiet_NaN()) {
  double sum = u_num * u_num + granularity * granularity;
  if (!std::isnan(mc_sigma)) {
    sum += mc_sigma * mc_sigma;
  }
  return std::sqrt(sum);
}

inline double RSig(double delta, double u_total) {
  if (!(u_total > 0.0)) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  return std::fabs(delta) / u_total;
}

inline SigBand ClassifySigBand(double r_sig) {
  if (r_sig > kSigClear) {
    return SigBand::kDistinguishable;
  }
  if (r_sig >= kSigMarginalLo) {
    return SigBand::kMarginal;
  }
  return SigBand::kIndistinguishable;
}

}  // namespace tests

TEST(SignalMath, DeltaOfKnownVectorsIsExact) {
  const std::vector<double> kWith = {10.0, -89.70};
  const std::vector<double> kWithout = {10.0, -89.73};
  const std::vector<double> kDelta = tests::EffectDelta(kWith, kWithout);
  ASSERT_EQ(kDelta.size(), 2U);
  EXPECT_DOUBLE_EQ(kDelta.at(0), 0.0);
  EXPECT_NEAR(kDelta.at(1), 0.03, 1e-12);
}

TEST(SignalMath, UTotalFollowsHypotenuse) {
  EXPECT_DOUBLE_EQ(tests::UTotal(3.0, 4.0), 5.0);
  EXPECT_DOUBLE_EQ(
      tests::UTotal(3.0, 4.0, std::numeric_limits<double>::quiet_NaN()), 5.0);
  EXPECT_DOUBLE_EQ(tests::UTotal(3.0, 4.0, 0.0), 5.0);
  EXPECT_DOUBLE_EQ(tests::UTotal(0.0, 0.0, 5.0), 5.0);
}

TEST(SignalMath, RSigGuardsZeroYardstick) {
  EXPECT_DOUBLE_EQ(tests::RSig(10.0, 5.0), 2.0);
  EXPECT_TRUE(std::isnan(tests::RSig(1.0, 0.0)));
  EXPECT_TRUE(std::isnan(tests::RSig(1.0, -1.0)));
  EXPECT_TRUE(std::isnan(tests::RSig(0.0, 0.0)));
}

TEST(SignalMath, BandsAtTenAndTenth) {
  EXPECT_EQ(tests::ClassifySigBand(11.0), tests::SigBand::kDistinguishable);
  EXPECT_EQ(tests::ClassifySigBand(1.0), tests::SigBand::kMarginal);
  EXPECT_EQ(tests::ClassifySigBand(0.01),
            tests::SigBand::kIndistinguishable);
  EXPECT_EQ(tests::ClassifySigBand(10.0), tests::SigBand::kMarginal);
  EXPECT_EQ(tests::ClassifySigBand(0.1), tests::SigBand::kMarginal);
  EXPECT_EQ(tests::ClassifySigBand(std::numeric_limits<double>::quiet_NaN()),
            tests::SigBand::kIndistinguishable);
}
