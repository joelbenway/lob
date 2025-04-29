// This file is a part of lob, an exterior ballistics calculation library
// Copyright (c) 2025  Joel Benway
// Please see end of file for extended copyright information

#include "lob/lob.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <utility>

#include "calc.hpp"
#include "cartesian.hpp"
#include "constants.hpp"
#include "eng_units.hpp"
#include "helpers.hpp"
#include "ode.hpp"
#include "tables.hpp"
#include "version.hpp"

namespace lob {

const char* Version() { return kProjectVersion; }

class Impl {
  friend class Builder;

 public:
  LbsPerCuFtT air_density_lbs_per_cu_ft{NaN<double>()};
  FeetT altitude_ft{NaN<double>()};
  FeetT altitude_of_barometer_ft{NaN<double>()};
  FeetT altitude_of_thermometer_ft{NaN<double>()};
  AtmosphereReferenceT atmosphere_reference{
      AtmosphereReferenceT::kArmyStandardMetro};
  RadiansT azimuth_rad{NaN<double>()};
  PmsiT ballistic_coefficient_psi{NaN<double>()};
  InchT base_diameter_in{NaN<double>()};
  InHgT air_pressure_in_hg{NaN<double>()};
  InchT diameter_in{NaN<double>()};
  RadiansT latitude_rad{NaN<double>()};
  InchT length_in{NaN<double>()};
  InchT meplat_diameter_in{NaN<double>()};
  InchT nose_length_in{NaN<double>()};
  double ogive_rtr{NaN<double>()};
  const std::array<uint16_t, kTableSize>* pdrag_lut{&kG1Drags};
  RadiansT range_angle_rad{NaN<double>()};
  double relative_humidity_percent{NaN<double>()};
  InchT tail_length_in{NaN<double>()};
  DegFT temperature_deg_f{NaN<double>()};
  InchPerTwistT twist_inches_per_turn{NaN<double>()};
  RadiansT wind_heading_rad{NaN<double>()};
  FpsT wind_speed_fps{NaN<double>()};
  FeetT zero_distance_ft{NaN<double>()};
  FeetT zero_impact_height{NaN<double>()};

