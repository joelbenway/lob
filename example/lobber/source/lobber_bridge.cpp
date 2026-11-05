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
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "lob/lob.hpp"
#include "lobber_cli.hpp"

namespace example {

namespace {

constexpr double kYardsToFeet = 3.0;
constexpr double kZeroThreshold = 1e-4;
constexpr int kTableColumnWidth = 12;

double ZeroFlush(double val) {
  return (std::fabs(val) < kZeroThreshold) ? 0.0 : val;
}

std::string Trim(const std::string& s) {
  auto start = s.begin();
  while (start != s.end() &&
         std::isspace(static_cast<unsigned char>(*start)) != 0) {
    ++start;
  }
  auto end = s.end();
  while (end != start &&
         std::isspace(static_cast<unsigned char>(*(end - 1))) != 0) {
    --end;
  }
  return {start, end};
}

std::string ToLower(std::string str) {
  std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return str;
}

std::vector<std::string> SplitLines(const std::string& text) {
  std::vector<std::string> lines;
  std::istringstream stream(text);
  std::string line;
  while (std::getline(stream, line)) {
    const std::string kTrimmed = Trim(line);
    if (!kTrimmed.empty()) {
      lines.push_back(kTrimmed);
    }
  }
  return lines;
}

std::vector<std::string> SplitTokens(const std::string& line) {
  std::vector<std::string> tokens;
  std::string cur;
  for (const char kCh : line) {
    if (kCh == ',' || kCh == '\t' || kCh == ' ') {
      if (!cur.empty()) {
        tokens.push_back(cur);
        cur.clear();
      }
    } else {
      cur += kCh;
    }
  }
  if (!cur.empty()) {
    tokens.push_back(cur);
  }
  return tokens;
}

bool TryParseBandObject(const nlohmann::json& item, BandPoint* out) {
  if (item.is_array() && item.size() >= 2) {
    *out = {item.at(0).get<float>(), item.at(1).get<float>()};
    return true;
  }
  if (item.is_object()) {
    float fps = 0.0F;
    if (item.contains("fps")) {
      fps = item["fps"].get<float>();
    } else if (item.contains("velocity")) {
      fps = item["velocity"].get<float>();
    }
    float bc = 0.0F;
    if (item.contains("bc")) {
      bc = item["bc"].get<float>();
    }
    if (fps > 0.0F && bc > 0.0F) {
      *out = {fps, bc};
      return true;
    }
  }
  return false;
}

bool TryParseDragObject(const nlohmann::json& item, DragTablePoint* out) {
  if (item.is_array() && item.size() >= 2) {
    *out = {item.at(0).get<float>(), item.at(1).get<float>()};
    return true;
  }
  if (item.is_object()) {
    float mach = 0.0F;
    if (item.contains("mach")) {
      mach = item["mach"].get<float>();
    }
    float cd = 0.0F;
    if (item.contains("cd")) {
      cd = item["cd"].get<float>();
    } else if (item.contains("drag")) {
      cd = item["drag"].get<float>();
    }
    *out = {mach, cd};
    return true;
  }
  return false;
}

bool TryParseWindObject(const nlohmann::json& item, lob::WindPoint* out) {
  if (item.is_array() && item.size() >= 3) {
    const double kRange = item.at(0).get<double>();
    const double kHeading = item.at(1).get<double>();
    const double kSpeed = item.at(2).get<double>();
    const double kHeight = (item.size() >= 4) ? item.at(3).get<double>() : 0.0;
    *out = {kRange, kHeading, kSpeed, kHeight};
    return true;
  }
  if (item.is_object()) {
    double range = 0.0;
    if (item.contains("range_ft")) {
      range = item["range_ft"].get<double>();
    } else if (item.contains("range")) {
      range = item["range"].get<double>();
    }
    double heading = 0.0;
    if (item.contains("heading_deg")) {
      heading = item["heading_deg"].get<double>();
    } else if (item.contains("heading")) {
      heading = item["heading"].get<double>();
    }
    double speed = 0.0;
    if (item.contains("speed_mph")) {
      speed = item["speed_mph"].get<double>();
    } else if (item.contains("speed")) {
      speed = item["speed"].get<double>();
    }
    double height = 0.0;
    if (item.contains("height_ft")) {
      height = item["height_ft"].get<double>();
    } else if (item.contains("height")) {
      height = item["height"].get<double>();
    }
    *out = {range, heading, speed, height};
    return true;
  }
  return false;
}

struct DragBuffers {
  std::vector<float> band_fps;
  std::vector<float> band_bcs;
  std::vector<float> table_mach;
  std::vector<float> table_cd;
  std::vector<float> spline_coeffs;
};

void ApplyCoefficients(lob::Builder* builder, std::vector<float> coefs,
                       DragBuffers* buffers) {
  if (coefs.size() == lob::kLobCoeffsSize) {
    buffers->spline_coeffs = std::move(coefs);
    builder->SplineCoefficients(buffers->spline_coeffs.data());
  }
}

void ApplyDragTable(lob::Builder* builder,
                    const std::vector<DragTablePoint>& table,
                    DragBuffers* buffers) {
  constexpr size_t kMinPoints = 2;
  if (table.size() >= kMinPoints) {
    buffers->table_mach.resize(table.size());
    buffers->table_cd.resize(table.size());
    for (size_t i = 0; i < table.size(); ++i) {
      buffers->table_mach.at(i) = table.at(i).mach;
      buffers->table_cd.at(i) = table.at(i).cd;
    }
    builder->MachVsDragTable(buffers->table_mach.data(),
                             buffers->table_cd.data(), table.size());
  }
}

void ApplyVelocityBands(lob::Builder* builder, const nlohmann::json& j,
                        const std::vector<BandPoint>& bands,
                        DragBuffers* buffers) {
  constexpr size_t kMinPoints = 2;
  if (bands.size() >= kMinPoints) {
    buffers->band_fps.resize(bands.size());
    buffers->band_bcs.resize(bands.size());
    for (size_t i = 0; i < bands.size(); ++i) {
      buffers->band_fps.at(i) = bands.at(i).fps;
      buffers->band_bcs.at(i) = bands.at(i).bc;
    }
    builder->BCVelocityBands(buffers->band_fps.data(), buffers->band_bcs.data(),
                             bands.size());
    builder->BCDragFunction(JsonToDragFunction(j, "BCDragFunction"));
    builder->BCAtmosphere(JsonToAtmosphere(j, "BCAtmosphere"));
  }
}

void ApplySingleBc(lob::Builder* builder, const nlohmann::json& j) {
  double bc = JsonToDouble(j, "BallisticCoefficientPsi");
  if (std::isnan(bc)) {
    bc = JsonToDouble(j, "bc");
  }
  if (!std::isnan(bc) && bc > 0.0) {
    builder->BallisticCoefficientPsi(bc);
    builder->BCDragFunction(JsonToDragFunction(j, "BCDragFunction"));
    builder->BCAtmosphere(JsonToAtmosphere(j, "BCAtmosphere"));
  }
}

std::string ExtractDragSource(const nlohmann::json& j) {
  if (j.contains("DragSource") && j["DragSource"].is_string()) {
    return j["DragSource"].get<std::string>();
  }
  if (j.contains("dragSource") && j["dragSource"].is_string()) {
    return j["dragSource"].get<std::string>();
  }
  return {};
}

void ApplyDrag(lob::Builder* builder, const nlohmann::json& j,
               DragBuffers* buffers) {
  const std::string kDragSource = ExtractDragSource(j);
  auto bands = ParseBands(j);
  auto table = ParseDragTable(j);
  auto coefs = ParseSplineCoefficients(j);
  const double kExplicitBc = JsonToDouble(j, "BallisticCoefficientPsi");

  if (kDragSource == "coefficients") {
    ApplyCoefficients(builder, std::move(coefs), buffers);
    return;
  }
  if (kDragSource == "table") {
    ApplyDragTable(builder, table, buffers);
    return;
  }
  if (kDragSource == "bands") {
    ApplyVelocityBands(builder, j, bands, buffers);
    return;
  }

  if (std::isnan(kExplicitBc)) {
    if (!coefs.empty() && bands.empty() && table.empty()) {
      ApplyCoefficients(builder, std::move(coefs), buffers);
      return;
    }
    if (!table.empty() && bands.empty()) {
      ApplyDragTable(builder, table, buffers);
      return;
    }
    if (!bands.empty()) {
      ApplyVelocityBands(builder, j, bands, buffers);
      return;
    }
  }

  ApplySingleBc(builder, j);
}

void ApplyProjectile(lob::Builder* builder, const nlohmann::json& j) {
  uint16_t mv = JsonToU16(j, "InitialVelocityFps");
  if (mv == 0) {
    mv = JsonToU16(j, "velocityFps");
  }
  if (mv > 0) {
    builder->InitialVelocityFps(mv);
  }

  double mass = JsonToDouble(j, "MassGrains");
  if (std::isnan(mass)) {
    mass = JsonToDouble(j, "massGrains");
  }
  if (!std::isnan(mass) && mass > 0.0) {
    builder->MassGrains(mass);
  }

  const double kDia = std::isnan(JsonToDouble(j, "DiameterInch"))
                          ? JsonToDouble(j, "diameterInch")
                          : JsonToDouble(j, "DiameterInch");
  const double kLen = std::isnan(JsonToDouble(j, "LengthInch"))
                          ? JsonToDouble(j, "lengthInch")
                          : JsonToDouble(j, "LengthInch");
  const double kTwist = std::isnan(JsonToDouble(j, "TwistInchesPerTurn"))
                            ? JsonToDouble(j, "twistInchesPerTurn")
                            : JsonToDouble(j, "TwistInchesPerTurn");
  if (!std::isnan(kDia) && kDia > 0.0 && !std::isnan(kLen) && kLen > 0.0 &&
      !std::isnan(kTwist) && kTwist > 0.0) {
    builder->DiameterInch(kDia).LengthInch(kLen).TwistInchesPerTurn(kTwist);
  }

  const double kNose = std::isnan(JsonToDouble(j, "NoseLengthInch"))
                           ? JsonToDouble(j, "noseLengthInch")
                           : JsonToDouble(j, "NoseLengthInch");
  const double kBase = std::isnan(JsonToDouble(j, "BaseDiameterInch"))
                           ? JsonToDouble(j, "baseDiameterInch")
                           : JsonToDouble(j, "BaseDiameterInch");
  const double kOgive = std::isnan(JsonToDouble(j, "OgiveRtR"))
                            ? JsonToDouble(j, "ogiveRtR")
                            : JsonToDouble(j, "OgiveRtR");
  if (!std::isnan(kNose) && kNose > 0.0 && !std::isnan(kBase) && kBase > 0.0 &&
      !std::isnan(kOgive) && kOgive > 0.0) {
    builder->NoseLengthInch(kNose).BaseDiameterInch(kBase).OgiveRtR(kOgive);
    const double kMeplat = std::isnan(JsonToDouble(j, "MeplatDiameterInch"))
                               ? JsonToDouble(j, "meplatDiameterInch")
                               : JsonToDouble(j, "MeplatDiameterInch");
    if (!std::isnan(kMeplat) && kMeplat >= 0.0) {
      builder->MeplatDiameterInch(kMeplat);
    }
    const double kTail = std::isnan(JsonToDouble(j, "TailLengthInch"))
                             ? JsonToDouble(j, "tailLengthInch")
                             : JsonToDouble(j, "TailLengthInch");
    if (!std::isnan(kTail) && kTail >= 0.0) {
      builder->TailLengthInch(kTail);
    }
  }
}

void ApplyZero(lob::Builder* builder, const nlohmann::json& j) {
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
    builder->ZeroAngleMOA(zero_angle);
  } else if (!std::isnan(zero_dist) && zero_dist > 0.0) {
    builder->ZeroDistanceYds(zero_dist);
    if (!std::isnan(zero_impact)) {
      builder->ZeroImpactHeightInches(zero_impact);
    }
  }

