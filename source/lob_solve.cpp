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
#include "lob/lob.hpp"
#include "ode.hpp"
#include "solve_step.hpp"

namespace lob {

namespace {

Output LerpOutput(const TrajectoryStateT& s_now, const SecT t_now,
                  const TrajectoryStateT& s_prev, const SecT t_prev,
                  double alpha, const Input& input) {
  const CartesianT<FeetT> kP =
      s_prev.P() + (s_now.P() - s_prev.P()) * FeetT(alpha);
  const CartesianT<FpsT> kV =
      (s_prev.V() + (s_now.V() - s_prev.V()) * FpsT(alpha));
  const SecT kTimeOfFlight = t_prev + (t_now - t_prev) * SecT(alpha);
  const FpsT kVelocity = kV.Magnitude();
  const FtLbsT kEnergy =
      CalculateKineticEnergy(kVelocity, SlugT(LbsT(input.mass)));

  Output out;
  out.range = kP.X().U32();
  out.velocity = kVelocity.U16();
  out.energy = kEnergy.U32();
  out.elevation = InchT(kP.Y() - FeetT(input.optic_height)).Value();
  out.deflection = InchT(kP.Z()).Value();
  out.time_of_flight = kTimeOfFlight.Value();
  return out;
}

void ApplyGyroscopicSpinDrift(const Input& in, Output* pouts, size_t size) {
  assert(pouts != nullptr);
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
          litz::CalculateGyroscopicSpinDrift(in.stability_factor,
                                             SecT(pouts[i].time_of_flight))
              .Value();
    }
  }
}
}  // namespace

size_t Solve(const Input& in, const uint32_t* pranges, Output* pouts,
             size_t size) {
  assert(pranges != nullptr);
  assert(pouts != nullptr);
  assert(size > 0);
  if (in.error != ErrorT::kNone || pranges == nullptr || pouts == nullptr ||
      size == 0) {
    return 0;
  }
  const FpsT kMinimumSpeed(in.minimum_speed);
  const auto kAngle =
      RadiansT(MoaT(in.zero_angle + in.aerodynamic_jump)).Value();
  TrajectoryStateT s(
      CartesianT<FeetT>(FeetT(0.0)),
      CartesianT<FpsT>(FpsT(in.velocity) * std::cos(kAngle),
                       FpsT(in.velocity) * std::sin(kAngle), FpsT(0.0)));
  SecT t(0);
  size_t index = 0;

  while (true) {
    const TrajectoryStateT kS = s;
    const SecT kT = t;

    if (in.step_size != 0) {
      SolveStep(&s, &t, in, SecT(UsecT(in.step_size)));
    } else {
      SolveStep(&s, &t, in);
    }

    if (s.P().X() >= FeetT(pranges[index]) && kS.P().X() < s.P().X()) {
      const double kAlpha =
          ((FeetT(pranges[index]) - kS.P().X()) / (s.P().X() - kS.P().X()))
              .Value();
      pouts[index] = LerpOutput(s, t, kS, kT, kAlpha, in);
      index++;
    }

    if (index >= size) {
      break;
    }

    if (t >= SecT(in.max_time) && kT < SecT(in.max_time)) {
      const double kAlpha = ((SecT(in.max_time) - kT) / (t - kT)).Value();
      pouts[index] = LerpOutput(s, t, kS, kT, kAlpha, in);
      index++;
      break;
    }
    if (s.V().Magnitude() <= kMinimumSpeed &&
        kS.V().Magnitude() > kMinimumSpeed) {
      const double kAlpha = ((kMinimumSpeed - kS.V().Magnitude()) /
                             (s.V().Magnitude() - kS.V().Magnitude()))
                                .Value();
      pouts[index] = LerpOutput(s, t, kS, kT, kAlpha, in);
      index++;
      break;
    }
    if (std::abs(s.V().Y().Value()) > s.V().X().Value() * 3) {
      pouts[index] = LerpOutput(s, t, kS, kT, 1, in);
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
