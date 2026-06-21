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
#endif

#define LOB_TABLE_SIZE 85
#define LOB_BUILDER_BUFFER_SIZE 576

typedef uint8_t LobDragFunctionT;
enum {
  kLobDragFunctionG1 = 1,
  kLobDragFunctionG2 = 2,
  kLobDragFunctionG5 = 5,
  kLobDragFunctionG6 = 6,
  kLobDragFunctionG7 = 7,
  kLobDragFunctionG8 = 8
};

typedef uint8_t LobAtmosphereReferenceT;
enum {
  kLobAtmosphereReferenceArmyStandardMetro = 0,
  kLobAtmosphereReferenceIcao = 1
};

typedef uint8_t LobClockAngleT;
enum {
  kLobClockAngleI = 1,
  kkLobClockAngleII = 2,
  kkkLobClockAngleIII = 3,
  kkLobClockAngleIV = 4,
  kLobClockAngleV = 5,
  kkLobClockAngleVI = 6,
  kkkLobClockAngleVII = 7,
  kkkkLobClockAngleVIII = 8,
  kkLobClockAngleIX = 9,
  kLobClockAngleX = 10,
  kkLobClockAngleXI = 11,
  kkkLobClockAngleXII = 12
};

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

typedef struct {
  double x;
  double y;
} LobGravity;

typedef struct {
  double x;
  double z;
} LobWind;

typedef struct {
  double cos_l_sin_a;
  double sin_l;
  double cos_l_cos_a;
} LobCoriolis;

typedef struct {
  uint16_t drags[LOB_TABLE_SIZE];
  double table_coefficient;
  double speed_of_sound;
  uint16_t velocity;
  double mass;
  double optic_height;
  LobGravity gravity;
  LobWind wind;
  LobCoriolis coriolis;
  double zero_angle;
  double stability_factor;
  double aerodynamic_jump;
  double spindrift_factor;
  uint16_t minimum_speed;
  double max_time;
  uint16_t step_size;
  LobErrorT error;
} LobInput;

typedef struct {
  uint32_t range;
  uint16_t velocity;
  uint32_t energy;
  double elevation;
  double deflection;
  double time_of_flight;
} LobOutput;

typedef struct {
  union {
    LobInput align_input;
    double align_double;
    size_t align_size;
    uint8_t buffer[LOB_BUILDER_BUFFER_SIZE];
  } data;
} LobBuilder;

LOB_EXPORT extern const char* LobVersion(void);

LOB_EXPORT extern void LobBuilderInit(LobBuilder* builder);
LOB_EXPORT extern void LobBuilderDestroy(LobBuilder* builder);
LOB_EXPORT extern void LobBuilderCopy(LobBuilder* dst, const LobBuilder* src);
LOB_EXPORT extern LobBuilder* LobBuilderReset(LobBuilder* builder);

LOB_EXPORT extern LobBuilder* LobBuilderBallisticCoefficientPsi(
    LobBuilder* builder, double value);
LOB_EXPORT extern LobBuilder* LobBuilderBCAtmosphere(
    LobBuilder* builder, LobAtmosphereReferenceT type);
LOB_EXPORT extern LobBuilder* LobBuilderBCDragFunction(
    LobBuilder* builder, LobDragFunctionT type);
LOB_EXPORT extern LobBuilder* LobBuilderDiameterInch(
    LobBuilder* builder, double value);
LOB_EXPORT extern LobBuilder* LobBuilderMeplatDiameterInch(
    LobBuilder* builder, double value);
LOB_EXPORT extern LobBuilder* LobBuilderBaseDiameterInch(
    LobBuilder* builder, double value);
LOB_EXPORT extern LobBuilder* LobBuilderLengthInch(
    LobBuilder* builder, double value);
LOB_EXPORT extern LobBuilder* LobBuilderNoseLengthInch(
    LobBuilder* builder, double value);
LOB_EXPORT extern LobBuilder* LobBuilderTailLengthInch(
    LobBuilder* builder, double value);
LOB_EXPORT extern LobBuilder* LobBuilderOgiveRtR(
    LobBuilder* builder, double value);
LOB_EXPORT extern LobBuilder* LobBuilderMachVsDragTable(
    LobBuilder* builder, const float* pmachs, const float* pdrags, size_t size);
LOB_EXPORT extern LobBuilder* LobBuilderMassGrains(
    LobBuilder* builder, double value);
LOB_EXPORT extern LobBuilder* LobBuilderInitialVelocityFps(
    LobBuilder* builder, uint16_t value);
LOB_EXPORT extern LobBuilder* LobBuilderOpticHeightInches(
    LobBuilder* builder, double value);
LOB_EXPORT extern LobBuilder* LobBuilderTwistInchesPerTurn(
    LobBuilder* builder, double value);
LOB_EXPORT extern LobBuilder* LobBuilderZeroAngleMOA(
    LobBuilder* builder, double value);
LOB_EXPORT extern LobBuilder* LobBuilderZeroDistanceYds(
    LobBuilder* builder, double value);
LOB_EXPORT extern LobBuilder* LobBuilderZeroImpactHeightInches(
    LobBuilder* builder, double value);
