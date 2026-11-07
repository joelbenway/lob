// Copyright (c) 2026  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

/**
 * @file index.mjs
 * @brief JavaScript & WebAssembly bindings for the lob exterior ballistics solver.
 *
 * Provides high-level Builder and Context classes for configuring and solving
 * external ballistics trajectories with support for standard drag functions (G1..G8),
 * piecewise BC velocity bands, custom Mach/Cd tables, wind profiles, Coriolis effects,
 * and atmospheric corrections.
 *
 * @see @ref okf_point_mass_model
 * @see @ref okf_builder_pattern
 * @see @ref okf_solver_context
 */

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

/**
 * Standard drag functions accepted by {@link Builder.bcDragFunction}.
 *
 * Values correspond to G-function reference projectiles. String representations
 * ("G1", "G2", "G5", "G6", "G7", "G8") are also accepted by the builder.
 *
 * @see @ref okf_drag_model
 */
export const DragFunction = Object.freeze({
  G1: 1,
  G2: 2,
  G5: 5,
  G6: 6,
  G7: 7,
  G8: 8,
});

/**
 * Sequence of numbers accepted by array inputs (Array or numeric TypedArray).
 *
 * @typedef {readonly number[] | Float64Array | Float32Array | Int32Array | Int16Array | Int8Array | Uint32Array | Uint16Array | Uint8Array | Uint8ClampedArray} NumberSequence
 */

/**
 * Reference atmospheric models accepted by {@link Builder.bcAtmosphere}.
 *
 * String representations ("ArmyStandardMetro", "ICAO") are also accepted.
 *
 * @see @ref okf_atmospheric_model
 */
export const Atmosphere = Object.freeze({
  ArmyStandardMetro: 0,
  ICAO: 1,
});

/**
 * Error thrown when the builder rejects configuration inputs.
 *
 * Carries a numeric `.code` matching error codes defined in the C library.
 */
export class LobError extends Error {
  /**
   * @param {number} code Numeric error code from the C solver.
   */
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

/**
 * Fluent builder for configuring external ballistics simulation parameters.
 *
 * All physical and aerodynamic setters are chainable and return `this`.
 *
 * @see @ref okf_builder_pattern
 */
export class Builder {
  #ptr;
  #buffers;

  /**
   * Initializes a new Builder allocated in WebAssembly memory.
   *
   * @throws {Error} If WebAssembly heap allocation fails.
   */
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

  /**
   * Allocate a 32-bit float buffer in WASM memory and copy values into it.
   *
   * Allocated WASM buffers remain retained until reset() or dispose().
   *
   * @param {number[]} values
   * @param {string} name
   * @returns {number} Pointer to allocated buffer in WASM memory.
   */
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

  /**
   * Set ballistic coefficient in standard sectional density units (lb/in^2).
   *
   * @param {number} value Ballistic coefficient ($lb/in^2$). Must be positive.
   * @returns {this}
   * @throws {TypeError} If `value` is not a number.
   * @throws {RangeError} If `value` is non-finite.
   * @see @ref okf_ballistic_coefficient
   */
  ballisticCoefficientPsi(value) {
    return this.#call(
      "_LobBuilderBallisticCoefficientPsi",
      finiteNumber(value, "ballisticCoefficientPsi"),
    );
  }

