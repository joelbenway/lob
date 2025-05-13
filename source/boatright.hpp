// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#pragma once

#include "eng_units.hpp"

namespace lob {
namespace boatright {
PsiT CalculateDynamicPressure(LbsPerCuFtT air_density, FpsT velocity);

CaliberT CalculateRadiusOfTangentOgive(CaliberT nose_length,
                                       CaliberT meplat_diameter);

CaliberT CalculateFullNoseLength(CaliberT nose_length, CaliberT meplat_diameter,
                                 double ogive_rtr);

double CalculateRelativeDensity(InchT diameter, InchT length,
                                InchT meplat_diameter, InchT nose_length,
                                InchT base_diameter, InchT base_length,
                                GrainT mass);

double CalculateRelativeDensity(InchT diameter, InchT length,
                                InchT meplat_diameter, InchT nose_length,
                                InchT base_diameter, InchT base_length,
                                double ogive_rtr, GrainT mass);

double CalculateCoefficientOfLift(CaliberT nose_length,
                                  CaliberT meplat_diameter, double ogive_rtr,
                                  MachT velocity);

double CalculateInertialRatio(InchT caliber, CaliberT length,
                              CaliberT nose_length, CaliberT full_nose_length,
                              GrainT mass, double relative_density);

HzT CalculateSpinRate(FpsT velocity, InchPerTwistT twist);

double CalculateAspectRatio(CaliberT length, CaliberT full_nose_length,
                            CaliberT tail_length, CaliberT base_diameter);

double CalculateYawDragCoefficient(MachT speed, double coefficient_of_lift,
                                   double aspect_ratio);

double CalculateEpicyclicRatio(double stability);

uint16_t CalculateNutationCyclesNeeded(double epicyclic_ratio);

HzT CalculateGyroscopicRateSum(HzT spin_rate, double inertial_ratio);

HzT CalculateGyroscopicRateF2(HzT gyroscopic_rate_sum, double epicyclic_ratio);

SecT CalculateFirstNutationPeriod(HzT f1, HzT f2);

double CalculateCrosswindAngleGamma(MphT zwind, FpsT velocity);

double CalculateZeroYawDragCoefficientOfDrag(double cd_ref, GrainT mass,
                                             InchT diameter, PmsiT bc);

double CalculateYawDragAdjustment(double gamma, double r, double cda);

double CalculateVerticalPitch(double gamma, double r, double n);

double CalculateVerticalImpulse(InchPerTwistT twist, uint16_t n, SecT tn,
                                PsiT q, SqInT s, double cl, double cd,
                                double pitch);
double CalculateMagnitudeOfMomentum(GrainT mass, FpsT velocity);
}  // namespace boatright

MoaT CalculateBRAerodynamicJump(InchT diameter, InchT meplat_diameter,
                                InchT base_diameter, InchT length,
                                InchT nose_length, InchT tail_length,
                                double ogive_rtr, GrainT mass, FpsT velocity,
                                double stability, InchPerTwistT twist,
                                FpsT zwind, LbsPerCuFtT air_density,
                                FpsT speed_of_sound, PmsiT bc, double cd_ref);

namespace boatright {
double CalculateKV(FpsT initial_velocity, FpsT target_velocity);

double CalculateKOmega(InchT diameter, SecT supersonic_time);

RadiansT CalculateYawOfRepose(FpsT initial_velocity, InchPerTwistT twist,
                              double inertial_ratio, double epicyclic_ratio,
                              double komega, double kv);

double CalculatePotentialDragForce(InchT diameter, LbsPerCuFtT air_density,
                                   FpsT target_velocity);

double CalculateCoefficentOfLiftAtT(double cl0, FpsT initial_velocity,
                                    SecT supersonic_time);

double CalculateSpinDriftScaleFactor(double potential_drag_force,
                                     RadiansT yaw_of_repose,
                                     double coefficent_of_lift, GrainT mass);

InchT CalculateSpinDrift(double scale_factor, InchT drop);

}  // namespace boatright

double CalculateBRSpinDriftFactor(InchT diameter, InchT meplat_diameter,
                                  InchT base_diameter, InchT length,
                                  InchT nose_length, InchT tail_length,
                                  double ogive_rtr, GrainT mass, FpsT velocity,
                                  double stability, InchPerTwistT twist,
                                  LbsPerCuFtT air_density,
                                  SecT supersonic_time);

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