  double optic_ht = JsonToDouble(j, "OpticHeightInches");
  if (std::isnan(optic_ht)) {
    optic_ht = JsonToDouble(j, "sightHeightInches");
  }
  if (!std::isnan(optic_ht) && optic_ht >= 0.0) {
    builder->OpticHeightInches(optic_ht);
  }
}

void ApplyAtmosphere(lob::Builder* builder, const nlohmann::json& j) {
  double alt = JsonToDouble(j, "AltitudeOfFiringSiteFt");
  if (std::isnan(alt)) {
    alt = JsonToDouble(j, "altitudeOfFiringSiteFt");
  }
  if (!std::isnan(alt)) {
    builder->AltitudeOfFiringSiteFt(alt);
  }

  double pressure = JsonToDouble(j, "AirPressureInHg");
  if (std::isnan(pressure)) {
    pressure = JsonToDouble(j, "airPressureInHg");
  }
  if (!std::isnan(pressure) && pressure > 0.0) {
    builder->AirPressureInHg(pressure);
  }

  double baro_alt = JsonToDouble(j, "AltitudeOfBarometerFt");
  if (std::isnan(baro_alt)) {
    baro_alt = JsonToDouble(j, "altitudeOfBarometerFt");
  }
  if (!std::isnan(baro_alt)) {
    builder->AltitudeOfBarometerFt(baro_alt);
  }

  double temp = JsonToDouble(j, "TemperatureDegF");
  if (std::isnan(temp)) {
    temp = JsonToDouble(j, "temperatureDegF");
  }
  if (!std::isnan(temp)) {
    builder->TemperatureDegF(temp);
  }

  double thermo_alt = JsonToDouble(j, "AltitudeOfThermometerFt");
  if (std::isnan(thermo_alt)) {
    thermo_alt = JsonToDouble(j, "altitudeOfThermometerFt");
  }
  if (!std::isnan(thermo_alt)) {
    builder->AltitudeOfThermometerFt(thermo_alt);
  }

  double rh = JsonToDouble(j, "RelativeHumidityPercent");
  if (std::isnan(rh)) {
    rh = JsonToDouble(j, "relativeHumidityPercent");
  }
  if (!std::isnan(rh) && rh >= 0.0) {
    builder->RelativeHumidityPercent(rh);
  }
}

void ApplyWind(lob::Builder* builder, const nlohmann::json& j,
               std::vector<lob::WindPoint>* wind_buffer) {
  *wind_buffer = ParseWindProfile(j);
  if (!wind_buffer->empty()) {
    builder->WindProfile(wind_buffer->data(), wind_buffer->size());
    double shear = JsonToDouble(j, "WindShearExponent");
    if (std::isnan(shear)) {
      shear = JsonToDouble(j, "windShearExponent");
    }
    if (!std::isnan(shear) && shear >= 0.0) {
      builder->WindShearExponent(shear);
    }
  } else {
    double wspd = JsonToDouble(j, "WindSpeedMph");
    if (std::isnan(wspd)) {
      wspd = JsonToDouble(j, "windSpeedMph");
    }
    if (!std::isnan(wspd) && wspd > 0.0) {
      builder->WindSpeedMph(wspd);
      if (j.contains("WindHeading")) {
        builder->WindHeading(JsonToClockAngle(j, "WindHeading"));
      } else if (j.contains("windClock")) {
        builder->WindHeading(JsonToClockAngle(j, "windClock"));
      }
    }
  }
}

void ApplyGeometry(lob::Builder* builder, const nlohmann::json& j) {
  double range_angle = JsonToDouble(j, "RangeAngleDeg");
  if (std::isnan(range_angle)) {
    range_angle = JsonToDouble(j, "rangeAngleDeg");
  }
  if (!std::isnan(range_angle)) {
    builder->RangeAngleDeg(range_angle);
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
    builder->LatitudeDeg(lat).AzimuthDeg(az);
  }
}

void ApplyControls(lob::Builder* builder, const nlohmann::json& j) {
  uint16_t step_size = JsonToU16(j, "StepSize");
  if (step_size == 0) {
    step_size = JsonToU16(j, "stepSizeInch");
  }
  if (step_size > 0) {
    builder->StepSize(step_size);
  }

  uint16_t min_spd = JsonToU16(j, "MinimumSpeed");
  if (min_spd == 0) {
    min_spd = JsonToU16(j, "minimumSpeedFps");
  }
  if (min_spd > 0) {
    builder->MinimumSpeed(min_spd);
  }

  uint16_t min_en = JsonToU16(j, "MinimumEnergy");
  if (min_en == 0) {
    min_en = JsonToU16(j, "minimumEnergyFtLbs");
  }
  if (min_en > 0) {
    builder->MinimumEnergy(min_en);
  }

  double max_t = JsonToDouble(j, "MaximumTime");
  if (std::isnan(max_t)) {
    max_t = JsonToDouble(j, "maximumTimeSec");
  }
  if (!std::isnan(max_t) && max_t > 0.0) {
    builder->MaximumTime(max_t);
  }
}

void PrintInverseRow(const lob::Output& out, AngularUnit unit, int width) {
  const double kElevMoa = ZeroFlush(out.elevation);
  const double kElevMil = ZeroFlush(lob::MoaToMil(kElevMoa));
  const double kElevInch =
      (out.range == 0) ? 0.0 : ZeroFlush(lob::MoaToInch(kElevMoa, out.range));
  const double kWindMoa = ZeroFlush(out.deflection);
  const double kWindMil = ZeroFlush(lob::MoaToMil(kWindMoa));
  const double kWindInch =
      (out.range == 0) ? 0.0 : ZeroFlush(lob::MoaToInch(kWindMoa, out.range));

  std::cout << std::left << std::setw(width)
            << (out.range / static_cast<uint32_t>(kYardsToFeet))
            << std::setw(width) << out.velocity << std::setw(width)
            << out.energy << std::setw(width) << std::fixed
            << std::setprecision(2)
            << (unit == AngularUnit::kMil ? kElevMil : kElevMoa)
            << std::setw(width)
            << (unit == AngularUnit::kMil ? kElevMoa : kElevMil)
            << std::setw(width) << kElevInch << std::setw(width)
            << (unit == AngularUnit::kMil ? kWindMil : kWindMoa)
            << std::setw(width)
            << (unit == AngularUnit::kMil ? kWindMoa : kWindMil)
            << std::setw(width) << kWindInch << std::setw(width)
            << std::setprecision(3) << out.time_of_flight << "\n";
}

void PrintForwardRow(const lob::Output& out, int width) {
  const double kElevInch = ZeroFlush(out.elevation);
  const double kElevMoa = ZeroFlush(lob::InchToMoa(kElevInch, out.range));
  const double kElevMil = ZeroFlush(lob::InchToMil(kElevInch, out.range));
  const double kWindInch = ZeroFlush(out.deflection);
  const double kWindMoa = ZeroFlush(lob::InchToMoa(kWindInch, out.range));
  const double kWindMil = ZeroFlush(lob::InchToMil(kWindInch, out.range));

  std::cout << std::left << std::setw(width)
            << (out.range / static_cast<uint32_t>(kYardsToFeet))
            << std::setw(width) << out.velocity << std::setw(width)
            << out.energy << std::setw(width) << std::fixed
            << std::setprecision(2) << kElevInch << std::setw(width) << kElevMoa
            << std::setw(width) << kElevMil << std::setw(width) << kWindInch
            << std::setw(width) << kWindMoa << std::setw(width) << kWindMil
            << std::setw(width) << std::setprecision(3) << out.time_of_flight
            << "\n";
}

nlohmann::json MakeRowJson(const lob::Output& out, SolverMode mode) {
  double elev_moa = 0.0;
  double elev_mil = 0.0;
  double elev_inch = 0.0;
  double wind_moa = 0.0;
  double wind_mil = 0.0;
  double wind_inch = 0.0;

  if (mode == SolverMode::kInverse) {
    elev_moa = ZeroFlush(out.elevation);
    elev_mil = ZeroFlush(lob::MoaToMil(elev_moa));
    elev_inch =
        (out.range == 0) ? 0.0 : ZeroFlush(lob::MoaToInch(elev_moa, out.range));
    wind_moa = ZeroFlush(out.deflection);
    wind_mil = ZeroFlush(lob::MoaToMil(wind_moa));
    wind_inch =
        (out.range == 0) ? 0.0 : ZeroFlush(lob::MoaToInch(wind_moa, out.range));
  } else {
    elev_inch = ZeroFlush(out.elevation);
    elev_moa = ZeroFlush(lob::InchToMoa(elev_inch, out.range));
    elev_mil = ZeroFlush(lob::InchToMil(elev_inch, out.range));
    wind_inch = ZeroFlush(out.deflection);
    wind_moa = ZeroFlush(lob::InchToMoa(wind_inch, out.range));
    wind_mil = ZeroFlush(lob::InchToMil(wind_inch, out.range));
  }

  return {
      {"range", out.range},           {"velocity", out.velocity},
      {"energy", out.energy},         {"elevation", out.elevation},
      {"deflection", out.deflection}, {"time_of_flight", out.time_of_flight},
      {"elevation_moa", elev_moa},    {"elevation_mil", elev_mil},
      {"elevation_inch", elev_inch},  {"deflection_moa", wind_moa},
      {"deflection_mil", wind_mil},   {"deflection_inch", wind_inch},
  };
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
  const auto kCode = static_cast<int>(std::round(kV));
  constexpr int kLegacyIcao = 2;
  if (kCode == static_cast<int>(lob::AtmosphereReferenceT::kIcao) ||
      kCode == kLegacyIcao) {
    return lob::AtmosphereReferenceT::kIcao;
  }
  return lob::AtmosphereReferenceT::kArmyStandardMetro;
}

lob::DragFunctionT JsonToDragFunction(const nlohmann::json& j,
                                      const std::string& key) {
  if (!j.contains(key) || j[key].is_null()) {
    return lob::DragFunctionT::kG1;
  }
  if (j[key].is_string()) {
    const std::string kVal = j[key].get<std::string>();
    if (kVal == "G1" || kVal == "g1" || kVal == "1") {
      return lob::DragFunctionT::kG1;
    }
    if (kVal == "G2" || kVal == "g2" || kVal == "2") {
      return lob::DragFunctionT::kG2;
    }
    if (kVal == "G5" || kVal == "g5" || kVal == "5") {
      return lob::DragFunctionT::kG5;
    }
    if (kVal == "G6" || kVal == "g6" || kVal == "6") {
      return lob::DragFunctionT::kG6;
    }
    if (kVal == "G7" || kVal == "g7" || kVal == "7") {
      return lob::DragFunctionT::kG7;
    }
    if (kVal == "G8" || kVal == "g8" || kVal == "8") {
      return lob::DragFunctionT::kG8;
    }
    return lob::DragFunctionT::kG1;
  }
  const double kV = JsonToDouble(j, key);
  if (std::isnan(kV)) {
    return lob::DragFunctionT::kG1;
  }
  const auto kChoice = static_cast<int>(kV);
  switch (kChoice) {
    case static_cast<int>(lob::DragFunctionT::kG1):
      return lob::DragFunctionT::kG1;
    case static_cast<int>(lob::DragFunctionT::kG2):
      return lob::DragFunctionT::kG2;
    case static_cast<int>(lob::DragFunctionT::kG5):
      return lob::DragFunctionT::kG5;
    case static_cast<int>(lob::DragFunctionT::kG6):
      return lob::DragFunctionT::kG6;
    case static_cast<int>(lob::DragFunctionT::kG7):
      return lob::DragFunctionT::kG7;
    case static_cast<int>(lob::DragFunctionT::kG8):
      return lob::DragFunctionT::kG8;
    default:
      return lob::DragFunctionT::kG1;
  }
}

lob::ClockAngleT JsonToClockAngle(const nlohmann::json& j,
                                  const std::string& key) {
  if (!j.contains(key) || j[key].is_null()) {
    return lob::ClockAngleT::kXII;
  }
  const double kV = JsonToDouble(j, key);
  if (std::isnan(kV)) {
    return lob::ClockAngleT::kXII;
  }
  const auto kChoice = static_cast<int>(kV);
  switch (kChoice) {
    case static_cast<int>(lob::ClockAngleT::kI):
      return lob::ClockAngleT::kI;
    case static_cast<int>(lob::ClockAngleT::kII):
      return lob::ClockAngleT::kII;
    case static_cast<int>(lob::ClockAngleT::kIII):
      return lob::ClockAngleT::kIII;
    case static_cast<int>(lob::ClockAngleT::kIV):
      return lob::ClockAngleT::kIV;
    case static_cast<int>(lob::ClockAngleT::kV):
      return lob::ClockAngleT::kV;
    case static_cast<int>(lob::ClockAngleT::kVI):
      return lob::ClockAngleT::kVI;
    case static_cast<int>(lob::ClockAngleT::kVII):
      return lob::ClockAngleT::kVII;
    case static_cast<int>(lob::ClockAngleT::kVIII):
      return lob::ClockAngleT::kVIII;
    case static_cast<int>(lob::ClockAngleT::kIX):
      return lob::ClockAngleT::kIX;
    case static_cast<int>(lob::ClockAngleT::kX):
      return lob::ClockAngleT::kX;
    case static_cast<int>(lob::ClockAngleT::kXI):
      return lob::ClockAngleT::kXI;
    case static_cast<int>(lob::ClockAngleT::kXII):
    default:
      return lob::ClockAngleT::kXII;
  }
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
    const std::string kVal = ToLower(j["SolverPath"].get<std::string>());
    if (kVal == "elr") {
      return SolverPath::kELR;
    }
    if (kVal == "fast") {
      return SolverPath::kFast;
    }
  }
  return SolverPath::kAuto;
}

