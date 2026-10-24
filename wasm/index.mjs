// Copyright (c) 2026  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

import createLobModule from "./lob.mjs";

const wasm = await createLobModule();

const BUILDER_SIZE = wasm._LobWasmBuilderSize();
const CONTEXT_SIZE = wasm._LobWasmContextSize();
const OUTPUT_SIZE = wasm._LobWasmOutputSize();
const WIND_POINT_SIZE = wasm._LobWasmWindPointSize();
const OUTPUT_FIELDS = 6;
const UINT32_MAX = 0xffffffff;

const ERROR_NAMES = [
  "None",
  "AirPressureOOR",
  "AltitudeOfBarometerOOR",
  "AltitudeOfFiringSiteOOR",
  "AltitudeOfThermometerOOR",
  "AzimuthOOR",
  "BallisticCoefficientOOR",
  "BallisticCoefficientRequired",
  "BaseDiameterOOR",
  "BcBandsInvalid",
  "BcBandsNotMonotonic",
  "BcBandsTooShort",
  "DiameterOOR",
  "HumidityOOR",
  "InitialVelocityRequired",
  "InternalError",
  "LatitudeOOR",
  "LaunchAngleRequired",
  "LengthOOR",
  "MachDragTableInvalid",
  "MachDragTableNotMonotonic",
  "MachDragTableTooShort",
  "MassOOR",
  "MaximumTimeOOR",
  "MeplatDiameterOOR",
  "NoseLengthOOR",
  "NotFormed",
  "OgiveRtROOR",
  "RangeAngleOOR",
  "SplineCoefsInvalid",
  "TailLengthOOR",
  "WindHeadingOOR",
  "WindProfileInvalid",
  "WindProfileNotMonotonic",
  "WindProfileTooLong",
  "ZeroAngleOOR",
  "ZeroDistanceOOR",
  "ZeroUnreachable",
  "NumberOfErrors",
];

const registry = new FinalizationRegistry(({ kind, ptr, buffers }) => {
  if (kind === "builder" && ptr !== 0) {
    wasm._LobBuilderDestroy(ptr);
  }
  for (const buffer of buffers) {
    wasm._free(buffer);
  }
  wasm._free(ptr);
});

/** @brief Drag functions accepted by {@link Builder.bcDragFunction}. */
export const DragFunction = Object.freeze({
  G1: 1,
  G2: 2,
  G5: 5,
  G6: 6,
  G7: 7,
  G8: 8,
});

/** @brief BC atmosphere references accepted by {@link Builder.bcAtmosphere}. */
export const Atmosphere = Object.freeze({
  ArmyStandardMetro: 0,
  ICAO: 1,
});

/** @brief Error thrown when the builder rejects its inputs. */
export class LobError extends Error {
  constructor(code) {
    const name = ERROR_NAMES[code] ?? "UnknownError";
    super(`lob: ${name} (code ${code})`);
    this.name = "LobError";
    this.code = code;
  }
}

function enumValue(value, values, name) {
  if (typeof value === "string" && Object.hasOwn(values, value)) {
    return values[value];
  }
  if (typeof value === "number" && Object.values(values).includes(value)) {
    return value;
  }
  throw new RangeError(`lob: invalid ${name}: ${value}`);
}

function uint16(value, name) {
  if (!Number.isInteger(value) || value < 0 || value > 0xffff) {
    throw new RangeError(`lob: ${name} must be an integer in 0..65535`);
  }
  return value;
}

function finiteNumber(value, name) {
  if (typeof value !== "number" || !Number.isFinite(value)) {
    throw new RangeError(`lob: ${name} must be a finite number`);
  }
  return value;
}

function numbers(values, name) {
  const array = Array.isArray(values)
    ? values
    : ArrayBuffer.isView(values)
      ? Array.from(values)
      : null;
  if (array === null) {
    throw new TypeError(`lob: ${name} must be an array of numbers`);
  }
  return array.map((value, index) => finiteNumber(value, `${name}[${index}]`));
}

function rangesToFeet(values, yards) {
  if (!Array.isArray(values) && !ArrayBuffer.isView(values)) {
    throw new TypeError("lob: ranges must be an array of numbers");
  }
  const feet = [];
  let previous = -1;
  for (const [index, value] of Array.from(values).entries()) {
    if (typeof value !== "number" || !Number.isFinite(value) || value < 0) {
      throw new RangeError(
        `lob: ranges[${index}] must be a finite non-negative number`,
      );
    }
    const range = yards ? Math.round(value * 3) : value;
    if (!Number.isInteger(range) || range > UINT32_MAX) {
      throw new RangeError(`lob: ranges[${index}] is out of range`);
    }
    if (range <= previous) {
      throw new RangeError("lob: ranges must be strictly ascending");
    }
    previous = range;
    feet.push(range);
  }
  return feet;
}

