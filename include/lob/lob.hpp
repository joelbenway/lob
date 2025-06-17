// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

#include "lob/lob_export.hpp"

namespace lob {

/**
 * @brief Gets the library version in major.minor.patch format.
 * @return Version string.
 */
LOB_EXPORT const char* Version();

/**
 * @brief Enumerates the supported drag functions.
 */
enum class LOB_EXPORT DragFunctionT : uint8_t {
  kG1 = 1U,  /// @brief G1 drag function
  kG2 = 2U,  /// @brief G2 drag function
  kG5 = 5U,  /// @brief G5 drag function
  kG6 = 6U,  /// @brief G6 drag function
  kG7 = 7U,  /// @brief G7 drag function
  kG8 = 8U   /// @brief G8 drag function
};

/**
 * @brief Enumerates the supported atmosphere reference types.
 */
enum class LOB_EXPORT AtmosphereReferenceT : uint8_t {
  kArmyStandardMetro,  /// @brief Army Standard Metro
  kIcao  /// @brief International Civil Aviation Organization (ICAO)
};

/**
 * @brief Enumerates clock angle positions.
 * @note Values are named with Roman numerals. This is used for reasoning about
 * wind direction.
 */
enum class LOB_EXPORT ClockAngleT : uint8_t {
  kI = 1U,  /// @brief one o'clock
  kII,      /// @brief two o'clock
  kIII,     /// @brief three o'clock
  kIV,      /// @brief four o'clock
  kV,       /// @brief five o'clock
  kVI,      /// @brief six o'clock
  kVII,     /// @brief seven o'clock
  kVIII,    /// @brief eight o'clock
  kIX,      /// @brief nine o'clock
  kX,       /// @brief ten o'clock
  kXI,      /// @brief eleven o'clock
  kXII      /// @brief twelve o'clock
};  // enum class ClockAngleT

enum class LOB_EXPORT ErrorT : uint8_t {
  kNone,
  kAirPressure,
  kAltitude,
  kAzimuth,
  kBallisticCoefficient,
  kBaseDiameter,
  kDiameter,
  kHumidity,
  kInitialVelocity,
  kLatitude,
  kLength,
  kMachDragTable,
  kMass,
  kMaximumTime,
  kMeplatDiameter,
  kNoseLength,
  kOgiveRtR,
  kRangeAngle,
  kTailLength,
  kWindHeading,
  kZeroAngle,
  kZeroDistance,
  kNotFormed
};  // enum class ErrorT

/// @brief Not-a-Number for floating-point values.
template <typename T = double>
constexpr T NaN() {
  static_assert(std::is_floating_point<T>::value,
                "NaN() only supports floating-point types");
  return std::numeric_limits<T>::quiet_NaN();
}

/**
 * @brief Structure of input parameters consumed by the solver.
 * @note This is is not a user-friendly structure. Generate `Input` using the
 * provided `Builder` class.
 */
struct LOB_EXPORT Input {
  static constexpr uint8_t kTableSize{85};   /// @brief The size of drag table.
  std::array<uint16_t, kTableSize> drags{};  /// @brief The drag table.
  double table_coefficient{NaN()};  /// @brief Used to scale the drag table.
  double speed_of_sound{NaN()};     /// @brief The local speed of sound in Fps.
  uint16_t velocity{0};        /// @brief Initial velocity of projectile in Fps.
  double mass{NaN()};          /// @brief Mass of the projectile in pounds.
  double optic_height{NaN()};  /// @brief Height of the optic above the bore.
  struct Gravity {             /// @brief Gravity vector.
    double x{NaN()};  /// @brief Acceleration ft/s/s in the x-direction.
    double y{NaN()};  /// @brief Acceleration ft/s/s in the y-direction.
  } gravity;
  struct Wind {       /// @brief Wind vector.
    double x{NaN()};  /// @brief Wind speed in fps in the x-direction.
    double z{NaN()};  /// @brief Wind speed in fps in the z-direction.
  } wind;
  struct Coriolis {             /// @brief Coriolis effect parameters.
    double cos_l_sin_a{NaN()};  /// @brief 2Ωcos(latitude)sin(azimuth)
    double sin_l{NaN()};        /// @brief 2Ωsin(latitude)
    double cos_l_cos_a{NaN()};  /// @brief 2Ωcos(latitude)cos(azimuth)
  } corilolis;
  double zero_angle{NaN()};  /// @brief Angle between sight and trajectory.
  double stability_factor{NaN()};    /// @brief Miller stability factor.
  double aerodynamic_jump{NaN()};    /// @brief Aerodynamic jump effect in Moa.
  double spindrift_factor{NaN()};    /// @brief Spin drift factor.
  uint16_t minimum_speed{0};         /// @brief Minimum speed for solver.
  double max_time{NaN()};            /// @brief Max time of flight for solver.
  uint16_t step_size{0};             /// @brief Step size for solver.
  ErrorT error{ErrorT::kNotFormed};  /// @brief Builder error field.
};  // struct Input

class Impl;

/**
 * @brief Builder class for constructing `Input` objects with a friendly
 * interface.
 */
class LOB_EXPORT Builder {
 public:
  Builder();                          /// @brief Default constructor
  ~Builder();                         /// @brief Default destructor
  Builder(const Builder& other);      /// @brief Copy constructor
  Builder(Builder&& other) noexcept;  /// @brief Move constructor
  Builder& operator=(
      const Builder& rhs);  /// @brief Copy assignment constructor
  Builder& operator=(
      Builder&& rhs) noexcept;  /// @brief Move assignment constructor

