// This file is a part of lob, an exterior ballistics calculation library
// Copyright (c) 2024  Joel Benway
// Please see end of file for extended copyright information

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

#include "lob/lob_export.hpp"

namespace lob {

/**
 * @brief Gets the library version in major.minor.patch format.
 * @return Version string.
 */
const char* Version();

enum class DragFunctionT : uint8_t { kG1, kG2, kG5, kG6, kG7, kG8 };
enum class AtmosphereReferenceT : uint8_t { kArmyStandardMetro, kIcao };

enum class ClockAngleT : uint8_t {
  kIII = 0U,
  kII,
  kI,
  kXII,
  kXI,
  kX,
  kIX,
  kVIII,
  kVII,
  kVI,
  kV,
  kIV
};

/*
template <size_t SIZE>
struct Input {
  

};

struct Output {

};

bool Verify(const Input& in);

bool Solve(const Input& in, const Output& out);
*/

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
     * @brief Sets the ballistic coefficient in pounds mass per square inch
     * (PSI).
     * @details Projectile manufacturers don't always publish BC with units in
     * which case, PSI is a safe assumption.
     * @note Ballistic Coefficient is required for a well-formed solution.
     * @param value The ballistic coefficient value in PSI.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& BallisticCoefficentPsi(double value);

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
     * @note Projectile diameter is required for a well-formed solution.
     * @param value The diameter value in inches.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& DiameterInch(double value);

