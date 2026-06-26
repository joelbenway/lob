// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "boatright.hpp"
#include "calc.hpp"
#include "cartesian.hpp"
#include "constants.hpp"
#include "eng_units.hpp"
#include "helpers.hpp"
#include "litz.hpp"
#include "lob/lob.h"
#include "ode.hpp"
#include "solve_step.hpp"
#include "tables.hpp"

namespace lob {

class Impl {
 public:
  LbsPerCuFtT air_density_lbs_per_cu_ft{NaN()};
  FeetT altitude_ft{NaN()};
  FeetT altitude_of_barometer_ft{NaN()};
  FeetT altitude_of_thermometer_ft{NaN()};
  LobAtmosphereReferenceT atmosphere_reference{
      kLobAtmosphereReferenceArmyStandardMetro};
  RadiansT azimuth_rad{NaN()};
  PmsiT ballistic_coefficient_psi{NaN()};
  InchT base_diameter_in{NaN()};
  InHgT air_pressure_in_hg{NaN()};
  InchT diameter_in{NaN()};
  RadiansT latitude_rad{NaN()};
  InchT length_in{NaN()};
  InchT meplat_diameter_in{NaN()};
  FtLbsT minimum_energy_ft_lbs{NaN()};
  InchT nose_length_in{NaN()};
  double ogive_rtr{NaN()};
  const uint16_t* pdrag_lut{kG1Drags.data()};
  RadiansT range_angle_rad{NaN()};
  double relative_humidity_percent{NaN()};
  InchT tail_length_in{NaN()};
  DegFT temperature_deg_f{NaN()};
  InchPerTwistT twist_inches_per_turn{NaN()};
  RadiansT wind_heading_rad{NaN()};
  FpsT wind_speed_fps{NaN()};
  FeetT zero_distance_ft{NaN()};
  FeetT zero_impact_height{NaN()};