SolverMode JsonToSolverMode(const nlohmann::json& j) {
  if (j.contains("SolverMode") && j["SolverMode"].is_string()) {
    const std::string kVal = ToLower(j["SolverMode"].get<std::string>());
    if (kVal == "inverse") {
      return SolverMode::kInverse;
    }
    if (kVal == "forward") {
      return SolverMode::kForward;
    }
  }
  return SolverMode::kAuto;
}

AngularUnit JsonToAngularUnit(const nlohmann::json& j) {
  if (j.contains("AngularUnit") && j["AngularUnit"].is_string()) {
    const std::string kVal = ToLower(j["AngularUnit"].get<std::string>());
    if (kVal == "mil") {
      return AngularUnit::kMil;
    }
    if (kVal == "moa") {
      return AngularUnit::kMoa;
    }
  }
  return AngularUnit::kMoa;
}

std::vector<BandPoint> ParseBandsFromString(const std::string& text) {
  std::vector<BandPoint> result;
  for (const auto& line : SplitLines(text)) {
    const auto kTokens = SplitTokens(line);
    if (kTokens.size() >= 2) {
      char* end1 = nullptr;
      char* end2 = nullptr;
      const float kFps = std::strtof(kTokens.at(0).c_str(), &end1);
      const float kBc = std::strtof(kTokens.at(1).c_str(), &end2);
      if (end1 != kTokens.at(0).c_str() && end2 != kTokens.at(1).c_str()) {
        result.push_back({kFps, kBc});
      }
    }
  }
  return result;
}

