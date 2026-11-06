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
  if (typeof value === "string") {
    if (Object.hasOwn(values, value)) {
      return values[value];
    }
    throw new RangeError(`lob: invalid ${name}: ${value}`);
  }
  if (typeof value === "number") {
    if (Object.values(values).includes(value)) {
      return value;
    }
    throw new RangeError(`lob: invalid ${name}: ${value}`);
  }
  throw new TypeError(`lob: ${name} must be a number or string`);
}

function uint16(value, name) {
  if (typeof value !== "number") {
    throw new TypeError(`lob: ${name} must be a number`);
  }
  if (
    !Number.isFinite(value) ||
    !Number.isInteger(value) ||
    value < 0 ||
    value > 0xffff
  ) {
    throw new RangeError(`lob: ${name} must be an integer in 0..65535`);
  }
  return value;
}

function clockAngle(value, name) {
  if (typeof value !== "number") {
    throw new TypeError(`lob: ${name} must be a number`);
  }
  if (
    !Number.isFinite(value) ||
    !Number.isInteger(value) ||
    value < 1 ||
    value > 12
  ) {
    throw new RangeError(`lob: ${name} must be an integer in 1..12`);
  }
  return value;
}

function finiteNumber(value, name) {
  if (typeof value !== "number") {
    throw new TypeError(`lob: ${name} must be a number`);
  }
  if (!Number.isFinite(value)) {
    throw new RangeError(`lob: ${name} must be a finite number`);
  }
  return value;
}

function numbers(values, name) {
  if (!Array.isArray(values) && !ArrayBuffer.isView(values)) {
    throw new TypeError(`lob: ${name} must be an array of numbers`);
  }
  return Array.from(values).map((value, index) =>
    finiteNumber(value, `${name}[${index}]`),
  );
}

