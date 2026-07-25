// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include <algorithm>
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
#include "splines.hpp"

namespace lob {

class Impl {
 public:
  LbsPerCuFtT air_density_lbs_per_cu_ft{NaN()};
  LbsT mass_lbs{NaN()};
  FeetT optic_height_ft{NaN()};
  MoaT zero_angle_moa{NaN()};
  double max_time_sec{NaN()};
  FeetT altitude_ft{NaN()};
  FeetT altitude_of_barometer_ft{NaN()};
  FeetT altitude_of_thermometer_ft{NaN()};
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
  RadiansT range_angle_rad{NaN()};
  PercentT relative_humidity_percent{NaN()};
  InchT tail_length_in{NaN()};
  DegFT temperature_deg_f{NaN()};
  InchPerTwistT twist_inches_per_turn{NaN()};
  RadiansT wind_heading_rad{NaN()};
  FpsT wind_speed_fps{NaN()};
  FeetT zero_distance_ft{NaN()};
  FeetT zero_impact_height{NaN()};

  size_t custom_count{0};
  const float* custom_machs{nullptr};
  const float* custom_drags{nullptr};

  FpsT velocity_fps{NaN()};
  FpsT minimum_speed_fps{NaN()};
  uint16_t step_size_us{0};
  LobAtmosphereReferenceT atmosphere_reference{
      kLobAtmosphereReferenceArmyStandardMetro};
  LobDragFunctionT drag_function{kLobDragFunctionG1};
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

void BuildEnvironment(Impl* pimpl, LobContext* out) {
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
    out->error = kLobErrorRangeAngleOOR;
    return;
  }

  out->gravity.x = kStandardGravityFtPerSecSq * -1 *
                   std::sin(pimpl->range_angle_rad.Value());
  out->gravity.y = kStandardGravityFtPerSecSq * -1 *
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
      out->error = kLobErrorAltitudeOfFiringSiteOOR;
      return;
    }

    if (!is_altitude_valid(altitude_of_barometer)) {
      out->error = kLobErrorAltitudeOfBarometerOOR;
      return;
    }

    if (!is_altitude_valid(altitude_of_thermometer)) {
      out->error = kLobErrorAltitudeOfThermometerOOR;
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
      out->error = kLobErrorAirPressureOOR;
      return;
    }
    pressure_at_firing_site =
        BarometricFormula(altitude_of_firing_site - altitude_of_barometer,
                          pimpl->air_pressure_in_hg, temperature_at_barometer);
  }

  if (std::isnan(pimpl->relative_humidity_percent)) {
    pimpl->relative_humidity_percent = PercentT(kIsaSeaLevelHumidityPercent);
  }

  if (pimpl->relative_humidity_percent < PercentT(0.0)) {
    out->error = kLobErrorHumidityOOR;
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

  out->speed_of_sound = kSpeedOfSound.Value();
}

