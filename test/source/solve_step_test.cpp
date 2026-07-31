// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include "solve_step.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "cartesian.hpp"
#include "eng_units.hpp"
#include "lob/lob.h"
#include "ode.hpp"
#include "splines.hpp"

namespace tests {
namespace {

constexpr double kTestBC = 0.436;
constexpr uint16_t kTestMuzzleVelocity = 3100U;
constexpr double kTestZeroAngle = 6.11;

LobContext BuildContext(uint16_t step_size_in) {
  LobBuilder builder{};
  LobBuilderInit(&builder);
  LobBuilderBallisticCoefficientPsi(&builder, kTestBC);
  LobBuilderInitialVelocityFps(&builder, kTestMuzzleVelocity);
  LobBuilderZeroAngleMOA(&builder, kTestZeroAngle);
  LobBuilderStepSize(&builder, step_size_in);
  LobContext ctx{};
  LobBuilderBuild(&builder, &ctx);
  EXPECT_EQ(ctx.error, kLobErrorNone);
  LobBuilderDestroy(&builder);
  return ctx;
}

size_t CountStepsTo(const LobContext& ctx, lob::FeetT target) {
  const double kAngle = lob::RadiansT(lob::MoaT(ctx.zero_angle)).Value();
  lob::TrajectoryStateT s(
      lob::CartesianT<lob::FeetT>(lob::FeetT(0.0)),
      lob::CartesianT<lob::FpsT>(lob::FpsT(ctx.velocity) * std::cos(kAngle),
                                 lob::FpsT(ctx.velocity) * std::sin(kAngle),
                                 lob::FpsT(0.0)));
  lob::spline::CurveView curve(lob::spline::kKnots.data(), &ctx.drags[0]);
  size_t steps = 0;
  // Tolerance absorbs floating point rounding in the integrator (noise is
  // ~1e-13 ft); any real step-size error is at least 1 inch.
  constexpr double kRoundingToleranceFt = 1e-6;
  while (s.P().X() < target - lob::FeetT(kRoundingToleranceFt)) {
    lob::SolveStep(ctx, &s, &curve, target);
    ++steps;
  }
  return steps;
}
}  // namespace

TEST(SolveStepTests, TwelveInchStepOneYardSolveTakesThreeSteps) {
  const LobContext kCtx = BuildContext(12U);
  const lob::FeetT kTarget = lob::YardT(1);

  EXPECT_EQ(CountStepsTo(kCtx, kTarget), 3U);
}

TEST(SolveStepTests, OneHundredEightyInchStepTenYardSolveTakesTwoSteps) {
  const LobContext kCtx = BuildContext(180U);
  const lob::FeetT kTarget = lob::FeetT(lob::YardT(10));

  EXPECT_EQ(CountStepsTo(kCtx, kTarget), 2U);
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
