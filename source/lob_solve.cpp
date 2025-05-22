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

namespace lob {

namespace {

FpsT CalculateMinimumVelocity(FpsT min_speed, FtLbsT min_energy, SlugT mass) {
  return std::max(CalculateVelocityFromKineticEnergy(min_energy, mass),
                  min_speed);
}

Output SaveOutput(FeetT range, FpsT velocity, InchT elevation, InchT deflection,
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

void ApplyGyroscopicSpinDrift(const Input& in, Output* pouts, size_t size) {
  // If we can apply Boatright-Ruiz spin drift, prefer it
  if (in.spindrift_factor > 0) {
    for (size_t i = 0; i < size; i++) {
      pouts[i].deflection +=
          in.spindrift_factor * std::fabs(pouts[i].elevation);
    }
    return;
  }
  // If we can apply Litz spin drift, use that
  if (std::fabs(in.stability_factor) > 0) {
    for (size_t i = 0; i < size; i++) {
      pouts[i].deflection +=
          CalculateLitzGyroscopicSpinDrift(in.stability_factor,
                                           SecT(pouts[i].time_of_flight))
              .Value();
    }
  }
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
      pouts[index] = SaveOutput(s.P().X(), kVelocity,
                                InchT(s.P().Y() - FeetT(in.optic_height)),
                                InchT(s.P().Z()), t, LbsT(in.mass));
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
      pouts[index] = SaveOutput(s.P().X(), kVelocity,
                                InchT(s.P().Y() - FeetT(in.optic_height)),
                                InchT(s.P().Z()), t, LbsT(in.mass));
      index++;
      break;
    }
  }
  ApplyGyroscopicSpinDrift(in, pouts, index);
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
