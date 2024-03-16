// This file is a part of lob, an exterior ballistics calculation library
// Copyright (c) 2024  Joel Benway
// Please see end of file for extended copyright information

#include "lob/lob.hpp"

#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <utility>

#include "calc.hpp"
#include "cartesian.hpp"
#include "constants.hpp"
#include "eng_units.hpp"
#include "ode.hpp"

namespace lob {

class Lob::Impl {
  friend class Lob::Builder;
  friend class Lob;
  static constexpr auto kNaN = std::numeric_limits<double>::quiet_NaN();

  FeetT altitude_ft_{kNaN};
  RadiansT angle_cw_from_north_rad_{kNaN};
  AtmosphereReferenceT atmosphere_reference_{
      AtmosphereReferenceT::kArmyStandardMetro};
  PmsiT ballistic_coefficent_psi_{kNaN};
  InHgT barometric_pressure_in_hg_{kNaN};
  InchT diameter_{kNaN};
  FpsT initial_velocity_fps_{kNaN};
  double lattitude_degrees_{kNaN};
  InchT length_inch_{kNaN};
  FeetT limit_max_distance_ft_{kNaN};
  FtLbsT limit_min_energy_ft_lbs_{kNaN};
  SecT limit_time_of_flight_sec_{kNaN};
  LbsT mass_lbs_{kNaN};
  double optic_adjustment_per_click_{kNaN};
  double optic_adjustment_per_rev_{kNaN};
  AdjustmentT optic_adjustment_units_{AdjustmentT::kMoa};
  FeetT optic_height_{kNaN};
  double relative_humidity_percent_{kNaN};
  PmsiT sectional_density_psi_{kNaN};
  double stability_factor_{kNaN};
  RadiansT target_angle_rad_{kNaN};
  FeetT target_distance_ft_{kNaN};
  DegFT temperature_deg_f_{kNaN};
  InchPerTwistT twist_inches_per_turn_{kNaN};
  RadiansT wind_approach_angle_rad_{kNaN};
  FpsT wind_speed_fps_{kNaN};
  RadiansT zero_angle_rad_{kNaN};
  FeetT zero_distance_ft_{kNaN};
  FeetT zero_impact_height_{kNaN};

  LbsPerCuFtT air_density_{kNaN};
  FpsT local_speed_of_sound_{kNaN};
  double coefficent_of_drag_coefficent_{kNaN};
  const std::array<float, kTableSize>* pdrag_coefficent_lut_{
      &kG1DragCoefficents};
  Lob::Solution solution_{};