std::vector<BandPoint> ParseBands(const nlohmann::json& j) {
  constexpr std::array<const char*, 3> kKeys = {"BCVelocityBands", "bands",
                                                "bandsText"};
  for (const auto* key : kKeys) {
    if (!j.contains(key) || j[key].is_null()) {
      continue;
    }
    if (j[key].is_string()) {
      return ParseBandsFromString(j[key].get<std::string>());
    }
    if (j[key].is_object()) {
      if (j[key].contains("fps") && j[key].contains("bcs") &&
          j[key]["fps"].is_array() && j[key]["bcs"].is_array()) {
        std::vector<BandPoint> bands;
        const size_t kCount =
            std::min(j[key]["fps"].size(), j[key]["bcs"].size());
        bands.reserve(kCount);
        for (size_t i = 0; i < kCount; ++i) {
          bands.push_back({j[key]["fps"].at(i).get<float>(),
                           j[key]["bcs"].at(i).get<float>()});
        }
        return bands;
      }
    }
    if (j[key].is_array()) {
      std::vector<BandPoint> bands;
      for (const auto& item : j[key]) {
        BandPoint pt;
        if (TryParseBandObject(item, &pt)) {
          bands.push_back(pt);
        }
      }
      return bands;
    }
  }
  return {};
}

