import assert from "node:assert/strict";
import test from "node:test";

import { applyConfig, buildRanges, readConfig } from "../inputs.mjs";

const valid = {
  dragSource: "bc",
  bc: "0.243",
  bcAtmosphere: "ArmyStandardMetro",
  dragFunction: "G7",
  projectileTier: "mass",
  massGrains: "168",
  velocityFps: "2700",
  zeroMode: "distance",
  zeroYds: "100",
  zeroImpactHeightInches: "0",
  sightHeightInches: "1.5",
  weatherSource: "standard",
  windSource: "uniform",
  windSpeedMph: "10",
  windClock: "3",
  rangeAngleEnabled: "",
  coriolisEnabled: "",
  advancedEnabled: "",
  maxYds: "1000",
  stepYds: "100",
};

test("buildRanges includes zero, steps, and an exact non-multiple maximum", () => {
  assert.deepEqual(buildRanges(1000, 100).slice(0, 3), [0, 100, 200]);
  assert.deepEqual(buildRanges(1050, 100).slice(-2), [1000, 1050]);
  assert.deepEqual(buildRanges(50, 100), [0, 50]);
});

test("buildRanges rejects non-positive or non-integer inputs", () => {
  assert.throws(() => buildRanges(0, 100), RangeError);
  assert.throws(() => buildRanges(1000, 0), RangeError);
  assert.throws(() => buildRanges(1000, 12.5), RangeError);
});

test("buildRanges rejects an unreasonable sample count", () => {
  assert.throws(() => buildRanges(100_000, 1), /too many ranges/i);
});

test("readConfig parses the standard BC path", () => {
  const config = readConfig(valid);
  assert.equal(config.dragSource, "bc");
  assert.equal(config.bc, 0.243);
  assert.equal(config.dragFunction, "G7");
  assert.equal(config.massGrains, 168);
  assert.equal(config.zeroYds, 100);
  assert.equal(config.windClock, 3);
});

test("readConfig reports readable field names and bounds", () => {
  assert.throws(
    () => readConfig({ ...valid, bc: "0" }),
    /Ballistic coefficient must be greater than 0/,
  );
  assert.throws(
    () => readConfig({ ...valid, velocityFps: "70000" }),
    /Muzzle velocity must be in 1\.\.65535/,
  );
  assert.throws(
    () =>
      readConfig({
        ...valid,
        dragSource: "bands",
        bandsText: "0,0.2\n2000,0.3",
      }),
    /Velocity bands row 1 must be greater than 0/,
  );
});

test("readConfig formats open and closed bounds", () => {
  assert.throws(
    () =>
      readConfig({
        ...valid,
        zeroMode: "angle",
        zeroAngleMoa: "Infinity",
      }),
    /Zero angle must be a finite number/,
  );
  assert.throws(
    () => readConfig({ ...valid, sightHeightInches: "-1" }),
    /Sight height must be at least 0/,
  );
  assert.throws(
    () =>
      readConfig({
        ...valid,
        projectileTier: "full",
        diameterInch: "0.308",
        lengthInch: "1.24",
        twistInchesPerTurn: "11",
        noseLengthInch: "0.6",
        meplatDiameterInch: "0.05",
        baseDiameterInch: "0.28",
        tailLengthInch: "0.2",
        ogiveRtR: "2",
      }),
    /Ogive Rt\/R must be greater than 0 and at most 1/,
  );
});

test("readConfig parses and validates band rows", () => {
  const config = readConfig({
    ...valid,
    dragSource: "bands",
    bandsText: "2000,0.235\n2700,0.243",
  });
  assert.deepEqual(config.bands, { fps: [2000, 2700], bcs: [0.235, 0.243] });
  assert.throws(
    () =>
      readConfig({ ...valid, dragSource: "bands", bandsText: "2700,0.243" }),
    /bands/,
  );
  assert.throws(
    () =>
      readConfig({
        ...valid,
        dragSource: "bands",
        bandsText: "2700,0.243\n2000,0.235",
      }),
    /bands/,
  );
});

test("readConfig parses custom tables and native coefficients", () => {
  const table = readConfig({
    ...valid,
    dragSource: "table",
    dragTableText: "0,0.12\n1,0.25\n2,0.38",
  });
  assert.deepEqual(table.dragTable, {
    machs: [0, 1, 2],
    drags: [0.12, 0.25, 0.38],
  });

  const coefficients = Array.from({ length: 60 }, (_, index) => String(index));
  const native = readConfig({
    ...valid,
    dragSource: "coefficients",
    coefficientsText: coefficients.join(" "),
  });
  assert.equal(native.coefficients.length, 60);
  assert.throws(
    () =>
      readConfig({
        ...valid,
        dragSource: "coefficients",
        coefficientsText: "1 2 3",
      }),
    /coefficients/,
  );
});

test("readConfig enforces projectile tiers", () => {
  assert.throws(
    () => readConfig({ ...valid, projectileTier: "mass", massGrains: "" }),
    /mass/i,
  );
  const stability = readConfig({
    ...valid,
    projectileTier: "stability",
    diameterInch: "0.308",
    lengthInch: "1.24",
    twistInchesPerTurn: "11",
  });
  assert.equal(stability.diameterInch, 0.308);
  assert.equal(stability.noseLengthInch, undefined);

  const full = readConfig({
    ...valid,
    projectileTier: "full",
    diameterInch: "0.308",
    lengthInch: "1.24",
    twistInchesPerTurn: "11",
    noseLengthInch: "0.6",
    meplatDiameterInch: "0.05",
    baseDiameterInch: "0.28",
    tailLengthInch: "0.2",
    ogiveRtR: "0.5",
  });
  assert.equal(full.noseLengthInch, 0.6);
  assert.throws(
    () =>
      readConfig({
        ...valid,
        projectileTier: "full",
        diameterInch: "0.308",
        lengthInch: "1.24",
        twistInchesPerTurn: "11",
      }),
    /nose length/i,
  );
});

