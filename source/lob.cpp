// This file is a part of lob, an exterior ballistics calculation library
// Copyright (c) 2024  Joel Benway
// Please see end of file for extended copyright information

#include "lob/lob.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <utility>

#include "calc.hpp"
#include "cartesian.hpp"
#include "constants.hpp"
#include "eng_units.hpp"
#include "ode.hpp"
#include "version.hpp"

namespace lob {

namespace {
uint16_t ToU16(double x) { return static_cast<uint16_t>(std::round(x)); }
}  // namespace

const char* Version() { return kProjectVersion; }

class Lob::Impl {
  friend class Lob::Builder;
  friend class Lob;
  static constexpr auto kNaN = std::numeric_limits<double>::quiet_NaN();

  RadiansT aerodynamic_jump_rad_{kNaN};
  FeetT altitude_ft_{kNaN};
  FeetT altitude_of_barometer_ft_{kNaN};
  FeetT altitude_of_thermometer_ft_{kNaN};
  AtmosphereReferenceT atmosphere_reference_{
      AtmosphereReferenceT::kArmyStandardMetro};
  RadiansT azimuth_rad_{kNaN};
  PmsiT ballistic_coefficent_psi_{kNaN};
  InHgT air_pressure_in_hg_{kNaN};
  InchT diameter_in_{kNaN};
  FpsT initial_velocity_fps_{kNaN};
  RadiansT latitude_rad_{kNaN};
  InchT length_in_{kNaN};
  FeetT limit_max_distance_ft_{kNaN};
  FtLbsT limit_min_energy_ft_lbs_{kNaN};
  SecT limit_time_of_flight_sec_{kNaN};
  LbsT mass_lbs_{kNaN};
  FeetT optic_height_ft_{kNaN};
  double relative_humidity_percent_{kNaN};
  double stability_factor_{kNaN};
  SecT step_size_sec_{kNaN};
  RadiansT target_angle_rad_{kNaN};
  FeetT target_distance_ft_{kNaN};
  DegFT temperature_deg_f_{kNaN};
  InchPerTwistT twist_inches_per_turn_{kNaN};
  bool twist_is_right_hand_{true};
  RadiansT wind_heading_rad_{kNaN};
  FpsT wind_speed_fps_{kNaN};
  RadiansT zero_angle_rad_{kNaN};
  FeetT zero_distance_ft_{kNaN};
  FeetT zero_impact_height_{kNaN};

  LbsPerCuFtT air_density_{kNaN};
  FpsT local_speed_of_sound_fps_{kNaN};
  double coefficent_of_drag_coefficent_{kNaN};
  const std::array<float, kTableSize>* pdrag_coefficent_lut_{
      &kG1DragCoefficents};
  double coriolis_cos_l_sin_a_{kNaN};
  double coriolis_sin_l_{kNaN};
  double corilois_cos_l_cos_a_{kNaN};

