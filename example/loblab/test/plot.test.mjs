// Copyright (c) 2026  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

import assert from "node:assert/strict";
import test from "node:test";

import { trajectoryChart } from "../plot.mjs";

const rows = [
  { rangeYds: 0, elevationInch: -1.5 },
  { rangeYds: 100, elevationInch: 0 },
  { rangeYds: 200, elevationInch: -12 },
];

test("maps the sampled curve across the chart width", () => {
  const chart = trajectoryChart(rows);
  assert.ok(chart);
  assert.equal(chart.xTicks[0].x, chart.left);
  assert.equal(chart.xTicks.at(-1).x, chart.right);
  assert.deepEqual(
    chart.xTicks.map((tick) => tick.label),
    ["0", "100", "200"],
  );
  assert.equal(chart.curve.split(" ").length, rows.length);
});

test("puts the zero line at zero elevation", () => {
  const chart = trajectoryChart(rows);
  assert.ok(chart);
  const ys = chart.curve.split(" ").map((point) => Number(point.split(",")[1]));
  assert.ok(ys.includes(Number(chart.zeroY.toFixed(1))));
  assert.ok(chart.zeroY > chart.top && chart.zeroY < chart.bottom);
});

test("needs at least two distinct ranges", () => {
  assert.equal(trajectoryChart([]), null);
  assert.equal(trajectoryChart([rows[0]]), null);
  assert.equal(trajectoryChart([rows[0], rows[0]]), null);
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