  LobInput build{};
};

namespace {

Impl* Pimpl(LobBuilder* builder) {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  return reinterpret_cast<Impl*>(&builder->buffer);
}

const Impl* Pimpl(const LobBuilder* builder) {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  return reinterpret_cast<const Impl*>(&builder->buffer);
}

void BuildEnvironment(Impl* pimpl) {
  assert(pimpl != nullptr);
  FeetT altitude_of_firing_site = FeetT(0);
  FeetT altitude_of_barometer = FeetT(0);
  FeetT altitude_of_thermometer = FeetT(0);
  DegFT temperature_at_firing_site = DegFT(kIsaSeaLevelDegF);
  DegFT temperature_at_barometer = DegFT(kIsaSeaLevelDegF);
  InHgT pressure_at_firing_site = InHgT(kIsaSeaLevelPressureInHg);

  if (std::isnan(pimpl->range_angle_rad)) {
    pimpl->range_angle_rad = RadiansT(DegreesT(0));
  }

  const bool kRangeAngleValid = pimpl->range_angle_rad > DegreesT(-90.0) &&
                                pimpl->range_angle_rad < DegreesT(90.0);
  if (!kRangeAngleValid) {
    pimpl->build.error = kLobErrorRangeAngleOOR;
    return;
  }

  pimpl->build.gravity.x = kStandardGravityFtPerSecSq * -1 *
                           std::sin(pimpl->range_angle_rad.Value());
  pimpl->build.gravity.y = kStandardGravityFtPerSecSq * -1 *
                           std::cos(pimpl->range_angle_rad.Value());

  if (!std::isnan(pimpl->altitude_ft)) {
    altitude_of_firing_site = pimpl->altitude_ft;
    altitude_of_barometer = std::isnan(pimpl->altitude_of_barometer_ft)
                                ? pimpl->altitude_ft
                                : pimpl->altitude_of_barometer_ft;
    altitude_of_thermometer = std::isnan(pimpl->altitude_of_thermometer_ft)
                                  ? pimpl->altitude_ft
                                  : pimpl->altitude_of_thermometer_ft;

    auto is_altitude_valid = [](FeetT altitude) -> bool {
      return FeetT(-kIsaStratosphereAltitudeFt) < altitude &&
             altitude < FeetT(kIsaStratosphereAltitudeFt);
    };

    if (!is_altitude_valid(altitude_of_firing_site)) {
      pimpl->build.error = kLobErrorAltitudeOfFiringSiteOOR;
      return;
    }

    if (!is_altitude_valid(altitude_of_barometer)) {
      pimpl->build.error = kLobErrorAltitudeOfBarometerOOR;
      return;
    }

    if (!is_altitude_valid(altitude_of_thermometer)) {
      pimpl->build.error = kLobErrorAltitudeOfThermometerOOR;
      return;
    }

    temperature_at_firing_site = CalculateTemperatureAtAltitude(
        altitude_of_firing_site, DegFT(kIsaSeaLevelDegF));

    pressure_at_firing_site = BarometricFormula(altitude_of_firing_site,
                                                InHgT(kIsaSeaLevelPressureInHg),
                                                DegFT(kIsaSeaLevelDegF));
  }

  if (!std::isnan(pimpl->temperature_deg_f)) {
    temperature_at_firing_site = CalculateTemperatureAtAltitude(
        altitude_of_firing_site - altitude_of_thermometer,
        pimpl->temperature_deg_f);
    temperature_at_barometer = CalculateTemperatureAtAltitude(
        altitude_of_barometer - altitude_of_thermometer,
        pimpl->temperature_deg_f);
  }

  if (!std::isnan(pimpl->air_pressure_in_hg)) {
    if (pimpl->air_pressure_in_hg < InHgT(0.0)) {
      pimpl->build.error = kLobErrorAirPressureOOR;
      return;
    }
    pressure_at_firing_site =
        BarometricFormula(altitude_of_firing_site - altitude_of_barometer,
                          pimpl->air_pressure_in_hg, temperature_at_barometer);
  }

  if (std::isnan(pimpl->relative_humidity_percent)) {
    pimpl->relative_humidity_percent = kIsaSeaLevelHumidityPercent;
  }

  if (pimpl->relative_humidity_percent < 0.0) {
    pimpl->build.error = kLobErrorHumidityOOR;
    return;
  }

  const InHgT kWaterVaporSaturationPressureInHg =
      CalculateWaterVaporSaturationPressure(temperature_at_firing_site);

  const double kAirDensityRatio = CalculateAirDensityRatio(
      pressure_at_firing_site, temperature_at_firing_site);

  const double kHumidityCorrection = CalculateAirDensityRatioHumidityCorrection(
      pimpl->relative_humidity_percent, kWaterVaporSaturationPressureInHg);

  const LbsPerCuFtT kAirDensity(kIsaSeaLevelAirDensityLbsPerCuFt *
                                kAirDensityRatio * kHumidityCorrection);

  pimpl->air_density_lbs_per_cu_ft = kAirDensity;

  const double kSpeedOfSoundCorrection =
      CalculateSpeedOfSoundHumidityCorrection(
          pimpl->relative_humidity_percent, kWaterVaporSaturationPressureInHg);

  const FpsT kSpeedOfSound =
      CalculateSpeedOfSoundInAir(temperature_at_firing_site) *
      kSpeedOfSoundCorrection;

  pimpl->build.speed_of_sound = kSpeedOfSound.Value();
}

void BuildTable(Impl* pimpl) {
  assert(pimpl != nullptr);
  assert(!std::isnan(pimpl->air_density_lbs_per_cu_ft));

  if (pimpl->ballistic_coefficient_psi.IsNaN()) {
    pimpl->build.error = kLobErrorBallisticCoefficientRequired;
    return;
  }

  if (pimpl->ballistic_coefficient_psi <= PmsiT(0.0)) {
    pimpl->build.error = kLobErrorBallisticCoefficientOOR;
    return;
  }

  if (pimpl->atmosphere_reference == kLobAtmosphereReferenceArmyStandardMetro) {
    pimpl->ballistic_coefficient_psi *= kArmyToIcaoBcConversionFactor;
    pimpl->atmosphere_reference = kLobAtmosphereReferenceIcao;
  }

  static_assert(LOB_TABLE_SIZE == kTableSize, "Table size not identical.");
  if (pimpl->pdrag_lut != &pimpl->build.drags[0]) {
    std::copy(pimpl->pdrag_lut, pimpl->pdrag_lut + LOB_TABLE_SIZE,
              &pimpl->build.drags[0]);
  }
  const double kCdCoefficient = CalculateCdCoefficient(
      pimpl->air_density_lbs_per_cu_ft, pimpl->ballistic_coefficient_psi);
  pimpl->build.table_coefficient = kCdCoefficient;
}

void BuildWind(Impl* pimpl) {
  assert(pimpl != nullptr);

  if (std::isnan(pimpl->wind_heading_rad)) {
    pimpl->wind_heading_rad = DegreesT(0);
  }

  const DegreesT kFullTurn(kDegreesPerTurn);
  if (pimpl->wind_heading_rad > kFullTurn ||
      pimpl->wind_heading_rad < kFullTurn * -1) {
    pimpl->build.error = kLobErrorWindHeadingOOR;
    return;
  }

  if (std::isnan(pimpl->wind_speed_fps)) {
    pimpl->wind_speed_fps = FpsT(0);
  }

  pimpl->build.wind.x =
      FpsT(pimpl->wind_speed_fps * std::sin(pimpl->wind_heading_rad.Value()))
          .Value();
  pimpl->build.wind.z =
      FpsT(pimpl->wind_speed_fps * std::cos(pimpl->wind_heading_rad.Value()))
          .Value();
}

void BuildOpticHeight(Impl* pimpl) {
  assert(pimpl != nullptr);
  if (std::isnan(pimpl->build.optic_height)) {
    constexpr FeetT kDefaultOpticHeight = InchT(1.5);
    pimpl->build.optic_height = kDefaultOpticHeight.Value();
  }
}

void BuildStability(Impl* pimpl) {
  assert(pimpl != nullptr);
  assert(!std::isnan(pimpl->air_density_lbs_per_cu_ft));

  if (pimpl->build.velocity == 0) {
    pimpl->build.error = kLobErrorInitialVelocityRequired;
    return;
  }

  if (pimpl->diameter_in <= InchT(0)) {
    pimpl->build.error = kLobErrorDiameterOOR;
    return;
  }

  if (pimpl->length_in <= InchT(0)) {
    pimpl->build.error = kLobErrorLengthOOR;
    return;
  }

  if (pimpl->build.mass < 0) {
    pimpl->build.error = kLobErrorMassOOR;
    return;
  }

  if (pimpl->diameter_in.IsNaN() || pimpl->length_in.IsNaN() ||
      std::isnan(pimpl->build.mass) || pimpl->twist_inches_per_turn.IsNaN() ||
      AreEqual(pimpl->twist_inches_per_turn, InchPerTwistT(0))) {
    return;
  }

  const double kFtp = CalculateMillerTwistRuleCorrectionFactor(
      pimpl->air_density_lbs_per_cu_ft);
  pimpl->build.stability_factor =
      kFtp * CalculateMillerTwistRuleStabilityFactor(
                 pimpl->diameter_in, GrainT(LbsT(pimpl->build.mass)),
                 pimpl->length_in, pimpl->twist_inches_per_turn,
                 FpsT(pimpl->build.velocity));
}

void BuildCoriolis(Impl* pimpl) {
  assert(pimpl != nullptr);

  if (!std::isnan(pimpl->azimuth_rad) && !std::isnan(pimpl->latitude_rad)) {
    const DegreesT kAzimuthLimit(kDegreesPerTurn);
    if (pimpl->azimuth_rad > kAzimuthLimit ||
        pimpl->azimuth_rad < kAzimuthLimit * -1) {
      pimpl->build.error = kLobErrorAzimuthOOR;
      return;
    }
    const DegreesT kLatitudeLimit(90);
    if (pimpl->latitude_rad > kLatitudeLimit ||
        pimpl->latitude_rad < kLatitudeLimit * -1) {
      pimpl->build.error = kLobErrorLatitudeOOR;
      return;
    }
    const double kCosL = std::cos(pimpl->latitude_rad).Value();
    const double kSinA = std::sin(pimpl->azimuth_rad).Value();
    const double kSinL = std::sin(pimpl->latitude_rad).Value();
    const double kCosA = std::cos(pimpl->azimuth_rad).Value();

    pimpl->build.coriolis.cos_l_sin_a =
        2 * kAngularVelocityOfEarthRadPerSec * kCosL * kSinA;
    pimpl->build.coriolis.sin_l = 2 * kAngularVelocityOfEarthRadPerSec * kSinL;
    pimpl->build.coriolis.cos_l_cos_a =
        2 * kAngularVelocityOfEarthRadPerSec * kCosL * kCosA;
  } else {
    pimpl->build.coriolis.cos_l_sin_a = 0;
    pimpl->build.coriolis.sin_l = 0;
    pimpl->build.coriolis.cos_l_cos_a = 0;
  }
}

void BuildBoatright(Impl* pimpl) {
  assert(pimpl != nullptr);
  assert(pimpl->pdrag_lut != nullptr);

  if (pimpl->meplat_diameter_in < InchT(0)) {
    pimpl->build.error = kLobErrorMeplatDiameterOOR;
    return;
  }

  if (pimpl->base_diameter_in <= InchT(0)) {
    pimpl->build.error = kLobErrorBaseDiameterOOR;
    return;
  }

  if (pimpl->nose_length_in < InchT(0)) {
    pimpl->build.error = kLobErrorNoseLengthOOR;
    return;
  }

  if (pimpl->tail_length_in < InchT(0)) {
    pimpl->build.error = kLobErrorTailLengthOOR;
    return;
  }

  if (pimpl->ogive_rtr < 0 || pimpl->ogive_rtr > 1.0) {
    pimpl->build.error = kLobErrorOgiveRtROOR;
    return;
  }

  const InchT kD(pimpl->diameter_in);
  const CaliberT kDM(pimpl->meplat_diameter_in, kD.Inverse());
  const CaliberT kDB(pimpl->base_diameter_in, kD.Inverse());
  const CaliberT kL(pimpl->length_in, kD.Inverse());
  const CaliberT kLN(pimpl->nose_length_in, kD.Inverse());
  const CaliberT kLBT(pimpl->tail_length_in, kD.Inverse());
  const auto kRTR(pimpl->ogive_rtr);
  const FpsT kVelocity(pimpl->build.velocity);
  const FpsT kSos(pimpl->build.speed_of_sound);
  const GrainT kMass = LbsT(pimpl->build.mass);
  const InchPerTwistT kTwist(pimpl->twist_inches_per_turn);
  const LbsPerCuFtT kAirDensity(pimpl->air_density_lbs_per_cu_ft);
  const double kSg(pimpl->build.stability_factor);
  const PmsiT kBc(pimpl->ballistic_coefficient_psi);
  const FpsT kZWind(pimpl->build.wind.z);

  if (kD.IsNaN() || kDM.IsNaN() || kDB.IsNaN() || kL.IsNaN() || kLN.IsNaN() ||
      kLBT.IsNaN() || std::isnan(kRTR) || !(kVelocity > FpsT(0)) ||
      kSos.IsNaN() || kMass.IsNaN() || kTwist.IsNaN() || kAirDensity.IsNaN() ||
      std::isnan(kSg) || kBc.IsNaN() || kZWind.IsNaN()) {
    return;
  }

  const CaliberT kRT = boatright::CalculateRadiusOfTangentOgive(kLN, kDM);
  const CaliberT kLFN = boatright::CalculateFullNoseLength(kLN, kDM, kRT, kRTR);
  const PsiT kQ = boatright::CalculateDynamicPressure(kAirDensity, kVelocity);
  const SqInT kS = CalculateProjectileReferenceArea(kD);
  const auto kAR = boatright::CalculateAspectRatio(kL, kLFN, kLBT, kDB);
  const auto kM = lob::MachT(kVelocity, kSos.Inverse());
  const auto kCL = boatright::CalculateCoefficientOfLift(kLFN, kM);
  const auto kCDa = boatright::CalculateYawDragCoefficient(kM, kCL, kAR);
  const auto kRho = boatright::CalculateFastAverageDensity(kD, kL, kDM, kLN,
                                                           kDB, kLBT, kMass);
  const auto kIyOverIx =
      boatright::CalculateInertialRatio(kD, kL, kLN, kLFN, kMass, kRho);
  const auto kP = boatright::CalculateSpinRate(kVelocity, kTwist);
  const auto kR = boatright::CalculateEpicyclicRatio(kSg);
  const auto kN = boatright::CalculateNutationCyclesNeeded(kR);
  const auto kF1F2Sum = boatright::CalculateGyroscopicRateSum(kP, kIyOverIx);
  const auto kF2 = boatright::CalculateGyroscopicRateF2(kF1F2Sum, kR);
  const auto kTn = boatright::CalculateFirstNutationPeriod(kF1F2Sum - kF2, kF2);
  const auto kGamma =
      boatright::CalculateCrosswindAngleGamma(kZWind, kVelocity);
  const double kCdRef = LobLerp(kMachs.data(), pimpl->pdrag_lut, LOB_TABLE_SIZE,
                                kM.Value() * kTableScale) /
                        kTableScale;
  const auto kCD0 =
      boatright::CalculateZeroYawDragCoefficientOfDrag(kCdRef, kMass, kD, kBc);
  const auto kCDAdjustment =
      boatright::CalculateYawDragAdjustment(kGamma, kR, kCDa);
  const auto kCD = kCD0 + kCDAdjustment;
  const auto kPitch = boatright::CalculateVerticalPitch(kGamma, kR, kN);
  const auto kJv = boatright::CalculateVerticalImpulse(kTwist, kN, kTn, kQ, kS,
                                                       kCL, kCD, kPitch);
  const auto kMOM = boatright::CalculateMagnitudeOfMomentum(kMass, kVelocity);
  const MoaT kJump = RadiansT(-1 * kJv / kMOM);
  pimpl->build.aerodynamic_jump = kJump.Value();

  TrajectoryStateT s(
      CartesianT<FeetT>(FeetT(0.0)),
      CartesianT<FpsT>(FpsT(kVelocity * std::cos(0)),
                       FpsT(kVelocity * std::sin(0)), FpsT(0.0)));

  SecT t(0.0);

  static const FpsT kTransonicBarrier(MachT(1.2), kSos);
  constexpr SecT kTransonicTimeout(60.0);
  while (s.V().X() > kTransonicBarrier) {
    if (t > kTransonicTimeout) {
      pimpl->build.error = kLobErrorInternalError;
      return;
    }
    SolveStep(&s, &t, pimpl->build);
  }

  const auto kV = boatright::CalculateKV(kVelocity, kTransonicBarrier);
  const auto kOmega = boatright::CalculateKOmega(kD, t);
  const double kQTS = boatright::CalculatePotentialDragForce(kD, kAirDensity,
                                                             kTransonicBarrier);
  const auto kBetaROfT = boatright::CalculateYawOfRepose(
      kVelocity, kTwist, kIyOverIx, kR, kOmega, kV);

  PmsiT bc_g7(0);
  if (pimpl->pdrag_lut == kG7Drags.data()) {
    bc_g7 = kBc;
  } else {
    const double kFormFactor =
        litz::CalculateG7FormFactorPrediction(kD, kLN, kRTR, kDM, kLBT, kDB);
    bc_g7 = litz::CalculateBallisticCoefficient(kMass, kD, kFormFactor);
  }
  const double kClBoattailAdjustment =
      boatright::CalculateCLBoattailAdjustmentFactor(bc_g7);
  const double kClOf0 = kClBoattailAdjustment * kCL;
  const auto kClOfT =
      boatright::CalculateCoefficientOfLiftAtT(kClOf0, kVelocity, t);
  pimpl->build.spindrift_factor =
      boatright::CalculateSpinDriftScaleFactor(kQTS, kBetaROfT, kClOfT, kMass);
}

void BuildLitzAerodynamicJump(Impl* pimpl) {
  assert(pimpl != nullptr);
  assert(!std::isnan(pimpl->build.wind.z));

  if (!std::isnan(pimpl->build.aerodynamic_jump)) {
    return;
  }

  if (AreEqual(pimpl->build.wind.z, 0.0)) {
    pimpl->build.aerodynamic_jump = MoaT(0).Value();
    return;
  }

  if (!std::isnan(pimpl->build.stability_factor) &&
      !std::isnan(pimpl->diameter_in) && !std::isnan(pimpl->length_in)) {
    pimpl->build.aerodynamic_jump =
        litz::CalculateAerodynamicJump(pimpl->build.stability_factor,
                                       pimpl->diameter_in, pimpl->length_in,
                                       MphT(FpsT(pimpl->build.wind.z)))
            .Value();
    return;
  }

  if (std::isnan(pimpl->build.aerodynamic_jump)) {
    pimpl->build.aerodynamic_jump = MoaT(0).Value();
    return;
  }
}

void BuildZeroAngle(Impl* pimpl) {
  assert(pimpl != nullptr);

  if (!std::isnan(pimpl->build.zero_angle)) {
    const double kZeroAngleLimit = MoaT(DegreesT(45)).Value();
    if (pimpl->build.zero_angle > kZeroAngleLimit ||
        pimpl->build.zero_angle < kZeroAngleLimit * -1) {
      pimpl->build.error = kLobErrorZeroAngleOOR;
    }
    return;
  }

  if (pimpl->zero_distance_ft.IsNaN()) {
    pimpl->build.error = kLobErrorZeroDataRequired;
    return;
  }

  if (pimpl->zero_distance_ft <= FeetT(0)) {
    pimpl->build.error = kLobErrorZeroDistanceOOR;
    return;
  }

  assert(pimpl->build.velocity > 0);
  assert(!std::isnan(pimpl->build.aerodynamic_jump));

  if (std::isnan(pimpl->zero_impact_height)) {
    pimpl->zero_impact_height = FeetT(0.0);
  }

  constexpr RadiansT kZeroAngleError = MoaT(0.01);
  constexpr RadiansT kMaxZeroAngle = DegreesT(45);
  constexpr RadiansT kMinZeroAngle = DegreesT(0.0);
  RadiansT high_angle = kMaxZeroAngle;
  RadiansT low_angle = kMinZeroAngle;

  while (high_angle - low_angle > kZeroAngleError) {
    const RadiansT kZeroAngle = (low_angle + high_angle) / 2;
    const RadiansT kAngle =
        kZeroAngle + RadiansT(MoaT(pimpl->build.aerodynamic_jump));
    const FpsT kVelocity = FpsT(pimpl->build.velocity);

    TrajectoryStateT s(
        CartesianT<FeetT>(FeetT(0.0)),
        CartesianT<FpsT>(kVelocity * std::cos(kAngle.Value()),
                         kVelocity * std::sin(kAngle.Value()), FpsT(0.0)));

    SecT t(0.0);

    const auto kSavedStepSize = pimpl->build.step_size;
    pimpl->build.step_size = 0U;

    constexpr SecT kMaxZeroTime(60);
    while (s.P().X() < pimpl->zero_distance_ft) {
      if (t >= kMaxZeroTime) {
        pimpl->build.error = kLobErrorInternalError;
        return;
      }
      SolveStep(&s, &t, pimpl->build);
    }

    pimpl->build.step_size = kSavedStepSize;

    if (s.P().Y() - FeetT(pimpl->build.optic_height) >
        pimpl->zero_impact_height) {
      high_angle = kZeroAngle;
    } else {
      low_angle = kZeroAngle;
    }
  }
  pimpl->build.zero_angle = MoaT((low_angle + high_angle) / 2).Value();
}

void BuildOptions(Impl* pimpl) {
  assert(pimpl != nullptr);

  if (pimpl->build.max_time < 0.0) {
    pimpl->build.error = kLobErrorMaximumTimeOOR;
    return;
  }

  const FpsT kMinSpeed = CalculateVelocityFromKineticEnergy(
      pimpl->minimum_energy_ft_lbs, SlugT(LbsT(pimpl->build.mass)));
  pimpl->build.minimum_speed =
      std::max(pimpl->build.minimum_speed, kMinSpeed.U16());
}

}  // namespace
}  // namespace lob