  Input build{};
};

Builder::Builder() : pimpl_(new(buffer_.data()) Impl()) {
  static_assert(kBufferSize >= sizeof(Impl), "Buffer is too small.");
}

Builder::~Builder() { pimpl_->~Impl(); }

Builder::Builder(const Builder& other)
    : pimpl_(new(buffer_.data()) Impl(*other.pimpl_)) {}

Builder::Builder(Builder&& other) noexcept {
  if (this != &other) {
    if (pimpl_ != nullptr) {
      pimpl_->~Impl();
    }
    pimpl_ = new (buffer_.data()) Impl();
    std::swap(pimpl_, other.pimpl_);
  }
}

Builder& Builder::operator=(const Builder& rhs) {
  if (this != &rhs) {
    if (pimpl_ != nullptr) {
      pimpl_->~Impl();
    }
    pimpl_ = new (buffer_.data()) Impl(*rhs.pimpl_);
  }
  return *this;
}

Builder& Builder::operator=(Builder&& rhs) noexcept {
  if (this != &rhs) {
    if (pimpl_ != nullptr) {
      pimpl_->~Impl();
    }
    pimpl_ = new (buffer_.data()) Impl();
    std::swap(pimpl_, rhs.pimpl_);
  }
  return *this;
}

Builder& Builder::AltitudeOfFiringSiteFt(float value) {
  pimpl_->altitude_ft = FeetT(value);
  return *this;
}

Builder& Builder::AltitudeOfBarometerFt(float value) {
  pimpl_->altitude_of_barometer_ft = FeetT(value);
  return *this;
}

Builder& Builder::AltitudeOfThermometerFt(float value) {
  pimpl_->altitude_of_thermometer_ft = FeetT(value);
  return *this;
}

Builder& Builder::AzimuthDeg(float value) {
  pimpl_->azimuth_rad = DegreesT(value);
  return *this;
}

Builder& Builder::BallisticCoefficientPsi(float value) {
  pimpl_->ballistic_coefficient_psi = PmsiT(value);
  return *this;
}

Builder& Builder::AirPressureInHg(float value) {
  pimpl_->air_pressure_in_hg = InHgT(value);
  return *this;
}

Builder& Builder::BaseDiameterInch(float value) {
  pimpl_->base_diameter_in = InchT(value);
  return *this;
}

Builder& Builder::BCAtmosphere(AtmosphereReferenceT type) {
  pimpl_->atmosphere_reference = type;
  return *this;
}

Builder& Builder::BCDragFunction(DragFunctionT type) {
  switch (type) {
    case DragFunctionT::kG1: {
      pimpl_->pdrag_lut = &kG1Drags;
      break;
    }
    case DragFunctionT::kG2: {
      pimpl_->pdrag_lut = &kG2Drags;
      break;
    }
    case DragFunctionT::kG5: {
      pimpl_->pdrag_lut = &kG5Drags;
      break;
    }
    case DragFunctionT::kG6: {
      pimpl_->pdrag_lut = &kG6Drags;
      break;
    }
    case DragFunctionT::kG7: {
      pimpl_->pdrag_lut = &kG7Drags;
      break;
    }
    case DragFunctionT::kG8: {
      pimpl_->pdrag_lut = &kG8Drags;
      break;
    }
    default: {
      break;
    }
  }
  return *this;
}

Builder& Builder::DiameterInch(float value) {
  pimpl_->diameter_in = InchT(value);
  return *this;
}

Builder& Builder::InitialVelocityFps(uint16_t value) {
  pimpl_->build.velocity = FpsT(value).U16();
  return *this;
}

Builder& Builder::LatitudeDeg(float value) {
  pimpl_->latitude_rad = DegreesT(value);
  return *this;
}

Builder& Builder::LengthInch(float value) {
  pimpl_->length_in = InchT(value);
  return *this;
}

Builder& Builder::MachVsDragTable(const float* pmachs, const float* pdrags,
                                  size_t size) {
  for (size_t i = 0; i < kTableSize; i++) {
    const auto kMach = static_cast<double>(kMachs.at(i)) / kTableScale;
    const auto kDrag = static_cast<uint16_t>(
        std::round(LobLerp(pmachs, pdrags, size, kMach) * kTableScale));
    pimpl_->build.drags.at(i) = kDrag;
  }
  pimpl_->pdrag_lut = &pimpl_->build.drags;
  pimpl_->ballistic_coefficient_psi = PmsiT(1);
  return *this;
}

Builder& Builder::MassGrains(float value) {
  pimpl_->build.mass = LbsT(GrainT(value)).Float();
  return *this;
}

Builder& Builder::MeplatDiameterInch(float value) {
  pimpl_->meplat_diameter_in = InchT(value);
  return *this;
}

Builder& Builder::NoseLengthInch(float value) {
  pimpl_->nose_length_in = InchT(value);
  return *this;
}

Builder& Builder::OgiveRtR(float value) {
  pimpl_->ogive_rtr = static_cast<double>(value);
  return *this;
}

Builder& Builder::OpticHeightInches(float value) {
  pimpl_->build.optic_height = FeetT(InchT(value)).Float();
  return *this;
}

Builder& Builder::RelativeHumidityPercent(float value) {
  pimpl_->relative_humidity_percent = value;
  return *this;
}

Builder& Builder::RangeAngleDeg(float value) {
  pimpl_->range_angle_rad = RadiansT(DegreesT(value));
  return *this;
}

Builder& Builder::TailLengthInch(float value) {
  pimpl_->tail_length_in = InchT(value);
  return *this;
}

Builder& Builder::TemperatureDegF(float value) {
  pimpl_->temperature_deg_f = DegFT(value);
  return *this;
}

Builder& Builder::TwistInchesPerTurn(float value) {
  pimpl_->twist_inches_per_turn = InchPerTwistT(value);
  return *this;
}

Builder& Builder::WindHeading(ClockAngleT value) {
  constexpr DegreesT kDegreesPerClockNumber = DegreesT(kDegreesPerTurn) / 12;
  pimpl_->wind_heading_rad =
      DegreesT(kDegreesPerClockNumber * static_cast<uint8_t>(value));
  return *this;
}

Builder& Builder::WindHeadingDeg(float value) {
  constexpr DegreesT kFullTurn(kDegreesPerTurn);
  constexpr DegreesT kQuarterTurn(kFullTurn / 4);

  DegreesT angle(value);

  while (angle > DegreesT(0)) {
    angle -= kFullTurn;
  }

  while (angle < kFullTurn * -1.0) {
    angle += kFullTurn;
  }

  angle = angle * -1 + kQuarterTurn;

  while (angle >= kFullTurn) {
    angle -= kFullTurn;
  }

  assert(angle >= DegreesT(0));
  assert(angle < kFullTurn);

  pimpl_->wind_heading_rad = angle;

  return *this;
}

Builder& Builder::WindSpeedFps(float value) {
  pimpl_->wind_speed_fps = FpsT(value);
  return *this;
}

Builder& Builder::WindSpeedMph(float value) {
  pimpl_->wind_speed_fps = MphT(value);
  return *this;
}

Builder& Builder::ZeroAngleMOA(float value) {
  pimpl_->build.zero_angle = MoaT(value).Float();
  return *this;
}

Builder& Builder::ZeroDistanceYds(float value) {
  pimpl_->zero_distance_ft = YardT(value);
  return *this;
}

Builder& Builder::ZeroImpactHeightInches(float value) {
  pimpl_->zero_impact_height = InchT(value);
  return *this;
}

namespace {
void SolveStep(SpvT* ps, SecT* pt, const Input& input, uint16_t step_size = 0) {
  auto ds_dt = [input](double t, const SpvT& s) -> SpvT {
    static_cast<void>(t);  // t is unused
    const CartesianT<FeetT> kPosition(FeetT(s.V().X().Value()),
                                      FeetT(s.V().Y().Value()),
                                      FeetT(s.V().Z().Value()));
    const CartesianT<FpsT> kWind(FpsT(input.wind.x), FpsT(0.0),
                                 FpsT(input.wind.z));
    const MachT kMach(s.V().Magnitude(), FpsT(input.speed_of_sound).Inverse());
    const double kCd = LobLerp(kMachs, input.drags, kMach) *
                       static_cast<double>(input.table_coefficient);
    const FpsT kScalarVelocity = (s.V() - kWind).Magnitude();
    CartesianT<FpsT> velocity =
        (s.V() - kWind) * FpsT(-1 * kCd) * kScalarVelocity;
    velocity.X(velocity.X() - s.V().Y() * input.corilolis.cos_l_sin_a -
               s.V().Z() * input.corilolis.sin_l);
    velocity.Y(velocity.Y() + s.V().X() * input.corilolis.cos_l_sin_a +
               s.V().Z() * input.corilolis.cos_l_cos_a);
    velocity.Z(velocity.Z() + s.V().X() * input.corilolis.sin_l -
               s.V().Y() * input.corilolis.cos_l_cos_a);
    velocity.X(velocity.X() + input.gravity.x);
    velocity.Y(velocity.Y() + input.gravity.y);
    return SpvT{kPosition, velocity};
  };  // ds_dt

  SecT dt(0);
  if (step_size == 0) {
    dt = SecT(1 / ps->V().Magnitude().Value() / 2);
  } else {
    dt = SecT(UsecT(step_size));
  }

  *ps = HeunStep(0.0, *ps, dt.Value(), ds_dt);
  *pt += dt;
}

bool ValidateBuild(const Impl& impl) {
  const bool kBCisOk = !std::isnan(impl.ballistic_coefficient_psi);
  const bool kVelocityIsOk = impl.build.velocity > 0;
  const bool kZeroIsOk =
      !std::isnan(impl.zero_distance_ft) || !std::isnan(impl.build.zero_angle);

  return (kBCisOk && kVelocityIsOk && kZeroIsOk);
}

void BuildEnvironment(Impl* pimpl) {
  FeetT altitude_of_firing_site = FeetT(0);
  FeetT altitude_of_barometer = FeetT(0);
  FeetT altitude_of_thermometer = FeetT(0);
  DegFT temperature_at_firing_site = DegFT(kIsaSeaLevelDegF);
  DegFT temperature_at_barometer = DegFT(kIsaSeaLevelDegF);
  InHgT pressure_at_firing_site = InHgT(kIsaSeaLevelPressureInHg);

  if (std::isnan(pimpl->range_angle_rad)) {
    pimpl->range_angle_rad = RadiansT(DegreesT(0));
  }

  pimpl->build.gravity.x =
      static_cast<float>(kStandardGravityFtPerSecSq * -1 *
                         std::sin(pimpl->range_angle_rad.Value()));
  pimpl->build.gravity.y =
      static_cast<float>(kStandardGravityFtPerSecSq * -1 *
                         std::cos(pimpl->range_angle_rad.Value()));

  if (!std::isnan(pimpl->altitude_ft)) {
    altitude_of_firing_site = pimpl->altitude_ft;
    altitude_of_barometer = std::isnan(pimpl->altitude_of_barometer_ft)
                                ? pimpl->altitude_ft
                                : pimpl->altitude_of_barometer_ft;
    altitude_of_thermometer = std::isnan(pimpl->altitude_of_thermometer_ft)
                                  ? pimpl->altitude_ft
                                  : pimpl->altitude_of_thermometer_ft;

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
    pressure_at_firing_site =
        BarometricFormula(altitude_of_firing_site - altitude_of_barometer,
                          pimpl->air_pressure_in_hg, temperature_at_barometer);
  }

  if (std::isnan(pimpl->relative_humidity_percent)) {
    pimpl->relative_humidity_percent = kIsaSeaLevelHumidityPercent;
  }

  const auto kWaterVaporSaturationPressureInHg =
      CalculateWaterVaporSaturationPressure(temperature_at_firing_site);

  pimpl->air_density_lbs_per_cu_ft = LbsPerCuFtT(
      kIsaSeaLevelAirDensityLbsPerCuFt *
      CalcualteAirDensityRatio(pressure_at_firing_site,
                               temperature_at_firing_site) *
      CalculateAirDensityRatioHumidityCorrection(
          pimpl->relative_humidity_percent, kWaterVaporSaturationPressureInHg));

  pimpl->build.speed_of_sound =
      FpsT(CalculateSpeedOfSoundInAir(temperature_at_firing_site) *
           CalculateSpeedOfSoundHumidityCorrection(
               pimpl->relative_humidity_percent,
               kWaterVaporSaturationPressureInHg))
          .Float();
}

void BuildTable(Impl* pimpl) {
  assert(!std::isnan(pimpl->ballistic_coefficient_psi));
  assert(!std::isnan(pimpl->air_density_lbs_per_cu_ft));

  if (pimpl->atmosphere_reference == AtmosphereReferenceT::kArmyStandardMetro) {
    pimpl->ballistic_coefficient_psi *= kArmyToIcaoBcConversionFactor;
    pimpl->atmosphere_reference = AtmosphereReferenceT::kIcao;
  }

  static_assert(Input::kTableSize == kTableSize, "Table size not identical.");
  if (pimpl->pdrag_lut != &pimpl->build.drags) {
    std::copy(pimpl->pdrag_lut->begin(), pimpl->pdrag_lut->end(),
              pimpl->build.drags.begin());
  }
  // scale for air density and bc
  const double kCdCoefficient = CalculateCdCoefficient(
      pimpl->air_density_lbs_per_cu_ft, pimpl->ballistic_coefficient_psi);
  pimpl->build.table_coefficient = static_cast<float>(kCdCoefficient);
}

void BuildWind(Impl* pimpl) {
  if (std::isnan(pimpl->wind_heading_rad)) {
    pimpl->wind_heading_rad = DegreesT(0);
  }

  if (std::isnan(pimpl->wind_speed_fps)) {
    pimpl->wind_speed_fps = FpsT(0);
  }

  pimpl->build.wind.x =
      FpsT(pimpl->wind_speed_fps * std::sin(pimpl->wind_heading_rad.Value()))
          .Float();
  pimpl->build.wind.z =
      FpsT(pimpl->wind_speed_fps * std::cos(pimpl->wind_heading_rad.Value()))
          .Float();
}

void BuildTwistEffects(Impl* pimpl) {
  assert(pimpl->build.velocity > 0);
  assert(!std::isnan(pimpl->ballistic_coefficient_psi));
  assert(!std::isnan(pimpl->build.speed_of_sound));
  assert(!std::isnan(pimpl->build.wind.z));

  if (!std::isnan(pimpl->diameter_in) && !std::isnan(pimpl->length_in) &&
      !std::isnan(pimpl->twist_inches_per_turn) &&
      !std::isnan(pimpl->build.mass)) {
    const double kFtp = CalculateMillerTwistRuleCorrectionFactor(
        pimpl->air_density_lbs_per_cu_ft);
    pimpl->build.stability_factor = static_cast<float>(
        kFtp * CalculateMillerTwistRuleStabilityFactor(
                   pimpl->diameter_in, GrainT(LbsT(pimpl->build.mass)),
                   pimpl->length_in, pimpl->twist_inches_per_turn,
                   FpsT(pimpl->build.velocity)));

    if (AreEqual(pimpl->build.wind.z, 0.0F)) {
      pimpl->build.aerodynamic_jump = MoaT(0).Float();
      return;
    }

    if (!std::isnan(pimpl->nose_length_in) &&
        !std::isnan(pimpl->meplat_diameter_in) &&
        !std::isnan(pimpl->tail_length_in) &&
        !std::isnan(pimpl->base_diameter_in) && !std::isnan(pimpl->ogive_rtr)) {
      const MachT kMach(FpsT(pimpl->build.velocity),
                        FpsT(pimpl->build.speed_of_sound).Inverse());
      const double kCDref = LobLerp(kMachs, *pimpl->pdrag_lut, kMach);
      pimpl->build.aerodynamic_jump =
          CalculateBRAerodynamicJump(
              pimpl->diameter_in, pimpl->meplat_diameter_in,
              pimpl->base_diameter_in, pimpl->length_in, pimpl->nose_length_in,
              pimpl->tail_length_in, pimpl->ogive_rtr,
              GrainT(LbsT(pimpl->build.mass)), FpsT(pimpl->build.velocity),
              pimpl->build.stability_factor, pimpl->twist_inches_per_turn,
              MphT(FpsT(pimpl->build.wind.z)), pimpl->air_density_lbs_per_cu_ft,
              FpsT(pimpl->build.speed_of_sound),
              pimpl->ballistic_coefficient_psi, kCDref)
              .Float();
      return;
    }

    pimpl->build.aerodynamic_jump =
        CalculateLitzAerodynamicJump(pimpl->build.stability_factor,
                                     pimpl->diameter_in, pimpl->length_in,
                                     MphT(FpsT(pimpl->build.wind.z)))
            .Float();
    return;
  }

  if (std::isnan(pimpl->build.aerodynamic_jump)) {
    pimpl->build.aerodynamic_jump = MoaT(0).Float();
    return;
  }
}

void BuildCoriolis(Impl* pimpl) {
  if (!std::isnan(pimpl->azimuth_rad) && !std::isnan(pimpl->latitude_rad)) {
    // Coriolis Effect Page 178 of Modern Exterior Ballistics - McCoy
    const double kCosL = std::cos(pimpl->latitude_rad).Value();
    const double kSinA = std::sin(pimpl->azimuth_rad).Value();
    const double kSinL = std::sin(pimpl->latitude_rad).Value();
    const double kCosA = std::cos(pimpl->azimuth_rad).Value();

    pimpl->build.corilolis.cos_l_sin_a = static_cast<float>(
        2 * kAngularVelocityOfEarthRadPerSec * kCosL * kSinA);
    pimpl->build.corilolis.sin_l =
        static_cast<float>(2 * kAngularVelocityOfEarthRadPerSec * kSinL);
    pimpl->build.corilolis.cos_l_cos_a = static_cast<float>(
        2 * kAngularVelocityOfEarthRadPerSec * kCosL * kCosA);
  } else {
    pimpl->build.corilolis.cos_l_sin_a = 0;
    pimpl->build.corilolis.sin_l = 0;
    pimpl->build.corilolis.cos_l_cos_a = 0;
  }
}

void BuildZeroAngle(Impl* pimpl) {
  if (!std::isnan(pimpl->build.zero_angle)) {
    return;
  }

  assert(!std::isnan(pimpl->zero_distance_ft));
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

    SpvT s(CartesianT<FeetT>(FeetT(0.0)),
           CartesianT<FpsT>(
               FpsT(pimpl->build.velocity) *
                   std::cos(kZeroAngle +
                            RadiansT(MoaT(pimpl->build.aerodynamic_jump)))
                       .Value(),
               FpsT(pimpl->build.velocity) *
                   std::sin(kZeroAngle +
                            RadiansT(MoaT(pimpl->build.aerodynamic_jump)))
                       .Value(),
               FpsT(0.0)));

    SecT t(0.0);

    while (s.P().X() < pimpl->zero_distance_ft) {
      SolveStep(&s, &t, pimpl->build);
    }

    if (s.P().Y() - FeetT(pimpl->build.optic_height) >
        pimpl->zero_impact_height) {
      high_angle = kZeroAngle;
    } else {
      low_angle = kZeroAngle;
    }
  }
  pimpl->build.zero_angle = MoaT((low_angle + high_angle) / 2).Float();
}
}  // namespace

