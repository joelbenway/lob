// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

#include "boatright.hpp"
#include "calc.hpp"
#include "constants.hpp"
#include "eng_units.hpp"
#include "gauss_legendre.hpp"
#include "helpers.hpp"
#include "litz.hpp"
#include "lob/lob.h"
#include "solve_angle.hpp"
#include "splines.hpp"

namespace lob {

namespace {

enum class DragTableMode : uint8_t {
  kBcBands,
  kCustomTable,
  kNativeCoefs,
  kStandard,
};

enum class WindTableMode : uint8_t {
  kProfile,
  kUniform,
};

constexpr double kMaxBallisticCoefficient = 1e5;

bool HasLateralPerturbation(const LobContext& ctx) noexcept {
  if (std::fabs(ctx.coriolis.sin_l) > 0.0 ||
      std::fabs(ctx.coriolis.cos_l_cos_a) > 0.0) {
    return true;
  }
  return std::any_of(
      std::begin(ctx.wind_nodes), std::end(ctx.wind_nodes),
      [](const LobWindNode& node) { return std::fabs(node.z_fps) > 0.0; });
}

double EstimateDragPerDistance(const LobContext& ctx,
                               double velocity_fps) noexcept {
  if (ctx.speed_of_sound <= 0.0) {
    return 0.0;
  }
  spline::CurveView drag_curve(spline::kKnots.data(), &ctx.drags[0]);
  const double kMach = velocity_fps / ctx.speed_of_sound;
  const double kCd = drag_curve.Eval(MachT(kMach));
  if (std::isfinite(kCd) && kCd > 0.0 && std::isfinite(ctx.drag_coeff) &&
      ctx.drag_coeff > 0.0) {
    return ctx.drag_coeff * kCd;
  }
  return 0.0;
}

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
  FeetT optic_offset_ft{NaN()};
  RadiansT range_angle_rad{NaN()};
  PercentT relative_humidity_percent{NaN()};
  InchT tail_length_in{NaN()};
  DegFT temperature_deg_f{NaN()};
  InchPerTwistT twist_inches_per_turn{NaN()};
  FpsT velocity_fps{NaN()};
  RadiansT wind_heading_rad{NaN()};
  FpsT wind_speed_fps{NaN()};
  MoaT zero_angle_moa{NaN()};
  MoaT zero_azimuth_moa{NaN()};
  FeetT zero_distance_ft{NaN()};
  FeetT zero_impact_height{NaN()};

  const LobWindPoint* wind_profile_points{nullptr};
  size_t wind_profile_count{0};
  double wind_shear_exponent{0.0};
  size_t table_count{0};
  const float* table_xs{nullptr};
  const float* table_ys{nullptr};