test("readConfig selects one zero source", () => {
  const angle = readConfig({
    ...valid,
    zeroMode: "angle",
    zeroAngleMoa: "4.5",
  });
  assert.equal(angle.zeroAngleMoa, 4.5);
  assert.equal(angle.zeroYds, undefined);
});

test("readConfig handles weather sources and station altitudes", () => {
  const standard = readConfig(valid);
  assert.equal(standard.altitudeOfFiringSiteFt, undefined);

  const measured = readConfig({
    ...valid,
    weatherSource: "measured",
    airPressureInHg: "29.5",
    temperatureDegF: "70",
    measuredAway: "on",
    altitudeOfBarometerFt: "50",
    altitudeOfThermometerFt: "100",
  });
  assert.equal(measured.altitudeOfBarometerFt, 50);
  assert.equal(measured.altitudeOfThermometerFt, 100);

  assert.throws(
    () =>
      readConfig({
        ...valid,
        weatherSource: "measured",
        airPressureInHg: "29.5",
        measuredAway: "on",
      }),
    /barometer/i,
  );
});

test("readConfig handles wind modes and profile validation", () => {
  const profile = readConfig({
    ...valid,
    windSource: "profile",
    windProfileText: "0,90,10,1\n500,90,12,10",
    windShearExponent: "0.14",
  });
  assert.equal(profile.windProfile.length, 2);
  assert.equal(profile.windShearExponent, 0.14);
  assert.throws(
    () =>
      readConfig({
        ...valid,
        windSource: "profile",
        windProfileText: "100,90,10",
      }),
    /wind/i,
  );
});

test("readConfig handles geometry, Coriolis, and advanced limits", () => {
  const config = readConfig({
    ...valid,
    rangeAngleEnabled: "on",
    rangeAngleDeg: "-5",
    coriolisEnabled: "on",
    latitudeDeg: "40",
    azimuthDeg: "270",
    advancedEnabled: "on",
    maximumTimeSec: "3",
    minimumSpeedFps: "1000",
    minimumEnergyFtLbs: "500",
    stepSizeInch: "36",
  });
  assert.equal(config.rangeAngleDeg, -5);
  assert.equal(config.latitudeDeg, 40);
  assert.equal(config.minimumEnergyFtLbs, 500);
  assert.throws(
    () =>
      readConfig({
        ...valid,
        projectileTier: "bc",
        massGrains: "",
        advancedEnabled: "on",
        minimumEnergyFtLbs: "500",
      }),
    /energy/i,
  );
});

function recordingBuilder() {
  const calls = [];
  const builder = new Proxy(
    {},
    {
      get:
        (_target, name) =>
        (...args) => {
          calls.push([name, ...args]);
          return builder;
        },
    },
  );
  return { builder, calls };
}

test("applyConfig calls only the selected sources", () => {
  const { builder, calls } = recordingBuilder();
  applyConfig(builder, readConfig(valid));
  assert.deepEqual(
    calls.map(([name]) => name),
    [
      "ballisticCoefficientPsi",
      "bcDragFunction",
      "bcAtmosphere",
      "massGrains",
      "initialVelocityFps",
      "zeroDistanceYds",
      "zeroImpactHeightInches",
      "opticHeightInches",
      "windSpeedMph",
      "windHeading",
    ],
  );
});

test("applyConfig selects exactly one drag source", () => {
  const cases = [
    [
      "bc",
      valid,
      ["ballisticCoefficientPsi", "bcDragFunction", "bcAtmosphere"],
    ],
    [
      "bands",
      {
        ...valid,
        dragSource: "bands",
        bandsText: "2000,0.235\n2700,0.243",
      },
      ["bcVelocityBands", "bcDragFunction", "bcAtmosphere"],
    ],
    [
      "table",
      {
        ...valid,
        dragSource: "table",
        dragTableText: "0,0.12\n1,0.25",
      },
      ["machVsDragTable"],
    ],
    [
      "coefficients",
      {
        ...valid,
        dragSource: "coefficients",
        coefficientsText: Array.from({ length: 60 }, (_, i) => String(i)).join(
          " ",
        ),
      },
      ["splineCoefficients"],
    ],
  ];

  for (const [name, values, expected] of cases) {
    const { builder, calls } = recordingBuilder();
    applyConfig(builder, readConfig(values));
    const dragCalls = calls
      .filter(([call]) =>
        [
          "ballisticCoefficientPsi",
          "bcVelocityBands",
          "machVsDragTable",
          "splineCoefficients",
          "bcDragFunction",
          "bcAtmosphere",
        ].includes(call),
      )
      .map(([call]) => call);
    assert.deepEqual(dragCalls, expected, name);
  }
});

test("applyConfig selects one zero and one wind source", () => {
  const angle = recordingBuilder();
  applyConfig(
    angle.builder,
    readConfig({ ...valid, zeroMode: "angle", zeroAngleMoa: "4.5" }),
  );
  const angleNames = angle.calls.map(([name]) => name);
  assert.ok(angleNames.includes("zeroAngleMoa"));
  assert.ok(!angleNames.includes("zeroDistanceYds"));

  const profile = recordingBuilder();
  applyConfig(
    profile.builder,
    readConfig({
      ...valid,
      windSource: "profile",
      windProfileText: "0,90,10\n500,90,12",
    }),
  );
  const profileNames = profile.calls.map(([name]) => name);
  assert.ok(profileNames.includes("windProfile"));
  assert.ok(!profileNames.includes("windSpeedMph"));
  assert.ok(!profileNames.includes("windHeading"));
});
