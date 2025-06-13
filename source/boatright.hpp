// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#pragma once

#include <cassert>
#include <cmath>
#include <cstdint>

#include "calc.hpp"
#include "constants.hpp"
#include "eng_units.hpp"

namespace lob {
namespace boatright {

inline PsiT CalculateDynamicPressure(LbsPerCuFtT air_density, FpsT velocity) {
  const double kRho = air_density.Value() * SlugT(LbsT(1)).Value();
  const double kQ = kRho / 2 * velocity.Value() * velocity.Value();
  const double kSqInPerSqFt = (InchT(FeetT(1)) * InchT(FeetT(1))).Value();
  return PsiT(kQ / kSqInPerSqFt);
}

inline CaliberT CalculateRadiusOfTangentOgive(CaliberT ogive_length,
                                              CaliberT meplat_diameter) {
  const auto kLN = ogive_length;
  const auto kDM = meplat_diameter;
  return (kLN * kLN + std::pow((1 - kDM.Value()) / 2, 2)) / (1 - kDM.Value());
}

inline CaliberT CalculateFullNoseLength(CaliberT ogive_length,
                                        CaliberT meplat_diameter,
                                        CaliberT radius_of_tangent,
                                        double ogive_rtr) {
  const auto kLFT = std::sqrt(radius_of_tangent - 0.25);
  const auto kLFC = ogive_length / (1 - meplat_diameter.Value());
  return (kLFT * ogive_rtr) + (kLFC * (1 - ogive_rtr));
}

inline SqInT CalculateOgiveCrossSectionalArea(InchT x, InchT rho,
                                              double alpha) {
  const auto kA = rho * rho;
  const auto kB = (rho * std::cos(alpha)) - x;
  const auto kC = rho * std::sin(alpha);
  assert(kA > (kB * kB) && "Domain error");
  const auto kY = std::sqrt(kA - (kB * kB)) - kC;
  return SqInT(kPi * kY.Value() * kY.Value());
}

inline double CalculateOgiveSimpsonIntegral(InchT a, InchT b, uint16_t n,
                                            InchT rho, double alpha) {
  const uint16_t kEvenN = (n % 2U == 0) ? n : n + 1U;
  const InchT kH = (b - a) / kEvenN;

  SqInT sum = CalculateOgiveCrossSectionalArea(a, rho, alpha) +
              CalculateOgiveCrossSectionalArea(b, rho, alpha);

  for (uint16_t i = 1U; i < kEvenN; ++i) {
    const auto kX = a + kH * i;
    const auto kMultiple = (i % 2U == 0) ? 2U : 4U;
    sum += CalculateOgiveCrossSectionalArea(kX, rho, alpha) * kMultiple;
  }

  return (kH.Value() / 3) * sum.Value();
}

inline double CalculateOgiveVolume(InchT diameter, InchT ogive_length,
                                   InchT full_ogive_length,
                                   InchT ogive_radius) {
  const InchT kR = diameter / 2.0;
  const InchT kLo = full_ogive_length;
  const InchT kRho = ogive_radius;

  const double kAlpha =
      std::acos(std::sqrt(kR * kR + kLo * kLo).Value() / (kRho * 2.0).Value()) -
      std::atan(kR / kLo).Value();

  const InchT kA = full_ogive_length - ogive_length;
  const InchT kB = full_ogive_length;
  const uint16_t kN = 100;
  return CalculateOgiveSimpsonIntegral(kA, kB, kN, kRho, kAlpha);
}

inline double CalculateFrustrumVolume(InchT d1, InchT d2, InchT length) {
  const double kR1 = d1.Value() / 2;
  const double kR2 = d2.Value() / 2;
  const double kL = length.Value();
  return (kL * kPi / 3 * ((kR1 * kR1) + (kR1 * kR2) + (kR2 * kR2)));
}

inline double CalculateCylinderVolume(InchT diameter, InchT length) {
  return (std::pow(diameter / 2, 2) * kPi * length).Value();
}

inline double CalculateAverageDensity(InchT diameter, InchT length,
                                      InchT ogive_length,
                                      InchT ogive_full_length,
                                      InchT ogive_radius, InchT base_diameter,
                                      InchT tail_length, GrainT mass) {
  const double kOgiveVolume = CalculateOgiveVolume(
      diameter, ogive_length, ogive_full_length, ogive_radius);

  const double kBodyVolume =
      CalculateCylinderVolume(diameter, length - ogive_length - tail_length);

  const double kTailVolume =
      CalculateFrustrumVolume(diameter, base_diameter, tail_length);

  const double kRho = mass.Value() / (kOgiveVolume + kBodyVolume + kTailVolume);
  return kRho;
}

inline double CalculateAverageDensity(InchT diameter, CaliberT length,
                                      CaliberT ogive_length,
                                      CaliberT ogive_full_length,
                                      CaliberT ogive_radius,
                                      CaliberT base_diameter,
                                      CaliberT tail_length, GrainT mass) {
  const InchT kL(length, diameter);
  const InchT kLN(ogive_length, diameter);
  const InchT kLFN(ogive_full_length, diameter);
  const InchT kR(ogive_radius, diameter);
  const InchT kDB(base_diameter, diameter);
  const InchT kLBT(tail_length, diameter);
  return CalculateAverageDensity(diameter, kL, kLN, kLFN, kR, kDB, kLBT, mass);
}

inline double CalculateFastAverageDensity(InchT diameter, InchT length,
                                          InchT meplat_diameter,
                                          InchT ogive_length,
                                          InchT base_diameter,
                                          InchT tail_length, GrainT mass) {
  const double kPinocchioFactor = 1.3;
  const double kOgiveVolume =
      kPinocchioFactor *
      CalculateFrustrumVolume(diameter, meplat_diameter, ogive_length);
  const double kBodyVolume =
      CalculateCylinderVolume(diameter, length - ogive_length - tail_length);
  const double kTailVolume =
      CalculateFrustrumVolume(diameter, base_diameter, tail_length);
  const double kRho = mass.Value() / (kOgiveVolume + kBodyVolume + kTailVolume);
  return kRho;
}

inline double CalculateFastAverageDensity(InchT diameter, CaliberT length,
                                          CaliberT meplat_diameter,
                                          CaliberT ogive_length,
                                          CaliberT base_diameter,
                                          CaliberT tail_length, GrainT mass) {
  const InchT kL(length, diameter);
  const InchT kDM(meplat_diameter, diameter);
  const InchT kLN(ogive_length, diameter);
  const InchT kDB(base_diameter, diameter);
  const InchT kLBT(tail_length, diameter);
  return CalculateFastAverageDensity(diameter, kL, kDM, kLN, kDB, kLBT, mass);
}

inline double CalculateCoefficientOfLift(CaliberT full_ogive_length,
                                         MachT velocity) {
  const double kB = std::sqrt((velocity * velocity) - 1).Value();
  const double kNum1 = 1.974;
  const double kNum2 = 0.921;
  return kNum1 + (kNum2 * (kB / full_ogive_length.Value()));
}

inline double CalculateCLBoattailAdjustmentFactor(PmsiT g7) {
  const double kBoattailAdjustmentFactor = std::sqrt(0.2720 / g7.Value());
  return kBoattailAdjustmentFactor;
}

inline double CalculateInertialRatio(InchT caliber, CaliberT length,
                                     CaliberT ogive_length,
                                     CaliberT full_ogive_length, GrainT mass,
                                     double average_density) {
  const auto kLL = length - ogive_length + full_ogive_length;
  const auto kH = full_ogive_length.Value() / kLL.Value();
  const GrainT kWtCalc =
      GrainT(kPi / 4 * average_density * std::pow(caliber.Value(), 3) *
             kLL.Value() * (1 - 2 * kH / 3));
  const double kF1 = 15 - (12 * kH) +
                     ((kLL * kLL).Value() *
                      (60 - (160 * kH) + (180 * std::pow(kH, 2)) -
                       (96 * std::pow(kH, 3)) + (19 * std::pow(kH, 4))) /
                      (3 - (2 * kH)));
  const double kIyOverIx =
      std::pow(mass / kWtCalc, 0.894).Value() * kF1 / (30 * (1 - (4 * kH / 5)));
  return kIyOverIx;
}

inline HzT CalculateSpinRate(FpsT velocity, InchPerTwistT twist) {
  const double kInchesPerFoot = InchT(FeetT(1)).Value();
  return HzT(kInchesPerFoot * velocity.Value() / std::abs(twist.Value()));
}

inline double CalculateAspectRatio(CaliberT length, CaliberT full_ogive_length,
                                   CaliberT tail_length,
                                   CaliberT base_diameter) {
  const double kAR =
      length.Value() -
      ((2.0 / 3.0) * (full_ogive_length.Value() +
                      (tail_length.Value() * (1.0 - base_diameter.Value()))));
  return kAR;
}

inline double CalculateYawDragCoefficient(MachT speed,
                                          double coefficient_of_lift,
                                          double aspect_ratio) {
  const double kCLSquared = coefficient_of_lift * coefficient_of_lift;
  const double kCDa =
      1.33 * (1.41 - 0.18 * speed.Value()) *
      (9.825 - 3.95 * speed.Value() +
       (0.1458 * speed.Value() - 0.1594) * kCLSquared * aspect_ratio);
  return kCDa;
}

inline double CalculateEpicyclicRatio(double stability) {
  const double kSg = std::abs(stability);
  const double kR = (2 * (kSg + std::sqrt(kSg * (kSg - 1)))) - 1;
  return kR;
}

inline uint16_t CalculateNutationCyclesNeeded(double epicyclic_ratio) {
  const auto kN =
      static_cast<uint16_t>(std::floor((epicyclic_ratio - 1) / 4) + 1);
  return kN;
}

inline HzT CalculateGyroscopicRateSum(HzT spin_rate, double inertial_ratio) {
  return spin_rate / inertial_ratio;
}

inline HzT CalculateGyroscopicRateF2(HzT gyroscopic_rate_sum,
                                     double epicyclic_ratio) {
  return gyroscopic_rate_sum / (epicyclic_ratio + 1);
}

inline SecT CalculateFirstNutationPeriod(HzT f1, HzT f2) {
  assert(f1 > f2);
  const SecT kTn(1 / (f1.Value() - f2.Value()));
  return kTn;
}

inline double CalculateCrosswindAngleGamma(MphT zwind, FpsT velocity) {
  const double kGamma = FpsT(zwind).Value() / velocity.Value();
  return kGamma;
}

inline double CalculateZeroYawDragCoefficientOfDrag(double cd_ref, GrainT mass,
                                                    InchT diameter, PmsiT bc) {
  const double kCD0 = cd_ref * (LbsT(mass).Value() /
                                (diameter * diameter).Value() / bc.Value());
  return kCD0;
}

inline double CalculateYawDragAdjustment(double gamma, double r, double cda) {
  const double kEpicyclicSwerveMagnitude = gamma * r / (r - 1);
  const double kAdjustment = std::pow(kEpicyclicSwerveMagnitude, 2) * cda;
  return kAdjustment;
}

inline double CalculateVerticalPitch(double gamma, double r, double n) {
  const double kPitch = gamma * (((r * r) - 1) / (n * 2 * kPi * r)) *
                        (1 - std::cos(n * 2 * kPi / (r - 1)));
  return kPitch;
}

inline double CalculateVerticalImpulse(InchPerTwistT twist, uint16_t n, SecT tn,
                                       PsiT q, SqInT s, double cl, double cd,
                                       double pitch) {
  const double kSign = twist.Value() < 0 ? -1.0 : 1.0;
  const double kJv = kSign * (n * tn.Value()) * (q.Value() * s.Value()) *
                     (cl + cd) * std::sin(pitch);
  return kJv;
}

inline double CalculateMagnitudeOfMomentum(GrainT mass, FpsT velocity) {
  const double kMOM =
      LbsT(mass).Value() / kStandardGravityFtPerSecSq * velocity.Value();
  return kMOM;
}

inline MoaT CalculateAerodynamicJump(
    InchT diameter, InchT meplat_diameter, InchT base_diameter, InchT length,
    InchT ogive_length, InchT tail_length, double ogive_rtr, GrainT mass,
    FpsT velocity, double stability, InchPerTwistT twist, FpsT zwind,
    LbsPerCuFtT air_density, FpsT speed_of_sound, PmsiT bc, double cd_ref) {
  const CaliberT kDM(meplat_diameter, diameter.Inverse());
  const CaliberT kDB(base_diameter, diameter.Inverse());
  const CaliberT kL(length, diameter.Inverse());
  const CaliberT kLN(ogive_length, diameter.Inverse());
  const CaliberT kLBT(tail_length, diameter.Inverse());
  const auto kRTR(ogive_rtr);
  const CaliberT kRT = boatright::CalculateRadiusOfTangentOgive(kLN, kDM);
  const CaliberT kLFN = boatright::CalculateFullNoseLength(kLN, kDM, kRT, kRTR);
  const PsiT kQ = boatright::CalculateDynamicPressure(air_density, velocity);
  const SqInT kS = CalculateProjectileReferenceArea(diameter);
  const auto kAR = boatright::CalculateAspectRatio(kL, kLFN, kLBT, kDB);
  const auto kM = lob::MachT(velocity, speed_of_sound.Inverse());
  const auto kCL = boatright::CalculateCoefficientOfLift(kLFN, kM);
  const auto kCDa = boatright::CalculateYawDragCoefficient(kM, kCL, kAR);
  const auto kRho = boatright::CalculateFastAverageDensity(
      diameter, kL, kDM, kLN, kDB, kLBT, mass);
  const auto kIyOverIx =
      boatright::CalculateInertialRatio(diameter, kL, kLN, kLFN, mass, kRho);
  const auto kP = boatright::CalculateSpinRate(velocity, twist);
  const auto kR = boatright::CalculateEpicyclicRatio(stability);
  const auto kN = boatright::CalculateNutationCyclesNeeded(kR);
  const auto kF1F2Sum = boatright::CalculateGyroscopicRateSum(kP, kIyOverIx);
  const auto kF2 = boatright::CalculateGyroscopicRateF2(kF1F2Sum, kR);
  const auto kTn = boatright::CalculateFirstNutationPeriod(kF1F2Sum - kF2, kF2);
  const auto kGamma =
      boatright::CalculateCrosswindAngleGamma(MphT(zwind), velocity);
  const auto kCD0 = boatright::CalculateZeroYawDragCoefficientOfDrag(
      cd_ref, mass, diameter, bc);
  const auto kCDAdjustment =
      boatright::CalculateYawDragAdjustment(kGamma, kR, kCDa);
  const auto kCD = kCD0 + kCDAdjustment;
  const auto kPitch = boatright::CalculateVerticalPitch(kGamma, kR, kN);
  const auto kJv = boatright::CalculateVerticalImpulse(twist, kN, kTn, kQ, kS,
                                                       kCL, kCD, kPitch);
  const auto kMOM = boatright::CalculateMagnitudeOfMomentum(mass, velocity);
  const auto kJump = -1 * kJv / kMOM;
  return MoaT(RadiansT(kJump));
}

inline double CalculateKV(FpsT initial_velocity, FpsT target_velocity) {
  return std::log(target_velocity.Value() / initial_velocity.Value());
}

inline double CalculateKOmega(InchT diameter, SecT supersonic_time) {
  const double kA = 0.585;
  const double kB = 0.0321;
  return -1 * (kA + kB / diameter.Value()) * supersonic_time.Value();
}

inline RadiansT CalculateYawOfRepose(FpsT initial_velocity, InchPerTwistT twist,
                                     double inertial_ratio,
                                     double epicyclic_ratio, double komega,
                                     double kv) {
  const double kSum = komega + kv;
  const double kFeetPerTurn = twist.Value() / 12.0;

  const double kNumerator = -1 * kStandardGravityFtPerSecSq * kFeetPerTurn *
                            inertial_ratio * (epicyclic_ratio + 1.0) *
                            (std::exp(-kSum) - 1.0);
  const double kDenominator =
      (initial_velocity * initial_velocity).Value() * kSum;
  assert(!AreEqual(kDenominator, 0.0) && "Denominator must not be zero.");
  return RadiansT(kNumerator / kDenominator);
}

inline double CalculatePotentialDragForce(InchT diameter,
                                          LbsPerCuFtT air_density,
                                          FpsT target_velocity) {
  const auto kS = SqFtT(CalculateProjectileReferenceArea(diameter));
  const double kQTS = kS.Value() *
                      ((air_density.Value() / kStandardGravityFtPerSecSq) / 2) *
                      target_velocity.Value() * target_velocity.Value();
  return kQTS;  // Lbs of force
}

inline double CalculateCoefficientOfLiftAtT(double cl0, FpsT initial_velocity,
                                            SecT supersonic_time) {
  const double kA = std::pow(initial_velocity / FpsT(2600), 2.0).Value();
  const double kB = 1.430 / supersonic_time.Value();
  const double kExponent = -0.3711 * kA * kB;
  return cl0 * std::exp(kExponent);
}

inline double CalculateSpinDriftScaleFactor(double potential_drag_force,
                                            RadiansT yaw_of_repose,
                                            double coefficent_of_lift,
                                            GrainT mass) {
  const double kNumerator = 0.388132 * potential_drag_force *
                            yaw_of_repose.Value() * coefficent_of_lift;
  return kNumerator / LbsT(mass).Value();
}

inline InchT CalculateSpinDrift(double scale_factor, InchT drop) {
  return drop * scale_factor;
}

}  // namespace boatright
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