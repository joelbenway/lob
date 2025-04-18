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
  const double kK = 6.858E-6 + (2.776E-11 * altitude.Value());
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
LbsPerCuFtT CalculateAirDensityAtAltitude(FeetT altitude,
                                          LbsPerCuFtT sea_level_density) {
  const double kHFactorPerFt = 2.926E-5 + (1E-10 * altitude.Value());

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

  return 1.0 - (kAVal * humidity_pct * water_vapor_sat_pressure.Value() /
                kIsaSeaLevelPressureInHg);
}

// Page 168 of Modern Exterior Ballistics - McCoy
double CalculateSpeedOfSoundHumidityCorrection(double humidity_pct,
                                               InHgT water_vapor_sat_pressure) {
  const double kAVal = 0.0014;

  return 1.0 + (kAVal * humidity_pct * water_vapor_sat_pressure.Value() /
                kIsaSeaLevelPressureInHg);
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
                                  MphT zwind) {
  const double kSgCoeff = 0.01;
  const double kLCoeff = 0.0024;
  const double kIntercept = 0.032;
  const double kY = (kSgCoeff * std::abs(stability)) -
                    (kLCoeff * (length / caliber).Value()) + kIntercept;
  const double kDirection = stability >= 0 ? -1.0 : 1.0;
  return MoaT(kDirection * kY * zwind.Value());
}

// Page 33 of Modern Exterior Ballistics - McCoy
SqInT CalculateProjectileReferenceArea(InchT bullet_diameter) {
  return SqInT(std::pow(bullet_diameter, 2).Value() * kPi / 4);
}

FtLbsT CalculateKineticEnergy(FpsT velocity, SlugT mass) {
  return FtLbsT(mass.Value() * std::pow(velocity.Value(), 2) / 2);
}

// Page 90 of Modern Exterior Ballistics - McCoy
PmsiT CalculateSectionalDensity(InchT bullet_diameter, LbsT bullet_mass) {
  return PmsiT(bullet_mass.Value() / std::pow(bullet_diameter.Value(), 2));
}

