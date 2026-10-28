// Copyright (c) 2026  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

import { applyConfig, buildRanges, readConfig } from "./inputs.mjs";
import { trajectoryChart } from "./plot.mjs";

const form = document.querySelector("#solver-form");
const status = document.querySelector("#status");
const summary = document.querySelector("#model-summary");
const table = document.querySelector("#results");
const tbody = table.querySelector("tbody");
const plot = document.querySelector("#plot");
const plotSvg = document.querySelector("#plot-svg");

let lob;

function element(name) {
  return form.elements.namedItem(name);
}

function value(name) {
  return element(name)?.value ?? "";
}

function isChecked(name) {
  return Boolean(element(name)?.checked);
}

function setValue(name, next) {
  const control = element(name);
  if (control) {
    control.value = next;
  }
}

function hasHeight(text) {
  return text.split(/\r?\n/).some((line) => {
    const cells = line.split(",");
    return cells.length >= 4 && cells[3].trim() !== "";
  });
}

function syncVisibility() {
  const dragSource = value("dragSource");
  const tier = value("projectileTier");
  const zeroMode = value("zeroMode");
  const weather = value("weatherSource");
  const wind = value("windSource");
  const measuredAway = isChecked("measuredAway");

  const visible = {
    "drag-bc": dragSource === "bc",
    "drag-bands": dragSource === "bands",
    "drag-table": dragSource === "table",
    "drag-coefficients": dragSource === "coefficients",
    "drag-bc-ref": dragSource === "bc" || dragSource === "bands",
    "tier-mass": tier !== "bc",
    "tier-stability": tier === "stability" || tier === "full",
    "tier-full": tier === "full",
    "zero-distance": zeroMode === "distance",
    "zero-angle": zeroMode === "angle",
    "weather-altitude": weather === "altitude",
    "weather-measured": weather === "measured",
    "weather-humidity": weather === "altitude" || weather === "measured",
    "weather-barometer-alt":
      weather === "measured" &&
      measuredAway &&
      value("airPressureInHg").trim() !== "",
    "weather-thermometer-alt":
      weather === "measured" &&
      measuredAway &&
      value("temperatureDegF").trim() !== "",
    "wind-uniform": wind === "uniform",
    "wind-profile": wind === "profile",
    "wind-shear": wind === "profile" && hasHeight(value("windProfileText")),
    "range-angle": isChecked("rangeAngleEnabled"),
    coriolis: isChecked("coriolisEnabled"),
    advanced: isChecked("advancedEnabled"),
  };

  for (const group of form.querySelectorAll("[data-group]")) {
    const active = visible[group.dataset.group] ?? true;
    group.hidden = !active;
    for (const control of group.querySelectorAll(
      "input, select, textarea, button",
    )) {
      control.disabled = !active;
    }
  }

  const energyActive = isChecked("advancedEnabled") && tier !== "bc";
  for (const field of form.querySelectorAll("[data-energy]")) {
    field.hidden = !energyActive;
    for (const control of field.querySelectorAll(
      "input, select, textarea, button",
    )) {
      control.disabled = !energyActive;
    }
  }
}

function clearDependent(target) {
  if (target.name === "dragSource") {
    const stale = {
      bc: ["bandsText", "dragTableText", "coefficientsText"],
      bands: ["bc", "dragTableText", "coefficientsText"],
      table: ["bc", "bandsText", "coefficientsText"],
      coefficients: ["bc", "bandsText", "dragTableText"],
    };
    for (const name of stale[target.value] ?? []) {
      setValue(name, "");
    }
  }
  if (target.name === "zeroMode") {
    if (target.value === "distance") {
      setValue("zeroAngleMoa", "");
    } else {
      setValue("zeroYds", "");
      setValue("zeroImpactHeightInches", "");
    }
  }
  if (target.name === "windSource") {
    if (target.value === "uniform") {
      setValue("windProfileText", "");
      setValue("windShearExponent", "");
    } else if (target.value === "profile") {
      setValue("windSpeedMph", "");
      setValue("windClock", "");
    } else {
      setValue("windSpeedMph", "");
      setValue("windClock", "");
      setValue("windProfileText", "");
      setValue("windShearExponent", "");
    }
  }
}

