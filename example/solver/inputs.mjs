// Copyright (c) 2026  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

export const DRAG_FUNCTIONS = ["G1", "G2", "G5", "G6", "G7", "G8"];
export const ATMOSPHERES = ["ArmyStandardMetro", "ICAO"];

const MIN_POSITIVE = Number.MIN_VALUE;
const MAX_RANGES = 2000;

const FIELD_LABELS = {
  airPressureInHg: "Air pressure",
  altitudeOfBarometerFt: "Barometer altitude",
  altitudeOfFiringSiteFt: "Firing site altitude",
  altitudeOfThermometerFt: "Thermometer altitude",
  azimuthDeg: "Azimuth",
  bandsText: "Velocity bands",
  baseDiameterInch: "Base diameter",
  bc: "Ballistic coefficient",
  bcAtmosphere: "BC atmosphere",
  coefficientsText: "Spline coefficients",
  diameterInch: "Diameter",
  dragFunction: "Drag function",
  dragSource: "Drag source",
  dragTableText: "Mach/Cd table",
  latitudeDeg: "Latitude",
  lengthInch: "Length",
  massGrains: "Mass",
  maximumTimeSec: "Maximum time",
  maxYds: "Maximum range",
  meplatDiameterInch: "Meplat diameter",
  minimumEnergyFtLbs: "Minimum energy",
  minimumSpeedFps: "Minimum speed",
  noseLengthInch: "Nose length",
  ogiveRtR: "Ogive Rt/R",
  projectileTier: "Projectile detail",
  rangeAngleDeg: "Range angle",
  relativeHumidityPercent: "Relative humidity",
  sightHeightInches: "Sight height",
  stepSizeInch: "Step size",
  stepYds: "Range step",
  tailLengthInch: "Tail length",
  temperatureDegF: "Temperature",
  twistInchesPerTurn: "Twist",
  velocityFps: "Muzzle velocity",
  weatherSource: "Atmosphere source",
  windClock: "Heading",
  windProfileText: "Wind profile",
  windShearExponent: "Shear exponent",
  windSource: "Wind source",
  windSpeedMph: "Wind speed",
  zeroAngleMoa: "Zero angle",
  zeroImpactHeightInches: "Zero impact height",
  zeroMode: "Zero source",
  zeroYds: "Zero distance",
};

function fieldLabel(key) {
  return FIELD_LABELS[key] ?? key;
}

function formatBounds(min, max) {
  if (min === MIN_POSITIVE) {
    return max === Infinity
      ? "greater than 0"
      : `greater than 0 and at most ${max}`;
  }
  if (min === -Infinity) {
    return "a finite number";
  }
  if (max === Infinity) {
    return `at least ${min}`;
  }
  return `in ${min}..${max}`;
}

function requiredNumber(values, key, min, max, integer = false) {
  const text = values[key];
  if (typeof text !== "string" || text.trim() === "") {
    throw new RangeError(`${fieldLabel(key)} is required`);
  }
  const value = Number(text);
  if (!Number.isFinite(value) || value < min || value > max) {
    throw new RangeError(
      `${fieldLabel(key)} must be ${formatBounds(min, max)}`,
    );
  }
  if (integer && !Number.isInteger(value)) {
    throw new RangeError(`${fieldLabel(key)} must be an integer`);
  }
  return value;
}

function optionalNumber(values, key, min, max, integer = false) {
  const text = values[key];
  if (typeof text !== "string" || text.trim() === "") {
    return undefined;
  }
  return requiredNumber(values, key, min, max, integer);
}

function choice(values, key, allowed) {
  if (!allowed.includes(values[key])) {
    throw new RangeError(
      `${fieldLabel(key)} must be one of ${allowed.join(", ")}`,
    );
  }
  return values[key];
}

function checkbox(values, key) {
  return values[key] === "on" || values[key] === "true";
}

