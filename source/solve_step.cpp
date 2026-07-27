// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include "solve_step.hpp"

#include <cassert>
#include <cmath>

#include "cartesian.hpp"
#include "eng_units.hpp"
#include "lob/lob.h"
#include "ode.hpp"
#include "splines.hpp"

namespace lob {
namespace {
TrajectoryStateT DsDt(const LobContext& ctx, const TrajectoryStateT& s,
                      spline::CurveView* pcurve) {
  const CartesianT<FpsT> kWind(FpsT(ctx.wind.x), FpsT(0.0), FpsT(ctx.wind.z));
  const MachT kMach(s.V().Magnitude(), FpsT(ctx.speed_of_sound).Inverse());
  const double kCd = pcurve->Eval(kMach) * ctx.drag_coeff;

  const CartesianT<FeetT> kDpDt(FeetT(s.V().X().Value()),
                                FeetT(s.V().Y().Value()),
                                FeetT(s.V().Z().Value()));
  const FpsT kScalarVelocity = (s.V() - kWind).Magnitude();
  CartesianT<FpsT> dv_dt = (s.V() - kWind) * FpsT(-1 * kCd) * kScalarVelocity;
  dv_dt.X(dv_dt.X() - s.V().Y() * ctx.coriolis.cos_l_sin_a -
          s.V().Z() * ctx.coriolis.sin_l);
  dv_dt.Y(dv_dt.Y() + s.V().X() * ctx.coriolis.cos_l_sin_a +
          s.V().Z() * ctx.coriolis.cos_l_cos_a);
  dv_dt.Z(dv_dt.Z() + s.V().X() * ctx.coriolis.sin_l -
          s.V().Y() * ctx.coriolis.cos_l_cos_a);
  dv_dt.X(dv_dt.X() + ctx.gravity.x);
  dv_dt.Y(dv_dt.Y() + ctx.gravity.y);
  return TrajectoryStateT{kDpDt, dv_dt, s.TOF()};
}

TrajectoryStateT DsDx(const LobContext& ctx, const TrajectoryStateT& s,
                      spline::CurveView* pcurve) {
  const CartesianT<FpsT> kWind(FpsT(ctx.wind.x), FpsT(0.0), FpsT(ctx.wind.z));
  const MachT kMach(s.V().Magnitude(), FpsT(ctx.speed_of_sound).Inverse());
  const double kCd = pcurve->Eval(kMach) * ctx.drag_coeff;

  const CartesianT<FeetT> kDpDt(FeetT(s.V().X().Value()),
                                FeetT(s.V().Y().Value()),
                                FeetT(s.V().Z().Value()));
  const FpsT kScalarVelocity = (s.V() - kWind).Magnitude();
  CartesianT<FpsT> dv_dt = (s.V() - kWind) * FpsT(-1 * kCd) * kScalarVelocity;
  dv_dt.X(dv_dt.X() - s.V().Y() * ctx.coriolis.cos_l_sin_a -
          s.V().Z() * ctx.coriolis.sin_l);
  dv_dt.Y(dv_dt.Y() + s.V().X() * ctx.coriolis.cos_l_sin_a +
          s.V().Z() * ctx.coriolis.cos_l_cos_a);
  dv_dt.Z(dv_dt.Z() + s.V().X() * ctx.coriolis.sin_l -
          s.V().Y() * ctx.coriolis.cos_l_cos_a);
  dv_dt.X(dv_dt.X() + ctx.gravity.x);
  dv_dt.Y(dv_dt.Y() + ctx.gravity.y);
  return TrajectoryStateT{kDpDt, dv_dt, s.TOF()};
}
}  // namespace

void SolveStep(const LobContext& ctx, TrajectoryStateT* ps,
               spline::CurveView* pcurve) {
  assert(ps != nullptr && pcurve != nullptr);

  const FeetT kStep(1);

  auto f = [&](FeetT /*x*/, const TrajectoryStateT& s) -> TrajectoryStateT {
    const FpsT kVx = s.V().X();
    assert(kVx > FpsT(0));
    return DsDx(ctx, s, pcurve) * kVx.Inverse();
  };
  *ps = RungeKuttaStep(FeetT(0), *ps, kStep, f);
  const FpsT kVx = ps->V().X();
  assert(kVx > FpsT(0));
  ps->TOF(ps->TOF() + SecT(kStep.Value() * kVx.Inverse().Value()));
}

void SolveTimeStep(const LobContext& ctx, TrajectoryStateT* ps,
                   spline::CurveView* pcurve) {
  assert(ps != nullptr && pcurve != nullptr);
  const SecT kStep(100);

  auto f = [&](SecT /*t*/, const TrajectoryStateT& s) -> TrajectoryStateT {
    return DsDt(ctx, s, pcurve);
  };
  *ps = RungeKuttaStep(SecT(0), *ps, kStep, f);
  ps->TOF(ps->TOF() + kStep);
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
