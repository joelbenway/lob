// This file is a part of lob, an exterior ballistics calculation library
// Copyright (c) 2025  Joel Benway
// Please see end of file for extended copyright information

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

#include "lob/lob_export.hpp"

namespace lob {

/**
 * @brief Gets the library version in major.minor.patch format.
 * @return Version string.
 */
LOB_EXPORT const char* Version();

enum class LOB_EXPORT DragFunctionT : uint8_t { kG1, kG2, kG5, kG6, kG7, kG8 };
enum class LOB_EXPORT AtmosphereReferenceT : uint8_t {
  kArmyStandardMetro,
  kIcao
};

enum class LOB_EXPORT ClockAngleT : uint8_t {
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
};  // enum class ClockAngleT

static constexpr auto kNaN = std::numeric_limits<double>::quiet_NaN();

struct LOB_EXPORT Input {
  static constexpr uint8_t kTableSize{85};
  std::array<uint16_t, kTableSize> drags{};
  float table_coefficent{kNaN};
  float speed_of_sound{kNaN};
  uint16_t velocity{0};
  float mass{kNaN};
  float optic_height{kNaN};
  struct Gravity {
    float x{kNaN};
    float y{kNaN};
  } gravity;
  struct Wind {
    float x{kNaN};
    float z{kNaN};
  } wind;
  struct Coriolis {
    float cos_l_sin_a{kNaN};
    float sin_l{kNaN};
    float cos_l_cos_a{kNaN};
  } corilolis;
  float zero_angle{kNaN};
  float aerodynamic_jump{kNaN};
  float stability_factor{kNaN};
};  // struct Input

class Impl;

class LOB_EXPORT Builder {
 public:
  Builder();
  ~Builder();
  Builder(const Builder& other);
  Builder(Builder&& other) noexcept;
  Builder& operator=(const Builder& rhs);
  Builder& operator=(Builder&& rhs) noexcept;

  Builder& BallisticCoefficentPsi(float value);
  Builder& BCAtmosphere(AtmosphereReferenceT type);
  Builder& BCDragFunction(DragFunctionT type);
  Builder& DiameterInch(float value);
  Builder& LengthInch(float value);
  Builder& MassGrains(float value);
  Builder& InitialVelocityFps(uint16_t value);
  Builder& OpticHeightInches(float value);
  Builder& TwistInchesPerTurn(float value);
  Builder& ZeroAngleMOA(float value);
  Builder& ZeroDistanceYds(float value);
  Builder& ZeroImpactHeightInches(float value);
  Builder& AltitudeOfFiringSiteFt(float value);
  Builder& AirPressureInHg(float value);
  Builder& AltitudeOfBarometerFt(float value);
  Builder& TemperatureDegF(float value);
  Builder& AltitudeOfThermometerFt(float value);
  Builder& RelativeHumidityPercent(float value);
  Builder& WindHeading(ClockAngleT value);
  Builder& WindHeadingDeg(float value);
  Builder& WindSpeedFps(float value);
  Builder& WindSpeedMph(float value);
  Builder& AzimuthDeg(float value);
  Builder& LatitudeDeg(float value);
  Builder& RangeAngleDeg(float value);
  Input Build();

 private:
  static constexpr size_t kBufferSize{392};
  alignas(double) std::array<uint8_t, kBufferSize> buffer_{};
  Impl* pimpl_{nullptr};
};  // class Builder

struct LOB_EXPORT Options {
  uint16_t min_speed{0};
  uint16_t min_energy{0};
  float max_time{kNaN};
  uint16_t step_size{0};
};  // struct Options

struct LOB_EXPORT Output {
  uint32_t range{0};
  uint16_t velocity{0};
  uint32_t energy{0};
  float elevation{0.0F};
  float deflection{0.0F};
  float time_of_flight{0.0F};
};  // struct Output

LOB_EXPORT size_t Solve(const Input& in, const uint32_t* pranges, Output* pouts,
                        size_t size, const Options& options);

template <size_t N>
LOB_EXPORT size_t Solve(const Input& in, const std::array<uint32_t, N>* pranges,
                        std::array<Output, N>* pouts,
                        const Options& options = Options{}) {
  return Solve(in, pranges->data(), pouts->data(), N, options);
}

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
 * @brief Converts minutes of angle (MOA) to inches per hundred yards (IPHY).
 * @param value Angle in MOA.
 * @return Equivalent angle in IPHY.
 */
constexpr double MoaToIphy(double value);

/**
 * @brief Converts minutes of angle (MOA) to projected inches at a given
 * range in feet.
 * @param value Angle in MOA.
 * @param range_ft Range in feet.
 * @return Equivalent projected inches.
 */
constexpr double MoaToInch(double value, double range_ft);

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
 * @brief Converts milliradians (MIL) to projected inches at a given
 * range in feet.
 * @param value Angle in MIL.
 * @param range_ft Range in feet.
 * @return Equivalent projected inches.
 */
constexpr double MilToInch(double value, double range_ft);

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
constexpr double InchToMil(double value, double range_ft);

/**
 * @brief Inches of projection at a given range to degrees.
 * @param value Projected inches.
 * @param range_ft Range in feet.
 * @return Equivalent angle in degrees.
 */
constexpr double InchToDeg(double value, double range_ft);

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
LOB_EXPORT double YdToFt(double value);

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