function rows(text, key, minimumColumns, maximumColumns) {
  const label = fieldLabel(key);
  if (typeof text !== "string" || text.trim() === "") {
    throw new RangeError(`${label} is required`);
  }
  const lines = text
    .split(/\r?\n/)
    .map((line) => line.trim())
    .filter((line) => line !== "");
  if (lines.length === 0) {
    throw new RangeError(`${label} is required`);
  }
  const expectedColumns =
    minimumColumns === maximumColumns
      ? `${minimumColumns}`
      : `${minimumColumns} to ${maximumColumns}`;
  return lines.map((line, index) => {
    const cells = line.split(",").map((cell) => cell.trim());
    if (cells.length < minimumColumns || cells.length > maximumColumns) {
      throw new RangeError(
        `${label} row ${index + 1} must have ${expectedColumns} columns`,
      );
    }
    return cells;
  });
}

function cell(cells, index, key, row, min, max) {
  const text = cells[index];
  if (text === "") {
    throw new RangeError(`${fieldLabel(key)} row ${row + 1} is required`);
  }
  const value = Number(text);
  if (!Number.isFinite(value) || value < min || value > max) {
    throw new RangeError(
      `${fieldLabel(key)} row ${row + 1} must be ${formatBounds(min, max)}`,
    );
  }
  return value;
}

function parseBands(text) {
  const parsed = rows(text, "bandsText", 2, 2);
  const label = fieldLabel("bandsText");
  if (parsed.length < 2 || parsed.length > 16) {
    throw new RangeError(`${label} must have 2 to 16 rows`);
  }
  const fps = [];
  const bcs = [];
  parsed.forEach((cells, index) => {
    const velocity = cell(cells, 0, "bandsText", index, MIN_POSITIVE, Infinity);
    const bc = cell(cells, 1, "bandsText", index, MIN_POSITIVE, Infinity);
    if (index > 0 && velocity <= fps[index - 1]) {
      throw new RangeError(`${label} velocities must be strictly increasing`);
    }
    fps.push(velocity);
    bcs.push(bc);
  });
  return { fps, bcs };
}

function parseDragTable(text) {
  const parsed = rows(text, "dragTableText", 2, 2);
  const label = fieldLabel("dragTableText");
  if (parsed.length < 2) {
    throw new RangeError(`${label} must have at least 2 rows`);
  }
  const machs = [];
  const drags = [];
  parsed.forEach((cells, index) => {
    const mach = cell(cells, 0, "dragTableText", index, 0, Infinity);
    const drag = cell(cells, 1, "dragTableText", index, 0, Infinity);
    if (index > 0 && mach <= machs[index - 1]) {
      throw new RangeError(`${label} Mach values must be strictly increasing`);
    }
    machs.push(mach);
    drags.push(drag);
  });
  return { machs, drags };
}

function parseCoefficients(text) {
  const label = fieldLabel("coefficientsText");
  if (typeof text !== "string" || text.trim() === "") {
    throw new RangeError(`${label} is required`);
  }
  const values = text
    .split(/[\s,]+/)
    .filter((value) => value !== "")
    .map(Number);
  if (values.length !== 60 || values.some((value) => !Number.isFinite(value))) {
    throw new RangeError(`${label} must contain exactly 60 finite numbers`);
  }
  return values;
}

function parseWindProfile(text) {
  const parsed = rows(text, "windProfileText", 3, 4);
  const label = fieldLabel("windProfileText");
  if (parsed.length > 8) {
    throw new RangeError(`${label} must have at most 8 rows`);
  }
  const profile = [];
  parsed.forEach((cells, index) => {
    const rangeFt = cell(cells, 0, "windProfileText", index, 0, Infinity);
    const headingDeg = cell(cells, 1, "windProfileText", index, 0, 360);
    const speedMph = cell(cells, 2, "windProfileText", index, 0, 1000);
    const point = { rangeFt, headingDeg, speedMph };
    if (index === 0 && rangeFt !== 0) {
      throw new RangeError(`${label} first range must be 0`);
    }
    if (index > 0 && rangeFt <= profile[index - 1].rangeFt) {
      throw new RangeError(`${label} ranges must be strictly increasing`);
    }
    if (cells[3] !== undefined && cells[3] !== "") {
      point.heightFt = Number(cells[3]);
      if (!Number.isFinite(point.heightFt) || point.heightFt <= 0) {
        throw new RangeError(`${label} row ${index + 1} height is invalid`);
      }
    }
    profile.push(point);
  });
  return profile;
}

