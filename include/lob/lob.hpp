// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "lob/lob.h"

namespace lob {

/** @brief The size of a drag table. */
constexpr size_t kLobTableSize = LOB_TABLE_SIZE;
/** @brief The size in bytes of the builder buffer. */
constexpr size_t kLobBuilderBufferSize = LOB_BUILDER_BUFFER_SIZE;

/** @brief Enumerates the supported drag functions. */
enum class DragFunctionT : LobDragFunctionT {
  kG1 = ::kLobDragFunctionG1,  ///< @brief G1 drag function
  kG2 = ::kLobDragFunctionG2,  ///< @brief G2 drag function
  kG5 = ::kLobDragFunctionG5,  ///< @brief G5 drag function
  kG6 = ::kLobDragFunctionG6,  ///< @brief G6 drag function
  kG7 = ::kLobDragFunctionG7,  ///< @brief G7 drag function
  kG8 = ::kLobDragFunctionG8   ///< @brief G8 drag function
};

/** @brief Enumerates the supported atmosphere reference types. */
enum class AtmosphereReferenceT : LobAtmosphereReferenceT {
  kArmyStandardMetro =
      ::kLobAtmosphereReferenceArmyStandardMetro,  ///< @brief Army Standard
                                                   ///< Metro
  kIcao = ::kLobAtmosphereReferenceIcao  ///< @brief International Civil
                                         ///< Aviation Organization (ICAO)
};

/**
 * @brief Enumerates clock angle positions.
 * @note Values are named with Roman numerals. This is used for reasoning about
 * wind direction.
 */
enum class ClockAngleT : LobClockAngleT {
  kI = ::kLobClockAngleI,        ///< @brief one o'clock
  kII = ::kLobClockAngleII,      ///< @brief two o'clock
  kIII = ::kLobClockAngleIII,    ///< @brief three o'clock
  kIV = ::kLobClockAngleIV,      ///< @brief four o'clock
  kV = ::kLobClockAngleV,        ///< @brief five o'clock
  kVI = ::kLobClockAngleVI,      ///< @brief six o'clock
  kVII = ::kLobClockAngleVII,    ///< @brief seven o'clock
  kVIII = ::kLobClockAngleVIII,  ///< @brief eight o'clock
  kIX = ::kLobClockAngleIX,      ///< @brief nine o'clock
  kX = ::kLobClockAngleX,        ///< @brief ten o'clock
  kXI = ::kLobClockAngleXI,      ///< @brief eleven o'clock
  kXII = ::kLobClockAngleXII     ///< @brief twelve o'clock
};

/** @brief Error codes returned by the builder. */
enum class ErrorT : LobErrorT {
  kNone = ::kLobErrorNone,
  kAirPressureOOR = ::kLobErrorAirPressureOOR,
  kAltitudeOfBarometerOOR = ::kLobErrorAltitudeOfBarometerOOR,
  kAltitudeOfFiringSiteOOR = ::kLobErrorAltitudeOfFiringSiteOOR,
  kAltitudeOfThermometerOOR = ::kLobErrorAltitudeOfThermometerOOR,
  kAzimuthOOR = ::kLobErrorAzimuthOOR,
  kBallisticCoefficientOOR = ::kLobErrorBallisticCoefficientOOR,
  kBallisticCoefficientRequired = ::kLobErrorBallisticCoefficientRequired,
  kBaseDiameterOOR = ::kLobErrorBaseDiameterOOR,
  kDiameterOOR = ::kLobErrorDiameterOOR,
  kHumidityOOR = ::kLobErrorHumidityOOR,
  kInitialVelocityRequired = ::kLobErrorInitialVelocityRequired,
  kInternalError = ::kLobErrorInternalError,
  kLatitudeOOR = ::kLobErrorLatitudeOOR,
  kLengthOOR = ::kLobErrorLengthOOR,
  kMassOOR = ::kLobErrorMassOOR,
  kMaximumTimeOOR = ::kLobErrorMaximumTimeOOR,
  kMeplatDiameterOOR = ::kLobErrorMeplatDiameterOOR,
  kNoseLengthOOR = ::kLobErrorNoseLengthOOR,
  kOgiveRtROOR = ::kLobErrorOgiveRtROOR,
  kRangeAngleOOR = ::kLobErrorRangeAngleOOR,
  kTailLengthOOR = ::kLobErrorTailLengthOOR,
  kWindHeadingOOR = ::kLobErrorWindHeadingOOR,
  kZeroAngleOOR = ::kLobErrorZeroAngleOOR,
  kZeroDataRequired = ::kLobErrorZeroDataRequired,
  kZeroDistanceOOR = ::kLobErrorZeroDistanceOOR,
  kNotFormed = ::kLobErrorNotFormed
};

/**
 * @brief Converts an integer value to an enum class.
 * @tparam E The enum type to convert to.
 * @param value The underlying integer value.
 * @return The enum value corresponding to the integer.
 */
template <typename E>
constexpr E ToEnum(std::underlying_type_t<E> value) noexcept {
  static_assert(std::is_enum<E>::value, "ToEnum() only supports enum types");
  return static_cast<E>(value);
}

/**
 * @brief Converts an enum class value to its underlying integer type.
 * @tparam E The enum type to convert from.
 * @param value The enum value.
 * @return The underlying integer value.
 */
template <typename E>
constexpr std::underlying_type_t<E> ToUnderlying(E value) noexcept {
  return static_cast<std::underlying_type_t<E>>(value);
}

/** @brief Gravity vector. See @c LobGravity for member details. */
using Gravity = ::LobGravity;
/** @brief Wind vector. See @c LobWind for member details. */
using Wind = ::LobWind;
/** @brief Coriolis effect parameters. See @c LobCoriolis for member details. */
using Coriolis = ::LobCoriolis;
/**
 * @brief Structure of input parameters consumed by the solver.
 * @note This is not a user-friendly structure. Generate Input using the
 * provided Builder class. See @c LobInput for member details.
 */
using Input = ::LobInput;
/** @brief Structure holding output results. See @c LobOutput for member
 * details. */
using Output = ::LobOutput;

inline bool operator==(::LobErrorT a, ErrorT b) noexcept {
  return a == static_cast<::LobErrorT>(b);
}
inline bool operator!=(::LobErrorT a, ErrorT b) noexcept {
  return a != static_cast<::LobErrorT>(b);
}
inline bool operator==(ErrorT a, ::LobErrorT b) noexcept {
  return static_cast<::LobErrorT>(a) == b;
}
inline bool operator!=(ErrorT a, ::LobErrorT b) noexcept {
  return static_cast<::LobErrorT>(a) != b;
}

/**
 * @brief Builder class for constructing Input objects with a friendly
 * interface.
 */
class Builder {
 public:
  Builder() : builder_{} {
    ::LobBuilderInit(&builder_);
  }  ///< @brief Default constructor
  ~Builder() { ::LobBuilderDestroy(&builder_); }  ///< @brief Destructor