    /**
     * @brief Sets the projectile length in inches.
     * @note Length, along with twist rate is used for calculating stability
     * factor and accounting for the effects of gyroscopic spin drift on the
     * ballistic solution.
     * @param value The length value in inches.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& LengthInch(double value);

    /**
     * @brief Sets the projectile mass in grains.
     * @note Projectile mass is required for a well-formed solution.
     * @param value The mass value in grains.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& MassGrains(double value);

    /**
     * @brief Sets the initial velocity in feet per second (fps).
     * @note Initial velocity is required for a well-formed solution.
     * @param value The initial velocity value in fps.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& InitialVelocityFps(double value);

    /**
     * @brief Sets the optic height in inches.
     * @param value The optic height value in inches.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& OpticHeightInches(double value);

    /**
     * @brief Sets the barrel twist rate in inches per turn.
     * @note Twist rate, along with projectile length, are used for calculating
     * stability factor and accounting for the effects of gyroscopic spin drift
     * on the ballistic solution.
     * @param value The twist rate value in inches per turn.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& TwistInchesPerTurn(double value);

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
     * @brief Sets the zero distance in yards.
     * @note Either zero angle or zero distance is required for a well-formed
     * solution.
     * @param value The zero distance value in yards.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& ZeroDistanceYds(double value);

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
     * @brief Sets the altitude of the firing site in feet.
     * @details Altitude is used to estimate temperature and air pressure in the
     * absence of empirical data. If using pressure or temperature measured at
     * an elevation different from the firing site, altitude may be used to
     * adjust the measurements.
     * @note There is no reason to include this if providing temperature and air
     * pressure measured at the firing site.
     * @param value The altitude value in feet.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& AltitudeOfFiringSiteFt(double value);

    /**
     * @brief Sets the air pressure in inches of mercury (InHg).
     * @param value The air pressure value in inHg.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& AirPressureInHg(double value);

    /**
     * @brief Sets the altitude in feet of the site associated with a provided
     * air pressure measurement.
     * @details This is intended for use with air pressures measured or adjusted
     * to altitudes that differ from that of the firing site such as from a
     * weather station.
     * @note If this is omitted it will be assumed that air pressure was
     * measured at the firing site. If using air pressure adjusted for sea level
     * use a value of 0 feet.
     * @param value The altitude of the barometer site in feet.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& AltitudeOfBarometerFt(double value);

    /**
     * @brief Sets the temperature in degrees Fahrenheit.
     * @param value The temperature value in degrees Fahrenheit.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& TemperatureDegF(double value);

    /**
     * @brief Sets the altitude in feet of the site associated with a provided
     * temperature.
     * @details This is intended for use with temperatures measured or adjusted
     * to altitudes that differ from that of the firing site such as from a
     * weather station.
     * @note If this is omitted it will be assumed that temperature was measured
     * at the firing site. If using temperature adjusted for sea level use a
     * value of 0 feet.
     * @param value The altitude of the thermometer site in feet.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& AltitudeOfThermometerFt(double value);

    /**
     * @brief Sets the relative humidity at the firing site as a percentage.
     * @param value The relative humidity value as a percentage.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& RelativeHumidityPercent(double value);

    /**
     * @brief Sets the wind heading using clock angle notation.
     * @details 12 o'clock would be a pure tailwind. 6, a headwind.
     * @param value The wind heading as a ClockAngleT value.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& WindHeading(ClockAngleT value);

    /**
     * @brief Sets the wind heading in degrees clockwise from a pure tailwind.
     * @details 0 or 360 would be a pure tailwind, 90 would be left-to-right,
     * 180 would be a headwind, and 270 would be a right-to-left wind.
     * @param value The wind heading in degrees.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& WindHeadingDeg(double value);

    /**
     * @brief Sets the wind speed in feet per second (fps).
     * @param value The wind speed value in fps.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& WindSpeedFps(double value);

    /**
     * @brief Sets the wind speed in miles per hour. (mph).
     * @param value The wind speed value in mph.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& WindSpeedMph(double value);

    /**
     * @brief Sets the Azimuth angle of fire in degrees, measured clockwise from
     * North.
     * @details North is 0 degrees.
     * @note This value is used for accounting for the Coriolis effect in the
     * ballistic solution.
     * @param value The Azimuth angle of fire in degrees clockwise from North.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& AzimuthDeg(double value);

    /**
     * @brief Sets the latitude of the firing site.
     * @details Use negative values for the Southern hemisphere.
     * @note This value is used for accounting for the Coriolis effect in the
     * ballistic solution.
     * @param value The latitude in Degrees of the firing site.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& LatitudeDeg(double value);

    /**
     * @brief Sets the maximum distance in yards before the solver stops.
     * @param value The maximum distance limit value in yards.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& LimitMaxDistanceYds(double value);

    /**
     * @brief Sets the minimum energy in foot-pounds before the solver stops.
     * @param value The minimum energy limit value in foot-pounds.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& LimitMinEnergyFtLbs(double value);

    /**
     * @brief Sets the time of flight in seconds before the solver stops.
     * @param value The time of flight limit value in seconds.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& LimitTimeOfFlightSec(double value);

    /**
     * @brief Sets the size of the time step the solver uses when calculating a
     * solution.
     * @note The smaller the time step, the more precise the solution but at
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
     * @brief Sets the target distance in yards.
     * @param value The target distance value in yards.
     * @return Reference to the Builder object for method chaining.
     */
    Builder& TargetDistanceYds(double value);

    /**
     * @brief Builds and returns a unique pointer to a Lob object.
     * @return std::unique_ptr<Lob> A unique pointer to the constructed Lob
     * object.
     */
    std::unique_ptr<Lob> Build();