function rangesToFeet(values, yards) {
  if (!Array.isArray(values) && !ArrayBuffer.isView(values)) {
    throw new TypeError("lob: ranges must be an array of numbers");
  }
  const feet = [];
  let previous = -1;
  for (const [index, value] of Array.from(values).entries()) {
    if (typeof value !== "number") {
      throw new TypeError(`lob: ranges[${index}] must be a number`);
    }
    if (!Number.isFinite(value) || value < 0) {
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
    const count = values.length;
    const pointer = wasm._malloc(
      Math.max(1, count) * Float32Array.BYTES_PER_ELEMENT,
    );
    if (pointer === 0) {
      throw new Error(`lob: out of wasm memory for ${name}`);
    }
    this.#buffers.push(pointer);
    if (count > 0) {
      new Float32Array(wasm.HEAPF32.buffer, pointer, count).set(values);
    }
    return pointer;
  }

  #freeBuffers() {
    for (const pointer of this.#buffers) {
      wasm._free(pointer);
    }
    this.#buffers.length = 0;
  }

  ballisticCoefficientPsi(value) {
    return this.#call(
      "_LobBuilderBallisticCoefficientPsi",
      finiteNumber(value, "ballisticCoefficientPsi"),
    );
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
    return this.#call(
      "_LobBuilderDiameterInch",
      finiteNumber(value, "diameterInch"),
    );
  }

  meplatDiameterInch(value) {
    return this.#call(
      "_LobBuilderMeplatDiameterInch",
      finiteNumber(value, "meplatDiameterInch"),
    );
  }

  baseDiameterInch(value) {
    return this.#call(
      "_LobBuilderBaseDiameterInch",
      finiteNumber(value, "baseDiameterInch"),
    );
  }

  lengthInch(value) {
    return this.#call(
      "_LobBuilderLengthInch",
      finiteNumber(value, "lengthInch"),
    );
  }

  noseLengthInch(value) {
    return this.#call(
      "_LobBuilderNoseLengthInch",
      finiteNumber(value, "noseLengthInch"),
    );
  }

  tailLengthInch(value) {
    return this.#call(
      "_LobBuilderTailLengthInch",
      finiteNumber(value, "tailLengthInch"),
    );
  }

  ogiveRtR(value) {
    return this.#call("_LobBuilderOgiveRtR", finiteNumber(value, "ogiveRtR"));
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
    return this.#call(
      "_LobBuilderMassGrains",
      finiteNumber(value, "massGrains"),
    );
  }

  initialVelocityFps(value) {
    return this.#call(
      "_LobBuilderInitialVelocityFps",
      uint16(value, "initialVelocityFps"),
    );
  }

  opticHeightInches(value) {
    return this.#call(
      "_LobBuilderOpticHeightInches",
      finiteNumber(value, "opticHeightInches"),
    );
  }

  twistInchesPerTurn(value) {
    return this.#call(
      "_LobBuilderTwistInchesPerTurn",
      finiteNumber(value, "twistInchesPerTurn"),
    );
  }

  zeroAngleMoa(value) {
    return this.#call(
      "_LobBuilderZeroAngleMOA",
      finiteNumber(value, "zeroAngleMoa"),
    );
  }

  zeroDistanceYds(value) {
    return this.#call(
      "_LobBuilderZeroDistanceYds",
      finiteNumber(value, "zeroDistanceYds"),
    );
  }

  zeroImpactHeightInches(value) {
    return this.#call(
      "_LobBuilderZeroImpactHeightInches",
      finiteNumber(value, "zeroImpactHeightInches"),
    );
  }

  altitudeOfFiringSiteFt(value) {
    return this.#call(
      "_LobBuilderAltitudeOfFiringSiteFt",
      finiteNumber(value, "altitudeOfFiringSiteFt"),
    );
  }

  airPressureInHg(value) {
    return this.#call(
      "_LobBuilderAirPressureInHg",
      finiteNumber(value, "airPressureInHg"),
    );
  }

  altitudeOfBarometerFt(value) {
    return this.#call(
      "_LobBuilderAltitudeOfBarometerFt",
      finiteNumber(value, "altitudeOfBarometerFt"),
    );
  }

  temperatureDegF(value) {
    return this.#call(
      "_LobBuilderTemperatureDegF",
      finiteNumber(value, "temperatureDegF"),
    );
  }

  altitudeOfThermometerFt(value) {
    return this.#call(
      "_LobBuilderAltitudeOfThermometerFt",
      finiteNumber(value, "altitudeOfThermometerFt"),
    );
  }

  relativeHumidityPercent(value) {
    return this.#call(
      "_LobBuilderRelativeHumidityPercent",
      finiteNumber(value, "relativeHumidityPercent"),
    );
  }

  windHeading(value) {
    return this.#call(
      "_LobBuilderWindHeading",
      clockAngle(value, "windHeading"),
    );
  }

  windHeadingDeg(value) {
    return this.#call(
      "_LobBuilderWindHeadingDeg",
      finiteNumber(value, "windHeadingDeg"),
    );
  }

  windSpeedFps(value) {
    return this.#call(
      "_LobBuilderWindSpeedFps",
      finiteNumber(value, "windSpeedFps"),
    );
  }

  windSpeedMph(value) {
    return this.#call(
      "_LobBuilderWindSpeedMph",
      finiteNumber(value, "windSpeedMph"),
    );
  }

  windProfile(points) {
    if (!Array.isArray(points)) {
      throw new TypeError("lob: points must be an array");
    }
    const rows = points.map((point, index) => {
      if (typeof point !== "object" || point === null) {
        throw new TypeError(`lob: points[${index}] must be an object`);
      }
      const rangeFt = finiteNumber(point.rangeFt, `points[${index}].rangeFt`);
      const headingDeg = finiteNumber(
        point.headingDeg,
        `points[${index}].headingDeg`,
      );
      const speedMph = finiteNumber(
        point.speedMph,
        `points[${index}].speedMph`,
      );
      let height = NaN;
      if (point.heightFt !== undefined && point.heightFt !== null) {
        height = finiteNumber(point.heightFt, `points[${index}].heightFt`);
      }
      return [rangeFt, headingDeg, speedMph, height];
    });
    this.#assert();
    const count = points.length;
    const pointer = wasm._malloc(Math.max(1, count) * WIND_POINT_SIZE);
    if (pointer === 0) {
      throw new Error("lob: out of wasm memory for wind profile");
    }
    this.#buffers.push(pointer);
    if (count > 0) {
      const view = new Float64Array(wasm.HEAPF64.buffer, pointer, count * 4);
      for (const [index, row] of rows.entries()) {
        view.set(row, index * 4);
      }
    }
    return this.#call("_LobBuilderWindProfile", pointer, count);
  }

  windShearExponent(value) {
    return this.#call(
      "_LobBuilderWindShearExponent",
      finiteNumber(value, "windShearExponent"),
    );
  }

  azimuthDeg(value) {
    return this.#call(
      "_LobBuilderAzimuthDeg",
      finiteNumber(value, "azimuthDeg"),
    );
  }

  latitudeDeg(value) {
    return this.#call(
      "_LobBuilderLatitudeDeg",
      finiteNumber(value, "latitudeDeg"),
    );
  }

  rangeAngleDeg(value) {
    return this.#call(
      "_LobBuilderRangeAngleDeg",
      finiteNumber(value, "rangeAngleDeg"),
    );
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
    return this.#call(
      "_LobBuilderMaximumTime",
      finiteNumber(value, "maximumTime"),
    );
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

  solveFt(rangesFt) {
    return this.#run(rangesToFeet(rangesFt, false), "solve");
  }

  solveYds(rangesYd) {
    return this.#run(rangesToFeet(rangesYd, true), "solve");
  }

  solveELRFt(rangesFt) {
    return this.#run(rangesToFeet(rangesFt, false), "solve-elr");
  }

  solveELRYds(rangesYd) {
    return this.#run(rangesToFeet(rangesYd, true), "solve-elr");
  }

  solveInverseFt(rangesFt) {
    return this.#run(rangesToFeet(rangesFt, false), "inverse");
  }

  solveInverseYds(rangesYd) {
    return this.#run(rangesToFeet(rangesYd, true), "inverse");
  }

  solveInverseELRFt(rangesFt) {
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
  return wasm._LobMoaToMil(finiteNumber(value, "value"));
}