Input Builder::Build() {
  if (ValidateBuild(*pimpl_)) {
    // This order matters
    BuildEnvironment(pimpl_);
    BuildTable(pimpl_);
    BuildWind(pimpl_);
    if (std::isnan(pimpl_->build.optic_height)) {
      constexpr FeetT kDefaultOpticHeight = InchT(1.5);
      pimpl_->build.optic_height = kDefaultOpticHeight.Float();
    }
    BuildTwistEffects(pimpl_);
    BuildCoriolis(pimpl_);
    BuildZeroAngle(pimpl_);
  }
  return pimpl_->build;
}

namespace {
FpsT CalculateMinimumVelocity(FpsT min_speed, FtLbsT min_energy, SlugT mass) {
  return std::max(CalculateVelocityFromKineticEnergy(min_energy, mass),
                  min_speed);
}
}  // namespace

namespace {
Output Save(FeetT range, FpsT velocity, InchT elevation, InchT deflection,
            SecT time_of_flight, GrainT mass) {
  Output out;
  const FtLbsT kEnergy = CalculateKineticEnergy(velocity, mass);
  out.range = range.U32();
  out.velocity = velocity.U16();
  out.energy = kEnergy.U32();
  out.elevation = elevation.Float();
  out.deflection = deflection.Float();
  out.time_of_flight = time_of_flight.Float();
  return out;
}
}  // namespace