   private:
    LOB_SUPPRESS_C4251
    std::unique_ptr<Lob> plob_;
  };  // class Builder

  /**
   * @brief Gets the Air Density calculated in the built configuration. Units
   * are in pounds per cubic foot.
   * @return float Stability Factor.
   */
  float GetAirDensityLbsPerCuFt() const;

  /**
   * @brief Gets the local speed of sound for the built configuration. Units are
   * in feet per second.
   * @return float Stability Factor.
   */
  float GetSpeedOfSoundFps() const;

  /**
   * @brief Gets the Miller Stability Factor for the built configuration.
   * @return float Stability Factor.
   */
  float GetStabilityFactor() const;

  /**
   * @brief Gets the angle between the line of sight and launch trajectory
   * required to achieve zero. Angle is given in MOA.
   * @note Because the adjustments used to zero a rifle are dependent on
   * variables like environment, this zero angle is a way to preserve that
   * context for future calculations. There is also a calculation speed benefit
   * to supplying zero angle vs zero distance.
   * @return float zero angle in MOA.
   */
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

  /**
   * @brief Solves for the built configuration at specified points.
   * @param psolution a pointer to an array of solution objects of size length
   * that will be populated with data upon a successful solve.
   * @param pranges a pointer to an array of ranges of size length. These should
   * be ascending values. If nullptr is passed the solution ranges will simply
   * be increments of target distance divided by length.
   * @param length size of both psolution and pranges arrays.
   * @return size of the available length populated with solution data.
   */
  size_t Solve(Solution* psolution, const uint16_t* pranges,
               size_t length) const;

 private:
  Lob();
  Lob(const Lob& other);
  class Impl;
  const Impl* Pimpl() const;
  Impl* Pimpl();
  LOB_SUPPRESS_C4251
  std::unique_ptr<Impl> pimpl_;
};  // class Lob

/**
 * @brief Converts minutes of angle (MOA) to milliradians (MIL).
 * @param value Angle in MOA.
 * @return Equivalent angle in MIL.
 */
constexpr double MoaToMil(double value);

/**
 * @brief Converts minutes of angle (MOA) to degrees.
 * @param value Angle in MOA.
 * @return Equivalent angle in degrees.
 */
constexpr double MoaToDeg(double value);

/**
 * @brief Converts milliradians (MIL) to minutes of angle (MOA).
 * @param value Angle in MIL.
 * @return Equivalent angle in MOA.
 */
constexpr double MilToMoa(double value);

/**
 * @brief Converts milliradians (MIL) to degrees.
 * @param value Angle in MIL.
 * @return Equivalent angle in degrees.
 */
constexpr double MilToDeg(double value);

/**
 * @brief Converts degrees to minutes of angle (MOA).
 * @param value Angle in degrees.
 * @return Equivalent angle in MOA.
 */
constexpr double DegToMoa(double value);

/**
 * @brief Converts degrees to milliradians (MIL).
 * @param value Angle in degrees.
 * @return Equivalent angle in MIL.
 */
constexpr double DegToMil(double value);

/**
 * @brief Converts joules to foot-pounds.
 * @param value Energy in joules.
 * @return Equivalent energy in foot-pounds.
 */
constexpr double JToFtLbs(double value);

/**
 * @brief Converts foot-pounds to joules.
 * @param value Energy in foot-pounds.
 * @return Equivalent energy in joules.
 */
constexpr double FtLbsToJ(double value);

/**
 * @brief Converts meters to yards.
 * @param value Length in meters.
 * @return Equivalent length in yards.
 */
constexpr double MtoYd(double value);

/**
 * @brief Converts yards to feet.
 * @param value Length in yards.
 * @return Equivalent length in feet.
 */
constexpr double YdToFt(double value);

/**
 * @brief Converts meters to feet.
 * @param value Length in meters.
 * @return Equivalent length in feet.
 */
constexpr double MToFt(double value);

/**
 * @brief Converts feet to inches.
 * @param value Length in feet.
 * @return Equivalent length in inches.
 */
constexpr double FtToIn(double value);

/**
 * @brief Converts millimeters to inches.
 * @param value Length in millimeters.
 * @return Equivalent length in inches.
 */
constexpr double MmToIn(double value);

/**
 * @brief Converts centimeters to inches.
 * @param value Length in centimeters.
 * @return Equivalent length in inches.
 */
constexpr double CmToIn(double value);

/**
 * @brief Converts yards to meters.
 * @param value Length in yards.
 * @return Equivalent length in meters.
 */