function readRows(count, pointer, inverse) {
  const values = new Float64Array(
    wasm.HEAPF64.buffer,
    pointer,
    count * OUTPUT_FIELDS,
  );
  const elevation = inverse ? "elevationMoa" : "elevationInch";
  const deflection = inverse ? "deflectionMoa" : "deflectionInch";
  const rows = [];
  for (let i = 0; i < count; i++) {
    const base = i * OUTPUT_FIELDS;
    rows.push({
      rangeFt: values[base],
      rangeYds: wasm._LobFtToYd(values[base]),
      velocityFps: values[base + 1],
      energyFtLbs: values[base + 2],
      [elevation]: values[base + 3],
      [deflection]: values[base + 4],
      timeOfFlightSec: values[base + 5],
    });
  }
  return rows;
}

/** @brief Chainable configuration object that builds a {@link Context}. */
export class Builder {
  #ptr;
  #buffers;

  constructor() {
    this.#ptr = wasm._malloc(BUILDER_SIZE);
    if (this.#ptr === 0) {
      throw new Error("lob: out of wasm memory");
    }
    this.#buffers = [];
    wasm._LobBuilderInit(this.#ptr);
    registry.register(
      this,
      { kind: "builder", ptr: this.#ptr, buffers: this.#buffers },
      this,
    );
  }

  #assert() {
    if (this.#ptr === 0) {
      throw new Error("lob: Builder has been disposed");
    }
  }

  #call(name, ...args) {
    this.#assert();
    wasm[name](this.#ptr, ...args);
    return this;
  }

  #floatBuffer(values, name) {
    this.#assert();
    const pointer = wasm._malloc(
      values.length * Float32Array.BYTES_PER_ELEMENT,
    );
    if (pointer === 0) {
      throw new Error(`lob: out of wasm memory for ${name}`);
    }
    this.#buffers.push(pointer);
    new Float32Array(wasm.HEAPF32.buffer, pointer, values.length).set(values);
    return pointer;
  }

