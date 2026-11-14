// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>

#include "cartesian.hpp"
#include "constants.hpp"
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

struct SightAnchor {
  FeetT y;
  FeetT z;
};

inline SightAnchor GetSightAnchor(const LobContext& ctx) noexcept {
  return {FeetT(ctx.optic_height), FeetT(ctx.optic_offset)};
}

inline double EstimateDragPerDistance(const LobContext& ctx,
                                      double velocity_fps) noexcept {
  if (ctx.speed_of_sound <= 0.0) {
    return 0.0;
  }
  spline::CurveView drag_curve(spline::kKnots.data(), &ctx.drags[0]);
  const double kMach = velocity_fps / ctx.speed_of_sound;
  const double kCd = drag_curve.Eval(MachT(kMach));
  if (std::isfinite(kCd) && kCd > 0.0 && std::isfinite(ctx.drag_coeff) &&
      ctx.drag_coeff > 0.0) {
    return ctx.drag_coeff * kCd;
  }
  return 0.0;
}

inline RadiansT EstimateAerodynamicSeed(const LobContext& ctx, FeetT range,
                                        FeetT impact_height) noexcept {
  if (!std::isfinite(range.Value()) || range <= FeetT(0.0) ||
      ctx.velocity == 0) {
    return RadiansT(0.0);
  }
  const double kDist = range.Value();
  const auto kV = static_cast<double>(ctx.velocity);
  const double kG = kStandardGravityFtPerSecSq;
  const double kKdrag = EstimateDragPerDistance(ctx, kV);
  const double kAnchorY = std::isnan(ctx.optic_height) ? 0.0 : ctx.optic_height;
  const double kImpactY = impact_height.IsNaN() ? 0.0 : impact_height.Value();
  const double kClimb = kAnchorY + kImpactY;
  const double kSightAngle = std::atan(kClimb / kDist);
  const double kDrop =
      (0.5 * kG * kDist / (kV * kV)) * (1.0 + ((2.0 / 3.0) * kKdrag * kDist));
  const double kJump = std::isnan(ctx.aerodynamic_jump)
                           ? 0.0
                           : RadiansT(MoaT(ctx.aerodynamic_jump)).Value();
  const double kSeedAngle = kSightAngle + kDrop - kJump;
  return std::isfinite(kSeedAngle)
             ? std::max(constant::kMinAngle,
                        std::min(constant::kMaxAngle, RadiansT(kSeedAngle)))
             : RadiansT(0.0);
}

inline RadiansT EstimateAzimuthSeed(const LobContext& ctx,
                                    FeetT range) noexcept {
  if (!std::isfinite(range.Value()) || range <= FeetT(0.0)) {
    return RadiansT(0.0);
  }
  const double kOffset = std::isnan(ctx.optic_offset) ? 0.0 : ctx.optic_offset;
  const double kSeed = std::atan(kOffset / range.Value());
  return std::isfinite(kSeed)
             ? std::max(constant::kMinAngle,
                        std::min(constant::kMaxAngle, RadiansT(kSeed)))
             : RadiansT(0.0);
}