export function readConfig(values) {
  const dragSource = choice(values, "dragSource", [
    "bc",
    "bands",
    "table",
    "coefficients",
  ]);
  const config = { dragSource };

  if (dragSource === "bc" || dragSource === "bands") {
    config.bcAtmosphere = choice(values, "bcAtmosphere", ATMOSPHERES);
    config.dragFunction = choice(values, "dragFunction", DRAG_FUNCTIONS);
  }
  if (dragSource === "bc") {
    config.bc = requiredNumber(values, "bc", MIN_POSITIVE, Infinity);
  }
  if (dragSource === "bands") {
    config.bands = parseBands(values.bandsText);
  }
  if (dragSource === "table") {
    config.dragTable = parseDragTable(values.dragTableText);
  }
  if (dragSource === "coefficients") {
    config.coefficients = parseCoefficients(values.coefficientsText);
  }

  config.projectileTier = choice(values, "projectileTier", [
    "bc",
    "mass",
    "stability",
    "full",
  ]);
  if (config.projectileTier !== "bc") {
    config.massGrains = requiredNumber(
      values,
      "massGrains",
      MIN_POSITIVE,
      Infinity,
    );
  }
  if (
    config.projectileTier === "stability" ||
    config.projectileTier === "full"
  ) {
    config.diameterInch = requiredNumber(
      values,
      "diameterInch",
      MIN_POSITIVE,
      Infinity,
    );
    config.lengthInch = requiredNumber(
      values,
      "lengthInch",
      MIN_POSITIVE,
      Infinity,
    );
    config.twistInchesPerTurn = requiredNumber(
      values,
      "twistInchesPerTurn",
      MIN_POSITIVE,
      Infinity,
    );
  }
  if (config.projectileTier === "full") {
    config.noseLengthInch = requiredNumber(
      values,
      "noseLengthInch",
      MIN_POSITIVE,
      Infinity,
    );
    config.meplatDiameterInch = requiredNumber(
      values,
      "meplatDiameterInch",
      0,
      Infinity,
    );
    config.baseDiameterInch = requiredNumber(
      values,
      "baseDiameterInch",
      MIN_POSITIVE,
      Infinity,
    );
    config.tailLengthInch = requiredNumber(
      values,
      "tailLengthInch",
      0,
      Infinity,
    );
    config.ogiveRtR = requiredNumber(values, "ogiveRtR", MIN_POSITIVE, 1);
  }

  config.velocityFps = requiredNumber(values, "velocityFps", 1, 65535, true);

  config.zeroMode = choice(values, "zeroMode", ["distance", "angle"]);
  if (config.zeroMode === "distance") {
    config.zeroYds = requiredNumber(values, "zeroYds", MIN_POSITIVE, Infinity);
    config.zeroImpactHeightInches = optionalNumber(
      values,
      "zeroImpactHeightInches",
      -Infinity,
      Infinity,
    );
  } else {
    config.zeroAngleMoa = requiredNumber(
      values,
      "zeroAngleMoa",
      -Infinity,
      Infinity,
    );
  }
  config.sightHeightInches = optionalNumber(
    values,
    "sightHeightInches",
    0,
    Infinity,
  );

  config.weatherSource = choice(values, "weatherSource", [
    "standard",
    "altitude",
    "measured",
  ]);
  if (config.weatherSource === "altitude") {
    config.altitudeOfFiringSiteFt = requiredNumber(
      values,
      "altitudeOfFiringSiteFt",
      -100000,
      100000,
    );
    config.relativeHumidityPercent = optionalNumber(
      values,
      "relativeHumidityPercent",
      0,
      100,
    );
  }
  if (config.weatherSource === "measured") {
    config.altitudeOfFiringSiteFt = optionalNumber(
      values,
      "altitudeOfFiringSiteFt",
      -100000,
      100000,
    );
    config.airPressureInHg = optionalNumber(values, "airPressureInHg", 0, 100);
    config.temperatureDegF = optionalNumber(
      values,
      "temperatureDegF",
      -100,
      200,
    );
    config.relativeHumidityPercent = optionalNumber(
      values,
      "relativeHumidityPercent",
      0,
      100,
    );
    config.measuredAway = checkbox(values, "measuredAway");
    if (config.measuredAway && config.airPressureInHg !== undefined) {
      config.altitudeOfBarometerFt = requiredNumber(
        values,
        "altitudeOfBarometerFt",
        -100000,
        100000,
      );
    }
    if (config.measuredAway && config.temperatureDegF !== undefined) {
      config.altitudeOfThermometerFt = requiredNumber(
        values,
        "altitudeOfThermometerFt",
        -100000,
        100000,
      );
    }
  }

  config.windSource = choice(values, "windSource", [
    "none",
    "uniform",
    "profile",
  ]);
  if (config.windSource === "uniform") {
    config.windSpeedMph = requiredNumber(values, "windSpeedMph", 0, 1000);
    if (config.windSpeedMph > 0) {
      config.windClock = requiredNumber(values, "windClock", 1, 12, true);
    }
  }
  if (config.windSource === "profile") {
    config.windProfile = parseWindProfile(values.windProfileText);
    const hasHeight = config.windProfile.some(
      (point) => point.heightFt !== undefined,
    );
    if (hasHeight) {
      config.windShearExponent = optionalNumber(
        values,
        "windShearExponent",
        0,
        1,
      );
    }
  }

  config.rangeAngleEnabled = checkbox(values, "rangeAngleEnabled");
  if (config.rangeAngleEnabled) {
    config.rangeAngleDeg = requiredNumber(
      values,
      "rangeAngleDeg",
      -89.999,
      89.999,
    );
  }
  config.coriolisEnabled = checkbox(values, "coriolisEnabled");
  if (config.coriolisEnabled) {
    config.latitudeDeg = requiredNumber(values, "latitudeDeg", -90, 90);
    config.azimuthDeg = requiredNumber(values, "azimuthDeg", 0, 360);
  }

  config.advancedEnabled = checkbox(values, "advancedEnabled");
  if (config.advancedEnabled) {
    config.maximumTimeSec = optionalNumber(
      values,
      "maximumTimeSec",
      0,
      Infinity,
    );
    config.minimumSpeedFps = optionalNumber(
      values,
      "minimumSpeedFps",
      0,
      65535,
      true,
    );
    config.minimumEnergyFtLbs = optionalNumber(
      values,
      "minimumEnergyFtLbs",
      0,
      65535,
      true,
    );
    if (
      config.minimumEnergyFtLbs !== undefined &&
      config.projectileTier === "bc"
    ) {
      throw new RangeError(
        `${fieldLabel("minimumEnergyFtLbs")} requires projectile mass`,
      );
    }
    config.stepSizeInch = optionalNumber(
      values,
      "stepSizeInch",
      0,
      65535,
      true,
    );
  }

  config.maxYds = requiredNumber(values, "maxYds", 1, Infinity, true);
  config.stepYds = requiredNumber(values, "stepYds", 1, Infinity, true);
  return config;
}