  /**
   * @brief Sets the ballistic coefficient (Psi).
   * @param value The ballistic coefficient value.
   * @return A reference to the Builder object.
   */
  Builder& BallisticCoefficientPsi(double value);

  /**
   * @brief Sets the atmosphere reference associated with ballistic coefficient.
   * @param type The atmosphere reference type.
   * @return A reference to the Builder object.
   */
  Builder& BCAtmosphere(AtmosphereReferenceT type);

  /**
   * @brief Sets the drag function associated with ballistic coefficient.
   * @param type The drag function type.
   * @return A reference to the Builder object.
   */
  Builder& BCDragFunction(DragFunctionT type);

  /**
   * @brief Sets the projectile diameter (caliber) in inches.
   * @param value The diameter in inches.
   * @return A reference to the Builder object.
   */
  Builder& DiameterInch(double value);

  /**
   * @brief Sets the projectile meplat diameter in inches.
   * @param value The meplat in inches.
   * @return A reference to the Builder object.
   */
  Builder& MeplatDiameterInch(double value);

  /**
   * @brief Sets the projectile base diameter in inches.
   * @param value The base diameter in inches.
   * @return A reference to the Builder object.
   */
  Builder& BaseDiameterInch(double value);

  /**
   * @brief Sets the projectile length in inches.
   * @param value The length in inches.
   * @return A reference to the Builder object.
   */
  Builder& LengthInch(double value);

  /**
   * @brief Sets the projectile nose length in inches.
   * @param value The nose length in inches.
   * @return A reference to the Builder object.
   */
  Builder& NoseLengthInch(double value);

  /**
   * @brief Sets the projectile tail length in inches.
   * @param value The tail length in inches.
   * @return A reference to the Builder object.
   */
  Builder& TailLengthInch(double value);

  /**
   * @brief Sets the Rt/R ratio of the projectile ogive.
   * @param value The Rt/R ratio.
   * @return A reference to the Builder object.
   */
  Builder& OgiveRtR(double value);

  /**
   * @brief Loads a custom Mach vs Drag table for the projectile.
   * @note This is a direct alternative to using a ballistic coefficient and a
   * reference drag function.
   * @tparam N The number of mach-drag pairs in the table.
   * @param pmachs Pointer to an array of mach values.
   * @param pdrags Pointer to an array of associated drag values.
   * @param size The number of mach-drag pairs in the table.
   * @return A reference to the Builder object.
   */
  Builder& MachVsDragTable(const float* pmachs, const float* pdrags,
                           size_t size);
  /**
   * @brief Loads a custom Mach vs Drag table for the projectile.
   * @note This is a direct alternative to using a ballistic coefficient and a
   * reference drag function.
   * @tparam N The number of mach-drag pairs in the table.
   * @param machs Reference to an array of mach values.
   * @param drags Reference to an array of associated drag values.
   * @return A reference to the Builder object.
   */
  template <size_t N>
  Builder& MachVsDragTable(const std::array<float, N>& machs,
                           const std::array<float, N>& drags) {
    return MachVsDragTable(machs.data(), drags.data(), N);
  }