  Builder(const Builder& other) : builder_{} {  ///< @brief Copy constructor
    ::LobBuilderCopy(&builder_, &other.builder_);
  }

  Builder(Builder&& other) noexcept : builder_{} {  ///< @brief Move constructor
    ::LobBuilderCopy(&builder_, &other.builder_);
    ::LobBuilderReset(&other.builder_);
  }

  /** @brief Copy assignment operator. */
  Builder& operator=(const Builder& rhs) {
    if (this != &rhs) {
      ::LobBuilderCopy(&builder_, &rhs.builder_);
    }
    return *this;
  }

  /** @brief Move assignment operator. */
  Builder& operator=(Builder&& rhs) noexcept {
    if (this != &rhs) {
      ::LobBuilderCopy(&builder_, &rhs.builder_);
      ::LobBuilderReset(&rhs.builder_);
    }
    return *this;
  }

  /**
   * @brief Sets the ballistic coefficient (Psi).
   * @param value The ballistic coefficient value.
   * @return A reference to the Builder object.
   */
  Builder& BallisticCoefficientPsi(double value) {
    ::LobBuilderBallisticCoefficientPsi(&builder_, value);
    return *this;
  }

  /**
   * @brief Sets the atmosphere reference associated with ballistic coefficient.
   * @param type The atmosphere reference type.
   * @return A reference to the Builder object.
   */
  Builder& BCAtmosphere(AtmosphereReferenceT type) {
    ::LobBuilderBCAtmosphere(&builder_,
                             static_cast<::LobAtmosphereReferenceT>(type));
    return *this;
  }

