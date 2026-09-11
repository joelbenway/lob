// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include "solve_step.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>

#include "cartesian.hpp"
#include "constants.hpp"
#include "eng_units.hpp"
#include "lob/lob.h"
#include "ode.hpp"
#include "splines.hpp"

namespace lob {
namespace {
inline double GetDimensionlessAltitude(const LobContext& ctx,
                                       const TrajectoryStateT& s) noexcept {
  const double kGDotR =
      (s.P().X().Value() * ctx.gravity.x) + (s.P().Y().Value() * ctx.gravity.y);
  return -ctx.k_lapse * kGDotR;
}

inline double GetScaledDragCoeff(const LobContext& ctx, double u) noexcept {
  constexpr double kAlpha =
      (isa::kHydrostaticExponent - 1.0) / (2.0 * isa::kHydrostaticExponent);
  const double kDensityRatio = 1.0 - (u * (1.0 - (kAlpha * u)));
  return ctx.drag_coeff * kDensityRatio;
}

inline FpsT GetScaledSpeedOfSound(const LobContext& ctx, double u) noexcept {
  constexpr double kBeta = 1.0 / (2.0 * isa::kHydrostaticExponent);
  const double kSpeedOfSoundRatio = 1.0 - (kBeta * u);
  return FpsT(ctx.speed_of_sound * kSpeedOfSoundRatio);
}

inline double GetDtDx(FpsT vx) noexcept { return 1.0 / vx.Value(); }

inline MachT GetMach(const TrajectoryStateT& s, FpsT speed_of_sound) noexcept {
  return MachT(s.V().Magnitude(), speed_of_sound.Inverse());
}

inline double GetCd(spline::CurveView* pcurve, MachT mach,
                    double drag_coeff) noexcept {
  return pcurve->Eval(mach) * drag_coeff;
}

inline CartesianT<FeetT> GetDpDt(const TrajectoryStateT& s) noexcept {
  return {FeetT(s.V().X().Value()), FeetT(s.V().Y().Value()),
          FeetT(s.V().Z().Value())};
}

inline CartesianT<FpsT> GetDvDt(const LobContext& ctx,
                                const TrajectoryStateT& s,
                                const CartesianT<FpsT>& wind, double cd) {
  const FpsT kScalarVelocity = (s.V() - wind).Magnitude();
  const double kScale = -cd * kScalarVelocity.Value();
  CartesianT<FpsT> dv_dt = (s.V() - wind) * kScale;
  dv_dt.X(dv_dt.X() - s.V().Y() * ctx.coriolis.cos_l_sin_a -
          s.V().Z() * ctx.coriolis.sin_l);
  dv_dt.Y(dv_dt.Y() + s.V().X() * ctx.coriolis.cos_l_sin_a +
          s.V().Z() * ctx.coriolis.cos_l_cos_a);
  dv_dt.Z(dv_dt.Z() + s.V().X() * ctx.coriolis.sin_l -
          s.V().Y() * ctx.coriolis.cos_l_cos_a);
  dv_dt.X(dv_dt.X() + ctx.gravity.x);
  dv_dt.Y(dv_dt.Y() + ctx.gravity.y);
  return dv_dt;
}

inline TrajectoryStateT DsDxCore(const LobContext& ctx,
                                 const TrajectoryStateT& s,
                                 spline::CurveView* pcurve, double drag_coeff,
                                 FpsT speed_of_sound) {
  const FpsT kVx = s.V().X();
  if (kVx <= FpsT(0)) {
    return {CartesianT<FeetT>(FeetT(0)), CartesianT<FpsT>(FpsT(0))};
  }
  const double kDtDx = GetDtDx(kVx);
  const CartesianT<FpsT> kWind = GetWind(ctx, s);
  const MachT kMach = GetMach(s, speed_of_sound);
  const double kCd = GetCd(pcurve, kMach, drag_coeff);
  const CartesianT<FeetT> kDpDt = GetDpDt(s);
  const CartesianT<FpsT> kDvDt = GetDvDt(ctx, s, kWind, kCd);
  return TrajectoryStateT{kDpDt * kDtDx, kDvDt * kDtDx, SecT(kDtDx)};
}

TrajectoryStateT FastDsDx(const LobContext& ctx, const TrajectoryStateT& s,
                          spline::CurveView* pcurve) {
  return DsDxCore(ctx, s, pcurve, ctx.drag_coeff, FpsT(ctx.speed_of_sound));
}

TrajectoryStateT DsDx(const LobContext& ctx, const TrajectoryStateT& s,
                      spline::CurveView* pcurve) {
  const double kU = GetDimensionlessAltitude(ctx, s);
  const double kScaledDragCoeff = GetScaledDragCoeff(ctx, kU);
  const FpsT kScaledSpeedOfSound = GetScaledSpeedOfSound(ctx, kU);
  return DsDxCore(ctx, s, pcurve, kScaledDragCoeff, kScaledSpeedOfSound);
}

inline FeetT ComputeStep(const LobContext& ctx, const TrajectoryStateT& s,
                         FeetT target_x) noexcept {
  const FeetT kStepSize = ctx.step_size == 0
                              ? FeetT(YardT(1))
                              : static_cast<FeetT>(InchT(ctx.step_size));
  return target_x > s.P().X() ? std::min(target_x - s.P().X(), kStepSize)
                              : kStepSize;
}
}  // namespace

CartesianT<FpsT> GetWind(const LobContext& ctx,
                         const TrajectoryStateT& s) noexcept {
  // Historical path: uniform, flat, unscaled.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
  const bool kIsHistorical = ctx.wind_count <= 1 && ctx.wind_sin == 0.0 &&
                             std::isnan(ctx.wind_roughness_ft);
#pragma GCC diagnostic pop
  if (kIsHistorical) {
    return {FpsT(ctx.wind.x), FpsT(0.0), FpsT(ctx.wind.z)};
  }
  // 1. Downrange lerp over horizontal components (point 0 = ctx.wind at x=0).
  double hx = ctx.wind.x;
  double hz = ctx.wind.z;
  size_t count = static_cast<size_t>(ctx.wind_count);
  if (count > LOB_WIND_POINTS) {
    count = LOB_WIND_POINTS;  // defensive: hand-built contexts
  }
  const double kX = s.P().X().Value();
  if (count > 1 && kX > 0.0) {
    double px = 0.0;
    double phx = ctx.wind.x;
    double phz = ctx.wind.z;
    bool kFound = false;
    for (size_t i = 0; i + 1 < count; ++i) {
      const LobWindPoint& kPt = ctx.wind_points[i];
      const double kDen = kPt.range_ft - px;
      if (kX <= kPt.range_ft) {
        const double kT = kDen > 0.0 ? (kX - px) / kDen : 0.0;
        hx = phx + kT * (kPt.x_fps - phx);
        hz = phz + kT * (kPt.z_fps - phz);
        kFound = true;
        break;
      }
      px = kPt.range_ft;
      phx = kPt.x_fps;
      phz = kPt.z_fps;
    }
    if (!kFound && kX > px) {  // clamp above the final point
      hx = phx;
      hz = phz;
    }
  }
  // 2. Pitch into the shooting frame.
  double wx = hx * ctx.wind_cos;
  double wy = -hx * ctx.wind_sin;
  double wz = hz;
  // 3. Altitude scale about true vertical (flat-terrain assumption).
  if (!std::isnan(ctx.wind_roughness_ft)) {
    const double kGx = ctx.gravity.x;
    const double kGy = ctx.gravity.y;
    const double kG = std::sqrt(kGx * kGx + kGy * kGy);
    const double kH =
        kG > 0.0 ? -((s.P().X().Value() * kGx + s.P().Y().Value() * kGy) / kG)
                 : 0.0;
    double zagl = kH + ctx.wind_muzzle_height_ft;
    const double kFloor = 1.05 * ctx.wind_roughness_ft;
    if (!(zagl > kFloor)) {
      zagl = kFloor;
    }
    const double kS =
        std::log(zagl / ctx.wind_roughness_ft) * ctx.wind_inv_ln_denom;
    wx *= kS;
    wy *= kS;
    wz *= kS;
  }
  return {FpsT(wx), FpsT(wy), FpsT(wz)};
}

void FastSolveStep(const LobContext& ctx, TrajectoryStateT* ps,
                   spline::CurveView* pcurve, FeetT target_x) {
  assert(ps != nullptr && pcurve != nullptr);
  const FeetT kStep = ComputeStep(ctx, *ps, target_x);
  auto f = [&](FeetT, const TrajectoryStateT& s) {
    return FastDsDx(ctx, s, pcurve);
  };
  *ps = HeunStep(FeetT(0), *ps, kStep, f);
  const FpsT kVx = ps->V().X();
  if (kVx <= FpsT(0)) {
    ps->V(FpsT(0));
    return;
  }
}

void SolveStep(const LobContext& ctx, TrajectoryStateT* ps,
               spline::CurveView* pcurve, FeetT target_x) {
  assert(ps != nullptr && pcurve != nullptr);
  const FeetT kStep = ComputeStep(ctx, *ps, target_x);
  auto f = [&](FeetT, const TrajectoryStateT& s) {
    return DsDx(ctx, s, pcurve);
  };
  *ps = HeunStep(FeetT(0), *ps, kStep, f);
  const FpsT kVx = ps->V().X();
  if (kVx <= FpsT(0)) {
    ps->V(FpsT(0));
    return;
  }
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
