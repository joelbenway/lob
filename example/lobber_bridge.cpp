#include "lobber_bridge.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "lob/lob.hpp"

namespace example {
namespace {

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
  return j[key].get<uint16_t>();
}

lob::AtmosphereReferenceT JsonToAtmosphere(const nlohmann::json& j,
                                            const std::string& key) {
  double v = JsonToDouble(j, key);
  if (std::isnan(v)) {
    return lob::AtmosphereReferenceT::kArmyStandardMetro;
  }
  return static_cast<int>(std::round(v)) == 2
             ? lob::AtmosphereReferenceT::kIcao
             : lob::AtmosphereReferenceT::kArmyStandardMetro;
}

lob::DragFunctionT JsonToDragFunction(const nlohmann::json& j,
                                       const std::string& key) {
  double v = JsonToDouble(j, key);
  if (std::isnan(v)) {
    return lob::DragFunctionT::kG1;
  }
  switch (static_cast<int>(std::round(v))) {
    case 2: return lob::DragFunctionT::kG2;
    case 5: return lob::DragFunctionT::kG5;
    case 6: return lob::DragFunctionT::kG6;
    case 7: return lob::DragFunctionT::kG7;
    case 8: return lob::DragFunctionT::kG8;
    default: return lob::DragFunctionT::kG1;
  }
}

lob::ClockAngleT JsonToClockAngle(const nlohmann::json& j,
                                   const std::string& key) {
  double v = JsonToDouble(j, key);
  if (std::isnan(v)) {
    return lob::ClockAngleT::kXII;
  }
  switch (static_cast<int>(std::round(v))) {
    case 1: return lob::ClockAngleT::kI;
    case 2: return lob::ClockAngleT::kII;
    case 3: return lob::ClockAngleT::kIII;
    case 4: return lob::ClockAngleT::kIV;
    case 5: return lob::ClockAngleT::kV;
    case 6: return lob::ClockAngleT::kVI;
    case 7: return lob::ClockAngleT::kVII;
    case 8: return lob::ClockAngleT::kVIII;
    case 9: return lob::ClockAngleT::kIX;
    case 10: return lob::ClockAngleT::kX;
    case 11: return lob::ClockAngleT::kXI;
    default: return lob::ClockAngleT::kXII;
  }
}

}  // namespace