export function buildRanges(maxYds, stepYds) {
  if (!Number.isInteger(maxYds) || maxYds < 1) {
    throw new RangeError(`${fieldLabel("maxYds")} must be a positive integer`);
  }
  if (!Number.isInteger(stepYds) || stepYds < 1) {
    throw new RangeError(`${fieldLabel("stepYds")} must be a positive integer`);
  }
  const count =
    Math.floor(maxYds / stepYds) + 1 + (maxYds % stepYds === 0 ? 0 : 1);
  if (count > MAX_RANGES) {
    throw new RangeError(
      `Too many ranges; increase the range step or lower the maximum (limit ${MAX_RANGES}).`,
    );
  }
  const ranges = [];
  for (let range = 0; range <= maxYds; range += stepYds) {
    ranges.push(range);
  }
  if (ranges[ranges.length - 1] !== maxYds) {
    ranges.push(maxYds);
  }
  return ranges;
}

export function applyConfig(builder, config) {
  switch (config.dragSource) {
    case "bc":
      builder
        .ballisticCoefficientPsi(config.bc)
        .bcDragFunction(config.dragFunction)
        .bcAtmosphere(config.bcAtmosphere);
      break;
    case "bands":
      builder
        .bcVelocityBands(config.bands.fps, config.bands.bcs)
        .bcDragFunction(config.dragFunction)
        .bcAtmosphere(config.bcAtmosphere);
      break;
    case "table":
      builder.machVsDragTable(config.dragTable.machs, config.dragTable.drags);
      break;
    case "coefficients":
      builder.splineCoefficients(config.coefficients);
      break;
  }

  if (config.projectileTier !== "bc") {
    builder.massGrains(config.massGrains);
  }
  if (
    config.projectileTier === "stability" ||
    config.projectileTier === "full"
  ) {
    builder
      .diameterInch(config.diameterInch)
      .lengthInch(config.lengthInch)
      .twistInchesPerTurn(config.twistInchesPerTurn);
  }
  if (config.projectileTier === "full") {
    builder
      .noseLengthInch(config.noseLengthInch)
      .meplatDiameterInch(config.meplatDiameterInch)
      .baseDiameterInch(config.baseDiameterInch)
      .tailLengthInch(config.tailLengthInch)
      .ogiveRtR(config.ogiveRtR);
  }

  builder.initialVelocityFps(config.velocityFps);
  if (config.zeroMode === "distance") {
    builder.zeroDistanceYds(config.zeroYds);
    if (config.zeroImpactHeightInches !== undefined) {
      builder.zeroImpactHeightInches(config.zeroImpactHeightInches);
    }
  } else {
    builder.zeroAngleMoa(config.zeroAngleMoa);
  }
  if (config.sightHeightInches !== undefined) {
    builder.opticHeightInches(config.sightHeightInches);
  }

  if (config.weatherSource === "altitude") {
    builder.altitudeOfFiringSiteFt(config.altitudeOfFiringSiteFt);
    if (config.relativeHumidityPercent !== undefined) {
      builder.relativeHumidityPercent(config.relativeHumidityPercent);
    }
  }
  if (config.weatherSource === "measured") {
    if (config.airPressureInHg !== undefined) {
      builder.airPressureInHg(config.airPressureInHg);
    }
    if (config.temperatureDegF !== undefined) {
      builder.temperatureDegF(config.temperatureDegF);
    }
    if (config.relativeHumidityPercent !== undefined) {
      builder.relativeHumidityPercent(config.relativeHumidityPercent);
    }
    if (config.altitudeOfFiringSiteFt !== undefined) {
      builder.altitudeOfFiringSiteFt(config.altitudeOfFiringSiteFt);
    }
    if (config.measuredAway && config.airPressureInHg !== undefined) {
      builder.altitudeOfBarometerFt(config.altitudeOfBarometerFt);
    }
    if (config.measuredAway && config.temperatureDegF !== undefined) {
      builder.altitudeOfThermometerFt(config.altitudeOfThermometerFt);
    }
  }

  if (config.windSource === "uniform" && config.windSpeedMph > 0) {
    builder.windSpeedMph(config.windSpeedMph).windHeading(config.windClock);
  }
  if (config.windSource === "profile") {
    builder.windProfile(config.windProfile);
    if (config.windShearExponent !== undefined) {
      builder.windShearExponent(config.windShearExponent);
    }
  }

  if (config.rangeAngleEnabled) {
    builder.rangeAngleDeg(config.rangeAngleDeg);
  }
  if (config.coriolisEnabled) {
    builder.latitudeDeg(config.latitudeDeg).azimuthDeg(config.azimuthDeg);
  }

  if (config.advancedEnabled) {
    if (config.maximumTimeSec !== undefined) {
      builder.maximumTime(config.maximumTimeSec);
    }
    if (config.minimumSpeedFps !== undefined) {
      builder.minimumSpeed(config.minimumSpeedFps);
    }
    if (config.minimumEnergyFtLbs !== undefined) {
      builder.minimumEnergy(config.minimumEnergyFtLbs);
    }
    if (config.stepSizeInch !== undefined) {
      builder.stepSize(config.stepSizeInch);
    }
  }

  return builder;
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
