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
#include "solve_angle.hpp"
#include "solve_step.hpp"
#include "splines.hpp"

namespace lob {

namespace {

enum class DragTableMode : uint8_t {
  kStandard,
  kCustomTable,
  kBcBands,
};

}  // namespace

class Impl {
 public:
  LbsPerCuFtT air_density_lbs_per_cu_ft{NaN()};
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
  LbsT mass_lbs{NaN()};
  SecT max_time_sec{NaN()};
  InchT meplat_diameter_in{NaN()};
  FtLbsT minimum_energy_ft_lbs{NaN()};
  FpsT minimum_speed_fps{NaN()};
  InchT nose_length_in{NaN()};
  double ogive_rtr{NaN()};
  FeetT optic_height_ft{NaN()};
  RadiansT range_angle_rad{NaN()};
  PercentT relative_humidity_percent{NaN()};
  InchT tail_length_in{NaN()};
  DegFT temperature_deg_f{NaN()};
  InchPerTwistT twist_inches_per_turn{NaN()};
  FpsT velocity_fps{NaN()};
  RadiansT wind_heading_rad{NaN()};
  FpsT wind_speed_fps{NaN()};
  MoaT zero_angle_moa{NaN()};
  FeetT zero_distance_ft{NaN()};
  FeetT zero_impact_height{NaN()};

  size_t table_count{0};
  const float* table_xs{nullptr};
  const float* table_ys{nullptr};
  uint16_t step_size_in{0};
  LobAtmosphereReferenceT atmosphere_reference{
      kLobAtmosphereReferenceArmyStandardMetro};
  LobDragFunctionT drag_function{kLobDragFunctionG1};
  DragTableMode drag_table_mode{DragTableMode::kStandard};
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