BridgeResult SolveFromJson(const nlohmann::json& j) {
  auto input = lob::Builder()
    .BallisticCoefficientPsi(JsonToDouble(j, "BallisticCoefficientPsi"))
    .BCAtmosphere(JsonToAtmosphere(j, "BCAtmosphere"))
    .BCDragFunction(JsonToDragFunction(j, "BCDragFunction"))
    .DiameterInch(JsonToDouble(j, "DiameterInch"))
    .MeplatDiameterInch(JsonToDouble(j, "MeplatDiameterInch"))
    .BaseDiameterInch(JsonToDouble(j, "BaseDiameterInch"))
    .LengthInch(JsonToDouble(j, "LengthInch"))
    .NoseLengthInch(JsonToDouble(j, "NoseLengthInch"))
    .TailLengthInch(JsonToDouble(j, "TailLengthInch"))
    .OgiveRtR(JsonToDouble(j, "OgiveRtR"))
    .MassGrains(JsonToDouble(j, "MassGrains"))
    .InitialVelocityFps(JsonToU16(j, "InitialVelocityFps"))
    .OpticHeightInches(JsonToDouble(j, "OpticHeightInches"))
    .TwistInchesPerTurn(JsonToDouble(j, "TwistInchesPerTurn"))
    .ZeroAngleMOA(JsonToDouble(j, "ZeroAngleMOA"))
    .ZeroDistanceYds(JsonToDouble(j, "ZeroDistanceYds"))
    .ZeroImpactHeightInches(JsonToDouble(j, "ZeroImpactHeightInches"))
    .AltitudeOfFiringSiteFt(JsonToDouble(j, "AltitudeOfFiringSiteFt"))
    .AirPressureInHg(JsonToDouble(j, "AirPressureInHg"))
    .AltitudeOfBarometerFt(JsonToDouble(j, "AltitudeOfBarometerFt"))
    .TemperatureDegF(JsonToDouble(j, "TemperatureDegF"))
    .AltitudeOfThermometerFt(JsonToDouble(j, "AltitudeOfThermometerFt"))
    .RelativeHumidityPercent(JsonToDouble(j, "RelativeHumidityPercent"))
    .WindHeading(JsonToClockAngle(j, "WindHeading"))
    .WindSpeedMph(JsonToDouble(j, "WindSpeedMph"))
    .AzimuthDeg(JsonToDouble(j, "AzimuthDeg"))
    .LatitudeDeg(JsonToDouble(j, "LatitudeDeg"))
    .RangeAngleDeg(JsonToDouble(j, "RangeAngleDeg"))
    .MinimumSpeed(JsonToU16(j, "MinimumSpeed"))
    .MinimumEnergy(JsonToU16(j, "MinimumEnergy"))
    .MaximumTime(JsonToDouble(j, "MaximumTime"))
    .StepSize(100)
    .Build();

  std::vector<uint32_t> ranges;
  if (j.contains("Ranges") && j["Ranges"].is_array()) {
    for (const auto& r : j["Ranges"]) {
      ranges.push_back(static_cast<uint32_t>(r.get<double>() * 3.0));
    }
  } else {
    static const uint32_t kDefaultRanges[] = {0, 50, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000};
    ranges.assign(kDefaultRanges, kDefaultRanges + 12);
  }

  std::vector<lob::Output> outputs(ranges.size());
  size_t count = lob::Solve(input, ranges.data(), outputs.data(), ranges.size());

  return {input, std::move(outputs), count};
}

void PrintTable(const lob::Input& input, const lob::Output* outputs, size_t count) {
  const uint8_t kExtra = 3;

  auto print_row = [](const std::string& label, auto value, int width) {
    std::cout << std::left << std::setw(width) << value;
  };

  // Extra info
  const std::string kZA("Zero Angle");
  const std::string kSF("Stability Factor");
  const std::string kSS("Speed of Sound");
  const std::string kE("Error");
  int zAw = static_cast<int>(kZA.size() + kExtra);
  int sFw = static_cast<int>(kSF.size() + kExtra);
  int sSw = static_cast<int>(kSS.size() + kExtra);
  int eEw = static_cast<int>(kE.size() + kExtra);

  std::cout << "\033[33m" << std::left << std::setw(zAw) << kZA
            << std::setw(sFw) << kSF << std::setw(sSw) << kSS
            << std::setw(eEw) << kE << "\033[0m\n";
  std::cout << std::left << std::setw(zAw) << std::fixed
            << std::setprecision(2) << input.zero_angle << std::setw(sFw)
            << input.stability_factor << std::setw(sSw)
            << input.speed_of_sound << std::setw(eEw) << std::hex
            << std::showbase << static_cast<unsigned int>(input.error)
            << std::dec << std::noshowbase << "\n\n";

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
    std::cout << std::left << std::setw(kWidth) << outputs[i].range / 3
              << std::setw(kWidth) << outputs[i].velocity
              << std::setw(kWidth) << outputs[i].energy
              << std::setw(kWidth) << std::fixed << std::setprecision(2)
              << outputs[i].elevation << std::setw(kWidth)
              << lob::InchToMoa(outputs[i].elevation, outputs[i].range)
              << std::setw(kWidth)
              << lob::InchToMil(outputs[i].elevation, outputs[i].range)
              << std::setw(kWidth) << outputs[i].deflection
              << std::setw(kWidth)
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
    j.push_back({
      {"range", outputs[i].range},
      {"velocity", outputs[i].velocity},
      {"energy", outputs[i].energy},
      {"elevation", outputs[i].elevation},
      {"deflection", outputs[i].deflection},
      {"time_of_flight", outputs[i].time_of_flight}
    });
  }
  return j;
}

}  // namespace example
