// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

#include "lobber_bridge.hpp"
#include "lobber_cli.hpp"
#include "lobber_wizard.hpp"

int main(int argc, char* argv[]) {
  auto config = example::ParseArgs(argc, argv);

  if (config.show_help) {
    example::PrintHelp();
  std::cout << "Lobber version: " << kProjectVersion << "\n"
            << "Lob version:    " << lob::Version() << "\n\n";
  PrintGH();
}

void PrintHelp() {
  std::cout << "Usage: lobber [options] [< input.json]\n"
            << "Options:\n"
            << "  --h, --help     Show this help message\n"
            << "  --v, --version  Show version information\n"
            << "  --json          Output results to stdout in json format\n"
            << "  --if=FILE       Input json file containing data to use "
               "instead of user prompts\n"
            << "  --of=FILE       Output json file where data is saved for "
               "future use as an input file\n"
            << "\n"
            << "Note: When run interactively, a wizard prompts for input.\n"
            << "      When stdin is redirected, JSON data is read from stdin.\n"
            << "Example:\n"
            << colors::kYellow << "  lobber --of=my_rifle_load.json\n\n"
            << colors::kReset;
  PrintGH();
}

void PrintGreeting() {
  std::cout
      << "Welcome to lobber, a minimal example program included with the lob "
         "ballistics library. Follow the prompts to enter data.\n\n";
}

// Returns true if the program is being run in an interactive terminal.
bool IsInteractive() {
#ifdef _WIN32
  return _isatty(_fileno(stdin)) != 0;
#else
  return isatty(STDIN_FILENO) != 0;
#endif
}

lob::DragFunctionT ConvertDF(double input) {
  switch (static_cast<int>(std::round(input))) {
    case 2:  // NOLINT(cppcoreguidelines-avoid-magic-numbers,
             // readability-magic-numbers)
      return lob::DragFunctionT::kG2;
    case 5:  // NOLINT(cppcoreguidelines-avoid-magic-numbers,
             // readability-magic-numbers)
      return lob::DragFunctionT::kG5;
    case 6:  // NOLINT(cppcoreguidelines-avoid-magic-numbers,
             // readability-magic-numbers)
      return lob::DragFunctionT::kG6;
    case 7:  // NOLINT(cppcoreguidelines-avoid-magic-numbers,
             // readability-magic-numbers)
      return lob::DragFunctionT::kG7;
    case 8:  // NOLINT(cppcoreguidelines-avoid-magic-numbers,
             // readability-magic-numbers)
      return lob::DragFunctionT::kG8;
    case 1:  // NOLINT(cppcoreguidelines-avoid-magic-numbers,
             // readability-magic-numbers)
    default:
      return lob::DragFunctionT::kG1;
  }
}

lob::AtmosphereReferenceT ConvertAR(double input) {
  return 2 == static_cast<int>(std::round(input))
             ? lob::AtmosphereReferenceT::kIcao
             : lob::AtmosphereReferenceT::kArmyStandardMetro;
}

lob::ClockAngleT ConvertCA(double input) {
  switch (static_cast<int>(std::round(input))) {
    case 1:  // NOLINT(cppcoreguidelines-avoid-magic-numbers,
             // readability-magic-numbers)
      return lob::ClockAngleT::kI;
    case 2:  // NOLINT(cppcoreguidelines-avoid-magic-numbers,
             // readability-magic-numbers)
      return lob::ClockAngleT::kII;
    case 3:  // NOLINT(cppcoreguidelines-avoid-magic-numbers,
             // readability-magic-numbers)
      return lob::ClockAngleT::kIII;
    case 4:  // NOLINT(cppcoreguidelines-avoid-magic-numbers,
             // readability-magic-numbers)
      return lob::ClockAngleT::kIV;
    case 5:  // NOLINT(cppcoreguidelines-avoid-magic-numbers,
             // readability-magic-numbers)
      return lob::ClockAngleT::kV;
    case 6:  // NOLINT(cppcoreguidelines-avoid-magic-numbers,
             // readability-magic-numbers)
      return lob::ClockAngleT::kVI;
    case 7:  // NOLINT(cppcoreguidelines-avoid-magic-numbers,
             // readability-magic-numbers)
      return lob::ClockAngleT::kVII;
    case 8:  // NOLINT(cppcoreguidelines-avoid-magic-numbers,
             // readability-magic-numbers)
      return lob::ClockAngleT::kVIII;
    case 9:  // NOLINT(cppcoreguidelines-avoid-magic-numbers,
             // readability-magic-numbers)
      return lob::ClockAngleT::kIX;
    case 10:  // NOLINT(cppcoreguidelines-avoid-magic-numbers,
              // readability-magic-numbers)
      return lob::ClockAngleT::kX;
    case 11:  // NOLINT(cppcoreguidelines-avoid-magic-numbers,
              // readability-magic-numbers)
      return lob::ClockAngleT::kXI;
    case 12:  // NOLINT(cppcoreguidelines-avoid-magic-numbers,
              // readability-magic-numbers)
    default:
      return lob::ClockAngleT::kXII;
  }
}

enum class StateType : uint8_t {
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
  kMachVsDragTable,  // unused
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

const std::map<StateType, std::string>& GetStateKeys() {
  static const std::map<StateType, std::string> kStateKeys{
      {StateType::kBallisticCoefficientPsi, "BallisticCoefficientPsi"},
      {StateType::kBCAtmosphere, "BCAtmosphere"},
      {StateType::kBCDragFunction, "BCDragFunction"},
      {StateType::kDiameterInch, "DiameterInch"},
      {StateType::kMeplatDiameterInch, "MeplatDiameterInch"},
      {StateType::kBaseDiameterInch, "BaseDiameterInch"},
      {StateType::kLengthInch, "LengthInch"},
      {StateType::kNoseLengthInch, "NoseLengthInch"},
      {StateType::kTailLengthInch, "TailLengthInch"},
      {StateType::kOgiveRtR, "OgiveRtR"},
      {StateType::kMassGrains, "MassGrains"},
      {StateType::kInitialVelocityFps, "InitialVelocityFps"},
      {StateType::kOpticHeightInches, "OpticHeightInches"},
      {StateType::kTwistInchesPerTurn, "TwistInchesPerTurn"},
      {StateType::kZeroAngleMOA, "ZeroAngleMOA"},
      {StateType::kZeroDistanceYds, "ZeroDistanceYds"},
      {StateType::kZeroImpactHeightInches, "ZeroImpactHeightInches"},
      {StateType::kAltitudeOfFiringSiteFt, "AltitudeOfFiringSiteFt"},
      {StateType::kAirPressureInHg, "AirPressureInHg"},
      {StateType::kAltitudeOfBarometerFt, "AltitudeOfBarometerFt"},
      {StateType::kTemperatureDegF, "TemperatureDegF"},
      {StateType::kAltitudeOfThermometerFt, "AltitudeOfThermometerFt"},
      {StateType::kRelativeHumidityPercent, "RelativeHumidityPercent"},
      {StateType::kWindHeading, "WindHeading"},
      {StateType::kWindSpeedMph, "WindSpeedMph"},
      {StateType::kAzimuthDeg, "AzimuthDeg"},
      {StateType::kLatitudeDeg, "LatitudeDeg"},
      {StateType::kRangeAngleDeg, "RangeAngleDeg"},
      {StateType::kMinimumSpeed, "MinimumSpeed"},
      {StateType::kMinimumEnergy, "MinimumEnergy"},
      {StateType::kMaximumTime, "MaximumTime"},
      {StateType::kRanges, "Ranges"}};
  return kStateKeys;
}

const std::map<StateType, std::string>& GetStatePrompts() {
  static const std::map<StateType, std::string> kStatePrompts{
      {StateType::kBallisticCoefficientPsi,
       "Enter ballistic coefficient in PSI"},
      {StateType::kBCAtmosphere,
       "Enter 1 for Army Standard Metro or 2 for ICAO reference atmosphere"},
      {StateType::kBCDragFunction,
       "Enter 1, 2, 5, 6, 7, or 8 for associated drag function"},
      {StateType::kDiameterInch, "Enter bullet diameter in inches"},
      {StateType::kMeplatDiameterInch, "Enter meplat diameter in inches"},
      {StateType::kBaseDiameterInch, "Enter base diameter in inches"},
      {StateType::kLengthInch, "Enter bullet length in inches"},
      {StateType::kNoseLengthInch, "Enter nose length in inches"},
      {StateType::kTailLengthInch, "Enter tail length in inches"},
      {StateType::kOgiveRtR, "Enter ogive radius to length ratio (Rt/R)"},
      {StateType::kMassGrains, "Enter bullet weight in grains"},
      {StateType::kInitialVelocityFps, "Enter muzzle velocity in fps"},
      {StateType::kOpticHeightInches, "Enter optic height in inches"},
      {StateType::kTwistInchesPerTurn, "Enter twist rate in inches per turn"},
      {StateType::kZeroAngleMOA, "Enter zero angle in MOA"},
      {StateType::kZeroDistanceYds, "Enter zero range in yards"},
      {StateType::kZeroImpactHeightInches,
       "Enter zero impact height in inches"},
      {StateType::kAltitudeOfFiringSiteFt,
       "Enter altitude of firing site in feet"},
      {StateType::kAirPressureInHg, "Enter air pressure in inches of mercury"},
      {StateType::kAltitudeOfBarometerFt,
       "Enter altitude of barometer in feet"},
      {StateType::kTemperatureDegF, "Enter temperature in degrees Fahrenheit"},
      {StateType::kAltitudeOfThermometerFt,
       "Enter altitude of thermometer in feet"},
      {StateType::kRelativeHumidityPercent,
       "Enter relative humidity in percent"},
      {StateType::kWindHeading,
       "Enter wind heading as a clock angle (1 though 12)"},
      {StateType::kWindSpeedMph, "Enter wind speed in miles per hour"},
      {StateType::kAzimuthDeg, "Enter azimuth in degrees"},
      {StateType::kLatitudeDeg, "Enter latitude in degrees"},
      {StateType::kRangeAngleDeg, "Enter range angle in degrees"},
      {StateType::kMinimumSpeed, "Enter minimum speed in feet per second"},
      {StateType::kMinimumEnergy, "Enter minimum energy in foot pounds"},
      {StateType::kMaximumTime, "Enter maximum time in seconds"},
      {StateType::kRanges, "Enter a range in yards to solve for"}};
  return kStatePrompts;
}

bool Prompt(StateType state, nlohmann::json* pjson) {
  bool is_valid = false;
  bool user_input = false;
  double input = 0;
  const std::string kKey = GetStateKeys().at(state);
  std::string str;

  while (!is_valid) {
    std::cout << GetStatePrompts().at(state) << '\n' << '>';
    std::getline(std::cin, str);
    if (str.empty()) {
      input = std::numeric_limits<double>::quiet_NaN();
      is_valid = true;
    } else {
      std::istringstream iss(str);
      if ((iss >> input) || (iss >> std::ws).eof()) {
        is_valid = true;
        user_input = true;
      } else {
        std::cerr << colors::kRed
                  << "Invalid input. Enter a number or omit to skip."
                  << colors::kReset << "\n";
      }
    }
  }
  if (!std::isnan(input)) {
    (*pjson)[kKey] = input;
  }
  return user_input;
}

bool PromptList(StateType state, nlohmann::json* pjson) {
  bool user_input = false;
  nlohmann::json list;
  double input = 0;
  const std::string kKey = GetStateKeys().at(state);
  std::string str;

  while (!std::isnan(input)) {
    bool is_valid = false;
    while (!is_valid) {
      std::cout << GetStatePrompts().at(state) << '\n' << '>';
      std::getline(std::cin, str);
      if (str.empty()) {
        input = std::numeric_limits<double>::quiet_NaN();
        is_valid = true;
      } else {
        std::istringstream iss(str);
        if ((iss >> input) || (iss >> std::ws).eof()) {
          list.push_back(input);
          is_valid = true;
          user_input = true;
        } else {
          std::cerr << colors::kRed
                    << "Invalid input. Enter a number or omit to skip."
                    << colors::kReset << "\n";
        }
      }
    }
  }
  (*pjson)[kKey] = list;
  return user_input;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void PromptWizard(nlohmann::json* pjson) {
  StateType state = StateType::kBallisticCoefficientPsi;
  while (true) {
    const bool kRealInput = Prompt(state, pjson);
    switch (state) {
      case StateType::kBallisticCoefficientPsi:
        state = kRealInput ? StateType::kBCAtmosphere
                           : StateType::kBallisticCoefficientPsi;
        break;
      case StateType::kBCAtmosphere:
        state = StateType::kBCDragFunction;
        break;
      case StateType::kBCDragFunction:
        state = StateType::kDiameterInch;
        break;
      case StateType::kDiameterInch:
        state = kRealInput ? StateType::kLengthInch : StateType::kMassGrains;
        break;
      case StateType::kMeplatDiameterInch:
        state = kRealInput ? StateType::kBaseDiameterInch
                           : StateType::kTwistInchesPerTurn;
        break;
      case StateType::kBaseDiameterInch:
        state = kRealInput ? StateType::kTwistInchesPerTurn
                           : StateType::kTailLengthInch;
        break;
      case StateType::kLengthInch:
        state =
            kRealInput ? StateType::kNoseLengthInch : StateType::kMassGrains;
        break;
      case StateType::kNoseLengthInch:
        state = kRealInput ? StateType::kMeplatDiameterInch
                           : StateType::kTwistInchesPerTurn;
        break;
      case StateType::kTailLengthInch:
        state =
            kRealInput ? StateType::kOgiveRtR : StateType::kTwistInchesPerTurn;
        break;
      case StateType::kOgiveRtR:
        state = StateType::kTwistInchesPerTurn;
        break;
      // case StateType::kMachVsDragTable:
      //   break;
      case StateType::kMassGrains:
        state = StateType::kInitialVelocityFps;
        break;
      case StateType::kInitialVelocityFps:
        state = kRealInput ? StateType::kOpticHeightInches
                           : StateType::kInitialVelocityFps;
        break;
      case StateType::kOpticHeightInches:
        state = StateType::kZeroAngleMOA;
        break;
      case StateType::kTwistInchesPerTurn:
        state = StateType::kMassGrains;
        break;
      case StateType::kZeroAngleMOA:
        state = kRealInput ? StateType::kAltitudeOfFiringSiteFt
                           : StateType::kZeroDistanceYds;
        break;
      case StateType::kZeroDistanceYds:
        state = kRealInput ? StateType::kZeroImpactHeightInches
                           : StateType::kZeroAngleMOA;
        break;
      case StateType::kZeroImpactHeightInches:
        state = StateType::kAltitudeOfFiringSiteFt;
        break;
      case StateType::kAltitudeOfFiringSiteFt:
        state = StateType::kAirPressureInHg;
        break;
      case StateType::kAirPressureInHg:
        state = kRealInput ? StateType::kAltitudeOfBarometerFt
                           : StateType::kTemperatureDegF;
        break;
      case StateType::kAltitudeOfBarometerFt:
        state = StateType::kTemperatureDegF;
        break;
      case StateType::kTemperatureDegF:
        state = kRealInput ? StateType::kAltitudeOfThermometerFt
                           : StateType::kRelativeHumidityPercent;
        break;
      case StateType::kAltitudeOfThermometerFt:
        state = StateType::kRelativeHumidityPercent;
        break;
      case StateType::kRelativeHumidityPercent:
        state = StateType::kWindSpeedMph;
        break;
      case StateType::kWindHeading:
        state = StateType::kAzimuthDeg;
        break;
      case StateType::kWindSpeedMph:
        if (!kRealInput || (*pjson)[GetStateKeys().at(state)] == 0) {
          state = StateType::kAzimuthDeg;
        } else {
          state = StateType::kWindHeading;
        }
        break;
      case StateType::kAzimuthDeg:
        state =
            kRealInput ? StateType::kLatitudeDeg : StateType::kRangeAngleDeg;
        break;
      case StateType::kLatitudeDeg:
        state = StateType::kRangeAngleDeg;
        break;
      case StateType::kRangeAngleDeg:
        state = StateType::kMinimumSpeed;
        break;
      case StateType::kMinimumSpeed:
        state = StateType::kMinimumEnergy;
        break;
      case StateType::kMinimumEnergy:
        state = StateType::kMaximumTime;
        break;
      case StateType::kMaximumTime:
      case StateType::kRanges:
        state = StateType::kRanges;
        if (!PromptList(state, pjson)) {
          const nlohmann::json kList{0,   50,  100, 200, 300, 400,
                                     500, 600, 700, 800, 900, 1000};
          (*pjson)[GetStateKeys().at(state)] = kList;
        }
        return;
    }
  }
}

  nlohmann::json json;

  // Read input from stdin or wizard
  if (example::IsInteractive()) {
    json = example::RunWizard();
  } else {
    if (std::cin.peek() != std::char_traits<char>::eof()) {
      try {
        std::cin >> json;
      } catch (const nlohmann::json::parse_error& e) {
        std::cerr << "\033[31mError parsing JSON: \033[0m" << e.what() << "\n";
        return 1;
      }
    }
  }

  if (json.empty()) {
    std::cerr << "\033[31mError: No input data provided.\033[0m\n\n";
    example::PrintHelp();
    return 1;
  }

  // Save input config if requested (before solving)
  if (config.has_save_input_path) {
    std::ofstream file(config.save_input_path);
    if (file.is_open()) {
      file << json.dump(4);
      file.close();
    } else {
      std::cerr << "\033[31mError: Could not open save file!\033[0m\n";
      return 1;
    }
  }

  // Solve
  auto result = example::SolveFromJson(json);

  if (config.json_mode) {
    auto jout = example::OutputsToJson(result.outputs.data(), result.count);
    std::cout << jout.dump(4) << "\n";
  } else {
    example::PrintTable(result.input, result.outputs.data(), result.count);
    std::cout << "Report bugs or give feedback here: "
              << "\033[34mhttps://github.com/joelbenway/lob\033[0m\n";
  }

  return 0;
}

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
