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
  const LobWindNode* pnodes = &ctx.wind_nodes[0];
  const size_t kCount = std::min<size_t>(ctx.wind_count, LOB_WIND_POINTS);
  if (kCount == 0) {
    return {FpsT(0.0), FpsT(0.0), FpsT(0.0)};
  }
  FpsT wind_x(pnodes[0].x_fps);
  FpsT wind_y(pnodes[0].y_fps);
  FpsT wind_z(pnodes[0].z_fps);
  const FeetT kDownrange = s.P().X();
  if (kCount > 1 && kDownrange > FeetT(0.0)) {
    FeetT previous_range(pnodes[0].range_ft);
    FpsT previous_x = wind_x;
    FpsT previous_y = wind_y;
    FpsT previous_z = wind_z;
    bool interpolated = false;
    for (size_t i = 1; i < kCount; ++i) {
      const FeetT kNodeRange(pnodes[i].range_ft);
      const FpsT kNodeX(pnodes[i].x_fps);
      const FpsT kNodeY(pnodes[i].y_fps);
      const FpsT kNodeZ(pnodes[i].z_fps);
      if (kDownrange <= kNodeRange) {
        const FeetT kSegment = kNodeRange - previous_range;
        const double kT =
            kSegment > FeetT(0.0)
                ? ((kDownrange - previous_range) / kSegment).Value()
                : 0.0;
        wind_x = Lerp(previous_x, kNodeX, kT);
        wind_y = Lerp(previous_y, kNodeY, kT);
        wind_z = Lerp(previous_z, kNodeZ, kT);
        interpolated = true;
        break;
      }
      previous_range = kNodeRange;
      previous_x = kNodeX;
      previous_y = kNodeY;
      previous_z = kNodeZ;
    }
    if (!interpolated && kDownrange > previous_range) {
      wind_x = previous_x;
      wind_y = previous_y;
      wind_z = previous_z;
    }
  }

  if (ctx.wind_shear_exponent > 0.0 || ctx.wind_shear_exponent < 0.0) {
    constexpr FeetT kMinWindHeightFt(1.0);
    constexpr FeetT kMaxWindHeightFt(300.0);
    const double kGravityY = ctx.gravity.y;
    const double kGravityMagnitude =
        std::sqrt((ctx.gravity.x * ctx.gravity.x) + (kGravityY * kGravityY));
    const double kCosRangeAngle = (kGravityMagnitude > 0.0 && -kGravityY > 0.0)
                                      ? -kGravityY / kGravityMagnitude
                                      : 1.0;
    FeetT height_above_shot_plane =
        FeetT(s.P().Y().Value() / kCosRangeAngle) + kWindReferenceHeightFt;
    if (!(height_above_shot_plane > kMinWindHeightFt)) {
      height_above_shot_plane = kMinWindHeightFt;
    }
    if (!(height_above_shot_plane < kMaxWindHeightFt)) {
      height_above_shot_plane = kMaxWindHeightFt;
    }
    const double kHeightFactor = CalculatePowerLawWindFactor(
        height_above_shot_plane, FeetT(kWindReferenceHeightFt),
        ctx.wind_shear_exponent);
    wind_x *= kHeightFactor;
    wind_y *= kHeightFactor;
    wind_z *= kHeightFactor;
  }
  return {wind_x, wind_y, wind_z};
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