namespace detail {
inline TrajectoryStateT MakeInitialState(const LobContext& ctx,
                                         RadiansT launch_angle) noexcept {
  const FpsT kVelocity = FpsT(ctx.velocity);
  const double kTheta =
      (launch_angle + RadiansT(MoaT(ctx.aerodynamic_jump))).Value();
  const double kPhi = RadiansT(MoaT(ctx.zero_azimuth)).Value();
  return {CartesianT<FeetT>(FeetT(0.0)),
          CartesianT<FpsT>(kVelocity * std::cos(kTheta) * std::cos(kPhi),
                           kVelocity * std::sin(kTheta),
                           kVelocity * std::sin(kPhi))};
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

inline TrajectoryStateT MakeInitialStateAzimuth(
    const LobContext& ctx, RadiansT launch_azimuth) noexcept {
  const FpsT kVelocity = FpsT(ctx.velocity);
  const RadiansT kElev = RadiansT(MoaT(ctx.zero_angle + ctx.aerodynamic_jump));
  return {CartesianT<FeetT>(FeetT(0.0)),
          CartesianT<FpsT>(kVelocity * std::cos(kElev.Value()) *
                               std::cos(launch_azimuth.Value()),
                           kVelocity * std::sin(kElev.Value()),
                           kVelocity * std::sin(launch_azimuth.Value()))};
}

template <typename StepFunc>
inline FeetT FireToTargetAzimuth(const LobContext& ctx, FeetT range,
                                 RadiansT launch_azimuth, StepFunc step) {
  TrajectoryStateT s = MakeInitialStateAzimuth(ctx, launch_azimuth);
  const FpsT kMinimumSpeed = FpsT(ctx.minimum_speed);
  spline::CurveView drag_curve(spline::kKnots.data(), &ctx.drags[0]);
  while (s.P().X() < range) {
    if (IsTerminal(s, ctx, kMinimumSpeed)) {
      return FeetT(NaN());
    }
    step(ctx, &s, &drag_curve, range);
  }
  return s.P().Z() - FeetT(ctx.optic_offset);
}

template <typename StepFunc>
inline MoaT SolveAngleAzimuthImpl(const LobContext& ctx, FeetT range,
                                  RadiansT seed, RadiansT tolerance,
                                  StepFunc step) {
  constexpr size_t kMaxIterations = 10;
  auto fire = [&](RadiansT launch_azimuth) -> FeetT {
    return FireToTargetAzimuth(ctx, range, launch_azimuth, step);
  };
  RadiansT phi = seed;
  FeetT f = fire(phi);
  for (size_t iter = 0; iter < kMaxIterations && std::isfinite(f.Value());
       ++iter) {
    const RadiansT kPhiNext = FastInverseAngle(phi, f, range);
    if (kPhiNext < constant::kMinAngle || kPhiNext > constant::kMaxAngle ||
        std::isnan(kPhiNext.Value())) {
      return MoaT(NaN());
    }
    if (std::abs((kPhiNext - phi).Value()) <= tolerance.Value()) {
      return MoaT(kPhiNext);
    }
    phi = kPhiNext;
    f = fire(phi);
  }
  return MoaT(NaN());
}
}  // namespace detail

inline MoaT SolveAngle(const LobContext& ctx, FeetT range,
                       FeetT impact_height = FeetT(0.0),
                       RadiansT seed = RadiansT(NaN()),
                       RadiansT tolerance = constant::kDefaultAngleTolerance) {
  const RadiansT kEffectiveSeed =
      std::isnan(seed.Value())
          ? EstimateAerodynamicSeed(ctx, range, impact_height)
          : seed;
  return detail::SolveAngleImpl(ctx, range, impact_height, kEffectiveSeed,
                                tolerance, SolveStep);
}

inline MoaT SolveAngleAzimuth(
    const LobContext& ctx, FeetT range, RadiansT seed = RadiansT(NaN()),
    RadiansT tolerance = constant::kDefaultAngleTolerance) {
  const RadiansT kEffectiveSeed =
      std::isnan(seed.Value()) ? EstimateAzimuthSeed(ctx, range) : seed;
  return detail::SolveAngleAzimuthImpl(ctx, range, kEffectiveSeed, tolerance,
                                       SolveStep);
}

inline MoaT SolveAngleELR(
    const LobContext& ctx, FeetT range, FeetT impact_height = FeetT(0.0),
    RadiansT seed = RadiansT(NaN()),
    RadiansT tolerance = constant::kDefaultAngleTolerance) {
  const RadiansT kEffectiveSeed =
      std::isnan(seed.Value())
          ? EstimateAerodynamicSeed(ctx, range, impact_height)
          : seed;
  return detail::SolveAngleImpl(ctx, range, impact_height, kEffectiveSeed,
                                tolerance, SolveStepELR);
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