  #freeBuffers() {
    for (const pointer of this.#buffers) {
      wasm._free(pointer);
    }
    this.#buffers.length = 0;
  }

  ballisticCoefficientPsi(value) {
    return this.#call("_LobBuilderBallisticCoefficientPsi", value);
  }

  bcAtmosphere(value) {
    return this.#call(
      "_LobBuilderBCAtmosphere",
      enumValue(value, Atmosphere, "bcAtmosphere"),
    );
  }

  bcDragFunction(value) {
    return this.#call(
      "_LobBuilderBCDragFunction",
      enumValue(value, DragFunction, "bcDragFunction"),
    );
  }

  diameterInch(value) {
    return this.#call("_LobBuilderDiameterInch", value);
  }

  meplatDiameterInch(value) {
    return this.#call("_LobBuilderMeplatDiameterInch", value);
  }

  baseDiameterInch(value) {
    return this.#call("_LobBuilderBaseDiameterInch", value);
  }

  lengthInch(value) {
    return this.#call("_LobBuilderLengthInch", value);
  }

  noseLengthInch(value) {
    return this.#call("_LobBuilderNoseLengthInch", value);
  }

  tailLengthInch(value) {
    return this.#call("_LobBuilderTailLengthInch", value);
  }

  ogiveRtR(value) {
    return this.#call("_LobBuilderOgiveRtR", value);
  }

  machVsDragTable(machs, drags) {
    const machValues = numbers(machs, "machs");
    const dragValues = numbers(drags, "drags");
    if (machValues.length !== dragValues.length) {
      throw new RangeError("lob: machs and drags must have the same length");
    }
    const machPointer = this.#floatBuffer(machValues, "machs");
    const dragPointer = this.#floatBuffer(dragValues, "drags");
    return this.#call(
      "_LobBuilderSplineFitTable",
      machPointer,
      dragPointer,
      machValues.length,
    );
  }

  bcVelocityBands(fps, bcs) {
    const fpsValues = numbers(fps, "fps");
    const bcValues = numbers(bcs, "bcs");
    if (fpsValues.length !== bcValues.length) {
      throw new RangeError("lob: fps and bcs must have the same length");
    }
    const fpsPointer = this.#floatBuffer(fpsValues, "fps");
    const bcPointer = this.#floatBuffer(bcValues, "bcs");
    return this.#call(
      "_LobBuilderBCVelocityBands",
      fpsPointer,
      bcPointer,
      fpsValues.length,
    );
  }

  splineCoefficients(coefs) {
    const values = numbers(coefs, "coefs");
    if (values.length !== 60) {
      throw new RangeError("lob: coefs must have 60 entries");
    }
    return this.#call(
      "_LobBuilderSplineCoefficients",
      this.#floatBuffer(values, "coefs"),
    );
  }

  massGrains(value) {
    return this.#call("_LobBuilderMassGrains", value);
  }

  initialVelocityFps(value) {
    return this.#call(
      "_LobBuilderInitialVelocityFps",
      uint16(value, "initialVelocityFps"),
    );
  }

  opticHeightInches(value) {
    return this.#call("_LobBuilderOpticHeightInches", value);
  }

  twistInchesPerTurn(value) {
    return this.#call("_LobBuilderTwistInchesPerTurn", value);
  }

  zeroAngleMoa(value) {
    return this.#call("_LobBuilderZeroAngleMOA", value);
  }

  zeroDistanceYds(value) {
    return this.#call("_LobBuilderZeroDistanceYds", value);
  }

  zeroImpactHeightInches(value) {
    return this.#call("_LobBuilderZeroImpactHeightInches", value);
  }

  altitudeOfFiringSiteFt(value) {
    return this.#call("_LobBuilderAltitudeOfFiringSiteFt", value);
  }

  airPressureInHg(value) {
    return this.#call("_LobBuilderAirPressureInHg", value);
  }

  altitudeOfBarometerFt(value) {
    return this.#call("_LobBuilderAltitudeOfBarometerFt", value);
  }

  temperatureDegF(value) {
    return this.#call("_LobBuilderTemperatureDegF", value);
  }

  altitudeOfThermometerFt(value) {
    return this.#call("_LobBuilderAltitudeOfThermometerFt", value);
  }

  relativeHumidityPercent(value) {
    return this.#call("_LobBuilderRelativeHumidityPercent", value);
  }

  windHeading(value) {
    if (!Number.isInteger(value) || value < 1 || value > 12) {
      throw new RangeError("lob: windHeading must be an integer in 1..12");
    }
    return this.#call("_LobBuilderWindHeading", value);
  }

  windHeadingDeg(value) {
    return this.#call("_LobBuilderWindHeadingDeg", value);
  }

  windSpeedFps(value) {
    return this.#call("_LobBuilderWindSpeedFps", value);
  }

  windSpeedMph(value) {
    return this.#call("_LobBuilderWindSpeedMph", value);
  }

  windProfile(points) {
    if (!Array.isArray(points)) {
      throw new TypeError("lob: points must be an array");
    }
    const rows = points.map((point, index) => {
      if (typeof point !== "object" || point === null) {
        throw new TypeError(`lob: points[${index}] must be an object`);
      }
      const height = point.heightFt ?? NaN;
      return [
        finiteNumber(point.rangeFt, `points[${index}].rangeFt`),
        finiteNumber(point.headingDeg, `points[${index}].headingDeg`),
        finiteNumber(point.speedMph, `points[${index}].speedMph`),
        Number.isNaN(height)
          ? height
          : finiteNumber(height, `points[${index}].heightFt`),
      ];
    });
    this.#assert();
    const pointer = wasm._malloc(points.length * WIND_POINT_SIZE);
    if (pointer === 0) {
      throw new Error("lob: out of wasm memory for wind profile");
    }
    this.#buffers.push(pointer);
    const view = new Float64Array(
      wasm.HEAPF64.buffer,
      pointer,
      points.length * 4,
    );
    for (const [index, row] of rows.entries()) {
      view.set(row, index * 4);
    }
    return this.#call("_LobBuilderWindProfile", pointer, points.length);
  }

  windShearExponent(value) {
    return this.#call("_LobBuilderWindShearExponent", value);
  }

  azimuthDeg(value) {
    return this.#call("_LobBuilderAzimuthDeg", value);
  }

  latitudeDeg(value) {
    return this.#call("_LobBuilderLatitudeDeg", value);
  }

  rangeAngleDeg(value) {
    return this.#call("_LobBuilderRangeAngleDeg", value);
  }

  minimumSpeed(value) {
    return this.#call("_LobBuilderMinimumSpeed", uint16(value, "minimumSpeed"));
  }

  minimumEnergy(value) {
    return this.#call(
      "_LobBuilderMinimumEnergy",
      uint16(value, "minimumEnergy"),
    );
  }

  maximumTime(value) {
    return this.#call("_LobBuilderMaximumTime", value);
  }

  stepSize(value) {
    return this.#call("_LobBuilderStepSize", uint16(value, "stepSize"));
  }

  reset() {
    this.#assert();
    wasm._LobBuilderReset(this.#ptr);
    this.#freeBuffers();
    return this;
  }

  build() {
    this.#assert();
    const pointer = wasm._malloc(CONTEXT_SIZE);
    if (pointer === 0) {
      throw new Error("lob: out of wasm memory");
    }
    wasm._LobBuilderBuild(this.#ptr, pointer);
    const code = wasm._LobWasmContextError(pointer);
    if (code !== 0) {
      wasm._free(pointer);
      throw new LobError(code);
    }
    return new Context(pointer, CONTEXT_TOKEN);
  }

  dispose() {
    if (this.#ptr === 0) {
      return;
    }
    registry.unregister(this);
    wasm._LobBuilderDestroy(this.#ptr);
    wasm._free(this.#ptr);
    this.#ptr = 0;
    this.#freeBuffers();
  }
}

const CONTEXT_TOKEN = Symbol("Context");

/** @brief Solver-ready context produced by {@link Builder.build}. */
export class Context {
  #ptr;

  constructor(pointer, token) {
    if (token !== CONTEXT_TOKEN) {
      throw new TypeError("lob: Context must be created by Builder.build");
    }
    if (pointer === 0) {
      throw new Error("lob: invalid Context pointer");
    }
    this.#ptr = pointer;
    registry.register(
      this,
      { kind: "context", ptr: pointer, buffers: [] },
      this,
    );
  }

  #assert() {
    if (this.#ptr === 0) {
      throw new Error("lob: Context has been disposed");
    }
  }

  #run(feet, mode) {
    this.#assert();
    const count = feet.length;
    if (count === 0) {
      return [];
    }
    const rangesPointer = wasm._malloc(count * Uint32Array.BYTES_PER_ELEMENT);
    const outputsPointer = wasm._malloc(count * OUTPUT_SIZE);
    const doublesPointer = wasm._malloc(
      count * OUTPUT_FIELDS * Float64Array.BYTES_PER_ELEMENT,
    );
    if (rangesPointer === 0 || outputsPointer === 0 || doublesPointer === 0) {
      wasm._free(rangesPointer);
      wasm._free(outputsPointer);
      wasm._free(doublesPointer);
      throw new Error("lob: out of wasm memory");
    }
    try {
      new Uint32Array(wasm.HEAPU32.buffer, rangesPointer, count).set(feet);
      let solved;
      if (mode === "solve") {
        solved = wasm._LobSolve(
          this.#ptr,
          rangesPointer,
          outputsPointer,
          count,
        );
      } else if (mode === "solve-elr") {
        solved = wasm._LobSolveELR(
          this.#ptr,
          rangesPointer,
          outputsPointer,
          count,
        );
      } else if (mode === "inverse") {
        solved = wasm._LobSolveInverse(
          this.#ptr,
          rangesPointer,
          outputsPointer,
          count,
        );
      } else {
        solved = wasm._LobSolveInverseELR(
          this.#ptr,
          rangesPointer,
          outputsPointer,
          count,
        );
      }
      wasm._LobWasmOutputsToDoubles(outputsPointer, solved, doublesPointer);
      const isInverse = mode === "inverse" || mode === "inverse-elr";
      return readRows(solved, doublesPointer, isInverse);
    } finally {
      wasm._free(rangesPointer);
      wasm._free(outputsPointer);
      wasm._free(doublesPointer);
    }
  }

  solve(rangesFt) {
    return this.#run(rangesToFeet(rangesFt, false), "solve");
  }

  solveYds(rangesYd) {
    return this.#run(rangesToFeet(rangesYd, true), "solve");
  }

  solveELR(rangesFt) {
    return this.#run(rangesToFeet(rangesFt, false), "solve-elr");
  }

  solveELRYds(rangesYd) {
    return this.#run(rangesToFeet(rangesYd, true), "solve-elr");
  }

  solveInverse(rangesFt) {
    return this.#run(rangesToFeet(rangesFt, false), "inverse");
  }

  solveInverseYds(rangesYd) {
    return this.#run(rangesToFeet(rangesYd, true), "inverse");
  }

  solveInverseELR(rangesFt) {
    return this.#run(rangesToFeet(rangesFt, false), "inverse-elr");
  }

  solveInverseELRYds(rangesYd) {
    return this.#run(rangesToFeet(rangesYd, true), "inverse-elr");
  }

  dispose() {
    if (this.#ptr === 0) {
      return;
    }
    registry.unregister(this);
    wasm._free(this.#ptr);
    this.#ptr = 0;
  }
}

/** @brief Returns the library version string. */
export function version() {
  return wasm.UTF8ToString(wasm._LobVersion());
}

export function moaToMil(value) {
  return wasm._LobMoaToMil(value);
}

export function moaToDeg(value) {
  return wasm._LobMoaToDeg(value);
}

export function moaToIphy(value) {
  return wasm._LobMoaToIphy(value);
}

export function moaToInch(value, rangeFt) {
  return wasm._LobMoaToInch(value, rangeFt);
}

export function milToMoa(value) {
  return wasm._LobMilToMoa(value);
}

export function milToDeg(value) {
  return wasm._LobMilToDeg(value);
}

export function milToIphy(value) {
  return wasm._LobMilToIphy(value);
}

export function milToInch(value, rangeFt) {
  return wasm._LobMilToInch(value, rangeFt);
}

export function degToMoa(value) {
  return wasm._LobDegToMoa(value);
}

export function degToMil(value) {
  return wasm._LobDegToMil(value);
}

export function inchToMoa(value, rangeFt) {
  return wasm._LobInchToMoa(value, rangeFt);
}

export function inchToMil(value, rangeFt) {
  return wasm._LobInchToMil(value, rangeFt);
}

export function inchToDeg(value, rangeFt) {
  return wasm._LobInchToDeg(value, rangeFt);
}

export function jToFtLbs(value) {
  return wasm._LobJToFtLbs(value);
}

export function ftLbsToJ(value) {
  return wasm._LobFtLbsToJ(value);
}

export function mToYd(value) {
  return wasm._LobMToYd(value);
}

export function ydToFt(value) {
  return wasm._LobYdToFt(value);
}

export function mToFt(value) {
  return wasm._LobMToFt(value);
}

export function ftToIn(value) {
  return wasm._LobFtToIn(value);
}

export function mmToIn(value) {
  return wasm._LobMmToIn(value);
}

export function cmToIn(value) {
  return wasm._LobCmToIn(value);
}

export function ydToM(value) {
  return wasm._LobYdToM(value);
}

export function ftToM(value) {
  return wasm._LobFtToM(value);
}

export function ftToYd(value) {
  return wasm._LobFtToYd(value);
}

export function inToMm(value) {
  return wasm._LobInToMm(value);
}

export function inToCm(value) {
  return wasm._LobInToCm(value);
}

export function inToFt(value) {
  return wasm._LobInToFt(value);
}

export function paToInHg(value) {
  return wasm._LobPaToInHg(value);
}

export function mbarToInHg(value) {
  return wasm._LobMbarToInHg(value);
}

export function psiToInHg(value) {
  return wasm._LobPsiToInHg(value);
}

export function lbsToGrain(value) {
  return wasm._LobLbsToGrain(value);
}

export function gToGrain(value) {
  return wasm._LobGToGrain(value);
}

export function kgToGrain(value) {
  return wasm._LobKgToGrain(value);
}

export function kgSqMToPmsi(value) {
  return wasm._LobKgSqMToPmsi(value);
}

export function fpsToMps(value) {
  return wasm._LobFpsToMps(value);
}

export function mpsToFps(value) {
  return wasm._LobMpsToFps(value);
}

export function kphToMph(value) {
  return wasm._LobKphToMph(value);
}

export function knToMph(value) {
  return wasm._LobKnToMph(value);
}

export function msToS(value) {
  return wasm._LobMsToS(value);
}

export function usToS(value) {
  return wasm._LobUsToS(value);
}

export function sToMs(value) {
  return wasm._LobSToMs(value);
}

export function sToUs(value) {
  return wasm._LobSToUs(value);
}

export function degCToDegF(value) {
  return wasm._LobDegCToDegF(value);
}

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
