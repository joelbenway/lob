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

/**
 * @brief Reads a double-precision floating point number from a JSON object.
 * @param j JSON object to read from.
 * @param key Key name to look up.
 * @return Parsed double value, or NaN if absent or invalid.
 */
double JsonToDouble(const nlohmann::json& j, const std::string& key);

/**
 * @brief Reads a 16-bit unsigned integer from a JSON object.
 * @param j JSON object to read from.
 * @param key Key name to look up.
 * @return Parsed uint16_t value, or 0 if absent or invalid.
 */
uint16_t JsonToU16(const nlohmann::json& j, const std::string& key);

/**
 * @brief Parses an atmosphere reference enum from a JSON object.
 * @param j JSON object to read from.
 * @param key Key name to look up.
 * @return AtmosphereReferenceT matching the key string, or ICAO by default.
 */
lob::AtmosphereReferenceT JsonToAtmosphere(const nlohmann::json& j,
                                           const std::string& key);

/**
 * @brief Parses a standard drag function enum from a JSON object.
 * @param j JSON object to read from.
 * @param key Key name to look up.
 * @return DragFunctionT matching the key string, or G1 by default.
 */
lob::DragFunctionT JsonToDragFunction(const nlohmann::json& j,
                                      const std::string& key);

/**
 * @brief Parses a clock angle enum from a JSON object.
 * @param j JSON object to read from.
 * @param key Key name to look up.
 * @return ClockAngleT matching the key string or number, or 12 o'clock by
 * default.
 */
lob::ClockAngleT JsonToClockAngle(const nlohmann::json& j,
                                  const std::string& key);

/**
 * @brief Converts a Lob error code to a human-readable description string.
 * @param err Error code from the Lob library.
 * @return Descriptive string explaining the error.
 */
std::string ErrorToString(lob::ErrorT err);

/**
 * @brief Resolves the solver path (Fast vs ELR) from JSON.
 * @param j JSON object containing solver configuration.
 * @return Resolved SolverPath enum.
 */
SolverPath JsonToSolverPath(const nlohmann::json& j);

/**
 * @brief Resolves the solver mode (Forward vs Inverse) from JSON.
 * @param j JSON object containing solver configuration.
 * @return Resolved SolverMode enum.
 */
SolverMode JsonToSolverMode(const nlohmann::json& j);

/**
 * @brief Resolves the angular unit (MOA vs MIL) from JSON.
 * @param j JSON object containing angular unit configuration.
 * @return Resolved AngularUnit enum.
 */
AngularUnit JsonToAngularUnit(const nlohmann::json& j);

/**
 * @brief Represents a velocity-dependent ballistic coefficient data point.
 */
struct BandPoint {
  float fps{0.0F};  ///< @brief Velocity station in feet per second.
  float bc{0.0F};   ///< @brief Ballistic coefficient at this velocity.
};

/**
 * @brief Parses velocity-dependent BC bands from a JSON object.
 * @param j JSON object containing band definitions.
 * @return Vector of parsed BandPoint entries.
 */
std::vector<BandPoint> ParseBands(const nlohmann::json& j);

/**
 * @brief Parses velocity-dependent BC bands from delimited text.
 * @param text Delimited string of velocity and BC pairs.
 * @return Vector of parsed BandPoint entries.
 */
std::vector<BandPoint> ParseBandsFromString(const std::string& text);

/**
 * @brief Represents a Mach vs Drag coefficient (Cd) point.
 */
struct DragTablePoint {
  float mach{0.0F};  ///< @brief Mach number.
  float cd{0.0F};    ///< @brief Drag coefficient.
};

/**
 * @brief Parses Mach vs Cd points from a JSON object.
 * @param j JSON object containing drag table definitions.
 * @return Vector of parsed DragTablePoint entries.
 */
std::vector<DragTablePoint> ParseDragTable(const nlohmann::json& j);

/**
 * @brief Parses Mach vs Cd points from delimited text.
 * @param text Delimited string of Mach and Cd pairs.
 * @return Vector of parsed DragTablePoint entries.
 */
std::vector<DragTablePoint> ParseDragTableFromString(const std::string& text);

/**
 * @brief Parses 60 spline coefficients from a JSON object.
 * @param j JSON object containing spline coefficients.
 * @return Vector of parsed floating-point coefficients.
 */
std::vector<float> ParseSplineCoefficients(const nlohmann::json& j);

/**
 * @brief Parses 60 spline coefficients from delimited text.
 * @param text Delimited string of floating-point numbers.
 * @return Vector of parsed floating-point coefficients.
 */
std::vector<float> ParseSplineCoefficientsFromString(const std::string& text);

/**
 * @brief Parses downrange wind profile stations from a JSON object.
 * @param j JSON object containing wind profile definitions.
 * @return Vector of parsed WindPoint stations.
 */
std::vector<lob::WindPoint> ParseWindProfile(const nlohmann::json& j);

/**
 * @brief Parses downrange wind profile stations from delimited text.
 * @param text Delimited string of wind profile stations.
 * @return Vector of parsed WindPoint stations.
 */
std::vector<lob::WindPoint> ParseWindProfileFromString(const std::string& text);

/**
 * @brief Parses target downrange distances in feet from a JSON object.
 * @param j JSON object containing Ranges or MaxYds/StepYds.
 * @return Vector of ranges in feet.
 */
std::vector<uint32_t> ParseRanges(const nlohmann::json& j);

/**
 * @brief Builds a Lob solver Context from a JSON object.
 * @param j Complete JSON configuration object.
 * @return Initialized Lob Context ready for solving.
 */
lob::Context BuildContextFromJson(const nlohmann::json& j);

/**
 * @brief Prints formatted tabular trajectory output to standard output.
 * @param ctx Configured solver context.
 * @param outputs Array of solved Output records.
 * @param count Number of valid solved outputs in the array.
 * @param requested_count Number of target ranges requested.
 * @param mode Solution mode (forward or inverse).
 * @param unit Angular unit (MOA or MIL).
 */
void PrintTable(const lob::Context& ctx, const lob::Output* outputs,
                size_t count, size_t requested_count = 0,
                SolverMode mode = SolverMode::kForward,
                AngularUnit unit = AngularUnit::kMoa);

/**
 * @brief Serializes trajectory outputs to a JSON array.
 * @param outputs Array of solved Output records.
 * @param count Number of solved output records.
 * @param mode Solution mode (forward or inverse).
 * @param unit Angular unit (MOA or MIL).
 * @return JSON array containing serialized output records.
 */
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