  if (pimpl->relative_humidity_percent < PercentT(0.0) ||
      pimpl->relative_humidity_percent > PercentT(100.0)) {
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

LobErrorT ValidateCustomTable(Impl* pimpl) {
  if (pimpl->table_count < 2) {
    return kLobErrorMachDragTableTooShort;
  }
  if (pimpl->table_xs[0] > spline::kKnots.front() ||
      pimpl->table_xs[pimpl->table_count - 1] < spline::kKnots.back()) {
    return kLobErrorMachDragTableTooNarrow;
  }
  for (size_t i = 0; i < pimpl->table_count; i++) {
    if (std::isnan(pimpl->table_xs[i]) || std::isnan(pimpl->table_ys[i]) ||
        pimpl->table_xs[i] < 0.0F || pimpl->table_ys[i] < 0.0F) {
      return kLobErrorMachDragTableInvalid;
    }
    if (i > 0 && pimpl->table_xs[i] <= pimpl->table_xs[i - 1]) {
      return kLobErrorMachDragTableNotMonotonic;
    }
  }
  return kLobErrorNone;
}

LobErrorT ValidateBcBands(Impl* pimpl) {
  if (pimpl->table_count < 2) {
    return kLobErrorBcBandsTooShort;
  }
  if (pimpl->table_count > spline::kKnotCount) {
    return kLobErrorBcBandsInvalid;
  }
  for (size_t i = 0; i < pimpl->table_count; i++) {
    if (std::isnan(pimpl->table_xs[i]) || std::isnan(pimpl->table_ys[i]) ||
        pimpl->table_xs[i] <= 0.0F || pimpl->table_ys[i] <= 0.0F) {
      return kLobErrorBcBandsInvalid;
    }
    if (i > 0 && pimpl->table_xs[i] <= pimpl->table_xs[i - 1]) {
      return kLobErrorBcBandsNotMonotonic;
    }
  }
  return kLobErrorNone;
}

void BuildSpline(Impl* pimpl, LobContext* pout) {
  assert(pimpl != nullptr && pout != nullptr);

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

  if (pimpl->drag_table_mode == DragTableMode::kCustomTable) {
    const LobErrorT kErr = ValidateCustomTable(pimpl);
    if (kErr != kLobErrorNone) {
      pout->error = kErr;
      return;
    }
    spline::Build(pimpl->table_xs, pimpl->table_ys, pimpl->table_count,
                  spline::kKnots.data(), spline::kKnotCount, &pout->drags[0]);

    pimpl->ballistic_coefficient_psi = PmsiT(1);
    pimpl->atmosphere_reference = kLobAtmosphereReferenceIcao;
    return;
  }

  if (pimpl->drag_table_mode == DragTableMode::kBcBands) {
    const LobErrorT kErr = ValidateBcBands(pimpl);
    if (kErr != kLobErrorNone) {
      pout->error = kErr;
      return;
    }
    const auto kSos = static_cast<float>(pout->speed_of_sound);
    if (pimpl->table_xs[pimpl->table_count - 1] / kSos >=
        spline::kKnots.back()) {
      pout->error = kLobErrorBcBandsInvalid;
      return;
    }
    const auto kConvert =
        pimpl->atmosphere_reference == kLobAtmosphereReferenceArmyStandardMetro
            ? static_cast<float>(kArmyToIcaoBcConversionFactor)
            : 1.0F;
    std::array<float, spline::kKnotCount> machs{};
    std::array<float, spline::kKnotCount> bcs{};
    for (size_t i = 0; i < pimpl->table_count; i++) {
      machs.at(i) = pimpl->table_xs[i] / kSos;
      bcs.at(i) = pimpl->table_ys[i] * kConvert;
    }
    std::array<float, spline::kCoefsSize> scale_coefs{};
    spline::MakeRetardationCoefs(machs.data(), bcs.data(), pimpl->table_count,
                                 scale_coefs.data());
    spline::CurveView scaling_curve(spline::kKnots, scale_coefs);
    spline::CurveView drag_curve(spline::kKnots, *coefs);
    auto merged_curve = spline::Merge(scaling_curve, drag_curve);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    std::copy_n(merged_curve.data(), spline::kCoefsSize, &pout->drags[0]);

    pimpl->ballistic_coefficient_psi = PmsiT(1);
    pimpl->atmosphere_reference = kLobAtmosphereReferenceIcao;
    return;
  }

  const double kConvert =
      pimpl->atmosphere_reference == kLobAtmosphereReferenceArmyStandardMetro
          ? kArmyToIcaoBcConversionFactor
          : 1.0;
  const float kInvBc =
      1.0F /
      static_cast<float>(pimpl->ballistic_coefficient_psi.Value() * kConvert);
  for (size_t i = 0; i < spline::kCoefsSize; ++i) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    pout->drags[i] = coefs->at(i) * kInvBc;
  }
}

void BuildCoefficients(Impl* pimpl, LobContext* pout) {
  assert(pimpl != nullptr && pout != nullptr);

  if (pimpl->ballistic_coefficient_psi.IsNaN()) {
    pout->error = kLobErrorBallisticCoefficientRequired;
    return;
  }

  if (pimpl->ballistic_coefficient_psi <= PmsiT(0.0)) {
    pout->error = kLobErrorBallisticCoefficientOOR;
    return;
  }

  assert(!pimpl->air_density_lbs_per_cu_ft.IsNaN());
  pout->drag_coeff =
      CalculateCdCoefficient(pimpl->air_density_lbs_per_cu_ft, PmsiT(1));
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
  assert(pimpl != nullptr && pout != nullptr);

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
  const auto kCD0 = pimpl->drag_table_mode == DragTableMode::kStandard ||
                            pimpl->drag_table_mode == DragTableMode::kBcBands
                        ? boatright::CalculateZeroYawDragCoefficientOfDrag(
                              kCdRef, kMass, kD, PmsiT(1))
                        : static_cast<double>(kCdRef);
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

  const FpsT kTransonicBarrier(MachT(1.2), kSos);
  constexpr SecT kTransonicTimeout(60.0);
  while (s.V().X() > kTransonicBarrier) {
    if (s.TOF() > kTransonicTimeout) {
      pout->error = kLobErrorInternalError;
      return;
    }
    SolveStep(*pout, &s, &drag_curve);
  }

  const auto kV = boatright::CalculateKV(kVelocity, kTransonicBarrier);
  const auto kOmega = boatright::CalculateKOmega(kD, s.TOF());
  const double kQTS = boatright::CalculatePotentialDragForce(
      kD, pimpl->air_density_lbs_per_cu_ft, kTransonicBarrier);
  const auto kBetaROfT = boatright::CalculateYawOfRepose(
      kVelocity, kTwist, kIyOverIx, kR, kOmega, kV);

  PmsiT bc_g7(0);
  if (pimpl->drag_table_mode == DragTableMode::kStandard &&
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
  assert(pimpl != nullptr && pout != nullptr);

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

  pout->aerodynamic_jump = MoaT(0).Value();
}

void BuildZeroAngle(Impl* pimpl, LobContext* pout) {
  assert(pimpl != nullptr && pout != nullptr);

  if (!std::isnan(pimpl->zero_angle_moa)) {
    if (pimpl->zero_angle_moa > constant::kMaxAngle ||
        pimpl->zero_angle_moa < constant::kMinAngle) {
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

  assert(!std::isnan(pout->aerodynamic_jump));

  if (std::isnan(pimpl->zero_impact_height)) {
    pimpl->zero_impact_height = FeetT(0.0);
  }

  assert(pimpl->velocity_fps > FpsT(0));
  const auto kVacuumSeed = RadiansT(
      0.5 * kStandardGravityFtPerSecSq * pimpl->zero_distance_ft.Value() /
      (pimpl->velocity_fps * pimpl->velocity_fps).Value());
  const auto kClampedVacuumSeed =
      std::max(constant::kMinAngle, std::min(constant::kMaxAngle, kVacuumSeed));
  const MoaT kAngle = SolveAngle(*pout, pimpl->zero_distance_ft,
                                 pimpl->zero_impact_height, kClampedVacuumSeed);
  if (std::isnan(kAngle)) {
    pout->error = kLobErrorZeroUnreachable;
    return;
  }
  pout->zero_angle = kAngle.Value();
}

void BuildOptions(Impl* pimpl, LobContext* pout) {
  assert(pimpl != nullptr && pout != nullptr);

  if (pout->max_time < 0.0) {
    pout->error = kLobErrorMaximumTimeOOR;
    return;
  }

  const FpsT kMinSpeed = CalculateVelocityFromKineticEnergy(
      pimpl->minimum_energy_ft_lbs, SlugT(pimpl->mass_lbs));
  pout->minimum_speed =
      std::max(pout->minimum_speed,
               kMinSpeed.IsNaN() ? static_cast<uint16_t>(0) : kMinSpeed.U16());
}

}  // namespace
}  // namespace lob

extern "C" {
using namespace lob;  // NOLINT(google-build-using-namespace)

void LobBuilderInit(LobBuilder* pbuilder) {
  static_assert(sizeof(Impl) <= LOB_BUILDER_BUFFER_SIZE,
                "LOB_BUILDER_BUFFER_SIZE too small");
  if (pbuilder == nullptr) {
    return;
  }
  ::new (&pbuilder->buffer) Impl();
}

void LobBuilderDestroy(LobBuilder* pbuilder) {
  if (pbuilder != nullptr) {
    Pimpl(pbuilder)->~Impl();
  }
}

void LobBuilderCopy(LobBuilder* dst, const LobBuilder* src) {
  if (dst == nullptr || src == nullptr) {
    return;
  }
  if (dst != src) {
    Pimpl(dst)->~Impl();
    ::new (&dst->buffer) Impl(*Pimpl(src));
  }
}

LobBuilder* LobBuilderReset(LobBuilder* pbuilder) {
  if (pbuilder == nullptr) {
    return nullptr;
  }
  auto* pimpl = Pimpl(pbuilder);
  pimpl->~Impl();
  ::new (&pbuilder->buffer) Impl();
  return pbuilder;
}

LobBuilder* LobBuilderBallisticCoefficientPsi(LobBuilder* pbuilder,
                                              double value) {
  if (pbuilder == nullptr) {
    return nullptr;
  }
  auto* pimpl = Pimpl(pbuilder);
  pimpl->ballistic_coefficient_psi = PmsiT(value);
  return pbuilder;
}

LobBuilder* LobBuilderBCAtmosphere(LobBuilder* pbuilder,
                                   LobAtmosphereReferenceT type) {
  if (pbuilder == nullptr) {
    return nullptr;
  }
  auto* pimpl = Pimpl(pbuilder);
  pimpl->atmosphere_reference = type;
  return pbuilder;
}

LobBuilder* LobBuilderBCDragFunction(LobBuilder* pbuilder,
                                     LobDragFunctionT type) {
  if (pbuilder == nullptr) {
    return nullptr;
  }
  auto* pimpl = Pimpl(pbuilder);
  pimpl->drag_function = type;
  return pbuilder;
}

LobBuilder* LobBuilderDiameterInch(LobBuilder* pbuilder, double value) {
  if (pbuilder == nullptr) {
    return nullptr;
  }
  auto* pimpl = Pimpl(pbuilder);
  pimpl->diameter_in = InchT(value);
  return pbuilder;
}

LobBuilder* LobBuilderMeplatDiameterInch(LobBuilder* pbuilder, double value) {
  if (pbuilder == nullptr) {
    return nullptr;
  }
  auto* pimpl = Pimpl(pbuilder);
  pimpl->meplat_diameter_in = InchT(value);
  return pbuilder;
}

LobBuilder* LobBuilderBaseDiameterInch(LobBuilder* pbuilder, double value) {
  if (pbuilder == nullptr) {
    return nullptr;
  }
  auto* pimpl = Pimpl(pbuilder);
  pimpl->base_diameter_in = InchT(value);
  return pbuilder;
}

LobBuilder* LobBuilderLengthInch(LobBuilder* pbuilder, double value) {
  if (pbuilder == nullptr) {
    return nullptr;
  }
  auto* pimpl = Pimpl(pbuilder);
  pimpl->length_in = InchT(value);
  return pbuilder;
}

LobBuilder* LobBuilderNoseLengthInch(LobBuilder* pbuilder, double value) {
  if (pbuilder == nullptr) {
    return nullptr;
  }
  auto* pimpl = Pimpl(pbuilder);
  pimpl->nose_length_in = InchT(value);
  return pbuilder;
}

LobBuilder* LobBuilderTailLengthInch(LobBuilder* pbuilder, double value) {
  if (pbuilder == nullptr) {
    return nullptr;
  }
  auto* pimpl = Pimpl(pbuilder);
  pimpl->tail_length_in = InchT(value);
  return pbuilder;
}

LobBuilder* LobBuilderOgiveRtR(LobBuilder* pbuilder, double value) {
  if (pbuilder == nullptr) {
    return nullptr;
  }
  auto* pimpl = Pimpl(pbuilder);
  pimpl->ogive_rtr = value;
  return pbuilder;
}

LobBuilder* LobBuilderSplineFitTable(LobBuilder* pbuilder, const float* pmachs,
                                     const float* pdrags, size_t size) {
  if (pbuilder == nullptr || pmachs == nullptr || pdrags == nullptr) {
    return pbuilder;
  }
  auto* pimpl = Pimpl(pbuilder);
  pimpl->table_xs = pmachs;
  pimpl->table_ys = pdrags;
  pimpl->table_count = size;
  pimpl->drag_table_mode = DragTableMode::kCustomTable;
  return pbuilder;
}

LobBuilder* LobBuilderBCVelocityBands(LobBuilder* pbuilder, const float* pfps,
                                      const float* pbcs, size_t size) {
  if (pbuilder == nullptr || pfps == nullptr || pbcs == nullptr) {
    return pbuilder;
  }
  auto* pimpl = Pimpl(pbuilder);
  pimpl->table_xs = pfps;
  pimpl->table_ys = pbcs;
  pimpl->table_count = size;
  pimpl->drag_table_mode = DragTableMode::kBcBands;
  return pbuilder;
}

LobBuilder* LobBuilderMassGrains(LobBuilder* pbuilder, double value) {
  if (pbuilder == nullptr) {
    return nullptr;
  }
  auto* pimpl = Pimpl(pbuilder);
  pimpl->mass_lbs = LbsT(GrainT(value));
  return pbuilder;
}

LobBuilder* LobBuilderInitialVelocityFps(LobBuilder* pbuilder, uint16_t value) {
  if (pbuilder == nullptr) {
    return nullptr;
  }
  auto* pimpl = Pimpl(pbuilder);
  pimpl->velocity_fps = FpsT(value);
  return pbuilder;
}

LobBuilder* LobBuilderOpticHeightInches(LobBuilder* pbuilder, double value) {
  if (pbuilder == nullptr) {
    return nullptr;
  }
  auto* pimpl = Pimpl(pbuilder);
  pimpl->optic_height_ft = FeetT(InchT(value));
  return pbuilder;
}

LobBuilder* LobBuilderTwistInchesPerTurn(LobBuilder* pbuilder, double value) {
  if (pbuilder == nullptr) {
    return nullptr;
  }
  auto* pimpl = Pimpl(pbuilder);
  pimpl->twist_inches_per_turn = InchPerTwistT(value);
  return pbuilder;
}

LobBuilder* LobBuilderZeroAngleMOA(LobBuilder* pbuilder, double value) {
  if (pbuilder == nullptr) {
    return nullptr;
  }
  auto* pimpl = Pimpl(pbuilder);
  pimpl->zero_angle_moa = MoaT(value);
  return pbuilder;
}

LobBuilder* LobBuilderZeroDistanceYds(LobBuilder* pbuilder, double value) {
  if (pbuilder == nullptr) {
    return nullptr;
  }
  auto* pimpl = Pimpl(pbuilder);
  pimpl->zero_distance_ft = YardT(value);
  return pbuilder;
}

LobBuilder* LobBuilderZeroImpactHeightInches(LobBuilder* pbuilder,
                                             double value) {
  if (pbuilder == nullptr) {
    return nullptr;
  }
  auto* pimpl = Pimpl(pbuilder);
  pimpl->zero_impact_height = InchT(value);
  return pbuilder;
}

LobBuilder* LobBuilderAltitudeOfFiringSiteFt(LobBuilder* pbuilder,
                                             double value) {
  if (pbuilder == nullptr) {
    return nullptr;
  }
  auto* pimpl = Pimpl(pbuilder);
  pimpl->altitude_ft = FeetT(value);
  return pbuilder;
}

LobBuilder* LobBuilderAirPressureInHg(LobBuilder* pbuilder, double value) {
  if (pbuilder == nullptr) {
    return nullptr;
  }
  auto* pimpl = Pimpl(pbuilder);
  pimpl->air_pressure_in_hg = InHgT(value);
  return pbuilder;
}

LobBuilder* LobBuilderAltitudeOfBarometerFt(LobBuilder* pbuilder,
                                            double value) {
  if (pbuilder == nullptr) {
    return nullptr;
  }
  auto* pimpl = Pimpl(pbuilder);
  pimpl->altitude_of_barometer_ft = FeetT(value);
  return pbuilder;
}

LobBuilder* LobBuilderTemperatureDegF(LobBuilder* pbuilder, double value) {
  if (pbuilder == nullptr) {
    return nullptr;
  }
  auto* pimpl = Pimpl(pbuilder);
  pimpl->temperature_deg_f = DegFT(value);
  return pbuilder;
}

LobBuilder* LobBuilderAltitudeOfThermometerFt(LobBuilder* pbuilder,
                                              double value) {
  if (pbuilder == nullptr) {
    return nullptr;
  }
  auto* pimpl = Pimpl(pbuilder);
  pimpl->altitude_of_thermometer_ft = FeetT(value);
  return pbuilder;
}

LobBuilder* LobBuilderRelativeHumidityPercent(LobBuilder* pbuilder,
                                              double value) {
  if (pbuilder == nullptr) {
    return nullptr;
  }
  auto* pimpl = Pimpl(pbuilder);
  pimpl->relative_humidity_percent = PercentT(value);
  return pbuilder;
}

LobBuilder* LobBuilderWindHeading(LobBuilder* pbuilder, LobClockAngleT value) {
  if (pbuilder == nullptr) {
    return nullptr;
  }
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
  if (pbuilder == nullptr) {
    return nullptr;
  }
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
  if (pbuilder == nullptr) {
    return nullptr;
  }
  auto* pimpl = Pimpl(pbuilder);
  pimpl->wind_speed_fps = FpsT(value);
  return pbuilder;
}

LobBuilder* LobBuilderWindSpeedMph(LobBuilder* pbuilder, double value) {
  if (pbuilder == nullptr) {
    return nullptr;
  }
  auto* pimpl = Pimpl(pbuilder);
  pimpl->wind_speed_fps = MphT(value);
  return pbuilder;
}

LobBuilder* LobBuilderAzimuthDeg(LobBuilder* pbuilder, double value) {
  if (pbuilder == nullptr) {
    return nullptr;
  }
  auto* pimpl = Pimpl(pbuilder);
  pimpl->azimuth_rad = DegreesT(value);
  return pbuilder;
}

LobBuilder* LobBuilderLatitudeDeg(LobBuilder* pbuilder, double value) {
  if (pbuilder == nullptr) {
    return nullptr;
  }
  auto* pimpl = Pimpl(pbuilder);
  pimpl->latitude_rad = DegreesT(value);
  return pbuilder;
}

LobBuilder* LobBuilderRangeAngleDeg(LobBuilder* pbuilder, double value) {
  if (pbuilder == nullptr) {
    return nullptr;
  }
  auto* pimpl = Pimpl(pbuilder);
  pimpl->range_angle_rad = RadiansT(DegreesT(value));
  return pbuilder;
}

LobBuilder* LobBuilderMinimumSpeed(LobBuilder* pbuilder, uint16_t value) {
  if (pbuilder == nullptr) {
    return nullptr;
  }
  auto* pimpl = Pimpl(pbuilder);
  pimpl->minimum_speed_fps = FpsT(value);
  return pbuilder;
}

LobBuilder* LobBuilderMinimumEnergy(LobBuilder* pbuilder, uint16_t value) {
  if (pbuilder == nullptr) {
    return nullptr;
  }
  auto* pimpl = Pimpl(pbuilder);
  pimpl->minimum_energy_ft_lbs = FtLbsT(value);
  return pbuilder;
}

LobBuilder* LobBuilderMaximumTime(LobBuilder* pbuilder, double value) {
  if (pbuilder == nullptr) {
    return nullptr;
  }
  auto* pimpl = Pimpl(pbuilder);
  pimpl->max_time_sec = SecT(value);
  return pbuilder;
}

LobBuilder* LobBuilderStepSize(LobBuilder* pbuilder, uint16_t value) {
  if (pbuilder == nullptr) {
    return nullptr;
  }
  auto* pimpl = Pimpl(pbuilder);
  pimpl->step_size_in = value;
  return pbuilder;
}

void LobBuilderBuild(LobBuilder* pbuilder, LobContext* presult) {
  if (pbuilder == nullptr || presult == nullptr) {
    return;
  }
  auto* pimpl = Pimpl(pbuilder);

  presult->mass = pimpl->mass_lbs.Value();
  presult->velocity =
      pimpl->velocity_fps.IsNaN() ? 0 : pimpl->velocity_fps.U16();
  presult->minimum_speed =
      pimpl->minimum_speed_fps.IsNaN() ? 0 : pimpl->minimum_speed_fps.U16();
  presult->step_size = pimpl->step_size_in;
  presult->max_time = pimpl->max_time_sec.Value();
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
  BuildSpline(pimpl, presult);
  if (presult->error != kLobErrorNotFormed) {
    return;
  }
  BuildCoefficients(pimpl, presult);
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
  BuildOptions(pimpl, presult);
  if (presult->error != kLobErrorNotFormed) {
    return;
  }
  BuildZeroAngle(pimpl, presult);
  if (presult->error != kLobErrorNotFormed) {
    return;
  }

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