size_t Solve(const Input& in, const uint32_t* pranges, Output* pouts,
             size_t size, const Options& options) {
  if (std::isnan(in.table_coefficient)) {
    return 0;
  }
  const auto kAngle =
      RadiansT(MoaT(in.zero_angle + in.aerodynamic_jump)).Value();
  SpvT s(CartesianT<FeetT>(FeetT(0.0)),
         CartesianT<FpsT>(FpsT(in.velocity) * std::cos(kAngle),
                          FpsT(in.velocity) * std::sin(kAngle), FpsT(0.0)));
  const FpsT kMinimumVelocity = CalculateMinimumVelocity(
      FpsT(options.min_speed), FtLbsT(options.min_energy), LbsT(in.mass));
  SecT t(0.0);
  size_t index = 0;

  while (true) {
    SolveStep(&s, &t, in, options.step_size);
    const FpsT kVelocity = s.V().Magnitude();
    if (s.P().X() >= FeetT(pranges[index])) {
      const InchT kSpinDrift =
          CalculateLitzGyroscopicSpinDrift(in.stability_factor, t);
      pouts[index] =
          Save(s.P().X(), kVelocity, InchT(s.P().Y() - FeetT(in.optic_height)),
               InchT(s.P().Z()) + kSpinDrift, t, LbsT(in.mass));
      index++;
    }

    if (index >= size) {
      break;
    }

    const bool kTimeMaxLimit =
        (t > SecT(options.max_time) && !AreEqual(options.max_time, 0.0F));
    const bool kVelocityLimit = (kVelocity < kMinimumVelocity);
    const bool kFallLimit = (s.V().Y() > s.V().X() * 3);

    if (kTimeMaxLimit || kVelocityLimit || kFallLimit) {
      const InchT kSpinDrift =
          CalculateLitzGyroscopicSpinDrift(in.stability_factor, t);
      pouts[index] =
          Save(s.P().X(), kVelocity, InchT(s.P().Y() - FeetT(in.optic_height)),
               InchT(s.P().Z()) + kSpinDrift, t, LbsT(in.mass));
      index++;
      break;
    }
  }
  return index;
}