namespace cwaj {
PsiT CalculateDynamicPressure(LbsPerCuFtT air_density, FpsT velocity) {
  const double kRho = air_density.Value() * SlugT(LbsT(1)).Value();
  const double kQ = kRho / 2 * velocity.Value() * velocity.Value();
  const double kSqInPerSqFt = (InchT(FeetT(1)) * InchT(FeetT(1))).Value();
  return PsiT(kQ / kSqInPerSqFt);
}

CaliberT CalculateRadiusOfTangentOgive(CaliberT nose_length,
                                       CaliberT meplat_diameter) {
  const auto kLN = nose_length;
  const auto kDM = meplat_diameter;
  return (kLN * kLN + std::pow((1 - kDM.Value()) / 2, 2)) / (1 - kDM.Value());
}

CaliberT CalculateFullNoseLength(CaliberT nose_length, CaliberT meplat_diameter,
                                 double ogive_rtr) {
  const auto kRT = CalculateRadiusOfTangentOgive(nose_length, meplat_diameter);
  const auto kLFT = std::sqrt(kRT - 0.25);
  const auto kLFC = nose_length / (1 - meplat_diameter.Value());
  return (kLFT * ogive_rtr) + (kLFC * (1 - ogive_rtr));
}

double CalculateRelativeDensity(InchT diameter, InchT length,
                                InchT meplat_diameter, InchT nose_length,
                                InchT base_diameter, InchT base_length,
                                GrainT mass) {
  auto frustum_volume = [](double r1, double r2, double length) {
    return length * kPi / 3 * ((r1 * r1) + (r1 * r2) + (r2 * r2));
  };
  // Modeling the nose as a frustum ensures volume is underestimated.
  const double kNoseFudgeFactor = 1.1;
  const double kNoseVolume =
      kNoseFudgeFactor * frustum_volume(diameter.Value() / 2,
                                        meplat_diameter.Value() / 2,
                                        nose_length.Value());
  const double kBodyVolume =
      (std::pow(diameter / 2, 2) * kPi * (length - nose_length - base_length))
          .Value();
  const double kBaseVolume = frustum_volume(
      diameter.Value() / 2, base_diameter.Value() / 2, base_length.Value());

  return mass.Value() / (kNoseVolume + kBodyVolume + kBaseVolume);
}

double CalculateCoefficentOfLift(CaliberT nose_length, CaliberT meplat_diameter,
                                 double ogive_rtr, MachT velocity) {
  const auto kLFN =
      CalculateFullNoseLength(nose_length, meplat_diameter, ogive_rtr);
  const double kB = std::sqrt((velocity * velocity) - 1).Value();
  const double kNum1 = 1.974;
  const double kNum2 = 0.921;
  return kNum1 + (kNum2 * kB / kLFN.Value());
}

double CalculateInertialRatio(InchT caliber, CaliberT length,
                              CaliberT nose_length, CaliberT full_nose_length,
                              GrainT mass, double relative_density) {
  const auto kLL = length - nose_length + full_nose_length;
  const auto kH = full_nose_length.Value() / kLL.Value();
  const GrainT kWtCalc =
      GrainT(kPi / 4 * relative_density * std::pow(caliber.Value(), 3) *
             kLL.Value() * (1 - 2 * kH / 3));
  const double kF1 = 15 - (12 * kH) +
                     ((kLL * kLL).Value() *
                      (60 - (160 * kH) + (180 * std::pow(kH, 2)) -
                       (96 * std::pow(kH, 3)) + (19 * std::pow(kH, 4))) /
                      (3 - (2 * kH)));
  const double kIyIxRatio =
      std::pow(mass / kWtCalc, 0.894).Value() * kF1 / (30 * (1 - (4 * kH / 5)));
  return kIyIxRatio;
}

HzT CalculateSpinRate(FpsT velocity, InchPerTwistT twist) {
  const double kInchesPerFoot = InchT(FeetT(1)).Value();
  return HzT(kInchesPerFoot * velocity.Value() / std::abs(twist.Value()));
}

double CalculateAspectRatio(CaliberT length, CaliberT full_nose_length,
                            CaliberT boat_tail_length, CaliberT base_diameter) {
  const double kAR =
      length.Value() - ((2.0 / 3.0) * (full_nose_length.Value() +
                                       (boat_tail_length.Value() *
                                        (1.0 - base_diameter.Value()))));
  return kAR;
}

double CalculateYawDragCoefficent(MachT speed, double coefficent_of_lift,
                                  double aspect_ratio) {
  const double kCLSquared = coefficent_of_lift * coefficent_of_lift;
  const double kCDa =
      1.33 * (1.41 - 0.18 * speed.Value()) *
      (9.825 - 3.95 * speed.Value() +
       (0.1458 * speed.Value() - 0.1594) * kCLSquared * aspect_ratio);
  return kCDa;
}

double CalculateEpicyclicRatio(double stability) {
  const double kSg = std::abs(stability);
  const double kR = (2 * (kSg + std::sqrt(kSg * (kSg - 1)))) - 1;
  return kR;
}

uint16_t CalculateNutationCyclesNeeded(double epicyclic_ratio) {
  const auto kN =
      static_cast<uint16_t>(std::floor((epicyclic_ratio - 1) / 4) + 1);
  return kN;
}

HzT CalculateGyroscopicRateSum(HzT spin_rate, double inertial_ratio) {
  return spin_rate / inertial_ratio;
}

HzT CalculateGyroscopicRateF2(HzT gyroscopic_rate_sum, double epicyclic_ratio) {
  return gyroscopic_rate_sum / (epicyclic_ratio + 1);
}

SecT CalculateFirstNutationPeriod(HzT f1, HzT f2) {
  const SecT kTn(1 / (f1.Value() - f2.Value()));
  return kTn;
}

double CalculateCrosswindAngleGamma(MphT zwind, FpsT velocity) {
  const double kGamma = FpsT(zwind).Value() / velocity.Value();
  return kGamma;
}

double CalculateZeroYawDragCoefficientOfDrag(double cd_ref, GrainT mass,
                                             InchT diameter, PmsiT bc) {
  const double kCD0 = cd_ref * (LbsT(mass).Value() /
                                (diameter * diameter).Value() / bc.Value());
  return kCD0;
}

double CalculateYawDragAdjustment(double gamma, double r, double cda) {
  const double kEpicyclicSwerveMagnitude = gamma * r / (r - 1);
  const double kAdjustment = std::pow(kEpicyclicSwerveMagnitude, 2) * cda;
  return kAdjustment;
}

double CalculateVerticalPitch(double gamma, double r, double n) {
  const double kPitch = gamma * (((r * r) - 1) / (n * 2 * kPi * r)) *
                        (1 - std::cos(n * 2 * kPi / (r - 1)));
  return kPitch;
}

double CalculateVerticalImpulse(InchPerTwistT twist, uint16_t n, SecT tn,
                                PsiT q, SqInT s, double cl, double cd,
                                double pitch) {
  const double kSign = twist.Value() < 0 ? -1.0 : 1.0;
  const double kJv = kSign * (n * tn.Value()) * (q.Value() * s.Value()) *
                     (cl + cd) * std::sin(pitch);
  return kJv;
}

double CalculateMagnitudeOfMomentum(GrainT mass, FpsT velocity) {
  const double kMOM =
      LbsT(mass).Value() / kStandardGravityFtPerSecSq * velocity.Value();
  return kMOM;
}

}  // namespace cwaj