  /**
   * @brief Sets the projectile mass in grains.
   * @param value The mass in grains.
   * @return A reference to the Builder object.
   */
  Builder& MassGrains(double value);

  /**
   * @brief Sets the initial velocity of the projectile in feet per second.
   * @param value The initial velocity in fps.
   * @return A reference to the Builder object.
   */
  Builder& InitialVelocityFps(uint16_t value);
  /**
   * @brief Sets the height of the optic above the bore in inches.
   * @param value The optic height in inches.
   * @return A reference to the Builder object.
   */
  Builder& OpticHeightInches(double value);

  /**
   * @brief Sets the twist rate of the barrel in inches per turn.
   * @note Used to calculate adjustments for spin drift and aerodynamic jump.
   * @param value The twist rate in inches per turn.
   * @return A reference to the Builder object.
   */
  Builder& TwistInchesPerTurn(double value);

  /**
   * @brief Sets the angle between the sight and launch angle used to achieve
   * zero.
   * @note This is a portable zero value useful when firing conditions differ
   * from zeroing conditions.
   * @param value The zero angle in MOA.
   * @return A reference to the Builder object.
   */
  Builder& ZeroAngleMOA(double value);

  /**
   * @brief Sets the zero distance in yards.
   * @param value The zero distance in yards.
   * @return A reference to the Builder object.
   */
  Builder& ZeroDistanceYds(double value);

  /**
   * @brief Sets the zero impact height in inches.
   * @note This would be used if zeroing three inches high at 100 yards for
   * example.
   * @param value The zero impact height in inches.
   * @return A reference to the Builder object.
   */
  Builder& ZeroImpactHeightInches(double value);

  /**
   * @brief Sets the altitude of the firing site in feet.
   * @param value The altitude in feet.
   * @return A reference to the Builder object.
   */
  Builder& AltitudeOfFiringSiteFt(double value);

  /**
   * @brief Sets the air pressure in inches of mercury (inHg).
   * @param value The air pressure in inHg.
   * @return A reference to the Builder object.
   */
  Builder& AirPressureInHg(double value);

  /**
   * @brief Sets the altitude of the location where air pressure was taken in
   * feet.
   * @note This only has an effect if the airpressure was taken from a site
   * other than the firing site with a different altitude such as a nearby
   * weather station.
   * @param value The altitude in feet.
   * @return A reference to the Builder object.
   */
  Builder& AltitudeOfBarometerFt(double value);

  /**
   * @brief Sets the temperature in degrees Fahrenheit.
   * @param value The temperature in degrees F.
   * @return A reference to the Builder object.
   */
  Builder& TemperatureDegF(double value);

  /**
   * @brief Sets the altitude of the location where temperature was taken in
   * feet.
   * @note This only has an effect if the temperature was taken from a site
   * other than the firing site with a different altitude such as a nearby
   * weather station.
   * @param value The altitude in feet.
   * @return A reference to the Builder object.
   */
  Builder& AltitudeOfThermometerFt(double value);

  /**
   * @brief Sets the relative humidity at the firing site in percent.
   * @param value The relative humidity in percent.
   * @return A reference to the Builder object.
   */
  Builder& RelativeHumidityPercent(double value);

  /**
   * @brief Sets the wind heading using a clock angle.
   * @note Twelve O'Clock is pure tailwind, Six O'Clock is a pure headwind.
   * @param value The wind heading as a clock angle.
   * @return A reference to the Builder object.
   */
  Builder& WindHeading(ClockAngleT value);

  /**
   * @brief Sets the wind heading in degrees.
   * @note 0 is pure tailwind, 180 is pure headwind.
   * @param value The wind heading in degrees.
   * @return A reference to the Builder object.
   */
  Builder& WindHeadingDeg(double value);

  /**
   * @brief Sets the wind speed in feet per second.
   * @param value The wind speed in fps.
   * @return A reference to the Builder object.
   */
  Builder& WindSpeedFps(double value);

  /**
   * @brief Sets the wind speed in miles per hour.
   * @param value The wind speed in mph.
   * @return A reference to the Builder object.
   */
  Builder& WindSpeedMph(double value);

