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

Impl* Pimpl(LobBuilder* pbuilder) {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  return reinterpret_cast<Impl*>(&pbuilder->buffer);
}

const Impl* Pimpl(const LobBuilder* pbuilder) {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  return reinterpret_cast<const Impl*>(&pbuilder->buffer);
}

void BuildEnvironment(Impl* pimpl, LobContext* pout) {
  assert(pimpl != nullptr && pout != nullptr);
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
    pout->error = kLobErrorRangeAngleOOR;
    return;
  }

  pout->gravity.x = kStandardGravityFtPerSecSq * -1 *
                    std::sin(pimpl->range_angle_rad.Value());
  pout->gravity.y = kStandardGravityFtPerSecSq * -1 *
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
      pout->error = kLobErrorAltitudeOfFiringSiteOOR;
      return;
    }

    if (!is_altitude_valid(altitude_of_barometer)) {
      pout->error = kLobErrorAltitudeOfBarometerOOR;
      return;
    }

    if (!is_altitude_valid(altitude_of_thermometer)) {
      pout->error = kLobErrorAltitudeOfThermometerOOR;
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
      pout->error = kLobErrorAirPressureOOR;
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
    pout->error = kLobErrorHumidityOOR;
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

  pout->speed_of_sound = kSpeedOfSound.Value();
}

void BuildTable(Impl* pimpl, LobContext* pout) {
  assert(pimpl != nullptr && pout != nullptr);
  if (pimpl->custom_machs != nullptr) {
    if (pimpl->custom_count < 2) {
      pout->error = kLobErrorMachDragTableTooShort;
      return;
    }
    if (pimpl->custom_machs[0] > spline::kKnots.front() ||
        pimpl->custom_machs[pimpl->custom_count - 1] < spline::kKnots.back()) {
      pout->error = kLobErrorMachDragTableTooNarrow;
      return;
    }
    for (size_t i = 0; i < pimpl->custom_count; i++) {
      if (pimpl->custom_machs[i] < 0.0F || pimpl->custom_drags[i] < 0.0F) {
        pout->error = kLobErrorMachDragTableNegative;
        return;
      }
      if (i > 0 && pimpl->custom_machs[i] <= pimpl->custom_machs[i - 1]) {
        pout->error = kLobErrorMachDragTableNotMonotonic;
        return;
      }
    }
    spline::Build(pimpl->custom_machs, pimpl->custom_drags, pimpl->custom_count,
                  spline::kKnots.data(), spline::kKnotCount, &pout->drags[0]);

    pimpl->ballistic_coefficient_psi = PmsiT(1);
  }

  if (pimpl->ballistic_coefficient_psi.IsNaN()) {
    pout->error = kLobErrorBallisticCoefficientRequired;
    return;
  }

  if (pimpl->ballistic_coefficient_psi <= PmsiT(0.0)) {
    pout->error = kLobErrorBallisticCoefficientOOR;
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
    std::copy_n(coefs->data(), spline::kCoefsSize, &pout->drags[0]);
  }
  pout->drag_coeff = CalculateCdCoefficient(pimpl->air_density_lbs_per_cu_ft,
                                            pimpl->ballistic_coefficient_psi);
}

void BuildWind(Impl* pimpl, LobContext* pout) {
  assert(pimpl != nullptr && pout != nullptr);

  if (std::isnan(pimpl->wind_heading_rad)) {
    pimpl->wind_heading_rad = DegreesT(0);
  }

  const DegreesT kFullTurn(kDegreesPerTurn);
  if (pimpl->wind_heading_rad > kFullTurn ||
      pimpl->wind_heading_rad < kFullTurn * -1) {
    pout->error = kLobErrorWindHeadingOOR;
    return;
  }

  if (std::isnan(pimpl->wind_speed_fps)) {
    pimpl->wind_speed_fps = FpsT(0);
  }

  pout->wind.x =
      FpsT(pimpl->wind_speed_fps * std::sin(pimpl->wind_heading_rad.Value()))
          .Value();
  pout->wind.z =
      FpsT(pimpl->wind_speed_fps * std::cos(pimpl->wind_heading_rad.Value()))
          .Value();
}

