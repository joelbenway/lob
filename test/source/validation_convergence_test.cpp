// Copyright (c) 2026  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>

#include "lob/lob.hpp"
#include "testing.hpp"

namespace tests {

TEST(ValidationConvergenceScaffold, C1SolvesAtDefaultStep) {
  const std::array<uint32_t, 4> kRanges = {300U, 900U, 1800U, 3000U};
  std::array<lob::Output, 4> outs{};
  const lob::Context kCtx = BuildAtStep(MakeC1IcaoBuilder(), 36U);
  ASSERT_EQ(kCtx.error, lob::ErrorT::kNone);
  EXPECT_EQ(SolveN(kCtx, kRanges, &outs), kRanges.size());
}

TEST(ValidationMath, ElevMoaDiffUsesSameRange) {
  lob::Output a{};
  lob::Output b{};
  a.range = 900U;
  b.range = 900U;
  a.elevation = 10.0;
  b.elevation = 13.0;
  EXPECT_DOUBLE_EQ(ElevInDiff(a, b), 3.0);
  EXPECT_DOUBLE_EQ(ElevMoaDiff(a, b),
                   lob::InchToMoa(13.0, 900.0) - lob::InchToMoa(10.0, 900.0));
}

TEST(ValidationMath, QuantizedChannelsFloorAtOneLsb) {
  lob::Output a{};
  lob::Output b{};
  a.range = 900U;
  b.range = 900U;
  a.velocity = 2000U;
  b.velocity = 2000U;
  a.energy = 1500U;
  b.energy = 1500U;
  EXPECT_TRUE(IsAtFloor(VelDiff(a, b), kVelFloorFps));
  EXPECT_TRUE(IsAtFloor(EnergyDiff(a, b), kEnergyFloorFtLbs));
}

TEST(ValidationMath, MonotoneAllowsEqualityOnlyAtFloor) {
  EXPECT_TRUE(DecreasesOrAtFloor(0.5, 0.3, 0.01));
  EXPECT_TRUE(DecreasesOrAtFloor(0.005, 0.005, 0.01));
  EXPECT_FALSE(DecreasesOrAtFloor(0.3, 0.5, 0.01));
  EXPECT_FALSE(DecreasesOrAtFloor(0.005, 0.02, 0.01));
}

TEST(ValidationMath, ObservedOrderSecondOrderCase) {
  EXPECT_NEAR(ObservedOrder(0.8, 0.2), 2.0, 1e-9);
  EXPECT_TRUE(std::isnan(ObservedOrder(0.8, 0.0)));
}

}  // namespace tests
