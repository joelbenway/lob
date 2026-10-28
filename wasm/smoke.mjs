// Copyright (c) 2026  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information
//
// Smoke test: instantiate the wasm module and check its exported ABI.
// Usage: node wasm/smoke.mjs [path/to/lob.mjs]

import { pathToFileURL } from "node:url";

const modulePath = process.argv[2] ?? "build/wasm/lob.mjs";
const createModule = (await import(pathToFileURL(modulePath).href)).default;
const wasm = await createModule();

const version = wasm.UTF8ToString(wasm._LobVersion());
if (!/^\d+\.\d+\.\d+$/.test(version)) {
  throw new Error(`unexpected version: ${version}`);
}

const builderSize = wasm._LobWasmBuilderSize();
const contextSize = wasm._LobWasmContextSize();
if (!(builderSize > 0) || !(contextSize > 0)) {
  throw new Error(
    `unexpected sizes: builder=${builderSize} context=${contextSize}`,
  );
}

console.log(
  `wasm smoke ok: lob ${version}, builder ${builderSize} bytes, ` +
    `context ${contextSize} bytes`,
);

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