void BuildOpticHeight(Impl* pimpl, LobContext* pout) {
  assert(pimpl != nullptr && pout != nullptr);
  if (!std::isnan(pimpl->optic_height_ft)) {
    pout->optic_height = pimpl->optic_height_ft.Value();
  } else {
    constexpr FeetT kDefaultOpticHeight = InchT(1.5);
    pout->optic_height = kDefaultOpticHeight.Value();
  }
}

void BuildStability(Impl* pimpl, LobContext* pout) {
  assert(pimpl != nullptr && pout != nullptr);

  if (pimpl->velocity_fps.IsNaN() || pimpl->velocity_fps <= FpsT(0)) {
    pout->error = kLobErrorInitialVelocityRequired;
    return;
  }

  if (pimpl->diameter_in <= InchT(0)) {
    pout->error = kLobErrorDiameterOOR;
    return;
  }

  if (pimpl->length_in <= InchT(0)) {
    pout->error = kLobErrorLengthOOR;
    return;
  }

  if (pimpl->mass_lbs < LbsT(0)) {
    pout->error = kLobErrorMassOOR;
    return;
  }

  if (pimpl->diameter_in.IsNaN() || pimpl->length_in.IsNaN() ||
      pimpl->mass_lbs.IsNaN() || pimpl->twist_inches_per_turn.IsNaN() ||
      AreEqual(pimpl->twist_inches_per_turn, InchPerTwistT(0))) {
    return;
  }

  const double kFtp = CalculateMillerTwistRuleCorrectionFactor(
      pimpl->air_density_lbs_per_cu_ft);
  pout->stability_factor =
      kFtp * CalculateMillerTwistRuleStabilityFactor(
                 pimpl->diameter_in, GrainT(pimpl->mass_lbs), pimpl->length_in,
                 pimpl->twist_inches_per_turn, pimpl->velocity_fps);
}

void BuildCoriolis(Impl* pimpl, LobContext* pout) {
  assert(pimpl != nullptr && pout != nullptr);

  if (!std::isnan(pimpl->azimuth_rad) && !std::isnan(pimpl->latitude_rad)) {
    const DegreesT kAzimuthLimit(kDegreesPerTurn);
    if (pimpl->azimuth_rad > kAzimuthLimit ||
        pimpl->azimuth_rad < kAzimuthLimit * -1) {
      pout->error = kLobErrorAzimuthOOR;
      return;
    }
    const DegreesT kLatitudeLimit(90);
    if (pimpl->latitude_rad > kLatitudeLimit ||
        pimpl->latitude_rad < kLatitudeLimit * -1) {
      pout->error = kLobErrorLatitudeOOR;
      return;
    }
    const double kCosL = std::cos(pimpl->latitude_rad).Value();
    const double kSinA = std::sin(pimpl->azimuth_rad).Value();
    const double kSinL = std::sin(pimpl->latitude_rad).Value();
    const double kCosA = std::cos(pimpl->azimuth_rad).Value();

    pout->coriolis.cos_l_sin_a =
        2 * kAngularVelocityOfEarthRadPerSec * kCosL * kSinA;
    pout->coriolis.sin_l = 2 * kAngularVelocityOfEarthRadPerSec * kSinL;
    pout->coriolis.cos_l_cos_a =
        2 * kAngularVelocityOfEarthRadPerSec * kCosL * kCosA;
  } else {
    pout->coriolis.cos_l_sin_a = 0;
    pout->coriolis.sin_l = 0;
    pout->coriolis.cos_l_cos_a = 0;
  }
}

