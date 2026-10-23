// Copyright (c) 2026  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

import assert from "node:assert/strict";
import { test } from "node:test";

const api = await import(
  new URL("../../build/wasm/index.mjs", import.meta.url).href
);
const {
  Atmosphere,
  Builder,
  DragFunction,
  degCToDegF,
  ftToYd,
  inchToMoa,
  moaToInch,
  version,
  ydToFt,
} = api;

function makeBuilder() {
  return new Builder()
    .ballisticCoefficientPsi(0.243)
    .bcDragFunction(DragFunction.G7)
    .diameterInch(0.308)
    .massGrains(168)
    .initialVelocityFps(2700)
    .zeroDistanceYds(100);
}

test("version is a semantic version string", () => {
  assert.match(version(), /^\d+\.\d+\.\d+$/);
});

test("builder setters are chainable", () => {
  const builder = new Builder();
  assert.equal(builder.ballisticCoefficientPsi(0.243), builder);
  assert.equal(builder.bcDragFunction("G7"), builder);
  assert.equal(builder.bcAtmosphere(Atmosphere.ICAO), builder);
  assert.equal(builder.initialVelocityFps(2700), builder);
  assert.equal(builder.zeroDistanceYds(100), builder);
  assert.equal(builder.windHeading(3), builder);
  assert.equal(builder.minimumSpeed(1000), builder);
  assert.equal(builder.maximumTime(2), builder);
  const context = builder.build();
  assert.equal(context.solveYds([100]).length, 1);
  context.dispose();
  builder.dispose();
});

test("solveYds returns forward solutions with rows in range order", () => {
  const context = makeBuilder().build();
  const rows = context.solveYds([0, 100, 200, 300]);
  assert.equal(rows.length, 4);
  assert.deepEqual(
    rows.map((row) => row.rangeYds),
    [0, 100, 200, 300],
  );
  for (const row of rows) {
    assert.ok(Number.isFinite(row.velocityFps));
    assert.ok(row.velocityFps > 0);
    assert.ok(Number.isFinite(row.energyFtLbs));
    assert.ok(Number.isFinite(row.elevationInch));
    assert.ok(Number.isFinite(row.deflectionInch));
    assert.ok(Number.isFinite(row.timeOfFlightSec));
    assert.equal("elevationMoa" in row, false);
  }
  for (let i = 1; i < rows.length; i++) {
    assert.ok(rows[i].velocityFps < rows[i - 1].velocityFps);
  }
  context.dispose();
});

test("solve in feet matches solveYds", () => {
  const context = makeBuilder().build();
  const feet = context.solve([0, 300, 600, 900]);
  const yards = context.solveYds([0, 100, 200, 300]);
  assert.equal(feet.length, yards.length);
  for (let i = 0; i < feet.length; i++) {
    assert.equal(feet[i].rangeFt, yards[i].rangeFt);
    assert.equal(feet[i].velocityFps, yards[i].velocityFps);
    assert.equal(feet[i].elevationInch, yards[i].elevationInch);
  }
  context.dispose();
});

test("inverse solutions return MOA adjustments", () => {
  const context = makeBuilder().build();
  const rows = context.solveInverseYds([100, 200, 300]);
  assert.equal(rows.length, 3);
  for (const row of rows) {
    assert.ok(Number.isFinite(row.elevationMoa));
    assert.ok(Number.isFinite(row.deflectionMoa));
    assert.equal("elevationInch" in row, false);
  }
  context.dispose();
});

test("cheap inverse counts zero ranges", () => {
  const context = makeBuilder().build();
  const cheap = context.solveInverseYds([0, 100, 200]);
  assert.equal(cheap.length, 3);
  assert.equal(cheap[0].elevationMoa, 0);
  assert.equal(cheap[0].deflectionMoa, 0);
  context.dispose();
});

test("ELR forward matches fast forward on ordinary trajectories", () => {
  const context = makeBuilder().build();
  const fast = context.solveYds([0, 100, 200, 300]);
  const elr = context.solveELRYds([0, 100, 200, 300]);
  assert.equal(elr.length, fast.length);
  for (let i = 0; i < fast.length; i++) {
    assert.equal(elr[i].rangeYds, fast[i].rangeYds);
    assert.ok(Math.abs(elr[i].velocityFps - fast[i].velocityFps) <= 1);
    assert.ok(Number.isFinite(elr[i].elevationInch));
  }
  context.dispose();
});

test("ELR inverse refines the cheap answer", () => {
  const context = makeBuilder().build();
  const cheap = context.solveInverseYds([100, 200, 300]);
  const elr = context.solveInverseELRYds([100, 200, 300]);
  assert.equal(elr.length, cheap.length);
  for (let i = 0; i < cheap.length; i++) {
    assert.ok(Number.isFinite(elr[i].elevationMoa));
    assert.ok(Math.abs(elr[i].elevationMoa - cheap[i].elevationMoa) < 0.5);
  }
  context.dispose();
});

