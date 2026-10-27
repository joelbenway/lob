// Copyright (c) 2026  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

const WIDTH = 640;
const HEIGHT = 220;
const LEFT = 52;
const RIGHT = 18;
const TOP = 12;
const BOTTOM = 30;

/**
 * Maps forward-solve rows to a polyline, area, and tick positions in SVG
 * viewBox units. Returns null when there is no curve to draw.
 */
export function trajectoryChart(rows) {
  const samples = rows
    .filter(
      (row) =>
        Number.isFinite(row.rangeYds) && Number.isFinite(row.elevationInch),
    )
    .map((row) => [row.rangeYds, row.elevationInch]);
  const maxX = Math.max(...samples.map(([x]) => x));

  if (samples.length < 2 || !(maxX > 0)) {
    return null;
  }

  const lowY = Math.min(0, ...samples.map(([, elevation]) => elevation));
  const highY = Math.max(0, ...samples.map(([, elevation]) => elevation));
  const padY = (highY - lowY || 1) * 0.06;
  const topY = highY + padY;
  const bottomY = lowY - padY;

  const x = (value) => LEFT + (value / maxX) * (WIDTH - LEFT - RIGHT);
  const y = (value) =>
    TOP + ((topY - value) / (topY - bottomY)) * (HEIGHT - TOP - BOTTOM);

  const points = samples.map(
    ([range, elevation]) => `${x(range).toFixed(1)},${y(elevation).toFixed(1)}`,
  );
  const zeroY = y(0);

  return {
    width: WIDTH,
    height: HEIGHT,
    left: LEFT,
    right: WIDTH - RIGHT,
    top: TOP,
    bottom: HEIGHT - BOTTOM,
    curve: points.join(" "),
    area: [
      `M ${x(samples[0][0]).toFixed(1)},${zeroY.toFixed(1)}`,
      ...points.map((point) => `L ${point}`),
      `L ${x(maxX).toFixed(1)},${zeroY.toFixed(1)}`,
      "Z",
    ].join(" "),
    zeroY,
    xTicks: [
      { x: LEFT, label: "0", anchor: "start" },
      { x: x(maxX / 2), label: String(Math.round(maxX / 2)), anchor: "middle" },
      { x: WIDTH - RIGHT, label: String(Math.round(maxX)), anchor: "end" },
    ],
    yTicks: [lowY, (lowY + highY) / 2, highY].map((value) => ({
      y: y(value),
      label: String(Math.round(value)),
    })),
    maxYds: maxX,
    minInch: lowY,
    maxInch: highY,
  };
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
