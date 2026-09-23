// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include "solve_step.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>

#include "calc.hpp"
#include "cartesian.hpp"
#include "constants.hpp"
#include "eng_units.hpp"
#include "lob/lob.h"
#include "ode.hpp"
#include "splines.hpp"

namespace lob {
namespace {
inline CartesianT<FpsT> GetWind(const LobContext& ctx,
                                const TrajectoryStateT& s) noexcept {
  // Stored nodes are frame-resolved at Build (pitch baked in), so the query
  // is pure downrange lerp plus optional altitude scaling: no trigonometry,
  // no direction conversion. A single-point profile falls out naturally.
  const LobWindNode* wind_nodes = &ctx.wind_nodes[0];
  auto count = std::min(static_cast<size_t>(ctx.wind_count),
                        static_cast<size_t>(LOB_WIND_POINTS));
  if (count == 0) {  // hand-packed context with no nodes: calm, not garbage
    return {FpsT(0.0), FpsT(0.0), FpsT(0.0)};
  }
  // Node components load directly; all math below is double.
  double wx = wind_nodes[0].x_fps;
  double wy = wind_nodes[0].y_fps;
  double wz = wind_nodes[0].z_fps;
  const double kX = s.P().X().Value();
  if (count > 1 && kX > 0.0) {
    double px = wind_nodes[0].range_ft;
    double phx = wx;
    double phy = wy;
    double phz = wz;
    bool found = false;
    for (size_t i = 1; i < count; ++i) {
      const double kRange = wind_nodes[i].range_ft;
      const double kNx = wind_nodes[i].x_fps;
      const double kNy = wind_nodes[i].y_fps;
      const double kNz = wind_nodes[i].z_fps;
      if (kX <= kRange) {
        const double kDen = kRange - px;
        const double kT = kDen > 0.0 ? (kX - px) / kDen : 0.0;
        wx = phx + kT * (kNx - phx);
        wy = phy + kT * (kNy - phy);
        wz = phz + kT * (kNz - phz);
        found = true;
        break;
      }
      px = kRange;
      phx = kNx;
      phy = kNy;
      phz = kNz;
    }
    if (!found && kX > px) {  // clamp above the final point
      wx = phx;
      wy = phy;
      wz = phz;
    }
  }
  // Altitude scale above the shot-parallel ground plane. The plane
  // through the muzzle parallel to the shot passes through every ground
  // target's footing, so S = 1 at both ends; height above it is frame-Y
  // over cos(range angle), recovered from gravity (strictly positive
  // since ±90° is rejected at Build). Identical to frame-Y at θ = 0.
  if (ctx.wind_shear_exponent > 0.0 || ctx.wind_shear_exponent < 0.0) {
    // Fixed 1-ft reference (prone muzzle height): S = 1 there by
    // construction. Evaluated heights clamp to the surface-layer band;
    // the power law is not extrapolated past it.
    constexpr double kWindReferenceHeightFt = 1.0;
    constexpr double kMinWindHeightFt = 1.0;
    constexpr double kMaxWindHeightFt = 300.0;
    const double kGy = ctx.gravity.y;
    const double kG = std::sqrt((ctx.gravity.x * ctx.gravity.x) + (kGy * kGy));
    const double kCosT = (kG > 0.0 && -kGy > 0.0) ? -kGy / kG : 1.0;
    double zagl = (s.P().Y().Value() / kCosT) + kWindReferenceHeightFt;
    if (!(zagl > kMinWindHeightFt)) {
      zagl = kMinWindHeightFt;
    }
    if (!(zagl < kMaxWindHeightFt)) {
      zagl = kMaxWindHeightFt;
    }
    const double kS = CalculatePowerLawWindFactor(
        FeetT(zagl), FeetT(kWindReferenceHeightFt), ctx.wind_shear_exponent);
    wx *= kS;
    wy *= kS;
    wz *= kS;
  }
  return {FpsT(wx), FpsT(wy), FpsT(wz)};
}

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
