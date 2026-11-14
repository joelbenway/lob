// Copyright (c) 2026  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

import assert from "node:assert/strict";
import { test } from "node:test";

const api = await import(
  new URL("../../build/wasm/index.mjs", import.meta.url).href
);
const { Atmosphere, Builder, DragFunction, LobError, moaToInch, ydToFt } = api;

test("scalar float setters reject wrong primitive types with TypeError", () => {
  const setters = [
    "ballisticCoefficientPsi",
    "diameterInch",
    "meplatDiameterInch",
    "baseDiameterInch",
    "lengthInch",
    "noseLengthInch",
    "tailLengthInch",
    "ogiveRtR",
    "massGrains",
    "opticHeightInches",
    "opticLateralOffsetInches",
    "twistInchesPerTurn",
    "zeroAngleMoa",
    "zeroAzimuthMoa",
    "zeroDistanceYds",
    "zeroImpactHeightInches",
    "altitudeOfFiringSiteFt",
    "airPressureInHg",
    "altitudeOfBarometerFt",
    "temperatureDegF",
    "altitudeOfThermometerFt",
    "relativeHumidityPercent",
    "windHeadingDeg",
    "windSpeedFps",
    "windSpeedMph",
    "windShearExponent",
    "azimuthDeg",
    "latitudeDeg",
    "rangeAngleDeg",
    "maximumTime",
  ];

  const invalidTypes = ["100", true, false, null, undefined, {}, Symbol("x")];

  for (const setter of setters) {
    const builder = new Builder();
    for (const bad of invalidTypes) {
      assert.throws(
        () => builder[setter](bad),
        TypeError,
        `${setter}(${String(bad)}) should throw TypeError`,
      );
    }
    builder.dispose();
  }
});

test("scalar float setters reject non-finite numbers with RangeError", () => {
  const setters = [
    "ballisticCoefficientPsi",
    "diameterInch",
    "meplatDiameterInch",
    "baseDiameterInch",
    "lengthInch",
    "noseLengthInch",
    "tailLengthInch",
    "ogiveRtR",
    "massGrains",
    "opticHeightInches",
    "opticLateralOffsetInches",
    "twistInchesPerTurn",
    "zeroAngleMoa",
    "zeroAzimuthMoa",
    "zeroDistanceYds",
    "zeroImpactHeightInches",
    "altitudeOfFiringSiteFt",
    "airPressureInHg",
    "altitudeOfBarometerFt",
    "temperatureDegF",
    "altitudeOfThermometerFt",
    "relativeHumidityPercent",
    "windHeadingDeg",
    "windSpeedFps",
    "windSpeedMph",
    "windShearExponent",
    "azimuthDeg",
    "latitudeDeg",
    "rangeAngleDeg",
    "maximumTime",
  ];

  const nonFiniteValues = [NaN, Infinity, -Infinity];

  for (const setter of setters) {
    const builder = new Builder();
    for (const bad of nonFiniteValues) {
      assert.throws(
        () => builder[setter](bad),
        RangeError,
        `${setter}(${bad}) should throw RangeError`,
      );
    }
    builder.dispose();
  }
});

test("uint16 setters enforce types, finiteness, and integer bounds", () => {
  const uint16Setters = [
    "initialVelocityFps",
    "minimumSpeed",
    "minimumEnergy",
    "stepSize",
  ];

  for (const setter of uint16Setters) {
    const builder = new Builder();
    // TypeError for non-numbers
    assert.throws(() => builder[setter]("100"), TypeError);
    assert.throws(() => builder[setter](null), TypeError);
    assert.throws(() => builder[setter](true), TypeError);

    // RangeError for non-finite, float, negative, or > 65535
    assert.throws(() => builder[setter](NaN), RangeError);
    assert.throws(() => builder[setter](Infinity), RangeError);
    assert.throws(() => builder[setter](100.5), RangeError);
    assert.throws(() => builder[setter](-1), RangeError);
    assert.throws(() => builder[setter](65536), RangeError);

    // Valid boundaries
    assert.equal(builder[setter](0), builder);
    assert.equal(builder[setter](65535), builder);
    builder.dispose();
  }
});

