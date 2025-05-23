// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include "solve_step.hpp"

#include <cassert>
#include <cmath>

#include "cartesian.hpp"
#include "eng_units.hpp"
#include "helpers.hpp"
#include "lob/lob.hpp"
#include "ode.hpp"
#include "tables.hpp"

namespace lob {

void SolveStep(TrajectoryStateT* ps, SecT* pt, const Input& input, SecT step) {
  assert(ps != nullptr);
  assert(pt != nullptr);
  assert(step.Value() > 0 && "step is not a valid number");
  const CartesianT<FpsT> kWind(FpsT(input.wind.x), FpsT(0.0),
                               FpsT(input.wind.z));

  // For best accuracy all calculations which depend on the velocity state would
  // occur inside the lambda as the numerical method updates the velocity (and
  // thus the coefficient of drag) several times. However, LobLerp is an
  // expensive calculation and the difference between doing it once or several
  // times per step is negligible.
  const MachT kMach(ps->V().Magnitude(), FpsT(input.speed_of_sound).Inverse());
  const double kCd = LobLerp(kMachs, input.drags, kMach) *
                     static_cast<double>(input.table_coefficient);

  auto ds_dt = [&](SecT t, const TrajectoryStateT& s) -> TrajectoryStateT {
    static_cast<void>(t);  // t is unused
    const CartesianT<FeetT> kDpDt(FeetT(s.V().X().Value()),
                                  FeetT(s.V().Y().Value()),
                                  FeetT(s.V().Z().Value()));
    const FpsT kScalarVelocity = (s.V() - kWind).Magnitude();
    CartesianT<FpsT> dv_dt = (s.V() - kWind) * FpsT(-1 * kCd) * kScalarVelocity;
    dv_dt.X(dv_dt.X() - s.V().Y() * input.corilolis.cos_l_sin_a -
            s.V().Z() * input.corilolis.sin_l);
    dv_dt.Y(dv_dt.Y() + s.V().X() * input.corilolis.cos_l_sin_a +
            s.V().Z() * input.corilolis.cos_l_cos_a);
    dv_dt.Z(dv_dt.Z() + s.V().X() * input.corilolis.sin_l -
            s.V().Y() * input.corilolis.cos_l_cos_a);
    dv_dt.X(dv_dt.X() + input.gravity.x);
    dv_dt.Y(dv_dt.Y() + input.gravity.y);
    return TrajectoryStateT{kDpDt, dv_dt};
  };  // ds_dt

  *ps = HeunStep(SecT(0), *ps, step, ds_dt);
  *pt += step;
}

void SolveStep(TrajectoryStateT* ps, SecT* pt, const Input& input, FeetT step) {
  assert(ps != nullptr);
  assert(pt != nullptr);
  assert(step.Value() > 0 && "step is not a valid number");
  const SecT kDt = AreEqual(ps->V().X().Value(), 0.0)
                       ? SecT(UsecT(100))
                       : SecT(ps->V().X().Inverse().Value() * step.Value());
  SolveStep(ps, pt, input, kDt);
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
