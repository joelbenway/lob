// This file is a part of lob, an exterior ballistics calculation library
// Copyright (c) 2025  Joel Benway
// Please see end of file for extended copyright information

#pragma once

#include "eng_units.hpp"

namespace lob {

DegFT CalculateTemperatureAtAltitude(FeetT altitude, DegFT temperature);

DegFT CalculateTemperatureAtAltitudeMcCoy(FeetT altitude,
                                          DegFT sea_level_temperature);

InHgT BarometricFormula(FeetT altitude, InHgT pressure, DegFT temperature);

LbsPerCuFtT CalculateAirDensityAtAltitude(
    FeetT altitude, LbsPerCuFtT sea_level_density =
                        LbsPerCuFtT(kIsaSeaLevelAirDensityLbsPerCuFt));

FpsT CalculateSpeedOfSoundInAir(DegFT temperature);

InHgT CalculateWaterVaporSaturationPressure(DegFT temperature);

double CalcualteAirDensityRatio(InHgT pressure, DegFT temperature);

double CalculateAirDensityRatioHumidityCorrection(
    double humidity_pct, InHgT water_vapor_sat_pressure);

double CalculateSpeedOfSoundHumidityCorrection(double humidity_pct,
                                               InHgT water_vapor_sat_pressure);

double CalculateCdCoefficient(LbsPerCuFtT air_density, PmsiT bc);

double CalculateMillerTwistRuleStabilityFactor(InchT bullet_diameter,
                                               GrainT bullet_mass,
                                               InchT bullet_length,
                                               InchPerTwistT barrel_twist,
                                               FpsT muzzle_velocity);

double CalculateMillerTwistRuleCorrectionFactor(InHgT pressure,
                                                DegFT temperature);

double CalculateMillerTwistRuleCorrectionFactor(LbsPerCuFtT air_density);

InchT CalculateLitzGyroscopicSpinDrift(double stability, SecT time);

MoaT CalculateLitzAerodynamicJump(double stability, InchT caliber, InchT length,
                                  MphT zwind);

SqInT CalculateProjectileReferenceArea(InchT bullet_diameter);

FtLbsT CalculateKineticEnergy(FpsT velocity, SlugT mass);

PmsiT CalculateSectionalDensity(InchT bullet_diameter, LbsT bullet_mass);

namespace cwaj {
PsiT CalculateDynamicPressure(LbsPerCuFtT air_density, FpsT velocity);

CaliberT CalculateRadiusOfTangentOgive(CaliberT nose_length,
                                       CaliberT meplat_diameter);

CaliberT CalculateFullNoseLength(CaliberT nose_length, CaliberT meplat_diameter,
                                 double ogive_rtr);

double CalculateRelativeDensity(InchT diameter, InchT length,
                                InchT meplat_diameter, InchT nose_length,
                                InchT base_diameter, InchT base_length,
                                GrainT mass);

double CalculateCoefficientOfLift(CaliberT nose_length,
                                  CaliberT meplat_diameter, double ogive_rtr,
                                  MachT velocity);

double CalculateInertialRatio(InchT caliber, CaliberT length,
                              CaliberT nose_length, CaliberT full_nose_length,
                              GrainT mass, double relative_density);

HzT CalculateSpinRate(FpsT velocity, InchPerTwistT twist);

double CalculateAspectRatio(CaliberT length, CaliberT full_nose_length,
                            CaliberT boat_tail_length, CaliberT base_diameter);

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
}  // namespace cwaj

MoaT CalculateBRAerodynamicJump(InchT diameter, InchT meplat_diameter,
                                InchT base_diameter, InchT length,
                                InchT nose_length, InchT boat_tail_length,
                                double ogive_rtr, GrainT mass, FpsT velocity,
                                double stability, InchPerTwistT twist,
                                FpsT zwind, LbsPerCuFtT air_density,
                                FpsT speed_of_sound, PmsiT bc, double cd_ref);

}  // namespace lob

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.