  /**
   * @brief Sets the azimuth (bearing) of the target in degrees.
   * @note Used for making coriolis effect corrections.
   * @param value The azimuth in degrees.
   * @return A reference to the Builder object.
   */
  Builder& AzimuthDeg(double value);

  /**
   * @brief Sets the latitude of the firing location in degrees.
   * @note Used for making coriolis effect corrections.
   * @param value The latitude in degrees.
   * @return A reference to the Builder object.
   */
  Builder& LatitudeDeg(double value);

  /**
   * @brief Sets the range angle (inclination) to the target in degrees.
   * @param value The range angle in degrees.
   * @return A reference to the Builder object.
   */
  Builder& RangeAngleDeg(double value);

  /**
   * @brief Sets the minimum speed threshold for the solver.
   * @param value The minimum speed in feet per second (fps) at which the solver
   * will stop calculations.
   * @return A reference to the Builder object.
   */
  Builder& MinimumSpeed(uint16_t value);

  /**
   * @brief Sets the minimum energy threshold for the solver.
   * @param value The minimum energy in foot-pounds (ft·lbf) at which the solver
   * will stop calculations.
   * @return A reference to the Builder object.
   */
  Builder& MinimumEnergy(uint16_t value);

  /**
   * @brief Sets the maximum time of flight for the solver.
   * @param value The maximum time in seconds after which the solver will stop
   * calculations.
   * @return A reference to the Builder object.
   */
  Builder& MaximumTime(double value);

  /**
   * @brief Sets the step size for the numerical solver.
   * @param value The time step size in microseconds (µs) used by the solver.
   * @return A reference to the Builder object.
   */
  Builder& StepSize(uint16_t value);

  /**
   * @brief Resets the builder state by creating a fresh Impl object.
   * @return A reference to the Builder object.
   */
  Builder& Reset() noexcept;

  /**
   * @brief Builds the `Input` object with the configured parameters.
   * @return The constructed `Input` object.
   */
  Input Build();

