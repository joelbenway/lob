// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#pragma once

#include <cstddef>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "lob/lob.hpp"

namespace example {

// JSON reader helpers
double JsonToDouble(const nlohmann::json& j, const std::string& key);
uint16_t JsonToU16(const nlohmann::json& j, const std::string& key);
lob::AtmosphereReferenceT JsonToAtmosphere(const nlohmann::json& j,
                                           const std::string& key);
lob::DragFunctionT JsonToDragFunction(const nlohmann::json& j,
                                      const std::string& key);
lob::ClockAngleT JsonToClockAngle(const nlohmann::json& j,
                                  const std::string& key);

// Range parsing (defaults to 0–1000 yd in 100 yd increments if not specified)
std::vector<uint32_t> ParseRanges(const nlohmann::json& j);

// View helpers
void PrintTable(const lob::Context& input, const lob::Output* outputs,
                size_t count);
nlohmann::json OutputsToJson(const lob::Output* outputs, size_t count);

}  // namespace example

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
