// Copyright (c) 2026  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

// Static-only: includes internal solver headers (never add to
// LOB_TEST_SOURCES).

#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>

#include "cartesian.hpp"
#include "eng_units.hpp"
#include "lob/lob.h"
#include "ode.hpp"
#include "solve_angle.hpp"
#include "solve_step.hpp"
#include "splines.hpp"

namespace tests {
namespace {

LobContext BuildC1CContext() {
  // Same C1-ICAO point as MakeC1IcaoBuilder() (testing.hpp), via the C API
  // used by test/source/solve_angle_test.cpp — no C++ wrapper casts.
  constexpr double kBcPsi = 0.232;
  constexpr double kDiameterInch = 0.308;
  constexpr double kMassGrains = 155.0;
  constexpr uint16_t kMuzzleVelocityFps = 2800U;
  constexpr double kZeroAngleMoa = 3.66;
  constexpr double kOpticHeightInches = 1.5;
  LobBuilder builder{};
  LobBuilderInit(&builder);
  LobBuilderBallisticCoefficientPsi(&builder, kBcPsi);
  LobBuilderBCDragFunction(&builder, kLobDragFunctionG7);
  LobBuilderBCAtmosphere(&builder, kLobAtmosphereReferenceIcao);
  LobBuilderDiameterInch(&builder, kDiameterInch);
  LobBuilderMassGrains(&builder, kMassGrains);
  LobBuilderInitialVelocityFps(&builder, kMuzzleVelocityFps);
  LobBuilderZeroAngleMOA(&builder, kZeroAngleMoa);
  LobBuilderOpticHeightInches(&builder, kOpticHeightInches);
  LobContext ctx{};
  LobBuilderBuild(&builder, &ctx);
  LobBuilderDestroy(&builder);
  return ctx;
}

lob::FeetT FireResidual(const LobContext& ctx, const lob::MoaT& angle,
                        const lob::FeetT& range) {
  const double kAngleRad =
      lob::RadiansT(angle).Value() +
      lob::RadiansT(lob::MoaT(ctx.aerodynamic_jump)).Value();
  lob::TrajectoryStateT s(
      lob::CartesianT<lob::FeetT>(lob::FeetT(0.0)),
      lob::CartesianT<lob::FpsT>(lob::FpsT(ctx.velocity) * std::cos(kAngleRad),
                                 lob::FpsT(ctx.velocity) * std::sin(kAngleRad),
                                 lob::FpsT(0.0)));
  lob::spline::CurveView curve(lob::spline::kKnots.data(), &ctx.drags[0]);
  while (s.P().X() < range) {
    lob::FastSolveStep(ctx, &s, &curve, range);
  }
  return s.P().Y() - lob::FeetT(ctx.optic_height);
}

}  // namespace

TEST(ValidationAngleConvergence, TighteningNeverRegressesResidual) {
  const LobContext kCtx = BuildC1CContext();
  ASSERT_EQ(kCtx.error, kLobErrorNone);
  const lob::FeetT kRange(900.0);
  const lob::MoaT kDefault =
      lob::FastSolveAngle(kCtx, kRange, lob::FeetT(0.0), lob::RadiansT(0.0));
  const lob::MoaT kTight =
      lob::FastSolveAngle(kCtx, kRange, lob::FeetT(0.0), lob::RadiansT(0.0),
                          lob::RadiansT(lob::MoaT(0.001)));
  ASSERT_FALSE(kDefault.IsNaN());
  ASSERT_FALSE(kTight.IsNaN());
  // Tighter tolerance lands within one default-tolerance of the default
  // solution (0.01 MOA granularity), and its re-integrated residual is no
  // worse, bounded by the repo's ±0.1 in round-trip granularity.
  EXPECT_LE(std::fabs((kTight - kDefault).Value()), 0.02);
  EXPECT_LE(std::fabs(FireResidual(kCtx, kTight, kRange).Value()),
            std::fabs(FireResidual(kCtx, kDefault, kRange).Value()));
  EXPECT_LE(std::fabs(FireResidual(kCtx, kTight, kRange).Value()), 0.1);
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
