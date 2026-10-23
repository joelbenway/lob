// Copyright (c) 2026  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

import assert from "node:assert/strict";
import { readFileSync } from "node:fs";
import { test } from "node:test";

import {
  Atmosphere,
  Builder,
  DragFunction,
  LobError,
} from "../../build/wasm/index.mjs";

const header = readFileSync(
  new URL("../../include/lob/lob.h", import.meta.url),
  "utf8",
);

function enumEntries(typeName, prefix) {
  const start = header.indexOf(`typedef uint8_t ${typeName};`);
  assert.ok(start >= 0, `missing ${typeName}`);
  const open = header.indexOf("enum {", start);
  const close = header.indexOf("};", open);
  const block = header.slice(open, close);
  const entries = [];
  let previous = -1;
  for (const match of block.matchAll(
    new RegExp(`^\\s*${prefix}(\\w+)(?:\\s*=\\s*(\\d+))?`, "gm"),
  )) {
    const value = match[2] === undefined ? previous + 1 : Number(match[2]);
    entries.push([match[1], value]);
    previous = value;
  }
  return entries;
}

test("drag and atmosphere enums match lob.h", () => {
  const drag = enumEntries("LobDragFunctionT", "kLobDragFunction");
  assert.deepEqual(Object.entries(DragFunction), drag);

  const atmosphere = enumEntries(
    "LobAtmosphereReferenceT",
    "kLobAtmosphereReference",
  );
  assert.equal(Object.keys(Atmosphere).length, atmosphere.length);
  for (const [name, value] of atmosphere) {
    const key = name === "Icao" ? "ICAO" : name;
    assert.equal(Atmosphere[key], value, name);
  }
});

test("every lob.h error code has a name and message", () => {
  const errors = enumEntries("LobErrorT", "kLobError");
  for (const [name, value] of errors) {
    assert.equal(new LobError(value).message, `lob: ${name} (code ${value})`);
  }
});

test("windHeading accepts every lob.h clock angle", () => {
  const clock = enumEntries("LobClockAngleT", "kLobClockAngle");
  const builder = new Builder();
  try {
    for (const [name, value] of clock) {
      assert.equal(builder.windHeading(value), builder, name);
    }
    assert.throws(() => builder.windHeading(0), RangeError);
  } finally {
    builder.dispose();
  }
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
