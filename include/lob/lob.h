// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#ifndef LOB_H
#define LOB_H

#include <stddef.h>
#include <stdint.h>

#include "lob/lob_export.hpp"

#ifdef __cplusplus
extern "C" {
// NOLINTBEGIN
#endif

/** @brief The number of entries in a drag table. */
#define LOB_TABLE_SIZE 85
/** @brief The size in bytes of the builder buffer. */
#define LOB_BUILDER_BUFFER_SIZE 576

/** @brief Drag function type. */
typedef uint8_t LobDragFunctionT;
enum {
  kLobDragFunctionG1 = 1,  ///< @brief G1 drag function
  kLobDragFunctionG2 = 2,  ///< @brief G2 drag function
  kLobDragFunctionG5 = 5,  ///< @brief G5 drag function
  kLobDragFunctionG6 = 6,  ///< @brief G6 drag function
  kLobDragFunctionG7 = 7,  ///< @brief G7 drag function
  kLobDragFunctionG8 = 8   ///< @brief G8 drag function
};

/** @brief Atmosphere reference type. */
typedef uint8_t LobAtmosphereReferenceT;
enum {
  kLobAtmosphereReferenceArmyStandardMetro = 0,  ///< @brief Army Standard Metro
  kLobAtmosphereReferenceIcao =
      1  ///< @brief International Civil Aviation Organization (ICAO)
};

/** @brief Clock angle positions for wind direction. */
typedef uint8_t LobClockAngleT;
enum {
  kLobClockAngleI = 1,     ///< @brief one o'clock
  kLobClockAngleII = 2,    ///< @brief two o'clock
  kLobClockAngleIII = 3,   ///< @brief three o'clock
  kLobClockAngleIV = 4,    ///< @brief four o'clock
  kLobClockAngleV = 5,     ///< @brief five o'clock
  kLobClockAngleVI = 6,    ///< @brief six o'clock
  kLobClockAngleVII = 7,   ///< @brief seven o'clock
  kLobClockAngleVIII = 8,  ///< @brief eight o'clock
  kLobClockAngleIX = 9,    ///< @brief nine o'clock
  kLobClockAngleX = 10,    ///< @brief ten o'clock
  kLobClockAngleXI = 11,   ///< @brief eleven o'clock
  kLobClockAngleXII = 12   ///< @brief twelve o'clock
};

/** @brief Error codes returned by the builder. */
typedef uint8_t LobErrorT;
enum {
  kLobErrorNone = 0,
  kLobErrorAirPressureOOR,
  kLobErrorAltitudeOfBarometerOOR,
  kLobErrorAltitudeOfFiringSiteOOR,
  kLobErrorAltitudeOfThermometerOOR,
  kLobErrorAzimuthOOR,
  kLobErrorBallisticCoefficientOOR,
  kLobErrorBallisticCoefficientRequired,
  kLobErrorBaseDiameterOOR,
  kLobErrorDiameterOOR,
  kLobErrorHumidityOOR,
  kLobErrorInitialVelocityRequired,
  kLobErrorInternalError,
  kLobErrorLatitudeOOR,
  kLobErrorLengthOOR,
  kLobErrorMassOOR,
  kLobErrorMaximumTimeOOR,
  kLobErrorMeplatDiameterOOR,
  kLobErrorNoseLengthOOR,
  kLobErrorOgiveRtROOR,
  kLobErrorRangeAngleOOR,
  kLobErrorTailLengthOOR,
  kLobErrorWindHeadingOOR,
  kLobErrorZeroAngleOOR,
  kLobErrorZeroDataRequired,
  kLobErrorZeroDistanceOOR,
  kLobErrorNotFormed
};

/** @brief Gravity vector. */
typedef struct {
  double x;  ///< @brief Acceleration ft/s/s in the x-direction.
  double y;  ///< @brief Acceleration ft/s/s in the y-direction.
} LobGravity;

/** @brief Wind vector. */
typedef struct {
  double x;  ///< @brief Wind speed in fps in the x-direction.
  double z;  ///< @brief Wind speed in fps in the z-direction.
} LobWind;

/** @brief Coriolis effect parameters. */
typedef struct {
  double cos_l_sin_a;  ///< @brief 2*Omega*cos(latitude)*sin(azimuth)
  double sin_l;        ///< @brief 2*Omega*sin(latitude)
  double cos_l_cos_a;  ///< @brief 2*Omega*cos(latitude)*cos(azimuth)
} LobCoriolis;

/**
 * @brief Structure of input parameters consumed by the solver.
 * @note This is not a user-friendly structure. Generate LobInput using the
 * provided LobBuilder.
 */
typedef struct {
  uint16_t drags[LOB_TABLE_SIZE];  ///< @brief The drag table.
  double table_coefficient;        ///< @brief Used to scale the drag table.
  double speed_of_sound;           ///< @brief The local speed of sound in Fps.
  uint16_t velocity;        ///< @brief Initial velocity of projectile in Fps.
  double mass;              ///< @brief Mass of the projectile in pounds.
  double optic_height;      ///< @brief Height of the optic above the bore.
  LobGravity gravity;       ///< @brief Gravity vector.
  LobWind wind;             ///< @brief Wind vector.
  LobCoriolis coriolis;     ///< @brief Coriolis effect parameters.
  double zero_angle;        ///< @brief Angle between sight and trajectory.
  double stability_factor;  ///< @brief Miller stability factor.
  double aerodynamic_jump;  ///< @brief Aerodynamic jump effect in Moa.
  double spindrift_factor;  ///< @brief Spin drift factor.
  uint16_t minimum_speed;   ///< @brief Minimum speed for solver.
  double max_time;          ///< @brief Max time of flight for solver.
  uint16_t step_size;       ///< @brief Step size for solver.
  LobErrorT error;          ///< @brief Builder error field.
} LobInput;

/** @brief Structure holding the output results of a ballistic calculation. */
typedef struct {
  uint32_t range;         ///< @brief Associated range in yards.
  uint16_t velocity;      ///< @brief Calculated velocity in feet per second.
  uint32_t energy;        ///< @brief Calculated energy in foot-pounds.
  double elevation;       ///< @brief Calculated elevation change in inches.
  double deflection;      ///< @brief Calculated windage deflection in inches.
  double time_of_flight;  ///< @brief Time of flight in seconds.
} LobOutput;

/** @brief Opaque builder type for constructing LobInput objects. */
typedef struct {
  union {
    LobInput align_input;
    double align_double;
    size_t align_size;
    uint8_t buffer[LOB_BUILDER_BUFFER_SIZE];
  } buffer;
} LobBuilder;

/** @brief Gets the library version in major.minor.patch format. */
LOB_EXPORT extern const char* LobVersion(void);

/** @brief Initializes a builder to default state. */
LOB_EXPORT extern void LobBuilderInit(LobBuilder* builder);
/** @brief Destroys a builder and releases resources. */
LOB_EXPORT extern void LobBuilderDestroy(LobBuilder* builder);
/** @brief Copies builder state from src to dst. */
LOB_EXPORT extern void LobBuilderCopy(LobBuilder* dst, const LobBuilder* src);
/** @brief Resets a builder state by re-initializing. */
LOB_EXPORT extern LobBuilder* LobBuilderReset(LobBuilder* builder);

/**
 * @brief Sets the ballistic coefficient (Psi).
 * @param builder Pointer to the builder.
 * @param value The ballistic coefficient value.
 * @return Pointer to the builder.
 */
LOB_EXPORT extern LobBuilder* LobBuilderBallisticCoefficientPsi(
    LobBuilder* builder, double value);

/**
 * @brief Sets the atmosphere reference associated with ballistic coefficient.
 * @param builder Pointer to the builder.
 * @param type The atmosphere reference type.
 * @return Pointer to the builder.
 */
LOB_EXPORT extern LobBuilder* LobBuilderBCAtmosphere(
    LobBuilder* builder, LobAtmosphereReferenceT type);

/**
 * @brief Sets the drag function associated with ballistic coefficient.
 * @param builder Pointer to the builder.
 * @param type The drag function type.
 * @return Pointer to the builder.
 */
LOB_EXPORT extern LobBuilder* LobBuilderBCDragFunction(LobBuilder* builder,
                                                       LobDragFunctionT type);

/**
 * @brief Sets the projectile diameter (caliber) in inches.
 * @param builder Pointer to the builder.
 * @param value The diameter in inches.
 * @return Pointer to the builder.
 */
LOB_EXPORT extern LobBuilder* LobBuilderDiameterInch(LobBuilder* builder,
                                                     double value);

/**
 * @brief Sets the projectile meplat diameter in inches.
 * @param builder Pointer to the builder.
 * @param value The meplat in inches.
 * @return Pointer to the builder.
 */
LOB_EXPORT extern LobBuilder* LobBuilderMeplatDiameterInch(LobBuilder* builder,
                                                           double value);

/**
 * @brief Sets the projectile base diameter in inches.
 * @param builder Pointer to the builder.
 * @param value The base diameter in inches.
 * @return Pointer to the builder.
 */
LOB_EXPORT extern LobBuilder* LobBuilderBaseDiameterInch(LobBuilder* builder,
                                                         double value);

/**
 * @brief Sets the projectile length in inches.
 * @param builder Pointer to the builder.
 * @param value The length in inches.
 * @return Pointer to the builder.
 */
LOB_EXPORT extern LobBuilder* LobBuilderLengthInch(LobBuilder* builder,
                                                   double value);

/**
 * @brief Sets the projectile nose length in inches.
 * @param builder Pointer to the builder.
 * @param value The nose length in inches.
 * @return Pointer to the builder.
 */
LOB_EXPORT extern LobBuilder* LobBuilderNoseLengthInch(LobBuilder* builder,
                                                       double value);

/**
 * @brief Sets the projectile tail length in inches.
 * @param builder Pointer to the builder.
 * @param value The tail length in inches.
 * @return Pointer to the builder.
 */
LOB_EXPORT extern LobBuilder* LobBuilderTailLengthInch(LobBuilder* builder,
                                                       double value);

/**
 * @brief Sets the Rt/R ratio of the projectile ogive.
 * @param builder Pointer to the builder.
 * @param value The Rt/R ratio.
 * @return Pointer to the builder.
 */
LOB_EXPORT extern LobBuilder* LobBuilderOgiveRtR(LobBuilder* builder,
                                                 double value);

/**
 * @brief Loads a custom Mach vs Drag table for the projectile.
 * @note This is a direct alternative to using a ballistic coefficient and a
 * reference drag function.
 * @param builder Pointer to the builder.
 * @param pmachs Pointer to an array of mach values.
 * @param pdrags Pointer to an array of associated drag values.
 * @param size The number of mach-drag pairs in the table.
 * @return Pointer to the builder.
 */
LOB_EXPORT extern LobBuilder* LobBuilderMachVsDragTable(LobBuilder* builder,
                                                        const float* pmachs,
                                                        const float* pdrags,
                                                        size_t size);

/**
 * @brief Sets the projectile mass in grains.
 * @param builder Pointer to the builder.
 * @param value The mass in grains.
 * @return Pointer to the builder.
 */
LOB_EXPORT extern LobBuilder* LobBuilderMassGrains(LobBuilder* builder,
                                                   double value);

/**
 * @brief Sets the initial velocity of the projectile in feet per second.
 * @param builder Pointer to the builder.
 * @param value The initial velocity in fps.
 * @return Pointer to the builder.
 */
LOB_EXPORT extern LobBuilder* LobBuilderInitialVelocityFps(LobBuilder* builder,
                                                           uint16_t value);

/**
 * @brief Sets the height of the optic above the bore in inches.
 * @param builder Pointer to the builder.
 * @param value The optic height in inches.
 * @return Pointer to the builder.
 */
LOB_EXPORT extern LobBuilder* LobBuilderOpticHeightInches(LobBuilder* builder,
                                                          double value);

/**
 * @brief Sets the twist rate of the barrel in inches per turn.
 * @note Used to calculate adjustments for spin drift and aerodynamic jump.
 * @param builder Pointer to the builder.
 * @param value The twist rate in inches per turn.
 * @return Pointer to the builder.
 */
LOB_EXPORT extern LobBuilder* LobBuilderTwistInchesPerTurn(LobBuilder* builder,
                                                           double value);

/**
 * @brief Sets the angle between the sight and launch angle used to achieve
 * zero.
 * @note This is a portable zero value useful when firing conditions differ
 * from zeroing conditions.
 * @param builder Pointer to the builder.
 * @param value The zero angle in MOA.
 * @return Pointer to the builder.
 */
LOB_EXPORT extern LobBuilder* LobBuilderZeroAngleMOA(LobBuilder* builder,
                                                     double value);

/**
 * @brief Sets the zero distance in yards.
 * @param builder Pointer to the builder.
 * @param value The zero distance in yards.
 * @return Pointer to the builder.
 */
LOB_EXPORT extern LobBuilder* LobBuilderZeroDistanceYds(LobBuilder* builder,
                                                        double value);

/**
 * @brief Sets the zero impact height in inches.
 * @note This would be used if zeroing three inches high at 100 yards for
 * example.
 * @param builder Pointer to the builder.
 * @param value The zero impact height in inches.
 * @return Pointer to the builder.
 */
LOB_EXPORT extern LobBuilder* LobBuilderZeroImpactHeightInches(
    LobBuilder* builder, double value);

/**
 * @brief Sets the altitude of the firing site in feet.
 * @param builder Pointer to the builder.
 * @param value The altitude in feet.
 * @return Pointer to the builder.
 */
LOB_EXPORT extern LobBuilder* LobBuilderAltitudeOfFiringSiteFt(
    LobBuilder* builder, double value);

/**
 * @brief Sets the air pressure in inches of mercury (inHg).
 * @param builder Pointer to the builder.
 * @param value The air pressure in inHg.
 * @return Pointer to the builder.
 */
LOB_EXPORT extern LobBuilder* LobBuilderAirPressureInHg(LobBuilder* builder,
                                                        double value);

/**
 * @brief Sets the altitude of the location where air pressure was taken in
 * feet.
 * @note This only has an effect if the air pressure was taken from a site
 * other than the firing site with a different altitude such as a nearby
 * weather station.
 * @param builder Pointer to the builder.
 * @param value The altitude in feet.
 * @return Pointer to the builder.
 */
LOB_EXPORT extern LobBuilder* LobBuilderAltitudeOfBarometerFt(
    LobBuilder* builder, double value);

/**
 * @brief Sets the temperature in degrees Fahrenheit.
 * @param builder Pointer to the builder.
 * @param value The temperature in degrees F.
 * @return Pointer to the builder.
 */
LOB_EXPORT extern LobBuilder* LobBuilderTemperatureDegF(LobBuilder* builder,
                                                        double value);

/**
 * @brief Sets the altitude of the location where temperature was taken in
 * feet.
 * @note This only has an effect if the temperature was taken from a site
 * other than the firing site with a different altitude such as a nearby
 * weather station.
 * @param builder Pointer to the builder.
 * @param value The altitude in feet.
 * @return Pointer to the builder.
 */
LOB_EXPORT extern LobBuilder* LobBuilderAltitudeOfThermometerFt(
    LobBuilder* builder, double value);

/**
 * @brief Sets the relative humidity at the firing site in percent.
 * @param builder Pointer to the builder.
 * @param value The relative humidity in percent.
 * @return Pointer to the builder.
 */
LOB_EXPORT extern LobBuilder* LobBuilderRelativeHumidityPercent(
    LobBuilder* builder, double value);

/**
 * @brief Sets the wind heading using a clock angle.
 * @note Twelve O'Clock is pure tailwind, Six O'Clock is a pure headwind.
 * @param builder Pointer to the builder.
 * @param value The wind heading as a clock angle.
 * @return Pointer to the builder.
 */
LOB_EXPORT extern LobBuilder* LobBuilderWindHeading(LobBuilder* builder,
                                                    LobClockAngleT value);

/**
 * @brief Sets the wind heading in degrees.
 * @note 0 is pure tailwind, 180 is pure headwind.
 * @param builder Pointer to the builder.
 * @param value The wind heading in degrees.
 * @return Pointer to the builder.
 */
LOB_EXPORT extern LobBuilder* LobBuilderWindHeadingDeg(LobBuilder* builder,
                                                       double value);

/**
 * @brief Sets the wind speed in feet per second.
 * @param builder Pointer to the builder.
 * @param value The wind speed in fps.
 * @return Pointer to the builder.
 */
LOB_EXPORT extern LobBuilder* LobBuilderWindSpeedFps(LobBuilder* builder,
                                                     double value);

/**
 * @brief Sets the wind speed in miles per hour.
 * @param builder Pointer to the builder.
 * @param value The wind speed in mph.
 * @return Pointer to the builder.
 */
LOB_EXPORT extern LobBuilder* LobBuilderWindSpeedMph(LobBuilder* builder,
                                                     double value);

/**
 * @brief Sets the azimuth (bearing) of the target in degrees.
 * @note Used for making coriolis effect corrections.
 * @param builder Pointer to the builder.
 * @param value The azimuth in degrees.
 * @return Pointer to the builder.
 */
LOB_EXPORT extern LobBuilder* LobBuilderAzimuthDeg(LobBuilder* builder,
                                                   double value);

/**
 * @brief Sets the latitude of the firing location in degrees.
 * @note Used for making coriolis effect corrections.
 * @param builder Pointer to the builder.
 * @param value The latitude in degrees.
 * @return Pointer to the builder.
 */
LOB_EXPORT extern LobBuilder* LobBuilderLatitudeDeg(LobBuilder* builder,
                                                    double value);

/**
 * @brief Sets the range angle (inclination) to the target in degrees.
 * @param builder Pointer to the builder.
 * @param value The range angle in degrees.
 * @return Pointer to the builder.
 */
LOB_EXPORT extern LobBuilder* LobBuilderRangeAngleDeg(LobBuilder* builder,
                                                      double value);

/**
 * @brief Sets the minimum speed threshold for the solver.
 * @param builder Pointer to the builder.
 * @param value The minimum speed in feet per second (fps) at which the solver
 * will stop calculations.
 * @return Pointer to the builder.
 */
LOB_EXPORT extern LobBuilder* LobBuilderMinimumSpeed(LobBuilder* builder,
                                                     uint16_t value);

/**
 * @brief Sets the minimum energy threshold for the solver.
 * @param builder Pointer to the builder.
 * @param value The minimum energy in foot-pounds (ft*lbf) at which the solver
 * will stop calculations.
 * @return Pointer to the builder.
 */
LOB_EXPORT extern LobBuilder* LobBuilderMinimumEnergy(LobBuilder* builder,
                                                      uint16_t value);

/**
 * @brief Sets the maximum time of flight for the solver.
 * @param builder Pointer to the builder.
 * @param value The maximum time in seconds after which the solver will stop
 * calculations.
 * @return Pointer to the builder.
 */
LOB_EXPORT extern LobBuilder* LobBuilderMaximumTime(LobBuilder* builder,
                                                    double value);

/**
 * @brief Sets the step size for the numerical solver.
 * @note If set to zero the solver will use a variable step.
 * @param builder Pointer to the builder.
 * @param value The time step size in microseconds used by the solver.
 * @return Pointer to the builder.
 */
LOB_EXPORT extern LobBuilder* LobBuilderStepSize(LobBuilder* builder,
                                                 uint16_t value);

/**
 * @brief Builds the LobInput object with the configured parameters.
 * @param builder Pointer to the builder.
 * @return The constructed LobInput object.
 */
LOB_EXPORT extern LobInput LobBuilderBuild(LobBuilder* builder);

/**
 * @brief Solves the exterior ballistics problem for a given set of ranges.
 * @param in Pointer to input parameters for the calculation.
 * @param pranges Pointer to an array of ranges (in feet) to solve for.
 * @param pouts Pointer to an array where the output results will be stored.
 * @param size The number of ranges to solve for.
 * @return The number of successful solutions.
 */
LOB_EXPORT extern size_t LobSolve(const LobInput* in, const uint32_t* pranges,
                                  LobOutput* pouts, size_t size);

/** @brief Converts minutes of angle (MOA) to milliradians (MIL).
 * @param value Angle in MOA.
 * @return Equivalent angle in MIL.
 */
LOB_EXPORT extern double LobMoaToMil(double value);
/** @brief Converts minutes of angle (MOA) to degrees.
 * @param value Angle in MOA.
 * @return Equivalent angle in degrees.
 */
LOB_EXPORT extern double LobMoaToDeg(double value);
/** @brief Converts minutes of angle (MOA) to inches per hundred yards (IPHY).
 * @param value Angle in MOA.
 * @return Equivalent angle in IPHY.
 */
LOB_EXPORT extern double LobMoaToIphy(double value);
/**
 * @brief Converts minutes of angle (MOA) to projected inches at a given
 * range in feet.
 * @param value Angle in MOA.
 * @param range_ft Range in feet.
 * @return Equivalent projected inches.
 */
LOB_EXPORT extern double LobMoaToInch(double value, double range_ft);

/** @brief Converts milliradians (MIL) to minutes of angle (MOA).
 * @param value Angle in MIL.
 * @return Equivalent angle in MOA.
 */
LOB_EXPORT extern double LobMilToMoa(double value);
/** @brief Converts milliradians (MIL) to degrees.
 * @param value Angle in MIL.
 * @return Equivalent angle in degrees.
 */
LOB_EXPORT extern double LobMilToDeg(double value);
/** @brief Converts milliradians (MIL) to inches per hundred yards (IPHY).
 * @param value Angle in MIL.
 * @return Equivalent angle in IPHY.
 */
LOB_EXPORT extern double LobMilToIphy(double value);
/**
 * @brief Converts milliradians (MIL) to projected inches at a given
 * range in feet.
 * @param value Angle in MIL.
 * @param range_ft Range in feet.
 * @return Equivalent projected inches.
 */
LOB_EXPORT extern double LobMilToInch(double value, double range_ft);

/** @brief Converts degrees to minutes of angle (MOA).
 * @param value Angle in degrees.
 * @return Equivalent angle in MOA.
 */
LOB_EXPORT extern double LobDegToMoa(double value);
/** @brief Converts degrees to milliradians (MIL).
 * @param value Angle in degrees.
 * @return Equivalent angle in MIL.
 */
LOB_EXPORT extern double LobDegToMil(double value);

/**
 * @brief Inches of projection at a given range to minutes of angle (MOA).
 * @param value Projected inches.
 * @param range_ft Range in feet.
 * @return Equivalent angle in MOA.
 */
LOB_EXPORT extern double LobInchToMoa(double value, double range_ft);
/**
 * @brief Inches of projection at a given range to milliradians (MIL).
 * @param value Projected inches.
 * @param range_ft Range in feet.
 * @return Equivalent angle in MIL.
 */
LOB_EXPORT extern double LobInchToMil(double value, double range_ft);
/**
 * @brief Inches of projection at a given range to degrees.
 * @param value Projected inches.
 * @param range_ft Range in feet.
 * @return Equivalent angle in degrees.
 */
LOB_EXPORT extern double LobInchToDeg(double value, double range_ft);

/** @brief Converts joules to foot-pounds.
 * @param value Energy in joules.
 * @return Equivalent energy in foot-pounds.
 */
LOB_EXPORT extern double LobJToFtLbs(double value);
/** @brief Converts foot-pounds to joules.
 * @param value Energy in foot-pounds.
 * @return Equivalent energy in joules.
 */
LOB_EXPORT extern double LobFtLbsToJ(double value);

/** @brief Converts meters to yards.
 * @param value Length in meters.
 * @return Equivalent length in yards.
 */
LOB_EXPORT extern double LobMToYd(double value);
/** @brief Converts yards to feet.
 * @param value Length in yards.
 * @return Equivalent length in feet.
 */
LOB_EXPORT extern double LobYdToFt(double value);
/** @brief Converts meters to feet.
 * @param value Length in meters.
 * @return Equivalent length in feet.
 */
LOB_EXPORT extern double LobMToFt(double value);
/** @brief Converts feet to inches.
 * @param value Length in feet.
 * @return Equivalent length in inches.
 */
LOB_EXPORT extern double LobFtToIn(double value);
/** @brief Converts millimeters to inches.
 * @param value Length in millimeters.
 * @return Equivalent length in inches.
 */
LOB_EXPORT extern double LobMmToIn(double value);
/** @brief Converts centimeters to inches.
 * @param value Length in centimeters.
 * @return Equivalent length in inches.
 */
LOB_EXPORT extern double LobCmToIn(double value);
/** @brief Converts yards to meters.
 * @param value Length in yards.
 * @return Equivalent length in meters.
 */
LOB_EXPORT extern double LobYdToM(double value);
/** @brief Converts feet to meters.
 * @param value Length in feet.
 * @return Equivalent length in meters.
 */
LOB_EXPORT extern double LobFtToM(double value);
/** @brief Converts feet to yards.
 * @param value Length in feet.
 * @return Equivalent length in yards.
 */
LOB_EXPORT extern double LobFtToYd(double value);
/** @brief Converts inches to millimeters.
 * @param value Length in inches.
 * @return Equivalent length in millimeters.
 */
LOB_EXPORT extern double LobInToMm(double value);
/** @brief Converts inches to centimeters.
 * @param value Length in inches.
 * @return Equivalent length in centimeters.
 */
LOB_EXPORT extern double LobInToCm(double value);
/** @brief Converts inches to feet.
 * @param value Length in inches.
 * @return Equivalent length in feet.
 */
LOB_EXPORT extern double LobInToFt(double value);

/** @brief Converts pascals to inches of mercury.
 * @param value Pressure in pascals.
 * @return Equivalent pressure in inches of mercury.
 */
LOB_EXPORT extern double LobPaToInHg(double value);
/** @brief Converts millibars to inches of mercury.
 * @param value Pressure in millibars.
 * @return Equivalent pressure in inches of mercury.
 */
LOB_EXPORT extern double LobMbarToInHg(double value);
/** @brief Converts pounds per square inch (PSI) to inches of mercury.
 * @param value Pressure in PSI.
 * @return Equivalent pressure in inches of mercury.
 */
LOB_EXPORT extern double LobPsiToInHg(double value);

/** @brief Converts pounds to grains.
 * @param value Mass in pounds.
 * @return Equivalent mass in grains.
 */
LOB_EXPORT extern double LobLbsToGrain(double value);
/** @brief Converts grams to grains.
 * @param value Mass in grams.
 * @return Equivalent mass in grains.
 */
LOB_EXPORT extern double LobGToGrain(double value);
/** @brief Converts kilograms to grains.
 * @param value Mass in kilograms.
 * @return Equivalent mass in grains.
 */
LOB_EXPORT extern double LobKgToGrain(double value);

/**
 * @brief Converts kilograms per square meter to pounds mass per square inch.
 * @param value Sectional density in Kg/m^2.
 * @return Equivalent sectional density in lb/in^2.
 */
LOB_EXPORT extern double LobKgSqMToPmsi(double value);

/** @brief Converts feet per second to meters per second.
 * @param value Speed in feet per second.
 * @return Equivalent speed in meters per second.
 */
LOB_EXPORT extern double LobFpsToMps(double value);
/** @brief Converts meters per second to feet per second.
 * @param value Speed in meters per second.
 * @return Equivalent speed in feet per second.
 */
LOB_EXPORT extern double LobMpsToFps(double value);
/** @brief Converts kilometers per hour to miles per hour.
 * @param value Speed in kilometers per hour.
 * @return Equivalent speed in miles per hour.
 */
LOB_EXPORT extern double LobKphToMph(double value);
/** @brief Converts Knots to miles per hour.
 * @param value Speed in Knots.
 * @return Equivalent speed in miles per hour.
 */
LOB_EXPORT extern double LobKnToMph(double value);

/** @brief Converts milliseconds to seconds.
 * @param value Time in milliseconds.
 * @return Equivalent time in seconds.
 */
LOB_EXPORT extern double LobMsToS(double value);
/** @brief Converts microseconds to seconds.
 * @param value Time in microseconds.
 * @return Equivalent time in seconds.
 */
LOB_EXPORT extern double LobUsToS(double value);
/** @brief Converts seconds to milliseconds.
 * @param value Time in seconds.
 * @return Equivalent time in milliseconds.
 */
LOB_EXPORT extern double LobSToMs(double value);
/** @brief Converts seconds to microseconds.
 * @param value Time in seconds.
 * @return Equivalent time in microseconds.
 */
LOB_EXPORT extern double LobSToUs(double value);

/**
 * @brief Converts degrees celsius to degrees fahrenheit.
 * @param value Temperature in degrees celsius.
 * @return Equivalent temperature in degrees fahrenheit.
 */
LOB_EXPORT extern double LobDegCToDegF(double value);

#ifdef __cplusplus
// NOLINTEND
}
#endif

#endif  // LOB_H

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
