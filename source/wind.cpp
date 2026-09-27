// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include "wind.hpp"

#include <cmath>
#include <cstddef>

#include "calc.hpp"
#include "cartesian.hpp"
#include "constants.hpp"
#include "eng_units.hpp"
#include "lob/lob.h"
#include "ode.hpp"

namespace lob {

CartesianT<FpsT> GetWind(const LobContext& ctx, const TrajectoryStateT& s,
                         size_t count) noexcept {
  const LobWindNode* pnodes = &ctx.wind_nodes[0];
  FpsT wind_x(pnodes[0].x_fps);
  FpsT wind_y(pnodes[0].y_fps);
  FpsT wind_z(pnodes[0].z_fps);
  const FeetT kDownrange = s.P().X();
  if (count > 1 && kDownrange > FeetT(0.0)) {
    FeetT previous_range(pnodes[0].range_ft);
    FpsT previous_x = wind_x;
    FpsT previous_y = wind_y;
    FpsT previous_z = wind_z;
    bool interpolated = false;
    for (size_t i = 1; i < count; ++i) {
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
// A PARTICULAR PURPOSE. See the GNU General Public License along with
// lob. If not, see <https://www.gnu.org/licenses/>.
