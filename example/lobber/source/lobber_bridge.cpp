// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include "lobber_bridge.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <sstream>
#include <string>
#include <vector>

#include "lob/lob.hpp"
#include "lobber_cli.hpp"

namespace example {

namespace {

constexpr double kYardsToFeet = 3.0;

std::string Trim(const std::string& s) {
  auto start = s.begin();
  while (start != s.end() &&
         std::isspace(static_cast<unsigned char>(*start)) != 0) {
    ++start;
  }
  auto end = s.end();
  do {
    if (end == start) {
      break;
    }
    --end;
  } while (std::isspace(static_cast<unsigned char>(*end)) != 0);
  return {start, end + 1};
}

std::vector<std::string> SplitLines(const std::string& text) {
  std::vector<std::string> lines;
  std::istringstream stream(text);
  std::string line;
  while (std::getline(stream, line)) {
    std::string trimmed = Trim(line);
    if (!trimmed.empty()) {
      lines.push_back(trimmed);
    }
  }
  return lines;
}

std::vector<std::string> SplitTokens(const std::string& line) {
  std::vector<std::string> tokens;
  std::string cur;
  for (char ch : line) {
    if (ch == ',' || ch == '\t' || ch == ' ') {
      if (!cur.empty()) {
        tokens.push_back(cur);
        cur.clear();
      }
    } else {
      cur += ch;
    }
  }
  if (!cur.empty()) {
    tokens.push_back(cur);
  }
  return tokens;
}

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
  if (!j.contains(key) || j[key].is_null()) {
    return lob::AtmosphereReferenceT::kArmyStandardMetro;
  }
  if (j[key].is_string()) {
    const std::string kVal = j[key].get<std::string>();
    if (kVal == "1" || kVal == "2" || kVal == "icao" || kVal == "ICAO") {
      return lob::AtmosphereReferenceT::kIcao;
    }
    return lob::AtmosphereReferenceT::kArmyStandardMetro;
  }
  const double kV = JsonToDouble(j, key);
  if (std::isnan(kV)) {
    return lob::AtmosphereReferenceT::kArmyStandardMetro;
  }
  const int kRounded = static_cast<int>(std::round(kV));
  return (kRounded == static_cast<int>(lob::AtmosphereReferenceT::kIcao) ||
          kRounded == 2)
             ? lob::AtmosphereReferenceT::kIcao
             : lob::AtmosphereReferenceT::kArmyStandardMetro;
}

lob::DragFunctionT JsonToDragFunction(const nlohmann::json& j,
                                      const std::string& key) {
  if (!j.contains(key) || j[key].is_null()) {
    return lob::DragFunctionT::kG1;
  }
  if (j[key].is_string()) {
    const std::string kStr = j[key].get<std::string>();
    if (kStr == "G1" || kStr == "g1" || kStr == "1") {
      return lob::DragFunctionT::kG1;
    }
    if (kStr == "G2" || kStr == "g2" || kStr == "2") {
      return lob::DragFunctionT::kG2;
    }
    if (kStr == "G5" || kStr == "g5" || kStr == "5") {
      return lob::DragFunctionT::kG5;
    }
    if (kStr == "G6" || kStr == "g6" || kStr == "6") {
      return lob::DragFunctionT::kG6;
    }
    if (kStr == "G7" || kStr == "g7" || kStr == "7") {
      return lob::DragFunctionT::kG7;
    }
    if (kStr == "G8" || kStr == "g8" || kStr == "8") {
      return lob::DragFunctionT::kG8;
    }
  }
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

std::string ErrorToString(lob::ErrorT err) {
  switch (err) {
    case lob::ErrorT::kNone:
      return "None";
    case lob::ErrorT::kAirPressureOOR:
      return "Air pressure out of range";
    case lob::ErrorT::kAltitudeOfBarometerOOR:
      return "Barometer altitude out of range";
    case lob::ErrorT::kAltitudeOfFiringSiteOOR:
      return "Firing site altitude out of range";
    case lob::ErrorT::kAltitudeOfThermometerOOR:
      return "Thermometer altitude out of range";
    case lob::ErrorT::kAzimuthOOR:
      return "Azimuth out of range";
    case lob::ErrorT::kBallisticCoefficientOOR:
      return "Ballistic coefficient out of range";
    case lob::ErrorT::kBallisticCoefficientRequired:
      return "Ballistic coefficient required";
    case lob::ErrorT::kBaseDiameterOOR:
      return "Base diameter out of range";
    case lob::ErrorT::kBcBandsInvalid:
      return "BC velocity bands invalid";
    case lob::ErrorT::kBcBandsNotMonotonic:
      return "BC velocity bands not strictly increasing";
    case lob::ErrorT::kBcBandsTooShort:
      return "BC velocity bands too short (at least 2 required)";
    case lob::ErrorT::kDiameterOOR:
      return "Diameter out of range";
    case lob::ErrorT::kHumidityOOR:
      return "Humidity out of range";
    case lob::ErrorT::kInitialVelocityRequired:
      return "Initial velocity required";
    case lob::ErrorT::kInternalError:
      return "Internal solver error";
    case lob::ErrorT::kLatitudeOOR:
      return "Latitude out of range";
    case lob::ErrorT::kLaunchAngleRequired:
      return "Launch angle required";
    case lob::ErrorT::kLengthOOR:
      return "Length out of range";
    case lob::ErrorT::kMachDragTableInvalid:
      return "Mach drag table invalid";
    case lob::ErrorT::kMachDragTableNotMonotonic:
      return "Mach drag table not strictly increasing";
    case lob::ErrorT::kMachDragTableTooShort:
      return "Mach drag table too short (at least 2 required)";
    case lob::ErrorT::kMassOOR:
      return "Mass out of range";
    case lob::ErrorT::kMaximumTimeOOR:
      return "Maximum time out of range";
    case lob::ErrorT::kMeplatDiameterOOR:
      return "Meplat diameter out of range";
    case lob::ErrorT::kNoseLengthOOR:
      return "Nose length out of range";
    case lob::ErrorT::kNotFormed:
      return "Context not formed";
    case lob::ErrorT::kOgiveRtROOR:
      return "Ogive Rt/R out of range";
    case lob::ErrorT::kRangeAngleOOR:
      return "Range angle out of range";
    case lob::ErrorT::kSplineCoefsInvalid:
      return "Spline coefficients invalid";
    case lob::ErrorT::kTailLengthOOR:
      return "Tail length out of range";
    case lob::ErrorT::kWindHeadingOOR:
      return "Wind heading out of range";
    case lob::ErrorT::kWindProfileInvalid:
      return "Wind profile invalid";
    case lob::ErrorT::kWindProfileNotMonotonic:
      return "Wind profile ranges not strictly increasing";
    case lob::ErrorT::kWindProfileTooLong:
      return "Wind profile too long (maximum 8 points)";
    case lob::ErrorT::kZeroAngleOOR:
      return "Zero angle out of range";
    case lob::ErrorT::kZeroDistanceOOR:
      return "Zero distance out of range";
    case lob::ErrorT::kZeroUnreachable:
      return "Zero range unreachable by projectile";
    default:
      return "Unknown error (" + std::to_string(static_cast<int>(err)) + ")";
  }
}

SolverPath JsonToSolverPath(const nlohmann::json& j) {
  if (j.contains("SolverPath") && j["SolverPath"].is_string()) {
    std::string val = j["SolverPath"].get<std::string>();
    std::transform(val.begin(), val.end(), val.begin(), [](unsigned char c) {
      return static_cast<char>(std::tolower(c));
    });
    if (val == "elr") {
      return SolverPath::kELR;
    }
    if (val == "fast") {
      return SolverPath::kFast;
    }
  }
  return SolverPath::kAuto;
}

SolverMode JsonToSolverMode(const nlohmann::json& j) {
  if (j.contains("SolverMode") && j["SolverMode"].is_string()) {
    std::string val = j["SolverMode"].get<std::string>();
    std::transform(val.begin(), val.end(), val.begin(), [](unsigned char c) {
      return static_cast<char>(std::tolower(c));
    });
    if (val == "inverse") {
      return SolverMode::kInverse;
    }
    if (val == "forward") {
      return SolverMode::kForward;
    }
  }
  return SolverMode::kAuto;
}

AngularUnit JsonToAngularUnit(const nlohmann::json& j) {
  if (j.contains("AngularUnit") && j["AngularUnit"].is_string()) {
    std::string val = j["AngularUnit"].get<std::string>();
    std::transform(val.begin(), val.end(), val.begin(), [](unsigned char c) {
      return static_cast<char>(std::tolower(c));
    });
    if (val == "mil") {
      return AngularUnit::kMil;
    }
    if (val == "moa") {
      return AngularUnit::kMoa;
    }
  }
  return AngularUnit::kMoa;
}

std::vector<BandPoint> ParseBandsFromString(const std::string& text) {
  std::vector<BandPoint> result;
  for (const auto& line : SplitLines(text)) {
    auto tokens = SplitTokens(line);
    if (tokens.size() >= 2) {
      char* end1 = nullptr;
      char* end2 = nullptr;
      float fps = std::strtof(tokens[0].c_str(), &end1);
      float bc = std::strtof(tokens[1].c_str(), &end2);
      if (end1 != tokens[0].c_str() && end2 != tokens[1].c_str()) {
        result.push_back({fps, bc});
      }
    }
  }
  return result;
}

std::vector<BandPoint> ParseBands(const nlohmann::json& j) {
  std::vector<BandPoint> bands;
  const std::string keys[] = {"BCVelocityBands", "bands", "bandsText"};
  for (const auto& key : keys) {
    if (!j.contains(key) || j[key].is_null()) {
      continue;
    }
    if (j[key].is_string()) {
      return ParseBandsFromString(j[key].get<std::string>());
    }
    if (j[key].is_object()) {
      if (j[key].contains("fps") && j[key].contains("bcs") &&
          j[key]["fps"].is_array() && j[key]["bcs"].is_array()) {
        const size_t n = std::min(j[key]["fps"].size(), j[key]["bcs"].size());
        for (size_t i = 0; i < n; ++i) {
          bands.push_back(
              {j[key]["fps"][i].get<float>(), j[key]["bcs"][i].get<float>()});
        }
        return bands;
      }
    }
    if (j[key].is_array()) {
      for (const auto& item : j[key]) {
        if (item.is_array() && item.size() >= 2) {
          bands.push_back({item[0].get<float>(), item[1].get<float>()});
        } else if (item.is_object()) {
          float fps = 0.0f;
          float bc = 0.0f;
          if (item.contains("fps")) {
            fps = item["fps"].get<float>();
          } else if (item.contains("velocity")) {
            fps = item["velocity"].get<float>();
          }
          if (item.contains("bc")) {
            bc = item["bc"].get<float>();
          }
          if (fps > 0.0f && bc > 0.0f) {
            bands.push_back({fps, bc});
          }
        }
      }
      return bands;
    }
  }
  return bands;
}

std::vector<DragTablePoint> ParseDragTableFromString(const std::string& text) {
  std::vector<DragTablePoint> result;
  for (const auto& line : SplitLines(text)) {
    auto tokens = SplitTokens(line);
    if (tokens.size() >= 2) {
      char* end1 = nullptr;
      char* end2 = nullptr;
      float mach = std::strtof(tokens[0].c_str(), &end1);
      float cd = std::strtof(tokens[1].c_str(), &end2);
      if (end1 != tokens[0].c_str() && end2 != tokens[1].c_str()) {
        result.push_back({mach, cd});
      }
    }
  }
  return result;
}

std::vector<DragTablePoint> ParseDragTable(const nlohmann::json& j) {
  std::vector<DragTablePoint> table;
  const std::string keys[] = {"SplineFitTable", "DragTable", "MachVsDragTable",
                              "dragTableText", "table"};
  for (const auto& key : keys) {
    if (!j.contains(key) || j[key].is_null()) {
      continue;
    }
    if (j[key].is_string()) {
      return ParseDragTableFromString(j[key].get<std::string>());
    }
    if (j[key].is_object()) {
      if (j[key].contains("machs") && j[key].contains("drags") &&
          j[key]["machs"].is_array() && j[key]["drags"].is_array()) {
        const size_t n =
            std::min(j[key]["machs"].size(), j[key]["drags"].size());
        for (size_t i = 0; i < n; ++i) {
          table.push_back({j[key]["machs"][i].get<float>(),
                           j[key]["drags"][i].get<float>()});
        }
        return table;
      }
    }
    if (j[key].is_array()) {
      for (const auto& item : j[key]) {
        if (item.is_array() && item.size() >= 2) {
          table.push_back({item[0].get<float>(), item[1].get<float>()});
        } else if (item.is_object()) {
          float mach = 0.0f;
          float cd = 0.0f;
          if (item.contains("mach")) {
            mach = item["mach"].get<float>();
          }
          if (item.contains("cd")) {
            cd = item["cd"].get<float>();
          } else if (item.contains("drag")) {
            cd = item["drag"].get<float>();
          }
          table.push_back({mach, cd});
        }
      }
      return table;
    }
  }
  return table;
}

std::vector<float> ParseSplineCoefficientsFromString(const std::string& text) {
  std::vector<float> result;
  for (const auto& line : SplitLines(text)) {
    for (const auto& tok : SplitTokens(line)) {
      char* end = nullptr;
      float val = std::strtof(tok.c_str(), &end);
      if (end != tok.c_str() && std::isfinite(val)) {
        result.push_back(val);
      }
    }
  }
  return result;
}

std::vector<float> ParseSplineCoefficients(const nlohmann::json& j) {
  const std::string keys[] = {"SplineCoefficients", "coefficients",
                              "coefficientsText"};
  for (const auto& key : keys) {
    if (!j.contains(key) || j[key].is_null()) {
      continue;
    }
    if (j[key].is_string()) {
      return ParseSplineCoefficientsFromString(j[key].get<std::string>());
    }
    if (j[key].is_array()) {
      std::vector<float> coefs;
      for (const auto& val : j[key]) {
        if (val.is_number()) {
          coefs.push_back(val.get<float>());
        }
      }
      return coefs;
    }
  }
  return {};
}

std::vector<lob::WindPoint> ParseWindProfileFromString(
    const std::string& text) {
  std::vector<lob::WindPoint> result;
  for (const auto& line : SplitLines(text)) {
    auto tokens = SplitTokens(line);
    if (tokens.size() >= 3) {
      char* end1 = nullptr;
      char* end2 = nullptr;
      char* end3 = nullptr;
      double range = std::strtod(tokens[0].c_str(), &end1);
      double heading = std::strtod(tokens[1].c_str(), &end2);
      double speed = std::strtod(tokens[2].c_str(), &end3);
      double height = 0.0;
      if (tokens.size() >= 4) {
        char* end4 = nullptr;
        height = std::strtod(tokens[3].c_str(), &end4);
      }
      if (end1 != tokens[0].c_str() && end2 != tokens[1].c_str() &&
          end3 != tokens[2].c_str()) {
        result.push_back({range, heading, speed, height});
      }
    }
  }
  return result;
}

std::vector<lob::WindPoint> ParseWindProfile(const nlohmann::json& j) {
  const std::string keys[] = {"WindProfile", "windProfile", "windProfileText"};
  for (const auto& key : keys) {
    if (!j.contains(key) || j[key].is_null()) {
      continue;
    }
    if (j[key].is_string()) {
      return ParseWindProfileFromString(j[key].get<std::string>());
    }
    if (j[key].is_array()) {
      std::vector<lob::WindPoint> profile;
      for (const auto& item : j[key]) {
        if (item.is_array() && item.size() >= 3) {
          double range = item[0].get<double>();
          double heading = item[1].get<double>();
          double speed = item[2].get<double>();
          double height = item.size() >= 4 ? item[3].get<double>() : 0.0;
          profile.push_back({range, heading, speed, height});
        } else if (item.is_object()) {
          double range = 0.0;
          double heading = 0.0;
          double speed = 0.0;
          double height = 0.0;
          if (item.contains("range_ft")) {
            range = item["range_ft"].get<double>();
          } else if (item.contains("range")) {
            range = item["range"].get<double>();
          }
          if (item.contains("heading_deg")) {
            heading = item["heading_deg"].get<double>();
          } else if (item.contains("heading")) {
            heading = item["heading"].get<double>();
          }
          if (item.contains("speed_mph")) {
            speed = item["speed_mph"].get<double>();
          } else if (item.contains("speed")) {
            speed = item["speed"].get<double>();
          }
          if (item.contains("height_ft")) {
            height = item["height_ft"].get<double>();
          } else if (item.contains("height")) {
            height = item["height"].get<double>();
          }
          profile.push_back({range, heading, speed, height});
        }
      }
      return profile;
    }
  }
  return {};
}

std::vector<uint32_t> ParseRanges(const nlohmann::json& j) {
  if (j.contains("Ranges") && j["Ranges"].is_array()) {
    std::vector<uint32_t> ranges;
    for (const auto& r : j["Ranges"]) {
      ranges.push_back(static_cast<uint32_t>(r.get<double>() * kYardsToFeet));
    }
    return ranges;
  }
  if (j.contains("ranges") && j["ranges"].is_array()) {
    std::vector<uint32_t> ranges;
    for (const auto& r : j["ranges"]) {
      ranges.push_back(static_cast<uint32_t>(r.get<double>() * kYardsToFeet));
    }
    return ranges;
  }
  if (j.contains("MaxYds") || j.contains("maxYds")) {
    double max_yds = j.contains("MaxYds") ? j["MaxYds"].get<double>()
                                          : j["maxYds"].get<double>();
    double step_yds = 100.0;
    if (j.contains("StepYds")) {
      step_yds = j["StepYds"].get<double>();
    } else if (j.contains("stepYds")) {
      step_yds = j["stepYds"].get<double>();
    }
    if (step_yds <= 0.0) {
      step_yds = 100.0;
    }
    std::vector<uint32_t> ranges;
    for (double r = 0.0; r <= max_yds; r += step_yds) {
      ranges.push_back(static_cast<uint32_t>(r * kYardsToFeet));
    }
    if (ranges.empty() ||
        ranges.back() != static_cast<uint32_t>(max_yds * kYardsToFeet)) {
      ranges.push_back(static_cast<uint32_t>(max_yds * kYardsToFeet));
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

lob::Context BuildContextFromJson(const nlohmann::json& j) {
  lob::Builder builder;

  // Local buffers to hold array data alive until builder.Build() finishes
  std::vector<float> band_fps;
  std::vector<float> band_bcs;
  std::vector<float> table_mach;
  std::vector<float> table_cd;
  std::vector<float> spline_coeffs;
  std::vector<lob::WindPoint> wind_points;

  // Drag source determination
  std::string drag_source;
  if (j.contains("DragSource") && j["DragSource"].is_string()) {
    drag_source = j["DragSource"].get<std::string>();
  } else if (j.contains("dragSource") && j["dragSource"].is_string()) {
    drag_source = j["dragSource"].get<std::string>();
  }

  auto bands = ParseBands(j);
  auto table = ParseDragTable(j);
  auto coefs = ParseSplineCoefficients(j);

  if (drag_source == "coefficients" ||
      (!coefs.empty() && drag_source.empty() && bands.empty() &&
       table.empty() &&
       std::isnan(JsonToDouble(j, "BallisticCoefficientPsi")))) {
    if (coefs.size() == lob::kLobCoeffsSize) {
      spline_coeffs = std::move(coefs);
      builder.SplineCoefficients(spline_coeffs.data());
    }
  } else if (drag_source == "table" ||
             (!table.empty() && drag_source.empty() && bands.empty() &&
              std::isnan(JsonToDouble(j, "BallisticCoefficientPsi")))) {
    if (table.size() >= 2) {
      table_mach.resize(table.size());
      table_cd.resize(table.size());
      for (size_t i = 0; i < table.size(); ++i) {
        table_mach[i] = table[i].mach;
        table_cd[i] = table[i].cd;
      }
      builder.MachVsDragTable(table_mach.data(), table_cd.data(), table.size());
    }
  } else if (drag_source == "bands" ||
             (!bands.empty() && drag_source.empty() &&
              std::isnan(JsonToDouble(j, "BallisticCoefficientPsi")))) {
    if (bands.size() >= 2) {
      band_fps.resize(bands.size());
      band_bcs.resize(bands.size());
      for (size_t i = 0; i < bands.size(); ++i) {
        band_fps[i] = bands[i].fps;
        band_bcs[i] = bands[i].bc;
      }
      builder.BCVelocityBands(band_fps.data(), band_bcs.data(), bands.size());
      builder.BCDragFunction(JsonToDragFunction(j, "BCDragFunction"));
      builder.BCAtmosphere(JsonToAtmosphere(j, "BCAtmosphere"));
    }
  } else {
    double bc = JsonToDouble(j, "BallisticCoefficientPsi");
    if (std::isnan(bc)) {
      bc = JsonToDouble(j, "bc");
    }
    if (!std::isnan(bc) && bc > 0.0) {
      builder.BallisticCoefficientPsi(bc);
      builder.BCDragFunction(JsonToDragFunction(j, "BCDragFunction"));
      builder.BCAtmosphere(JsonToAtmosphere(j, "BCAtmosphere"));
    }
  }

  // Muzzle velocity
  uint16_t mv = JsonToU16(j, "InitialVelocityFps");
  if (mv == 0) {
    mv = JsonToU16(j, "velocityFps");
  }
  if (mv > 0) {
    builder.InitialVelocityFps(mv);
  }

  // Projectile detail tiers
  double mass = JsonToDouble(j, "MassGrains");
  if (std::isnan(mass)) {
    mass = JsonToDouble(j, "massGrains");
  }
  if (!std::isnan(mass) && mass > 0.0) {
    builder.MassGrains(mass);
  }

  double dia = JsonToDouble(j, "DiameterInch");
  if (std::isnan(dia)) {
    dia = JsonToDouble(j, "diameterInch");
  }
  double len = JsonToDouble(j, "LengthInch");
  if (std::isnan(len)) {
    len = JsonToDouble(j, "lengthInch");
  }
  double twist = JsonToDouble(j, "TwistInchesPerTurn");
  if (std::isnan(twist)) {
    twist = JsonToDouble(j, "twistInchesPerTurn");
  }
  if (!std::isnan(dia) && dia > 0.0 && !std::isnan(len) && len > 0.0 &&
      !std::isnan(twist) && twist > 0.0) {
    builder.DiameterInch(dia).LengthInch(len).TwistInchesPerTurn(twist);
  }

  double nose = JsonToDouble(j, "NoseLengthInch");
  if (std::isnan(nose)) {
    nose = JsonToDouble(j, "noseLengthInch");
  }
  double meplat = JsonToDouble(j, "MeplatDiameterInch");
  if (std::isnan(meplat)) {
    meplat = JsonToDouble(j, "meplatDiameterInch");
  }
  double base = JsonToDouble(j, "BaseDiameterInch");
  if (std::isnan(base)) {
    base = JsonToDouble(j, "baseDiameterInch");
  }
  double tail = JsonToDouble(j, "TailLengthInch");
  if (std::isnan(tail)) {
    tail = JsonToDouble(j, "tailLengthInch");
  }
  double ogive = JsonToDouble(j, "OgiveRtR");
  if (std::isnan(ogive)) {
    ogive = JsonToDouble(j, "ogiveRtR");
  }
  if (!std::isnan(nose) && nose > 0.0 && !std::isnan(base) && base > 0.0 &&
      !std::isnan(ogive) && ogive > 0.0) {
    builder.NoseLengthInch(nose).BaseDiameterInch(base).OgiveRtR(ogive);
    if (!std::isnan(meplat) && meplat >= 0.0) {
      builder.MeplatDiameterInch(meplat);
    }
    if (!std::isnan(tail) && tail >= 0.0) {
      builder.TailLengthInch(tail);
    }
  }

  // Zero
  double zero_angle = JsonToDouble(j, "ZeroAngleMOA");
  if (std::isnan(zero_angle)) {
    zero_angle = JsonToDouble(j, "zeroAngleMoa");
  }
  double zero_dist = JsonToDouble(j, "ZeroDistanceYds");
  if (std::isnan(zero_dist)) {
    zero_dist = JsonToDouble(j, "zeroYds");
  }
  double zero_impact = JsonToDouble(j, "ZeroImpactHeightInches");
  if (std::isnan(zero_impact)) {
    zero_impact = JsonToDouble(j, "zeroImpactHeightInches");
  }
  if (!std::isnan(zero_angle)) {
    builder.ZeroAngleMOA(zero_angle);
  } else if (!std::isnan(zero_dist) && zero_dist > 0.0) {
    builder.ZeroDistanceYds(zero_dist);
    if (!std::isnan(zero_impact)) {
      builder.ZeroImpactHeightInches(zero_impact);
    }
  }

  double optic_ht = JsonToDouble(j, "OpticHeightInches");
  if (std::isnan(optic_ht)) {
    optic_ht = JsonToDouble(j, "sightHeightInches");
  }
  if (!std::isnan(optic_ht) && optic_ht >= 0.0) {
    builder.OpticHeightInches(optic_ht);
  }

  // Atmosphere
  double alt = JsonToDouble(j, "AltitudeOfFiringSiteFt");
  if (std::isnan(alt)) {
    alt = JsonToDouble(j, "altitudeOfFiringSiteFt");
  }
  if (!std::isnan(alt)) {
    builder.AltitudeOfFiringSiteFt(alt);
  }

  double pres = JsonToDouble(j, "AirPressureInHg");
  if (std::isnan(pres)) {
    pres = JsonToDouble(j, "airPressureInHg");
  }
  if (!std::isnan(pres) && pres > 0.0) {
    builder.AirPressureInHg(pres);
  }

  double baro_alt = JsonToDouble(j, "AltitudeOfBarometerFt");
  if (std::isnan(baro_alt)) {
    baro_alt = JsonToDouble(j, "altitudeOfBarometerFt");
  }
  if (!std::isnan(baro_alt)) {
    builder.AltitudeOfBarometerFt(baro_alt);
  }

  double temp = JsonToDouble(j, "TemperatureDegF");
  if (std::isnan(temp)) {
    temp = JsonToDouble(j, "temperatureDegF");
  }
  if (!std::isnan(temp)) {
    builder.TemperatureDegF(temp);
  }

  double thermo_alt = JsonToDouble(j, "AltitudeOfThermometerFt");
  if (std::isnan(thermo_alt)) {
    thermo_alt = JsonToDouble(j, "altitudeOfThermometerFt");
  }
  if (!std::isnan(thermo_alt)) {
    builder.AltitudeOfThermometerFt(thermo_alt);
  }

  double rh = JsonToDouble(j, "RelativeHumidityPercent");
  if (std::isnan(rh)) {
    rh = JsonToDouble(j, "relativeHumidityPercent");
  }
  if (!std::isnan(rh) && rh >= 0.0) {
    builder.RelativeHumidityPercent(rh);
  }

  // Wind
  wind_points = ParseWindProfile(j);
  if (!wind_points.empty()) {
    builder.WindProfile(wind_points.data(), wind_points.size());
    double shear = JsonToDouble(j, "WindShearExponent");
    if (std::isnan(shear)) {
      shear = JsonToDouble(j, "windShearExponent");
    }
    if (!std::isnan(shear) && shear >= 0.0) {
      builder.WindShearExponent(shear);
    }
  } else {
    double wspd = JsonToDouble(j, "WindSpeedMph");
    if (std::isnan(wspd)) {
      wspd = JsonToDouble(j, "windSpeedMph");
    }
    if (!std::isnan(wspd) && wspd > 0.0) {
      builder.WindSpeedMph(wspd);
      if (j.contains("WindHeading")) {
        builder.WindHeading(JsonToClockAngle(j, "WindHeading"));
      } else if (j.contains("windClock")) {
        builder.WindHeading(JsonToClockAngle(j, "windClock"));
      }
    }
  }

  // Shot geometry & Coriolis
  double range_angle = JsonToDouble(j, "RangeAngleDeg");
  if (std::isnan(range_angle)) {
    range_angle = JsonToDouble(j, "rangeAngleDeg");
  }
  if (!std::isnan(range_angle)) {
    builder.RangeAngleDeg(range_angle);
  }

  double lat = JsonToDouble(j, "LatitudeDeg");
  if (std::isnan(lat)) {
    lat = JsonToDouble(j, "latitudeDeg");
  }
  double az = JsonToDouble(j, "AzimuthDeg");
  if (std::isnan(az)) {
    az = JsonToDouble(j, "azimuthDeg");
  }
  if (!std::isnan(lat) && !std::isnan(az)) {
    builder.LatitudeDeg(lat).AzimuthDeg(az);
  }

  // Advanced controls
  uint16_t step_size = JsonToU16(j, "StepSize");
  if (step_size == 0) {
    step_size = JsonToU16(j, "stepSizeInch");
  }
  if (step_size > 0) {
    builder.StepSize(step_size);
  }

  uint16_t min_spd = JsonToU16(j, "MinimumSpeed");
  if (min_spd == 0) {
    min_spd = JsonToU16(j, "minimumSpeedFps");
  }
  if (min_spd > 0) {
    builder.MinimumSpeed(min_spd);
  }

  uint16_t min_en = JsonToU16(j, "MinimumEnergy");
  if (min_en == 0) {
    min_en = JsonToU16(j, "minimumEnergyFtLbs");
  }
  if (min_en > 0) {
    builder.MinimumEnergy(min_en);
  }

  double max_t = JsonToDouble(j, "MaximumTime");
  if (std::isnan(max_t)) {
    max_t = JsonToDouble(j, "maximumTimeSec");
  }
  if (!std::isnan(max_t) && max_t > 0.0) {
    builder.MaximumTime(max_t);
  }

  return builder.Build();
}

void PrintTable(const lob::Context& ctx, const lob::Output* outputs,
                size_t count, size_t requested_count, SolverMode mode,
                AngularUnit unit) {
  constexpr uint8_t kExtra = 3;

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

  if (mode == SolverMode::kInverse) {
    std::cout
        << "\033[36mMode: Inverse Solution (Sight Adjustments: +Elevation "
           "dials UP, +Windage dials RIGHT)\033[0m\n\n";
  } else {
    std::cout << "\033[36mMode: Forward Solution (Trajectory Drop and Windage "
                 "in Inches)\033[0m\n\n";
  }

  if (requested_count > 0 && count < requested_count) {
    std::cout << "\033[33mNotice: Solved " << count << " of " << requested_count
              << " ranges. Projectile stopped early (fall-short or cutoff "
                 "condition met).\033[0m\n\n";
  }

  const int kWidth = 12;
  if (mode == SolverMode::kInverse) {
    std::cout << "\033[32m" << std::left << std::setw(kWidth) << "Yards"
              << std::setw(kWidth) << "FPS" << std::setw(kWidth) << "FtLbs"
              << std::setw(kWidth)
              << (unit == AngularUnit::kMil ? "Hold MIL" : "Hold MOA")
              << std::setw(kWidth)
              << (unit == AngularUnit::kMil ? "Hold MOA" : "Hold MIL")
              << std::setw(kWidth) << "Elev Inch" << std::setw(kWidth)
              << (unit == AngularUnit::kMil ? "Wind MIL" : "Wind MOA")
              << std::setw(kWidth)
              << (unit == AngularUnit::kMil ? "Wind MOA" : "Wind MIL")
              << std::setw(kWidth) << "Wind Inch" << std::setw(kWidth)
              << "Seconds" << "\033[0m\n";
  } else {
    std::cout << "\033[32m" << std::left << std::setw(kWidth) << "Yards"
              << std::setw(kWidth) << "FPS" << std::setw(kWidth) << "FtLbs"
              << std::setw(kWidth) << "Elev Inch" << std::setw(kWidth)
              << "Elev MOA" << std::setw(kWidth) << "Elev MIL"
              << std::setw(kWidth) << "Wind Inch" << std::setw(kWidth)
              << "Wind MOA" << std::setw(kWidth) << "Wind MIL"
              << std::setw(kWidth) << "Seconds" << "\033[0m\n";
  }

  auto zero_flush = [](double v) { return std::fabs(v) < 1e-4 ? 0.0 : v; };

  for (size_t i = 0; i < count; ++i) {
    double elev_moa = 0.0;
    double elev_mil = 0.0;
    double elev_inch = 0.0;
    double wind_moa = 0.0;
    double wind_mil = 0.0;
    double wind_inch = 0.0;

    if (mode == SolverMode::kInverse) {
      elev_moa = zero_flush(outputs[i].elevation);
      elev_mil = zero_flush(lob::MoaToMil(elev_moa));
      elev_inch = (outputs[i].range == 0)
                      ? 0.0
                      : zero_flush(lob::MoaToInch(elev_moa, outputs[i].range));
      wind_moa = zero_flush(outputs[i].deflection);
      wind_mil = zero_flush(lob::MoaToMil(wind_moa));
      wind_inch = (outputs[i].range == 0)
                      ? 0.0
                      : zero_flush(lob::MoaToInch(wind_moa, outputs[i].range));

      std::cout << std::left << std::setw(kWidth)
                << outputs[i].range / static_cast<uint32_t>(kYardsToFeet)
                << std::setw(kWidth) << outputs[i].velocity << std::setw(kWidth)
                << outputs[i].energy << std::setw(kWidth) << std::fixed
                << std::setprecision(2)
                << (unit == AngularUnit::kMil ? elev_mil : elev_moa)
                << std::setw(kWidth)
                << (unit == AngularUnit::kMil ? elev_moa : elev_mil)
                << std::setw(kWidth) << elev_inch << std::setw(kWidth)
                << (unit == AngularUnit::kMil ? wind_mil : wind_moa)
                << std::setw(kWidth)
                << (unit == AngularUnit::kMil ? wind_moa : wind_mil)
                << std::setw(kWidth) << wind_inch << std::setw(kWidth)
                << std::setprecision(3) << outputs[i].time_of_flight << "\n";
    } else {
      elev_inch = zero_flush(outputs[i].elevation);
      elev_moa = zero_flush(lob::InchToMoa(elev_inch, outputs[i].range));
      elev_mil = zero_flush(lob::InchToMil(elev_inch, outputs[i].range));
      wind_inch = zero_flush(outputs[i].deflection);
      wind_moa = zero_flush(lob::InchToMoa(wind_inch, outputs[i].range));
      wind_mil = zero_flush(lob::InchToMil(wind_inch, outputs[i].range));

      std::cout << std::left << std::setw(kWidth)
                << outputs[i].range / static_cast<uint32_t>(kYardsToFeet)
                << std::setw(kWidth) << outputs[i].velocity << std::setw(kWidth)
                << outputs[i].energy << std::setw(kWidth) << std::fixed
                << std::setprecision(2) << elev_inch << std::setw(kWidth)
                << elev_moa << std::setw(kWidth) << elev_mil
                << std::setw(kWidth) << wind_inch << std::setw(kWidth)
                << wind_moa << std::setw(kWidth) << wind_mil
                << std::setw(kWidth) << std::setprecision(3)
                << outputs[i].time_of_flight << "\n";
    }
  }
  std::cout << "\n";
}

nlohmann::json OutputsToJson(const lob::Output* outputs, size_t count,
                             SolverMode mode, AngularUnit unit) {
  (void)unit;
  auto zero_flush = [](double v) { return std::fabs(v) < 1e-4 ? 0.0 : v; };
  nlohmann::json j = nlohmann::json::array();
  for (size_t i = 0; i < count; ++i) {
    double elev_moa = 0.0;
    double elev_mil = 0.0;
    double elev_inch = 0.0;
    double wind_moa = 0.0;
    double wind_mil = 0.0;
    double wind_inch = 0.0;

    if (mode == SolverMode::kInverse) {
      elev_moa = zero_flush(outputs[i].elevation);
      elev_mil = zero_flush(lob::MoaToMil(elev_moa));
      elev_inch = (outputs[i].range == 0)
                      ? 0.0
                      : zero_flush(lob::MoaToInch(elev_moa, outputs[i].range));
      wind_moa = zero_flush(outputs[i].deflection);
      wind_mil = zero_flush(lob::MoaToMil(wind_moa));
      wind_inch = (outputs[i].range == 0)
                      ? 0.0
                      : zero_flush(lob::MoaToInch(wind_moa, outputs[i].range));
    } else {
      elev_inch = zero_flush(outputs[i].elevation);
      elev_moa = zero_flush(lob::InchToMoa(elev_inch, outputs[i].range));
      elev_mil = zero_flush(lob::InchToMil(elev_inch, outputs[i].range));
      wind_inch = zero_flush(outputs[i].deflection);
      wind_moa = zero_flush(lob::InchToMoa(wind_inch, outputs[i].range));
      wind_mil = zero_flush(lob::InchToMil(wind_inch, outputs[i].range));
    }

    j.push_back({
        {"range", outputs[i].range},
        {"velocity", outputs[i].velocity},
        {"energy", outputs[i].energy},
        {"elevation", outputs[i].elevation},
        {"deflection", outputs[i].deflection},
        {"time_of_flight", outputs[i].time_of_flight},
        {"elevation_moa", elev_moa},
        {"elevation_mil", elev_mil},
        {"elevation_inch", elev_inch},
        {"deflection_moa", wind_moa},
        {"deflection_mil", wind_mil},
        {"deflection_inch", wind_inch},
    });
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
