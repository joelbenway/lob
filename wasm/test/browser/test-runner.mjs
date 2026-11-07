// Copyright (c) 2026  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

const statusEl = document.querySelector("#status");
const outputEl = document.querySelector("#output");

function log(msg) {
  outputEl.textContent += msg + "\n";
  console.log(msg);
}

async function report(result) {
  try {
    await fetch("/api/report", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(result),
    });
  } catch (err) {
    console.error("Failed to post report to server:", err);
  }
}

async function runTests() {
  const checks = [];
  function assert(condition, message) {
    if (!condition) {
      throw new Error(`Assertion failed: ${message}`);
    }
    checks.push(message);
    log(`✔ ${message}`);
  }

  log("Importing @joelbenway/lob module...");
  const {
    Builder,
    Context,
    DragFunction,
    Atmosphere,
    LobError,
    version,
    ydToFt,
  } = await import("/package/index.mjs");

  // 1. Version and error class check
  const ver = version();
  assert(/^\d+\.\d+\.\d+$/.test(ver), `version is valid semver: ${ver}`);
  assert(
    typeof LobError === "function",
    "LobError is exported as an error class",
  );

  // 2. Unit conversions
  assert(ydToFt(100) === 300, "ydToFt converts 100 yards to 300 feet");

  // 3. Build valid ballistic context
  log("Building canonical .308 G7 context...");
  const builder = new Builder()
    .ballisticCoefficientPsi(0.243)
    .bcDragFunction(DragFunction.G7)
    .bcAtmosphere(Atmosphere.ICAO)
    .diameterInch(0.308)
    .massGrains(168)
    .initialVelocityFps(2700)
    .zeroDistanceYds(100);

  const context = builder.build();
  assert(
    context instanceof Context,
    "builder.build() returned a Context instance",
  );

  // 4. Forward solve in yards
  log("Solving forward trajectory in yards...");
  const yds = context.solveYds([0, 100, 200, 300]);
  assert(yds.length === 4, "solveYds returned 4 rows");
  assert(yds[0].rangeYds === 0, "row 0 range is 0 yd");
  assert(
    Math.abs(yds[0].elevationInch - -1.5) < 0.1,
    "row 0 elevation is optic offset (~ -1.5 in)",
  );
  assert(yds[1].rangeYds === 100, "row 1 range is 100 yd");
  assert(
    Math.abs(yds[1].elevationInch) < 0.1,
    "row 1 elevation is ~0 at 100 yd zero",
  );
  assert(yds[2].elevationInch < yds[1].elevationInch, "bullet drops at 200 yd");
  assert(
    yds[3].velocityFps < yds[2].velocityFps,
    "velocity monotonically decreases",
  );
  assert(yds[3].timeOfFlightSec > 0, "time of flight is positive");

  // 5. Forward solve in feet
  log("Solving forward trajectory in feet...");
  const ft = context.solveFt([0, 300, 600, 900]);
  assert(ft.length === 4, "solveFt returned 4 rows");
  assert(ft[1].rangeFt === 300, "row 1 range is 300 ft");
  assert(
    Math.abs(ft[1].elevationInch - yds[1].elevationInch) < 1e-4,
    "solveFt matches solveYds",
  );

  // 6. Inverse solve in yards
  log("Solving geometric inverse trajectory...");
  const inv = context.solveInverseYds([100, 200, 300]);
  assert(inv.length === 3, "solveInverseYds returned 3 rows");
  assert(
    Math.abs(inv[0].elevationMoa) < 0.1,
    "inverse elevation at 100 yd zero is ~0 MOA",
  );
  assert(
    inv[1].elevationMoa > 1.0 && inv[1].elevationMoa < 2.5,
    "inverse elevation at 200 yd is positive (~1.8 MOA)",
  );
  assert(
    inv.every(
      (row) =>
        "elevationMoa" in row &&
        "deflectionMoa" in row &&
        !("elevationInch" in row),
    ),
    "inverse rows expose elevationMoa and deflectionMoa, and not elevationInch",
  );

  // 7. Memory growth test
  log("Testing WebAssembly memory growth with 50,000 range points...");
  const largeRanges = Array.from({ length: 50000 }, (_, i) => i + 1);
  const largeRows = context.solveFt(largeRanges);
  assert(
    largeRows.length > 0,
    `large solve completed with ${largeRows.length} rows`,
  );

  // 8. Disposal lifecycle
  log("Testing disposal lifecycle...");
  context.dispose();
  context.dispose(); // idempotent
  let disposedThrew = false;
  try {
    context.solveYds([100]);
  } catch (err) {
    disposedThrew = /disposed/.test(err.message);
  }
  assert(
    disposedThrew,
    "calling solveYds after disposal throws disposed error",
  );

  builder.dispose();

  statusEl.dataset.state = "pass";
  statusEl.textContent = `PASS (${checks.length} assertions passed)`;
  log(`\nALL ${checks.length} BROWSER CHECKS PASSED.`);

  return {
    status: "pass",
    checksCount: checks.length,
    userAgent: navigator.userAgent,
  };
}

runTests()
  .then((res) => report(res))
  .catch((err) => {
    statusEl.dataset.state = "fail";
    statusEl.textContent = `FAIL: ${err.message}`;
    log(`\nFAIL: ${err.message}\n${err.stack}`);
    report({
      status: "fail",
      error: err.message,
      stack: err.stack,
      userAgent: navigator.userAgent,
    });
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
