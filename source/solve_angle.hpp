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

namespace detail {
inline TrajectoryStateT MakeInitialState(const LobContext& ctx,
                                         RadiansT launch_angle) noexcept {
  const FpsT kVelocity = FpsT(ctx.velocity);
  const RadiansT kAngle = launch_angle + RadiansT(MoaT(ctx.aerodynamic_jump));
  return {CartesianT<FeetT>(FeetT(0.0)),
          CartesianT<FpsT>(kVelocity * std::cos(kAngle.Value()),
                           kVelocity * std::sin(kAngle.Value()), FpsT(0.0))};
}

inline bool IsTerminal(const TrajectoryStateT& s, const LobContext& ctx,
                       FpsT minimum_speed) noexcept {
  return s.V().X() <= FpsT(0) || s.TOF() >= SecT(ctx.max_time) ||
         (s.V().X() <= minimum_speed && s.V().Magnitude() <= minimum_speed);
}

template <typename StepFunc>
inline FeetT FireToTarget(const LobContext& ctx, FeetT range,
                          FeetT impact_height, RadiansT launch_angle,
                          StepFunc step) {
  TrajectoryStateT s = MakeInitialState(ctx, launch_angle);
  const FpsT kMinimumSpeed = FpsT(ctx.minimum_speed);
  spline::CurveView drag_curve(spline::kKnots.data(), &ctx.drags[0]);
  while (s.P().X() < range) {
    if (IsTerminal(s, ctx, kMinimumSpeed)) {
      return FeetT(NaN());
    }
    step(ctx, &s, &drag_curve, range);
  }
  return s.P().Y() - FeetT(ctx.optic_height) - impact_height;
}

template <typename StepFunc>
inline MoaT SolveAngleImpl(const LobContext& ctx, FeetT range,
                           FeetT impact_height, RadiansT seed,
                           RadiansT tolerance, StepFunc step) {
  constexpr size_t kMaxIterations = 10;
  auto fire = [&](RadiansT launch_angle) -> FeetT {
    return FireToTarget(ctx, range, impact_height, launch_angle, step);
  };
  RadiansT theta = seed;
  FeetT f = fire(theta);
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
    f = fire(theta);
  }
  return MoaT(NaN());
}
}  // namespace detail

inline MoaT FastSolveAngle(
    const LobContext& ctx, FeetT range, FeetT impact_height, RadiansT seed,
    RadiansT tolerance = constant::kDefaultAngleTolerance) {
  return detail::SolveAngleImpl(ctx, range, impact_height, seed, tolerance,
                                FastSolveStep);
}

inline MoaT SolveAngle(const LobContext& ctx, FeetT range, FeetT impact_height,
                       RadiansT seed,
                       RadiansT tolerance = constant::kDefaultAngleTolerance) {
  return detail::SolveAngleImpl(ctx, range, impact_height, seed, tolerance,
                                SolveStep);
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