std::vector<DragTablePoint> ParseDragTableFromString(const std::string& text) {
  std::vector<DragTablePoint> result;
  for (const auto& line : SplitLines(text)) {
    const auto kTokens = SplitTokens(line);
    if (kTokens.size() >= 2) {
      char* end1 = nullptr;
      char* end2 = nullptr;
      const float kMach = std::strtof(kTokens.at(0).c_str(), &end1);
      const float kCd = std::strtof(kTokens.at(1).c_str(), &end2);
      if (end1 != kTokens.at(0).c_str() && end2 != kTokens.at(1).c_str()) {
        result.push_back({kMach, kCd});
      }
    }
  }
  return result;
}

std::vector<DragTablePoint> ParseDragTable(const nlohmann::json& j) {
  constexpr std::array<const char*, 5> kKeys = {"SplineFitTable", "DragTable",
                                                "MachVsDragTable",
                                                "dragTableText", "table"};
  for (const auto* key : kKeys) {
    if (!j.contains(key) || j[key].is_null()) {
      continue;
    }
    if (j[key].is_string()) {
      return ParseDragTableFromString(j[key].get<std::string>());
    }
    if (j[key].is_object()) {
      if (j[key].contains("machs") && j[key].contains("drags") &&
          j[key]["machs"].is_array() && j[key]["drags"].is_array()) {
        std::vector<DragTablePoint> table;
        const size_t kCount =
            std::min(j[key]["machs"].size(), j[key]["drags"].size());
        table.reserve(kCount);
        for (size_t i = 0; i < kCount; ++i) {
          table.push_back({j[key]["machs"].at(i).get<float>(),
                           j[key]["drags"].at(i).get<float>()});
        }
        return table;
      }
    }
    if (j[key].is_array()) {
      std::vector<DragTablePoint> table;
      for (const auto& item : j[key]) {
        DragTablePoint pt;
        if (TryParseDragObject(item, &pt)) {
          table.push_back(pt);
        }
      }
      return table;
    }
  }
  return {};
}