 private:
  static constexpr size_t kBufferSize{536};
  union AlignmentT {
    double foo;
    size_t bar;
  };
  alignas(AlignmentT) std::array<uint8_t, kBufferSize> buffer_{};
  Impl* pimpl_{nullptr};
};  // class Builder

/**
 * @brief Structure holding the output results of the ballistic calculation.
 */
struct LOB_EXPORT Output {
  uint32_t range{0};       /// @brief Associated range in yards.
  uint16_t velocity{0};    /// @brief Calculated velocity in feet per second.
  uint32_t energy{0};      /// @brief Calculated energy in foot-pounds.
  double elevation{0.0};   /// @brief Calculated elevation change in inches.
  double deflection{0.0};  /// @brief Calculated windage deflection in inches.
  double time_of_flight{0.0};  /// @brief Time of flight in seconds.
};  // struct Output

/**
 * @brief Solves the exterior ballistics problem for a given set of ranges.
 * @param in Input parameters for the calculation.
 * @param pranges Pointer to an array of ranges (in feet) to solve for.
 * @param pouts Pointer to an array wherec the output results will be stored.
 * @param size The number of ranges to solve for.
 * @return The number of successful solutions.
 */
LOB_EXPORT size_t Solve(const Input& in, const uint32_t* pranges, Output* pouts,
                        size_t size);

/**
 * @brief Solves the exterior ballistics problem for a given set of ranges.
 * @tparam N The number of ranges to solve for.
 * @param in Input parameters for the calculation.
 * @param pranges Reference to an array of ranges (in feet) to solve for.
 * @param pouts Reference to an array where the output results will be stored.
 * @return The number of successful solutions.
 */
template <size_t N>
size_t Solve(const Input& in, const std::array<uint32_t, N>& pranges,
             std::array<Output, N>& pouts) {
  return Solve(in, pranges.data(), pouts.data(), N);
}

/**
 * @brief Converts minutes of angle (MOA) to milliradians (MIL).
 * @param value Angle in MOA.
 * @return Equivalent angle in MIL.
 */
LOB_EXPORT double MoaToMil(double value);

/**
 * @brief Converts minutes of angle (MOA) to degrees.
 * @param value Angle in MOA.
 * @return Equivalent angle in degrees.
 */
LOB_EXPORT double MoaToDeg(double value);

/**
 * @brief Converts minutes of angle (MOA) to inches per hundred yards (IPHY).
 * @param value Angle in MOA.
 * @return Equivalent angle in IPHY.
 */
LOB_EXPORT double MoaToIphy(double value);

/**
 * @brief Converts minutes of angle (MOA) to projected inches at a given
 * range in feet.
 * @param value Angle in MOA.
 * @param range_ft Range in feet.
 * @return Equivalent projected inches.
 */
LOB_EXPORT double MoaToInch(double value, double range_ft);

/**
 * @brief Converts milliradians (MIL) to minutes of angle (MOA).
 * @param value Angle in MIL.
 * @return Equivalent angle in MOA.
 */
LOB_EXPORT double MilToMoa(double value);

/**
 * @brief Converts milliradians (MIL) to degrees.
 * @param value Angle in MIL.
 * @return Equivalent angle in degrees.
 */
LOB_EXPORT double MilToDeg(double value);

/**
 * @brief Converts milliradians (MIL) to inches per hundred yards (IPHY).
 * @param value Angle in MIL.
 * @return Equivalent angle in IPHY.
 */
LOB_EXPORT double MilToIphy(double value);

/**
 * @brief Converts milliradians (MIL) to projected inches at a given
 * range in feet.
 * @param value Angle in MIL.
 * @param range_ft Range in feet.
 * @return Equivalent projected inches.
 */
LOB_EXPORT double MilToInch(double value, double range_ft);

/**
 * @brief Converts degrees to minutes of angle (MOA).
 * @param value Angle in degrees.
 * @return Equivalent angle in MOA.
 */
LOB_EXPORT double DegToMoa(double value);

/**
 * @brief Converts degrees to milliradians (MIL).
 * @param value Angle in degrees.
 * @return Equivalent angle in MIL.
 */
LOB_EXPORT double DegToMil(double value);

/**
 * @brief Inches of projection at a given range to minutes of angle (MOA)
 * @param value Projected inches.
 * @param range_ft Range in feet.
 * @return Equivalent angle in MOA.
 */
LOB_EXPORT double InchToMoa(double value, double range_ft);

/**
 * @brief Inches of projection at a given range to milliradians (MIL)
 * @param value Projected inches.
 * @param range_ft Range in feet.
 * @return Equivalent angle in MIL.
 */
LOB_EXPORT double InchToMil(double value, double range_ft);

/**
 * @brief Inches of projection at a given range to degrees.
 * @param value Projected inches.
 * @param range_ft Range in feet.
 * @return Equivalent angle in degrees.
 */
LOB_EXPORT double InchToDeg(double value, double range_ft);

/**
 * @brief Converts joules to foot-pounds.
 * @param value Energy in joules.
 * @return Equivalent energy in foot-pounds.
 */
LOB_EXPORT double JToFtLbs(double value);

/**
 * @brief Converts foot-pounds to joules.
 * @param value Energy in foot-pounds.
 * @return Equivalent energy in joules.
 */
LOB_EXPORT double FtLbsToJ(double value);

/**
 * @brief Converts meters to yards.
 * @param value Length in meters.
 * @return Equivalent length in yards.
 */
LOB_EXPORT double MtoYd(double value);

/**
 * @brief Converts yards to feet.
 * @param value Length in yards.
 * @return Equivalent length in feet.
 */
LOB_EXPORT double YdToFt(double value);

/**
 * @brief Converts meters to feet.
 * @param value Length in meters.
 * @return Equivalent length in feet.
 */
LOB_EXPORT double MToFt(double value);

/**
 * @brief Converts feet to inches.
 * @param value Length in feet.
 * @return Equivalent length in inches.
 */
LOB_EXPORT double FtToIn(double value);

/**
 * @brief Converts millimeters to inches.
 * @param value Length in millimeters.
 * @return Equivalent length in inches.
 */
LOB_EXPORT double MmToIn(double value);

/**
 * @brief Converts centimeters to inches.
 * @param value Length in centimeters.
 * @return Equivalent length in inches.
 */
LOB_EXPORT double CmToIn(double value);

/**
 * @brief Converts yards to meters.
 * @param value Length in yards.
 * @return Equivalent length in meters.
 */
LOB_EXPORT double YdToM(double value);

/**
 * @brief Converts feet to meters.
 * @param value Length in feet.
 * @return Equivalent length in meters.
 */
LOB_EXPORT double FtToM(double value);

/**
 * @brief Converts feet to yards.
 * @param value Length in feet.
 * @return Equivalent length in yards.
 */
LOB_EXPORT double FtToYd(double value);

/**
 * @brief Converts inches to millimeters.
 * @param value Length in inches.
 * @return Equivalent length in millimeters.
 */
LOB_EXPORT double InToMm(double value);

/**
 * @brief Converts inches to centimeters.
 * @param value Length in inches.
 * @return Equivalent length in centimeters.
 */
LOB_EXPORT double InToCm(double value);

/**
 * @brief Converts inches to feet.
 * @param value Length in inches.
 * @return Equivalent length in feet.
 */
LOB_EXPORT double InToFt(double value);

/**
 * @brief Converts pascals to inches of mercury.
 * @param value Pressure in pascals.
 * @return Equivalent pressure in inches of mercury.
 */
LOB_EXPORT double PaToInHg(double value);

/**
 * @brief Converts millibars to inches of mercury.
 * @param value Pressure in millibars.
 * @return Equivalent pressure in inches of mercury.
 */
LOB_EXPORT double MbarToInHg(double value);

/**
 * @brief Converts pounds per square inch (PSI) to inches of mercury.
 * @param value Pressure in PSI.
 * @return Equivalent pressure in inches of mercury.
 */
LOB_EXPORT double PsiToInHg(double value);

/**
 * @brief Converts pounds to grains.
 * @param value Mass in pounds.
 * @return Equivalent mass in grains.
 */
LOB_EXPORT double LbsToGrain(double value);

/**
 * @brief Converts grams to grains.
 * @param value Mass in grams.
 * @return Equivalent mass in grains.
 */
LOB_EXPORT double GToGrain(double value);

/**
 * @brief Converts kilograms to grains.
 * @param value Mass in kilograms.
 * @return Equivalent mass in grains.
 */
LOB_EXPORT double KgToGrain(double value);

/**
 * @brief Converts kilograms per square meter to pounds mass per square inch.
 * @param value Sectional density in Kg/m².
 * @return Equivalent sectional density in lb/in².
 */
LOB_EXPORT double KgSqMToPmsi(double value);

/**
 * @brief Converts feet per second to meters per second.
 * @param value Speed in feet per second.
 * @return Equivalent speed in meters per second.
 */
LOB_EXPORT double FpsToMps(double value);

/**
 * @brief Converts meters per second to feet per second.
 * @param value Speed in meters per second.
 * @return Equivalent speed in feet per second.
 */
LOB_EXPORT double MpsToFps(double value);

/**
 * @brief Converts kilometers per hour to miles per hour.
 * @param value Speed in kilometers per hour.
 * @return Equivalent speed in miles per hour.
 */
LOB_EXPORT double KphToMph(double value);

/**
 * @brief Converts Knots to miles per hour.
 * @param value Speed in Knots.
 * @return Equivalent speed in miles per hour.
 */
LOB_EXPORT double KnToMph(double value);

/**
 * @brief Converts milliseconds to seconds.
 * @param value Time in milliseconds.
 * @return Equivalent time in seconds.
 */
LOB_EXPORT double MsToS(double value);

/**
 * @brief Converts microseconds to seconds.
 * @param value Time in microseconds.
 * @return Equivalent time in seconds.
 */
LOB_EXPORT double UsToS(double value);

/**
 * @brief Converts seconds to milliseconds.
 * @param value Time in seconds.
 * @return Equivalent time in milliseconds.
 */
LOB_EXPORT double SToMs(double value);

/**
 * @brief Converts seconds to microseconds.
 * @param value Time in seconds.
 * @return Equivalent time in microseconds.
 */
LOB_EXPORT double SToUs(double value);

/**
 * @brief Converts degrees celsius to degrees fahrenheit.
 * @param value Temperature in degrees celsius
 * @return Equivalent temperature in Degrees Fahrenheit
 */
LOB_EXPORT double DegCToDegF(double value);

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
