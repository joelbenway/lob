// Copyright (c) 2026  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Smoke test: instantiate the wasm module and call one exported function.
// Usage: node wasm/smoke.mjs [path/to/lob_exhibits.mjs]

import { pathToFileURL } from 'node:url';

const modulePath =
  process.argv[2] ?? 'build-wasm/wasm/lob_exhibits.mjs';
const createModule = (await import(pathToFileURL(modulePath).href)).default;
const wasm = await createModule();

if (wasm._LobExhibitsVersion() !== 1) {
  throw new Error('unexpected exhibits version');
}

const cdG1 = wasm._LobExhibitDragEvalG1(1.0);
const cdG7 = wasm._LobExhibitDragEvalG7(1.0);
if (!(cdG1 > 0.0) || !(cdG7 > 0.0)) {
  throw new Error(`unexpected drag coefficients: G1=${cdG1} G7=${cdG7}`);
}

console.log(
  `wasm smoke ok: Cd(G1, M1.0)=${cdG1.toFixed(6)} ` +
  `Cd(G7, M1.0)=${cdG7.toFixed(6)}`,
);