  void InitializeMembers();
  void InitializeShotMembers();
  void InitializeEnvironmentMembers();
  void InitializeAdvancedMembers();
  double GetCoefficentOfDrag(FpsT speed) const;
  void SolveStep(SpvT* ps, SecT* pt) const;
  RadiansT ZeroAngleSearch();
  size_t FullSolve(Lob::Solution* psolution, uint16_t* pranges,
                   size_t length) const;
  void SaveSolution(Lob::Solution* psolution, const SpvT& s,
                    const SecT& t) const;
};  // class Lob::Impl

Lob::Lob() : pimpl_{new Impl} {}

Lob::Lob(const Lob& other) : pimpl_{new Impl(*other.pimpl_)} {}

Lob::Lob(Lob&& other) noexcept = default;

Lob& Lob::operator=(const Lob& rhs) {
  if (this != &rhs) {
    pimpl_ = std::make_unique<Impl>(*rhs.pimpl_);
  }
  return *this;
}

Lob& Lob::operator=(Lob&& rhs) noexcept {
  if (this != &rhs) {
    std::swap(pimpl_, rhs.pimpl_);
  }
  return *this;
}

Lob::~Lob() = default;

const Lob::Impl* Lob::Pimpl() const { return pimpl_.get(); }

Lob::Impl* Lob::Pimpl() { return pimpl_.get(); }

Lob::Builder& Lob::Builder::AltitudeOfFiringSiteFt(double value) {
  plob_->Pimpl()->altitude_ft_ = FeetT(value);
  return *this;
}

Lob::Builder& Lob::Builder::AltitudeOfBarometerFt(double value) {
  plob_->Pimpl()->altitude_of_barometer_ft_ = FeetT(value);
  return *this;
}

Lob::Builder& Lob::Builder::AltitudeOfThermometerFt(double value) {
  plob_->Pimpl()->altitude_of_thermometer_ft_ = FeetT(value);
  return *this;
}

Lob::Builder& Lob::Builder::AzimuthDeg(double value) {
  plob_->Pimpl()->azimuth_rad_ = DegreesT(value);
  return *this;
}

Lob::Builder& Lob::Builder::BallisticCoefficentPsi(double value) {
  plob_->Pimpl()->ballistic_coefficent_psi_ = PmsiT(value);
  return *this;
}

Lob::Builder& Lob::Builder::AirPressureInHg(double value) {
  plob_->Pimpl()->air_pressure_in_hg_ = InHgT(value);
  return *this;
}

Lob::Builder& Lob::Builder::BCAtmosphere(AtmosphereReferenceT type) {
  plob_->Pimpl()->atmosphere_reference_ = type;
  return *this;
}

Lob::Builder& Lob::Builder::BCDragFunction(DragFunctionT type) {
  switch (type) {
    case DragFunctionT::kG1: {
      plob_->Pimpl()->pdrag_coefficent_lut_ = &kG1DragCoefficents;
      break;
    }
    case DragFunctionT::kG2: {
      plob_->Pimpl()->pdrag_coefficent_lut_ = &kG2DragCoefficents;
      break;
    }
    case DragFunctionT::kG5: {
      plob_->Pimpl()->pdrag_coefficent_lut_ = &kG5DragCoefficents;
      break;
    }
    case DragFunctionT::kG6: {
      plob_->Pimpl()->pdrag_coefficent_lut_ = &kG6DragCoefficents;
      break;
    }
    case DragFunctionT::kG7: {
      plob_->Pimpl()->pdrag_coefficent_lut_ = &kG7DragCoefficents;
      break;
    }
    case DragFunctionT::kG8: {
      plob_->Pimpl()->pdrag_coefficent_lut_ = &kG8DragCoefficents;
      break;
    }
    default: {
      break;
    }
  }
  return *this;
}

Lob::Builder& Lob::Builder::DiameterInch(double value) {
  plob_->Pimpl()->diameter_in_ = InchT(value);
  return *this;
}

Lob::Builder& Lob::Builder::InitialVelocityFps(double value) {
  plob_->Pimpl()->initial_velocity_fps_ = FpsT(value);
  return *this;
}

Lob::Builder& Lob::Builder::LatitudeDeg(double value) {
  plob_->Pimpl()->latitude_rad_ = DegreesT(value);
  return *this;
}

Lob::Builder& Lob::Builder::LengthInch(double value) {
  plob_->Pimpl()->length_in_ = InchT(value);
  return *this;
}

Lob::Builder& Lob::Builder::LimitMaxDistanceYds(double value) {
  plob_->Pimpl()->limit_max_distance_ft_ = YardT(value);
  return *this;
}

Lob::Builder& Lob::Builder::LimitMinEnergyFtLbs(double value) {
  plob_->Pimpl()->limit_min_energy_ft_lbs_ = FtLbsT(value);
  return *this;
}

Lob::Builder& Lob::Builder::LimitTimeOfFlightSec(double value) {
  plob_->Pimpl()->limit_time_of_flight_sec_ = SecT(value);
  return *this;
}

Lob::Builder& Lob::Builder::MassGrains(double value) {
  plob_->Pimpl()->mass_lbs_ = GrainT(value);
  return *this;
}

Lob::Builder& Lob::Builder::OpticHeightInches(double value) {
  plob_->Pimpl()->optic_height_ft_ = InchT(value);
  return *this;
}

Lob::Builder& Lob::Builder::RelativeHumidityPercent(double value) {
  plob_->Pimpl()->relative_humidity_percent_ = value;
  return *this;
}

Lob::Builder& Lob::Builder::SolverStepSizeUsec(uint16_t value) {
  plob_->Pimpl()->step_size_sec_ = UsecT(value);
  return *this;
}

Lob::Builder& Lob::Builder::TargetAngleDeg(double value) {
  plob_->Pimpl()->target_angle_rad_ = DegreesT(value);
  return *this;
}

Lob::Builder& Lob::Builder::TargetDistanceYds(double value) {
  plob_->Pimpl()->target_distance_ft_ = YardT(value);
  return *this;
}
Lob::Builder& Lob::Builder::TemperatureDegF(double value) {
  plob_->Pimpl()->temperature_deg_f_ = DegFT(value);
  return *this;
}

Lob::Builder& Lob::Builder::TwistInchesPerTurn(double value) {
  plob_->Pimpl()->twist_inches_per_turn_ = InchPerTwistT(value);
  plob_->Pimpl()->twist_is_right_hand_ = (value > 0);
  return *this;
}

Lob::Builder& Lob::Builder::WindHeading(ClockAngleT value) {
  constexpr DegreesT kDegreesPerClockNumber = DegreesT(kDegreesPerTurn) / 12;
  plob_->Pimpl()->wind_heading_rad_ =
      DegreesT(kDegreesPerClockNumber * static_cast<uint8_t>(value));
  return *this;
}

Lob::Builder& Lob::Builder::WindHeadingDeg(double value) {
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

  plob_->Pimpl()->wind_heading_rad_ = angle;

  return *this;
}

Lob::Builder& Lob::Builder::WindSpeedFps(double value) {
  plob_->Pimpl()->wind_speed_fps_ = FpsT(value);
  return *this;
}

Lob::Builder& Lob::Builder::WindSpeedMph(double value) {
  plob_->Pimpl()->wind_speed_fps_ = MphT(value);
  return *this;
}

Lob::Builder& Lob::Builder::ZeroAngleMOA(double value) {
  plob_->Pimpl()->zero_angle_rad_ = MoaT(value);
  return *this;
}

Lob::Builder& Lob::Builder::ZeroDistanceYds(double value) {
  plob_->Pimpl()->zero_distance_ft_ = YardT(value);
  return *this;
}

Lob::Builder& Lob::Builder::ZeroImpactHeightInches(double value) {
  plob_->Pimpl()->zero_impact_height_ = InchT(value);
  return *this;
}

std::unique_ptr<Lob> Lob::Builder::Build() {
  plob_->Pimpl()->InitializeMembers();
  return std::move(plob_);
}

void Lob::Impl::InitializeMembers() {
  assert(!std::isnan(ballistic_coefficent_psi_));
  assert(!std::isnan(initial_velocity_fps_));
  assert(!std::isnan(diameter_in_));
  assert(!std::isnan(mass_lbs_));
  assert(!std::isnan(zero_distance_ft_) || !std::isnan(zero_angle_rad_));
  assert(!std::isnan(target_distance_ft_) ||
         !std::isnan(limit_max_distance_ft_));

  InitializeShotMembers();
  InitializeEnvironmentMembers();
  InitializeAdvancedMembers();

  if (std::isnan(zero_angle_rad_)) {
    zero_angle_rad_ = ZeroAngleSearch();
  }
}

void Lob::Impl::InitializeShotMembers() {
  constexpr FeetT kDefaultOpticHeight = InchT(1.5);

  if (std::isnan(zero_impact_height_)) {
    zero_impact_height_ = FeetT(0.0);
  }

  if (std::isnan(target_angle_rad_)) {
    target_angle_rad_ = DegreesT(0.0);
  }

  if (atmosphere_reference_ == AtmosphereReferenceT::kArmyStandardMetro) {
    ballistic_coefficent_psi_ *= kArmyToIcaoBcConversionFactor;
    atmosphere_reference_ = AtmosphereReferenceT::kIcao;
  }

  if (std::isnan(optic_height_ft_)) {
    optic_height_ft_ = kDefaultOpticHeight;
  }

  if (!isnan(target_distance_ft_) && std::isnan(limit_max_distance_ft_)) {
    limit_max_distance_ft_ = target_distance_ft_;
  }

  if (std::isnan(target_distance_ft_) && !std::isnan(limit_max_distance_ft_)) {
    target_distance_ft_ = limit_max_distance_ft_;
  }

  if (!std::isnan(target_distance_ft_) && !std::isnan(limit_max_distance_ft_)) {
    limit_max_distance_ft_ =
        std::min(target_distance_ft_, limit_max_distance_ft_);
  }
}

void Lob::Impl::InitializeEnvironmentMembers() {
  FeetT altitude_of_firing_site = FeetT(0);
  FeetT altitude_of_barometer = FeetT(0);
  FeetT altitude_of_thermometer = FeetT(0);
  DegFT temperature_at_firing_site = DegFT(kIsaSeaLevelDegF);
  DegFT temperature_at_barometer = DegFT(kIsaSeaLevelDegF);
  InHgT pressure_at_firing_site = InHgT(kIsaSeaLevelPressureInHg);

  if (!std::isnan(altitude_ft_)) {
    altitude_of_firing_site = altitude_ft_;
    altitude_of_barometer = std::isnan(altitude_of_barometer_ft_)
                                ? altitude_ft_
                                : altitude_of_barometer_ft_;
    altitude_of_thermometer = std::isnan(altitude_of_thermometer_ft_)
                                  ? altitude_ft_
                                  : altitude_of_thermometer_ft_;

    temperature_at_firing_site =
        CalculateTemperatureAtAltitude(altitude_of_firing_site);
    pressure_at_firing_site = BarometricFormula(altitude_of_firing_site);
  }

  if (!std::isnan(temperature_deg_f_)) {
    temperature_at_firing_site = CalculateTemperatureAtAltitude(
        altitude_of_firing_site - altitude_of_thermometer, temperature_deg_f_);
    temperature_at_barometer = CalculateTemperatureAtAltitude(
        altitude_of_barometer - altitude_of_thermometer, temperature_deg_f_);
  }

  if (!std::isnan(air_pressure_in_hg_)) {
    pressure_at_firing_site =
        BarometricFormula(altitude_of_firing_site - altitude_of_barometer,
                          air_pressure_in_hg_, temperature_at_barometer);
  }

  if (std::isnan(relative_humidity_percent_)) {
    relative_humidity_percent_ = kIsaSeaLevelHumidityPercent;
  }

  if (std::isnan(wind_heading_rad_)) {
    wind_heading_rad_ = DegreesT(0);
  }

  if (std::isnan(wind_speed_fps_)) {
    wind_speed_fps_ = FpsT(0);
  }

  const auto kWaterVaporSaturationPressureInHg =
      CalculateWaterVaporSaturationPressure(temperature_at_firing_site);

  air_density_ =
      LbsPerCuFtT(kIsaSeaLevelAirDensityLbsPerCuFt) *
      CalcualteAirDensityRatio(pressure_at_firing_site,
                               temperature_at_firing_site) *
      CalculateAirDensityRatioHumidityCorrection(
          relative_humidity_percent_, kWaterVaporSaturationPressureInHg);

  local_speed_of_sound_fps_ =
      CalculateSpeedOfSoundInAir(temperature_at_firing_site) *
      CalculateSpeedOfSoundHumidityCorrection(
          relative_humidity_percent_, kWaterVaporSaturationPressureInHg);

  coefficent_of_drag_coefficent_ =
      CalculateCdCoefficent(air_density_, ballistic_coefficent_psi_);
}

void Lob::Impl::InitializeAdvancedMembers() {
  if (!std::isnan(length_in_) && !std::isnan(twist_inches_per_turn_) &&
      !std::isnan(mass_lbs_) && !std::isnan(diameter_in_)) {
    const auto kFtp = CalculateMillerTwistRuleCorrectionFactor(air_density_);
    stability_factor_ =
        kFtp * CalculateMillerTwistRuleStabilityFactor(
                   diameter_in_, GrainT(mass_lbs_), length_in_,
                   twist_inches_per_turn_, initial_velocity_fps_);

    if (!std::isnan(wind_speed_fps_) && !std::isnan(wind_heading_rad_)) {
      const MphT kXWind = wind_speed_fps_ * std::sin(wind_heading_rad_.Value());
      aerodynamic_jump_rad_ = CalculateLitzAerodynamicJump(
          stability_factor_, diameter_in_, length_in_, kXWind);
    }
  }

  if (std::isnan(aerodynamic_jump_rad_)) {
    aerodynamic_jump_rad_ = RadiansT(0);
  }

  if (!std::isnan(azimuth_rad_) && !std::isnan(latitude_rad_)) {
    // Coriolis Effect Page 178 of Modern Exterior Ballistics - McCoy
    constexpr double kAngularVelocityOfEarth = 7.292115E5;  // Radians/Sec
    const double kCosL = std::cos(latitude_rad_).Value();
    const double kSinA = std::sin(azimuth_rad_).Value();
    const double kSinL = std::sin(latitude_rad_).Value();
    const double kCosA = std::cos(azimuth_rad_).Value();

    coriolis_cos_l_sin_a_ = 2 * kAngularVelocityOfEarth * kCosL * kSinA;
    coriolis_sin_l_ = 2 * kAngularVelocityOfEarth * kSinL;
    corilois_cos_l_cos_a_ = 2 * kAngularVelocityOfEarth * kCosL * kCosA;
  } else {
    coriolis_cos_l_sin_a_ = 0;
    coriolis_sin_l_ = 0;
    corilois_cos_l_cos_a_ = 0;
  }
}

double Lob::Impl::GetCoefficentOfDrag(const FpsT speed) const {
  static size_t index = kTableSize - 1;
  const auto kMachSpeed = (speed / local_speed_of_sound_fps_).Value();
  const double kCd =
      LobLerp(kMachValues, *pdrag_coefficent_lut_, kMachSpeed, &index);
  return coefficent_of_drag_coefficent_ * kCd;
}

void Lob::Impl::SolveStep(SpvT* ps, SecT* pt) const {
  const CartesianT<FpsSqT> kGravity(
      FpsSqT(kStandardGravity * -1 * std::sin(target_angle_rad_.Value())),
      FpsSqT(kStandardGravity * -1 * std::cos(target_angle_rad_.Value())),
      FpsSqT(0.0));

  const CartesianT<FpsT> kWind(
      FpsT(wind_speed_fps_ * std::sin(wind_heading_rad_.Value())), FpsT(0.0),
      FpsT(wind_speed_fps_ * std::cos(wind_heading_rad_.Value())));

  const double kClSa = coriolis_cos_l_sin_a_;
  const double kSl = coriolis_sin_l_;
  const double kClCa = corilois_cos_l_cos_a_;

  // This variable is marked volatile because it's only read inside a closure
  // which fools static analyzers into warnings otherwise.
  volatile double cd = GetCoefficentOfDrag(ps->V().Magnitude());

  auto ds_dt = [kGravity, kWind, kClSa, kSl, kClCa, cd](double t,
                                                        const SpvT& s) -> SpvT {
    static_cast<void>(t);  // t is unused
    const CartesianT<FeetT> kPosition(FeetT(s.V().X().Value()),
                                      FeetT(s.V().Y().Value()),
                                      FeetT(s.V().Z().Value()));
    const auto kScalarVelocity = (s.V() - kWind).Magnitude();
    CartesianT<FpsT> velocity =
        (s.V() - kWind) * FpsT(-1 * cd) * kScalarVelocity;
    velocity.X(velocity.X() - velocity.Y() * kClSa - velocity.Z() * kSl);
    velocity.Y(velocity.Y() + velocity.X() * kClSa + velocity.Z() * kClCa);
    velocity.Z(velocity.Z() + velocity.X() * kSl - velocity.Y() * kClCa);
    velocity.X(velocity.X() + kGravity.X().Value());
    velocity.Y(velocity.Y() + kGravity.Y().Value());
    return SpvT{kPosition, velocity};
  };

  SecT dt(0);
  if (std::isnan(step_size_sec_)) {
    dt = SecT(1 / ps->V().Magnitude().Value());
  } else {
    dt = step_size_sec_;
  }

  *ps = HeunStep(0.0, *ps, dt.Value(), ds_dt);
  *pt += dt;

  cd = GetCoefficentOfDrag(ps->V().Magnitude());
}

RadiansT Lob::Impl::ZeroAngleSearch() {
  constexpr RadiansT kZeroAngleError = MoaT(0.01);
  constexpr RadiansT kMaxZeroAngle = DegreesT(45);
  constexpr RadiansT kMinZeroAngle = DegreesT(0.0);
  RadiansT high_angle = kMaxZeroAngle;
  RadiansT low_angle = kMinZeroAngle;

  while (high_angle - low_angle > kZeroAngleError) {
    const RadiansT kZeroAngle = (low_angle + high_angle) / 2;

    SpvT s(CartesianT<FeetT>(FeetT(0.0)),
           CartesianT<FpsT>(
               initial_velocity_fps_ *
                   std::cos(kZeroAngle + aerodynamic_jump_rad_).Value(),
               initial_velocity_fps_ *
                   std::sin(kZeroAngle + aerodynamic_jump_rad_).Value(),
               FpsT(0.0)));

    SecT t(0.0);

    while (s.P().X() < zero_distance_ft_) {
      SolveStep(&s, &t);
    }

    if (s.P().Y() - optic_height_ft_ > zero_impact_height_) {
      high_angle = kZeroAngle;
    } else {
      low_angle = kZeroAngle;
    }
  }
  return (low_angle + high_angle) / 2;
}

size_t Lob::Impl::FullSolve(Lob::Solution* psolution, uint16_t* pranges,
                            size_t length) const {
  // NOLINTNEXTLINE note level messages
  if (psolution == nullptr || pranges == nullptr) {
    return 0;
  }

  SpvT s(CartesianT<FeetT>(FeetT(0.0)),
         CartesianT<FpsT>(
             initial_velocity_fps_ *
                 std::cos(zero_angle_rad_ + aerodynamic_jump_rad_).Value(),
             initial_velocity_fps_ *
                 std::sin(zero_angle_rad_ + aerodynamic_jump_rad_).Value(),
             FpsT(0.0)));
  SecT t(0.0);
  std::size_t index = 0;

  if (s.P().X() >= FeetT(pranges[index])) {
    SaveSolution(&psolution[index], s, t);
    index++;
  }

  while (s.P().X() < target_distance_ft_) {
    SolveStep(&s, &t);

    // NOLINTNEXTLINE 1st function call argument is an uninitialized value
    if (s.P().X() >= FeetT(pranges[index])) {
      SaveSolution(&psolution[index], s, t);
      index++;
    }

    if (index >= length) {
      break;
    }
    if (t > limit_time_of_flight_sec_) {
      break;
    }
    if (limit_min_energy_ft_lbs_ >
        CalculateKineticEnergy(s.V().Magnitude(), mass_lbs_)) {
      break;
    }
    if (s.V().Y() > s.V().X() * 4) {
      break;
    }
  }
  return index;
}

void Lob::Impl::SaveSolution(Lob::Solution* psolution, const SpvT& s,
                             const SecT& t) const {
  psolution->range = ToU16(YardT(s.P().X()).Value());

  psolution->velocity = ToU16(s.V().Magnitude().Value());

  psolution->energy =
      ToU16(CalculateKineticEnergy(s.V().Magnitude(), mass_lbs_).Value());

  psolution->elevation_distance =
      static_cast<float>(InchT(s.P().Y() - optic_height_ft_).Value());

  const double kAngle = RadiansT(MoaT(1)).Value();

  if (InchT(s.P().X()).Value() == 0) {
    psolution->elevation_adjustments = 0;
  } else {
    psolution->elevation_adjustments =
        static_cast<float>((InchT(s.P().Y() - optic_height_ft_) /
                            (InchT(s.P().X()) * std::tan(kAngle)))
                               .Value());
  }

  InchT spin_drift(0);
  if (!std::isnan(stability_factor_) && !std::isnan(twist_inches_per_turn_)) {
    spin_drift = CalculateLitzGyroscopicSpinDrift(stability_factor_, t,
                                                  twist_is_right_hand_);
  }

  psolution->windage_distance =
      static_cast<float>(InchT(s.P().Z() + spin_drift).Value());

  if (InchT(s.P().X()).Value() == 0) {
    psolution->windage_adjustments = 0;
  } else {
    psolution->windage_adjustments =
        static_cast<float>(((InchT(s.P().Z()) + spin_drift) /
                            (InchT(s.P().X()) * std::tan(kAngle)))
                               .Value());
  }

  psolution->time_of_flight = static_cast<float>(t.Value());
}

float Lob::GetAirDensityLbsPerCuFt() const {
  if (std::isnan(Pimpl()->air_density_)) {
    return 0.0F;
  }
  return static_cast<float>(Pimpl()->air_density_.Value());
}

float Lob::GetSpeedOfSoundFps() const {
  if (std::isnan(Pimpl()->local_speed_of_sound_fps_)) {
    return 0.0F;
  }
  return static_cast<float>(Pimpl()->local_speed_of_sound_fps_.Value());
}

float Lob::GetStabilityFactor() const {
  if (std::isnan(Pimpl()->stability_factor_)) {
    return 0.0F;
  }
  return static_cast<float>(Pimpl()->stability_factor_);
}

float Lob::GetZeroAngleMOA() const {
  return static_cast<float>(MoaT(Pimpl()->zero_angle_rad_).Value());
}

Lob::Solution Lob::Solve() const {
  Lob::Solution solution = {0};
  uint16_t range = 0;

  if (!std::isnan(Pimpl()->target_distance_ft_)) {
    range = ToU16(Pimpl()->target_distance_ft_.Value());
  }

  Pimpl()->FullSolve(&solution, &range, 1);
  return solution;
}

size_t Lob::Solve(Lob::Solution* psolution, const uint16_t* pranges,
                  size_t length) const {
  // NOLINTNEXTLINE note level messages
  if (psolution == nullptr || length == 0) {
    return 0;
  }

  uint16_t ranges[length];  // NOLINT allow c-style array

  if (pranges != nullptr) {
    bool values_are_ascending = true;
    bool values_are_in_range = true;

    for (size_t i = 0; i < length; i++) {
      ranges[i] = ToU16(FeetT(YardT(pranges[i])).Value());
    }

    for (size_t i = 1; i < length; i++) {
      if (ranges[i] < ranges[i - 1]) {
        values_are_ascending = false;
        break;
      }
    }

    for (size_t i = 0; i < length; i++) {
      if (ranges[i] > Pimpl()->limit_max_distance_ft_.Value()) {
        values_are_in_range = false;
        break;
      }
    }

    if (values_are_ascending && values_are_in_range) {
      // NOLINTNEXTLINE implicitly decay an array into a pointer
      return Pimpl()->FullSolve(psolution, ranges, length);
    }
  }

  for (size_t i = 0; i < length; i++) {
    ranges[i] = ToU16(Pimpl()->target_distance_ft_.Value()) * (i + 1) / length;
  }

  // NOLINTNEXTLINE implicitly decay an array into a pointer
  return Pimpl()->FullSolve(psolution, ranges, length);
}

namespace {

// Angle
constexpr double MoaToMil(double value) { return MilT(MoaT(value)).Value(); }
constexpr double MoaToDeg(double value) {
  return DegreesT(MoaT(value)).Value();
}
constexpr double MilToMoa(double value) { return MoaT(MilT(value)).Value(); }
constexpr double MilToDeg(double value) {
  return DegreesT(MilT(value)).Value();
}
constexpr double DegToMoa(double value) {
  return MoaT(DegreesT(value)).Value();
}
constexpr double DegToMil(double value) {
  return MilT(DegreesT(value)).Value();
}

// Energy
constexpr double JToFtLbs(double value) {
  return FtLbsT(JouleT(value)).Value();
}
constexpr double FtLbsToJ(double value) {
  return JouleT(FtLbsT(value)).Value();
}

// Length
constexpr double MtoYd(double value) {
  return YardT(FeetT(MeterT(value))).Value();
}
constexpr double YdToFt(double value) { return FeetT(YardT(value)).Value(); }
constexpr double MToFt(double value) { return FeetT(MeterT(value)).Value(); }
constexpr double FtToIn(double value) { return InchT(FeetT(value)).Value(); }
constexpr double MmToIn(double value) { return InchT(MmT(value)).Value(); }
constexpr double CmToIn(double value) { return InchT(CmT(value)).Value(); }
constexpr double YdToM(double value) {
  return MeterT(FeetT(YardT(value))).Value();
}
constexpr double FtToM(double value) { return MeterT(FeetT(value)).Value(); }
constexpr double FtToYd(double value) { return YardT(FeetT(value)).Value(); }
constexpr double InToMm(double value) {
  return MmT(FeetT(InchT(value))).Value();
}
constexpr double InToCm(double value) {
  return CmT(FeetT(InchT(value))).Value();
}
constexpr double InToFt(double value) { return FeetT(InchT(value)).Value(); }

// Pressure
constexpr double PaToInHg(double value) { return InHgT(PaT(value)).Value(); }
constexpr double MbarToInHg(double value) {
  return InHgT(MbarT(value)).Value();
}
constexpr double PsiToInHg(double value) { return InHgT(PsiT(value)).Value(); }

// Mass
constexpr double LbsToGrain(double value) {
  return GrainT(LbsT(value)).Value();
}
constexpr double GToGrain(double value) {
  return GrainT(LbsT(GramT(value))).Value();
}
constexpr double KgToGrain(double value) {
  return GrainT(LbsT(KgT(value))).Value();
}

// Sectional Density / Ballistic Coefficient
constexpr double KgSqMToPmsi(double value) {
  return PmsiT(KgsmT(value)).Value();
}

// Speed
constexpr double FpsToMps(double value) { return MpsT(FpsT(value)).Value(); }
constexpr double MpsToFps(double value) { return FpsT(MpsT(value)).Value(); }
constexpr double KphToMph(double value) {
  return MphT(FpsT(KphT(value))).Value();
}
constexpr double KnToMph(double value) {
  return MphT(FpsT(KnT(value))).Value();
}

// Time
constexpr double MsToS(double value) { return SecT(MsecT(value)).Value(); }
constexpr double UsToS(double value) { return SecT(UsecT(value)).Value(); }
constexpr double SToMs(double value) { return MsecT(SecT(value)).Value(); }
constexpr double SToUs(double value) { return UsecT(SecT(value)).Value(); }

// Temperature
constexpr double DegCToDegF(double value) {
  return DegFT(DegCT(value)).Value();
}

}  // namespace
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