void BuildTable(Impl* pimpl, LobContext* out) {
  assert(pimpl != nullptr);
  if (pimpl->custom_machs != nullptr) {
    assert(pimpl->custom_drags != nullptr);
    if (pimpl->custom_count < 2) {
      out->error = kLobErrorMachDragTableTooShort;
      return;
    }
    if (pimpl->custom_machs[0] > spline::kKnots.front() ||
        pimpl->custom_machs[pimpl->custom_count - 1] < spline::kKnots.back()) {
      out->error = kLobErrorMachDragTableTooNarrow;
      return;
    }
    for (size_t i = 0; i < pimpl->custom_count; i++) {
      if (pimpl->custom_machs[i] < 0.0F || pimpl->custom_drags[i] < 0.0F) {
        out->error = kLobErrorMachDragTableNegative;
        return;
      }
      if (i > 0 && pimpl->custom_machs[i] <= pimpl->custom_machs[i - 1]) {
        out->error = kLobErrorMachDragTableNotMonotonic;
        return;
      }
    }
    spline::Build(pimpl->custom_machs, pimpl->custom_drags, pimpl->custom_count,
                  spline::kKnots.data(), spline::kKnotCount, &out->drags[0]);

    pimpl->ballistic_coefficient_psi = PmsiT(1);
  }

  if (pimpl->ballistic_coefficient_psi.IsNaN()) {
    out->error = kLobErrorBallisticCoefficientRequired;
    return;
  }

  if (pimpl->ballistic_coefficient_psi <= PmsiT(0.0)) {
    out->error = kLobErrorBallisticCoefficientOOR;
    return;
  }

  if (pimpl->atmosphere_reference == kLobAtmosphereReferenceArmyStandardMetro) {
    pimpl->ballistic_coefficient_psi *= kArmyToIcaoBcConversionFactor;
    pimpl->atmosphere_reference = kLobAtmosphereReferenceIcao;
  }

  if (pimpl->custom_machs == nullptr) {
    const std::array<float, spline::kCoefsSize>* coefs{nullptr};
    switch (pimpl->drag_function) {
      case kLobDragFunctionG2:
        coefs = &spline::kG2Coefs;
        break;
      case kLobDragFunctionG5:
        coefs = &spline::kG5Coefs;
        break;
      case kLobDragFunctionG6:
        coefs = &spline::kG6Coefs;
        break;
      case kLobDragFunctionG7:
        coefs = &spline::kG7Coefs;
        break;
      case kLobDragFunctionG8:
        coefs = &spline::kG8Coefs;
        break;
      default:
        coefs = &spline::kG1Coefs;
        break;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    std::copy_n(coefs->data(), spline::kCoefsSize, &out->drags[0]);
  }
  out->drag_coeff = CalculateCdCoefficient(pimpl->air_density_lbs_per_cu_ft,
                                           pimpl->ballistic_coefficient_psi);
}

void BuildWind(Impl* pimpl, LobContext* out) {
  assert(pimpl != nullptr);

  if (std::isnan(pimpl->wind_heading_rad)) {
    pimpl->wind_heading_rad = DegreesT(0);
  }

  const DegreesT kFullTurn(kDegreesPerTurn);
  if (pimpl->wind_heading_rad > kFullTurn ||
      pimpl->wind_heading_rad < kFullTurn * -1) {
    out->error = kLobErrorWindHeadingOOR;
    return;
  }

  if (std::isnan(pimpl->wind_speed_fps)) {
    pimpl->wind_speed_fps = FpsT(0);
  }

  out->wind.x =
      FpsT(pimpl->wind_speed_fps * std::sin(pimpl->wind_heading_rad.Value()))
          .Value();
  out->wind.z =
      FpsT(pimpl->wind_speed_fps * std::cos(pimpl->wind_heading_rad.Value()))
          .Value();
}

void BuildOpticHeight(Impl* pimpl, LobContext* out) {
  assert(pimpl != nullptr);
  if (!std::isnan(pimpl->optic_height_ft)) {
    out->optic_height = pimpl->optic_height_ft.Value();
  } else {
    constexpr FeetT kDefaultOpticHeight = InchT(1.5);
    out->optic_height = kDefaultOpticHeight.Value();
  }
}

void BuildStability(Impl* pimpl, LobContext* out) {
  assert(pimpl != nullptr);

  if (pimpl->velocity_fps.IsNaN() || pimpl->velocity_fps <= FpsT(0)) {
    out->error = kLobErrorInitialVelocityRequired;
    return;
  }

  if (pimpl->diameter_in <= InchT(0)) {
    out->error = kLobErrorDiameterOOR;
    return;
  }

  if (pimpl->length_in <= InchT(0)) {
    out->error = kLobErrorLengthOOR;
    return;
  }

  if (pimpl->mass_lbs < LbsT(0)) {
    out->error = kLobErrorMassOOR;
    return;
  }

  if (pimpl->diameter_in.IsNaN() || pimpl->length_in.IsNaN() ||
      pimpl->mass_lbs.IsNaN() || pimpl->twist_inches_per_turn.IsNaN() ||
      AreEqual(pimpl->twist_inches_per_turn, InchPerTwistT(0))) {
    return;
  }

  const double kFtp = CalculateMillerTwistRuleCorrectionFactor(
      pimpl->air_density_lbs_per_cu_ft);
  out->stability_factor =
      kFtp * CalculateMillerTwistRuleStabilityFactor(
                 pimpl->diameter_in, GrainT(pimpl->mass_lbs), pimpl->length_in,
                 pimpl->twist_inches_per_turn, pimpl->velocity_fps);
}

void BuildCoriolis(Impl* pimpl, LobContext* out) {
  assert(pimpl != nullptr);

  if (!std::isnan(pimpl->azimuth_rad) && !std::isnan(pimpl->latitude_rad)) {
    const DegreesT kAzimuthLimit(kDegreesPerTurn);
    if (pimpl->azimuth_rad > kAzimuthLimit ||
        pimpl->azimuth_rad < kAzimuthLimit * -1) {
      out->error = kLobErrorAzimuthOOR;
      return;
    }
    const DegreesT kLatitudeLimit(90);
    if (pimpl->latitude_rad > kLatitudeLimit ||
        pimpl->latitude_rad < kLatitudeLimit * -1) {
      out->error = kLobErrorLatitudeOOR;
      return;
    }
    const double kCosL = std::cos(pimpl->latitude_rad).Value();
    const double kSinA = std::sin(pimpl->azimuth_rad).Value();
    const double kSinL = std::sin(pimpl->latitude_rad).Value();
    const double kCosA = std::cos(pimpl->azimuth_rad).Value();

    out->coriolis.cos_l_sin_a =
        2 * kAngularVelocityOfEarthRadPerSec * kCosL * kSinA;
    out->coriolis.sin_l = 2 * kAngularVelocityOfEarthRadPerSec * kSinL;
    out->coriolis.cos_l_cos_a =
        2 * kAngularVelocityOfEarthRadPerSec * kCosL * kCosA;
  } else {
    out->coriolis.cos_l_sin_a = 0;
    out->coriolis.sin_l = 0;
    out->coriolis.cos_l_cos_a = 0;
  }
}

void BuildBoatright(Impl* pimpl, LobContext* out) {
  assert(pimpl != nullptr);

  if (pimpl->meplat_diameter_in < InchT(0)) {
    out->error = kLobErrorMeplatDiameterOOR;
    return;
  }

  if (pimpl->base_diameter_in <= InchT(0)) {
    out->error = kLobErrorBaseDiameterOOR;
    return;
  }

  if (pimpl->nose_length_in < InchT(0)) {
    out->error = kLobErrorNoseLengthOOR;
    return;
  }

  if (pimpl->tail_length_in < InchT(0)) {
    out->error = kLobErrorTailLengthOOR;
    return;
  }

  if (pimpl->ogive_rtr < 0 || pimpl->ogive_rtr > 1.0) {
    out->error = kLobErrorOgiveRtROOR;
    return;
  }

  const InchT kD(pimpl->diameter_in);
  const CaliberT kDM(pimpl->meplat_diameter_in, kD.Inverse());
  const CaliberT kDB(pimpl->base_diameter_in, kD.Inverse());
  const CaliberT kL(pimpl->length_in, kD.Inverse());
  const CaliberT kLN(pimpl->nose_length_in, kD.Inverse());
  const CaliberT kLBT(pimpl->tail_length_in, kD.Inverse());
  const auto kRTR(pimpl->ogive_rtr);
  const FpsT kVelocity(out->velocity);
  const FpsT kSos(out->speed_of_sound);
  const GrainT kMass = LbsT(out->mass);
  const InchPerTwistT kTwist(pimpl->twist_inches_per_turn);
  const double kSg(out->stability_factor);
  const PmsiT kBc(pimpl->ballistic_coefficient_psi);
  const FpsT kZWind(out->wind.z);

  if (kD.IsNaN() || kDM.IsNaN() || kDB.IsNaN() || kL.IsNaN() || kLN.IsNaN() ||
      kLBT.IsNaN() || std::isnan(kRTR) || !(kVelocity > FpsT(0)) ||
      kSos.IsNaN() || kMass.IsNaN() || kTwist.IsNaN() || std::isnan(kSg) ||
      kBc.IsNaN() || kZWind.IsNaN()) {
    return;
  }

  const CaliberT kRT = boatright::CalculateRadiusOfTangentOgive(kLN, kDM);
  const CaliberT kLFN = boatright::CalculateFullNoseLength(kLN, kDM, kRT, kRTR);
  const PsiT kQ = boatright::CalculateDynamicPressure(
      pimpl->air_density_lbs_per_cu_ft, kVelocity);
  const SqInT kS = CalculateProjectileReferenceArea(kD);
  const auto kAR = boatright::CalculateAspectRatio(kL, kLFN, kLBT, kDB);
  const auto kM = MachT(kVelocity, kSos.Inverse());
  spline::CurveView drag_curve(spline::kKnots.data(), &out->drags[0]);
  const auto kCdRef = drag_curve.Eval(kM);
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
  out->aerodynamic_jump = kJump.Value();

  TrajectoryStateT s(
      CartesianT<FeetT>(FeetT(0.0)),
      CartesianT<FpsT>(FpsT(kVelocity * std::cos(0)),
                       FpsT(kVelocity * std::sin(0)), FpsT(0.0)));

  const bool kDist = (out->step_size == 0);
  SecT t(0);
  FeetT x(0);

  static const FpsT kTransonicBarrier(MachT(1.2), kSos);
  constexpr SecT kTransonicTimeout(60.0);
  while (s.V().X() > kTransonicBarrier) {
    if (s.TOF() > kTransonicTimeout) {
      out->error = kLobErrorInternalError;
      return;
    }
    const MachT kBuildMach(s.V().Magnitude(), kSos.Inverse());

    if (kDist) SolveStep(&s, &x, &drag_curve, *out);
    else       SolveStep(&s, &t, &drag_curve, *out);
  }

  const auto kV = boatright::CalculateKV(kVelocity, kTransonicBarrier);
  const auto kOmega = boatright::CalculateKOmega(kD, s.TOF());
  const double kQTS = boatright::CalculatePotentialDragForce(
      kD, pimpl->air_density_lbs_per_cu_ft, kTransonicBarrier);
  const auto kBetaROfT = boatright::CalculateYawOfRepose(
      kVelocity, kTwist, kIyOverIx, kR, kOmega, kV);

  PmsiT bc_g7(0);
  if (pimpl->custom_machs == nullptr &&
      pimpl->drag_function == kLobDragFunctionG7) {
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
      boatright::CalculateCoefficientOfLiftAtT(kClOf0, kVelocity, s.TOF());
  out->spindrift_factor =
      boatright::CalculateSpinDriftScaleFactor(kQTS, kBetaROfT, kClOfT, kMass);
}

void BuildLitzAerodynamicJump(Impl* pimpl, LobContext* out) {
  assert(pimpl != nullptr);

  if (!std::isnan(out->aerodynamic_jump)) {
    return;
  }

  if (AreEqual(out->wind.z, 0.0)) {
    out->aerodynamic_jump = MoaT(0).Value();
    return;
  }

  if (!std::isnan(out->stability_factor) && !std::isnan(pimpl->diameter_in) &&
      !std::isnan(pimpl->length_in)) {
    out->aerodynamic_jump = litz::CalculateAerodynamicJump(
                                out->stability_factor, pimpl->diameter_in,
                                pimpl->length_in, MphT(FpsT(out->wind.z)))
                                .Value();
    return;
  }

  if (std::isnan(out->aerodynamic_jump)) {
    out->aerodynamic_jump = MoaT(0).Value();
    return;
  }
}

void BuildZeroAngle(Impl* pimpl, LobContext* out) {
  assert(pimpl != nullptr);

  if (!std::isnan(out->zero_angle)) {
    const double kZeroAngleLimit = MoaT(DegreesT(45)).Value();
    if (out->zero_angle > kZeroAngleLimit ||
        out->zero_angle < kZeroAngleLimit * -1) {
      out->error = kLobErrorZeroAngleOOR;
    }
    return;
  }

  if (pimpl->zero_distance_ft.IsNaN()) {
    out->error = kLobErrorZeroDataRequired;
    return;
  }

  if (pimpl->zero_distance_ft <= FeetT(0)) {
    out->error = kLobErrorZeroDistanceOOR;
    return;
  }

  assert(out->velocity > 0);
  assert(!std::isnan(out->aerodynamic_jump));

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
    const RadiansT kAngle = kZeroAngle + RadiansT(MoaT(out->aerodynamic_jump));
    const FpsT kVelocity = FpsT(out->velocity);

    TrajectoryStateT s(
        CartesianT<FeetT>(FeetT(0.0)),
        CartesianT<FpsT>(kVelocity * std::cos(kAngle.Value()),
                         kVelocity * std::sin(kAngle.Value()), FpsT(0.0)));

    SecT t(0.0);

    const auto kSavedStepSize = out->step_size;
    out->step_size = 0U;

    constexpr SecT kMaxZeroTime(60);
    while (s.P().X() < pimpl->zero_distance_ft) {
      if (t >= kMaxZeroTime) {
        out->error = kLobErrorInternalError;
        return;
      }
      const MachT kZeroMach(s.V().Magnitude(),
                            FpsT(out->speed_of_sound).Inverse());
      spline::CurveView zero_drag_curve(spline::kKnots.data(), &out->drags[0]);
      SolveStep(&s, &t, &zero_drag_curve, *out);
    }

    out->step_size = kSavedStepSize;

    if (s.P().Y() - FeetT(out->optic_height) > pimpl->zero_impact_height) {
      high_angle = kZeroAngle;
    } else {
      low_angle = kZeroAngle;
    }
  }
  out->zero_angle = MoaT((low_angle + high_angle) / 2).Value();
}

void BuildOptions(Impl* pimpl, LobContext* out) {
  assert(pimpl != nullptr);

  if (out->max_time < 0.0) {
    out->error = kLobErrorMaximumTimeOOR;
    return;
  }

  const FpsT kMinSpeed = CalculateVelocityFromKineticEnergy(
      pimpl->minimum_energy_ft_lbs, SlugT(pimpl->mass_lbs));
  out->minimum_speed = std::max(out->minimum_speed, kMinSpeed.U16());
}

}  // namespace
}  // namespace lob