constexpr double YdToM(double value);

/**
 * @brief Converts feet to meters.
 * @param value Length in feet.
 * @return Equivalent length in meters.
 */
constexpr double FtToM(double value);

/**
 * @brief Converts feet to yards.
 * @param value Length in feet.
 * @return Equivalent length in yards.
 */
constexpr double FtToYd(double value);

/**
 * @brief Converts inches to millimeters.
 * @param value Length in inches.
 * @return Equivalent length in millimeters.
 */
constexpr double InToMm(double value);

/**
 * @brief Converts inches to centimeters.
 * @param value Length in inches.
 * @return Equivalent length in centimeters.
 */
constexpr double InToCm(double value);

/**
 * @brief Converts inches to feet.
 * @param value Length in inches.
 * @return Equivalent length in feet.
 */
constexpr double InToFt(double value);

/**
 * @brief Converts pascals to inches of mercury.
 * @param value Pressure in pascals.
 * @return Equivalent pressure in inches of mercury.
 */
constexpr double PaToInHg(double value);

/**
 * @brief Converts millibars to inches of mercury.
 * @param value Pressure in millibars.
 * @return Equivalent pressure in inches of mercury.
 */
constexpr double MbarToInHg(double value);

/**
 * @brief Converts pounds per square inch (PSI) to inches of mercury.
 * @param value Pressure in PSI.
 * @return Equivalent pressure in inches of mercury.
 */
constexpr double PsiToInHg(double value);

/**
 * @brief Converts pounds to grains.
 * @param value Mass in pounds.
 * @return Equivalent mass in grains.
 */
constexpr double LbsToGrain(double value);

/**
 * @brief Converts grams to grains.
 * @param value Mass in grams.
 * @return Equivalent mass in grains.
 */
constexpr double GToGrain(double value);

/**
 * @brief Converts kilograms to grains.
 * @param value Mass in kilograms.
 * @return Equivalent mass in grains.
 */
constexpr double KgToGrain(double value);

/**
 * @brief Converts kilograms per square meter to pounds mass per square inch.
 * @param value Sectional density in Kg/m².
 * @return Equivalent sectional density in lb/in².
 */
constexpr double KgSqMToPmsi(double value);

/**
 * @brief Converts feet per second to meters per second.
 * @param value Speed in feet per second.
 * @return Equivalent speed in meters per second.
 */
constexpr double FpsToMps(double value);

/**
 * @brief Converts meters per second to feet per second.
 * @param value Speed in meters per second.
 * @return Equivalent speed in feet per second.
 */
constexpr double MpsToFps(double value);

/**
 * @brief Converts kilometers per hour to miles per hour.
 * @param value Speed in kilometers per hour.
 * @return Equivalent speed in miles per hour.
 */
constexpr double KphToMph(double value);

/**
 * @brief Converts Knots to miles per hour.
 * @param value Speed in Knots.
 * @return Equivalent speed in miles per hour.
 */
constexpr double KnToMph(double value);

/**
 * @brief Converts milliseconds to seconds.
 * @param value Time in milliseconds.
 * @return Equivalent time in seconds.
 */
constexpr double MsToS(double value);

/**
 * @brief Converts microseconds to seconds.
 * @param value Time in microseconds.
 * @return Equivalent time in seconds.
 */
constexpr double UsToS(double value);

/**
 * @brief Converts seconds to milliseconds.
 * @param value Time in seconds.
 * @return Equivalent time in milliseconds.
 */
constexpr double SToMs(double value);

/**
 * @brief Converts seconds to microseconds.
 * @param value Time in seconds.
 * @return Equivalent time in microseconds.
 */
constexpr double SToUs(double value);

/**
 * @brief Converts degrees celsius to degrees fahrenheit.
 * @param value Temperature in degrees celsius
 * @return Equivalent temperature in Degrees Fahrenheit
 */
constexpr double DegCToDegF(double value);

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
