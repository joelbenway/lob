// This file is a part of lob, an exterior ballistics calculation library
// Copyright (c) 2025  Joel Benway
// Please see end of file for extended copyright information

#include "calc.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "constants.hpp"
#include "eng_units.hpp"

namespace lob {

DegFT CalculateTemperatureAtAltitude(FeetT altitude, DegFT temperature) {
  const DegFT kTemperature =
      temperature - DegFT(kIsaLapseDegFPerFt * altitude.Value());
  return std::max(kTemperature, DegFT(kIsaMinimumTempDegF));
}

// Page 166 of Modern Exterior Ballistics - McCoy
DegFT CalculateTemperatureAtAltitudeMcCoy(FeetT altitude,
                                          DegFT sea_level_temperature) {
  const double kK = 6.858E-6 + 2.776E-11 * altitude.Value();
  const double kA = DegRT(DegFT(0)).Value();
  // Note that the formula printed in 2e of Modern External Ballistics omits the
  // negative sign. This is remedied here.
  return (sea_level_temperature + kA) * std::exp(-1.0 * kK * altitude.Value()) -
         kA;
}

// https://wikipedia.org/wiki/Barometric_formula
InHgT BarometricFormula(FeetT altitude, InHgT pressure, DegFT temperature) {
  const double kGasConstant = 1716.49;     // ft-lb / slug^{-1}R^{-1}
  const double kMolarMassOfAir = 28.9644;  // lb/lb-mol
  const FeetT kHeight = std::min(altitude, FeetT(kIsaTropopauseAltitudeFt));

  const double kExponent =
      kStandardGravity / (kGasConstant * kIsaLapseDegFPerFt);

  const double kBase =
      1.0 - (kIsaLapseDegFPerFt * kHeight.Value() / DegRT(temperature).Value());

  InHgT output = pressure * std::pow(kBase, kExponent);

  if (altitude > FeetT(kIsaTropopauseAltitudeFt)) {
    const double kNumberator = -1.0 * kStandardGravity * kMolarMassOfAir *
                               (altitude - kIsaTropopauseAltitudeFt).Value();

    const double kDemoninator =
        kGasConstant * DegRT(DegFT(kIsaMinimumTempDegF)).Value();

    output *= std::exp(kNumberator / kDemoninator);
  }

  return output;
}

// Page 167 of Modern Exterior Ballistics - McCoy
LbsPerCuFtT CalculateAirDensityAtAltitude(FeetT altitude,
                                          LbsPerCuFtT sea_level_density) {
  const double kHFactorPerFt = 2.926E-5 + 1E-10 * altitude.Value();

  return LbsPerCuFtT(sea_level_density *
                     exp(-1.0 * kHFactorPerFt * altitude.Value()));
}

// Page 167 of Modern Exterior Ballistics - McCoy
FpsT CalculateSpeedOfSoundInAir(DegFT temperature) {
  const double kCoeff = 49.0223;
  return FpsT(kCoeff * std::sqrt(DegRT(temperature).Value()));
}

// A Simple Accurate Formula for Calculating Saturation Vapor Pressure of Water
// and Ice - Huang
InHgT CalculateWaterVaporSaturationPressure(DegFT temperature) {
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
double CalcualteAirDensityRatio(InHgT pressure, DegFT temperature) {
  return pressure.Value() / kIsaSeaLevelPressureInHg *
         (DegRT(DegFT(kIsaSeaLevelDegF)) / DegRT(temperature)).Value();
}

// Page 167 of Modern Exterior Ballistics - McCoy
double CalculateAirDensityRatioHumidityCorrection(
    double humidity_pct, InHgT water_vapor_sat_pressure) {
  const double kAVal = 0.00378;

  return 1.0 - kAVal * humidity_pct * water_vapor_sat_pressure.Value() /
                   kIsaSeaLevelPressureInHg;
}

// Page 168 of Modern Exterior Ballistics - McCoy
double CalculateSpeedOfSoundHumidityCorrection(double humidity_pct,
                                               InHgT water_vapor_sat_pressure) {
  const double kAVal = 0.0014;

  return 1.0 + kAVal * humidity_pct * water_vapor_sat_pressure.Value() /
                   kIsaSeaLevelPressureInHg;
}

// Page 90 of Modern Exterior Ballistics - McCoy
double CalculateCdCoefficent(LbsPerCuFtT air_density, PmsiT bc) {
  const double kSqInPerSqFt = (InchT(FeetT(1)) * InchT(FeetT(1))).Value();
  const uint8_t kClangTidyPleaserEight = 8;
  return air_density.Value() * kPi /
         (bc.Value() * kSqInPerSqFt * kClangTidyPleaserEight);
}

// Precision Shooting, March, 43-48 (2005)
// A New Rule for Estimating Rifling Twist An Aid to Choosing Bullets and Rifles
// - Miller
double CalculateMillerTwistRuleStabilityFactor(InchT bullet_diameter,
                                               GrainT bullet_mass,
                                               InchT bullet_length,
                                               InchPerTwistT barrel_twist,
                                               FpsT muzzle_velocity) {
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

double CalculateMillerTwistRuleCorrectionFactor(InHgT pressure,
                                                DegFT temperature) {
  const double kAVal = 460.0;
  return (temperature.Value() + kAVal) / (kIsaSeaLevelDegF + kAVal) *
         (kIsaSeaLevelPressureInHg / pressure.Value());
}

double CalculateMillerTwistRuleCorrectionFactor(LbsPerCuFtT air_density) {
  return kIsaSeaLevelAirDensityLbsPerCuFt / air_density.Value();
}

// Page 97 of Applied Ballistics for Long-Range Shooting 3e - Litz
InchT CalculateLitzGyroscopicSpinDrift(double stability, SecT time) {
  const double kAVal = 1.25 * (stability >= 0 ? 1.0 : -1.0);
  const double kBVal = 1.2;
  const double kExponent = 1.83;

  return InchT(kAVal * (std::abs(stability) + kBVal) *
               std::pow(time.Value(), kExponent));
}

// Page 422 of Applied Ballistics for Long-Range Shooting 3e - Litz
MoaT CalculateLitzAerodynamicJump(double stability, InchT caliber, InchT length,
                                  MphT l2r_crosswind) {
  const double kSgCoeff = 0.01;
  const double kLCoeff = 0.0024;
  const double kIntercept = 0.032;
  const double kY = kSgCoeff * std::abs(stability) -
                    kLCoeff * (length / caliber).Value() + kIntercept;
  const double kDirection = stability >= 0 ? -1.0 : 1.0;
  return MoaT(kDirection * kY * l2r_crosswind.Value());
}

// Page 33 of Modern Exterior Ballistics - McCoy
SqFtT CalculateProjectileReferenceArea(InchT bullet_diameter) {
  return SqFtT((std::pow(FeetT(bullet_diameter), 2) * kPi / 4).Value());
}

FtLbsT CalculateKineticEnergy(FpsT velocity, SlugT mass) {
  return FtLbsT(mass.Value() * std::pow(velocity.Value(), 2) / 2);
}

// Page 90 of Modern Exterior Ballistics - McCoy
PmsiT CalculateSectionalDensity(InchT bullet_diameter, LbsT bullet_mass) {
  return PmsiT(bullet_mass.Value() / std::pow(bullet_diameter.Value(), 2));
}

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