export function moaToDeg(value) {
  return wasm._LobMoaToDeg(finiteNumber(value, "value"));
}

export function moaToIphy(value) {
  return wasm._LobMoaToIphy(finiteNumber(value, "value"));
}

export function moaToInch(value, rangeFt) {
  return wasm._LobMoaToInch(
    finiteNumber(value, "value"),
    finiteNumber(rangeFt, "rangeFt"),
  );
}

export function milToMoa(value) {
  return wasm._LobMilToMoa(finiteNumber(value, "value"));
}

export function milToDeg(value) {
  return wasm._LobMilToDeg(finiteNumber(value, "value"));
}

export function milToIphy(value) {
  return wasm._LobMilToIphy(finiteNumber(value, "value"));
}

export function milToInch(value, rangeFt) {
  return wasm._LobMilToInch(
    finiteNumber(value, "value"),
    finiteNumber(rangeFt, "rangeFt"),
  );
}

export function degToMoa(value) {
  return wasm._LobDegToMoa(finiteNumber(value, "value"));
}

export function degToMil(value) {
  return wasm._LobDegToMil(finiteNumber(value, "value"));
}

export function inchToMoa(value, rangeFt) {
  return wasm._LobInchToMoa(
    finiteNumber(value, "value"),
    finiteNumber(rangeFt, "rangeFt"),
  );
}

export function inchToMil(value, rangeFt) {
  return wasm._LobInchToMil(
    finiteNumber(value, "value"),
    finiteNumber(rangeFt, "rangeFt"),
  );
}

export function inchToDeg(value, rangeFt) {
  return wasm._LobInchToDeg(
    finiteNumber(value, "value"),
    finiteNumber(rangeFt, "rangeFt"),
  );
}

export function jToFtLbs(value) {
  return wasm._LobJToFtLbs(finiteNumber(value, "value"));
}

export function ftLbsToJ(value) {
  return wasm._LobFtLbsToJ(finiteNumber(value, "value"));
}

export function mToYd(value) {
  return wasm._LobMToYd(finiteNumber(value, "value"));
}

export function ydToFt(value) {
  return wasm._LobYdToFt(finiteNumber(value, "value"));
}

export function mToFt(value) {
  return wasm._LobMToFt(finiteNumber(value, "value"));
}

export function ftToIn(value) {
  return wasm._LobFtToIn(finiteNumber(value, "value"));
}

export function mmToIn(value) {
  return wasm._LobMmToIn(finiteNumber(value, "value"));
}

export function cmToIn(value) {
  return wasm._LobCmToIn(finiteNumber(value, "value"));
}

export function ydToM(value) {
  return wasm._LobYdToM(finiteNumber(value, "value"));
}

export function ftToM(value) {
  return wasm._LobFtToM(finiteNumber(value, "value"));
}

export function ftToYd(value) {
  return wasm._LobFtToYd(finiteNumber(value, "value"));
}

export function inToMm(value) {
  return wasm._LobInToMm(finiteNumber(value, "value"));
}

export function inToCm(value) {
  return wasm._LobInToCm(finiteNumber(value, "value"));
}

export function inToFt(value) {
  return wasm._LobInToFt(finiteNumber(value, "value"));
}

export function paToInHg(value) {
  return wasm._LobPaToInHg(finiteNumber(value, "value"));
}

export function mbarToInHg(value) {
  return wasm._LobMbarToInHg(finiteNumber(value, "value"));
}

export function psiToInHg(value) {
  return wasm._LobPsiToInHg(finiteNumber(value, "value"));
}

export function lbsToGrain(value) {
  return wasm._LobLbsToGrain(finiteNumber(value, "value"));
}

export function gToGrain(value) {
  return wasm._LobGToGrain(finiteNumber(value, "value"));
}

export function kgToGrain(value) {
  return wasm._LobKgToGrain(finiteNumber(value, "value"));
}

export function kgSqMToPmsi(value) {
  return wasm._LobKgSqMToPmsi(finiteNumber(value, "value"));
}

export function fpsToMps(value) {
  return wasm._LobFpsToMps(finiteNumber(value, "value"));
}

export function mpsToFps(value) {
  return wasm._LobMpsToFps(finiteNumber(value, "value"));
}

export function kphToMph(value) {
  return wasm._LobKphToMph(finiteNumber(value, "value"));
}

export function knToMph(value) {
  return wasm._LobKnToMph(finiteNumber(value, "value"));
}

export function msToS(value) {
  return wasm._LobMsToS(finiteNumber(value, "value"));
}

export function usToS(value) {
  return wasm._LobUsToS(finiteNumber(value, "value"));
}

export function sToMs(value) {
  return wasm._LobSToMs(finiteNumber(value, "value"));
}

export function sToUs(value) {
  return wasm._LobSToUs(finiteNumber(value, "value"));
}

export function degCToDegF(value) {
  return wasm._LobDegCToDegF(finiteNumber(value, "value"));
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
