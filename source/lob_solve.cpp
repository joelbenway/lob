// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "calc.hpp"
#include "cartesian.hpp"
#include "eng_units.hpp"
#include "litz.hpp"
#include "lob/lob.h"
#include "ode.hpp"
#include "solve_step.hpp"
#include "splines.hpp"

namespace lob {
namespace {

LobOutput LerpOutput(const TrajectoryStateT& s_now, const SecT t_now,
                     const TrajectoryStateT& s_prev, const SecT t_prev,
                     double alpha, const LobContext& ctx) {
  const CartesianT<FeetT> kP =
      s_prev.P() + (s_now.P() - s_prev.P()) * FeetT(alpha);
  const CartesianT<FpsT> kV =
      (s_prev.V() + (s_now.V() - s_prev.V()) * FpsT(alpha));
  const SecT kTimeOfFlight = t_prev + (t_now - t_prev) * SecT(alpha);
  const FpsT kVelocity = kV.Magnitude();
  const FtLbsT kEnergy =
      CalculateKineticEnergy(kVelocity, SlugT(LbsT(ctx.mass)));

  LobOutput out{};
  out.range = kP.X().U32();
  out.velocity = kVelocity.U16();
  out.energy = kEnergy.U32();
  out.elevation = InchT(kP.Y() - FeetT(ctx.optic_height)).Value();
  out.deflection = InchT(kP.Z()).Value();
  out.time_of_flight = kTimeOfFlight.Value();
  return out;
}

void ApplyGyroscopicSpinDrift(const LobContext& ctx, LobOutput* pouts,
                              size_t size) {
  assert(pouts != nullptr);
  if (!std::isnan(ctx.spindrift_factor)) {
    for (size_t i = 0; i < size; i++) {
      pouts[i].deflection +=
          ctx.spindrift_factor * std::fabs(pouts[i].elevation);
    }
    return;
  }
  if (std::fabs(ctx.stability_factor) > 0.0) {
    for (size_t i = 0; i < size; i++) {
      pouts[i].deflection +=
          litz::CalculateGyroscopicSpinDrift(ctx.stability_factor,
                                             SecT(pouts[i].time_of_flight))
              .Value();
    }
  }
}

}  // namespace
}  // namespace lob

extern "C" {
using namespace lob;  // NOLINT(google-build-using-namespace)

size_t LobSolve(const LobContext* ctx, const uint32_t* pranges,
                LobOutput* pouts, size_t size) {
  assert(ctx != nullptr);
  assert(pranges != nullptr);
  assert(pouts != nullptr);
  assert(size > 0);
  if (ctx == nullptr || ctx->error != kLobErrorNone || pranges == nullptr ||
      pouts == nullptr || size == 0 || ctx->velocity == 0 ||
      ctx->speed_of_sound <= 0.0) {
    return 0;
  }
  for (size_t i = 1; i < size; i++) {
    if (pranges[i] <= pranges[i - 1]) {
      return 0;
    }
  }
  const FpsT kMinimumSpeed(ctx->minimum_speed);
  const auto kAngle =
      RadiansT(MoaT(ctx->zero_angle + ctx->aerodynamic_jump)).Value();
  TrajectoryStateT s(
      CartesianT<FeetT>(FeetT(0.0)),
      CartesianT<FpsT>(FpsT(ctx->velocity) * std::cos(kAngle),
                       FpsT(ctx->velocity) * std::sin(kAngle), FpsT(0.0)));
  SecT t(0);
  size_t index = 0;

  spline::CurveView curve(spline::kKnots.data(), &ctx->drags[0]);

  while (true) {
    const TrajectoryStateT kS = s;
    const SecT kT = t;

    SolveStep(&s, &t, &curve, *ctx);

    if (s.P().X() >= FeetT(pranges[index]) && kS.P().X() < s.P().X()) {
      const double kAlpha =
          ((FeetT(pranges[index]) - kS.P().X()) / (s.P().X() - kS.P().X()))
              .Value();
      pouts[index] = LerpOutput(s, t, kS, kT, kAlpha, *ctx);
      index++;
    }

    if (index >= size) {
      break;
    }

    if (t >= SecT(ctx->max_time) && kT < SecT(ctx->max_time)) {
      const double kAlpha = ((SecT(ctx->max_time) - kT) / (t - kT)).Value();
      pouts[index] = LerpOutput(s, t, kS, kT, kAlpha, *ctx);
      index++;
      break;
    }
    if (s.V().Magnitude() <= kMinimumSpeed &&
        kS.V().Magnitude() > kMinimumSpeed) {
      const double kAlpha = ((kMinimumSpeed - kS.V().Magnitude()) /
                             (s.V().Magnitude() - kS.V().Magnitude()))
                                .Value();
      pouts[index] = LerpOutput(s, t, kS, kT, kAlpha, *ctx);
      index++;
      break;
    }
    // If vertical velocity exceeds 3x horizontal, consider falling straight
    // down.
    if (std::abs(s.V().Y().Value()) > s.V().X().Value() * 3) {
      pouts[index] = LerpOutput(s, t, kS, kT, 1, *ctx);
      index++;
      break;
    }
  }
  ApplyGyroscopicSpinDrift(*ctx, pouts, index);
  return index;
}

}  // extern "C"

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
