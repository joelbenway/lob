// Copyright (c) 2026  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

import assert from "node:assert/strict";
import { readFileSync } from "node:fs";
import { test } from "node:test";

const build = new URL("../../build/wasm/", import.meta.url);
const api = await import(new URL("index.mjs", build).href);

test("raw module exports every function declared in lob.h", async () => {
  const header = readFileSync(
    new URL("../../include/lob/lob.h", import.meta.url),
    "utf8",
  );
  const names = header
    .split("\n")
    .filter((line) => line.startsWith("LOB_EXPORT"))
    .map((line) => /\b(Lob\w+)\(/.exec(line)?.[1])
    .filter(Boolean);
  assert.equal(names.length, 91);

  const createModule = (await import(new URL("lob.mjs", build).href)).default;
  const raw = await createModule();
  for (const name of names) {
    assert.equal(typeof raw[`_${name}`], "function", name);
  }
  for (const shim of [
    "_malloc",
    "_free",
    "_LobWasmBuilderSize",
    "_LobWasmContextSize",
    "_LobWasmWindPointSize",
    "_LobWasmOutputSize",
    "_LobWasmContextError",
    "_LobWasmOutputsToDoubles",
  ]) {
    assert.equal(typeof raw[shim], "function", shim);
  }
  assert.equal(raw.UTF8ToString(raw._LobVersion()), api.version());
});

test("wrapper exports the public API", () => {
  const names = [
    "Builder",
    "Context",
    "LobError",
    "DragFunction",
    "Atmosphere",
    "version",
    "moaToMil",
    "moaToDeg",
    "moaToIphy",
    "moaToInch",
    "milToMoa",
    "milToDeg",
    "milToIphy",
    "milToInch",
    "degToMoa",
    "degToMil",
    "inchToMoa",
    "inchToMil",
    "inchToDeg",
    "jToFtLbs",
    "ftLbsToJ",
    "mToYd",
    "ydToFt",
    "mToFt",
    "ftToIn",
    "mmToIn",
    "cmToIn",
    "ydToM",
    "ftToM",
    "ftToYd",
    "inToMm",
    "inToCm",
    "inToFt",
    "paToInHg",
    "mbarToInHg",
    "psiToInHg",
    "lbsToGrain",
    "gToGrain",
    "kgToGrain",
    "kgSqMToPmsi",
    "fpsToMps",
    "mpsToFps",
    "kphToMph",
    "knToMph",
    "msToS",
    "usToS",
    "sToMs",
    "sToUs",
    "degCToDegF",
  ];
  for (const name of names) {
    assert.ok(name in api, name);
  }
  assert.equal(api.ydToFt(100), 300);
  assert.equal(api.ftToYd(300), 100);
  assert.ok(Math.abs(api.inchToMoa(1, 100) - 2.8648) < 1e-3);
  assert.ok(Math.abs(api.moaToInch(api.inchToMoa(1, 100), 100) - 1) < 1e-9);
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
