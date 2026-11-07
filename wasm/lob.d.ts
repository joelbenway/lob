// Copyright (c) 2026  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

/**
 * Low-level WebAssembly instance exposing the raw C ABI functions and memory buffers.
 */
export interface LobRawModule {
  _malloc(bytes: number): number;
  _free(ptr: number): void;
  _LobVersion(): number;
  _LobWasmBuilderSize(): number;
  _LobWasmContextSize(): number;
  _LobWasmWindPointSize(): number;
  _LobWasmOutputSize(): number;
  _LobWasmContextError(ptr: number): number;
  _LobWasmOutputsToDoubles(
    outputs: number,
    count: number,
    doubles: number,
  ): void;

  UTF8ToString(ptr: number): string;
  HEAPU8: Uint8Array;
  HEAPU32: Uint32Array;
  HEAPF32: Float32Array;
  HEAPF64: Float64Array;

  [key: `_${string}`]: (...args: any[]) => any;
}

/**
 * Asynchronously instantiates the low-level Emscripten WebAssembly module.
 */
export default function createLobModule(
  moduleArgs?: Record<string, unknown>,
): Promise<LobRawModule>;

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