function format(number, digits) {
  return Number.isFinite(number) ? number.toFixed(digits) : "—";
}

function makeCell(text, options = {}) {
  const cell = document.createElement("td");
  if (options.energy) {
    cell.dataset.energy = "";
  }
  if (options.col) {
    cell.dataset.col = options.col;
  }
  cell.textContent = text;
  return cell;
}

function renderRows(rows, config, inverse) {
  tbody.replaceChildren();
  for (const row of rows) {
    const tr = document.createElement("tr");
    tr.append(
      makeCell(format(row.rangeYds, 0)),
      makeCell(format(row.velocityFps, 0)),
      makeCell(format(row.energyFtLbs, 0), { energy: true }),
      makeCell(inverse ? "—" : format(row.elevationInch, 1), {
        col: "elevIn",
      }),
      makeCell(
        inverse
          ? format(row.elevationMoa, 2)
          : row.rangeFt > 0
            ? format(lob.inchToMoa(row.elevationInch, row.rangeFt), 2)
            : "—",
        { col: "elevMoa" },
      ),
      makeCell(inverse ? "—" : format(row.deflectionInch, 1), {
        col: "deflIn",
      }),
      makeCell(
        inverse
          ? format(row.deflectionMoa, 2)
          : row.rangeFt > 0
            ? format(lob.inchToMoa(row.deflectionInch, row.rangeFt), 2)
            : "—",
        { col: "deflMoa" },
      ),
      makeCell(format(row.timeOfFlightSec, 3)),
    );
    tbody.append(tr);
  }

  const hideEnergy = config.projectileTier === "bc";
  for (const cell of table.querySelectorAll("[data-energy]")) {
    cell.hidden = hideEnergy;
  }
  for (const cell of table.querySelectorAll(
    '[data-col="elevIn"], [data-col="deflIn"]',
  )) {
    cell.hidden = inverse;
  }
}

const SVG_NS = "http://www.w3.org/2000/svg";

function svgNode(name, attributes = {}, text) {
  const node = document.createElementNS(SVG_NS, name);
  for (const [key, attributeValue] of Object.entries(attributes)) {
    node.setAttribute(key, attributeValue);
  }
  if (text !== undefined) {
    node.textContent = text;
  }
  return node;
}

function drawPlot(rows, inverse) {
  const chart = inverse ? null : trajectoryChart(rows);
  plot.hidden = !chart;
  if (!chart) {
    plotSvg.replaceChildren();
    return;
  }

  const grid = [
    ...chart.yTicks.map(({ y }) =>
      svgNode("line", {
        class: "plot-grid",
        x1: chart.left,
        x2: chart.right,
        y1: y,
        y2: y,
      }),
    ),
    ...chart.xTicks.map(({ x }) =>
      svgNode("line", {
        class: "plot-grid",
        x1: x,
        x2: x,
        y1: chart.top,
        y2: chart.bottom,
      }),
    ),
  ];
  const labels = [
    ...chart.yTicks.map(({ y, label }) =>
      svgNode(
        "text",
        {
          class: "plot-tick",
          x: chart.left - 8,
          y: y + 4,
          "text-anchor": "end",
        },
        label,
      ),
    ),
    ...chart.xTicks.map(({ x, label, anchor }) =>
      svgNode(
        "text",
        {
          class: "plot-tick",
          x,
          y: chart.height - 9,
          "text-anchor": anchor,
        },
        label,
      ),
    ),
  ];

  plotSvg.replaceChildren(
    ...grid,
    svgNode("path", { class: "plot-area", d: chart.area }),
    svgNode("line", {
      class: "plot-zero",
      x1: chart.left,
      x2: chart.right,
      y1: chart.zeroY,
      y2: chart.zeroY,
    }),
    svgNode("polyline", { class: "plot-line", points: chart.curve }),
    ...labels,
  );
  plotSvg.setAttribute("viewBox", `0 0 ${chart.width} ${chart.height}`);
  plotSvg.setAttribute(
    "aria-label",
    `Elevation from ${format(chart.minInch, 1)} to ${format(chart.maxInch, 1)} inches over ${format(chart.maxYds, 0)} yards.`,
  );
}