  /**
   * @brief Sets the drag function associated with ballistic coefficient.
   * @param type The drag function type.
   * @return A reference to the Builder object.
   */
  Builder& BCDragFunction(DragFunctionT type) {
    ::LobBuilderBCDragFunction(&builder_,
                               static_cast<::LobDragFunctionT>(type));
    return *this;
  }

  /**
   * @brief Sets the projectile diameter (caliber) in inches.
   * @param value The diameter in inches.
   * @return A reference to the Builder object.
   */
  Builder& DiameterInch(double value) {
    ::LobBuilderDiameterInch(&builder_, value);
    return *this;
  }

  /**
   * @brief Sets the projectile meplat diameter in inches.
   * @param value The meplat in inches.
   * @return A reference to the Builder object.
   */
  Builder& MeplatDiameterInch(double value) {
    ::LobBuilderMeplatDiameterInch(&builder_, value);
    return *this;
  }

  /**
   * @brief Sets the projectile base diameter in inches.
   * @param value The base diameter in inches.
   * @return A reference to the Builder object.
   */
  Builder& BaseDiameterInch(double value) {
    ::LobBuilderBaseDiameterInch(&builder_, value);
    return *this;
  }

  /**
   * @brief Sets the projectile length in inches.
   * @param value The length in inches.
   * @return A reference to the Builder object.
   */
  Builder& LengthInch(double value) {
    ::LobBuilderLengthInch(&builder_, value);
    return *this;
  }

  /**
   * @brief Sets the projectile nose length in inches.
   * @param value The nose length in inches.
   * @return A reference to the Builder object.
   */
  Builder& NoseLengthInch(double value) {
    ::LobBuilderNoseLengthInch(&builder_, value);
    return *this;
  }

  /**
   * @brief Sets the projectile tail length in inches.
   * @param value The tail length in inches.
   * @return A reference to the Builder object.
   */
  Builder& TailLengthInch(double value) {
    ::LobBuilderTailLengthInch(&builder_, value);
    return *this;
  }

  /**
   * @brief Sets the Rt/R ratio of the projectile ogive.
   * @param value The Rt/R ratio.
   * @return A reference to the Builder object.
   */
  Builder& OgiveRtR(double value) {
    ::LobBuilderOgiveRtR(&builder_, value);
    return *this;
  }

  /**
   * @brief Loads a custom Mach vs Drag table for the projectile.
   * @note This is a direct alternative to using a ballistic coefficient and a
   * reference drag function.
   * @param pmachs Pointer to an array of mach values.
   * @param pdrags Pointer to an array of associated drag values.
   * @param size The number of mach-drag pairs in the table.
   * @return A reference to the Builder object.
   */
  Builder& MachVsDragTable(const float* pmachs, const float* pdrags,
                           size_t size) {
    ::LobBuilderMachVsDragTable(&builder_, pmachs, pdrags, size);
    return *this;
  }

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
    ::LobBuilderMachVsDragTable(&builder_, machs.data(), drags.data(), N);
    return *this;
  }