test("ELR inverse counts zero ranges", () => {
  const context = makeBuilder().build();
  const rows = context.solveInverseELRYds([0, 100, 200]);
  assert.equal(rows.length, 3);
  assert.equal(rows[0].elevationMoa, 0);
  assert.equal(rows[0].deflectionMoa, 0);
  assert.ok(Number.isFinite(rows[1].elevationMoa));
  context.dispose();
});

test("ELR inverse stops at the first unreachable range", () => {
  const context = makeBuilder().build();
  const rows = context.solveInverseELR([300, 50000]);
  assert.equal(rows.length, 1);
  assert.ok(Number.isFinite(rows[0].elevationMoa));
  assert.deepEqual(context.solveInverseELR([50000]), []);
  context.dispose();
});

test("BC velocity bands build and solve", () => {
  const context = new Builder()
    .ballisticCoefficientPsi(0.25)
    .bcAtmosphere(Atmosphere.ICAO)
    .bcDragFunction("G1")
    .bcVelocityBands([2000, 3000], [0.25, 0.25])
    .diameterInch(0.308)
    .massGrains(168)
    .initialVelocityFps(2700)
    .zeroDistanceYds(100)
    .build();
  assert.equal(context.solveYds([100]).length, 1);
  context.dispose();
});

test("wind profile build and solve", () => {
  const context = makeBuilder()
    .windProfile([
      { rangeFt: 0, headingDeg: 90, speedMph: 10, heightFt: 1 },
      { rangeFt: 1000, headingDeg: 90, speedMph: 10, heightFt: 10 },
    ])
    .build();
  assert.equal(context.solveYds([100, 200]).length, 2);
  context.dispose();
});

test("unit conversions are exposed", () => {
  assert.ok(Math.abs(ydToFt(100) - 300) < 1e-9);
  assert.ok(Math.abs(ftToYd(300) - 100) < 1e-9);
  assert.ok(Math.abs(degCToDegF(100) - 212) < 1e-9);
  assert.ok(Math.abs(moaToInch(inchToMoa(1, 100), 100) - 1) < 1e-9);
});

test("fall-short trajectories return only reached rows", () => {
  const context = makeBuilder().build();
  const rows = context.solve([50000]);
  assert.equal(rows.length, 1);
  assert.ok(rows[0].rangeFt < 50000);
  context.dispose();
});

test("large range lists grow wasm memory safely", () => {
  const context = makeBuilder().build();
  const ranges = Array.from({ length: 200000 }, (_, index) => index);
  const rows = context.solve(ranges);
  assert.ok(rows.length > 0);
  assert.ok(rows.length <= ranges.length);
  context.dispose();
});

test("builder can build twice after replacing arrays", () => {
  const builder = makeBuilder()
    .bcVelocityBands([2000, 3000], [0.24, 0.24])
    .diameterInch(0.308)
    .massGrains(168);
  const first = builder.build();
  const firstRow = first.solveYds([300])[0];
  first.dispose();
  builder.bcVelocityBands([2000, 3000], [0.26, 0.26]);
  const second = builder.build();
  const secondRow = second.solveYds([300])[0];
  second.dispose();
  builder.dispose();
  assert.ok(Number.isFinite(firstRow.elevationInch));
  assert.ok(Number.isFinite(secondRow.elevationInch));
  assert.notEqual(firstRow.elevationInch, secondRow.elevationInch);
});

test("reset clears configuration and releases retained arrays", () => {
  const builder = makeBuilder().bcVelocityBands([2000, 3000], [0.24, 0.24]);
  const first = builder.build();
  assert.equal(first.solveYds([100]).length, 1);
  first.dispose();

  builder.reset();
  builder
    .ballisticCoefficientPsi(0.243)
    .bcDragFunction("G7")
    .diameterInch(0.308)
    .massGrains(168)
    .initialVelocityFps(2700)
    .zeroDistanceYds(100)
    .bcVelocityBands([2000, 3000], [0.26, 0.26]);
  const second = builder.build();
  assert.equal(second.solveYds([100]).length, 1);
  second.dispose();
  builder.dispose();
});

test("Context cannot be constructed directly", () => {
  assert.throws(() => new api.Context(undefined), /Builder\.build/);
});

test("dispose is idempotent and use after dispose throws", () => {
  const builder = new Builder();
  builder.dispose();
  builder.dispose();
  assert.throws(() => builder.build(), /disposed/);

  const context = makeBuilder().build();
  context.dispose();
  context.dispose();
  assert.throws(() => context.solveYds([100]), /disposed/);
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