std::vector<float> ParseSplineCoefficientsFromString(const std::string& text) {
  std::vector<float> result;
  for (const auto& line : SplitLines(text)) {
    for (const auto& tok : SplitTokens(line)) {
      char* end = nullptr;
      const float kVal = std::strtof(tok.c_str(), &end);
      if (end != tok.c_str() && *end == '\0') {
        result.push_back(kVal);
      }
    }
  }
  return result;
}

std::vector<float> ParseSplineCoefficients(const nlohmann::json& j) {
  constexpr std::array<const char*, 5> kKeys = {
      "SplineCoefficients", "SplineCoeffs", "coefficients", "dragCoefficients",
      "coefficientsText"};
  for (const auto* key : kKeys) {
    if (!j.contains(key) || j[key].is_null()) {
      continue;
    }
    if (j[key].is_string()) {
      return ParseSplineCoefficientsFromString(j[key].get<std::string>());
    }
    if (j[key].is_array()) {
      std::vector<float> result;
      for (const auto& item : j[key]) {
        if (item.is_number()) {
          result.push_back(item.get<float>());
        }
      }
      return result;
    }
  }
  return {};
}

std::vector<lob::WindPoint> ParseWindProfileFromString(
    const std::string& text) {
  std::vector<lob::WindPoint> result;
  for (const auto& line : SplitLines(text)) {
    const auto kTokens = SplitTokens(line);
    if (kTokens.size() >= 3) {
      char* end1 = nullptr;
      char* end2 = nullptr;
      char* end3 = nullptr;
      const double kRange = std::strtod(kTokens.at(0).c_str(), &end1);
      const double kHeading = std::strtod(kTokens.at(1).c_str(), &end2);
      const double kSpeed = std::strtod(kTokens.at(2).c_str(), &end3);
      double height = 0.0;
      if (kTokens.size() >= 4) {
        char* end4 = nullptr;
        height = std::strtod(kTokens.at(3).c_str(), &end4);
      }
      if (end1 != kTokens.at(0).c_str() && end2 != kTokens.at(1).c_str() &&
          end3 != kTokens.at(2).c_str()) {
        result.push_back({kRange, kHeading, kSpeed, height});
      }
    }
  }
  return result;
}