namespace {
constexpr float kHundredYardsInFeet = FeetT(YardT(100)).Float();
}  // namespace

// Angle
double MoaToMil(double value) { return MilT(MoaT(value)).Value(); }
double MoaToDeg(double value) { return DegreesT(MoaT(value)).Value(); }
double MoaToIphy(double value) { return IphyT(MoaT(value)).Value(); }
double MoaToInch(double value, double range_ft) {
  return IphyT(MoaT(value)).Value() * range_ft /
         static_cast<double>(kHundredYardsInFeet);
}

double MilToMoa(double value) { return MoaT(MilT(value)).Value(); }
double MilToDeg(double value) { return DegreesT(MilT(value)).Value(); }
double MilToIphy(double value) { return IphyT(MilT(value)).Value(); }
double MilToInch(double value, double range_ft) {
  return IphyT(MilT(value)).Value() * range_ft /
         static_cast<double>(kHundredYardsInFeet);
}

double DegToMoa(double value) { return MoaT(DegreesT(value)).Value(); }
double DegToMil(double value) { return MilT(DegreesT(value)).Value(); }

double InchToMoa(double value, double range_ft) {
  if (AreEqual(range_ft, 0.0)) {
    return 0;
  }
  return MoaT(IphyT(value /
                    (range_ft / static_cast<double>(kHundredYardsInFeet))))
      .Value();
}

