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

LobOutput OutputAtState(const TrajectoryStateT& s, const LobContext& ctx) {
  const FpsT kVelocity = s.V().Magnitude();
  const FtLbsT kEnergy =
      CalculateKineticEnergy(kVelocity, SlugT(LbsT(ctx.mass)));
  LobOutput out{};
  out.range = s.P().X().U32();
  out.velocity = kVelocity.U16();
  out.energy = kEnergy.U32();
  out.elevation = InchT(s.P().Y() - FeetT(ctx.optic_height)).Value();
  out.deflection = InchT(s.P().Z()).Value();
  out.time_of_flight = s.TOF().Value();
  return out;
}

LobOutput LerpOutput(const TrajectoryStateT& s_now,
                     const TrajectoryStateT& s_prev, double alpha,
                     const LobContext& ctx) {
  const CartesianT<FeetT> kP =
      s_prev.P() + (s_now.P() - s_prev.P()) * FeetT(alpha);
  const CartesianT<FpsT> kV =
      (s_prev.V() + (s_now.V() - s_prev.V()) * FpsT(alpha));
  const SecT kTof = s_prev.TOF() + (s_now.TOF() - s_prev.TOF()) * SecT(alpha);
  const FpsT kVelocity = kV.Magnitude();
  const FtLbsT kEnergy =
      CalculateKineticEnergy(kVelocity, SlugT(LbsT(ctx.mass)));

  LobOutput out{};
  out.range = kP.X().U32();
  out.velocity = kVelocity.U16();
  out.energy = kEnergy.U32();
  out.elevation = InchT(kP.Y() - FeetT(ctx.optic_height)).Value();
  out.deflection = InchT(kP.Z()).Value();
  out.time_of_flight = kTof.Value();
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

size_t LobSolve(const LobContext* pctx, const uint32_t* pranges,
                LobOutput* pouts, size_t size) {
  assert(pctx != nullptr);
  assert(pranges != nullptr);
  assert(pouts != nullptr);
  assert(size > 0);

  if (pctx == nullptr || pctx->error != kLobErrorNone || pranges == nullptr ||
      pouts == nullptr || size == 0 || pctx->velocity == 0 ||
      pctx->speed_of_sound <= 0.0) {
    return 0;
  }
  for (size_t i = 1; i < size; i++) {
    if (pranges[i] <= pranges[i - 1]) {
      return 0;
    }
  }
  const FpsT kMinimumSpeed(pctx->minimum_speed);
  const auto kAngle =
      RadiansT(MoaT(pctx->zero_angle + pctx->aerodynamic_jump)).Value();
  TrajectoryStateT s(
      CartesianT<FeetT>(FeetT(0.0)),
      CartesianT<FpsT>(FpsT(pctx->velocity) * std::cos(kAngle),
                       FpsT(pctx->velocity) * std::sin(kAngle), FpsT(0.0)));
  size_t index = 0;

  spline::CurveView curve(spline::kKnots.data(), &pctx->drags[0]);

  while (true) {
    const TrajectoryStateT kS = s;

    if (pranges[index] > 0) {
      SolveStep(*pctx, &s, &curve, FeetT(pranges[index]));
    }

    if (s.P().X() >= FeetT(pranges[index])) {
      pouts[index] = OutputAtState(s, *pctx);
      index++;
    }

    if (index >= size) {
      break;
    }

    if (s.TOF() >= SecT(pctx->max_time) && kS.TOF() < SecT(pctx->max_time)) {
      const double kAlpha =
          ((SecT(pctx->max_time) - kS.TOF()) / (s.TOF() - kS.TOF())).Value();
      pouts[index] = LerpOutput(s, kS, kAlpha, *pctx);
      index++;
      break;
    }
    if (s.V().Magnitude() <= kMinimumSpeed &&
        kS.V().Magnitude() > kMinimumSpeed) {
      const double kAlpha = ((kMinimumSpeed - kS.V().Magnitude()) /
                             (s.V().Magnitude() - kS.V().Magnitude()))
                                .Value();
      pouts[index] = LerpOutput(s, kS, kAlpha, *pctx);
      index++;
      break;
    }
    // If vertical velocity exceeds 3x horizontal, consider falling straight
    // down.
    if (std::abs(s.V().Y().Value()) > s.V().X().Value() * 3) {
      pouts[index] = OutputAtState(s, *pctx);
      index++;
      break;
    }
  }
  ApplyGyroscopicSpinDrift(*pctx, pouts, index);
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
