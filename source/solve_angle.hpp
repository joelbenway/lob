// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#pragma once

#include <cmath>
#include <cstddef>

#include "cartesian.hpp"
#include "eng_units.hpp"
#include "helpers.hpp"
#include "lob/lob.h"
#include "ode.hpp"
#include "solve_step.hpp"
#include "splines.hpp"

namespace lob {

namespace constant {
constexpr RadiansT kDefaultAngleTolerance = RadiansT(MoaT(0.01));
constexpr RadiansT kMaxAngle = DegreesT(45);
constexpr RadiansT kMinAngle = kMaxAngle * -1;
}  // namespace constant

inline RadiansT FastInverseAngle(RadiansT launch_angle, FeetT residual,
                                 FeetT range) {
  if (range <= FeetT(0.0)) {
    return launch_angle;
  }
  return std::atan(std::tan(launch_angle) - (residual / range).Value());
}

inline MoaT SolveAngle(const LobContext& ctx, FeetT range, FeetT impact_height,
                       RadiansT seed,
                       RadiansT tolerance = constant::kDefaultAngleTolerance) {
  constexpr size_t kMaxIterations = 10;
  const FpsT kVelocity = FpsT(ctx.velocity);
  const FpsT kMinimumSpeed(ctx.minimum_speed);
  spline::CurveView drag_curve(spline::kKnots.data(), &ctx.drags[0]);

  auto fire_to_target = [&](RadiansT launch_angle) -> FeetT {
    const RadiansT kAngle = launch_angle + RadiansT(MoaT(ctx.aerodynamic_jump));
    TrajectoryStateT s(
        CartesianT<FeetT>(FeetT(0.0)),
        CartesianT<FpsT>(kVelocity * std::cos(kAngle.Value()),
                         kVelocity * std::sin(kAngle.Value()), FpsT(0.0)));
    while (s.P().X() < range) {
      if (s.V().X() <= FpsT(0) || s.TOF() >= SecT(ctx.max_time) ||
          s.V().Magnitude() <= kMinimumSpeed) {
        return FeetT(NaN());
      }
      SolveStep(ctx, &s, &drag_curve, range);
    }
    return s.P().Y() - FeetT(ctx.optic_height) - impact_height;
  };

  RadiansT theta = seed;
  FeetT f = fire_to_target(theta);
  for (size_t iter = 0; iter < kMaxIterations && std::isfinite(f.Value());
       ++iter) {
    const RadiansT kThetaNext = FastInverseAngle(theta, f, range);

    if (kThetaNext < constant::kMinAngle || kThetaNext > constant::kMaxAngle ||
        std::isnan(kThetaNext.Value())) {
      return MoaT(NaN());
    }

    if (std::abs((kThetaNext - theta).Value()) <= tolerance.Value()) {
      return MoaT(kThetaNext);
    }

    theta = kThetaNext;
    f = fire_to_target(theta);
  }
  return MoaT(NaN());
}

}  // namespace lob

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