  /**
   * @brief Sets the projectile mass in grains.
   * @param value The mass in grains.
   * @return A reference to the Builder object.
   */
  Builder& MassGrains(double value) {
    ::LobBuilderMassGrains(&builder_, value);
    return *this;
  }

  /**
   * @brief Sets the initial velocity of the projectile in feet per second.
   * @param value The initial velocity in fps.
   * @return A reference to the Builder object.
   */
  Builder& InitialVelocityFps(uint16_t value) {
    ::LobBuilderInitialVelocityFps(&builder_, value);
    return *this;
  }

  /**
   * @brief Sets the height of the optic above the bore in inches.
   * @param value The optic height in inches.
   * @return A reference to the Builder object.
   */
  Builder& OpticHeightInches(double value) {
    ::LobBuilderOpticHeightInches(&builder_, value);
    return *this;
  }

  /**
   * @brief Sets the twist rate of the barrel in inches per turn.
   * @note Used to calculate adjustments for spin drift and aerodynamic jump.
   * @param value The twist rate in inches per turn.
   * @return A reference to the Builder object.
   */
  Builder& TwistInchesPerTurn(double value) {
    ::LobBuilderTwistInchesPerTurn(&builder_, value);
    return *this;
  }

  /**
   * @brief Sets the angle between the sight and launch angle used to achieve
   * zero.
   * @note This is a portable zero value useful when firing conditions differ
   * from zeroing conditions.
   * @param value The zero angle in MOA.
   * @return A reference to the Builder object.
   */
  Builder& ZeroAngleMOA(double value) {
    ::LobBuilderZeroAngleMOA(&builder_, value);
    return *this;
  }

  /**
   * @brief Sets the zero distance in yards.
   * @param value The zero distance in yards.
   * @return A reference to the Builder object.
   */
  Builder& ZeroDistanceYds(double value) {
    ::LobBuilderZeroDistanceYds(&builder_, value);
    return *this;
  }

  /**
   * @brief Sets the zero impact height in inches.
   * @note This would be used if zeroing three inches high at 100 yards for
   * example.
   * @param value The zero impact height in inches.
   * @return A reference to the Builder object.
   */
  Builder& ZeroImpactHeightInches(double value) {
    ::LobBuilderZeroImpactHeightInches(&builder_, value);
    return *this;
  }

  /**
   * @brief Sets the altitude of the firing site in feet.
   * @param value The altitude in feet.
   * @return A reference to the Builder object.
   */
  Builder& AltitudeOfFiringSiteFt(double value) {
    ::LobBuilderAltitudeOfFiringSiteFt(&builder_, value);
    return *this;
  }

  /**
   * @brief Sets the air pressure in inches of mercury (inHg).
   * @param value The air pressure in inHg.
   * @return A reference to the Builder object.
   */
  Builder& AirPressureInHg(double value) {
    ::LobBuilderAirPressureInHg(&builder_, value);
    return *this;
  }

  /**
   * @brief Sets the altitude of the location where air pressure was taken in
   * feet.
   * @note This only has an effect if the air pressure was taken from a site
   * other than the firing site with a different altitude such as a nearby
   * weather station.
   * @param value The altitude in feet.
   * @return A reference to the Builder object.
   */
  Builder& AltitudeOfBarometerFt(double value) {
    ::LobBuilderAltitudeOfBarometerFt(&builder_, value);
    return *this;
  }

  /**
   * @brief Sets the temperature in degrees Fahrenheit.
   * @param value The temperature in degrees F.
   * @return A reference to the Builder object.
   */
  Builder& TemperatureDegF(double value) {
    ::LobBuilderTemperatureDegF(&builder_, value);
    return *this;
  }

  /**
   * @brief Sets the altitude of the location where temperature was taken in
   * feet.
   * @note This only has an effect if the temperature was taken from a site
   * other than the firing site with a different altitude such as a nearby
   * weather station.
   * @param value The altitude in feet.
   * @return A reference to the Builder object.
   */
  Builder& AltitudeOfThermometerFt(double value) {
    ::LobBuilderAltitudeOfThermometerFt(&builder_, value);
    return *this;
  }

