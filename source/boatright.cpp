// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include "boatright.hpp"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>

#include "calc.hpp"
#include "constants.hpp"
#include "eng_units.hpp"
#include "helpers.hpp"

namespace lob {

namespace boatright {
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
  auto frustum_volume = [](double r1, double r2, double height) {
    return height * kPi / 3 * ((r1 * r1) + (r1 * r2) + (r2 * r2));
  };
  // Modeling the nose as a frustum ensures volume is underestimated. This
  // factor makes it a little larger for an improved estimate.
  const double kPinocchioFactor = 1.1;
  const double kNoseVolume =
      kPinocchioFactor * frustum_volume(diameter.Value() / 2,
                                        meplat_diameter.Value() / 2,
                                        nose_length.Value());

  const double kBodyVolume =
      (std::pow(diameter / 2, 2) * kPi * (length - nose_length - base_length))
          .Value();
  const double kBaseVolume = frustum_volume(
      diameter.Value() / 2, base_diameter.Value() / 2, base_length.Value());

  const double kR1 = diameter.Value() / 2;
  const double kR2 = base_diameter.Value() / 2;
  const double kH = base_length.Value();

  return mass.Value() / (kNoseVolume + kBodyVolume + kBaseVolume);
}

double CalculateCoefficientOfLift(CaliberT nose_length,
                                  CaliberT meplat_diameter, double ogive_rtr,
                                  MachT velocity) {
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
  const double kIyOverIx =
      std::pow(mass / kWtCalc, 0.894).Value() * kF1 / (30 * (1 - (4 * kH / 5)));
  return kIyOverIx;
}

HzT CalculateSpinRate(FpsT velocity, InchPerTwistT twist) {
  const double kInchesPerFoot = InchT(FeetT(1)).Value();
  return HzT(kInchesPerFoot * velocity.Value() / std::abs(twist.Value()));
}

double CalculateAspectRatio(CaliberT length, CaliberT full_nose_length,
                            CaliberT tail_length, CaliberT base_diameter) {
  const double kAR =
      length.Value() -
      ((2.0 / 3.0) * (full_nose_length.Value() +
                      (tail_length.Value() * (1.0 - base_diameter.Value()))));
  return kAR;
}

double CalculateYawDragCoefficient(MachT speed, double coefficient_of_lift,
                                   double aspect_ratio) {
  const double kCLSquared = coefficient_of_lift * coefficient_of_lift;
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

}  // namespace boatright

MoaT CalculateBRAerodynamicJump(InchT diameter, InchT meplat_diameter,
                                InchT base_diameter, InchT length,
                                InchT nose_length, InchT tail_length,
                                double ogive_rtr, GrainT mass, FpsT velocity,
                                double stability, InchPerTwistT twist,
                                FpsT zwind, LbsPerCuFtT air_density,
                                FpsT speed_of_sound, PmsiT bc, double cd_ref) {
  const CaliberT kDM(meplat_diameter, diameter.Inverse());
  const CaliberT kDB(base_diameter, diameter.Inverse());
  const CaliberT kL(length, diameter.Inverse());
  const CaliberT kLN(nose_length, diameter.Inverse());
  const CaliberT kLBT(tail_length, diameter.Inverse());
  const auto kRTR(ogive_rtr);
  const CaliberT kLFN = boatright::CalculateFullNoseLength(kLN, kDM, kRTR);
  const PsiT kQ = boatright::CalculateDynamicPressure(air_density, velocity);
  const SqInT kS = CalculateProjectileReferenceArea(diameter);
  const auto kAR = boatright::CalculateAspectRatio(kL, kLFN, kLBT, kDB);
  const auto kM = lob::MachT(velocity, speed_of_sound.Inverse());
  const auto kCL = boatright::CalculateCoefficientOfLift(kLN, kDM, kRTR, kM);
  const auto kCDa = boatright::CalculateYawDragCoefficient(kM, kCL, kAR);
  const auto kRD = boatright::CalculateRelativeDensity(
      diameter, length, meplat_diameter, nose_length, base_diameter,
      tail_length, mass);
  const auto kIyOverIx =
      boatright::CalculateInertialRatio(diameter, kL, kLN, kLFN, mass, kRD);
  const auto kP = boatright::CalculateSpinRate(velocity, twist);
  const auto kR = boatright::CalculateEpicyclicRatio(stability);
  const auto kN = boatright::CalculateNutationCyclesNeeded(kR);
  const auto kF1F2Sum = boatright::CalculateGyroscopicRateSum(kP, kIyOverIx);
  const auto kF2 = boatright::CalculateGyroscopicRateF2(kF1F2Sum, kR);
  const auto kTn = boatright::CalculateFirstNutationPeriod(kF1F2Sum - kF2, kF2);
  const auto kGamma = boatright::CalculateCrosswindAngleGamma(zwind, velocity);
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

namespace boatright {
double CalculateKV(FpsT initial_velocity, FpsT target_velocity) {
  return std::log(target_velocity.Value() / initial_velocity.Value());
}

double CalculateKOmega(InchT diameter, SecT supersonic_time) {
  const double kA = 0.585;
  const double kB = 0.0321;
  return -1 * (kA + kB / diameter.Value()) * supersonic_time.Value();
}

RadiansT CalculateYawOfRepose(FpsT initial_velocity, InchPerTwistT twist,
                              double inertial_ratio, double epicyclic_ratio,
                              double komega, double kv) {
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

double CalculatePotentialDragForce(InchT diameter, LbsPerCuFtT air_density,
                                   FpsT target_velocity) {
  const auto kS = SqFtT(CalculateProjectileReferenceArea(diameter));
  const double kQTS = kS.Value() *
                      ((air_density.Value() / kStandardGravityFtPerSecSq) / 2) *
                      target_velocity.Value() * target_velocity.Value();
  return kQTS;  // Lbs of force
}

double CalculateCoefficentOfLiftAtT(double cl0, FpsT initial_velocity,
                                    SecT supersonic_time) {
  const double kA = std::pow(initial_velocity / FpsT(2600), 2.0).Value();
  const double kB = 1.430 / supersonic_time.Value();
  const double kExponent = -0.3711 * kA * kB;
  return cl0 * std::exp(kExponent);
}

double CalculateSpinDriftScaleFactor(double potential_drag_force,
                                     RadiansT yaw_of_repose,
                                     double coefficent_of_lift, GrainT mass) {
  const double kNumerator = 0.388132 * potential_drag_force *
                            yaw_of_repose.Value() * coefficent_of_lift;
  return kNumerator / LbsT(mass).Value();
}

InchT CalculateSpinDrift(double scale_factor, InchT drop) {
  return drop * scale_factor;
}

}  // namespace boatright

double CalculateBRSpinDriftFactor(InchT diameter, InchT meplat_diameter,
                                  InchT base_diameter, InchT length,
                                  InchT nose_length, InchT tail_length,
                                  double ogive_rtr, GrainT mass, FpsT velocity,
                                  double stability, InchPerTwistT twist,
                                  LbsPerCuFtT air_density,
                                  SecT supersonic_time) {
  const FpsT kTargetVelocity(1340.0);
  const CaliberT kDM(meplat_diameter, diameter.Inverse());
  // const CaliberT kDB(base_diameter, diameter.Inverse());
  const CaliberT kL(length, diameter.Inverse());
  const CaliberT kLN(nose_length, diameter.Inverse());
  // const CaliberT kLBT(tail_length, diameter.Inverse());
  const auto kRTR(ogive_rtr);
  const CaliberT kLFN = boatright::CalculateFullNoseLength(kLN, kDM, kRTR);
  const auto kRD = boatright::CalculateRelativeDensity(
      diameter, length, meplat_diameter, nose_length, base_diameter,
      tail_length, mass);
  std::cout << kRD << "\n";
  const double kIyOverIx =
      boatright::CalculateInertialRatio(diameter, kL, kLN, kLFN, mass, kRD);
  const auto kR = boatright::CalculateEpicyclicRatio(stability);
  const auto kV = boatright::CalculateKV(velocity, kTargetVelocity);
  const auto kOmega = boatright::CalculateKOmega(diameter, supersonic_time);
  const double kQTS = boatright::CalculatePotentialDragForce(
      diameter, air_density, kTargetVelocity);
  const auto kBetaROfT = boatright::CalculateYawOfRepose(
      velocity, twist, kIyOverIx, kR, kOmega, kV);
  const double kClOf0 = boatright::CalculateCoefficientOfLift(
      kLN, kDM, kRTR, lob::MachT(velocity, 1 / 1116.0));
  const auto kClOfT = boatright::CalculateCoefficentOfLiftAtT(kClOf0, velocity,
                                                              supersonic_time);
  return boatright::CalculateSpinDriftScaleFactor(kQTS, kBetaROfT, kClOfT,
                                                  mass);
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