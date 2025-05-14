// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "calc.hpp"
#include "cartesian.hpp"
#include "eng_units.hpp"
#include "helpers.hpp"
#include "lob/lob.hpp"
#include "ode.hpp"
#include "solve_step.hpp"
#include "tables.hpp"

namespace lob {

void SolveStep(TrajectoryStateT* ps, SecT* pt, const Input& input, SecT step) {
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
  assert(step.Value() > 0 && "step is not a valid number");
  const auto kDt = SecT(ps->V().X().Inverse().Value() * step.Value());
  SolveStep(ps, pt, input, kDt);
}

namespace {

FpsT CalculateMinimumVelocity(FpsT min_speed, FtLbsT min_energy, SlugT mass) {
  return std::max(CalculateVelocityFromKineticEnergy(min_energy, mass),
                  min_speed);
}

Output Save(FeetT range, FpsT velocity, InchT elevation, InchT deflection,
            SecT time_of_flight, GrainT mass) {
  Output out;
  const FtLbsT kEnergy = CalculateKineticEnergy(velocity, mass);
  out.range = range.U32();
  out.velocity = velocity.U16();
  out.energy = kEnergy.U32();
  out.elevation = elevation.Value();
  out.deflection = deflection.Value();
  out.time_of_flight = time_of_flight.Value();
  return out;
}
}  // namespace

size_t Solve(const Input& in, const uint32_t* pranges, Output* pouts,
             size_t size, const Options& options) {
  if (std::isnan(in.table_coefficient)) {
    return 0;
  }
  const auto kAngle =
      RadiansT(MoaT(in.zero_angle + in.aerodynamic_jump)).Value();
  TrajectoryStateT s(
      CartesianT<FeetT>(FeetT(0.0)),
      CartesianT<FpsT>(FpsT(in.velocity) * std::cos(kAngle),
                       FpsT(in.velocity) * std::sin(kAngle), FpsT(0.0)));
  const FpsT kMinimumVelocity = CalculateMinimumVelocity(
      FpsT(options.min_speed), FtLbsT(options.min_energy), LbsT(in.mass));
  size_t index = 0;
  SecT t(0);
  while (true) {
    SolveStep(&s, &t, in, SecT(UsecT(options.step_size)));
    const FpsT kVelocity = s.V().Magnitude();
    if (s.P().X() >= FeetT(pranges[index])) {
      const InchT kSpinDrift =
          CalculateLitzGyroscopicSpinDrift(in.stability_factor, t);
      pouts[index] =
          Save(s.P().X(), kVelocity, InchT(s.P().Y() - FeetT(in.optic_height)),
               InchT(s.P().Z()) + kSpinDrift, t, LbsT(in.mass));
      index++;
    }

    if (index >= size) {
      break;
    }

    const bool kTimeMaxLimit =
        (t > SecT(options.max_time) && !AreEqual(options.max_time, 0.0));
    const bool kVelocityLimit = (kVelocity < kMinimumVelocity);
    const bool kFallLimit = (s.V().Y() > s.V().X() * 3);

    if (kTimeMaxLimit || kVelocityLimit || kFallLimit) {
      const InchT kSpinDrift =
          CalculateLitzGyroscopicSpinDrift(in.stability_factor, t);
      pouts[index] =
          Save(s.P().X(), kVelocity, InchT(s.P().Y() - FeetT(in.optic_height)),
               InchT(s.P().Z()) + kSpinDrift, t, LbsT(in.mass));
      index++;
      break;
    }
  }
  return index;
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