  void ValidateBuild() const;
  void InitializeMembers();
  double GetCoefficentOfDrag(FpsT speed) const;
  void SolveStep(SpvT* ps, SecT* pt) const;
  RadiansT ZeroAngleSearch();
  void Solve();
};  // class Lob::Impl

Lob::Lob() : pimpl_{new Impl} {}

Lob::Lob(const Lob& other) : pimpl_{new Impl(*other.pimpl_)} {}

Lob::Lob(Lob&& other) noexcept = default;

Lob& Lob::operator=(Lob rhs) {
  if (this != &rhs) {
    *pimpl_ = *rhs.pimpl_;
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

Lob::Builder& Lob::Builder::AltitudeFt(double value) {
  plob_->pimpl_->altitude_ft_ = FeetT(value);
  return *this;
}

Lob::Builder& Lob::Builder::AltitudeM(double value) {
  plob_->pimpl_->altitude_ft_ = MeterT(value);
  return *this;
}

Lob::Builder& Lob::Builder::BallisticCoefficentPsi(double value) {
  plob_->pimpl_->ballistic_coefficent_psi_ = PmsiT(value);
  return *this;
}

Lob::Builder& Lob::Builder::BallisticCoefficentKgSqM(double value) {
  plob_->pimpl_->ballistic_coefficent_psi_ = KgsmT(value);
  return *this;
}

Lob::Builder& Lob::Builder::BarometricPressureInHg(double value) {
  plob_->pimpl_->barometric_pressure_in_hg_ = InHgT(value);
  return *this;
}

Lob::Builder& Lob::Builder::BarometricPressurePa(double value) {
  plob_->pimpl_->barometric_pressure_in_hg_ = PaT(value);
  return *this;
}

Lob::Builder& Lob::Builder::BarometricPressureMBar(double value) {
  plob_->pimpl_->barometric_pressure_in_hg_ = MillibarT(value);
  return *this;
}

Lob::Builder& Lob::Builder::BarometricPressurePsi(double value) {
  plob_->pimpl_->barometric_pressure_in_hg_ = PsiT(value);
  return *this;
}

Lob::Builder& Lob::Builder::BCAtmosphere(AtmosphereReferenceT type) {
  plob_->pimpl_->atmosphere_reference_ = type;
  return *this;
}

Lob::Builder& Lob::Builder::BCDragFunction(DragFunctionT type) {
  switch (type) {
    case DragFunctionT::kG1: {
      plob_->pimpl_->pdrag_coefficent_lut_ = &kG1DragCoefficents;
      break;
    }
    case DragFunctionT::kG2: {
      plob_->pimpl_->pdrag_coefficent_lut_ = &kG2DragCoefficents;
      break;
    }
    case DragFunctionT::kG5: {
      plob_->pimpl_->pdrag_coefficent_lut_ = &kG5DragCoefficents;
      break;
    }
    case DragFunctionT::kG6: {
      plob_->pimpl_->pdrag_coefficent_lut_ = &kG6DragCoefficents;
      break;
    }
    case DragFunctionT::kG7: {
      plob_->pimpl_->pdrag_coefficent_lut_ = &kG7DragCoefficents;
      break;
    }
    case DragFunctionT::kG8: {
      plob_->pimpl_->pdrag_coefficent_lut_ = &kG8DragCoefficents;
      break;
    }
    default: {
      break;
    }
  }
  return *this;
}

Lob::Builder& Lob::Builder::DiameterInch(double value) {
  plob_->pimpl_->diameter_ = InchT(value);
  return *this;
}

Lob::Builder& Lob::Builder::DiameterMm(double value) {
  plob_->pimpl_->diameter_ = MmT(value);
  return *this;
}

Lob::Builder& Lob::Builder::InitialVelocityFps(double value) {
  plob_->pimpl_->initial_velocity_fps_ = FpsT(value);
  return *this;
}

Lob::Builder& Lob::Builder::InitialVelocityMps(double value) {
  plob_->pimpl_->initial_velocity_fps_ = MpsT(value);
  return *this;
}

Lob::Builder& Lob::Builder::LengthInch(double value) {
  plob_->pimpl_->length_inch_ = InchT(value);
  return *this;
}

Lob::Builder& Lob::Builder::LengthMm(double value) {
  plob_->pimpl_->length_inch_ = MmT(value);
  return *this;
}

Lob::Builder& Lob::Builder::LimitMaxDistanceFt(double value) {
  plob_->pimpl_->limit_max_distance_ft_ = FeetT(value);
  return *this;
}

Lob::Builder& Lob::Builder::LimitMaxDistanceM(double value) {
  plob_->pimpl_->limit_max_distance_ft_ = MeterT(value);
  return *this;
}

Lob::Builder& Lob::Builder::LimitMaxDistanceYds(double value) {
  plob_->pimpl_->limit_max_distance_ft_ = YardT(value);
  return *this;
}

Lob::Builder& Lob::Builder::LimitMinEnergyFtLbs(double value) {
  plob_->pimpl_->limit_min_energy_ft_lbs_ = FtLbsT(value);
  return *this;
}

Lob::Builder& Lob::Builder::LimitMinEnergyJ(double value) {
  plob_->pimpl_->limit_min_energy_ft_lbs_ = JouleT(value);
  return *this;
}

Lob::Builder& Lob::Builder::LimitTimeOfFlightSec(double value) {
  plob_->pimpl_->limit_time_of_flight_sec_ = SecT(value);
  return *this;
}

Lob::Builder& Lob::Builder::MassGrains(double value) {
  plob_->pimpl_->mass_lbs_ = GrainT(value);
  return *this;
}

Lob::Builder& Lob::Builder::MassGrams(double value) {
  plob_->pimpl_->mass_lbs_ = GramT(value);
  return *this;
}

Lob::Builder& Lob::Builder::MassKg(double value) {
  plob_->pimpl_->mass_lbs_ = KgT(value);
  return *this;
}

Lob::Builder& Lob::Builder::MassLbs(double value) {
  plob_->pimpl_->mass_lbs_ = LbsT(value);
  return *this;
}

Lob::Builder& Lob::Builder::OpticHeightInches(double value) {
  plob_->pimpl_->optic_height_ = InchT(value);
  return *this;
}

Lob::Builder& Lob::Builder::OpticHeightMm(double value) {
  plob_->pimpl_->optic_height_ = MmT(value);
  return *this;
}

Lob::Builder& Lob::Builder::OpticAdjustments(AdjustmentT type) {
  plob_->pimpl_->optic_adjustment_units_ = type;
  return *this;
}

Lob::Builder& Lob::Builder::OpticAdjustmentsPerClick(double value) {
  plob_->pimpl_->optic_adjustment_per_click_ = value;
  return *this;
}

Lob::Builder& Lob::Builder::OpticAdjustmentsPerRevolution(double value) {
  plob_->pimpl_->optic_adjustment_per_rev_ = value;
  return *this;
}

Lob::Builder& Lob::Builder::RelativeHumidityPercent(double value) {
  plob_->pimpl_->relative_humidity_percent_ = value;
  return *this;
}

Lob::Builder& Lob::Builder::SectionalDensityPsi(double value) {
  plob_->pimpl_->sectional_density_psi_ = PmsiT(value);
  return *this;
}

Lob::Builder& Lob::Builder::SectionalDensityKgSqM(double value) {
  plob_->pimpl_->sectional_density_psi_ = KgsmT(value);
  return *this;
}

Lob::Builder& Lob::Builder::TargetAngleDeg(double value) {
  plob_->pimpl_->target_angle_rad_ = DegreesT(value);
  return *this;
}

Lob::Builder& Lob::Builder::TargetDistanceFt(double value) {
  plob_->pimpl_->target_distance_ft_ = FeetT(value);
  return *this;
}

Lob::Builder& Lob::Builder::TargetDistanceYds(double value) {
  plob_->pimpl_->target_distance_ft_ = YardT(value);
  return *this;
}

Lob::Builder& Lob::Builder::TargetDistanceM(double value) {
  plob_->pimpl_->target_distance_ft_ = MeterT(value);
  return *this;
}

Lob::Builder& Lob::Builder::TemperatureDegF(double value) {
  plob_->pimpl_->temperature_deg_f_ = DegFT(value);
  return *this;
}

Lob::Builder& Lob::Builder::TemperatureDegC(double value) {
  plob_->pimpl_->temperature_deg_f_ = DegCT(value);
  return *this;
}

Lob::Builder& Lob::Builder::TwistInchesPerTurn(double value) {
  plob_->pimpl_->twist_inches_per_turn_ = InchPerTwistT(value);
  return *this;
}

Lob::Builder& Lob::Builder::TwistMmPerTurn(double value) {
  plob_->pimpl_->twist_inches_per_turn_ = MmPerTwistT(value);
  return *this;
}

Lob::Builder& Lob::Builder::WindApproachAngle(ClockAngleT value) {
  constexpr int kDegreesPerClockNumber = kDegreesPerTurn / 12;
  plob_->pimpl_->wind_approach_angle_rad_ =
      DegreesT(kDegreesPerClockNumber * static_cast<uint8_t>(value));
  return *this;
}

Lob::Builder& Lob::Builder::WindApproachAngleCWFromHeadwindDeg(int32_t value) {
  constexpr int32_t kDepartingWindConversion = kDegreesPerTurn / 2;
  constexpr int32_t kQuarterTurn = kDegreesPerTurn / 4;

  int32_t angle = value + kDepartingWindConversion;

  while (angle > 0) {
    angle -= kDegreesPerTurn;
  }

  while (angle < -kDegreesPerTurn) {
    angle += kDegreesPerTurn;
  }

  angle = -1 * angle + kQuarterTurn;

  while (angle >= kDegreesPerTurn) {
    angle -= kDegreesPerTurn;
  }

  assert(angle >= 0);
  assert(angle < kDegreesPerTurn);

  plob_->pimpl_->wind_approach_angle_rad_ = DegreesT(angle);

  return *this;
}

Lob::Builder& Lob::Builder::WindSpeedFps(double value) {
  plob_->pimpl_->wind_speed_fps_ = FpsT(value);
  return *this;
}

Lob::Builder& Lob::Builder::WindSpeedMph(double value) {
  plob_->pimpl_->wind_speed_fps_ = MphT(value);
  return *this;
}

Lob::Builder& Lob::Builder::WindSpeedMps(double value) {
  plob_->pimpl_->wind_speed_fps_ = MpsT(value);
  return *this;
}

Lob::Builder& Lob::Builder::WindSpeedKph(double value) {
  plob_->pimpl_->wind_speed_fps_ = KphT(value);
  return *this;
}

Lob::Builder& Lob::Builder::WindSpeedKn(double value) {
  plob_->pimpl_->wind_speed_fps_ = KnT(value);
  return *this;
}

Lob::Builder& Lob::Builder::ZeroAngleMOA(double value) {
  plob_->pimpl_->zero_angle_rad_ = MoaT(value);
  return *this;
}

Lob::Builder& Lob::Builder::ZeroDistanceFt(double value) {
  plob_->pimpl_->zero_distance_ft_ = FeetT(value);
  return *this;
}

Lob::Builder& Lob::Builder::ZeroDistanceYds(double value) {
  plob_->pimpl_->zero_distance_ft_ = YardT(value);
  return *this;
}

Lob::Builder& Lob::Builder::ZeroDistanceM(double value) {
  plob_->pimpl_->zero_distance_ft_ = MeterT(value);
  return *this;
}

Lob::Builder& Lob::Builder::ZeroImpactHeightInches(double value) {
  plob_->pimpl_->zero_impact_height_ = InchT(value);
  return *this;
}

Lob::Builder& Lob::Builder::ZeroImpactHeightMm(double value) {
  plob_->pimpl_->zero_impact_height_ = MmT(value);
  return *this;
}

std::unique_ptr<Lob> Lob::Builder::Build() {
  plob_->pimpl_->ValidateBuild();
  plob_->pimpl_->InitializeMembers();
  return std::move(plob_);
}

void Lob::Impl::ValidateBuild() const {
  assert(!std::isnan(ballistic_coefficent_psi_));
  assert(!std::isnan(initial_velocity_fps_));
  assert((!std::isnan(diameter_) && !std::isnan(mass_lbs_)) ||
         !std::isnan(sectional_density_psi_));
  assert(!std::isnan(zero_distance_ft_) || !std::isnan(zero_angle_rad_));
  assert(!std::isnan(target_distance_ft_));
}

void Lob::Impl::InitializeMembers() {
  const double kDefaultOpticAdjustmentPerClick = 0.25;
  const FeetT kDefaultOpticHeight = InchT(1.5);

  if (std::isnan(zero_impact_height_)) {
    zero_impact_height_ = FeetT(0.0);
  }

  if (std::isnan(target_angle_rad_)) {
    target_angle_rad_ = DegreesT(0.0);
  }

  if (std::isnan(sectional_density_psi_) && !std::isnan(mass_lbs_) &&
      !std::isnan(diameter_)) {
    sectional_density_psi_ =
        PmsiT(mass_lbs_.Value() / std::pow(diameter_.Value(), 2));
  }

  if (atmosphere_reference_ == AtmosphereReferenceT::kArmyStandardMetro) {
    ballistic_coefficent_psi_ *= kArmyToIcaoBcConversionFactor;
    atmosphere_reference_ = AtmosphereReferenceT::kIcao;
  }

  if (std::isnan(optic_height_)) {
    optic_height_ = kDefaultOpticHeight;
  }

  if (std::isnan(optic_adjustment_per_click_)) {
    optic_adjustment_per_click_ = kDefaultOpticAdjustmentPerClick;
  }

  if (!std::isnan(altitude_ft_)) {
    if (std::isnan(temperature_deg_f_)) {
      temperature_deg_f_ = CalculateTemperatureAtAltitude(altitude_ft_);
    }
    if (std::isnan(barometric_pressure_in_hg_)) {
      barometric_pressure_in_hg_ = BarometricFormula(altitude_ft_);
    }
  }

  if (std::isnan(temperature_deg_f_)) {
    temperature_deg_f_ = DegFT(kIsaSeaLevelDegF);
  }

  if (std::isnan(barometric_pressure_in_hg_)) {
    barometric_pressure_in_hg_ = InHgT(kIsaSeaLevelPressureInHg);
  }

  if (std::isnan(relative_humidity_percent_)) {
    relative_humidity_percent_ = kIsaSeaLevelHumidityPercent;
  }

  if (std::isnan(wind_approach_angle_rad_)) {
    wind_approach_angle_rad_ = DegreesT(0);
  }

  if (std::isnan(wind_speed_fps_)) {
    wind_speed_fps_ = FpsT(0);
  }

  const auto kWaterVaporSaturationPressureInHg =
      CalculateWaterVaporSaturationPressure(temperature_deg_f_);

  air_density_ = LbsPerCuFtT(
      CalcualteAirDensityRatio(barometric_pressure_in_hg_, temperature_deg_f_) *
      CalculateAirDensityRatioHumidityCorrection(
          relative_humidity_percent_, kWaterVaporSaturationPressureInHg) *
      kIsaSeaLevelAirDensityLbsPerCuFt);

  local_speed_of_sound_ =
      CalculateSpeedOfSoundInAir(temperature_deg_f_) *
      CalculateSpeedOfSoundHumidityCorrection(
          relative_humidity_percent_, kWaterVaporSaturationPressureInHg);

  const double kSqInPerSqFt = (InchT(FeetT(1)) * InchT(FeetT(1))).Value();
  constexpr uint8_t kClangTidyPleaserEight = 8;

  coefficent_of_drag_coefficent_ = air_density_.Value() * kPi /
                                   (ballistic_coefficent_psi_.Value() *
                                    kSqInPerSqFt * kClangTidyPleaserEight);

  if (std::isnan(zero_angle_rad_)) {
    zero_angle_rad_ = ZeroAngleSearch();
  }

  if (!std::isnan(length_inch_) && !std::isnan(twist_inches_per_turn_) &&
      !std::isnan(mass_lbs_) && !std::isnan(diameter_)) {
    const auto kFtp = CalculateMillerTwistRuleCorrectionFactor(air_density_);
    stability_factor_ =
        kFtp * CalculateMillerTwistRuleStabilityFactor(
                   diameter_, GrainT(mass_lbs_), length_inch_,
                   twist_inches_per_turn_, initial_velocity_fps_);
  }
}

double Lob::Impl::GetCoefficentOfDrag(const FpsT speed) const {
  const auto kMachSpeed = (speed / local_speed_of_sound_).Value();
  const double kCd = LobLerp(kMachValues, *pdrag_coefficent_lut_, kMachSpeed);
  return coefficent_of_drag_coefficent_ * kCd;
}

void Lob::Impl::SolveStep(SpvT* ps, SecT* pt) const {
  const CartesianT<FpsSqT> kGravity(
      FpsSqT(kStandardGravity * -1 * std::sin(target_angle_rad_.Value())),
      FpsSqT(kStandardGravity * -1 * std::cos(target_angle_rad_.Value())),
      FpsSqT(0.0));

  const CartesianT<FpsT> kWind(
      FpsT(wind_speed_fps_ * std::sin(wind_approach_angle_rad_.Value())),
      FpsT(0.0),
      FpsT(wind_speed_fps_ * std::cos(wind_approach_angle_rad_.Value())));

  // This variable is marked volatile because it's only read inside a closure
  // which fools static analyzers into unused warnings.
  volatile double coefficent_of_drag = GetCoefficentOfDrag(ps->V().Magnitude());

  auto ds_dt = [kGravity, kWind, coefficent_of_drag](double t,
                                                     const SpvT& s) -> SpvT {
    static_cast<void>(t);  // t is unused
    const CartesianT<FeetT> kPosition(FeetT(s.V().X().Value()),
                                      FeetT(s.V().Y().Value()),
                                      FeetT(s.V().Z().Value()));
    const auto kScalarVelocity = (s.V() - kWind).Magnitude();
    CartesianT<FpsT> velocity =
        (s.V() - kWind) * FpsT(-1 * coefficent_of_drag) * kScalarVelocity;
    velocity.X(velocity.X() + kGravity.X().Value());
    velocity.Y(velocity.Y() + kGravity.Y().Value());
    return SpvT{kPosition, velocity};
  };

  const SecT kDt(1 / ps->V().Magnitude().Value());
  *ps = HeunStep(0.0, *ps, kDt.Value(), ds_dt);
  *pt += kDt;

  coefficent_of_drag = GetCoefficentOfDrag(ps->V().Magnitude());
}

RadiansT Lob::Impl::ZeroAngleSearch() {
  const RadiansT kZeroAngleError = MoaT(0.1);
  const RadiansT kMaxZeroAngle = DegreesT(45);
  const RadiansT kMinZeroAngle = DegreesT(-45.0);
  RadiansT high_angle = kMaxZeroAngle;
  RadiansT low_angle = kMinZeroAngle;

  while (high_angle - low_angle > kZeroAngleError) {
    const RadiansT kZeroAngle = (low_angle + high_angle) / 2;

    SpvT s(
        CartesianT<FeetT>(FeetT(0.0)),
        CartesianT<FpsT>(initial_velocity_fps_ * std::cos(kZeroAngle).Value(),
                         initial_velocity_fps_ * std::sin(kZeroAngle).Value(),
                         FpsT(0.0)));

    SecT t(0.0);

    while (s.P().X() < zero_distance_ft_) {
      SolveStep(&s, &t);
    }

    if (s.P().Y() - optic_height_ > zero_impact_height_) {
      high_angle = kZeroAngle;
    } else {
      low_angle = kZeroAngle;
    }
  }
  return (low_angle + high_angle) / 2;
}

void Lob::Impl::Solve() {
  SpvT s(CartesianT<FeetT>(FeetT(0.0)),
         CartesianT<FpsT>(
             initial_velocity_fps_ * std::cos(zero_angle_rad_).Value(),
             initial_velocity_fps_ * std::sin(zero_angle_rad_).Value(),
             FpsT(0.0)));
  SecT t(0.0);
  while (s.P().X() < target_distance_ft_) {
    SolveStep(&s, &t);

    if (t > limit_time_of_flight_sec_) {
      break;
    }
    if (limit_min_energy_ft_lbs_ >
        CalculateKineticEnergy(s.V().Magnitude(), mass_lbs_)) {
      break;
    }
  }
}

float Lob::GetStabilityFactor() const {
  if (std::isnan(pimpl_->stability_factor_)) {
    return 0.0F;
  }
  return static_cast<float>(pimpl_->stability_factor_);
}

float Lob::GetZeroAngleMOA() const {
  return static_cast<float>(MoaT(pimpl_->zero_angle_rad_).Value());
}

// Lob::Solution Lob::Solve() const {}

// size_t Lob::Solve(Lob::Solution* psolution, size_t length) const {}

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
