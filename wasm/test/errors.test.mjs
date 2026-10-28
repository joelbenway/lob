// Copyright (c) 2026  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

import assert from "node:assert/strict";
import { test } from "node:test";

const { Builder, LobError } = await import(
  new URL("../../build/wasm/index.mjs", import.meta.url).href
);

test("build throws LobError for missing initial velocity", () => {
  assert.throws(
    () =>
      new Builder()
        .ballisticCoefficientPsi(0.243)
        .initialVelocityFps(0)
        .zeroDistanceYds(100)
        .build(),
    (error) => error instanceof LobError && error.code === 14,
  );
});

test("build throws LobError for missing ballistic coefficient", () => {
  assert.throws(
    () => new Builder().initialVelocityFps(2700).zeroDistanceYds(100).build(),
    (error) => error instanceof LobError && error.code === 7,
  );
});

test("every error code has a named message", () => {
  for (let code = 0; code < 38; code++) {
    const error = new LobError(code);
    assert.equal(error.code, code);
    assert.doesNotMatch(error.message, /UnknownError/);
  }
});

test("build throws LobError for out-of-range ballistic coefficient", () => {
  assert.throws(
    () =>
      new Builder()
        .ballisticCoefficientPsi(-0.243)
        .initialVelocityFps(2700)
        .zeroDistanceYds(100)
        .build(),
    (error) => error instanceof LobError && error.code === 6,
  );
});

test("range input is validated", () => {
  const context = new Builder()
    .ballisticCoefficientPsi(0.243)
    .initialVelocityFps(2700)
    .zeroDistanceYds(100)
    .build();
  assert.throws(() => context.solve([100, 50]), RangeError);
  assert.throws(() => context.solve([-1]), RangeError);
  assert.throws(() => context.solve([1.5]), RangeError);
  assert.throws(() => context.solve("100"), TypeError);
  context.dispose();
});

test("invalid builder inputs are rejected early", () => {
  assert.throws(() => new Builder().windHeading(13), RangeError);
  assert.throws(() => new Builder().windHeading(0), RangeError);
  assert.throws(() => new Builder().bcDragFunction("G9"), RangeError);
  assert.throws(() => new Builder().bcAtmosphere("nope"), RangeError);
  assert.throws(() => new Builder().minimumSpeed(70000), RangeError);
  assert.throws(() => new Builder().splineCoefficients([1, 2]), RangeError);
  assert.throws(
    () => new Builder().bcVelocityBands([2000], [0.25, 0.25]),
    RangeError,
  );
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
