// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#pragma once

#include "eng_units.hpp"

namespace lob {
namespace litz {

PmsiT CalculateBallisticCoefficient(LbsT mass, InchT diameter,
                                    double form_factor);

MoaT CalculateAerodynamicJump(double stability, InchT caliber, InchT length,
                              MphT zwind);

InchT CalculateGyroscopicSpinDrift(double stability, SecT time);

double CalculateG7FormFactorPrediction(InchT diameter, CaliberT nose_length,
                                       double ogive_rtr,
                                       CaliberT meplat_diameter,
                                       CaliberT tail_length,
                                       DegreesT boattail_angle);

double CalculateG7FormFactorPrediction(InchT diameter, CaliberT nose_length,
                                       double ogive_rtr,
                                       CaliberT meplat_diameter,
                                       CaliberT tail_length,
                                       CaliberT base_diameter);

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