double InchToMil(double value, double range_ft) {
  if (AreEqual(range_ft, 0.0)) {
    return 0;
  }
  return MilT(IphyT(value /
                    (range_ft / static_cast<double>(kHundredYardsInFeet))))
      .Value();
}

double InchToDeg(double value, double range_ft) {
  if (AreEqual(range_ft, 0.0)) {
    return 0;
  }
  return DegreesT(IphyT(value /
                        (range_ft / static_cast<double>(kHundredYardsInFeet))))
      .Value();
}

// Energy
double JToFtLbs(double value) { return FtLbsT(JouleT(value)).Value(); }
double FtLbsToJ(double value) { return JouleT(FtLbsT(value)).Value(); }

// Length
double MtoYd(double value) { return YardT(MeterT(value)).Value(); }
double YdToFt(double value) { return FeetT(YardT(value)).Value(); }
double MToFt(double value) { return FeetT(MeterT(value)).Value(); }
double FtToIn(double value) { return InchT(FeetT(value)).Value(); }
double MmToIn(double value) { return InchT(MmT(value)).Value(); }
double CmToIn(double value) { return InchT(CmT(value)).Value(); }
double YdToM(double value) { return MeterT(YardT(value)).Value(); }
double FtToM(double value) { return MeterT(FeetT(value)).Value(); }
double FtToYd(double value) { return YardT(FeetT(value)).Value(); }
double InToMm(double value) { return MmT(InchT(value)).Value(); }
double InToCm(double value) { return CmT(InchT(value)).Value(); }
double InToFt(double value) { return FeetT(InchT(value)).Value(); }