  /**
   * Set reference atmosphere used for ballistic coefficient definition.
   *
   * @param {number|string} value `Atmosphere.ICAO`, `Atmosphere.ArmyStandardMetro`,
   *   `"ICAO"`, or `"ArmyStandardMetro"`.
   * @returns {this}
   * @throws {TypeError} If `value` is not a number or string.
   * @throws {RangeError} If `value` is not an accepted atmosphere reference.
   * @see @ref okf_atmospheric_model
   */
  bcAtmosphere(value) {
    return this.#call(
      "_LobBuilderBCAtmosphere",
      enumValue(value, Atmosphere, "bcAtmosphere"),
    );
  }

  /**
   * Set reference standard drag function for the ballistic coefficient.
   *
   * @param {number|string} value `DragFunction.G1`..`DragFunction.G8` or string `"G1"`..`"G8"`.
   * @returns {this}
   * @throws {TypeError} If `value` is not a number or string.
   * @throws {RangeError} If `value` is not an accepted drag function.
   * @see @ref okf_drag_model
   */
  bcDragFunction(value) {
    return this.#call(
      "_LobBuilderBCDragFunction",
      enumValue(value, DragFunction, "bcDragFunction"),
    );
  }

  /**
   * Set projectile diameter (caliber), in inches.
   *
   * @param {number} value Caliber in inches (e.g., 0.308).
   * @returns {this}
   * @throws {TypeError} If `value` is not a number.
   * @throws {RangeError} If `value` is non-finite.
   */
  diameterInch(value) {
    return this.#call(
      "_LobBuilderDiameterInch",
      finiteNumber(value, "diameterInch"),
    );
  }

  /**
   * Set projectile meplat (tip flat) diameter, in inches.
   *
   * @param {number} value Meplat diameter in inches.
   * @returns {this}
   * @throws {TypeError} If `value` is not a number.
   * @throws {RangeError} If `value` is non-finite.
   */
  meplatDiameterInch(value) {
    return this.#call(
      "_LobBuilderMeplatDiameterInch",
      finiteNumber(value, "meplatDiameterInch"),
    );
  }

  /**
   * Set projectile base diameter, in inches.
   *
   * @param {number} value Base diameter in inches.
   * @returns {this}
   * @throws {TypeError} If `value` is not a number.
   * @throws {RangeError} If `value` is non-finite.
   */
  baseDiameterInch(value) {
    return this.#call(
      "_LobBuilderBaseDiameterInch",
      finiteNumber(value, "baseDiameterInch"),
    );
  }

  /**
   * Set total projectile length, in inches.
   *
   * @param {number} value Projectile length in inches.
   * @returns {this}
   * @throws {TypeError} If `value` is not a number.
   * @throws {RangeError} If `value` is non-finite.
   */
  lengthInch(value) {
    return this.#call(
      "_LobBuilderLengthInch",
      finiteNumber(value, "lengthInch"),
    );
  }

  /**
   * Set projectile nose length, in inches.
   *
   * @param {number} value Nose length in inches.
   * @returns {this}
   * @throws {TypeError} If `value` is not a number.
   * @throws {RangeError} If `value` is non-finite.
   */
  noseLengthInch(value) {
    return this.#call(
      "_LobBuilderNoseLengthInch",
      finiteNumber(value, "noseLengthInch"),
    );
  }

  /**
   * Set projectile boat-tail length, in inches.
   *
   * @param {number} value Boat-tail length in inches.
   * @returns {this}
   * @throws {TypeError} If `value` is not a number.
   * @throws {RangeError} If `value` is non-finite.
   */
  tailLengthInch(value) {
    return this.#call(
      "_LobBuilderTailLengthInch",
      finiteNumber(value, "tailLengthInch"),
    );
  }

  /**
   * Set projectile ogive radius-to-caliber ratio (Rt/R).
   *
   * @param {number} value Ogive radius ratio (dimensionless).
   * @returns {this}
   * @throws {TypeError} If `value` is not a number.
   * @throws {RangeError} If `value` is non-finite.
   */
  ogiveRtR(value) {
    return this.#call("_LobBuilderOgiveRtR", finiteNumber(value, "ogiveRtR"));
  }

  /**
   * Load custom Mach vs Drag coefficient ($C_d$) curve data.
   *
   * The mach values must be strictly ascending. Retains allocated WASM buffers
   * until reset() or dispose().
   *
   * @param {NumberSequence} machs Array or numeric TypedArray of Mach numbers.
   * @param {NumberSequence} drags Array or numeric TypedArray of corresponding drag coefficients.
   * @returns {this}
   * @throws {TypeError} If inputs are not arrays of numbers.
   * @throws {RangeError} If array lengths do not match or contain non-finite numbers.
   * @see @ref okf_drag_model
   */
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

  /**
   * Load stepped ballistic coefficients across velocity thresholds.
   *
   * @param {NumberSequence} fps Array or numeric TypedArray of velocity thresholds in feet per second.
   * @param {NumberSequence} bcs Array or numeric TypedArray of ballistic coefficient values for each band.
   * @returns {this}
   * @throws {TypeError} If inputs are not arrays of numbers.
   * @throws {RangeError} If array lengths do not match or contain non-finite numbers.
   * @see @ref okf_bc_velocity_band_transformation
   */
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

  /**
   * Load custom 60-element cubic Hermite spline coefficients.
   *
   * @param {NumberSequence} coefs Array or numeric TypedArray containing exactly 60 coefficients.
   * @returns {this}
   * @throws {TypeError} If input is not an array of numbers.
   * @throws {RangeError} If length is not 60 or contains non-finite numbers.
   * @see @ref okf_cubic_hermite_splines
   */
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

  /**
   * Set projectile mass, in grains (7000 grains = 1 pound).
   *
   * @param {number} value Mass in grains (e.g., 168).
   * @returns {this}
   * @throws {TypeError} If `value` is not a number.
   * @throws {RangeError} If `value` is non-finite.
   */
  massGrains(value) {
    return this.#call(
      "_LobBuilderMassGrains",
      finiteNumber(value, "massGrains"),
    );
  }

  /**
   * Set projectile muzzle velocity, in feet per second.
   *
   * @param {number} value Muzzle velocity in fps. Must be an integer in 0..65535.
   * @returns {this}
   * @throws {TypeError} If `value` is not a number.
   * @throws {RangeError} If `value` is not an integer in 0..65535.
   */
  initialVelocityFps(value) {
    return this.#call(
      "_LobBuilderInitialVelocityFps",
      uint16(value, "initialVelocityFps"),
    );
  }

  /**
   * Set sight optical centerline height above bore centerline, in inches.
   *
   * @param {number} value Sight height in inches (e.g., 1.5).
   * @returns {this}
   * @throws {TypeError} If `value` is not a number.
   * @throws {RangeError} If `value` is non-finite.
   */
  opticHeightInches(value) {
    return this.#call(
      "_LobBuilderOpticHeightInches",
      finiteNumber(value, "opticHeightInches"),
    );
  }

  /**
   * Set barrel rifling twist rate, in inches per turn.
   *
   * Positive values indicate right-hand (clockwise) twist. Used for spin drift
   * and aerodynamic jump calculations.
   *
   * @param {number} value Twist rate in inches per turn (e.g., 11.0).
   * @returns {this}
   * @throws {TypeError} If `value` is not a number.
   * @throws {RangeError} If `value` is non-finite.
   * @see @ref okf_spin_drift_and_aerodynamic_jump
   */
  twistInchesPerTurn(value) {
    return this.#call(
      "_LobBuilderTwistInchesPerTurn",
      finiteNumber(value, "twistInchesPerTurn"),
    );
  }

  /**
   * Set bore sight angle relative to optical axis, in Minutes of Angle (MOA).
   *
   * @param {number} value Zero angle in MOA.
   * @returns {this}
   * @throws {TypeError} If `value` is not a number.
   * @throws {RangeError} If `value` is non-finite.
   * @see @ref okf_angle_solver
   */
  zeroAngleMoa(value) {
    return this.#call(
      "_LobBuilderZeroAngleMOA",
      finiteNumber(value, "zeroAngleMoa"),
    );
  }

  /**
   * Set zero distance, in yards.
   *
   * Distance downrange at which line of sight and trajectory intersect.
   *
   * @param {number} value Zero distance in yards (e.g., 100).
   * @returns {this}
   * @throws {TypeError} If `value` is not a number.
   * @throws {RangeError} If `value` is non-finite.
   */
  zeroDistanceYds(value) {
    return this.#call(
      "_LobBuilderZeroDistanceYds",
      finiteNumber(value, "zeroDistanceYds"),
    );
  }

  /**
   * Set point of impact vertical offset at zero distance, in inches.
   *
   * @param {number} value Vertical offset in inches (positive = high).
   * @returns {this}
   * @throws {TypeError} If `value` is not a number.
   * @throws {RangeError} If `value` is non-finite.
   */
  zeroImpactHeightInches(value) {
    return this.#call(
      "_LobBuilderZeroImpactHeightInches",
      finiteNumber(value, "zeroImpactHeightInches"),
    );
  }

  /**
   * Set altitude of firing site above sea level, in feet.
   *
   * @param {number} value Firing site altitude in feet.
   * @returns {this}
   * @throws {TypeError} If `value` is not a number.
   * @throws {RangeError} If `value` is non-finite.
   */
  altitudeOfFiringSiteFt(value) {
    return this.#call(
      "_LobBuilderAltitudeOfFiringSiteFt",
      finiteNumber(value, "altitudeOfFiringSiteFt"),
    );
  }

  /**
   * Set ambient station barometric air pressure, in inches of mercury (inHg).
   *
   * @param {number} value Barometric pressure in inHg (standard sea level = 29.92).
   * @returns {this}
   * @throws {TypeError} If `value` is not a number.
   * @throws {RangeError} If `value` is non-finite.
   * @see @ref okf_atmospheric_model
   */
  airPressureInHg(value) {
    return this.#call(
      "_LobBuilderAirPressureInHg",
      finiteNumber(value, "airPressureInHg"),
    );
  }

  /**
   * Set altitude of barometer station, in feet.
   *
   * @param {number} value Barometer altitude in feet.
   * @returns {this}
   * @throws {TypeError} If `value` is not a number.
   * @throws {RangeError} If `value` is non-finite.
   */
  altitudeOfBarometerFt(value) {
    return this.#call(
      "_LobBuilderAltitudeOfBarometerFt",
      finiteNumber(value, "altitudeOfBarometerFt"),
    );
  }

  /**
   * Set ambient air temperature, in degrees Fahrenheit.
   *
   * @param {number} value Temperature in °F (standard sea level = 59°F).
   * @returns {this}
   * @throws {TypeError} If `value` is not a number.
   * @throws {RangeError} If `value` is non-finite.
   */
  temperatureDegF(value) {
    return this.#call(
      "_LobBuilderTemperatureDegF",
      finiteNumber(value, "temperatureDegF"),
    );
  }

  /**
   * Set altitude of thermometer station, in feet.
   *
   * @param {number} value Thermometer altitude in feet.
   * @returns {this}
   * @throws {TypeError} If `value` is not a number.
   * @throws {RangeError} If `value` is non-finite.
   */
  altitudeOfThermometerFt(value) {
    return this.#call(
      "_LobBuilderAltitudeOfThermometerFt",
      finiteNumber(value, "altitudeOfThermometerFt"),
    );
  }

  /**
   * Set ambient relative humidity percentage (0..100%).
   *
   * @param {number} value Relative humidity percentage.
   * @returns {this}
   * @throws {TypeError} If `value` is not a number.
   * @throws {RangeError} If `value` is non-finite.
   */
  relativeHumidityPercent(value) {
    return this.#call(
      "_LobBuilderRelativeHumidityPercent",
      finiteNumber(value, "relativeHumidityPercent"),
    );
  }

  /**
   * Set wind direction using clock angle convention (1..12).
   *
   * 12 is a direct headwind (blowing toward shooter from target), 3 is a direct
   * right-to-left crosswind, 6 is a tailwind, and 9 is a left-to-right crosswind.
   *
   * @param {number} value Clock angle integer in 1..12.
   * @returns {this}
   * @throws {TypeError} If `value` is not a number.
   * @throws {RangeError} If `value` is not an integer in 1..12.
   * @see @ref okf_wind_and_coriolis
   */
  windHeading(value) {
    return this.#call(
      "_LobBuilderWindHeading",
      clockAngle(value, "windHeading"),
    );
  }

  /**
   * Set wind direction in degrees clockwise from line of fire (0..360°).
   *
   * 0° is a direct headwind, 90° is a direct crosswind from the right,
   * 180° is a tailwind, and 270° is a crosswind from the left.
   *
   * @param {number} value Direction in degrees (0..360).
   * @returns {this}
   * @throws {TypeError} If `value` is not a number.
   * @throws {RangeError} If `value` is non-finite.
   */
  windHeadingDeg(value) {
    return this.#call(
      "_LobBuilderWindHeadingDeg",
      finiteNumber(value, "windHeadingDeg"),
    );
  }

  /**
   * Set wind velocity, in feet per second.
   *
   * @param {number} value Wind speed in fps.
   * @returns {this}
   * @throws {TypeError} If `value` is not a number.
   * @throws {RangeError} If `value` is non-finite.
   */
  windSpeedFps(value) {
    return this.#call(
      "_LobBuilderWindSpeedFps",
      finiteNumber(value, "windSpeedFps"),
    );
  }

  /**
   * Set wind velocity, in miles per hour.
   *
   * @param {number} value Wind speed in mph.
   * @returns {this}
   * @throws {TypeError} If `value` is not a number.
   * @throws {RangeError} If `value` is non-finite.
   */
  windSpeedMph(value) {
    return this.#call(
      "_LobBuilderWindSpeedMph",
      finiteNumber(value, "windSpeedMph"),
    );
  }

  /**
   * Configure a downrange piecewise wind profile.
   *
   * Wind points define localized wind vectors at discrete distances from the
   * firing site. Range points must be strictly ascending.
   *
   * @param {Array<{rangeFt: number, headingDeg: number, speedMph: number, heightFt?: number}>} points
   *   Array of wind point objects.
   * @returns {this}
   * @throws {TypeError} If `points` is not an array or elements are not valid objects.
   * @throws {RangeError} If numerical properties are non-finite.
   * @see @ref okf_wind_and_coriolis
   */
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

  /**
   * Set atmospheric wind shear power law exponent.
   *
   * Defines how wind speed scales with altitude above ground level.
   *
   * @param {number} value Power law exponent (e.g., 0.14).
   * @returns {this}
   * @throws {TypeError} If `value` is not a number.
   * @throws {RangeError} If `value` is non-finite.
   */
  windShearExponent(value) {
    return this.#call(
      "_LobBuilderWindShearExponent",
      finiteNumber(value, "windShearExponent"),
    );
  }

  /**
   * Set compass azimuth of the line of fire, in degrees clockwise from True North (0..360°).
   *
   * Used for calculating horizontal and vertical Coriolis acceleration components.
   *
   * @param {number} value Azimuth in degrees (0..360).
   * @returns {this}
   * @throws {TypeError} If `value` is not a number.
   * @throws {RangeError} If `value` is non-finite.
   * @see @ref okf_wind_and_coriolis
   */
  azimuthDeg(value) {
    return this.#call(
      "_LobBuilderAzimuthDeg",
      finiteNumber(value, "azimuthDeg"),
    );
  }

  /**
   * Set geographic latitude of the firing location, in degrees (-90° to +90°).
   *
   * Positive values denote the Northern hemisphere; negative denotes Southern hemisphere.
   * Used for Coriolis acceleration calculations.
   *
   * @param {number} value Latitude in degrees (-90..90).
   * @returns {this}
   * @throws {TypeError} If `value` is not a number.
   * @throws {RangeError} If `value` is non-finite.
   * @see @ref okf_wind_and_coriolis
   */
  latitudeDeg(value) {
    return this.#call(
      "_LobBuilderLatitudeDeg",
      finiteNumber(value, "latitudeDeg"),
    );
  }

  /**
   * Set elevation/depression angle to target relative to horizontal, in degrees (-90° to +90°).
   *
   * @param {number} value Range angle in degrees.
   * @returns {this}
   * @throws {TypeError} If `value` is not a number.
   * @throws {RangeError} If `value` is non-finite.
   */
  rangeAngleDeg(value) {
    return this.#call(
      "_LobBuilderRangeAngleDeg",
      finiteNumber(value, "rangeAngleDeg"),
    );
  }

  /**
   * Set low-velocity simulation cutoff threshold, in feet per second.
   *
   * If projectile velocity drops below this value, the simulation terminates.
   *
   * @param {number} value Cutoff velocity in fps (0..65535).
   * @returns {this}
   * @throws {TypeError} If `value` is not a number.
   * @throws {RangeError} If `value` is not an integer in 0..65535.
   */
  minimumSpeed(value) {
    return this.#call("_LobBuilderMinimumSpeed", uint16(value, "minimumSpeed"));
  }

  /**
   * Set low-energy simulation cutoff threshold, in foot-pounds.
   *
   * If projectile kinetic energy drops below this value, the simulation terminates.
   *
   * @param {number} value Cutoff energy in ft-lbs (0..65535).
   * @returns {this}
   * @throws {TypeError} If `value` is not a number.
   * @throws {RangeError} If `value` is not an integer in 0..65535.
   */
  minimumEnergy(value) {
    return this.#call(
      "_LobBuilderMinimumEnergy",
      uint16(value, "minimumEnergy"),
    );
  }

  /**
   * Set maximum flight time simulation limit, in seconds.
   *
   * @param {number} value Maximum time of flight in seconds.
   * @returns {this}
   * @throws {TypeError} If `value` is not a number.
   * @throws {RangeError} If `value` is non-finite.
   */
  maximumTime(value) {
    return this.#call(
      "_LobBuilderMaximumTime",
      finiteNumber(value, "maximumTime"),
    );
  }

  /**
   * Set numerical integration time step size, in microseconds.
   *
   * Default is typically 1000 microseconds (1 millisecond). Smaller steps increase
   * precision at the expense of calculation time.
   *
   * @param {number} value Step size in microseconds (0..65535).
   * @returns {this}
   * @throws {TypeError} If `value` is not a number.
   * @throws {RangeError} If `value` is not an integer in 0..65535.
   * @see @ref okf_integration_accuracy
   */
  stepSize(value) {
    return this.#call("_LobBuilderStepSize", uint16(value, "stepSize"));
  }

  /**
   * Resets all builder options to initial defaults and frees retained array buffers.
   *
   * @returns {this}
   * @throws {Error} If called after the builder has been disposed.
   */
  reset() {
    this.#assert();
    wasm._LobBuilderReset(this.#ptr);
    this.#freeBuffers();
    return this;
  }

  /**
   * Validates configuration and compiles an immutable ballistic {@link Context}.
   *
   * May be called repeatedly on the same builder to produce independent Contexts.
   *
   * @returns {Context} A configured, executable ballistic solver context.
   * @throws {Error} If called after the builder has been disposed.
   * @throws {LobError} If the C ballistics engine rejects the configuration.
   * @see @ref okf_solver_context
   */
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

  /**
   * Releases allocated WebAssembly memory and retained array buffers.
   *
   * Idempotent; subsequent calls have no effect. Calling operational builder
   * methods after disposal throws an Error.
   */
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

/**
 * Compiled ballistic solver context produced by {@link Builder.build}.
 *
 * Provides methods for forward solves (output drop and deflection in inches)
 * and inverse solves (output elevation and windage in MOA).
 *
 * @see @ref okf_solver_context
 */
export class Context {
  #ptr;

  /**
   * @private
   */
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

  /**
   * Forward trajectory solution taking downrange distances in feet.
   *
   * Returns vertical drop and horizontal deflection in inches relative to line of sight.
   * Trajectories that fall short of distant ranges return fewer rows without throwing.
   *
   * @param {NumberSequence} rangesFt Strictly ascending array or numeric TypedArray of ranges in feet.
   * @returns {Array<{rangeFt: number, rangeYds: number, velocityFps: number, energyFtLbs: number, elevationInch: number, deflectionInch: number, timeOfFlightSec: number}>}
   * @throws {TypeError} If `rangesFt` is not an array of numbers.
   * @throws {RangeError} If ranges are negative, non-finite, or not strictly ascending.
   * @see @ref okf_point_mass_model
   */
  solveFt(rangesFt) {
    return this.#run(rangesToFeet(rangesFt, false), "solve");
  }

  /**
   * Forward trajectory solution taking downrange distances in yards.
   *
   * Yard values are converted to whole feet via `Math.round(yards * 3)`.
   * Returns vertical drop and horizontal deflection in inches relative to line of sight.
   *
   * @param {NumberSequence} rangesYd Strictly ascending array or numeric TypedArray of ranges in yards.
   * @returns {Array<{rangeFt: number, rangeYds: number, velocityFps: number, energyFtLbs: number, elevationInch: number, deflectionInch: number, timeOfFlightSec: number}>}
   * @throws {TypeError} If `rangesYd` is not an array of numbers.
   * @throws {RangeError} If ranges are negative, non-finite, or not strictly ascending.
   */
  solveYds(rangesYd) {
    return this.#run(rangesToFeet(rangesYd, true), "solve");
  }

  /**
   * Extended Long Range (ELR) forward trajectory solution in feet.
   *
   * Recalculates local air density, pressure, and speed of sound at each integration step
   * as projectile altitude changes along the flight path.
   *
   * @param {NumberSequence} rangesFt Strictly ascending array or numeric TypedArray of ranges in feet.
   * @returns {Array<{rangeFt: number, rangeYds: number, velocityFps: number, energyFtLbs: number, elevationInch: number, deflectionInch: number, timeOfFlightSec: number}>}
   * @throws {TypeError} If `rangesFt` is not an array of numbers.
   * @throws {RangeError} If ranges are negative, non-finite, or not strictly ascending.
   */
  solveELRFt(rangesFt) {
    return this.#run(rangesToFeet(rangesFt, false), "solve-elr");
  }

  /**
   * Extended Long Range (ELR) forward trajectory solution in yards.
   *
   * @param {NumberSequence} rangesYd Strictly ascending array or numeric TypedArray of ranges in yards.
   * @returns {Array<{rangeFt: number, rangeYds: number, velocityFps: number, energyFtLbs: number, elevationInch: number, deflectionInch: number, timeOfFlightSec: number}>}
   * @throws {TypeError} If `rangesYd` is not an array of numbers.
   * @throws {RangeError} If ranges are negative, non-finite, or not strictly ascending.
   */
  solveELRYds(rangesYd) {
    return this.#run(rangesToFeet(rangesYd, true), "solve-elr");
  }

  /**
   * Geometric inverse trajectory solution taking ranges in feet.
   *
   * Computes elevation (drop compensation) and windage deflection as angular sight
   * adjustments in Minutes of Angle (MOA). Positive elevation MOA indicates "dial up";
   * positive deflection MOA indicates "dial right".
   *
   * Uses a closed-form geometric correction applied to the forward solution.
   * Range 0 returns 0 MOA elevation and 0 MOA deflection.
   *
   * @param {NumberSequence} rangesFt Strictly ascending array or numeric TypedArray of ranges in feet.
   * @returns {Array<{rangeFt: number, rangeYds: number, velocityFps: number, energyFtLbs: number, elevationMoa: number, deflectionMoa: number, timeOfFlightSec: number}>}
   * @throws {TypeError} If `rangesFt` is not an array of numbers.
   * @throws {RangeError} If ranges are negative, non-finite, or not strictly ascending.
   * @see @ref okf_inverse_solving
   */
  solveInverseFt(rangesFt) {
    return this.#run(rangesToFeet(rangesFt, false), "inverse");
  }

  /**
   * Geometric inverse trajectory solution taking ranges in yards.
   *
   * Yard values are converted to whole feet via `Math.round(yards * 3)`.
   * Returns sight adjustments in Minutes of Angle (MOA).
   *
   * @param {NumberSequence} rangesYd Strictly ascending array or numeric TypedArray of ranges in yards.
   * @returns {Array<{rangeFt: number, rangeYds: number, velocityFps: number, energyFtLbs: number, elevationMoa: number, deflectionMoa: number, timeOfFlightSec: number}>}
   * @throws {TypeError} If `rangesYd` is not an array of numbers.
   * @throws {RangeError} If ranges are negative, non-finite, or not strictly ascending.
   * @see @ref okf_inverse_solving
   */
  solveInverseYds(rangesYd) {
    return this.#run(rangesToFeet(rangesYd, true), "inverse");
  }

  /**
   * Extended Long Range (ELR) inverse trajectory solution taking ranges in feet.
   *
   * Returns MOA sight adjustments with altitude-dependent atmospheric recalculation.
   *
   * @param {NumberSequence} rangesFt Strictly ascending array or numeric TypedArray of ranges in feet.
   * @returns {Array<{rangeFt: number, rangeYds: number, velocityFps: number, energyFtLbs: number, elevationMoa: number, deflectionMoa: number, timeOfFlightSec: number}>}
   * @throws {TypeError} If `rangesFt` is not an array of numbers.
   * @throws {RangeError} If ranges are negative, non-finite, or not strictly ascending.
   */
  solveInverseELRFt(rangesFt) {
    return this.#run(rangesToFeet(rangesFt, false), "inverse-elr");
  }

  /**
   * Extended Long Range (ELR) inverse trajectory solution taking ranges in yards.
   *
   * @param {NumberSequence} rangesYd Strictly ascending array or numeric TypedArray of ranges in yards.
   * @returns {Array<{rangeFt: number, rangeYds: number, velocityFps: number, energyFtLbs: number, elevationMoa: number, deflectionMoa: number, timeOfFlightSec: number}>}
   * @throws {TypeError} If `rangesYd` is not an array of numbers.
   * @throws {RangeError} If ranges are negative, non-finite, or not strictly ascending.
   */
  solveInverseELRYds(rangesYd) {
    return this.#run(rangesToFeet(rangesYd, true), "inverse-elr");
  }

  /**
   * Releases allocated WebAssembly context memory.
   *
   * Idempotent; subsequent calls have no effect. Calling solver methods
   * after disposal throws an Error.
   */
  dispose() {
    if (this.#ptr === 0) {
      return;
    }
    registry.unregister(this);
    wasm._free(this.#ptr);
    this.#ptr = 0;
  }
}

/**
 * Returns the library semantic version string.
 *
 * @returns {string} Version string (e.g., "1.0.0").
 */
export function version() {
  return wasm.UTF8ToString(wasm._LobVersion());
}

/**
 * Convert Minutes of Angle (MOA) to milliradians (mil).
 * @param {number} value Angle in MOA.
 * @returns {number} Angle in milliradians.
 */
export function moaToMil(value) {
  return wasm._LobMoaToMil(finiteNumber(value, "value"));
}

/**
 * Convert Minutes of Angle (MOA) to decimal degrees.
 * @param {number} value Angle in MOA.
 * @returns {number} Angle in degrees.
 */
export function moaToDeg(value) {
  return wasm._LobMoaToDeg(finiteNumber(value, "value"));
}

/**
 * Convert Minutes of Angle (MOA) to Inches Per Hundred Yards (IPHY).
 * @param {number} value Angle in MOA.
 * @returns {number} Value in IPHY.
 */
export function moaToIphy(value) {
  return wasm._LobMoaToIphy(finiteNumber(value, "value"));
}

/**
 * Convert Minutes of Angle (MOA) to subtended arc length in inches at range.
 * @param {number} value Angle in MOA.
 * @param {number} rangeFt Range distance in feet.
 * @returns {number} Subtended height/width in inches.
 */
export function moaToInch(value, rangeFt) {
  return wasm._LobMoaToInch(
    finiteNumber(value, "value"),
    finiteNumber(rangeFt, "rangeFt"),
  );
}

/**
 * Convert milliradians (mil) to Minutes of Angle (MOA).
 * @param {number} value Angle in milliradians.
 * @returns {number} Angle in MOA.
 */
export function milToMoa(value) {
  return wasm._LobMilToMoa(finiteNumber(value, "value"));
}

/**
 * Convert milliradians (mil) to decimal degrees.
 * @param {number} value Angle in milliradians.
 * @returns {number} Angle in degrees.
 */
export function milToDeg(value) {
  return wasm._LobMilToDeg(finiteNumber(value, "value"));
}

/**
 * Convert milliradians (mil) to Inches Per Hundred Yards (IPHY).
 * @param {number} value Angle in milliradians.
 * @returns {number} Value in IPHY.
 */
export function milToIphy(value) {
  return wasm._LobMilToIphy(finiteNumber(value, "value"));
}

/**
 * Convert milliradians (mil) to subtended arc length in inches at range.
 * @param {number} value Angle in milliradians.
 * @param {number} rangeFt Range distance in feet.
 * @returns {number} Subtended height/width in inches.
 */
export function milToInch(value, rangeFt) {
  return wasm._LobMilToInch(
    finiteNumber(value, "value"),
    finiteNumber(rangeFt, "rangeFt"),
  );
}

/**
 * Convert decimal degrees to Minutes of Angle (MOA).
 * @param {number} value Angle in degrees.
 * @returns {number} Angle in MOA.
 */
export function degToMoa(value) {
  return wasm._LobDegToMoa(finiteNumber(value, "value"));
}

/**
 * Convert decimal degrees to milliradians (mil).
 * @param {number} value Angle in degrees.
 * @returns {number} Angle in milliradians.
 */
export function degToMil(value) {
  return wasm._LobDegToMil(finiteNumber(value, "value"));
}

/**
 * Convert subtended length in inches at range to Minutes of Angle (MOA).
 * @param {number} value Subtended length in inches.
 * @param {number} rangeFt Range distance in feet.
 * @returns {number} Angle in MOA.
 */
export function inchToMoa(value, rangeFt) {
  return wasm._LobInchToMoa(
    finiteNumber(value, "value"),
    finiteNumber(rangeFt, "rangeFt"),
  );
}

/**
 * Convert subtended length in inches at range to milliradians (mil).
 * @param {number} value Subtended length in inches.
 * @param {number} rangeFt Range distance in feet.
 * @returns {number} Angle in milliradians.
 */
export function inchToMil(value, rangeFt) {
  return wasm._LobInchToMil(
    finiteNumber(value, "value"),
    finiteNumber(rangeFt, "rangeFt"),
  );
}

/**
 * Convert subtended length in inches at range to decimal degrees.
 * @param {number} value Subtended length in inches.
 * @param {number} rangeFt Range distance in feet.
 * @returns {number} Angle in degrees.
 */
export function inchToDeg(value, rangeFt) {
  return wasm._LobInchToDeg(
    finiteNumber(value, "value"),
    finiteNumber(rangeFt, "rangeFt"),
  );
}

/**
 * Convert energy in Joules to foot-pounds (ft-lbs).
 * @param {number} value Energy in Joules.
 * @returns {number} Energy in ft-lbs.
 */
export function jToFtLbs(value) {
  return wasm._LobJToFtLbs(finiteNumber(value, "value"));
}

/**
 * Convert energy in foot-pounds (ft-lbs) to Joules.
 * @param {number} value Energy in ft-lbs.
 * @returns {number} Energy in Joules.
 */
export function ftLbsToJ(value) {
  return wasm._LobFtLbsToJ(finiteNumber(value, "value"));
}

/**
 * Convert length in meters to yards.
 * @param {number} value Length in meters.
 * @returns {number} Length in yards.
 */
export function mToYd(value) {
  return wasm._LobMToYd(finiteNumber(value, "value"));
}

/**
 * Convert length in yards to feet.
 * @param {number} value Length in yards.
 * @returns {number} Length in feet.
 */
export function ydToFt(value) {
  return wasm._LobYdToFt(finiteNumber(value, "value"));
}

/**
 * Convert length in meters to feet.
 * @param {number} value Length in meters.
 * @returns {number} Length in feet.
 */
export function mToFt(value) {
  return wasm._LobMToFt(finiteNumber(value, "value"));
}

/**
 * Convert length in feet to inches.
 * @param {number} value Length in feet.
 * @returns {number} Length in inches.
 */
export function ftToIn(value) {
  return wasm._LobFtToIn(finiteNumber(value, "value"));
}

/**
 * Convert length in millimeters to inches.
 * @param {number} value Length in millimeters.
 * @returns {number} Length in inches.
 */
export function mmToIn(value) {
  return wasm._LobMmToIn(finiteNumber(value, "value"));
}

/**
 * Convert length in centimeters to inches.
 * @param {number} value Length in centimeters.
 * @returns {number} Length in inches.
 */
export function cmToIn(value) {
  return wasm._LobCmToIn(finiteNumber(value, "value"));
}

/**
 * Convert length in yards to meters.
 * @param {number} value Length in yards.
 * @returns {number} Length in meters.
 */
export function ydToM(value) {
  return wasm._LobYdToM(finiteNumber(value, "value"));
}

/**
 * Convert length in feet to meters.
 * @param {number} value Length in feet.
 * @returns {number} Length in meters.
 */
export function ftToM(value) {
  return wasm._LobFtToM(finiteNumber(value, "value"));
}

/**
 * Convert length in feet to yards.
 * @param {number} value Length in feet.
 * @returns {number} Length in yards.
 */
export function ftToYd(value) {
  return wasm._LobFtToYd(finiteNumber(value, "value"));
}

/**
 * Convert length in inches to millimeters.
 * @param {number} value Length in inches.
 * @returns {number} Length in millimeters.
 */
export function inToMm(value) {
  return wasm._LobInToMm(finiteNumber(value, "value"));
}

/**
 * Convert length in inches to centimeters.
 * @param {number} value Length in inches.
 * @returns {number} Length in centimeters.
 */
export function inToCm(value) {
  return wasm._LobInToCm(finiteNumber(value, "value"));
}

/**
 * Convert length in inches to feet.
 * @param {number} value Length in inches.
 * @returns {number} Length in feet.
 */
export function inToFt(value) {
  return wasm._LobInToFt(finiteNumber(value, "value"));
}

/**
 * Convert pressure in Pascals to inches of mercury (inHg).
 * @param {number} value Pressure in Pascals.
 * @returns {number} Pressure in inHg.
 */
export function paToInHg(value) {
  return wasm._LobPaToInHg(finiteNumber(value, "value"));
}

/**
 * Convert pressure in millibars (hPa) to inches of mercury (inHg).
 * @param {number} value Pressure in millibars.
 * @returns {number} Pressure in inHg.
 */
export function mbarToInHg(value) {
  return wasm._LobMbarToInHg(finiteNumber(value, "value"));
}

/**
 * Convert pressure in pounds per square inch (psi) to inches of mercury (inHg).
 * @param {number} value Pressure in psi.
 * @returns {number} Pressure in inHg.
 */
export function psiToInHg(value) {
  return wasm._LobPsiToInHg(finiteNumber(value, "value"));
}

/**
 * Convert mass in pounds to grains.
 * @param {number} value Mass in pounds.
 * @returns {number} Mass in grains.
 */
export function lbsToGrain(value) {
  return wasm._LobLbsToGrain(finiteNumber(value, "value"));
}

/**
 * Convert mass in grams to grains.
 * @param {number} value Mass in grams.
 * @returns {number} Mass in grains.
 */
export function gToGrain(value) {
  return wasm._LobGToGrain(finiteNumber(value, "value"));
}

/**
 * Convert mass in kilograms to grains.
 * @param {number} value Mass in kilograms.
 * @returns {number} Mass in grains.
 */
export function kgToGrain(value) {
  return wasm._LobKgToGrain(finiteNumber(value, "value"));
}

/**
 * Convert sectional density in $kg/m^2$ to pounds per square inch ($lb/in^2$).
 * @param {number} value Sectional density in $kg/m^2$.
 * @returns {number} Sectional density in $lb/in^2$.
 */
export function kgSqMToPmsi(value) {
  return wasm._LobKgSqMToPmsi(finiteNumber(value, "value"));
}

/**
 * Convert velocity in feet per second (fps) to meters per second (m/s).
 * @param {number} value Velocity in fps.
 * @returns {number} Velocity in m/s.
 */
export function fpsToMps(value) {
  return wasm._LobFpsToMps(finiteNumber(value, "value"));
}

/**
 * Convert velocity in meters per second (m/s) to feet per second (fps).
 * @param {number} value Velocity in m/s.
 * @returns {number} Velocity in fps.
 */
export function mpsToFps(value) {
  return wasm._LobMpsToFps(finiteNumber(value, "value"));
}

/**
 * Convert velocity in kilometers per hour (km/h) to miles per hour (mph).
 * @param {number} value Velocity in km/h.
 * @returns {number} Velocity in mph.
 */
export function kphToMph(value) {
  return wasm._LobKphToMph(finiteNumber(value, "value"));
}

/**
 * Convert velocity in knots (kn) to miles per hour (mph).
 * @param {number} value Velocity in knots.
 * @returns {number} Velocity in mph.
 */
export function knToMph(value) {
  return wasm._LobKnToMph(finiteNumber(value, "value"));
}

/**
 * Convert time in milliseconds (ms) to seconds (s).
 * @param {number} value Time in milliseconds.
 * @returns {number} Time in seconds.
 */
export function msToS(value) {
  return wasm._LobMsToS(finiteNumber(value, "value"));
}

/**
 * Convert time in microseconds (us) to seconds (s).
 * @param {number} value Time in microseconds.
 * @returns {number} Time in seconds.
 */
export function usToS(value) {
  return wasm._LobUsToS(finiteNumber(value, "value"));
}

/**
 * Convert time in seconds (s) to milliseconds (ms).
 * @param {number} value Time in seconds.
 * @returns {number} Time in milliseconds.
 */
export function sToMs(value) {
  return wasm._LobSToMs(finiteNumber(value, "value"));
}

/**
 * Convert time in seconds (s) to microseconds (us).
 * @param {number} value Time in seconds.
 * @returns {number} Time in microseconds.
 */
export function sToUs(value) {
  return wasm._LobSToUs(finiteNumber(value, "value"));
}

/**
 * Convert temperature in degrees Celsius (°C) to degrees Fahrenheit (°F).
 * @param {number} value Temperature in °C.
 * @returns {number} Temperature in °F.
 */
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
