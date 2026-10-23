// Copyright (c) 2026  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include <cstddef>
#include <cstdint>

#include "lob/lob.h"

namespace {
constexpr size_t kFieldsPerOutput = 6;

static_assert(sizeof(LobWindPoint) == 4 * sizeof(double),
              "JS wrapper writes four doubles per wind point");
}  // namespace

extern "C" {

// Sizes are exported so the JavaScript wrapper never hardcodes the wasm ABI.
size_t LobWasmBuilderSize(void) { return sizeof(LobBuilder); }

size_t LobWasmContextSize(void) { return sizeof(LobContext); }

size_t LobWasmWindPointSize(void) { return sizeof(LobWindPoint); }

size_t LobWasmOutputSize(void) { return sizeof(LobOutput); }

uint8_t LobWasmContextError(const LobContext* pctx) {
  return pctx == nullptr ? static_cast<uint8_t>(kLobErrorInternalError)
                         : pctx->error;
}

// Flattens six fields per output row: range, velocity, energy, elevation,
// deflection, and time of flight.
void LobWasmOutputsToDoubles(const LobOutput* pouts, size_t size,
                             double* pdst) {
  if (pouts == nullptr || pdst == nullptr) {
    return;
  }
  for (size_t i = 0; i < size; i++) {
    double* prow = pdst + (i * kFieldsPerOutput);
    *prow++ = static_cast<double>(pouts[i].range);
    *prow++ = static_cast<double>(pouts[i].velocity);
    *prow++ = static_cast<double>(pouts[i].energy);
    *prow++ = pouts[i].elevation;
    *prow++ = pouts[i].deflection;
    *prow = pouts[i].time_of_flight;
  }
}

}  // extern "C"

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
