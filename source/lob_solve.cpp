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
#include "solve_angle.hpp"
#include "solve_step.hpp"
#include "splines.hpp"

namespace lob {
namespace {
using StepFunction = void (*)(const LobContext&, TrajectoryStateT*,
                              spline::CurveView*, FeetT);

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

LobOutput LerpOutput(const TrajectoryStateT& s_prev,
                     const TrajectoryStateT& s_now, double alpha,
                     const LobContext& ctx) {
  const CartesianT<FeetT> kP = Lerp(s_prev.P(), s_now.P(), alpha);
  const CartesianT<FpsT> kV = Lerp(s_prev.V(), s_now.V(), alpha);
  const SecT kTof = Lerp(s_prev.TOF(), s_now.TOF(), alpha);
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
      if (InchT(pouts[i].elevation) < InchT(FeetT(-ctx.optic_height))) {
        pouts[i].deflection +=
            ctx.spindrift_factor * std::fabs(pouts[i].elevation);
      }
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

size_t ConvertForwardToInverse(const LobContext& ctx, LobOutput* pouts,
                               size_t count) {
  assert(pouts != nullptr);
  const RadiansT kTheta = RadiansT(MoaT(ctx.zero_angle + ctx.aerodynamic_jump));
  for (size_t i = 0; i < count; i++) {
    if (pouts[i].range == 0) {
      pouts[i].elevation = 0.0;
      pouts[i].deflection = 0.0;
      continue;
    }
    if (!std::isfinite(pouts[i].elevation) ||
        !std::isfinite(pouts[i].deflection)) {
      return i;
    }
    const FeetT kRange = FeetT(pouts[i].range);
    const FeetT kElevation = FeetT(InchT(pouts[i].elevation));
    pouts[i].elevation =
        MoaT(FastInverseAngle(kTheta, kElevation, kRange) - kTheta).Value();
    pouts[i].deflection = LobInchToMoa(-pouts[i].deflection, kRange.Value());
  }
  return count;
}

using StepFunction = void (*)(const LobContext&, TrajectoryStateT*,
                              spline::CurveView*, FeetT);

size_t SolveRanges(const LobContext& ctx, const uint32_t* pranges,
                   LobOutput* pouts, size_t size, StepFunction step) {
  const FpsT kMinimumSpeed(ctx.minimum_speed);
  const auto kAngle = RadiansT(MoaT(ctx.zero_angle + ctx.aerodynamic_jump));

  TrajectoryStateT s(
      CartesianT<FeetT>(FeetT(0.0)),
      CartesianT<FpsT>(FpsT(ctx.velocity) * std::cos(kAngle.Value()),
                       FpsT(ctx.velocity) * std::sin(kAngle.Value()),
                       FpsT(0.0)));
  size_t index = 0;

  spline::CurveView curve(spline::kKnots.data(), &ctx.drags[0]);

  while (true) {
    const TrajectoryStateT kS = s;

    if (pranges[index] > 0) {
      step(ctx, &s, &curve, FeetT(pranges[index]));
    }

    if (s.P().X() >= FeetT(pranges[index])) {
      pouts[index] = OutputAtState(s, ctx);
      index++;
    }

    if (index >= size) {
      break;
    }

    if (s.TOF() >= SecT(ctx.max_time) && kS.TOF() < SecT(ctx.max_time)) {
      const double kAlpha =
          ((SecT(ctx.max_time) - kS.TOF()) / (s.TOF() - kS.TOF())).Value();
      pouts[index] = LerpOutput(kS, s, kAlpha, ctx);
      index++;
      break;
    }
    if (s.V().Magnitude() <= kMinimumSpeed &&
        kS.V().Magnitude() > kMinimumSpeed) {
      const double kAlpha = ((kMinimumSpeed - kS.V().Magnitude()) /
                             (s.V().Magnitude() - kS.V().Magnitude()))
                                .Value();
      pouts[index] = LerpOutput(kS, s, kAlpha, ctx);
      index++;
      break;
    }
    // If vertical velocity exceeds 3x horizontal, consider falling straight
    // down.
    if (std::abs(s.V().Y().Value()) > s.V().X().Value() * 3) {
      pouts[index] = OutputAtState(s, ctx);
      index++;
      break;
    }
  }
  ApplyGyroscopicSpinDrift(ctx, pouts, index);
  return index;
}

}  // namespace

}  // namespace lob

extern "C" {
using namespace lob;  // NOLINT(google-build-using-namespace)

size_t LobSolve(const LobContext* pctx, const uint32_t* pranges,
                LobOutput* pouts, size_t size) {
  if (pctx == nullptr || pranges == nullptr || pouts == nullptr || size == 0 ||
      pctx->error != kLobErrorNone || pctx->velocity == 0 ||
      pctx->speed_of_sound <= 0.0 || MoaT(pctx->zero_angle).IsNaN()) {
    return 0;
  }
  for (size_t i = 1; i < size; i++) {
    if (pranges[i] <= pranges[i - 1]) {
      return 0;
    }
  }
  return SolveRanges(*pctx, pranges, pouts, size, FastSolveStep);
}

size_t LobSolveELR(const LobContext* pctx, const uint32_t* pranges,
                   LobOutput* pouts, size_t size) {
  if (pctx == nullptr || pranges == nullptr || pouts == nullptr || size == 0 ||
      pctx->error != kLobErrorNone || pctx->velocity == 0 ||
      pctx->speed_of_sound <= 0.0 || MoaT(pctx->zero_angle).IsNaN()) {
    return 0;
  }
  for (size_t i = 1; i < size; i++) {
    if (pranges[i] <= pranges[i - 1]) {
      return 0;
    }
  }
  return SolveRanges(*pctx, pranges, pouts, size, SolveStep);
}

size_t LobSolveInverse(const LobContext* pctx, const uint32_t* pranges,
                       LobOutput* pouts, size_t size) {
  if (pctx == nullptr || pranges == nullptr || pouts == nullptr || size == 0 ||
      pctx->error != kLobErrorNone || MoaT(pctx->zero_angle).IsNaN()) {
    return 0;
  }
  const size_t kForwardSolves = LobSolve(pctx, pranges, pouts, size);
  // Only reached ranges carry a residual worth converting. A fall-short row
  // reports a range below its request, so stop there instead of converting a
  // meaningless miss, mirroring the old iterative stop.
  size_t reached = 0;
  while (reached < kForwardSolves && pouts[reached].range >= pranges[reached]) {
    reached++;
  }
  return ConvertForwardToInverse(*pctx, pouts, reached);
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