// Pressure
double PaToInHg(double value) { return InHgT(PaT(value)).Value(); }
double MbarToInHg(double value) { return InHgT(MbarT(value)).Value(); }
double PsiToInHg(double value) { return InHgT(PsiT(value)).Value(); }

// Mass
double LbsToGrain(double value) { return GrainT(LbsT(value)).Value(); }
double GToGrain(double value) { return GrainT(LbsT(GramT(value))).Value(); }
double KgToGrain(double value) { return GrainT(LbsT(KgT(value))).Value(); }

// Sectional Density / Ballistic Coefficient
double KgSqMToPmsi(double value) { return PmsiT(KgsmT(value)).Value(); }

// Speed
double FpsToMps(double value) { return MpsT(FpsT(value)).Value(); }
double MpsToFps(double value) { return FpsT(MpsT(value)).Value(); }
double KphToMph(double value) { return MphT(FpsT(KphT(value))).Value(); }
double KnToMph(double value) { return MphT(FpsT(KnT(value))).Value(); }

// Time
double MsToS(double value) { return SecT(MsecT(value)).Value(); }
double UsToS(double value) { return SecT(UsecT(value)).Value(); }
double SToMs(double value) { return MsecT(SecT(value)).Value(); }
double SToUs(double value) { return UsecT(SecT(value)).Value(); }

// Temperature
double DegCToDegF(double value) { return DegFT(DegCT(value)).Value(); }
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