  /**
   * @brief Sets the relative humidity at the firing site in percent.
   * @param value The relative humidity in percent.
   * @return A reference to the Builder object.
   */
  Builder& RelativeHumidityPercent(double value) {
    ::LobBuilderRelativeHumidityPercent(&builder_, value);
    return *this;
  }

  /**
   * @brief Sets the wind heading using a clock angle.
   * @note Twelve O'Clock is pure tailwind, Six O'Clock is a pure headwind.
   * @param value The wind heading as a clock angle.
   * @return A reference to the Builder object.
   */
  Builder& WindHeading(ClockAngleT value) {
    ::LobBuilderWindHeading(&builder_, static_cast<::LobClockAngleT>(value));
    return *this;
  }

  /**
   * @brief Sets the wind heading in degrees.
   * @note 0 is pure tailwind, 180 is pure headwind.
   * @param value The wind heading in degrees.
   * @return A reference to the Builder object.
   */
  Builder& WindHeadingDeg(double value) {
    ::LobBuilderWindHeadingDeg(&builder_, value);
    return *this;
  }

  /**
   * @brief Sets the wind speed in feet per second.
   * @param value The wind speed in fps.
   * @return A reference to the Builder object.
   */
  Builder& WindSpeedFps(double value) {
    ::LobBuilderWindSpeedFps(&builder_, value);
    return *this;
  }

  /**
   * @brief Sets the wind speed in miles per hour.
   * @param value The wind speed in mph.
   * @return A reference to the Builder object.
   */
  Builder& WindSpeedMph(double value) {
    ::LobBuilderWindSpeedMph(&builder_, value);
    return *this;
  }

  /**
   * @brief Sets the azimuth (bearing) of the target in degrees.
   * @note Used for making coriolis effect corrections.
   * @param value The azimuth in degrees.
   * @return A reference to the Builder object.
   */
  Builder& AzimuthDeg(double value) {
    ::LobBuilderAzimuthDeg(&builder_, value);
    return *this;
  }

  /**
   * @brief Sets the latitude of the firing location in degrees.
   * @note Used for making coriolis effect corrections.
   * @param value The latitude in degrees.
   * @return A reference to the Builder object.
   */
  Builder& LatitudeDeg(double value) {
    ::LobBuilderLatitudeDeg(&builder_, value);
    return *this;
  }

  /**
   * @brief Sets the range angle (inclination) to the target in degrees.
   * @param value The range angle in degrees.
   * @return A reference to the Builder object.
   */
  Builder& RangeAngleDeg(double value) {
    ::LobBuilderRangeAngleDeg(&builder_, value);
    return *this;
  }

  /**
   * @brief Sets the minimum speed threshold for the solver.
   * @param value The minimum speed in feet per second (fps) at which the
   * solver will stop calculations.
   * @return A reference to the Builder object.
   */
  Builder& MinimumSpeed(uint16_t value) {
    ::LobBuilderMinimumSpeed(&builder_, value);
    return *this;
  }

  /**
   * @brief Sets the minimum energy threshold for the solver.
   * @param value The minimum energy in foot-pounds (ft*lbf) at which the
   * solver will stop calculations.
   * @return A reference to the Builder object.
   */
  Builder& MinimumEnergy(uint16_t value) {
    ::LobBuilderMinimumEnergy(&builder_, value);
    return *this;
  }

  /**
   * @brief Sets the maximum time of flight for the solver.
   * @param value The maximum time in seconds after which the solver will stop
   * calculations.
   * @return A reference to the Builder object.
   */
  Builder& MaximumTime(double value) {
    ::LobBuilderMaximumTime(&builder_, value);
    return *this;
  }

