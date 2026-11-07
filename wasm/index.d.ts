// Copyright (c) 2026  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

/**
 * Trajectory output row returned by forward solve methods (elevation and
 * deflection in inches).
 */
export interface ForwardTrajectoryRow {
  /** Range from the firing point, in whole feet. */
  readonly rangeFt: number;
  /** Range from the firing point, in yards. */
  readonly rangeYds: number;
  /** Projectile velocity at range, in feet per second. */
  readonly velocityFps: number;
  /** Projectile kinetic energy at range, in foot-pounds. */
  readonly energyFtLbs: number;
  /** Vertical drop/elevation relative to line of sight, in inches. */
  readonly elevationInch: number;
  /** Horizontal deflection relative to line of sight, in inches. */
  readonly deflectionInch: number;
  /** Elapsed time of flight from launch to range, in seconds. */
  readonly timeOfFlightSec: number;
}

/**
 * Trajectory output row returned by inverse solve methods (elevation and
 * deflection in MOA sight adjustments).
 */
export interface InverseTrajectoryRow {
  /** Range from the firing point, in whole feet. */
  readonly rangeFt: number;
  /** Range from the firing point, in yards. */
  readonly rangeYds: number;
  /** Projectile velocity at range, in feet per second. */
  readonly velocityFps: number;
  /** Projectile kinetic energy at range, in foot-pounds. */
  readonly energyFtLbs: number;
  /** Required sight elevation angle adjustment, in Minutes of Angle (MOA). */
  readonly elevationMoa: number;
  /** Required windage/deflection angle adjustment, in Minutes of Angle (MOA). */
  readonly deflectionMoa: number;
  /** Elapsed time of flight from launch to range, in seconds. */
  readonly timeOfFlightSec: number;
}

/**
 * A discrete wind measurement point along the trajectory.
 */
export interface WindPoint {
  /** Distance downrange from firing site, in feet. */
  readonly rangeFt: number;
  /** Wind direction angle, in degrees clockwise from line of fire (0..360). */
  readonly headingDeg: number;
  /** Wind velocity, in miles per hour. */
  readonly speedMph: number;
  /** Optional altitude of measurement above ground, in feet. */
  readonly heightFt?: number;
}

/** Standard drag functions. */
export const DragFunction: {
  readonly G1: 1;
  readonly G2: 2;
  readonly G5: 5;
  readonly G6: 6;
  readonly G7: 7;
  readonly G8: 8;
};

export type DragFunctionValue =
  (typeof DragFunction)[keyof typeof DragFunction];
export type DragFunctionKey = keyof typeof DragFunction;
export type DragFunctionInput = DragFunctionValue | DragFunctionKey;

/** Atmospheric model reference standards for ballistic coefficient. */
export const Atmosphere: {
  readonly ArmyStandardMetro: 0;
  readonly ICAO: 1;
};

export type AtmosphereValue = (typeof Atmosphere)[keyof typeof Atmosphere];
export type AtmosphereKey = keyof typeof Atmosphere;
export type AtmosphereInput = AtmosphereValue | AtmosphereKey;

/** Error thrown when the builder rejects invalid physical configuration inputs. */
export class LobError extends Error {
  readonly code: number;
  constructor(code: number);
}

/** Sequence of numbers, such as an Array or numeric TypedArray. */
export type NumberSequence =
  | readonly number[]
  | Float64Array
  | Float32Array
  | Int32Array
  | Int16Array
  | Int8Array
  | Uint32Array
  | Uint16Array
  | Uint8Array
  | Uint8ClampedArray;

/**
 * Fluent builder for configuring external ballistics simulation parameters.
 */
export class Builder {
  constructor();

  /** Set ballistic coefficient in standard sectional density units (lb/in^2). */
  ballisticCoefficientPsi(value: number): this;

  /** Set reference atmosphere used for ballistic coefficient. */
  bcAtmosphere(value: AtmosphereInput): this;

  /** Set drag model standard used for ballistic coefficient. */
  bcDragFunction(value: DragFunctionInput): this;

  /** Set projectile diameter (caliber), in inches. */
  diameterInch(value: number): this;

  /** Set projectile meplat (tip flat) diameter, in inches. */
  meplatDiameterInch(value: number): this;

  /** Set projectile base diameter, in inches. */
  baseDiameterInch(value: number): this;

  /** Set total projectile length, in inches. */
  lengthInch(value: number): this;

  /** Set projectile nose length, in inches. */
  noseLengthInch(value: number): this;

  /** Set projectile boat-tail length, in inches. */
  tailLengthInch(value: number): this;

  /** Set projectile ogive radius-to-caliber ratio (Rt/R). */
  ogiveRtR(value: number): this;

  /** Load a custom Mach vs Drag coefficient table. */
  machVsDragTable(machs: NumberSequence, drags: NumberSequence): this;

  /** Load stepped ballistic coefficients across velocity bands (fps). */
  bcVelocityBands(fps: NumberSequence, bcs: NumberSequence): this;

  /** Load custom 60-element cubic spline drag table coefficients. */
  splineCoefficients(coefs: NumberSequence): this;

  /** Set projectile mass, in grains. */
  massGrains(value: number): this;

  /** Set projectile muzzle velocity, in feet per second (0..65535). */
  initialVelocityFps(value: number): this;

  /** Set sight height above bore axis, in inches. */
  opticHeightInches(value: number): this;

  /** Set barrel rifling twist rate, in inches per turn. Positive is right-hand. */
  twistInchesPerTurn(value: number): this;

  /** Set bore sight angle relative to optical axis, in MOA. */
  zeroAngleMoa(value: number): this;

  /** Set zero distance, in yards. */
  zeroDistanceYds(value: number): this;

