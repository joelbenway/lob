// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
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

double CalculateAirDensityRatio(InHgT pressure, DegFT temperature);

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

FpsT CalculateVelocityFromKineticEnergy(FtLbsT energy, SlugT mass);

PmsiT CalculateSectionalDensity(InchT bullet_diameter, LbsT bullet_mass);

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