extern "C" {
using namespace lob;  // NOLINT(google-build-using-namespace)

void LobBuilderInit(LobBuilder* builder) {
  static_assert(sizeof(Impl) <= LOB_BUILDER_BUFFER_SIZE,
                "LOB_BUILDER_BUFFER_SIZE too small");
  ::new (&builder->buffer) Impl();
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
  pimpl->drag_function = type;
  pimpl->custom_machs = nullptr;
  pimpl->custom_drags = nullptr;
  pimpl->custom_count = 0;
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

LobBuilder* LobBuilderSplineFitTable(LobBuilder* builder, const float* pmachs,
                                     const float* pdrags, size_t size) {
  auto* pimpl = Pimpl(builder);
  if (pmachs == nullptr || pdrags == nullptr) {
    return builder;
  }
  pimpl->custom_machs = pmachs;
  pimpl->custom_drags = pdrags;
  pimpl->custom_count = size;
  return builder;
}

LobBuilder* LobBuilderMassGrains(LobBuilder* builder, double value) {
  auto* pimpl = Pimpl(builder);
  pimpl->mass_lbs = LbsT(GrainT(value));
  return builder;
}

LobBuilder* LobBuilderInitialVelocityFps(LobBuilder* builder, uint16_t value) {
  auto* pimpl = Pimpl(builder);
  pimpl->velocity_fps = FpsT(value);
  return builder;
}

LobBuilder* LobBuilderOpticHeightInches(LobBuilder* builder, double value) {
  auto* pimpl = Pimpl(builder);
  pimpl->optic_height_ft = FeetT(InchT(value));
  return builder;
}

LobBuilder* LobBuilderTwistInchesPerTurn(LobBuilder* builder, double value) {
  auto* pimpl = Pimpl(builder);
  pimpl->twist_inches_per_turn = InchPerTwistT(value);
  return builder;
}

LobBuilder* LobBuilderZeroAngleMOA(LobBuilder* builder, double value) {
  auto* pimpl = Pimpl(builder);
  pimpl->zero_angle_moa = MoaT(value);
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
  pimpl->relative_humidity_percent = PercentT(value);
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
  pimpl->minimum_speed_fps = FpsT(value);
  return builder;
}

LobBuilder* LobBuilderMinimumEnergy(LobBuilder* builder, uint16_t value) {
  auto* pimpl = Pimpl(builder);
  pimpl->minimum_energy_ft_lbs = FtLbsT(value);
  return builder;
}

LobBuilder* LobBuilderMaximumTime(LobBuilder* builder, double value) {
  auto* pimpl = Pimpl(builder);
  pimpl->max_time_sec = value;
  return builder;
}

LobBuilder* LobBuilderStepSize(LobBuilder* builder, uint16_t value) {
  auto* pimpl = Pimpl(builder);
  pimpl->step_size_us = value;
  return builder;
}

void LobBuilderBuild(LobBuilder* builder, LobContext* result) {
  auto* pimpl = Pimpl(builder);
  assert(result != nullptr);
  if (result == nullptr) {
    return;
  }

  result->mass = pimpl->mass_lbs.Value();
  result->velocity =
      pimpl->velocity_fps.IsNaN() ? 0 : pimpl->velocity_fps.U16();
  result->minimum_speed =
      pimpl->minimum_speed_fps.IsNaN() ? 0 : pimpl->minimum_speed_fps.U16();
  result->max_time = pimpl->max_time_sec;
  result->step_size = pimpl->step_size_us;
  result->zero_angle =
      pimpl->zero_angle_moa.IsNaN() ? NaN() : pimpl->zero_angle_moa.Value();

  result->error = kLobErrorNotFormed;
  result->aerodynamic_jump = NaN();
  result->spindrift_factor = NaN();
  result->optic_height = NaN();

  BuildEnvironment(pimpl, result);
  if (result->error != kLobErrorNotFormed) {
    return;
  }
  BuildTable(pimpl, result);
  if (result->error != kLobErrorNotFormed) {
    return;
  }
  BuildWind(pimpl, result);
  if (result->error != kLobErrorNotFormed) {
    return;
  }
  BuildOpticHeight(pimpl, result);
  BuildStability(pimpl, result);
  if (result->error != kLobErrorNotFormed) {
    return;
  }
  BuildCoriolis(pimpl, result);
  if (result->error != kLobErrorNotFormed) {
    return;
  }
  BuildBoatright(pimpl, result);
  if (result->error != kLobErrorNotFormed) {
    return;
  }
  BuildLitzAerodynamicJump(pimpl, result);
  BuildZeroAngle(pimpl, result);
  if (result->error != kLobErrorNotFormed) {
    return;
  }
  BuildOptions(pimpl, result);

  if (result->error == kLobErrorNotFormed) {
    result->error = kLobErrorNone;
  }
}

}  // extern "C"

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
