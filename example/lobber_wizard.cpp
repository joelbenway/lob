// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include "lobber_wizard.hpp"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <map>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <string>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

namespace example {
namespace {

enum class Step : uint8_t {
  kBallisticCoefficientPsi,
  kBCAtmosphere,
  kBCDragFunction,
  kDiameterInch,
  kMeplatDiameterInch,
  kBaseDiameterInch,
  kLengthInch,
  kNoseLengthInch,
  kTailLengthInch,
  kOgiveRtR,
  kMassGrains,
  kInitialVelocityFps,
  kOpticHeightInches,
  kTwistInchesPerTurn,
  kZeroAngleMOA,
  kZeroDistanceYds,
  kZeroImpactHeightInches,
  kAltitudeOfFiringSiteFt,
  kAirPressureInHg,
  kAltitudeOfBarometerFt,
  kTemperatureDegF,
  kAltitudeOfThermometerFt,
  kRelativeHumidityPercent,
  kWindHeading,
  kWindSpeedMph,
  kAzimuthDeg,
  kLatitudeDeg,
  kRangeAngleDeg,
  kMinimumSpeed,
  kMinimumEnergy,
  kMaximumTime,
  kRanges
};

struct StepInfo {
  Step given;
  Step skip;
  bool always_given = false;
  bool always_skip = false;
  bool is_list = false;
};

const std::map<Step, std::string>& GetKeys() {
  static const std::map<Step, std::string> kKeys{
      {Step::kBallisticCoefficientPsi, "BallisticCoefficientPsi"},
      {Step::kBCAtmosphere, "BCAtmosphere"},
      {Step::kBCDragFunction, "BCDragFunction"},
      {Step::kDiameterInch, "DiameterInch"},
      {Step::kMeplatDiameterInch, "MeplatDiameterInch"},
      {Step::kBaseDiameterInch, "BaseDiameterInch"},
      {Step::kLengthInch, "LengthInch"},
      {Step::kNoseLengthInch, "NoseLengthInch"},
      {Step::kTailLengthInch, "TailLengthInch"},
      {Step::kOgiveRtR, "OgiveRtR"},
      {Step::kMassGrains, "MassGrains"},
      {Step::kInitialVelocityFps, "InitialVelocityFps"},
      {Step::kOpticHeightInches, "OpticHeightInches"},
      {Step::kTwistInchesPerTurn, "TwistInchesPerTurn"},
      {Step::kZeroAngleMOA, "ZeroAngleMOA"},
      {Step::kZeroDistanceYds, "ZeroDistanceYds"},
      {Step::kZeroImpactHeightInches, "ZeroImpactHeightInches"},
      {Step::kAltitudeOfFiringSiteFt, "AltitudeOfFiringSiteFt"},
      {Step::kAirPressureInHg, "AirPressureInHg"},
      {Step::kAltitudeOfBarometerFt, "AltitudeOfBarometerFt"},
      {Step::kTemperatureDegF, "TemperatureDegF"},
      {Step::kAltitudeOfThermometerFt, "AltitudeOfThermometerFt"},
      {Step::kRelativeHumidityPercent, "RelativeHumidityPercent"},
      {Step::kWindHeading, "WindHeading"},
      {Step::kWindSpeedMph, "WindSpeedMph"},
      {Step::kAzimuthDeg, "AzimuthDeg"},
      {Step::kLatitudeDeg, "LatitudeDeg"},
      {Step::kRangeAngleDeg, "RangeAngleDeg"},
      {Step::kMinimumSpeed, "MinimumSpeed"},
      {Step::kMinimumEnergy, "MinimumEnergy"},
      {Step::kMaximumTime, "MaximumTime"},
      {Step::kRanges, "Ranges"}};
  return kKeys;
}

const std::map<Step, std::string>& GetPrompts() {
  static const std::map<Step, std::string> kPrompts{
      {Step::kBallisticCoefficientPsi, "Enter ballistic coefficient in PSI"},
      {Step::kBCAtmosphere,
       "Enter 1 for Army Standard Metro or 2 for ICAO reference atmosphere"},
      {Step::kBCDragFunction,
       "Enter 1, 2, 5, 6, 7, or 8 for associated drag function"},
      {Step::kDiameterInch, "Enter bullet diameter in inches"},
      {Step::kMeplatDiameterInch, "Enter meplat diameter in inches"},
      {Step::kBaseDiameterInch, "Enter base diameter in inches"},
      {Step::kLengthInch, "Enter bullet length in inches"},
      {Step::kNoseLengthInch, "Enter nose length in inches"},
      {Step::kTailLengthInch, "Enter tail length in inches"},
      {Step::kOgiveRtR, "Enter ogive radius to length ratio (Rt/R)"},
      {Step::kMassGrains, "Enter bullet weight in grains"},
      {Step::kInitialVelocityFps, "Enter muzzle velocity in fps"},
      {Step::kOpticHeightInches, "Enter optic height in inches"},
      {Step::kTwistInchesPerTurn, "Enter twist rate in inches per turn"},
      {Step::kZeroAngleMOA, "Enter zero angle in MOA"},
      {Step::kZeroDistanceYds, "Enter zero range in yards"},
      {Step::kZeroImpactHeightInches, "Enter zero impact height in inches"},
      {Step::kAltitudeOfFiringSiteFt, "Enter altitude of firing site in feet"},
      {Step::kAirPressureInHg, "Enter air pressure in inches of mercury"},
      {Step::kAltitudeOfBarometerFt, "Enter altitude of barometer in feet"},
      {Step::kTemperatureDegF, "Enter temperature in degrees Fahrenheit"},
      {Step::kAltitudeOfThermometerFt, "Enter altitude of thermometer in feet"},
      {Step::kRelativeHumidityPercent, "Enter relative humidity in percent"},
      {Step::kWindHeading, "Enter wind heading as a clock angle (1 though 12)"},
      {Step::kWindSpeedMph, "Enter wind speed in miles per hour"},
      {Step::kAzimuthDeg, "Enter azimuth in degrees"},
      {Step::kLatitudeDeg, "Enter latitude in degrees"},
      {Step::kRangeAngleDeg, "Enter range angle in degrees"},
      {Step::kMinimumSpeed, "Enter minimum speed in feet per second"},
      {Step::kMinimumEnergy, "Enter minimum energy in foot pounds"},
      {Step::kMaximumTime, "Enter maximum time in seconds"},
      {Step::kRanges, "Enter a range in yards to solve for"}};
  return kPrompts;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
StepInfo GetTransition(Step s) {
  switch (s) {
    case Step::kBallisticCoefficientPsi:
      return {Step::kBCAtmosphere, Step::kBallisticCoefficientPsi};
    case Step::kBCAtmosphere:
      return {Step::kBCDragFunction, Step::kBCDragFunction, true};
    case Step::kBCDragFunction:
      return {Step::kDiameterInch, Step::kDiameterInch, true};
    case Step::kDiameterInch:
      return {Step::kLengthInch, Step::kMassGrains};
    case Step::kLengthInch:
      return {Step::kNoseLengthInch, Step::kMassGrains};
    case Step::kNoseLengthInch:
      return {Step::kMeplatDiameterInch, Step::kTwistInchesPerTurn};
    case Step::kMeplatDiameterInch:
      return {Step::kBaseDiameterInch, Step::kTwistInchesPerTurn};
    case Step::kBaseDiameterInch:
      return {Step::kTwistInchesPerTurn, Step::kTailLengthInch};
    case Step::kTailLengthInch:
      return {Step::kOgiveRtR, Step::kTwistInchesPerTurn};
    case Step::kOgiveRtR:
      return {Step::kTwistInchesPerTurn, Step::kTwistInchesPerTurn, true};
    case Step::kTwistInchesPerTurn:
      return {Step::kMassGrains, Step::kMassGrains, true};
    case Step::kMassGrains:
      return {Step::kInitialVelocityFps, Step::kInitialVelocityFps, true};
    case Step::kInitialVelocityFps:
      return {Step::kOpticHeightInches, Step::kInitialVelocityFps};
    case Step::kOpticHeightInches:
      return {Step::kZeroAngleMOA, Step::kZeroAngleMOA, true};
    case Step::kZeroAngleMOA:
      return {Step::kAltitudeOfFiringSiteFt, Step::kZeroDistanceYds};
    case Step::kZeroDistanceYds:
      return {Step::kZeroImpactHeightInches, Step::kZeroAngleMOA};
    case Step::kZeroImpactHeightInches:
      return {Step::kAltitudeOfFiringSiteFt, Step::kAltitudeOfFiringSiteFt,
              true};
    case Step::kAltitudeOfFiringSiteFt:
      return {Step::kAirPressureInHg, Step::kAirPressureInHg, true};
    case Step::kAirPressureInHg:
      return {Step::kAltitudeOfBarometerFt, Step::kTemperatureDegF};
    case Step::kAltitudeOfBarometerFt:
      return {Step::kTemperatureDegF, Step::kTemperatureDegF, true};
    case Step::kTemperatureDegF:
      return {Step::kAltitudeOfThermometerFt, Step::kRelativeHumidityPercent};
    case Step::kAltitudeOfThermometerFt:
      return {Step::kRelativeHumidityPercent, Step::kRelativeHumidityPercent,
              true};
    case Step::kRelativeHumidityPercent:
      return {Step::kWindSpeedMph, Step::kWindSpeedMph, true};
    case Step::kWindSpeedMph:
      return {Step::kWindHeading, Step::kAzimuthDeg};
    case Step::kWindHeading:
      return {Step::kAzimuthDeg, Step::kAzimuthDeg, true};
    case Step::kAzimuthDeg:
      return {Step::kLatitudeDeg, Step::kRangeAngleDeg};
    case Step::kLatitudeDeg:
      return {Step::kRangeAngleDeg, Step::kRangeAngleDeg, true};
    case Step::kRangeAngleDeg:
      return {Step::kMinimumSpeed, Step::kMinimumSpeed, true};
    case Step::kMinimumSpeed:
      return {Step::kMinimumEnergy, Step::kMinimumEnergy, true};
    case Step::kMinimumEnergy:
      return {Step::kMaximumTime, Step::kMaximumTime, true};
    case Step::kMaximumTime:
      return {Step::kRanges, Step::kRanges, true};
    case Step::kRanges:
      return {Step::kRanges, Step::kRanges, false, false, true};
  }
  return {};
}

bool PromptSingle(Step s, nlohmann::json* j) {
  double input = std::numeric_limits<double>::quiet_NaN();
  bool has_input = false;
  std::string line;

  while (std::isnan(input)) {
    std::cout << GetPrompts().at(s) << '\n' << '>';
    std::getline(std::cin, line);
    if (line.empty()) {
      input = std::numeric_limits<double>::quiet_NaN();
      has_input = false;
      break;
    }
    char* end = nullptr;
    input = std::strtod(line.c_str(), &end);
    if (end != line.c_str() && *end == '\0' && std::isfinite(input)) {
      has_input = true;
    } else {
      std::cerr << "\033[31mInvalid input. Enter a number or omit to skip."
                << "\033[0m\n";
      input = std::numeric_limits<double>::quiet_NaN();
    }
  }

  if (has_input) {
    (*j)[GetKeys().at(s)] = input;
  }
  return has_input;
}

void PromptRanges(Step s, nlohmann::json* j) {
  nlohmann::json list;
  double input = 0;

  while (!std::isnan(input)) {
    input = std::numeric_limits<double>::quiet_NaN();
    std::string line;
    bool parsed = false;

    while (!parsed) {
      std::cout << GetPrompts().at(s) << '\n' << '>';
      std::getline(std::cin, line);
      if (line.empty()) {
        parsed = true;
      } else {
        char* end = nullptr;
        const double kVal = std::strtod(line.c_str(), &end);
        if (end != line.c_str() && *end == '\0' && std::isfinite(kVal)) {
          list.push_back(kVal);
          input = kVal;
          parsed = true;
        } else {
          std::cerr << "\033[31mInvalid input. Enter a number or omit to skip."
                    << "\033[0m\n";
        }
      }
    }
  }

  if (list.empty()) {
    static const nlohmann::json kDefaultRanges = {
        0, 50, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000};
    list = kDefaultRanges;
  }

  (*j)[GetKeys().at(s)] = list;
}

}  // namespace

bool IsInteractive() {
#ifdef _WIN32
  return _isatty(_fileno(stdin)) != 0;
#else
  return isatty(STDIN_FILENO) != 0;
#endif
}

void PrintGreeting() {
  std::cout << "Welcome to lobber, a minimal example program included with the "
               "lob ballistics library. Follow the prompts to enter data.\n\n";
}

nlohmann::json RunWizard() {
  nlohmann::json j;
  for (const auto& kv : GetKeys()) {
    j[kv.second] = "nan";
  }

  PrintGreeting();

  Step current = Step::kBallisticCoefficientPsi;
  while (true) {
    auto info = GetTransition(current);
    if (info.is_list) {
      PromptRanges(current, &j);
      return j;
    }

    const bool kHasInput = PromptSingle(current, &j);

    bool gave = info.always_given || (kHasInput && !info.always_skip);

    if (current == Step::kWindSpeedMph && kHasInput &&
        j[GetKeys().at(current)] == 0) {
      gave = false;
    }

    if (gave) {
      current = info.given;
    } else {
      current = info.skip;
    }
  }
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
