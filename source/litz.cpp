// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include "litz.hpp"

#include <cmath>
#include <cstdlib>

#include "eng_units.hpp"

namespace lob {
namespace litz {

// Page 417 of Applied Ballistics for Long-Range Shooting 3e - Litz
PmsiT CalculateBallisticCoefficient(LbsT mass, InchT diameter,
                                    double form_factor) {
  return PmsiT(mass.Value() /
               (diameter.Value() * diameter.Value() * form_factor));
}

// Page 422 of Applied Ballistics for Long-Range Shooting 3e - Litz
MoaT CalculateAerodynamicJump(double stability, InchT caliber, InchT length,
                              MphT zwind) {
  const double kSgCoeff = 0.01;
  const double kLCoeff = 0.0024;
  const double kIntercept = 0.032;
  const double kY = (kSgCoeff * std::abs(stability)) -
                    (kLCoeff * (length / caliber).Value()) + kIntercept;
  const double kDirection = stability >= 0 ? -1.0 : 1.0;
  return MoaT(kDirection * kY * zwind.Value());
}

// Page 423 of Applied Ballistics for Long-Range Shooting 3e - Litz
InchT CalculateGyroscopicSpinDrift(double stability, SecT time) {
  if (std::isnan(stability) || time.IsNaN()) {
    return InchT(0);
  }
  const double kAVal = 1.25 * (stability >= 0 ? 1.0 : -1.0);
  const double kBVal = 1.2;
  const double kExponent = 1.83;

  return InchT(kAVal * (std::abs(stability) + kBVal) *
               std::pow(time.Value(), kExponent));
}

// Page 427 of Applied Ballistics for Long-Range Shooting 3d - Litz
double CalculateG7FormFactorPrediction(InchT diameter, CaliberT nose_length,
                                       double ogive_rtr,
                                       CaliberT meplat_diameter,
                                       CaliberT tail_length,
                                       DegreesT boattail_angle) {
  const double kA = 1.470;
  const double kB = -0.346 * diameter.Value();
  const double kC = -0.162 * nose_length.Value();
  const double kD = 0.018 * ogive_rtr;
  const double kE = 0.072 * ogive_rtr * ogive_rtr;
  const double kF = 2.520 * meplat_diameter.Value();
  const double kG = -3.584 * std::pow(meplat_diameter.Value(), 2);
  const double kH = -0.171 * tail_length.Value();
  const double kI = -0.111 * boattail_angle.Value();
  const double kJ = 0.0118 * std::pow(boattail_angle.Value(), 2);
  const double kK = -0.000359 * std::pow(boattail_angle.Value(), 3);
  return kA + kB + kC + kD + kE + kF + kG + kH + kI + kJ + kK;
}

double CalculateG7FormFactorPrediction(InchT diameter, CaliberT nose_length,
                                       double ogive_rtr,
                                       CaliberT meplat_diameter,
                                       CaliberT tail_length,
                                       CaliberT base_diameter) {
  const RadiansT kBA(
      std::atan((1 - base_diameter.Value()) / (tail_length.Value() * 2)));
  return CalculateG7FormFactorPrediction(diameter, nose_length, ogive_rtr,
                                         meplat_diameter, tail_length, kBA);
}

}  // namespace litz
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