  uint16_t step_size_in{0};
  LobAtmosphereReferenceT atmosphere_reference{
      kLobAtmosphereReferenceArmyStandardMetro};
  LobDragFunctionT drag_function{kLobDragFunctionG1};
  DragTableMode drag_table_mode{DragTableMode::kStandard};
  WindTableMode wind_table_mode{WindTableMode::kUniform};
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

void BuildDynamicDensity(DegFT temperature_at_firing_site, LobContext* pout) {
  assert(pout != nullptr);
  constexpr double kInvLapseDenom =
      (1.0 / isa::kGasConstantAir) -
      (isa::kLapseDegFPerFt / kStandardGravityFtPerSecSq);
  const DegRT kTemp = DegRT(temperature_at_firing_site);
  assert(kTemp.Value() > 0.0);
  pout->k_lapse = kInvLapseDenom / kTemp.Value();
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void BuildEnvironment(Impl* pimpl, LobContext* pout) {
  assert(pimpl != nullptr && pout != nullptr);
  FeetT altitude_of_firing_site = FeetT(0);
  FeetT altitude_of_barometer = FeetT(0);
  FeetT altitude_of_thermometer = FeetT(0);
  DegFT temperature_at_firing_site = DegFT(isa::kSeaLevelDegF);
  DegFT temperature_at_barometer = DegFT(isa::kSeaLevelDegF);
  InHgT pressure_at_firing_site = InHgT(isa::kSeaLevelPressureInHg);

  if (std::isnan(pimpl->range_angle_rad)) {
    pimpl->range_angle_rad = RadiansT(DegreesT(0));
  }

  if (!std::isfinite(pimpl->range_angle_rad.Value())) {
    pout->error = kLobErrorRangeAngleOOR;
    return;
  }

  const bool kRangeAngleValid = pimpl->range_angle_rad > DegreesT(-90.0) &&
                                pimpl->range_angle_rad < DegreesT(90.0);
  if (!kRangeAngleValid) {
    pout->error = kLobErrorRangeAngleOOR;
    return;
  }

  if (!(pimpl->range_angle_rad > RadiansT(0) ||
        pimpl->range_angle_rad < RadiansT(0))) {
    pout->gravity.x = 0.0;
    pout->gravity.y = -kStandardGravityFtPerSecSq;
  } else {
    pout->gravity.x = kStandardGravityFtPerSecSq * -1 *
                      std::sin(pimpl->range_angle_rad.Value());
    pout->gravity.y = kStandardGravityFtPerSecSq * -1 *
                      std::cos(pimpl->range_angle_rad.Value());
  }

  auto is_altitude_valid = [](FeetT altitude) -> bool {
    return std::isfinite(altitude.Value()) &&
           FeetT(-isa::kStratosphereAltitudeFt) < altitude &&
           altitude < FeetT(isa::kStratosphereAltitudeFt);
  };

  if (!std::isnan(pimpl->altitude_ft)) {
    if (!is_altitude_valid(pimpl->altitude_ft)) {
      pout->error = kLobErrorAltitudeOfFiringSiteOOR;
      return;
    }
    altitude_of_firing_site = pimpl->altitude_ft;
  }

  if (!std::isnan(pimpl->altitude_of_barometer_ft)) {
    if (!is_altitude_valid(pimpl->altitude_of_barometer_ft)) {
      pout->error = kLobErrorAltitudeOfBarometerOOR;
      return;
    }
    altitude_of_barometer = pimpl->altitude_of_barometer_ft;
  } else {
    altitude_of_barometer = altitude_of_firing_site;
  }

  if (!std::isnan(pimpl->altitude_of_thermometer_ft)) {
    if (!is_altitude_valid(pimpl->altitude_of_thermometer_ft)) {
      pout->error = kLobErrorAltitudeOfThermometerOOR;
      return;
    }
    altitude_of_thermometer = pimpl->altitude_of_thermometer_ft;
  } else {
    altitude_of_thermometer = altitude_of_firing_site;
  }

  temperature_at_firing_site = CalculateTemperatureAtAltitude(
      altitude_of_firing_site, DegFT(isa::kSeaLevelDegF));
  pressure_at_firing_site = BarometricFormula(
      altitude_of_firing_site, InHgT(isa::kSeaLevelPressureInHg),
      DegFT(isa::kSeaLevelDegF));

  if (!std::isnan(pimpl->temperature_deg_f)) {
    if (!std::isfinite(pimpl->temperature_deg_f.Value()) ||
        pimpl->temperature_deg_f < DegFT(kAbsoluteZeroDegF)) {
      pout->error = kLobErrorAltitudeOfThermometerOOR;
      return;
    }
    temperature_at_firing_site = CalculateTemperatureAtAltitude(
        altitude_of_firing_site - altitude_of_thermometer,
        pimpl->temperature_deg_f);
    temperature_at_barometer = CalculateTemperatureAtAltitude(
        altitude_of_barometer - altitude_of_thermometer,
        pimpl->temperature_deg_f);
  }

  if (!std::isnan(pimpl->air_pressure_in_hg)) {
    if (!std::isfinite(pimpl->air_pressure_in_hg.Value()) ||
        pimpl->air_pressure_in_hg <= InHgT(0.0)) {
      pout->error = kLobErrorAirPressureOOR;
      return;
    }
    pressure_at_firing_site =
        BarometricFormula(altitude_of_firing_site - altitude_of_barometer,
                          pimpl->air_pressure_in_hg, temperature_at_barometer);
  }

  if (std::isnan(pimpl->relative_humidity_percent)) {
    pimpl->relative_humidity_percent = PercentT(isa::kSeaLevelHumidityPercent);
  }

  if (!std::isfinite(pimpl->relative_humidity_percent.Value()) ||
      pimpl->relative_humidity_percent < PercentT(0.0) ||
      pimpl->relative_humidity_percent > PercentT(100.0)) {
    pout->error = kLobErrorHumidityOOR;
    return;
  }

  const double kAirDensityRatio = CalculateAirDensityRatio(
      pressure_at_firing_site, temperature_at_firing_site);

  const InHgT kWaterVaporSaturationPressureInHg =
      pimpl->relative_humidity_percent > PercentT(0.0)
          ? CalculateWaterVaporSaturationPressure(temperature_at_firing_site)
          : InHgT(0.0);
  const double kHumidityCorrection =
      pimpl->relative_humidity_percent > PercentT(0.0)
          ? CalculateAirDensityRatioHumidityCorrection(
                pimpl->relative_humidity_percent,
                kWaterVaporSaturationPressureInHg)
          : 1.0;
  const double kSpeedOfSoundCorrection =
      pimpl->relative_humidity_percent > PercentT(0.0)
          ? CalculateSpeedOfSoundHumidityCorrection(
                pimpl->relative_humidity_percent,
                kWaterVaporSaturationPressureInHg)
          : 1.0;

  const LbsPerCuFtT kAirDensity(isa::kSeaLevelAirDensityLbsPerCuFt *
                                kAirDensityRatio * kHumidityCorrection);

  pimpl->air_density_lbs_per_cu_ft = kAirDensity;

  const FpsT kSpeedOfSound =
      CalculateSpeedOfSoundInAir(temperature_at_firing_site) *
      kSpeedOfSoundCorrection;

  pout->speed_of_sound = kSpeedOfSound.Value();

  BuildDynamicDensity(temperature_at_firing_site, pout);
}

RadiansT HeadingDegToRad(double value) {
  const DegreesT kFullTurn(kDegreesPerTurn);
  const DegreesT kQuarterTurn(kFullTurn / 4);
  DegreesT angle(value);

  angle = angle * -1 + kQuarterTurn;

  if (angle < DegreesT(0)) {
    angle += kFullTurn;
  }

  return RadiansT(angle);
}

LobErrorT ValidateWindProfile(Impl* pimpl) {
  assert(pimpl != nullptr);
  if (pimpl->wind_profile_points == nullptr || pimpl->wind_profile_count == 0 ||
      pimpl->wind_profile_count > LOB_WIND_POINTS) {
    return pimpl->wind_profile_count > LOB_WIND_POINTS
               ? kLobErrorWindProfileTooLong
               : kLobErrorWindProfileInvalid;
  }
  for (size_t i = 0; i < pimpl->wind_profile_count; i++) {
    const LobWindPoint& point = pimpl->wind_profile_points[i];
    if (!std::isfinite(point.range_ft) || !std::isfinite(point.heading_deg) ||
        !std::isfinite(point.speed_mph) ||
        (!std::isnan(point.height_ft) && !std::isfinite(point.height_ft))) {
      return kLobErrorWindProfileInvalid;
    }
    if (i == 0) {
      // Exact-zero test via ordered comparisons (avoids -Werror=float-equal).
      if (point.range_ft > 0.0 || point.range_ft < 0.0) {
        return kLobErrorWindProfileNotMonotonic;
      }
    } else if (!(point.range_ft > pimpl->wind_profile_points[i - 1].range_ft)) {
      return kLobErrorWindProfileNotMonotonic;
    }
    // Whole feet within uint32 storage (monotonicity rules out negatives).
    const double kTruncatedRange = std::floor(point.range_ft);
    if (point.range_ft > kTruncatedRange || point.range_ft < kTruncatedRange ||
        !(point.range_ft <=
          static_cast<double>(std::numeric_limits<uint32_t>::max()))) {
      return kLobErrorWindProfileInvalid;
    }
    if (!std::isnan(point.height_ft) && !(point.height_ft > 0.0)) {
      return kLobErrorWindProfileInvalid;
    }
  }
  return kLobErrorNone;
}

LobErrorT ValidateCustomTable(Impl* pimpl) {
  assert(pimpl != nullptr);
  if (pimpl->table_count < 2) {
    return kLobErrorMachDragTableTooShort;
  }
  for (size_t i = 0; i < pimpl->table_count; i++) {
    const float kX = pimpl->table_xs[i];
    const float kY = pimpl->table_ys[i];
    if (kX < 0.0F || kY < 0.0F || !std::isfinite(kX) || !std::isfinite(kY)) {
      return kLobErrorMachDragTableInvalid;
    }
    if (i > 0 && kX <= pimpl->table_xs[i - 1]) {
      return kLobErrorMachDragTableNotMonotonic;
    }
  }
  return kLobErrorNone;
}

LobErrorT BuildCustomTableSpline(Impl* pimpl, LobContext* pout) {
  spline::Build(pimpl->table_xs, pimpl->table_ys, pimpl->table_count,
                spline::kKnots.data(), spline::kKnotCount, &pout->drags[0]);
  spline::CurveView curve(spline::kKnots.data(), &pout->drags[0]);
  for (const float kKnot : spline::kKnots) {
    const float kCd = curve.Eval(kKnot);
    if (kCd < 0.0F || !std::isfinite(kCd)) {
      return kLobErrorMachDragTableInvalid;
    }
  }
  return kLobErrorNone;
}

LobErrorT ValidateBcBands(Impl* pimpl) {
  assert(pimpl != nullptr);
  if (pimpl->table_count < 2) {
    return kLobErrorBcBandsTooShort;
  }
  if (pimpl->table_count > spline::kKnotCount) {
    return kLobErrorBcBandsInvalid;
  }
  for (size_t i = 0; i < pimpl->table_count; i++) {
    const float kX = pimpl->table_xs[i];
    const float kY = pimpl->table_ys[i];
    if (kX <= 0.0F || kY <= 0.0F || !std::isfinite(kX) || !std::isfinite(kY)) {
      return kLobErrorBcBandsInvalid;
    }
    if (i > 0 && kX <= pimpl->table_xs[i - 1]) {
      return kLobErrorBcBandsNotMonotonic;
    }
  }
  return kLobErrorNone;
}

LobErrorT BuildBcBandsSpline(
    Impl* pimpl, LobContext* pout,
    const std::array<float, spline::kCoefsSize>* coefs) {
  const auto kSos = static_cast<float>(pout->speed_of_sound);
  if (pimpl->table_xs[pimpl->table_count - 1] / kSos >= spline::kKnots.back()) {
    return kLobErrorBcBandsInvalid;
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
  std::copy_n(merged_curve.data(), spline::kCoefsSize, &pout->drags[0]);
  return kLobErrorNone;
}

LobErrorT BuildNativeSpline(const float* src, LobContext* pout) {
  assert(src != nullptr && pout != nullptr);
  if (std::any_of(src, src + spline::kCoefsSize,
                  [](float v) { return !std::isfinite(v); })) {
    return kLobErrorSplineCoefsInvalid;
  }
  spline::CurveView curve(spline::kKnots.data(), src);
  for (const float kKnot : spline::kKnots) {
    const float kCd = curve.Eval(kKnot);
    if (kCd < 0.0F || !std::isfinite(kCd)) {
      return kLobErrorSplineCoefsInvalid;
    }
  }
  std::copy_n(src, spline::kCoefsSize, &pout->drags[0]);
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
    const LobErrorT kBuildErr = BuildCustomTableSpline(pimpl, pout);
    if (kBuildErr != kLobErrorNone) {
      pout->error = kBuildErr;
      return;
    }
    return;
  }

  if (pimpl->drag_table_mode == DragTableMode::kBcBands) {
    const LobErrorT kErr = ValidateBcBands(pimpl);
    if (kErr != kLobErrorNone) {
      pout->error = kErr;
      return;
    }
    const LobErrorT kBuildErr = BuildBcBandsSpline(pimpl, pout, coefs);
    if (kBuildErr != kLobErrorNone) {
      pout->error = kBuildErr;
      return;
    }
    return;
  }

  if (pimpl->drag_table_mode == DragTableMode::kNativeCoefs) {
    assert(pimpl->table_ys != nullptr);
    const LobErrorT kErr = BuildNativeSpline(pimpl->table_ys, pout);
    if (kErr != kLobErrorNone) {
      pout->error = kErr;
      return;
    }
    return;
  }

  if (pimpl->ballistic_coefficient_psi.IsNaN()) {
    pout->error = kLobErrorBallisticCoefficientRequired;
    return;
  }

  if (!std::isfinite(pimpl->ballistic_coefficient_psi.Value()) ||
      pimpl->ballistic_coefficient_psi <= PmsiT(0.0) ||
      pimpl->ballistic_coefficient_psi > PmsiT(kMaxBallisticCoefficient)) {
    pout->error = kLobErrorBallisticCoefficientOOR;
    return;
  }

  const double kConvert =
      pimpl->atmosphere_reference == kLobAtmosphereReferenceArmyStandardMetro
          ? kArmyToIcaoBcConversionFactor
          : 1.0;

  const float kScale =
      (pimpl->ballistic_coefficient_psi * kConvert).Inverse().Float();

  for (size_t i = 0; i < spline::kCoefsSize; ++i) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
    pout->drags[i] = coefs->at(i) * kScale;
  }
}

void BuildCoefficients(Impl* pimpl, LobContext* pout) {
  assert(pimpl != nullptr && pout != nullptr);

  if (pimpl->drag_table_mode == DragTableMode::kStandard) {
    if (pimpl->ballistic_coefficient_psi.IsNaN()) {
      pout->error = kLobErrorBallisticCoefficientRequired;
      return;
    }

    if (!std::isfinite(pimpl->ballistic_coefficient_psi.Value()) ||
        pimpl->ballistic_coefficient_psi <= PmsiT(0.0) ||
        pimpl->ballistic_coefficient_psi > PmsiT(kMaxBallisticCoefficient)) {
      pout->error = kLobErrorBallisticCoefficientOOR;
      return;
    }
  }

  assert(!pimpl->air_density_lbs_per_cu_ft.IsNaN());
  constexpr double kDragScale =
      CalculateCdCoefficient(LbsPerCuFtT(1), PmsiT(1));
  pout->drag_coeff = pimpl->air_density_lbs_per_cu_ft.Value() * kDragScale;
}
void BuildProfileWind(Impl* pimpl, LobContext* pout) {
  assert(pimpl != nullptr && pout != nullptr);

  const LobErrorT kErr = ValidateWindProfile(pimpl);
  if (kErr != kLobErrorNone) {
    pout->error = kErr;
    return;
  }

  const double kCos = std::cos(pimpl->range_angle_rad.Value());
  const double kSin = std::sin(pimpl->range_angle_rad.Value());

  // Raw pointer: count is runtime-validated (pro-bounds-constant-array-index).
  LobWindNode* wind_nodes = &pout->wind_nodes[0];

  for (size_t i = 0; i < pimpl->wind_profile_count; i++) {
    const LobWindPoint& point = pimpl->wind_profile_points[i];
    const double kHeightFactor =
        std::isnan(point.height_ft)
            ? 1.0
            : CalculatePowerLawWindFactor(FeetT(kWindReferenceHeightFt),
                                          FeetT(point.height_ft),
                                          pimpl->wind_shear_exponent);
    const double kSpeedFps = FpsT(MphT(point.speed_mph)).Value();
    double hx = 0.0;
    double hz = 0.0;
    if (kSpeedFps > 0.0 || kSpeedFps < 0.0) {
      const double kHeadingRad = HeadingDegToRad(point.heading_deg).Value();
      hx = FpsT(kSpeedFps * std::sin(kHeadingRad)).Value() * kHeightFactor;
      hz = FpsT(kSpeedFps * std::cos(kHeadingRad)).Value() * kHeightFactor;
    }
    wind_nodes[i].range_ft = static_cast<uint32_t>(point.range_ft);
    wind_nodes[i].x_fps = hx * kCos;
    wind_nodes[i].y_fps = -hx * kSin;
    wind_nodes[i].z_fps = hz;
  }
  pout->wind_count = static_cast<uint8_t>(pimpl->wind_profile_count);
}

void BuildUniformWind(Impl* pimpl, LobContext* pout) {
  assert(pimpl != nullptr && pout != nullptr);

  if (std::isnan(pimpl->wind_heading_rad)) {
    pimpl->wind_heading_rad = HeadingDegToRad(0.0);
  }

  const DegreesT kFullTurn(kDegreesPerTurn);
  if (!std::isfinite(pimpl->wind_heading_rad.Value()) ||
      pimpl->wind_heading_rad > kFullTurn ||
      pimpl->wind_heading_rad < kFullTurn * -1) {
    pout->error = kLobErrorWindHeadingOOR;
    return;
  }

  if (std::isnan(pimpl->wind_speed_fps)) {
    pimpl->wind_speed_fps = FpsT(0);
  }

  if (!std::isfinite(pimpl->wind_speed_fps.Value())) {
    pout->error = kLobErrorWindProfileInvalid;
    return;
  }

  const double kCos = std::cos(pimpl->range_angle_rad.Value());
  const double kSin = std::sin(pimpl->range_angle_rad.Value());

  LobWindNode* wind_nodes = &pout->wind_nodes[0];

  double wind_x_fps = 0.0;
  double wind_z_fps = 0.0;
  if (pimpl->wind_speed_fps > FpsT(0) || pimpl->wind_speed_fps < FpsT(0)) {
    wind_x_fps =
        FpsT(pimpl->wind_speed_fps * std::sin(pimpl->wind_heading_rad.Value()))
            .Value();
    wind_z_fps =
        FpsT(pimpl->wind_speed_fps * std::cos(pimpl->wind_heading_rad.Value()))
            .Value();
  }
  wind_nodes[0].range_ft = 0U;
  wind_nodes[0].x_fps = wind_x_fps * kCos;
  wind_nodes[0].y_fps = -wind_x_fps * kSin;
  wind_nodes[0].z_fps = wind_z_fps;
  pout->wind_count = 1;
}

void BuildWind(Impl* pimpl, LobContext* pout) {
  assert(pimpl != nullptr && pout != nullptr);

  constexpr double kMinWindShearExponent = 0.0;
  constexpr double kMaxWindShearExponent = 1.0;
  if (!std::isfinite(pimpl->wind_shear_exponent) ||
      pimpl->wind_shear_exponent < kMinWindShearExponent ||
      pimpl->wind_shear_exponent > kMaxWindShearExponent) {
    pout->error = kLobErrorWindProfileInvalid;
    return;
  }
  pout->wind_shear_exponent = pimpl->wind_shear_exponent;

  if (pimpl->wind_table_mode == WindTableMode::kProfile) {
    BuildProfileWind(pimpl, pout);
  } else {
    BuildUniformWind(pimpl, pout);
  }
  if (pout->error != kLobErrorNotFormed) {
    return;
  }

  LobWindNode* wind_nodes = &pout->wind_nodes[0];
  for (size_t i = pout->wind_count; i < LOB_WIND_POINTS; i++) {
    wind_nodes[i] = LobWindNode{};
  }
}

void BuildOpticHeight(Impl* pimpl, LobContext* pout) {
  assert(pimpl != nullptr && pout != nullptr);
  if (!std::isnan(pimpl->optic_height_ft)) {
    pout->optic_height = pimpl->optic_height_ft.Value();
  } else {
    constexpr FeetT kDefaultOpticHeight = InchT(1.5);
    pout->optic_height = kDefaultOpticHeight.Value();
  }
  if (!std::isnan(pimpl->optic_offset_ft)) {
    pout->optic_offset = pimpl->optic_offset_ft.Value();
  } else {
    pout->optic_offset = 0.0;
  }
}

void BuildStability(Impl* pimpl, LobContext* pout) {
  assert(pimpl != nullptr && pout != nullptr);

  if (pimpl->velocity_fps.IsNaN() || pimpl->velocity_fps <= FpsT(0) ||
      !std::isfinite(pimpl->velocity_fps.Value())) {
    pout->error = kLobErrorInitialVelocityRequired;
    return;
  }

  if (!pimpl->diameter_in.IsNaN() &&
      (!std::isfinite(pimpl->diameter_in.Value()) ||
       pimpl->diameter_in <= InchT(0))) {
    pout->error = kLobErrorDiameterOOR;
    return;
  }

  if (!pimpl->length_in.IsNaN() &&
      (!std::isfinite(pimpl->length_in.Value()) ||
       pimpl->length_in <= InchT(0))) {
    pout->error = kLobErrorLengthOOR;
    return;
  }

  if (!pimpl->mass_lbs.IsNaN() &&
      (!std::isfinite(pimpl->mass_lbs.Value()) ||
       pimpl->mass_lbs < LbsT(0))) {
    pout->error = kLobErrorMassOOR;
    return;
  }

  if (pimpl->diameter_in.IsNaN() || pimpl->length_in.IsNaN() ||
      pimpl->mass_lbs.IsNaN() || pimpl->twist_inches_per_turn.IsNaN() ||
      !std::isfinite(pimpl->twist_inches_per_turn.Value()) ||
      AreEqual(pimpl->twist_inches_per_turn, InchPerTwistT(0))) {
    return;
  }

  const double kFtp = CalculateMillerTwistRuleCorrectionFactor(
      pimpl->air_density_lbs_per_cu_ft);
  const double kSg =
      kFtp * CalculateMillerTwistRuleStabilityFactor(
                 pimpl->diameter_in, GrainT(pimpl->mass_lbs), pimpl->length_in,
                 pimpl->twist_inches_per_turn, pimpl->velocity_fps);
  if (std::isfinite(kSg) && HasGyroscopicSpin(kSg)) {
    pout->stability_factor = kSg;
  }
}

void BuildCoriolis(Impl* pimpl, LobContext* pout) {
  assert(pimpl != nullptr && pout != nullptr);

  if (!std::isnan(pimpl->azimuth_rad)) {
    const DegreesT kAzimuthLimit(kDegreesPerTurn);
    if (!std::isfinite(pimpl->azimuth_rad.Value()) ||
        pimpl->azimuth_rad > kAzimuthLimit ||
        pimpl->azimuth_rad < kAzimuthLimit * -1) {
      pout->error = kLobErrorAzimuthOOR;
      return;
    }
  }

  if (!std::isnan(pimpl->latitude_rad)) {
    const DegreesT kLatitudeLimit(90);
    if (!std::isfinite(pimpl->latitude_rad.Value()) ||
        pimpl->latitude_rad > kLatitudeLimit ||
        pimpl->latitude_rad < kLatitudeLimit * -1) {
      pout->error = kLobErrorLatitudeOOR;
      return;
    }
  }

  if (!std::isnan(pimpl->azimuth_rad) && !std::isnan(pimpl->latitude_rad)) {
    constexpr double k2Omega = 2.0 * kAngularVelocityOfEarthRadPerSec;
    const double kSinL = std::sin(pimpl->latitude_rad.Value());
    const double kCosL = std::cos(pimpl->latitude_rad.Value());
    const double kSinA = std::sin(pimpl->azimuth_rad.Value());
    const double kCosA = std::cos(pimpl->azimuth_rad.Value());
    const double kCosL2Omega = k2Omega * kCosL;

    pout->coriolis.cos_l_sin_a = kCosL2Omega * kSinA;
    pout->coriolis.sin_l = k2Omega * kSinL;
    pout->coriolis.cos_l_cos_a = kCosL2Omega * kCosA;
  } else {
    pout->coriolis.cos_l_sin_a = 0;
    pout->coriolis.sin_l = 0;
    pout->coriolis.cos_l_cos_a = 0;
  }
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
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

  if (pimpl->ogive_rtr <= 0 || pimpl->ogive_rtr > 1.0) {
    pout->error = kLobErrorOgiveRtROOR;
    return;
  }

  // Fast path: skip heavy Caliber math if geometry incomplete (common)
  if (pimpl->diameter_in.IsNaN() || pimpl->length_in.IsNaN() ||
      pimpl->mass_lbs.IsNaN() || pimpl->twist_inches_per_turn.IsNaN() ||
      AreEqual(pimpl->twist_inches_per_turn, InchPerTwistT(0)) ||
      !HasGyroscopicSpin(pout->stability_factor) ||
      std::isnan(pout->speed_of_sound) || pimpl->velocity_fps.IsNaN() ||
      (pimpl->drag_table_mode == DragTableMode::kStandard &&
       (pimpl->ballistic_coefficient_psi.IsNaN() ||
        !std::isfinite(pimpl->ballistic_coefficient_psi.Value())))) {
    return;
  }

  const InchT kD(pimpl->diameter_in);
  const double kInvD = 1.0 / kD.Value();
  const CaliberT kDM(pimpl->meplat_diameter_in.Value() * kInvD);
  const CaliberT kDB(pimpl->base_diameter_in.Value() * kInvD);
  const CaliberT kL(pimpl->length_in.Value() * kInvD);
  const CaliberT kLN(pimpl->nose_length_in.Value() * kInvD);
  const CaliberT kLBT(pimpl->tail_length_in.Value() * kInvD);
  const auto kRTR(pimpl->ogive_rtr);
  const FpsT kVelocity(pimpl->velocity_fps);
  const FpsT kSos(pout->speed_of_sound);
  const GrainT kMass = pimpl->mass_lbs;
  const InchPerTwistT kTwist(pimpl->twist_inches_per_turn);
  const double kSg(pout->stability_factor);
  const PmsiT kBc(pimpl->drag_table_mode == DragTableMode::kStandard
                      ? pimpl->ballistic_coefficient_psi
                      : PmsiT(1));
  const FpsT kZWind(pout->wind_nodes[0].z_fps);
  const LbsPerCuFtT kAirDensity = pimpl->air_density_lbs_per_cu_ft;

  if (kD.IsNaN() || kDM.IsNaN() || kDB.IsNaN() || kL.IsNaN() || kLN.IsNaN() ||
      kLBT.IsNaN() || std::isnan(kRTR) || !(kVelocity > FpsT(0)) ||
      kSos.IsNaN() || kMass.IsNaN() || kTwist.IsNaN() || std::isnan(kSg) ||
      kBc.IsNaN() || kZWind.IsNaN() || kAirDensity.IsNaN()) {
    return;
  }

  const CaliberT kRT = boatright::CalculateRadiusOfTangentOgive(kLN, kDM);
  const CaliberT kLFN = boatright::CalculateFullNoseLength(kLN, kDM, kRT, kRTR);
  const PsiT kQ = boatright::CalculateDynamicPressure(kAirDensity, kVelocity);
  const SqInT kS = CalculateProjectileReferenceArea(kD);
  const auto kAR = boatright::CalculateAspectRatio(kL, kLFN, kLBT, kDB);
  const auto kM = MachT(kVelocity, kSos.Inverse());
  spline::CurveView drag_curve(spline::kKnots.data(), &pout->drags[0]);
  const auto kCdRef = drag_curve.Eval(kM);
  const auto kCL = boatright::CalculateCoefficientOfLift(kLFN, kM);
  const auto kCDa = boatright::CalculateYawDragCoefficient(kM, kCL, kAR);
  const auto kRho = boatright::CalculateAverageDensity(
      kD, kL, kLN, kLFN, kRT / kRTR, kDB, kLBT, kMass);
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
      pimpl->drag_table_mode == DragTableMode::kCustomTable ||
              pimpl->drag_table_mode == DragTableMode::kNativeCoefs
          ? static_cast<double>(kCdRef)
          : boatright::CalculateZeroYawDragCoefficientOfDrag(kCdRef, kMass, kD,
                                                             PmsiT(1));
  const double kCD =
      (kGamma < 0.0 || kGamma > 0.0)
          ? kCD0 + boatright::CalculateYawDragAdjustment(kGamma, kR, kCDa)
          : kCD0;
  const double kPitch = (kGamma < 0.0 || kGamma > 0.0)
                            ? boatright::CalculateVerticalPitch(kGamma, kR, kN)
                            : 0.0;
  const double kJv = (kGamma < 0.0 || kGamma > 0.0)
                         ? boatright::CalculateVerticalImpulse(
                               kTwist, kN, kTn, kQ, kS, kCL, kCD, kPitch)
                         : 0.0;
  const auto kMOM = boatright::CalculateMagnitudeOfMomentum(kMass, kVelocity);
  const MoaT kJump = (kGamma < 0.0 || kGamma > 0.0)
                         ? MoaT(RadiansT(-1 * kJv / kMOM))
                         : MoaT(0);
  pout->aerodynamic_jump = kJump.Value();

  const FpsT kTransonicBarrier(MachT(1.2), kSos);
  SecT tof(0.0);

  if (kVelocity > kTransonicBarrier) {
    const double kIntegral = lob::IntegrateGaussLegendre<8>(
        kTransonicBarrier.Value(), kVelocity.Value(), [&](double v) {
          const auto kMach = static_cast<float>(v / kSos.Value());
          const auto kCd =
              static_cast<double>(drag_curve.Eval(kMach)) * pout->drag_coeff;
          return 1.0 / (v * v * kCd);
        });

    tof = SecT(kIntegral);
  }

  const auto kV = boatright::CalculateKV(kVelocity, kTransonicBarrier);
  const auto kOmega = boatright::CalculateKOmega(kD, tof);
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
      boatright::CalculateCoefficientOfLiftAtT(kClOf0, kVelocity, tof);
  pout->spindrift_factor =
      boatright::CalculateSpinDriftScaleFactor(kQTS, kBetaROfT, kClOfT, kMass);
}

void BuildLitzAerodynamicJump(Impl* pimpl, LobContext* pout) {
  assert(pimpl != nullptr && pout != nullptr);

  if (!std::isnan(pout->aerodynamic_jump)) {
    return;
  }

  const double kMuzzleCrosswindFps = pout->wind_nodes[0].z_fps;

  if (AreEqual(kMuzzleCrosswindFps, 0.0)) {
    pout->aerodynamic_jump = MoaT(0).Value();
    return;
  }

  if (HasGyroscopicSpin(pout->stability_factor) &&
      !std::isnan(pimpl->diameter_in) && !std::isnan(pimpl->length_in) &&
      !pimpl->twist_inches_per_turn.IsNaN() &&
      !AreEqual(pimpl->twist_inches_per_turn, InchPerTwistT(0))) {
    pout->aerodynamic_jump =
        litz::CalculateAerodynamicJump(pout->stability_factor,
                                       pimpl->diameter_in, pimpl->length_in,
                                       MphT(FpsT(kMuzzleCrosswindFps)))
            .Value();
    return;
  }

  pout->aerodynamic_jump = MoaT(0).Value();
}

void BuildZeroAngle(Impl* pimpl, LobContext* pout) {
  assert(pimpl != nullptr && pout != nullptr);

  if (!std::isnan(pimpl->zero_angle_moa)) {
    if (!std::isfinite(pimpl->zero_angle_moa.Value()) ||
        pimpl->zero_angle_moa > constant::kMaxAngle ||
        pimpl->zero_angle_moa < constant::kMinAngle) {
      pout->error = kLobErrorZeroAngleOOR;
    }
    return;
  }

  if (pimpl->zero_distance_ft.IsNaN()) {
    pout->error = kLobErrorLaunchAngleRequired;
    return;
  }

  if (!std::isfinite(pimpl->zero_distance_ft.Value()) ||
      pimpl->zero_distance_ft <= FeetT(0)) {
    pout->error = kLobErrorZeroDistanceOOR;
    return;
  }

  assert(!std::isnan(pout->aerodynamic_jump));

  if (std::isnan(pimpl->zero_impact_height)) {
    pimpl->zero_impact_height = FeetT(0.0);
  }

  assert(pimpl->velocity_fps > FpsT(0));
  const double kDist = pimpl->zero_distance_ft.Value();
  const double kV = pimpl->velocity_fps.Value();
  const double kG = kStandardGravityFtPerSecSq;
  const double kKdrag = EstimateDragPerDistance(*pout, kV);
  const double kAnchorY = pout->optic_height;
  const double kClimb = kAnchorY + pimpl->zero_impact_height.Value();
  const double kSightAngle = std::atan(kClimb / kDist);
  const double kDrop = (0.5 * kG * kDist / (kV * kV)) *
                       (1.0 + ((2.0 / 3.0) * kKdrag * kDist));
  const double kJump =
      std::isnan(pout->aerodynamic_jump)
          ? 0.0
          : RadiansT(MoaT(pout->aerodynamic_jump)).Value();
  const double kSeedAngle = kSightAngle + kDrop - kJump;

  const auto kClampedSeed =
      std::isfinite(kSeedAngle)
          ? std::max(constant::kMinAngle,
                     std::min(constant::kMaxAngle, RadiansT(kSeedAngle)))
          : RadiansT(0.0);
  const MoaT kAngle = SolveAngle(*pout, pimpl->zero_distance_ft,
                                 pimpl->zero_impact_height, kClampedSeed);
  if (std::isnan(kAngle)) {
    pout->error = kLobErrorZeroUnreachable;
    return;
  }
  pout->zero_angle = kAngle.Value();
}

void BuildZeroAzimuth(Impl* pimpl, LobContext* pout) {
  assert(pimpl != nullptr && pout != nullptr);

  if (!std::isnan(pimpl->zero_azimuth_moa)) {
    if (!std::isfinite(pimpl->zero_azimuth_moa.Value()) ||
        pimpl->zero_azimuth_moa > constant::kMaxAngle ||
        pimpl->zero_azimuth_moa < constant::kMinAngle) {
      pout->error = kLobErrorZeroAngleOOR;
      return;
    }
    pout->zero_azimuth = pimpl->zero_azimuth_moa.Value();
    return;
  }

  if (!std::isnan(pimpl->zero_angle_moa)) {
    // Portable zero path
    if (pimpl->zero_distance_ft.IsNaN() ||
        pimpl->zero_distance_ft <= FeetT(0)) {
      pout->zero_azimuth = 0.0;
      return;
    }
    // Explicit angle + zero distance: geometric solve without air
    const RadiansT kGeoAzimuth(
        std::atan(pout->optic_offset / pimpl->zero_distance_ft.Value()));
    pout->zero_azimuth = MoaT(kGeoAzimuth).Value();
    return;
  }

  if (pimpl->zero_distance_ft.IsNaN() || pimpl->zero_distance_ft <= FeetT(0)) {
    return;
  }

  // Fast path: when no lateral forces (crosswind, Coriolis) act on the bullet,
  // the horizontal trajectory is a straight line. For centered optics
  // (optic_offset = 0), zero_azimuth is identically 0.0. For offset optics,
  // the analytic closed-form geometric angle is exact.
  const bool kHasOffset = std::fabs(pout->optic_offset) > 0.0;
  if (!HasLateralPerturbation(*pout)) {
    if (!kHasOffset) {
      pout->zero_azimuth = 0.0;
    } else {
      const RadiansT kGeoAzimuth(
          std::atan(pout->optic_offset / pimpl->zero_distance_ft.Value()));
      pout->zero_azimuth = MoaT(kGeoAzimuth).Value();
    }
    return;
  }

  assert(pimpl->velocity_fps > FpsT(0));
  const auto kGeoSeed =
      RadiansT(std::atan(pout->optic_offset / pimpl->zero_distance_ft.Value()));
  const auto kClampedSeed =
      std::max(constant::kMinAngle, std::min(constant::kMaxAngle, kGeoSeed));
  const MoaT kAzimuth =
      SolveAngleAzimuth(*pout, pimpl->zero_distance_ft, kClampedSeed);
  if (std::isnan(kAzimuth)) {
    pout->error = kLobErrorZeroUnreachable;
    return;
  }
  pout->zero_azimuth = kAzimuth.Value();
}

void BuildOptions(Impl* pimpl, LobContext* pout) {
  assert(pimpl != nullptr && pout != nullptr);

  if (!std::isnan(pimpl->max_time_sec) &&
      (!std::isfinite(pimpl->max_time_sec.Value()) || pout->max_time < 0.0)) {
    pout->error = kLobErrorMaximumTimeOOR;
    return;
  }

  const FpsT kMinSpeed =
      pimpl->minimum_energy_ft_lbs.IsNaN()
          ? FpsT(0)
          : CalculateVelocityFromKineticEnergy(pimpl->minimum_energy_ft_lbs,
                                               SlugT(pimpl->mass_lbs));
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

LobBuilder* LobBuilderSplineCoefficients(LobBuilder* pbuilder,
                                         const float* pcoefs) {
  if (pbuilder == nullptr || pcoefs == nullptr) {
    return pbuilder;
  }
  auto* pimpl = Pimpl(pbuilder);
  pimpl->table_ys = pcoefs;
  pimpl->table_xs = nullptr;
  pimpl->table_count = 0;
  pimpl->drag_table_mode = DragTableMode::kNativeCoefs;
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

LobBuilder* LobBuilderOpticLateralOffsetInches(LobBuilder* pbuilder,
                                               double value) {
  if (pbuilder == nullptr) {
    return nullptr;
  }
  auto* pimpl = Pimpl(pbuilder);
  pimpl->optic_offset_ft =
      std::isnan(value) ? FeetT(NaN()) : FeetT(InchT(-value));
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

LobBuilder* LobBuilderZeroAzimuthMOA(LobBuilder* pbuilder, double value) {
  if (pbuilder == nullptr) {
    return nullptr;
  }
  auto* pimpl = Pimpl(pbuilder);
  pimpl->zero_azimuth_moa = MoaT(value);
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
  pimpl->wind_table_mode = WindTableMode::kUniform;
  return pbuilder;
}

LobBuilder* LobBuilderWindHeadingDeg(LobBuilder* pbuilder, double value) {
  if (pbuilder == nullptr) {
    return nullptr;
  }
  auto* pimpl = Pimpl(pbuilder);
  pimpl->wind_heading_rad = HeadingDegToRad(value);
  pimpl->wind_table_mode = WindTableMode::kUniform;
  return pbuilder;
}

LobBuilder* LobBuilderWindSpeedFps(LobBuilder* pbuilder, double value) {
  if (pbuilder == nullptr) {
    return nullptr;
  }
  auto* pimpl = Pimpl(pbuilder);
  pimpl->wind_speed_fps = FpsT(value);
  pimpl->wind_table_mode = WindTableMode::kUniform;
  return pbuilder;
}

LobBuilder* LobBuilderWindSpeedMph(LobBuilder* pbuilder, double value) {
  if (pbuilder == nullptr) {
    return nullptr;
  }
  auto* pimpl = Pimpl(pbuilder);
  pimpl->wind_speed_fps = MphT(value);
  pimpl->wind_table_mode = WindTableMode::kUniform;
  return pbuilder;
}

LobBuilder* LobBuilderWindProfile(LobBuilder* pbuilder,
                                  const LobWindPoint* ppoints, size_t count) {
  if (pbuilder == nullptr) {
    return nullptr;
  }
  auto* pimpl = Pimpl(pbuilder);
  pimpl->wind_profile_points = ppoints;
  pimpl->wind_profile_count = count;
  pimpl->wind_table_mode = WindTableMode::kProfile;
  return pbuilder;
}

LobBuilder* LobBuilderWindShearExponent(LobBuilder* pbuilder, double value) {
  if (pbuilder == nullptr) {
    return nullptr;
  }
  auto* pimpl = Pimpl(pbuilder);
  pimpl->wind_shear_exponent = value;
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

  LobContext local_ctx{};
  local_ctx.error = kLobErrorNotFormed;
  local_ctx.stability_factor = NaN();
  local_ctx.aerodynamic_jump = NaN();
  local_ctx.spindrift_factor = NaN();
  local_ctx.optic_height = NaN();
  local_ctx.optic_offset = NaN();
  local_ctx.zero_angle = NaN();
  local_ctx.zero_azimuth = 0.0;
  local_ctx.k_lapse = 0.0;
  local_ctx.speed_of_sound = NaN();
  local_ctx.drag_coeff = NaN();

  local_ctx.mass = pimpl->mass_lbs.IsNaN() ? NaN() : pimpl->mass_lbs.Value();
  local_ctx.velocity =
      pimpl->velocity_fps.IsNaN() ? 0 : pimpl->velocity_fps.U16();
  local_ctx.minimum_speed =
      pimpl->minimum_speed_fps.IsNaN() ? 0 : pimpl->minimum_speed_fps.U16();
  local_ctx.step_size = pimpl->step_size_in;
  local_ctx.max_time =
      pimpl->max_time_sec.IsNaN() ? NaN() : pimpl->max_time_sec.Value();
  local_ctx.zero_angle =
      pimpl->zero_angle_moa.IsNaN() ? NaN() : pimpl->zero_angle_moa.Value();
  local_ctx.zero_azimuth =
      pimpl->zero_azimuth_moa.IsNaN() ? 0.0 : pimpl->zero_azimuth_moa.Value();

  BuildEnvironment(pimpl, &local_ctx);
  if (local_ctx.error == kLobErrorNotFormed) {
    BuildSpline(pimpl, &local_ctx);
  }
  if (local_ctx.error == kLobErrorNotFormed) {
    BuildCoefficients(pimpl, &local_ctx);
  }
  if (local_ctx.error == kLobErrorNotFormed) {
    BuildWind(pimpl, &local_ctx);
  }
  if (local_ctx.error == kLobErrorNotFormed) {
    BuildOpticHeight(pimpl, &local_ctx);
    BuildStability(pimpl, &local_ctx);
  }
  if (local_ctx.error == kLobErrorNotFormed) {
    BuildCoriolis(pimpl, &local_ctx);
  }
  if (local_ctx.error == kLobErrorNotFormed) {
    BuildBoatright(pimpl, &local_ctx);
  }
  if (local_ctx.error == kLobErrorNotFormed) {
    BuildLitzAerodynamicJump(pimpl, &local_ctx);
    BuildOptions(pimpl, &local_ctx);
  }
  if (local_ctx.error == kLobErrorNotFormed) {
    BuildZeroAngle(pimpl, &local_ctx);
  }
  if (local_ctx.error == kLobErrorNotFormed) {
    BuildZeroAzimuth(pimpl, &local_ctx);
  }

  if (local_ctx.error == kLobErrorNotFormed) {
    local_ctx.error = kLobErrorNone;
    *presult = local_ctx;
    return;
  }

  // Atomic failure commit: preserve direct user configuration fields but clear all derived physics
  *presult = LobContext{};
  presult->error = local_ctx.error;
  presult->stability_factor = NaN();
  presult->aerodynamic_jump = NaN();
  presult->spindrift_factor = NaN();
  presult->optic_height = NaN();
  presult->optic_offset = NaN();
  presult->zero_angle = local_ctx.zero_angle;
  presult->zero_azimuth = local_ctx.zero_azimuth;
  presult->mass = local_ctx.mass;
  presult->velocity = local_ctx.velocity;
  presult->minimum_speed = local_ctx.minimum_speed;
  presult->step_size = local_ctx.step_size;
  presult->max_time = local_ctx.max_time;
  presult->k_lapse = 0.0;
  presult->speed_of_sound = NaN();
  presult->drag_coeff = NaN();
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
