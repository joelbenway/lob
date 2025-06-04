// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <utility>

#include "boatright.hpp"
#include "calc.hpp"
#include "cartesian.hpp"
#include "constants.hpp"
#include "eng_units.hpp"
#include "helpers.hpp"
#include "litz.hpp"
#include "lob/lob.hpp"
#include "ode.hpp"
#include "solve_step.hpp"
#include "tables.hpp"

namespace lob {

class Impl {
  friend class Builder;

 public:
  LbsPerCuFtT air_density_lbs_per_cu_ft{NaN()};
  FeetT altitude_ft{NaN()};
  FeetT altitude_of_barometer_ft{NaN()};
  FeetT altitude_of_thermometer_ft{NaN()};
  AtmosphereReferenceT atmosphere_reference{
      AtmosphereReferenceT::kArmyStandardMetro};
  RadiansT azimuth_rad{NaN()};
  PmsiT ballistic_coefficient_psi{NaN()};
  InchT base_diameter_in{NaN()};
  InHgT air_pressure_in_hg{NaN()};
  InchT diameter_in{NaN()};
  RadiansT latitude_rad{NaN()};
  InchT length_in{NaN()};
  InchT meplat_diameter_in{NaN()};
  InchT nose_length_in{NaN()};
  double ogive_rtr{NaN()};
  const std::array<uint16_t, kTableSize>* pdrag_lut{&kG1Drags};
  RadiansT range_angle_rad{NaN()};
  double relative_humidity_percent{NaN()};
  InchT tail_length_in{NaN()};
  DegFT temperature_deg_f{NaN()};
  InchPerTwistT twist_inches_per_turn{NaN()};
  RadiansT wind_heading_rad{NaN()};
  FpsT wind_speed_fps{NaN()};
  FeetT zero_distance_ft{NaN()};
  FeetT zero_impact_height{NaN()};

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

Builder& Builder::AltitudeOfFiringSiteFt(double value) {
  pimpl_->altitude_ft = FeetT(value);
  return *this;
}

Builder& Builder::AltitudeOfBarometerFt(double value) {
  pimpl_->altitude_of_barometer_ft = FeetT(value);
  return *this;
}

Builder& Builder::AltitudeOfThermometerFt(double value) {
  pimpl_->altitude_of_thermometer_ft = FeetT(value);
  return *this;
}

Builder& Builder::AzimuthDeg(double value) {
  pimpl_->azimuth_rad = DegreesT(value);
  return *this;
}

Builder& Builder::BallisticCoefficientPsi(double value) {
  pimpl_->ballistic_coefficient_psi = PmsiT(value);
  return *this;
}

Builder& Builder::AirPressureInHg(double value) {
  pimpl_->air_pressure_in_hg = InHgT(value);
  return *this;
}

Builder& Builder::BaseDiameterInch(double value) {
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

Builder& Builder::DiameterInch(double value) {
  pimpl_->diameter_in = InchT(value);
  return *this;
}

Builder& Builder::InitialVelocityFps(uint16_t value) {
  pimpl_->build.velocity = FpsT(value).U16();
  return *this;
}

Builder& Builder::LatitudeDeg(double value) {
  pimpl_->latitude_rad = DegreesT(value);
  return *this;
}

Builder& Builder::LengthInch(double value) {
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

Builder& Builder::MassGrains(double value) {
  pimpl_->build.mass = LbsT(GrainT(value)).Value();
  return *this;
}

Builder& Builder::MeplatDiameterInch(double value) {
  pimpl_->meplat_diameter_in = InchT(value);
  return *this;
}

Builder& Builder::NoseLengthInch(double value) {
  pimpl_->nose_length_in = InchT(value);
  return *this;
}

Builder& Builder::OgiveRtR(double value) {
  pimpl_->ogive_rtr = value;
  return *this;
}

Builder& Builder::OpticHeightInches(double value) {
  pimpl_->build.optic_height = FeetT(InchT(value)).Value();
  return *this;
}

Builder& Builder::RelativeHumidityPercent(double value) {
  pimpl_->relative_humidity_percent = value;
  return *this;
}

Builder& Builder::RangeAngleDeg(double value) {
  pimpl_->range_angle_rad = RadiansT(DegreesT(value));
  return *this;
}

Builder& Builder::TailLengthInch(double value) {
  pimpl_->tail_length_in = InchT(value);
  return *this;
}

Builder& Builder::TemperatureDegF(double value) {
  pimpl_->temperature_deg_f = DegFT(value);
  return *this;
}

Builder& Builder::TwistInchesPerTurn(double value) {
  pimpl_->twist_inches_per_turn = InchPerTwistT(value);
  return *this;
}

Builder& Builder::WindHeading(ClockAngleT value) {
  constexpr DegreesT kDegreesPerClockNumber = DegreesT(kDegreesPerTurn) / 12;
  pimpl_->wind_heading_rad =
      DegreesT(kDegreesPerClockNumber * static_cast<uint8_t>(value));
  return *this;
}

Builder& Builder::WindHeadingDeg(double value) {
  const DegreesT kFullTurn(kDegreesPerTurn);
  const DegreesT kQuarterTurn(kFullTurn / 4);

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

Builder& Builder::WindSpeedFps(double value) {
  pimpl_->wind_speed_fps = FpsT(value);
  return *this;
}

Builder& Builder::WindSpeedMph(double value) {
  pimpl_->wind_speed_fps = MphT(value);
  return *this;
}

Builder& Builder::ZeroAngleMOA(double value) {
  pimpl_->build.zero_angle = MoaT(value).Value();
  return *this;
}

Builder& Builder::ZeroDistanceYds(double value) {
  pimpl_->zero_distance_ft = YardT(value);
  return *this;
}

Builder& Builder::ZeroImpactHeightInches(double value) {
  pimpl_->zero_impact_height = InchT(value);
  return *this;
}

namespace {
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
      CalculateAirDensityRatio(pressure_at_firing_site,
                               temperature_at_firing_site) *
      CalculateAirDensityRatioHumidityCorrection(
          pimpl->relative_humidity_percent, kWaterVaporSaturationPressureInHg));

  pimpl->build.speed_of_sound =
      FpsT(CalculateSpeedOfSoundInAir(temperature_at_firing_site) *
           CalculateSpeedOfSoundHumidityCorrection(
               pimpl->relative_humidity_percent,
               kWaterVaporSaturationPressureInHg))
          .Value();
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
  pimpl->build.table_coefficient = kCdCoefficient;
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
          .Value();
  pimpl->build.wind.z =
      FpsT(pimpl->wind_speed_fps * std::cos(pimpl->wind_heading_rad.Value()))
          .Value();
}

void BuildStability(Impl* pimpl) {
  assert(pimpl->build.velocity > 0);
  assert(!std::isnan(pimpl->air_density_lbs_per_cu_ft));

  if (!std::isnan(pimpl->diameter_in) && !std::isnan(pimpl->length_in) &&
      !std::isnan(pimpl->twist_inches_per_turn) &&
      !std::isnan(pimpl->build.mass)) {
    const double kFtp = CalculateMillerTwistRuleCorrectionFactor(
        pimpl->air_density_lbs_per_cu_ft);
    pimpl->build.stability_factor =
        kFtp * CalculateMillerTwistRuleStabilityFactor(
                   pimpl->diameter_in, GrainT(LbsT(pimpl->build.mass)),
                   pimpl->length_in, pimpl->twist_inches_per_turn,
                   FpsT(pimpl->build.velocity));
  }
}

void BuildBoatright(Impl* pimpl) {
  assert(pimpl != nullptr);
  assert(pimpl->pdrag_lut != nullptr);

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

  if (kD.IsNaN() || kDM.IsNaN() || kDB.IsNaN() || kDB.IsNaN() || kL.IsNaN() ||
      kLN.IsNaN() || kLBT.IsNaN() || std::isnan(kRTR) ||
      !(kVelocity > FpsT(0)) || kSos.IsNaN() || kMass.IsNaN() ||
      kTwist.IsNaN() || kAirDensity.IsNaN() || std::isnan(kSg) || kBc.IsNaN() ||
      kZWind.IsNaN()) {
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
  const double kCdRef = LobLerp(kMachs, *pimpl->pdrag_lut, kM);
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

  while (s.V().X() > kTransonicBarrier) {
    assert(t.Value() < 100.0 && "This is taking too long");
    SolveStep(&s, &t, pimpl->build);
  }

  const auto kV = boatright::CalculateKV(kVelocity, kTransonicBarrier);
  const auto kOmega = boatright::CalculateKOmega(kD, t);
  const double kQTS = boatright::CalculatePotentialDragForce(kD, kAirDensity,
                                                             kTransonicBarrier);
  const auto kBetaROfT = boatright::CalculateYawOfRepose(
      kVelocity, kTwist, kIyOverIx, kR, kOmega, kV);

  PmsiT bc_g7(0);
  if (pimpl->pdrag_lut == &kG7Drags) {
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
  }

  if (std::isnan(pimpl->build.aerodynamic_jump)) {
    pimpl->build.aerodynamic_jump = MoaT(0).Value();
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

    pimpl->build.corilolis.cos_l_sin_a =
        2 * kAngularVelocityOfEarthRadPerSec * kCosL * kSinA;
    pimpl->build.corilolis.sin_l = 2 * kAngularVelocityOfEarthRadPerSec * kSinL;
    pimpl->build.corilolis.cos_l_cos_a =
        2 * kAngularVelocityOfEarthRadPerSec * kCosL * kCosA;
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

    TrajectoryStateT s(
        CartesianT<FeetT>(FeetT(0.0)),
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
  pimpl->build.zero_angle = MoaT((low_angle + high_angle) / 2).Value();
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
      pimpl_->build.optic_height = kDefaultOpticHeight.Value();
    }
    BuildStability(pimpl_);
    BuildCoriolis(pimpl_);
    BuildBoatright(pimpl_);
    BuildLitzAerodynamicJump(pimpl_);
    BuildZeroAngle(pimpl_);
  }
  return pimpl_->build;
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