  /**
   * @brief Sets the step size for the numerical solver.
   * @note If set to zero the solver will use a variable step.
   * @param value The time step size in microseconds used by the solver.
   * @return A reference to the Builder object.
   */
  Builder& StepSize(uint16_t value) {
    ::LobBuilderStepSize(&builder_, value);
    return *this;
  }

  /**
   * @brief Resets the builder state by re-initializing.
   * @return A reference to the Builder object.
   */
  Builder& Reset() noexcept {
    ::LobBuilderReset(&builder_);
    return *this;
  }

  /**
   * @brief Builds the Input object with the configured parameters.
   * @return The constructed Input object.
   */
  Input Build() { return ::LobBuilderBuild(&builder_); }

 private:
  ::LobBuilder builder_;
};

/**
 * @brief Solves the exterior ballistics problem for a given set of ranges.
 * @param in Input parameters for the calculation.
 * @param pranges Pointer to an array of ranges (in feet) to solve for.
 * @param pouts Pointer to an array where the output results will be stored.
 * @param size The number of ranges to solve for.
 * @return The number of successful solutions.
 */
inline size_t Solve(const Input& in, const uint32_t* pranges, Output* pouts,
                    size_t size) {
  return ::LobSolve(&in, pranges, pouts, size);
}

/**
 * @brief Solves the exterior ballistics problem for a given set of ranges.
 * @tparam N The number of ranges to solve for.
 * @param in Input parameters for the calculation.
 * @param pranges Reference to an array of ranges (in feet) to solve for.
 * @param pouts Reference to an array where the output results will be stored.
 * @return The number of successful solutions.
 */
template <size_t N>
inline size_t Solve(const Input& in, const std::array<uint32_t, N>& pranges,
                    std::array<Output, N>& pouts) {
  return ::LobSolve(&in, pranges.data(), pouts.data(), N);
}

/**
 * @brief Gets the library version in major.minor.patch format.
 * @return Version string.
 */
inline const char* Version() { return ::LobVersion(); }

/**
 * @brief Converts minutes of angle (MOA) to milliradians (MIL).
 * @param value Angle in MOA.
 * @return Equivalent angle in MIL.
 */
inline double MoaToMil(double value) { return ::LobMoaToMil(value); }
/**
 * @brief Converts minutes of angle (MOA) to degrees.
 * @param value Angle in MOA.
 * @return Equivalent angle in degrees.
 */
inline double MoaToDeg(double value) { return ::LobMoaToDeg(value); }
/**
 * @brief Converts minutes of angle (MOA) to inches per hundred yards (IPHY).
 * @param value Angle in MOA.
 * @return Equivalent angle in IPHY.
 */
inline double MoaToIphy(double value) { return ::LobMoaToIphy(value); }
/**
 * @brief Converts minutes of angle (MOA) to projected inches at a given
 * range in feet.
 * @param value Angle in MOA.
 * @param range_ft Range in feet.
 * @return Equivalent projected inches.
 */
inline double MoaToInch(double value, double range_ft) {
  return ::LobMoaToInch(value, range_ft);
}

/**
 * @brief Converts milliradians (MIL) to minutes of angle (MOA).
 * @param value Angle in MIL.
 * @return Equivalent angle in MOA.
 */
inline double MilToMoa(double value) { return ::LobMilToMoa(value); }
/**
 * @brief Converts milliradians (MIL) to degrees.
 * @param value Angle in MIL.
 * @return Equivalent angle in degrees.
 */
inline double MilToDeg(double value) { return ::LobMilToDeg(value); }
/**
 * @brief Converts milliradians (MIL) to inches per hundred yards (IPHY).
 * @param value Angle in MIL.
 * @return Equivalent angle in IPHY.
 */
inline double MilToIphy(double value) { return ::LobMilToIphy(value); }
/**
 * @brief Converts milliradians (MIL) to projected inches at a given
 * range in feet.
 * @param value Angle in MIL.
 * @param range_ft Range in feet.
 * @return Equivalent projected inches.
 */
