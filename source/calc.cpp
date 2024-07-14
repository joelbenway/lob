// This file is a part of lob, an exterior ballistics calculation library
// Copyright (c) 2024  Joel Benway
// Please see end of file for extended copyright information

#include "calc.hpp"

#include <algorithm>
#include <cmath>

#include "constants.hpp"
#include "eng_units.hpp"

namespace lob {

DegFT CalculateTemperatureAtAltitude(FeetT altitude) {
  const DegFT kTemperature =
      DegFT(kIsaSeaLevelDegF) - DegFT(kIsaLapseDegFPerFt * altitude.Value());
  return std::max(kTemperature, DegFT(kIsaMinimumTempDegF));
}

InHgT BarometricFormula(FeetT altitude) {
  const double kGasConstant =
      8.9494596E4 / DegRT(DegKT(1)).Value();   // lb·ft^2/(lb-mol·R·s^2)
  constexpr double kMolarMassOfAir = 28.9644;  // lb/lb-mol
  InHgT pressure = InHgT(0.0);
  const FeetT kHeight = std::min(altitude, FeetT(kIsaTropopauseAltitudeFt));

  const double kExponent =
      kStandardGravity * kMolarMassOfAir / (kIsaLapseDegFPerFt * kGasConstant);

  const double kBase = 1.0 - kIsaLapseDegFPerFt * kHeight.Value() /
                                 DegRT(DegFT(kIsaSeaLevelDegF)).Value();

  pressure = InHgT(kIsaSeaLevelPressureInHg) * std::pow(kBase, kExponent);

  if (altitude > FeetT(kIsaTropopauseAltitudeFt)) {
    const double kNumberator = -1.0 * kStandardGravity * kMolarMassOfAir *
                               (altitude.Value() - kIsaTropopauseAltitudeFt);

    const double kDemoninator =
        kGasConstant * DegRT(DegFT(kIsaMinimumTempDegF)).Value();

    pressure *= std::exp(kNumberator / kDemoninator);
  }

  return pressure;
}

LbsPerCuFtT CalculateAirDensityAtAltitude(FeetT altitude) {
  const double kHFactorPerFt = 2.926E-5 + 1E-10 * altitude.Value();

  return LbsPerCuFtT(kIsaSeaLevelAirDensityLbsPerCuFt *
                     exp(-1.0 * kHFactorPerFt * altitude.Value()));
}

FpsT CalculateSpeedOfSoundInAir(DegFT temperature) {
  const double kCoeff = 49.0223;
  return FpsT(kCoeff * std::sqrt(DegRT(temperature).Value()));
}

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

double CalcualteAirDensityRatio(InHgT pressure, DegFT temperature) {
  const double kAVal = 518.67;

  return pressure.Value() / kIsaSeaLevelPressureInHg * kAVal /
         DegRT(temperature).Value();
}

double CalculateAirDensityRatioHumidityCorrection(
    double humidity_pct, InHgT water_vapor_sat_pressure) {
  const double kAVal = 0.00378;

  return 1.0 - kAVal * humidity_pct * water_vapor_sat_pressure.Value() /
                   kIsaSeaLevelPressureInHg;
}

double CalculateSpeedOfSoundHumidityCorrection(double humidity_pct,
                                               InHgT water_vapor_sat_pressure) {
  const double kAVal = 0.0014;

  return 1.0 + kAVal * humidity_pct * water_vapor_sat_pressure.Value() /
                   kIsaSeaLevelPressureInHg;
}

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

  const auto kFv = std::pow(kMuzzleVelocity / kNominalVelocity, kBVal);

  return (kFv * kAVal * kMass) /
         (std::pow(kTwistRatio, 2) * std::pow(kDiameter, 3) * kLengthRatio *
          (1 + std::pow(kLengthRatio, 2)));
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

InchT CalculateGyroscopicSpinDrift(double stability, SecT time,
                                   bool is_rh_twist) {
  const double kAVal = 1.25 * (is_rh_twist ? 1.0 : -1.0);
  const double kBVal = 1.2;
  const double kExponent = 1.83;

  return InchT(kAVal * (stability + kBVal) * std::pow(time.Value(), kExponent));
}

SqFtT CalculateProjectileReferenceArea(InchT bullet_diameter) {
  return SqFtT((std::pow(FeetT(bullet_diameter), 2) * kPi / 4).Value());
}

FtLbsT CalculateKineticEnergy(FpsT velocity, SlugT mass) {
  return FtLbsT(mass.Value() * std::pow(velocity.Value(), 2) / 2);
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