void BuildBoatright(Impl* pimpl, LobContext* pout) {
  assert(pimpl != nullptr && pimpl != nullptr);

  if (pimpl->meplat_diameter_in < InchT(0)) {
    pout->error = kLobErrorMeplatDiameterOOR;
    return;
  }

  if (pimpl->base_diameter_in <= InchT(0)) {
    pout->error = kLobErrorBaseDiameterOOR;
    return;
  }

  if (pimpl->nose_length_in < InchT(0)) {
    pout->error = kLobErrorNoseLengthOOR;
    return;
  }

  if (pimpl->tail_length_in < InchT(0)) {
    pout->error = kLobErrorTailLengthOOR;
    return;
  }

  if (pimpl->ogive_rtr < 0 || pimpl->ogive_rtr > 1.0) {
    pout->error = kLobErrorOgiveRtROOR;
    return;
  }

  const InchT kD(pimpl->diameter_in);
  const CaliberT kDM(pimpl->meplat_diameter_in, kD.Inverse());
  const CaliberT kDB(pimpl->base_diameter_in, kD.Inverse());
  const CaliberT kL(pimpl->length_in, kD.Inverse());
  const CaliberT kLN(pimpl->nose_length_in, kD.Inverse());
  const CaliberT kLBT(pimpl->tail_length_in, kD.Inverse());
  const auto kRTR(pimpl->ogive_rtr);
  const FpsT kVelocity(pimpl->velocity_fps);
  const FpsT kSos(pout->speed_of_sound);
  const GrainT kMass = pimpl->mass_lbs;
  const InchPerTwistT kTwist(pimpl->twist_inches_per_turn);
  const double kSg(pout->stability_factor);
  const PmsiT kBc(pimpl->ballistic_coefficient_psi);
  const FpsT kZWind(pout->wind.z);

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
  spline::CurveView drag_curve(spline::kKnots.data(), &pout->drags[0]);
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
  pout->aerodynamic_jump = kJump.Value();

  TrajectoryStateT s(
      CartesianT<FeetT>(FeetT(0.0)),
      CartesianT<FpsT>(FpsT(kVelocity * std::cos(0)),
                       FpsT(kVelocity * std::sin(0)), FpsT(0.0)));

  static const FpsT kTransonicBarrier(MachT(1.2), kSos);
  constexpr SecT kTransonicTimeout(60.0);
  while (s.V().X() > kTransonicBarrier) {
    if (s.TOF() > kTransonicTimeout) {
      pout->error = kLobErrorInternalError;
      return;
    }
    const MachT kBuildMach(s.V().Magnitude(), kSos.Inverse());
    SolveStep(*pout, &s, &drag_curve);
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
  pout->spindrift_factor =
      boatright::CalculateSpinDriftScaleFactor(kQTS, kBetaROfT, kClOfT, kMass);
}

void BuildLitzAerodynamicJump(Impl* pimpl, LobContext* pout) {
  assert(pimpl != nullptr && pimpl != nullptr);

  if (!std::isnan(pout->aerodynamic_jump)) {
    return;
  }

  if (AreEqual(pout->wind.z, 0.0)) {
    pout->aerodynamic_jump = MoaT(0).Value();
    return;
  }

  if (!std::isnan(pout->stability_factor) && !std::isnan(pimpl->diameter_in) &&
      !std::isnan(pimpl->length_in)) {
    pout->aerodynamic_jump = litz::CalculateAerodynamicJump(
                                pout->stability_factor, pimpl->diameter_in,
                                pimpl->length_in, MphT(FpsT(pout->wind.z)))
                                .Value();
    return;
  }

  if (std::isnan(pout->aerodynamic_jump)) {
    pout->aerodynamic_jump = MoaT(0).Value();
    return;
  }
}

void BuildZeroAngle(Impl* pimpl, LobContext* pout) {
  assert(pimpl != nullptr && pimpl != nullptr);

  constexpr MoaT kZeroAngleLimit = DegreesT(45);

  if (!std::isnan(pout->zero_angle)) {
    if (pout->zero_angle > kZeroAngleLimit.Value() ||
        pout->zero_angle < kZeroAngleLimit.Value() * -1) {
      pout->error = kLobErrorZeroAngleOOR;
    }
    return;
  }

  if (pimpl->zero_distance_ft.IsNaN()) {
    pout->error = kLobErrorZeroDataRequired;
    return;
  }

  if (pimpl->zero_distance_ft <= FeetT(0)) {
    pout->error = kLobErrorZeroDistanceOOR;
    return;
  }

  assert(pout->velocity > 0);
  assert(!std::isnan(pout->aerodynamic_jump));

  if (std::isnan(pimpl->zero_impact_height)) {
    pimpl->zero_impact_height = FeetT(0.0);
  }

  // Human zero-by-correction: single-point linear iteration Δθ = -f/d
  // (radians). Same vacuum-projectile seed as benchmark/zero_angle.cpp
  // SearchHuman. Cap 10 iters; exhaustion or out-of-bracket surfaces as
  // kLobErrorInternalError. Bracket widened to ±45° (downward zeros).
  constexpr RadiansT kZeroAngleError = MoaT(0.01);
  constexpr RadiansT kMaxZeroAngle = kZeroAngleLimit;
  constexpr RadiansT kMinZeroAngle = kZeroAngleLimit * -1;
  constexpr size_t kMaxIterations = 10;

  const double kVSq = static_cast<double>(pout->velocity) *
                      static_cast<double>(pout->velocity);
  const double kRawSeed =
      kStandardGravityFtPerSecSq * pimpl->zero_distance_ft.Value() / kVSq;
  const double kClampedSeed =
      std::max(kMinZeroAngle.Value(),
               std::min(kMaxZeroAngle.Value(), kRawSeed));
  RadiansT theta = RadiansT(kClampedSeed);

  // Residual eval: impact_y - optic_height - zero_impact_height for theta.
  // step_size is saved/restored so the user's setting survives the search.
  auto fire_to_target = [&](RadiansT launch_angle) -> FeetT {
    const RadiansT kAngle =
        launch_angle + RadiansT(MoaT(pout->aerodynamic_jump));
    const FpsT kVelocity = FpsT(pout->velocity);
    TrajectoryStateT s(
        CartesianT<FeetT>(FeetT(0.0)),
        CartesianT<FpsT>(kVelocity * std::cos(kAngle.Value()),
                         kVelocity * std::sin(kAngle.Value()), FpsT(0.0)));
    const auto kSavedStepSize = pout->step_size;
    pout->step_size = 0U;
    constexpr SecT kMaxZeroTime(60);
    while (s.P().X() < pimpl->zero_distance_ft) {
      if (s.TOF() >= kMaxZeroTime) {
        pout->error = kLobErrorInternalError;
        return FeetT(NaN());
      }
      spline::CurveView zero_drag_curve(spline::kKnots.data(), &pout->drags[0]);
      SolveStep(*pout, &s, &zero_drag_curve);
    }
    pout->step_size = kSavedStepSize;
    return s.P().Y() - FeetT(pout->optic_height) - pimpl->zero_impact_height;
  };

  FeetT f = fire_to_target(theta);
  if (pout->error != kLobErrorNotFormed) {
    return;
  }

  for (size_t iter = 0; iter < kMaxIterations; ++iter) {
    const RadiansT kDTheta =
        RadiansT(-(f.Value() / pimpl->zero_distance_ft.Value()));
    const RadiansT kThetaNext = theta + kDTheta;

    if (kThetaNext < kMinZeroAngle || kThetaNext > kMaxZeroAngle ||
        std::isnan(kThetaNext.Value())) {
      pout->error = kLobErrorInternalError;
      return;
    }

    if (std::abs((kThetaNext - theta).Value()) <= kZeroAngleError.Value()) {
      pout->zero_angle = MoaT(kThetaNext).Value();
      return;
    }

    theta = kThetaNext;
    f = fire_to_target(theta);
    if (pout->error != kLobErrorNotFormed) {
      return;
    }
  }

  pout->error = kLobErrorInternalError;
}

void BuildOptions(Impl* pimpl, LobContext* pout) {
  assert(pimpl != nullptr && pimpl != nullptr);

  if (pout->max_time < 0.0) {
    pout->error = kLobErrorMaximumTimeOOR;
    return;
  }

  const FpsT kMinSpeed = CalculateVelocityFromKineticEnergy(
      pimpl->minimum_energy_ft_lbs, SlugT(pimpl->mass_lbs));
  pout->minimum_speed = std::max(pout->minimum_speed, kMinSpeed.U16());
}

}  // namespace
}  // namespace lob