test("windHeading enforces clock angle boundaries", () => {
  const builder = new Builder();
  assert.throws(() => builder.windHeading("3"), TypeError);
  assert.throws(() => builder.windHeading(null), TypeError);
  assert.throws(() => builder.windHeading(NaN), RangeError);
  assert.throws(() => builder.windHeading(3.5), RangeError);
  assert.throws(() => builder.windHeading(0), RangeError);
  assert.throws(() => builder.windHeading(13), RangeError);
  assert.throws(() => builder.windHeading(-1), RangeError);

  assert.equal(builder.windHeading(1), builder);
  assert.equal(builder.windHeading(12), builder);
  builder.dispose();
});

test("enum setters enforce accepted keys, values, and types", () => {
  const builder = new Builder();

  // TypeError for non-string / non-number
  assert.throws(() => builder.bcAtmosphere(true), TypeError);
  assert.throws(() => builder.bcAtmosphere(null), TypeError);
  assert.throws(() => builder.bcDragFunction({}), TypeError);

  // RangeError for invalid string or number
  assert.throws(() => builder.bcAtmosphere("Standard"), RangeError);
  assert.throws(() => builder.bcAtmosphere(99), RangeError);
  assert.throws(() => builder.bcDragFunction("G3"), RangeError);
  assert.throws(() => builder.bcDragFunction(3), RangeError);

  // Valid values
  assert.equal(builder.bcAtmosphere("ICAO"), builder);
  assert.equal(builder.bcAtmosphere(Atmosphere.ICAO), builder);
  assert.equal(builder.bcDragFunction("G7"), builder);
  assert.equal(builder.bcDragFunction(DragFunction.G7), builder);
  builder.dispose();
});

test("array table setters validate shapes, lengths, and numbers", () => {
  const builder = new Builder();

  // machVsDragTable
  assert.throws(() => builder.machVsDragTable("machs", [1]), TypeError);
  assert.throws(() => builder.machVsDragTable([1], "drags"), TypeError);
  assert.throws(() => builder.machVsDragTable(["1"], [1]), TypeError);
  assert.throws(() => builder.machVsDragTable([NaN], [1]), RangeError);
  assert.throws(() => builder.machVsDragTable([1, 2], [1]), RangeError); // mismatched length

  // Typed arrays are accepted
  assert.equal(
    builder.machVsDragTable(
      new Float32Array([1.5, 2.0]),
      new Float32Array([0.2, 0.3]),
    ),
    builder,
  );

  // bcVelocityBands
  assert.throws(() => builder.bcVelocityBands("fps", [0.2]), TypeError);
  assert.throws(() => builder.bcVelocityBands([2000], [NaN]), RangeError);
  assert.throws(() => builder.bcVelocityBands([2000, 3000], [0.2]), RangeError);

  // splineCoefficients requires exactly 60 entries
  assert.throws(() => builder.splineCoefficients("coefs"), TypeError);
  assert.throws(() => builder.splineCoefficients([1, 2, 3]), RangeError);
  assert.throws(
    () => builder.splineCoefficients(new Array(60).fill("0")),
    TypeError,
  );
  assert.throws(
    () => builder.splineCoefficients(new Array(60).fill(NaN)),
    RangeError,
  );
  assert.equal(
    builder.splineCoefficients(new Float32Array(60).fill(0.1)),
    builder,
  );

  builder.dispose();
});

test("empty array tables pass to C builder and fail domain validation at build() without leaking", () => {
  const builder = new Builder()
    .ballisticCoefficientPsi(0.243)
    .initialVelocityFps(2700)
    .zeroDistanceYds(100)
    .machVsDragTable([], []);

  assert.throws(
    () => builder.build(),
    (err) => err instanceof LobError,
  );
  builder.dispose();
});