LOB_EXPORT extern LobBuilder* LobBuilderAltitudeOfFiringSiteFt(
    LobBuilder* builder, double value);
LOB_EXPORT extern LobBuilder* LobBuilderAirPressureInHg(
    LobBuilder* builder, double value);
LOB_EXPORT extern LobBuilder* LobBuilderAltitudeOfBarometerFt(
    LobBuilder* builder, double value);
LOB_EXPORT extern LobBuilder* LobBuilderTemperatureDegF(
    LobBuilder* builder, double value);
LOB_EXPORT extern LobBuilder* LobBuilderAltitudeOfThermometerFt(
    LobBuilder* builder, double value);
LOB_EXPORT extern LobBuilder* LobBuilderRelativeHumidityPercent(
    LobBuilder* builder, double value);
LOB_EXPORT extern LobBuilder* LobBuilderWindHeading(
    LobBuilder* builder, LobClockAngleT value);
LOB_EXPORT extern LobBuilder* LobBuilderWindHeadingDeg(
    LobBuilder* builder, double value);
LOB_EXPORT extern LobBuilder* LobBuilderWindSpeedFps(
    LobBuilder* builder, double value);
LOB_EXPORT extern LobBuilder* LobBuilderWindSpeedMph(
    LobBuilder* builder, double value);
LOB_EXPORT extern LobBuilder* LobBuilderAzimuthDeg(
    LobBuilder* builder, double value);
LOB_EXPORT extern LobBuilder* LobBuilderLatitudeDeg(
    LobBuilder* builder, double value);
LOB_EXPORT extern LobBuilder* LobBuilderRangeAngleDeg(
    LobBuilder* builder, double value);
LOB_EXPORT extern LobBuilder* LobBuilderMinimumSpeed(
    LobBuilder* builder, uint16_t value);
LOB_EXPORT extern LobBuilder* LobBuilderMinimumEnergy(
    LobBuilder* builder, uint16_t value);
LOB_EXPORT extern LobBuilder* LobBuilderMaximumTime(
    LobBuilder* builder, double value);
LOB_EXPORT extern LobBuilder* LobBuilderStepSize(
    LobBuilder* builder, uint16_t value);

LOB_EXPORT extern LobInput LobBuilderBuild(LobBuilder* builder);

LOB_EXPORT extern size_t LobSolve(
    const LobInput* in, const uint32_t* pranges,
    LobOutput* pouts, size_t size);

LOB_EXPORT extern double LobMoaToMil(double value);
LOB_EXPORT extern double LobMoaToDeg(double value);
LOB_EXPORT extern double LobMoaToIphy(double value);
LOB_EXPORT extern double LobMoaToInch(double value, double range_ft);

LOB_EXPORT extern double LobMilToMoa(double value);
LOB_EXPORT extern double LobMilToDeg(double value);
LOB_EXPORT extern double LobMilToIphy(double value);
LOB_EXPORT extern double LobMilToInch(double value, double range_ft);

LOB_EXPORT extern double LobDegToMoa(double value);
LOB_EXPORT extern double LobDegToMil(double value);

LOB_EXPORT extern double LobInchToMoa(double value, double range_ft);
LOB_EXPORT extern double LobInchToMil(double value, double range_ft);
LOB_EXPORT extern double LobInchToDeg(double value, double range_ft);

LOB_EXPORT extern double LobJToFtLbs(double value);
LOB_EXPORT extern double LobFtLbsToJ(double value);

LOB_EXPORT extern double LobMToYd(double value);
LOB_EXPORT extern double LobYdToFt(double value);
LOB_EXPORT extern double LobMToFt(double value);
LOB_EXPORT extern double LobFtToIn(double value);
LOB_EXPORT extern double LobMmToIn(double value);
LOB_EXPORT extern double LobCmToIn(double value);
LOB_EXPORT extern double LobYdToM(double value);
LOB_EXPORT extern double LobFtToM(double value);
LOB_EXPORT extern double LobFtToYd(double value);
LOB_EXPORT extern double LobInToMm(double value);
LOB_EXPORT extern double LobInToCm(double value);
LOB_EXPORT extern double LobInToFt(double value);

LOB_EXPORT extern double LobPaToInHg(double value);
LOB_EXPORT extern double LobMbarToInHg(double value);
LOB_EXPORT extern double LobPsiToInHg(double value);

LOB_EXPORT extern double LobLbsToGrain(double value);
LOB_EXPORT extern double LobGToGrain(double value);
LOB_EXPORT extern double LobKgToGrain(double value);

LOB_EXPORT extern double LobKgSqMToPmsi(double value);

LOB_EXPORT extern double LobFpsToMps(double value);
LOB_EXPORT extern double LobMpsToFps(double value);
LOB_EXPORT extern double LobKphToMph(double value);
LOB_EXPORT extern double LobKnToMph(double value);

LOB_EXPORT extern double LobMsToS(double value);
LOB_EXPORT extern double LobUsToS(double value);
LOB_EXPORT extern double LobSToMs(double value);
LOB_EXPORT extern double LobSToUs(double value);

LOB_EXPORT extern double LobDegCToDegF(double value);

#ifdef __cplusplus
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
