// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include "solve_step.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>

#include "cartesian.hpp"
#include "eng_units.hpp"
#include "lob/lob.h"
#include "ode.hpp"

namespace lob {

void SolveStep(TrajectoryStateT* ps, SecT* pt, const LobContext& input,
               double cd) {
  assert(ps != nullptr);
  assert(pt != nullptr);

  const SecT kStep =
      input.step_size == 0 && ps->V().X() > FpsT(0)
          ? SecT(ps->V().X().Inverse().Value())
          : std::max(SecT(UsecT(input.step_size)), SecT(UsecT(1)));

  const CartesianT<FpsT> kWind(FpsT(input.wind.x), FpsT(0.0),
                               FpsT(input.wind.z));

  const MachT kMach(ps->V().Magnitude(), FpsT(input.speed_of_sound).Inverse());

  auto ds_dt = [&](SecT t, const TrajectoryStateT& s) -> TrajectoryStateT {
    static_cast<void>(t);  // t is unused
    const CartesianT<FeetT> kDpDt(FeetT(s.V().X().Value()),
                                  FeetT(s.V().Y().Value()),
                                  FeetT(s.V().Z().Value()));
    const FpsT kScalarVelocity = (s.V() - kWind).Magnitude();
    CartesianT<FpsT> dv_dt = (s.V() - kWind) * FpsT(-1 * cd) * kScalarVelocity;
    dv_dt.X(dv_dt.X() - s.V().Y() * input.coriolis.cos_l_sin_a -
            s.V().Z() * input.coriolis.sin_l);
    dv_dt.Y(dv_dt.Y() + s.V().X() * input.coriolis.cos_l_sin_a +
            s.V().Z() * input.coriolis.cos_l_cos_a);
    dv_dt.Z(dv_dt.Z() + s.V().X() * input.coriolis.sin_l -
            s.V().Y() * input.coriolis.cos_l_cos_a);
    dv_dt.X(dv_dt.X() + input.gravity.x);
    dv_dt.Y(dv_dt.Y() + input.gravity.y);
    return TrajectoryStateT{kDpDt, dv_dt};
  };  // ds_dt

  *ps = HeunStep(SecT(0), *ps, kStep, ds_dt);
  *pt += kStep;
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