// extern "C" functions inside namespace lob for unqualified access to helpers
namespace lob {
extern "C" {

void LobBuilderInit(LobBuilder* builder) {
  static_assert(sizeof(Impl) <= LOB_BUILDER_BUFFER_SIZE,
                "LOB_BUILDER_BUFFER_SIZE too small");
  auto* pimpl = ::new (&builder->buffer) Impl();
  pimpl->build.aerodynamic_jump = NaN();
  pimpl->build.zero_angle = NaN();
  pimpl->build.spindrift_factor = NaN();
  pimpl->build.optic_height = NaN();
}

void LobBuilderDestroy(LobBuilder* builder) {
  if (builder != nullptr) {
    Pimpl(builder)->~Impl();
  }
}

void LobBuilderCopy(LobBuilder* dst, const LobBuilder* src) {
  if (dst != src) {
    Pimpl(dst)->~Impl();
    ::new (&dst->buffer) Impl(*Pimpl(src));
  }
}

LobBuilder* LobBuilderReset(LobBuilder* builder) {
  auto* pimpl = Pimpl(builder);
  pimpl->~Impl();
  pimpl = ::new (&builder->buffer) Impl();
  pimpl->build.aerodynamic_jump = NaN();
  pimpl->build.zero_angle = NaN();
  pimpl->build.spindrift_factor = NaN();
  pimpl->build.optic_height = NaN();
  return builder;
}

LobBuilder* LobBuilderBallisticCoefficientPsi(LobBuilder* builder,
                                              double value) {
  auto* pimpl = Pimpl(builder);
  pimpl->ballistic_coefficient_psi = PmsiT(value);
  return builder;
}

LobBuilder* LobBuilderBCAtmosphere(LobBuilder* builder,
                                   LobAtmosphereReferenceT type) {
  auto* pimpl = Pimpl(builder);
  pimpl->atmosphere_reference = type;
  return builder;
}

LobBuilder* LobBuilderBCDragFunction(LobBuilder* builder,
                                     LobDragFunctionT type) {
  auto* pimpl = Pimpl(builder);
  switch (type) {
    case kLobDragFunctionG2: {
      pimpl->pdrag_lut = kG2Drags.data();
      break;
    }
    case kLobDragFunctionG5: {
      pimpl->pdrag_lut = kG5Drags.data();
      break;
    }
    case kLobDragFunctionG6: {
      pimpl->pdrag_lut = kG6Drags.data();
      break;
    }
    case kLobDragFunctionG7: {
      pimpl->pdrag_lut = kG7Drags.data();
      break;
    }
    case kLobDragFunctionG8: {
      pimpl->pdrag_lut = kG8Drags.data();
      break;
    }
    case kLobDragFunctionG1:
    default: {
      pimpl->pdrag_lut = kG1Drags.data();
      break;
    }
  }
  return builder;
}

LobBuilder* LobBuilderDiameterInch(LobBuilder* builder, double value) {
  auto* pimpl = Pimpl(builder);
  pimpl->diameter_in = InchT(value);
  return builder;
}

LobBuilder* LobBuilderMeplatDiameterInch(LobBuilder* builder, double value) {
  auto* pimpl = Pimpl(builder);
  pimpl->meplat_diameter_in = InchT(value);
  return builder;
}

LobBuilder* LobBuilderBaseDiameterInch(LobBuilder* builder, double value) {
  auto* pimpl = Pimpl(builder);
  pimpl->base_diameter_in = InchT(value);
  return builder;
}

LobBuilder* LobBuilderLengthInch(LobBuilder* builder, double value) {
  auto* pimpl = Pimpl(builder);
  pimpl->length_in = InchT(value);
  return builder;
}

LobBuilder* LobBuilderNoseLengthInch(LobBuilder* builder, double value) {
  auto* pimpl = Pimpl(builder);
  pimpl->nose_length_in = InchT(value);
  return builder;
}

LobBuilder* LobBuilderTailLengthInch(LobBuilder* builder, double value) {
  auto* pimpl = Pimpl(builder);
  pimpl->tail_length_in = InchT(value);
  return builder;
}

LobBuilder* LobBuilderOgiveRtR(LobBuilder* builder, double value) {
  auto* pimpl = Pimpl(builder);
  pimpl->ogive_rtr = value;
  return builder;
}

LobBuilder* LobBuilderMachVsDragTable(LobBuilder* builder, const float* pmachs,
                                      const float* pdrags, size_t size) {
  auto* pimpl = Pimpl(builder);
  if (pmachs == nullptr || pdrags == nullptr || size < 2) {
    return builder;
  }
  const double kFirstMach = pmachs[0];
  const double kLastMach = pmachs[size - 1];
  const double kMinSampleMach =
      static_cast<double>(kMachs.front()) / kTableScale;
  const double kMaxSampleMach =
      static_cast<double>(kMachs.back()) / kTableScale;
  if (kFirstMach > kMinSampleMach || kLastMach < kMaxSampleMach) {
    return builder;
  }
  for (size_t i = 1; i < size; i++) {
    if (pmachs[i] <= pmachs[i - 1]) {
      return builder;
    }
  }
  for (size_t i = 0; i < size; i++) {
    if (pdrags[i] < 0.0f || pdrags[i] > 65535.0f) {
      return builder;
    }
  }
  auto* pdrag = &pimpl->build.drags[0];
  for (size_t i = 0; i < LOB_TABLE_SIZE; i++) {
    const auto kMach = static_cast<double>(kMachs.at(i)) / kTableScale;
    const auto kDrag = static_cast<uint16_t>(
        std::round(LobLerp(pmachs, pdrags, size, kMach) * kTableScale));
    *pdrag++ = kDrag;
  }
  pimpl->pdrag_lut = &pimpl->build.drags[0];
  pimpl->ballistic_coefficient_psi = PmsiT(1);
  return builder;
}

LobBuilder* LobBuilderMassGrains(LobBuilder* builder, double value) {
  auto* pimpl = Pimpl(builder);
  pimpl->build.mass = LbsT(GrainT(value)).Value();
  return builder;
}

LobBuilder* LobBuilderInitialVelocityFps(LobBuilder* builder, uint16_t value) {
  auto* pimpl = Pimpl(builder);
  pimpl->build.velocity = FpsT(value).U16();
  return builder;
}

LobBuilder* LobBuilderOpticHeightInches(LobBuilder* builder, double value) {
  auto* pimpl = Pimpl(builder);
  pimpl->build.optic_height = FeetT(InchT(value)).Value();
  return builder;
}

LobBuilder* LobBuilderTwistInchesPerTurn(LobBuilder* builder, double value) {
  auto* pimpl = Pimpl(builder);
  pimpl->twist_inches_per_turn = InchPerTwistT(value);
  return builder;
}

LobBuilder* LobBuilderZeroAngleMOA(LobBuilder* builder, double value) {
  auto* pimpl = Pimpl(builder);
  pimpl->build.zero_angle = MoaT(value).Value();
  return builder;
}

LobBuilder* LobBuilderZeroDistanceYds(LobBuilder* builder, double value) {
  auto* pimpl = Pimpl(builder);
  pimpl->zero_distance_ft = YardT(value);
  return builder;
}

LobBuilder* LobBuilderZeroImpactHeightInches(LobBuilder* builder,
                                             double value) {
  auto* pimpl = Pimpl(builder);
  pimpl->zero_impact_height = InchT(value);
  return builder;
}

LobBuilder* LobBuilderAltitudeOfFiringSiteFt(LobBuilder* builder,
                                             double value) {
  auto* pimpl = Pimpl(builder);
  pimpl->altitude_ft = FeetT(value);
  return builder;
}

LobBuilder* LobBuilderAirPressureInHg(LobBuilder* builder, double value) {
  auto* pimpl = Pimpl(builder);
  pimpl->air_pressure_in_hg = InHgT(value);
  return builder;
}

LobBuilder* LobBuilderAltitudeOfBarometerFt(LobBuilder* builder, double value) {
  auto* pimpl = Pimpl(builder);
  pimpl->altitude_of_barometer_ft = FeetT(value);
  return builder;
}

LobBuilder* LobBuilderTemperatureDegF(LobBuilder* builder, double value) {
  auto* pimpl = Pimpl(builder);
  pimpl->temperature_deg_f = DegFT(value);
  return builder;
}

LobBuilder* LobBuilderAltitudeOfThermometerFt(LobBuilder* builder,
                                              double value) {
  auto* pimpl = Pimpl(builder);
  pimpl->altitude_of_thermometer_ft = FeetT(value);
  return builder;
}

LobBuilder* LobBuilderRelativeHumidityPercent(LobBuilder* builder,
                                              double value) {
  auto* pimpl = Pimpl(builder);
  pimpl->relative_humidity_percent = value;
  return builder;
}

LobBuilder* LobBuilderWindHeading(LobBuilder* builder, LobClockAngleT value) {
  auto* pimpl = Pimpl(builder);
  const DegreesT kDegreesPerClockNumber = DegreesT(kDegreesPerTurn) / 12;
  const DegreesT kPosition(3 - static_cast<uint8_t>(value));
  if (kPosition.Value() > 0) {
    pimpl->wind_heading_rad = kDegreesPerClockNumber * kPosition;
  } else {
    pimpl->wind_heading_rad =
        kDegreesPerClockNumber * kPosition + kDegreesPerTurn;
  }
  return builder;
}

LobBuilder* LobBuilderWindHeadingDeg(LobBuilder* builder, double value) {
  auto* pimpl = Pimpl(builder);
  const DegreesT kFullTurn(kDegreesPerTurn);
  const DegreesT kQuarterTurn(kFullTurn / 4);
  DegreesT angle(value);

  angle = angle * -1 + kQuarterTurn;

  if (angle < DegreesT(0)) {
    angle += kFullTurn;
  }

  pimpl->wind_heading_rad = angle;
  return builder;
}

LobBuilder* LobBuilderWindSpeedFps(LobBuilder* builder, double value) {
  auto* pimpl = Pimpl(builder);
  pimpl->wind_speed_fps = FpsT(value);
  return builder;
}

LobBuilder* LobBuilderWindSpeedMph(LobBuilder* builder, double value) {
  auto* pimpl = Pimpl(builder);
  pimpl->wind_speed_fps = MphT(value);
  return builder;
}

LobBuilder* LobBuilderAzimuthDeg(LobBuilder* builder, double value) {
  auto* pimpl = Pimpl(builder);
  pimpl->azimuth_rad = DegreesT(value);
  return builder;
}

LobBuilder* LobBuilderLatitudeDeg(LobBuilder* builder, double value) {
  auto* pimpl = Pimpl(builder);
  pimpl->latitude_rad = DegreesT(value);
  return builder;
}

LobBuilder* LobBuilderRangeAngleDeg(LobBuilder* builder, double value) {
  auto* pimpl = Pimpl(builder);
  pimpl->range_angle_rad = RadiansT(DegreesT(value));
  return builder;
}

LobBuilder* LobBuilderMinimumSpeed(LobBuilder* builder, uint16_t value) {
  auto* pimpl = Pimpl(builder);
  pimpl->build.minimum_speed = value;
  return builder;
}

LobBuilder* LobBuilderMinimumEnergy(LobBuilder* builder, uint16_t value) {
  auto* pimpl = Pimpl(builder);
  pimpl->minimum_energy_ft_lbs = FtLbsT(value);
  return builder;
}

LobBuilder* LobBuilderMaximumTime(LobBuilder* builder, double value) {
  auto* pimpl = Pimpl(builder);
  pimpl->build.max_time = value;
  return builder;
}

LobBuilder* LobBuilderStepSize(LobBuilder* builder, uint16_t value) {
  auto* pimpl = Pimpl(builder);
  pimpl->build.step_size = value;
  return builder;
}

LobInput LobBuilderBuild(LobBuilder* builder) {
  auto* pimpl = Pimpl(builder);
  pimpl->build.error = kLobErrorNotFormed;
  BuildEnvironment(pimpl);
  if (pimpl->build.error != kLobErrorNotFormed) {
    return pimpl->build;
  }
  BuildTable(pimpl);
  if (pimpl->build.error != kLobErrorNotFormed) {
    return pimpl->build;
  }
  BuildWind(pimpl);
  if (pimpl->build.error != kLobErrorNotFormed) {
    return pimpl->build;
  }
  BuildOpticHeight(pimpl);
  BuildStability(pimpl);
  if (pimpl->build.error != kLobErrorNotFormed) {
    return pimpl->build;
  }
  BuildCoriolis(pimpl);
  if (pimpl->build.error != kLobErrorNotFormed) {
    return pimpl->build;
  }
  BuildBoatright(pimpl);
  if (pimpl->build.error != kLobErrorNotFormed) {
    return pimpl->build;
  }
  BuildLitzAerodynamicJump(pimpl);
  BuildZeroAngle(pimpl);
  if (pimpl->build.error != kLobErrorNotFormed) {
    return pimpl->build;
  }
  BuildOptions(pimpl);

  if (pimpl->build.error == kLobErrorNotFormed) {
    pimpl->build.error = kLobErrorNone;
  }
  return pimpl->build;
}

}  // extern "C"
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
