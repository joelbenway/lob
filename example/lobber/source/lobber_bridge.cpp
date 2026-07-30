// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include "lobber_bridge.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <string>
#include <vector>

#include "lob/lob.hpp"

namespace example {

namespace {

constexpr double kYardsToFeet = 3.0;

}  // namespace

double JsonToDouble(const nlohmann::json& j, const std::string& key) {
  if (!j.contains(key) || j[key].is_null()) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  if (j[key].is_string() && j[key].get<std::string>() == "nan") {
    return std::numeric_limits<double>::quiet_NaN();
  }
  return j[key].get<double>();
}

uint16_t JsonToU16(const nlohmann::json& j, const std::string& key) {
  if (!j.contains(key) || j[key].is_null()) {
    return 0;
  }
  if (j[key].is_string() && j[key].get<std::string>() == "nan") {
    return 0;
  }
  const double kV = j[key].get<double>();
  if (kV < 0 || kV > std::numeric_limits<uint16_t>::max()) {
    return 0;
  }
  return static_cast<uint16_t>(kV);
}

lob::AtmosphereReferenceT JsonToAtmosphere(const nlohmann::json& j,
                                           const std::string& key) {
  const double kV = JsonToDouble(j, key);
  if (std::isnan(kV)) {
    return lob::AtmosphereReferenceT::kArmyStandardMetro;
  }
  return static_cast<int>(std::round(kV)) == 2
             ? lob::AtmosphereReferenceT::kIcao
             : lob::AtmosphereReferenceT::kArmyStandardMetro;
}

lob::DragFunctionT JsonToDragFunction(const nlohmann::json& j,
                                      const std::string& key) {
  const double kV = JsonToDouble(j, key);
  if (std::isnan(kV)) {
    return lob::DragFunctionT::kG1;
  }
  const int kRounded = static_cast<int>(std::round(kV));
  switch (kRounded) {
    case static_cast<int>(lob::DragFunctionT::kG1):
    case static_cast<int>(lob::DragFunctionT::kG2):
    case static_cast<int>(lob::DragFunctionT::kG5):
    case static_cast<int>(lob::DragFunctionT::kG6):
    case static_cast<int>(lob::DragFunctionT::kG7):
    case static_cast<int>(lob::DragFunctionT::kG8):
      return static_cast<lob::DragFunctionT>(static_cast<uint8_t>(kRounded));
    default:
      return lob::DragFunctionT::kG1;
  }
}

lob::ClockAngleT JsonToClockAngle(const nlohmann::json& j,
                                  const std::string& key) {
  const double kV = JsonToDouble(j, key);
  if (std::isnan(kV)) {
    return lob::ClockAngleT::kXII;
  }
  const auto kRounded = static_cast<int>(std::round(kV));
  const int kMinClock = 1;
  const int kMaxClock = 12;
  if (kRounded >= kMinClock && kRounded <= kMaxClock) {
    return static_cast<lob::ClockAngleT>(static_cast<uint8_t>(kRounded));
  }
  return lob::ClockAngleT::kXII;
}

std::vector<uint32_t> ParseRanges(const nlohmann::json& j) {
  if (j.contains("Ranges") && j["Ranges"].is_array()) {
    std::vector<uint32_t> ranges;
    for (const auto& r : j["Ranges"]) {
      ranges.push_back(static_cast<uint32_t>(r.get<double>() * kYardsToFeet));
    }
    return ranges;
  }
  const std::array<uint32_t, 12> kDefaultRanges = {
      0U,
      static_cast<uint32_t>(50 * kYardsToFeet),
      static_cast<uint32_t>(100 * kYardsToFeet),
      static_cast<uint32_t>(200 * kYardsToFeet),
      static_cast<uint32_t>(300 * kYardsToFeet),
      static_cast<uint32_t>(400 * kYardsToFeet),
      static_cast<uint32_t>(500 * kYardsToFeet),
      static_cast<uint32_t>(600 * kYardsToFeet),
      static_cast<uint32_t>(700 * kYardsToFeet),
      static_cast<uint32_t>(800 * kYardsToFeet),
      static_cast<uint32_t>(900 * kYardsToFeet),
      static_cast<uint32_t>(1000 * kYardsToFeet)};
  return {kDefaultRanges.begin(), kDefaultRanges.end()};
}

void PrintTable(const lob::Context& ctx, const lob::Output* outputs,
                size_t count) {
  // Error states are reported by the lobber.cpp caller before this function
  // is reached; only valid contexts are printed.
  constexpr uint8_t kExtra = 3;

  // Extra info
  auto extra_width = [](const std::string& s) {
    return static_cast<int>(s.size() + kExtra);
  };
  const std::string kZA("Zero Angle");
  const std::string kSF("Stability Factor");
  const std::string kSS("Speed of Sound");

  std::cout << "\033[33m" << std::left << std::setw(extra_width(kZA)) << kZA
            << std::setw(extra_width(kSF)) << kSF << std::setw(extra_width(kSS))
            << kSS << "\033[0m\n";
  std::cout << std::left << std::setw(extra_width(kZA)) << std::fixed
            << std::setprecision(2) << ctx.zero_angle
            << std::setw(extra_width(kSF)) << ctx.stability_factor
            << std::setw(extra_width(kSS)) << ctx.speed_of_sound << "\n\n";

  // Table header
  const int kWidth = 12;
  std::cout << std::left << std::setw(kWidth) << "\033[32mYards"
            << std::setw(kWidth) << "FPS" << std::setw(kWidth) << "FtLbs"
            << std::setw(kWidth) << "Elev Inch" << std::setw(kWidth)
            << "Elev MOA" << std::setw(kWidth) << "Elev MIL"
            << std::setw(kWidth) << "Wind Inch" << std::setw(kWidth)
            << "Wind MOA" << std::setw(kWidth) << "Wind MIL"
            << std::setw(kWidth) << "Seconds" << "\033[0m\n";

  for (size_t i = 0; i < count; ++i) {
    std::cout << std::left << std::setw(kWidth)
              << outputs[i].range / static_cast<uint32_t>(kYardsToFeet)
              << std::setw(kWidth) << outputs[i].velocity << std::setw(kWidth)
              << outputs[i].energy << std::setw(kWidth) << std::fixed
              << std::setprecision(2) << outputs[i].elevation
              << std::setw(kWidth)
              << lob::InchToMoa(outputs[i].elevation, outputs[i].range)
              << std::setw(kWidth)
              << lob::InchToMil(outputs[i].elevation, outputs[i].range)
              << std::setw(kWidth) << outputs[i].deflection << std::setw(kWidth)
              << lob::InchToMoa(outputs[i].deflection, outputs[i].range)
              << std::setw(kWidth)
              << lob::InchToMil(outputs[i].deflection, outputs[i].range)
              << std::setw(kWidth) << std::setprecision(3)
              << outputs[i].time_of_flight << "\n";
  }
  std::cout << "\n";
}

nlohmann::json OutputsToJson(const lob::Output* outputs, size_t count) {
  nlohmann::json j = nlohmann::json::array();
  for (size_t i = 0; i < count; ++i) {
    j.push_back({{"range", outputs[i].range},
                 {"velocity", outputs[i].velocity},
                 {"energy", outputs[i].energy},
                 {"elevation", outputs[i].elevation},
                 {"deflection", outputs[i].deflection},
                 {"time_of_flight", outputs[i].time_of_flight}});
  }
  return j;
}

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