test("windProfile validates array shape, point objects, and optional height", () => {
  const builder = new Builder();

  assert.throws(() => builder.windProfile("not-an-array"), TypeError);
  assert.throws(() => builder.windProfile([null]), TypeError);
  assert.throws(() => builder.windProfile(["not-an-object"]), TypeError);
  assert.throws(
    () => builder.windProfile([{ rangeFt: "0", headingDeg: 90, speedMph: 10 }]),
    TypeError,
  );
  assert.throws(
    () => builder.windProfile([{ rangeFt: NaN, headingDeg: 90, speedMph: 10 }]),
    RangeError,
  );
  assert.throws(
    () =>
      builder.windProfile([
        { rangeFt: 0, headingDeg: 90, speedMph: 10, heightFt: "10" },
      ]),
    TypeError,
  );
  assert.throws(
    () =>
      builder.windProfile([
        { rangeFt: 0, headingDeg: 90, speedMph: 10, heightFt: NaN },
      ]),
    RangeError,
  );

  // Valid point with heightFt omitted
  assert.equal(
    builder.windProfile([{ rangeFt: 0, headingDeg: 90, speedMph: 10 }]),
    builder,
  );
  // Valid point with explicit heightFt
  assert.equal(
    builder.windProfile([
      { rangeFt: 0, headingDeg: 90, speedMph: 10, heightFt: 5 },
    ]),
    builder,
  );
  // Empty wind profile is accepted
  assert.equal(builder.windProfile([]), builder);

  builder.dispose();
});

test("solver methods validate range types, finiteness, non-negativity, and ascending order", () => {
  const context = new Builder()
    .ballisticCoefficientPsi(0.243)
    .initialVelocityFps(2700)
    .zeroDistanceYds(100)
    .build();

  const methods = [
    "solveFt",
    "solveYds",
    "solveELRFt",
    "solveELRYds",
    "solveInverseFt",
    "solveInverseYds",
    "solveInverseELRFt",
    "solveInverseELRYds",
  ];

  for (const method of methods) {
    // Non-array
    assert.throws(() => context[method]("100"), TypeError, `${method} type`);
    assert.throws(() => context[method](null), TypeError, `${method} type`);

    // Non-number inside array
    assert.throws(
      () => context[method](["100"]),
      TypeError,
      `${method} element type`,
    );

    // Negative range
    assert.throws(
      () => context[method]([-10]),
      RangeError,
      `${method} negative`,
    );

    // Non-finite
    assert.throws(() => context[method]([NaN]), RangeError, `${method} NaN`);
    assert.throws(
      () => context[method]([Infinity]),
      RangeError,
      `${method} Infinity`,
    );

    // Non-ascending
    assert.throws(
      () => context[method]([100, 100]),
      RangeError,
      `${method} duplicate`,
    );
    assert.throws(
      () => context[method]([200, 100]),
      RangeError,
      `${method} descending`,
    );

    // Empty array returns empty array
    assert.deepEqual(context[method]([]), []);

    // Typed arrays accepted
    assert.equal(context[method](new Float64Array([100, 200])).length, 2);
  }

  context.dispose();
});

test("unit conversions validate input types and finiteness", () => {
  // Single-argument conversion
  assert.throws(() => ydToFt("100"), TypeError);
  assert.throws(() => ydToFt(null), TypeError);
  assert.throws(() => ydToFt(NaN), RangeError);
  assert.throws(() => ydToFt(Infinity), RangeError);
  assert.equal(ydToFt(100), 300);

  // Two-argument conversion
  assert.throws(() => moaToInch("1", 100), TypeError);
  assert.throws(() => moaToInch(1, "100"), TypeError);
  assert.throws(() => moaToInch(NaN, 100), RangeError);
  assert.throws(() => moaToInch(1, NaN), RangeError);
  assert.ok(Number.isFinite(moaToInch(1, 100)));
});

test("all operations after disposal throw Error", () => {
  const builder = new Builder();
  builder.dispose();
  assert.throws(() => builder.ballisticCoefficientPsi(0.243), /disposed/);
  assert.throws(() => builder.reset(), /disposed/);
  assert.throws(() => builder.build(), /disposed/);
  // dispose itself is idempotent
  builder.dispose();

  const context = new Builder()
    .ballisticCoefficientPsi(0.243)
    .initialVelocityFps(2700)
    .zeroDistanceYds(100)
    .build();
  context.dispose();
  assert.throws(() => context.solveFt([100]), /disposed/);
  assert.throws(() => context.solveYds([100]), /disposed/);
  assert.throws(() => context.solveELRFt([100]), /disposed/);
  assert.throws(() => context.solveELRYds([100]), /disposed/);
  assert.throws(() => context.solveInverseFt([100]), /disposed/);
  assert.throws(() => context.solveInverseYds([100]), /disposed/);
  assert.throws(() => context.solveInverseELRFt([100]), /disposed/);
  assert.throws(() => context.solveInverseELRYds([100]), /disposed/);
  // dispose itself is idempotent
  context.dispose();
});

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