inline double MilToInch(double value, double range_ft) {
  return ::LobMilToInch(value, range_ft);
}

/**
 * @brief Converts degrees to minutes of angle (MOA).
 * @param value Angle in degrees.
 * @return Equivalent angle in MOA.
 */
inline double DegToMoa(double value) { return ::LobDegToMoa(value); }
/**
 * @brief Converts degrees to milliradians (MIL).
 * @param value Angle in degrees.
 * @return Equivalent angle in MIL.
 */
inline double DegToMil(double value) { return ::LobDegToMil(value); }

/**
 * @brief Inches of projection at a given range to minutes of angle (MOA).
 * @param value Projected inches.
 * @param range_ft Range in feet.
 * @return Equivalent angle in MOA.
 */
inline double InchToMoa(double value, double range_ft) {
  return ::LobInchToMoa(value, range_ft);
}
/**
 * @brief Inches of projection at a given range to milliradians (MIL).
 * @param value Projected inches.
 * @param range_ft Range in feet.
 * @return Equivalent angle in MIL.
 */
inline double InchToMil(double value, double range_ft) {
  return ::LobInchToMil(value, range_ft);
}
/**
 * @brief Inches of projection at a given range to degrees.
 * @param value Projected inches.
 * @param range_ft Range in feet.
 * @return Equivalent angle in degrees.
 */
inline double InchToDeg(double value, double range_ft) {
  return ::LobInchToDeg(value, range_ft);
}

/**
 * @brief Converts joules to foot-pounds.
 * @param value Energy in joules.
 * @return Equivalent energy in foot-pounds.
 */
inline double JToFtLbs(double value) { return ::LobJToFtLbs(value); }
/**
 * @brief Converts foot-pounds to joules.
 * @param value Energy in foot-pounds.
 * @return Equivalent energy in joules.
 */
inline double FtLbsToJ(double value) { return ::LobFtLbsToJ(value); }

/**
 * @brief Converts meters to yards.
 * @param value Length in meters.
 * @return Equivalent length in yards.
 */
inline double MToYd(double value) { return ::LobMToYd(value); }

/**
 * @brief Converts yards to feet.
 * @param value Length in yards.
 * @return Equivalent length in feet.
 */
inline double YdToFt(double value) { return ::LobYdToFt(value); }
/**
 * @brief Converts meters to feet.
 * @param value Length in meters.
 * @return Equivalent length in feet.
 */
inline double MToFt(double value) { return ::LobMToFt(value); }
/**
 * @brief Converts feet to inches.
 * @param value Length in feet.
 * @return Equivalent length in inches.
 */
inline double FtToIn(double value) { return ::LobFtToIn(value); }
/**
 * @brief Converts millimeters to inches.
 * @param value Length in millimeters.
 * @return Equivalent length in inches.
 */
inline double MmToIn(double value) { return ::LobMmToIn(value); }
/**
 * @brief Converts centimeters to inches.
 * @param value Length in centimeters.
 * @return Equivalent length in inches.
 */
inline double CmToIn(double value) { return ::LobCmToIn(value); }
/**
 * @brief Converts yards to meters.
 * @param value Length in yards.
 * @return Equivalent length in meters.
 */
inline double YdToM(double value) { return ::LobYdToM(value); }
/**
 * @brief Converts feet to meters.
 * @param value Length in feet.
 * @return Equivalent length in meters.
 */
inline double FtToM(double value) { return ::LobFtToM(value); }
/**
 * @brief Converts feet to yards.
 * @param value Length in feet.
 * @return Equivalent length in yards.
 */
inline double FtToYd(double value) { return ::LobFtToYd(value); }
/**
 * @brief Converts inches to millimeters.
 * @param value Length in inches.
 * @return Equivalent length in millimeters.
 */
inline double InToMm(double value) { return ::LobInToMm(value); }
/**
 * @brief Converts inches to centimeters.
 * @param value Length in inches.
 * @return Equivalent length in centimeters.
 */