std::vector<lob::WindPoint> ParseWindProfile(const nlohmann::json& j) {
  constexpr std::array<const char*, 3> kKeys = {"WindProfile", "windProfile",
                                                "windProfileText"};
  for (const auto* key : kKeys) {
    if (!j.contains(key) || j[key].is_null()) {
      continue;
    }
    if (j[key].is_string()) {
      return ParseWindProfileFromString(j[key].get<std::string>());
    }
    if (j[key].is_array()) {
      std::vector<lob::WindPoint> profile;
      for (const auto& item : j[key]) {
        lob::WindPoint pt;
        if (TryParseWindObject(item, &pt)) {
          profile.push_back(pt);
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
    const double kMaxYds = j.contains("MaxYds") ? j["MaxYds"].get<double>()
                                                : j["maxYds"].get<double>();
    constexpr double kDefaultStep = 100.0;
    double step_yds = kDefaultStep;
    if (j.contains("StepYds")) {
      step_yds = j["StepYds"].get<double>();
    } else if (j.contains("stepYds")) {
      step_yds = j["stepYds"].get<double>();
    }
    if (step_yds <= 0.0) {
      step_yds = kDefaultStep;
    }
    std::vector<uint32_t> ranges;
    const auto kSteps = static_cast<size_t>(std::ceil(kMaxYds / step_yds));
    for (size_t step_idx = 0; step_idx <= kSteps; ++step_idx) {
      const double kYards =
          std::min(static_cast<double>(step_idx) * step_yds, kMaxYds);
      ranges.push_back(static_cast<uint32_t>(kYards * kYardsToFeet));
    }
    if (ranges.empty() ||
        ranges.back() != static_cast<uint32_t>(kMaxYds * kYardsToFeet)) {
      ranges.push_back(static_cast<uint32_t>(kMaxYds * kYardsToFeet));
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
  DragBuffers buffers;
  std::vector<lob::WindPoint> wind_points;

  ApplyDrag(&builder, j, &buffers);
  ApplyProjectile(&builder, j);
  ApplyZero(&builder, j);
  ApplyAtmosphere(&builder, j);
  ApplyWind(&builder, j, &wind_points);
  ApplyGeometry(&builder, j);
  ApplyControls(&builder, j);

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

  if (mode == SolverMode::kInverse) {
    std::cout << "\033[32m" << std::left << std::setw(kTableColumnWidth)
              << "Yards" << std::setw(kTableColumnWidth) << "FPS"
              << std::setw(kTableColumnWidth) << "FtLbs"
              << std::setw(kTableColumnWidth)
              << (unit == AngularUnit::kMil ? "Hold MIL" : "Hold MOA")
              << std::setw(kTableColumnWidth)
              << (unit == AngularUnit::kMil ? "Hold MOA" : "Hold MIL")
              << std::setw(kTableColumnWidth) << "Elev Inch"
              << std::setw(kTableColumnWidth)
              << (unit == AngularUnit::kMil ? "Wind MIL" : "Wind MOA")
              << std::setw(kTableColumnWidth)
              << (unit == AngularUnit::kMil ? "Wind MOA" : "Wind MIL")
              << std::setw(kTableColumnWidth) << "Wind Inch"
              << std::setw(kTableColumnWidth) << "Seconds" << "\033[0m\n";
  } else {
    std::cout << "\033[32m" << std::left << std::setw(kTableColumnWidth)
              << "Yards" << std::setw(kTableColumnWidth) << "FPS"
              << std::setw(kTableColumnWidth) << "FtLbs"
              << std::setw(kTableColumnWidth) << "Elev Inch"
              << std::setw(kTableColumnWidth) << "Elev MOA"
              << std::setw(kTableColumnWidth) << "Elev MIL"
              << std::setw(kTableColumnWidth) << "Wind Inch"
              << std::setw(kTableColumnWidth) << "Wind MOA"
              << std::setw(kTableColumnWidth) << "Wind MIL"
              << std::setw(kTableColumnWidth) << "Seconds" << "\033[0m\n";
  }

  for (size_t i = 0; i < count; ++i) {
    if (mode == SolverMode::kInverse) {
      PrintInverseRow(outputs[i], unit, kTableColumnWidth);
    } else {
      PrintForwardRow(outputs[i], kTableColumnWidth);
    }
  }
  std::cout << "\n";
}

nlohmann::json OutputsToJson(const lob::Output* outputs, size_t count,
                             SolverMode mode, AngularUnit unit) {
  (void)unit;
  nlohmann::json j = nlohmann::json::array();
  for (size_t i = 0; i < count; ++i) {
    j.push_back(MakeRowJson(outputs[i], mode));
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
