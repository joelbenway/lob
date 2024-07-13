// This file is a part of lob, an exterior ballistics calculation library
// Copyright (c) 2024  Joel Benway
// Please see end of file for extended copyright information

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

#include "lob/lob_export.hpp"

namespace lob {

enum class DragFunctionT : uint8_t { kG1, kG2, kG5, kG6, kG7, kG8 };
enum class AtmosphereReferenceT : uint8_t { kArmyStandardMetro, kIcao };
enum class AdjustmentT : uint8_t { kMoa, kMils };
enum class ClockAngleT : uint8_t {
  kIX = 0U,
  kVIII,
  kVII,
  kVI,
  kV,
  kIV,
  kIII,
  kII,
  kI,
  kXII,
  kXI,
  kX
};

class LOB_EXPORT Lob {
 public:
  Lob(Lob&& other) noexcept;
  Lob& operator=(const Lob& rhs);
  Lob& operator=(Lob&& rhs) noexcept;
  ~Lob();

  class Builder {
   public:
    Builder() : plob_{new Lob} {}
    explicit Builder(const Lob& lob) : plob_{new Lob(lob)} {}

    /**
     * @brief Sets the ballistic coefficient in pounds per square inch (PSI).
     * @details Projectile manufacturers don't always publish BC with units in
     * which case, PSI is a safe assumption.
     * @note Ballistic Coefficient is required for a well-formed solution.
     * @param value The ballistic coefficient value in PSI.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& BallisticCoefficentPsi(double value);

    /**
     * @brief Sets the ballistic coefficient in kilograms per square meter
     * (kg/m²).
     * @details Projectile manufacturers don't always publish BC with units in
     * which case, PSI is a safe assumption.
     * @note Ballistic Coefficient is required for a well-formed solution.
     * @param value The ballistic coefficient value in kg/m².
     * @return Reference to the Builder object for method chaining.
     */
    Builder& BallisticCoefficentKgSqM(double value);

    /**
     * @brief Sets the atmosphere reference type for the ballistic coefficient.
     * @details When projectile manufacturers publish their BC values, they do
     * so for a reference set of atmospheric conditions. The two most common
     * are Army Standard Metro and Icao. When in doubt, here is a guide:
     * Barnes: Army Standard Metro
     * Berger: ICAO
     * GI APG: ICAO
     * Hornady: Army Standard Metro
     * Nosler: ICAO
     * Lapua: ICAO
     * Sierra: Army Standard Metro
     * Speer: ICAO
     * Winchester: Army Standard Metro
     * @note If no atmospheric reference is specified, Army Standard Metro is
     * assumed.
     * @param type The atmosphere reference type.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& BCAtmosphere(AtmosphereReferenceT type);

    /**
     * @brief Sets the drag function for the ballistic coefficient.
     * @details BC's are calculated in the context of a specific drag function.
     * Of these the Gavre models (G1, G2, etc) are the most widely adopted and
     * one should be published alongside BC.
     * @note If no drag function is specified, G1 is used.
     * @param type The drag function type.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& BCDragFunction(DragFunctionT type);

    /**
     * @brief Sets the projectile diameter in inches.
     * @note Either sectional density or bullet diameter and mass are required
     * for a well-formed solution.
     * @param value The diameter value in inches.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& DiameterInch(double value);

    /**
     * @brief Sets the projectile diameter in millimeters.
     * @note Either sectional density or bullet diameter and mass are required
     * for a well-formed solution.
     * @param value The diameter value in millimeters.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& DiameterMm(double value);

    /**
     * @brief Sets the projectile length in inches.
     * @note Length is only used for calculating stability factor and has no
     * bearing on the ballistic solution.
     * @param value The length value in inches.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& LengthInch(double value);

    /**
     * @brief Sets the projectile length in millimeters.
     * @note Length is only used for calculating stability factor and has no
     * bearing on the ballistic solution.
     * @param value The length value in millimeters.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& LengthMm(double value);

    /**
     * @brief Sets the projectile mass in grains.
     * @note Either sectional density or bullet diameter and mass are required
     * for a well-formed solution.
     * @param value The mass value in grains.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& MassGrains(double value);

    /**
     * @brief Sets the projectile mass in pounds.
     * @note Either sectional density or bullet diameter and mass are required
     * for a well-formed solution. Without mass, energy will not be calculated.
     * @param value The mass value in pounds.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& MassLbs(double value);

    /**
     * @brief Sets the projectile mass in grams.
     * @note Either sectional density or bullet diameter and mass are required
     * for a well-formed solution. Without mass, energy will not be calculated.
     * @param value The mass value in grams.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& MassGrams(double value);

    /**
     * @brief Sets the projectile mass in kilograms.
     * @note Either sectional density or bullet diameter and mass are required
     * for a well-formed solution. Without mass, energy will not be calculated.
     * @param value The mass value in kilograms.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& MassKg(double value);

    /**
     * @brief Sets the sectional density in pounds per square inch (PSI).
     * @note Either sectional density or bullet diameter and mass are required
     * for a well-formed solution.
     * @param value The sectional density value in PSI.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& SectionalDensityPsi(double value);

    /**
     * @brief Sets the sectional density in kilograms per square meter (kg/m²).
     * @note Either sectional density or bullet diameter and mass are required
     * for a well-formed solution.
     * @param value The sectional density value in kg/m².
     * @return Reference to the Builder object for method chaining.
     */
    Builder& SectionalDensityKgSqM(double value);