  /** Set point of impact offset at zero range, in inches. */
  zeroImpactHeightInches(value: number): this;

  /** Set firing site altitude above sea level, in feet. */
  altitudeOfFiringSiteFt(value: number): this;

  /** Set ambient station air pressure, in inches of mercury (inHg). */
  airPressureInHg(value: number): this;

  /** Set altitude of barometer station, in feet. */
  altitudeOfBarometerFt(value: number): this;

  /** Set ambient temperature, in degrees Fahrenheit. */
  temperatureDegF(value: number): this;

  /** Set altitude of thermometer station, in feet. */
  altitudeOfThermometerFt(value: number): this;

  /** Set ambient relative humidity percentage (0..100). */
  relativeHumidityPercent(value: number): this;

  /** Set wind direction by clock angle (1..12). 12 is headwind, 3 is right-to-left. */
  windHeading(value: number): this;

  /** Set wind direction, in degrees clockwise from line of fire (0..360). */
  windHeadingDeg(value: number): this;

  /** Set wind speed, in feet per second. */
  windSpeedFps(value: number): this;

  /** Set wind speed, in miles per hour. */
  windSpeedMph(value: number): this;

  /** Set multi-point downrange wind profile. */
  windProfile(points: readonly WindPoint[]): this;

  /** Set atmospheric wind shear power law exponent. */
  windShearExponent(value: number): this;

  /** Set azimuth of line of fire from true North, in degrees (0..360). */
  azimuthDeg(value: number): this;

  /** Set firing location latitude, in degrees (-90..90). */
  latitudeDeg(value: number): this;

  /** Set elevation angle to target above horizontal, in degrees (-90..90). */
  rangeAngleDeg(value: number): this;

  /** Set minimum projectile velocity before trajectory halts, in fps (0..65535). */
  minimumSpeed(value: number): this;

  /** Set minimum projectile kinetic energy before trajectory halts, in ft-lbs (0..65535). */
  minimumEnergy(value: number): this;

  /** Set maximum flight time before trajectory halts, in seconds. */
  maximumTime(value: number): this;

  /** Set integration step size, in microseconds (0..65535). */
  stepSize(value: number): this;

  /** Reset all configuration values to defaults and release retained array buffers. */
  reset(): this;

  /** Validate configuration and produce a compiled ballistics Context. */
  build(): Context;

  /** Explicitly release WebAssembly resources. Idempotent. */
  dispose(): void;
}

/**
 * Compiled ballistic solver context.
 */
export class Context {
  private constructor();

  /** Forward solve for given downrange distances in feet. */
  solveFt(rangesFt: NumberSequence): ForwardTrajectoryRow[];

  /** Forward solve for given downrange distances in yards. */
  solveYds(rangesYd: NumberSequence): ForwardTrajectoryRow[];

  /** Extended Long Range (ELR) forward solve in feet with continuous atmosphere recalculation. */
  solveELRFt(rangesFt: NumberSequence): ForwardTrajectoryRow[];

  /** Extended Long Range (ELR) forward solve in yards with continuous atmosphere recalculation. */
  solveELRYds(rangesYd: NumberSequence): ForwardTrajectoryRow[];

  /** Geometric inverse solve in feet returning MOA sight adjustments. */
  solveInverseFt(rangesFt: NumberSequence): InverseTrajectoryRow[];

  /** Geometric inverse solve in yards returning MOA sight adjustments. */
  solveInverseYds(rangesYd: NumberSequence): InverseTrajectoryRow[];

  /** Extended Long Range (ELR) inverse solve in feet returning MOA adjustments. */
  solveInverseELRFt(rangesFt: NumberSequence): InverseTrajectoryRow[];

  /** Extended Long Range (ELR) inverse solve in yards returning MOA adjustments. */
  solveInverseELRYds(rangesYd: NumberSequence): InverseTrajectoryRow[];

  /** Explicitly release WebAssembly resources. Idempotent. */
  dispose(): void;
}

/** Returns the library semantic version string. */
export function version(): string;

export function moaToMil(value: number): number;
export function moaToDeg(value: number): number;
export function moaToIphy(value: number): number;
export function moaToInch(value: number, rangeFt: number): number;
export function milToMoa(value: number): number;
export function milToDeg(value: number): number;
export function milToIphy(value: number): number;
export function milToInch(value: number, rangeFt: number): number;
export function degToMoa(value: number): number;
export function degToMil(value: number): number;
export function inchToMoa(value: number, rangeFt: number): number;
export function inchToMil(value: number, rangeFt: number): number;
export function inchToDeg(value: number, rangeFt: number): number;
export function jToFtLbs(value: number): number;
export function ftLbsToJ(value: number): number;
export function mToYd(value: number): number;
export function ydToFt(value: number): number;
export function mToFt(value: number): number;
export function ftToIn(value: number): number;
export function mmToIn(value: number): number;
export function cmToIn(value: number): number;
export function ydToM(value: number): number;
export function ftToM(value: number): number;
export function ftToYd(value: number): number;
export function inToMm(value: number): number;
export function inToCm(value: number): number;
export function inToFt(value: number): number;
export function paToInHg(value: number): number;
export function mbarToInHg(value: number): number;
export function psiToInHg(value: number): number;
export function lbsToGrain(value: number): number;
export function gToGrain(value: number): number;
export function kgToGrain(value: number): number;
export function kgSqMToPmsi(value: number): number;
export function fpsToMps(value: number): number;
export function mpsToFps(value: number): number;
export function kphToMph(value: number): number;
export function knToMph(value: number): number;
export function msToS(value: number): number;
export function usToS(value: number): number;
export function sToMs(value: number): number;
export function sToUs(value: number): number;
export function degCToDegF(value: number): number;

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