inline double InToCm(double value) { return ::LobInToCm(value); }
/**
 * @brief Converts inches to feet.
 * @param value Length in inches.
 * @return Equivalent length in feet.
 */
inline double InToFt(double value) { return ::LobInToFt(value); }

/**
 * @brief Converts pascals to inches of mercury.
 * @param value Pressure in pascals.
 * @return Equivalent pressure in inches of mercury.
 */
inline double PaToInHg(double value) { return ::LobPaToInHg(value); }
/**
 * @brief Converts millibars to inches of mercury.
 * @param value Pressure in millibars.
 * @return Equivalent pressure in inches of mercury.
 */
inline double MbarToInHg(double value) { return ::LobMbarToInHg(value); }
/**
 * @brief Converts pounds per square inch (PSI) to inches of mercury.
 * @param value Pressure in PSI.
 * @return Equivalent pressure in inches of mercury.
 */
inline double PsiToInHg(double value) { return ::LobPsiToInHg(value); }

/**
 * @brief Converts pounds to grains.
 * @param value Mass in pounds.
 * @return Equivalent mass in grains.
 */
inline double LbsToGrain(double value) { return ::LobLbsToGrain(value); }
/**
 * @brief Converts grams to grains.
 * @param value Mass in grams.
 * @return Equivalent mass in grains.
 */
inline double GToGrain(double value) { return ::LobGToGrain(value); }
/**
 * @brief Converts kilograms to grains.
 * @param value Mass in kilograms.
 * @return Equivalent mass in grains.
 */
inline double KgToGrain(double value) { return ::LobKgToGrain(value); }

/**
 * @brief Converts kilograms per square meter to pounds mass per square inch.
 * @param value Sectional density in Kg/m^2.
 * @return Equivalent sectional density in lb/in^2.
 */
inline double KgSqMToPmsi(double value) { return ::LobKgSqMToPmsi(value); }

/**
 * @brief Converts feet per second to meters per second.
 * @param value Speed in feet per second.
 * @return Equivalent speed in meters per second.
 */
inline double FpsToMps(double value) { return ::LobFpsToMps(value); }
/**
 * @brief Converts meters per second to feet per second.
 * @param value Speed in meters per second.
 * @return Equivalent speed in feet per second.
 */
inline double MpsToFps(double value) { return ::LobMpsToFps(value); }
/**
 * @brief Converts kilometers per hour to miles per hour.
 * @param value Speed in kilometers per hour.
 * @return Equivalent speed in miles per hour.
 */
inline double KphToMph(double value) { return ::LobKphToMph(value); }
/**
 * @brief Converts Knots to miles per hour.
 * @param value Speed in Knots.
 * @return Equivalent speed in miles per hour.
 */
inline double KnToMph(double value) { return ::LobKnToMph(value); }

/**
 * @brief Converts milliseconds to seconds.
 * @param value Time in milliseconds.
 * @return Equivalent time in seconds.
 */
inline double MsToS(double value) { return ::LobMsToS(value); }
/**
 * @brief Converts microseconds to seconds.
 * @param value Time in microseconds.
 * @return Equivalent time in seconds.
 */
inline double UsToS(double value) { return ::LobUsToS(value); }
/**
 * @brief Converts seconds to milliseconds.
 * @param value Time in seconds.
 * @return Equivalent time in milliseconds.
 */
inline double SToMs(double value) { return ::LobSToMs(value); }
/**
 * @brief Converts seconds to microseconds.
 * @param value Time in seconds.
 * @return Equivalent time in microseconds.
 */
inline double SToUs(double value) { return ::LobSToUs(value); }

/**
 * @brief Converts degrees celsius to degrees fahrenheit.
 * @param value Temperature in degrees celsius.
 * @return Equivalent temperature in degrees fahrenheit.
 */
inline double DegCToDegF(double value) { return ::LobDegCToDegF(value); }

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