MoaT CalculateBRAerodynamicJump(InchT diameter, InchT meplat_diameter,
                                InchT base_diameter, InchT length,
                                InchT nose_length, InchT boat_tail_length,
                                double ogive_rtr, GrainT mass, FpsT velocity,
                                double stability, InchPerTwistT twist,
                                FpsT zwind, LbsPerCuFtT air_density,
                                FpsT speed_of_sound, PmsiT bc, double cd_ref) {
  const CaliberT kDM(meplat_diameter, diameter.Inverse());
  const CaliberT kDB(base_diameter, diameter.Inverse());
  const CaliberT kL(length, diameter.Inverse());
  const CaliberT kLN(nose_length, diameter.Inverse());
  const CaliberT kLBT(boat_tail_length, diameter.Inverse());
  const auto kRTR(ogive_rtr);
  const CaliberT kLFN = cwaj::CalculateFullNoseLength(kLN, kDM, kRTR);
  const PsiT kQ = cwaj::CalculateDynamicPressure(air_density, velocity);
  const SqInT kS = CalculateProjectileReferenceArea(diameter);
  const auto kAR = cwaj::CalculateAspectRatio(kL, kLFN, kLBT, kDB);
  const auto kM = lob::MachT(velocity, speed_of_sound.Inverse());
  const auto kCL = cwaj::CalculateCoefficentOfLift(kLN, kDM, kRTR, kM);
  const auto kCDa = cwaj::CalculateYawDragCoefficent(kM, kCL, kAR);
  const auto kRD = cwaj::CalculateRelativeDensity(
      diameter, length, meplat_diameter, nose_length, base_diameter,
      boat_tail_length, mass);
  const auto kIyPerIx =
      cwaj::CalculateInertialRatio(diameter, kL, kLN, kLFN, mass, kRD);
  const auto kP = cwaj::CalculateSpinRate(velocity, twist);
  const auto kR = cwaj::CalculateEpicyclicRatio(stability);
  const auto kN = cwaj::CalculateNutationCyclesNeeded(kR);
  const auto kF1F2Sum = cwaj::CalculateGyroscopicRateSum(kP, kIyPerIx);
  const auto kF2 = cwaj::CalculateGyroscopicRateF2(kF1F2Sum, kR);
  const auto kTn = cwaj::CalculateFirstNutationPeriod(kF1F2Sum - kF2, kF2);
  const auto kGamma = cwaj::CalculateCrosswindAngleGamma(zwind, velocity);
  const auto kCD0 =
      cwaj::CalculateZeroYawDragCoefficientOfDrag(cd_ref, mass, diameter, bc);
  const auto kCDAdjustment = cwaj::CalculateYawDragAdjustment(kGamma, kR, kCDa);
  const auto kCD = kCD0 + kCDAdjustment;
  const auto kPitch = cwaj::CalculateVerticalPitch(kGamma, kR, kN);
  const auto kJv =
      cwaj::CalculateVerticalImpulse(twist, kN, kTn, kQ, kS, kCL, kCD, kPitch);
  const auto kMOM = cwaj::CalculateMagnitudeOfMomentum(mass, velocity);
  const auto kJump = -1 * kJv / kMOM;
  return MoaT(RadiansT(kJump));
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