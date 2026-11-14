// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "lob/lob.hpp"

namespace tests {
namespace {

constexpr double kDiameter = 0.224;
constexpr double kMassGrains = 77.0;
constexpr uint16_t kMuzzleVelocityFps = 2750;
constexpr double kBcPsi = 0.372;
constexpr double kZeroDistanceYds = 100.0;
constexpr double kOpticHeightInches = 1.5;
constexpr double kOpticOffsetInches = 1.5;
constexpr double kNegOpticOffsetInches = -1.5;
constexpr double kTolLoose = 0.05;
constexpr double kTolTight = 0.01;
constexpr double kTolVeryTight = 1e-6;

TEST(LobOffsetTest, BitIdentityBaseline) {
  // Test Catalog #1: offset=0, no lateral air -> deflection column
  // bit-identical
  lob::Builder b1;
  b1.DiameterInch(kDiameter)
      .MassGrains(kMassGrains)
      .InitialVelocityFps(kMuzzleVelocityFps)
      .BallisticCoefficientPsi(kBcPsi)
      .ZeroDistanceYds(kZeroDistanceYds)
      .OpticHeightInches(kOpticHeightInches);
  const lob::Context kCtx1 = b1.Build();

  constexpr size_t kCount = 4;
  const std::array<uint32_t, kCount> kRanges = {0, 300, 600, 900};
  std::array<lob::Output, kCount> outs{};
  lob::Solve(kCtx1, kRanges, &outs);

  for (size_t i = 0; i < kCount; ++i) {
    EXPECT_DOUBLE_EQ(outs.at(i).deflection, 0.0);
  }
}

TEST(LobOffsetTest, ConvergenceAtZeroDistance) {
  // Test Catalog #2: Forward deflection at R0 ≈ 0; Inverse windage at R0 ≈ 0
  lob::Builder b;
  b.DiameterInch(kDiameter)
      .MassGrains(kMassGrains)
      .InitialVelocityFps(kMuzzleVelocityFps)
      .BallisticCoefficientPsi(kBcPsi)
      .ZeroDistanceYds(kZeroDistanceYds)
      .OpticHeightInches(kOpticHeightInches)
      .OpticLateralOffsetInches(kOpticOffsetInches);
  const lob::Context kCtx = b.Build();

  constexpr size_t kCount = 1;
  const std::array<uint32_t, kCount> kRanges = {300};  // 100 yd = 300 ft
  std::array<lob::Output, kCount> out_fwd{};
  lob::Solve(kCtx, kRanges, &out_fwd);
  EXPECT_NEAR(out_fwd.at(0).deflection, 0.0, kTolTight);  // within 0.01 inches

  std::array<lob::Output, kCount> out_inv{};
  lob::SolveInverse(kCtx, kRanges, &out_inv);
  EXPECT_NEAR(out_inv.at(0).deflection, 0.0, kTolTight);  // within 0.01 MOA
}

TEST(LobOffsetTest, MirrorSymmetry) {
  // Test Catalog #3 & Review Focus #1: +/- 1.5 in offset
  lob::Builder b_right;
  b_right.DiameterInch(kDiameter)
      .MassGrains(kMassGrains)
      .InitialVelocityFps(kMuzzleVelocityFps)
      .BallisticCoefficientPsi(kBcPsi)
      .ZeroDistanceYds(kZeroDistanceYds)
      .OpticLateralOffsetInches(kOpticOffsetInches);
  const lob::Context kCtxR = b_right.Build();

  lob::Builder b_left;
  b_left.DiameterInch(kDiameter)
      .MassGrains(kMassGrains)
      .InitialVelocityFps(kMuzzleVelocityFps)
      .BallisticCoefficientPsi(kBcPsi)
      .ZeroDistanceYds(kZeroDistanceYds)
      .OpticLateralOffsetInches(kNegOpticOffsetInches);
  const lob::Context kCtxL = b_left.Build();

  constexpr size_t kCount = 4;
  const std::array<uint32_t, kCount> kRanges = {0, 300, 600, 1500};
  std::array<lob::Output, kCount> out_r{};
  std::array<lob::Output, kCount> out_l{};
  lob::Solve(kCtxR, kRanges, &out_r);
  lob::Solve(kCtxL, kRanges, &out_l);

  for (size_t i = 1; i < kCount; ++i) {
    EXPECT_NEAR(out_r.at(i).deflection, -out_l.at(i).deflection, kTolVeryTight);
  }
}

TEST(LobOffsetTest, BehaviorChangeWindAtZeroDistance) {
  // Test Catalog #4: zeroing in wind absorbs drift at R0
  constexpr double kWindSpeedMph = 10.0;
  constexpr double kWindHeadingDeg = 90.0;

  lob::Builder b;
  b.DiameterInch(kDiameter)
      .MassGrains(kMassGrains)
      .InitialVelocityFps(kMuzzleVelocityFps)
      .BallisticCoefficientPsi(kBcPsi)
      .ZeroDistanceYds(kZeroDistanceYds)
      .WindSpeedMph(kWindSpeedMph)
      .WindHeadingDeg(kWindHeadingDeg);
  const lob::Context kCtx = b.Build();

  constexpr size_t kCount = 1;
  const std::array<uint32_t, kCount> kRanges = {300};
  std::array<lob::Output, kCount> out{};
  lob::Solve(kCtx, kRanges, &out);
  // Forward deflection at R0 is converged to ≈ 0 (was ~1.5 inches unzeroed)
  EXPECT_NEAR(out.at(0).deflection, 0.0, kTolLoose);
}

TEST(LobOffsetTest, NormativeWorkedExamplePhase1) {
  // Test Catalog #7 (Phase 1): offset = +1.5 in right
  lob::Builder b;
  b.DiameterInch(kDiameter)
      .MassGrains(kMassGrains)
      .InitialVelocityFps(kMuzzleVelocityFps)
      .BallisticCoefficientPsi(kBcPsi)
      .ZeroDistanceYds(kZeroDistanceYds)
      .OpticHeightInches(kOpticHeightInches)
      .OpticLateralOffsetInches(kOpticOffsetInches);
  const lob::Context kCtx = b.Build();

  constexpr size_t kCount = 2;
  const std::array<uint32_t, kCount> kRanges = {300, 1800};  // 100 yd, 600 yd
  std::array<lob::Output, kCount> fwd{};
  lob::Solve(kCtx, kRanges, &fwd);

  // At 100 yd, deflection ≈ 0
  EXPECT_NEAR(fwd.at(0).deflection, 0.0, kTolTight);
  // Past zero (600 yd), bullet crosses to shooter's right (-Z in frame ->
  // negative deflection)
  EXPECT_LT(fwd.at(1).deflection, 0.0);

  std::array<lob::Output, kCount> inv{};
  lob::SolveInverse(kCtx, kRanges, &inv);
  EXPECT_NEAR(inv.at(0).deflection, 0.0, kTolTight);
  EXPECT_GT(inv.at(1).deflection,
            0.0);  // Inverse hold is positive (aim left / dial right)
}

}  // namespace
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