function updateSummary(config, elr, inverse) {
  const atmosphere = {
    ArmyStandardMetro: "Army Standard Metro",
    ICAO: "ICAO",
  }[config.bcAtmosphere];
  const drag = {
    bc: `BC ${config.bc} (${config.dragFunction}, ${atmosphere})`,
    bands: `velocity bands (${config.dragFunction}, ${atmosphere})`,
    table: "custom Mach/Cd table",
    coefficients: "native spline coefficients",
  }[config.dragSource];
  const tier = {
    bc: "BC only",
    mass: `${config.massGrains} gr mass`,
    stability: "mass and stability geometry",
    full: "full bullet geometry",
  }[config.projectileTier];
  const weather = {
    standard: "standard atmosphere",
    altitude: `altitude ${config.altitudeOfFiringSiteFt} ft`,
    measured: "measured atmosphere",
  }[config.weatherSource];
  const wind = {
    none: "calm",
    uniform: `${config.windSpeedMph} mph at ${config.windClock} o’clock`,
    profile: `${config.windProfile?.length ?? 0} wind profile points`,
  }[config.windSource];
  const earth = [
    config.rangeAngleEnabled ? `range angle ${config.rangeAngleDeg}°` : null,
    config.coriolisEnabled ? "Coriolis" : null,
  ]
    .filter(Boolean)
    .join(" and ");

  summary.textContent = `${drag}; ${tier}; ${weather}; ${wind}; ${
    earth || "no Earth effects"
  }; ${elr ? "ELR" : "fast"} ${inverse ? "inverse" : "forward"}.`;
}

function solve(event) {
  event?.preventDefault();
  if (!lob) {
    return;
  }

  status.dataset.state = "";
  let builder;
  let context;
  try {
    const config = readConfig(Object.fromEntries(new FormData(form)));
    const ranges = buildRanges(config.maxYds, config.stepYds);
    const elr = value("solverPath") === "elr";
    const inverse = value("solutionKind") === "inverse";
    builder = new lob.Builder();
    context = applyConfig(builder, config).build();
    const started = performance.now();
    const rows = inverse
      ? elr
        ? context.solveInverseELRYds(ranges)
        : context.solveInverseYds(ranges)
      : elr
        ? context.solveELRYds(ranges)
        : context.solveYds(ranges);
    const elapsedMs = performance.now() - started;
    const elapsed = elapsedMs < 1 ? "< 1 ms" : `${Math.round(elapsedMs)} ms`;

    renderRows(rows, config, inverse);
    drawPlot(rows, inverse);
    updateSummary(config, elr, inverse);
    if (rows.length < ranges.length) {
      status.textContent = `Trajectory fell short after ${rows.length} of ${ranges.length} ranges in ${elapsed}.`;
    } else {
      status.textContent = `Solved ${rows.length} ranges with lob ${lob.version()} in ${elapsed}.`;
    }
  } catch (error) {
    tbody.replaceChildren();
    plot.hidden = true;
    status.dataset.state = "error";
    status.textContent = error instanceof Error ? error.message : String(error);
  } finally {
    context?.dispose();
    builder?.dispose();
  }
}

form.addEventListener("submit", solve);
form.addEventListener("change", (event) => {
  clearDependent(event.target);
  syncVisibility();
  solve();
});
form.addEventListener("input", syncVisibility);

async function load() {
  syncVisibility();
  try {
    lob = await import("./index.mjs");
    status.textContent = `Ready. lob ${lob.version()}.`;
    solve();
  } catch (error) {
    status.dataset.state = "error";
    status.textContent = `Could not load solver: ${
      error instanceof Error ? error.message : String(error)
    }`;
    for (const control of form.querySelectorAll(
      "button, input, select, textarea",
    )) {
      control.disabled = true;
    }
  }
}

load();

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