    /**
     * @brief Sets the initial velocity in feet per second (fps).
     * @note Initial velocity is required for a well-formed solution.
     * @param value The initial velocity value in fps.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& InitialVelocityFps(double value);

    /**
     * @brief Sets the initial velocity in meters per second (m/s).
     * @note Initial velocity is required for a well-formed solution.
     * @param value The initial velocity value in m/s.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& InitialVelocityMps(double value);

    /**
     * @brief Sets the units that optic adjustments will be given in.
     * @param type The adjustment type.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& OpticAdjustments(AdjustmentT type);

    /**
     * @brief Sets the optic height in inches.
     * @param value The optic height value in inches.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& OpticHeightInches(double value);

    /**
     * @brief Sets the optic height in millimeters.
     * @param value The optic height value in millimeters.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& OpticHeightMm(double value);

    /**
     * @brief Sets the barrel twist rate in inches per turn.
     * @note This is only used for calculating stability factor and has no
     * bearing on the ballistic solution.
     * @param value The twist rate value in inches per turn.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& TwistInchesPerTurn(double value);

    /**
     * @brief Sets the barrel twist rate in millimeters per turn.
     * @note This is only used for calculating stability factor and has no
     * bearing on the ballistic solution.
     * @param value The twist rate value in millimeters per turn.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& TwistMmPerTurn(double value);

    /**
     * @brief Sets the zero angle in Minutes of Angle (MOA).
     * @details This value represents the angle between the line of sight and
     * projectile launch angle of a zeroed rifle. This is not a normal thing for
     * a marksman to know about their rifle.
     * @note Either Zero angle or zero distance is required for a well-formed
     * solution. If angle is provided it does not have to be calculated speeding
     * up the solution.
     * @param value The zero angle value in MOA.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& ZeroAngleMOA(double value);

    /**
     * @brief Sets the zero distance in feet.
     * @note Either zero angle or zero distance is required for a well-formed
     * solution.
     * @param value The zero distance value in feet.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& ZeroDistanceFt(double value);

    /**
     * @brief Sets the zero distance in yards.
     * @note Either zero angle or zero distance is required for a well-formed
     * solution.
     * @param value The zero distance value in yards.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& ZeroDistanceYds(double value);

    /**
     * @brief Sets the zero distance in meters.
     * @note Either zero angle or zero distance is required for a well-formed
     * solution.
     * @param value The zero distance value in meters.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& ZeroDistanceM(double value);

    /**
     * @brief Sets the zero impact height in inches.
     * @details This is intended for those who use a zero such as the "3 inches
     * high at 100 yards" recommended by Jack O'Connor.
     * @note This can be omitted for setups with a traditional zero.
     * @param value The zero impact height value in inches.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& ZeroImpactHeightInches(double value);

    /**
     * @brief Sets the zero impact height in millimeters.
     * @details This is intended for those who use a zero such as the "3 inches
     * high at 100 yards" recommended by Jack O'Connor.
     * @note This can be omitted for setups with a traditional zero.
     * @param value The zero impact height value in millimeters.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& ZeroImpactHeightMm(double value);

    /**
     * @brief Sets the altitude in feet.
     * @details Altitude is used to estimate temperature and barometric
     * pressure. It's use is limited to times when it is impractical to measure
     * these directly.
     * @note There's no reason to specify this if providing temperature and
     * barometric pressure.
     * @param value The altitude value in feet.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& AltitudeFt(double value);

    /**
     * @brief Sets the altitude in meters.
     * @details Altitude is used to estimate temperature and barometric
     * pressure. It's use is limited to times when it is impractical to measure
     * these directly.
     * @note There's no reason to specify this if providing temperature and
     * barometric pressure.
     * @param value The altitude value in meters.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& AltitudeM(double value);

    /**
     * @brief Sets the barometric pressure in inches of mercury (inHg).
     * @param value The barometric pressure value in inHg.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& BarometricPressureInHg(double value);

    /**
     * @brief Sets the barometric pressure in pascals (Pa).
     * @param value The barometric pressure value in Pa.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& BarometricPressurePa(double value);

    /**
     * @brief Sets the barometric pressure in millibars (mbar).
     * @param value The barometric pressure value in mbar.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& BarometricPressureMBar(double value);

    /**
     * @brief Sets the barometric pressure in pounds per square inch (PSI).
     * @param value The barometric pressure value in PSI.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& BarometricPressurePsi(double value);

    /**
     * @brief Sets the relative humidity as a percentage.
     * @param value The relative humidity value as a percentage.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& RelativeHumidityPercent(double value);

    /**
     * @brief Sets the temperature in degrees Fahrenheit.
     * @param value The temperature value in degrees Fahrenheit.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& TemperatureDegF(double value);

    /**
     * @brief Sets the temperature in degrees Celsius.
     * @param value The temperature value in degrees Celsius.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& TemperatureDegC(double value);

    /**
     * @brief Sets the wind approach angle using clock angle notation.
     * @details 12 o'clock would be a pure headwind.
     * @param value The wind approach angle as a ClockAngleT value.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& WindApproachAngle(ClockAngleT value);

    /**
     * @brief Sets the wind approach angle in degrees clockwise from headwind.
     * @details 0 or 360 would be a pure headwind, 90 would be right-to-left,
     * 180 would be a tailwind, and 270 would be a left-to-right wind.
     * @param value The wind approach angle in degrees.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& WindApproachAngleCWFromHeadwindDeg(int32_t value);

    /**
     * @brief Sets the wind speed in feet per second (fps).
     * @param value The wind speed value in fps.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& WindSpeedFps(double value);

    /**
     * @brief Sets the wind speed in miles per hour (mph).
     * @param value The wind speed value in mph.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& WindSpeedMph(double value);

    /**
     * @brief Sets the wind speed in meters per second (m/s).
     * @param value The wind speed value in m/s.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& WindSpeedMps(double value);

    /**
     * @brief Sets the wind speed in kilometers per hour (km/h).
     * @param value The wind speed value in km/h.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& WindSpeedKph(double value);

    /**
     * @brief Sets the wind speed in knots (kn).
     * @param value The wind speed value in knots.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& WindSpeedKn(double value);

    /**
     * @brief Sets the maximum distance in feet before the solver stops.
     * @param value The maximum distance limit value in feet.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& LimitMaxDistanceFt(double value);

    /**
     * @brief Sets the maximum distance in yards before the solver stops.
     * @param value The maximum distance limit value in yards.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& LimitMaxDistanceYds(double value);

    /**
     * @brief Sets the maximum distance in meters before the solver stops.
     * @param value The maximum distance limit value in meters.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& LimitMaxDistanceM(double value);

    /**
     * @brief Sets the minimum energy in foot-pounds before the solver stops.
     * @param value The minimum energy limit value in foot-pounds.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& LimitMinEnergyFtLbs(double value);

    /**
     * @brief Sets the minimum energy in joules before the solver stops.
     * @param value The minimum energy limit value in joules.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& LimitMinEnergyJ(double value);

    /**
     * @brief Sets the time of flight in seconds before the solver stops.
     * @param value The time of flight limit value in seconds.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& LimitTimeOfFlightSec(double value);

    /**
     * @brief Sets the size of the time step the solver uses when calculating a
     * solution.
     * @details The smaller the time step, the more precise the solution but at
     * the cost of speed. By default the solver uses a variable time step
     * intended to balance speed and accuracy.
     * @param value The time step size in microseconds.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& SolverStepSizeUsec(uint16_t value);

    /**
     * @brief Sets the angle from the shooter to the target in degrees.
     * @param value The target angle value in degrees.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& TargetAngleDeg(double value);

    /**
     * @brief Sets the target distance in feet.
     * @param value The target distance value in feet.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& TargetDistanceFt(double value);

    /**
     * @brief Sets the target distance in yards.
     * @param value The target distance value in yards.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& TargetDistanceYds(double value);

    /**
     * @brief Sets the target distance in meters.
     * @param value The target distance value in meters.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& TargetDistanceM(double value);

    /**
     * @brief Builds and returns a unique pointer to a Lob object.
     * @return std::unique_ptr<Lob> A unique pointer to the constructed Lob
     * object.
     */
    std::unique_ptr<Lob> Build();

   private:
    LOB_SUPPRESS_C4251
    std::unique_ptr<Lob> plob_{};
  };  // class Builder

  float GetStabilityFactor() const;
  float GetZeroAngleMOA() const;

  struct Solution {
    uint16_t range;
    uint16_t velocity;
    uint16_t energy;
    float elevation_distance;
    float elevation_adjustments;
    float windage_distance;
    float windage_adjustments;
    float time_of_flight;
  };  // struct Solution

  Solution Solve() const;
  size_t Solve(Solution* psolution, size_t length) const;

 private:
  Lob();
  Lob(const Lob& other);
  class Impl;
  const Impl* Pimpl() const;
  Impl* Pimpl();
  LOB_SUPPRESS_C4251
  std::unique_ptr<Impl> pimpl_;
};  // class Lob

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
