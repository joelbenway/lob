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
#include "lobber_cli.hpp"

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
std::string ErrorToString(lob::ErrorT err);

// Solver config from JSON
SolverPath JsonToSolverPath(const nlohmann::json& j);
SolverMode JsonToSolverMode(const nlohmann::json& j);
AngularUnit JsonToAngularUnit(const nlohmann::json& j);

// Multi-point parsing helpers
struct BandPoint {
  float fps{0.0F};
  float bc{0.0F};
};
std::vector<BandPoint> ParseBands(const nlohmann::json& j);
std::vector<BandPoint> ParseBandsFromString(const std::string& text);

struct DragTablePoint {
  float mach{0.0F};
  float cd{0.0F};
};
std::vector<DragTablePoint> ParseDragTable(const nlohmann::json& j);
std::vector<DragTablePoint> ParseDragTableFromString(const std::string& text);

std::vector<float> ParseSplineCoefficients(const nlohmann::json& j);
std::vector<float> ParseSplineCoefficientsFromString(const std::string& text);

std::vector<lob::WindPoint> ParseWindProfile(const nlohmann::json& j);
std::vector<lob::WindPoint> ParseWindProfileFromString(const std::string& text);

// Range parsing (defaults to 0–1000 yd in 100 yd increments if not specified)
std::vector<uint32_t> ParseRanges(const nlohmann::json& j);

// Context building
lob::Context BuildContextFromJson(const nlohmann::json& j);

// View helpers
void PrintTable(const lob::Context& ctx, const lob::Output* outputs,
                size_t count, size_t requested_count = 0,
                SolverMode mode = SolverMode::kForward,
                AngularUnit unit = AngularUnit::kMoa);
nlohmann::json OutputsToJson(const lob::Output* outputs, size_t count,
                             SolverMode mode = SolverMode::kForward,
                             AngularUnit unit = AngularUnit::kMoa);

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
