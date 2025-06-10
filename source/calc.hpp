// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#pragma once

#include <cassert>
#include <cmath>

#include "constants.hpp"
#include "eng_units.hpp"

namespace lob {

inline DegFT CalculateTemperatureAtAltitude(FeetT altitude, DegFT temperature) {
  const DegFT kTemperature =
      temperature - DegFT(kIsaLapseDegFPerFt * altitude.Value());
  return std::max(kTemperature, DegFT(kIsaMinimumTempDegF));
}

// Page 166 of Modern Exterior Ballistics - McCoy
inline DegFT CalculateTemperatureAtAltitudeMcCoy(FeetT altitude,
                                                 DegFT sea_level_temperature) {
  const double kK = 6.858E-6 + (2.776E-11 * altitude.Value());
  const double kA = DegRT(DegFT(0)).Value();
  // Note that the formula printed in 2e of Modern External Ballistics omits the
  // negative sign. This is remedied here.
  return (sea_level_temperature + kA) * std::exp(-1.0 * kK * altitude.Value()) -
         kA;
}

// https://wikipedia.org/wiki/Barometric_formula
inline InHgT BarometricFormula(FeetT altitude, InHgT pressure,
                               DegFT temperature) {
  const double kGasConstant = 1716.49;     // ft-lb / slug^{-1}R^{-1}
  const double kMolarMassOfAir = 28.9644;  // lb/lb-mol
  const FeetT kHeight = std::min(altitude, FeetT(kIsaTropopauseAltitudeFt));

  const double kExponent =
      kStandardGravityFtPerSecSq / (kGasConstant * kIsaLapseDegFPerFt);

  const double kBase =
      1.0 - (kIsaLapseDegFPerFt * kHeight.Value() / DegRT(temperature).Value());

  InHgT output = pressure * std::pow(kBase, kExponent);

  if (altitude > FeetT(kIsaTropopauseAltitudeFt)) {
    const double kNumberator = -1.0 * kStandardGravityFtPerSecSq *
                               kMolarMassOfAir *
                               (altitude - kIsaTropopauseAltitudeFt).Value();

    const double kDemoninator =
        kGasConstant * DegRT(DegFT(kIsaMinimumTempDegF)).Value();

    output *= std::exp(kNumberator / kDemoninator);
  }

  return output;
}

// Page 167 of Modern Exterior Ballistics - McCoy
inline LbsPerCuFtT CalculateAirDensityAtAltitude(
    FeetT altitude, LbsPerCuFtT sea_level_density) {
  const double kHFactorPerFt = 2.926E-5 + (1E-10 * altitude.Value());

  return LbsPerCuFtT(sea_level_density *
                     exp(-1.0 * kHFactorPerFt * altitude.Value()));
}

// Page 167 of Modern Exterior Ballistics - McCoy
inline FpsT CalculateSpeedOfSoundInAir(DegFT temperature) {
  const double kCoeff = 49.0223;
  return FpsT(kCoeff * std::sqrt(DegRT(temperature).Value()));
}

// A Simple Accurate Formula for Calculating Saturation Vapor Pressure of Water
// and Ice - Huang
inline InHgT CalculateWaterVaporSaturationPressure(DegFT temperature) {
  const DegCT kTDegC = temperature;
  const bool kIsWater = kTDegC.Value() > 0;

  const double kAVal = (kIsWater) ? 34.494 : 43.494;
  const double kBVal = (kIsWater) ? 4924.99 : 6545.8;
  const double kCVal = (kIsWater) ? 1.57 : 2.0;
  const double kD1Val = (kIsWater) ? 237.1 : 278.0;
  const double kD2Val = (kIsWater) ? 105.0 : 868.0;

  const PaT kPOutPascal =
      PaT(std::exp(kAVal - (kBVal / (kTDegC.Value() + kD1Val))) /
          std::pow(kTDegC.Value() + kD2Val, kCVal));

  return InHgT(kPOutPascal);
}

// Page 167 of Modern Exterior Ballistics - McCoy
inline double CalculateAirDensityRatio(InHgT pressure, DegFT temperature) {
  return pressure.Value() / kIsaSeaLevelPressureInHg *
         (DegRT(DegFT(kIsaSeaLevelDegF)) / DegRT(temperature)).Value();
}

// Page 167 of Modern Exterior Ballistics - McCoy
inline double CalculateAirDensityRatioHumidityCorrection(
    double humidity_pct, InHgT water_vapor_sat_pressure) {
  const double kAVal = 0.00378;

  return 1.0 - (kAVal * humidity_pct * water_vapor_sat_pressure.Value() /
                kIsaSeaLevelPressureInHg);
}

// Page 168 of Modern Exterior Ballistics - McCoy
inline double CalculateSpeedOfSoundHumidityCorrection(
    double humidity_pct, InHgT water_vapor_sat_pressure) {
  const double kAVal = 0.0014;

  return 1.0 + (kAVal * humidity_pct * water_vapor_sat_pressure.Value() /
                kIsaSeaLevelPressureInHg);
}

// Page 90 of Modern Exterior Ballistics - McCoy
inline double CalculateCdCoefficient(LbsPerCuFtT air_density, PmsiT bc) {
  const double kSqInPerSqFt = (InchT(FeetT(1)) * InchT(FeetT(1))).Value();
  const double kCoeff =
      air_density.Value() * kPi / (bc.Value() * kSqInPerSqFt * 8);
  return kCoeff;
}

// Precision Shooting, March, 43-48 (2005)
// A New Rule for Estimating Rifling Twist An Aid to Choosing Bullets and Rifles
// - Miller
inline double CalculateMillerTwistRuleStabilityFactor(
    InchT bullet_diameter, GrainT bullet_mass, InchT bullet_length,
    InchPerTwistT barrel_twist, FpsT muzzle_velocity) {
  const auto kAVal = 30.0;
  const auto kBVal = 1.0 / 3.0;
  const auto kNominalVelocity = 2'800.0;
  const auto kDiameter = bullet_diameter.Value();
  const auto kMass = static_cast<double>(bullet_mass);
  const auto kLengthRatio = (bullet_length / bullet_diameter).Value();
  const auto kTwistRatio =
      std::abs(barrel_twist.Value() / bullet_diameter.Value());
  const auto kMuzzleVelocity = std::max(FpsT(1120.0), muzzle_velocity).Value();

  const double kFv = std::pow(kMuzzleVelocity / kNominalVelocity, kBVal);

  const double kOutput = (kFv * kAVal * kMass) /
                         (std::pow(kTwistRatio, 2) * std::pow(kDiameter, 3) *
                          kLengthRatio * (1 + std::pow(kLengthRatio, 2)));

  return kOutput * (barrel_twist.Value() >= 0 ? 1.0 : -1.0);
}

inline double CalculateMillerTwistRuleCorrectionFactor(InHgT pressure,
                                                       DegFT temperature) {
  const double kAVal = 460.0;
  return (temperature.Value() + kAVal) / (kIsaSeaLevelDegF + kAVal) *
         (kIsaSeaLevelPressureInHg / pressure.Value());
}

inline double CalculateMillerTwistRuleCorrectionFactor(
    LbsPerCuFtT air_density) {
  return kIsaSeaLevelAirDensityLbsPerCuFt / air_density.Value();
}

// Page 33 of Modern Exterior Ballistics - McCoy
inline SqInT CalculateProjectileReferenceArea(InchT bullet_diameter) {
  return SqInT(std::pow(bullet_diameter, 2).Value() * kPi / 4);
}

inline FtLbsT CalculateKineticEnergy(FpsT velocity, SlugT mass) {
  if (velocity.IsNaN() || mass.IsNaN()) {
    return FtLbsT(0);
  }
  return FtLbsT(mass.Value() * std::pow(velocity.Value(), 2) / 2);
}

inline FpsT CalculateVelocityFromKineticEnergy(FtLbsT energy, SlugT mass) {
  if (!(mass.Value() > 0)) {
    return FpsT(0);
  }
  return FpsT(std::sqrt(2 * energy.Value() / mass.Value()));
}

// Page 90 of Modern Exterior Ballistics - McCoy
inline PmsiT CalculateSectionalDensity(InchT bullet_diameter,
                                       LbsT bullet_mass) {
  assert(bullet_diameter > InchT(0) && "Bullet diameter must be positive");
  return PmsiT(bullet_mass.Value() / std::pow(bullet_diameter.Value(), 2));
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