extern "C" {
using namespace lob;  // NOLINT(google-build-using-namespace)

void LobBuilderInit(LobBuilder* pbuilder) {
  static_assert(sizeof(Impl) <= LOB_BUILDER_BUFFER_SIZE,
                "LOB_BUILDER_BUFFER_SIZE too small");
  ::new (&pbuilder->buffer) Impl();
}

void LobBuilderDestroy(LobBuilder* pbuilder) {
  if (pbuilder != nullptr) {
    Pimpl(pbuilder)->~Impl();
  }
}

void LobBuilderCopy(LobBuilder* dst, const LobBuilder* src) {
  if (dst != src) {
    Pimpl(dst)->~Impl();
    ::new (&dst->buffer) Impl(*Pimpl(src));
  }
}

LobBuilder* LobBuilderReset(LobBuilder* pbuilder) {
  auto* pimpl = Pimpl(pbuilder);
  pimpl->~Impl();
  pimpl = ::new (&pbuilder->buffer) Impl();
  return pbuilder;
}

LobBuilder* LobBuilderBallisticCoefficientPsi(LobBuilder* pbuilder,
                                              double value) {
  auto* pimpl = Pimpl(pbuilder);
  pimpl->ballistic_coefficient_psi = PmsiT(value);
  return pbuilder;
}

LobBuilder* LobBuilderBCAtmosphere(LobBuilder* pbuilder,
                                   LobAtmosphereReferenceT type) {
  auto* pimpl = Pimpl(pbuilder);
  pimpl->atmosphere_reference = type;
  return pbuilder;
}

LobBuilder* LobBuilderBCDragFunction(LobBuilder* pbuilder,
                                     LobDragFunctionT type) {
  auto* pimpl = Pimpl(pbuilder);
  pimpl->drag_function = type;
  pimpl->custom_machs = nullptr;
  pimpl->custom_drags = nullptr;
  pimpl->custom_count = 0;
  return pbuilder;
}

LobBuilder* LobBuilderDiameterInch(LobBuilder* pbuilder, double value) {
  auto* pimpl = Pimpl(pbuilder);
  pimpl->diameter_in = InchT(value);
  return pbuilder;
}

LobBuilder* LobBuilderMeplatDiameterInch(LobBuilder* pbuilder, double value) {
  auto* pimpl = Pimpl(pbuilder);
  pimpl->meplat_diameter_in = InchT(value);
  return pbuilder;
}

LobBuilder* LobBuilderBaseDiameterInch(LobBuilder* pbuilder, double value) {
  auto* pimpl = Pimpl(pbuilder);
  pimpl->base_diameter_in = InchT(value);
  return pbuilder;
}

LobBuilder* LobBuilderLengthInch(LobBuilder* pbuilder, double value) {
  auto* pimpl = Pimpl(pbuilder);
  pimpl->length_in = InchT(value);
  return pbuilder;
}

LobBuilder* LobBuilderNoseLengthInch(LobBuilder* pbuilder, double value) {
  auto* pimpl = Pimpl(pbuilder);
  pimpl->nose_length_in = InchT(value);
  return pbuilder;
}

LobBuilder* LobBuilderTailLengthInch(LobBuilder* pbuilder, double value) {
  auto* pimpl = Pimpl(pbuilder);
  pimpl->tail_length_in = InchT(value);
  return pbuilder;
}

LobBuilder* LobBuilderOgiveRtR(LobBuilder* pbuilder, double value) {
  auto* pimpl = Pimpl(pbuilder);
  pimpl->ogive_rtr = value;
  return pbuilder;
}

LobBuilder* LobBuilderSplineFitTable(LobBuilder* pbuilder, const float* pmachs,
                                     const float* pdrags, size_t size) {
  auto* pimpl = Pimpl(pbuilder);
  if (pmachs == nullptr || pdrags == nullptr) {
    return pbuilder;
  }
  pimpl->custom_machs = pmachs;
  pimpl->custom_drags = pdrags;
  pimpl->custom_count = size;
  return pbuilder;
}

LobBuilder* LobBuilderMassGrains(LobBuilder* pbuilder, double value) {
  auto* pimpl = Pimpl(pbuilder);
  pimpl->mass_lbs = LbsT(GrainT(value));
  return pbuilder;
}

LobBuilder* LobBuilderInitialVelocityFps(LobBuilder* pbuilder, uint16_t value) {
  auto* pimpl = Pimpl(pbuilder);
  pimpl->velocity_fps = FpsT(value);
  return pbuilder;
}

LobBuilder* LobBuilderOpticHeightInches(LobBuilder* pbuilder, double value) {
  auto* pimpl = Pimpl(pbuilder);
  pimpl->optic_height_ft = FeetT(InchT(value));
  return pbuilder;
}

LobBuilder* LobBuilderTwistInchesPerTurn(LobBuilder* pbuilder, double value) {
  auto* pimpl = Pimpl(pbuilder);
  pimpl->twist_inches_per_turn = InchPerTwistT(value);
  return pbuilder;
}

LobBuilder* LobBuilderZeroAngleMOA(LobBuilder* pbuilder, double value) {
  auto* pimpl = Pimpl(pbuilder);
  pimpl->zero_angle_moa = MoaT(value);
  return pbuilder;
}

LobBuilder* LobBuilderZeroDistanceYds(LobBuilder* pbuilder, double value) {
  auto* pimpl = Pimpl(pbuilder);
  pimpl->zero_distance_ft = YardT(value);
  return pbuilder;
}

LobBuilder* LobBuilderZeroImpactHeightInches(LobBuilder* pbuilder,
                                             double value) {
  auto* pimpl = Pimpl(pbuilder);
  pimpl->zero_impact_height = InchT(value);
  return pbuilder;
}

LobBuilder* LobBuilderAltitudeOfFiringSiteFt(LobBuilder* pbuilder,
                                             double value) {
  auto* pimpl = Pimpl(pbuilder);
  pimpl->altitude_ft = FeetT(value);
  return pbuilder;
}

LobBuilder* LobBuilderAirPressureInHg(LobBuilder* pbuilder, double value) {
  auto* pimpl = Pimpl(pbuilder);
  pimpl->air_pressure_in_hg = InHgT(value);
  return pbuilder;
}

LobBuilder* LobBuilderAltitudeOfBarometerFt(LobBuilder* pbuilder, double value) {
  auto* pimpl = Pimpl(pbuilder);
  pimpl->altitude_of_barometer_ft = FeetT(value);
  return pbuilder;
}

LobBuilder* LobBuilderTemperatureDegF(LobBuilder* pbuilder, double value) {
  auto* pimpl = Pimpl(pbuilder);
  pimpl->temperature_deg_f = DegFT(value);
  return pbuilder;
}

LobBuilder* LobBuilderAltitudeOfThermometerFt(LobBuilder* pbuilder,
                                              double value) {
  auto* pimpl = Pimpl(pbuilder);
  pimpl->altitude_of_thermometer_ft = FeetT(value);
  return pbuilder;
}

LobBuilder* LobBuilderRelativeHumidityPercent(LobBuilder* pbuilder,
                                              double value) {
  auto* pimpl = Pimpl(pbuilder);
  pimpl->relative_humidity_percent = PercentT(value);
  return pbuilder;
}

LobBuilder* LobBuilderWindHeading(LobBuilder* pbuilder, LobClockAngleT value) {
  auto* pimpl = Pimpl(pbuilder);
  const DegreesT kDegreesPerClockNumber = DegreesT(kDegreesPerTurn) / 12;
  const DegreesT kPosition(3 - static_cast<uint8_t>(value));
  if (kPosition.Value() > 0) {
    pimpl->wind_heading_rad = kDegreesPerClockNumber * kPosition;
  } else {
    pimpl->wind_heading_rad =
        kDegreesPerClockNumber * kPosition + kDegreesPerTurn;
  }
  return pbuilder;
}

LobBuilder* LobBuilderWindHeadingDeg(LobBuilder* pbuilder, double value) {
  auto* pimpl = Pimpl(pbuilder);
  const DegreesT kFullTurn(kDegreesPerTurn);
  const DegreesT kQuarterTurn(kFullTurn / 4);
  DegreesT angle(value);

  angle = angle * -1 + kQuarterTurn;

  if (angle < DegreesT(0)) {
    angle += kFullTurn;
  }

  pimpl->wind_heading_rad = angle;
  return pbuilder;
}

LobBuilder* LobBuilderWindSpeedFps(LobBuilder* pbuilder, double value) {
  auto* pimpl = Pimpl(pbuilder);
  pimpl->wind_speed_fps = FpsT(value);
  return pbuilder;
}

LobBuilder* LobBuilderWindSpeedMph(LobBuilder* pbuilder, double value) {
  auto* pimpl = Pimpl(pbuilder);
  pimpl->wind_speed_fps = MphT(value);
  return pbuilder;
}

LobBuilder* LobBuilderAzimuthDeg(LobBuilder* pbuilder, double value) {
  auto* pimpl = Pimpl(pbuilder);
  pimpl->azimuth_rad = DegreesT(value);
  return pbuilder;
}

LobBuilder* LobBuilderLatitudeDeg(LobBuilder* pbuilder, double value) {
  auto* pimpl = Pimpl(pbuilder);
  pimpl->latitude_rad = DegreesT(value);
  return pbuilder;
}

LobBuilder* LobBuilderRangeAngleDeg(LobBuilder* pbuilder, double value) {
  auto* pimpl = Pimpl(pbuilder);
  pimpl->range_angle_rad = RadiansT(DegreesT(value));
  return pbuilder;
}

LobBuilder* LobBuilderMinimumSpeed(LobBuilder* pbuilder, uint16_t value) {
  auto* pimpl = Pimpl(pbuilder);
  pimpl->minimum_speed_fps = FpsT(value);
  return pbuilder;
}

LobBuilder* LobBuilderMinimumEnergy(LobBuilder* pbuilder, uint16_t value) {
  auto* pimpl = Pimpl(pbuilder);
  pimpl->minimum_energy_ft_lbs = FtLbsT(value);
  return pbuilder;
}

LobBuilder* LobBuilderMaximumTime(LobBuilder* pbuilder, double value) {
  auto* pimpl = Pimpl(pbuilder);
  pimpl->max_time_sec = value;
  return pbuilder;
}

LobBuilder* LobBuilderStepSize(LobBuilder* pbuilder, uint16_t value) {
  auto* pimpl = Pimpl(pbuilder);
  pimpl->step_size_us = value;
  return pbuilder;
}

void LobBuilderBuild(LobBuilder* pbuilder, LobContext* presult) {
  assert(pbuilder != nullptr && presult != nullptr);
  if (pbuilder == nullptr || presult == nullptr) {
    return;
  }
  auto* pimpl = Pimpl(pbuilder);

  presult->mass = pimpl->mass_lbs.Value();
  presult->velocity =
      pimpl->velocity_fps.IsNaN() ? 0 : pimpl->velocity_fps.U16();
  presult->minimum_speed =
      pimpl->minimum_speed_fps.IsNaN() ? 0 : pimpl->minimum_speed_fps.U16();
  presult->max_time = pimpl->max_time_sec;
  presult->step_size = pimpl->step_size_us;
  presult->zero_angle =
      pimpl->zero_angle_moa.IsNaN() ? NaN() : pimpl->zero_angle_moa.Value();

  presult->error = kLobErrorNotFormed;
  presult->aerodynamic_jump = NaN();
  presult->spindrift_factor = NaN();
  presult->optic_height = NaN();

  BuildEnvironment(pimpl, presult);
  if (presult->error != kLobErrorNotFormed) {
    return;
  }
  BuildTable(pimpl, presult);
  if (presult->error != kLobErrorNotFormed) {
    return;
  }
  BuildWind(pimpl, presult);
  if (presult->error != kLobErrorNotFormed) {
    return;
  }
  BuildOpticHeight(pimpl, presult);
  BuildStability(pimpl, presult);
  if (presult->error != kLobErrorNotFormed) {
    return;
  }
  BuildCoriolis(pimpl, presult);
  if (presult->error != kLobErrorNotFormed) {
    return;
  }
  BuildBoatright(pimpl, presult);
  if (presult->error != kLobErrorNotFormed) {
    return;
  }
  BuildLitzAerodynamicJump(pimpl, presult);
  BuildZeroAngle(pimpl, presult);
  if (presult->error != kLobErrorNotFormed) {
    return;
  }
  BuildOptions(pimpl, presult);

  if (presult->error == kLobErrorNotFormed) {
    presult->error = kLobErrorNone;
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
