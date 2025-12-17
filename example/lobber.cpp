// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <initializer_list>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include "lob/lob.hpp"
#include "version.hpp"

namespace colors {
constexpr const char* kReset = "\033[0m";
constexpr const char* kRed = "\033[31m";
constexpr const char* kGreen = "\033[32m";
constexpr const char* kYellow = "\033[33m";
constexpr const char* kBlue = "\033[34m";
}  // namespace colors

namespace example {
namespace {

void PrintGH() {
  std::cout << "Report bugs or give feedback here: ";
  std::cout << colors::kBlue << "https://github.com/joelbenway/lob\n"
            << colors::kReset;
}

void PrintVersion() {
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
      case StateType::kMachVsDragTable:
        break;
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

double JsonToDouble(const nlohmann::json& json, StateType state) {
  const auto& key = GetStateKeys().at(state);
  if (json[key] == "nan") {
    return std::numeric_limits<double>::quiet_NaN();
  }
  return json[key];
}

uint16_t JsonToU16(const nlohmann::json& json, StateType state) {
  const auto& key = GetStateKeys().at(state);
  if (json[key] == "nan") {
    return 0;
  }
  return json[key];
}

lob::AtmosphereReferenceT JsonToAtmosphere(const nlohmann::json& json,
                                           StateType state) {
  const auto& key = GetStateKeys().at(state);
  if (json[key] == "nan") {
    return lob::AtmosphereReferenceT::kArmyStandardMetro;
  }
  return ConvertAR(json[key]);
}

lob::DragFunctionT JsonToDragFunction(const nlohmann::json& json,
                                      StateType state) {
  const auto& key = GetStateKeys().at(state);
  if (json[key] == "nan") {
    return lob::DragFunctionT::kG1;
  }
  return ConvertDF(json[key]);
}

lob::ClockAngleT JsonToClock(const nlohmann::json& json, StateType state) {
  const auto& key = GetStateKeys().at(state);
  if (json[key] == "nan") {
    return lob::ClockAngleT::kXII;
  }
  return ConvertCA(json[key]);
}

void WriteOutputFile(const std::string& output_file,
                     const nlohmann::json& json) {
  if (output_file.empty()) {
    return;
  }
  std::ofstream file(output_file);
  if (!file.is_open()) {
    std::cerr << colors::kRed << "File can't open!" << colors::kReset << "\n";
    return;
  }
  file << json.dump(4);
  file.close();
}

void PrintExtraInfo(const lob::Input& input) {
  const uint8_t kExtra = 3U;
  const std::string kZA("Zero Angle");
  const auto kZAw = static_cast<int>(kZA.length() + kExtra);
  const std::string kSF("Stability Factor");
  const auto kSFw = static_cast<int>(kSF.length() + kExtra);
  const std::string kSS("Speed of Sound");
  const auto kSSw = static_cast<int>(kSS.length() + kExtra);
  const std::string kE("Error");
  const auto kEw = static_cast<int>(kE.length() + kExtra);

  std::cout << colors::kYellow << std::left << std::setw(kZAw) << kZA
            << std::setw(kSFw) << kSF << std::setw(kSSw) << kSS
            << std::setw(kEw) << kE << colors::kReset << "\n";
  std::cout << std::left << std::setw(kZAw) << std::fixed
            << std::setprecision(2) << input.zero_angle << std::setw(kSFw)
            << input.stability_factor << std::setw(kSSw) << input.speed_of_sound
            << std::setw(kEw) << std::hex << std::showbase
            << static_cast<unsigned int>(input.error) << std::dec
            << std::noshowbase << "\n\n";
}

void PrintSolutionTable(const lob::Output* psolutions, size_t size) {
  // Column widths for better formatting
  const int kWidth = 12;

  // Print table header
  std::cout << std::left << std::setw(kWidth) << colors::kGreen << "Yards"
            << std::setw(kWidth) << "FPS" << std::setw(kWidth) << "FtLbs"
            << std::setw(kWidth) << "Elev Inch" << std::setw(kWidth)
            << "Elev MOA" << std::setw(kWidth) << "Elev MIL"
            << std::setw(kWidth) << "Wind Inch" << std::setw(kWidth)
            << "Wind MOA" << std::setw(kWidth) << "Wind MIL"
            << std::setw(kWidth) << "Seconds" << colors::kReset << "\n";

  // Print table content
  for (size_t i = 0; i < size; ++i) {
    std::cout << std::left << std::setw(kWidth) << psolutions[i].range / 3
              << std::setw(kWidth) << psolutions[i].velocity
              << std::setw(kWidth) << psolutions[i].energy << std::setw(kWidth)
              << std::fixed << std::setprecision(2) << psolutions[i].elevation
              << std::setw(kWidth)
              << lob::InchToMoa(psolutions[i].elevation, psolutions[i].range)
              << std::setw(kWidth)
              << lob::InchToMil(psolutions[i].elevation, psolutions[i].range)
              << std::setw(kWidth) << psolutions[i].deflection
              << std::setw(kWidth)
              << lob::InchToMoa(psolutions[i].deflection, psolutions[i].range)
              << std::setw(kWidth)
              << lob::InchToMil(psolutions[i].deflection, psolutions[i].range)
              << std::setw(kWidth) << std::setprecision(3)
              << psolutions[i].time_of_flight << "\n";
  }
  std::cout << "\n";
}

}  // namespace
}  // namespace example

namespace lob {
// NOLINTNEXTLINE(readability-identifier-naming, misc-use-internal-linkage)
void to_json(nlohmann::json& j, const lob::Output& o) {
  j = nlohmann::json{
      {"range", o.range},           {"velocity", o.velocity},
      {"energy", o.energy},         {"elevation", o.elevation},
      {"deflection", o.deflection}, {"time_of_flight", o.time_of_flight}};
}
}  // namespace lob

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
int main(int argc, char* argv[]) {
  const std::string kHelp = "--help";
  const std::string kH = "--h";
  const std::string kVersion = "--version";
  const std::string kV = "--v";
  const std::string kIf = "--if=";
  const std::string kOf = "--of=";
  const std::string kJson = "--json";

  std::string output_file;
  nlohmann::json json;
  bool output_in_json = false;

  for (int i = 1; i < argc; i++) {
    const std::string kArg = argv[i];
    if (kArg == kHelp || kArg == kH) {
      example::PrintHelp();
      return 0;
    }
    if (kArg == kVersion || kArg == kV) {
      example::PrintVersion();
      return 0;
    }
    if (kArg.substr(0, kIf.length()) == kIf) {
      const std::string kInputFile = kArg.substr(kIf.length());
      std::ifstream file(kInputFile);
      if (!file.is_open()) {
        std::cerr << colors::kRed << "Error: Could not open the JSON file!"
                  << colors::kReset << "\n";
        return 1;
      }
      try {
        file >> json;
      } catch (const nlohmann::json::parse_error& e) {
        std::cerr << colors::kRed << "Error parsing JSON: " << colors::kReset
                  << e.what() << "\n";
        return 1;
      }
      continue;
    }
    if (kArg.substr(0, kOf.length()) == kOf) {
      output_file = kArg.substr(kOf.length());
      continue;
    }
    if (kArg.substr(0, kJson.length()) == kJson) {
      output_in_json = true;
      continue;
    }
  }

  if (json.empty()) {
    if (example::IsInteractive()) {
      for (const auto& pair : example::GetStateKeys()) {
        json[pair.second] = "nan";
      }
      example::PrintGreeting();
      example::PromptWizard(&json);
    } else {
      if (std::cin.peek() != std::char_traits<char>::eof()) {
        try {
          std::cin >> json;
        } catch (const nlohmann::json::parse_error& e) {
          std::cerr << colors::kRed
                    << "Error parsing JSON from stdin: " << colors::kReset
                    << e.what() << "\n";
          return 1;
        }
      }
    }
  }

  if (json.empty()) {
    std::cerr << colors::kRed << "Error: No input data provided."
              << colors::kReset << "\n\n";
    example::PrintHelp();
    return 1;
  }

  using example::StateType;
  const lob::Input kSolverInput =
      lob::Builder()
          .BallisticCoefficientPsi(
              JsonToDouble(json, StateType::kBallisticCoefficientPsi))
          .BCAtmosphere(JsonToAtmosphere(json, StateType::kBCAtmosphere))
          .BCDragFunction(JsonToDragFunction(json, StateType::kBCDragFunction))
          .DiameterInch(JsonToDouble(json, StateType::kDiameterInch))
          .MeplatDiameterInch(
              JsonToDouble(json, StateType::kMeplatDiameterInch))
          .BaseDiameterInch(JsonToDouble(json, StateType::kBaseDiameterInch))
          .LengthInch(JsonToDouble(json, StateType::kLengthInch))
          .NoseLengthInch(JsonToDouble(json, StateType::kNoseLengthInch))
          .TailLengthInch(JsonToDouble(json, StateType::kTailLengthInch))
          .OgiveRtR(JsonToDouble(json, StateType::kOgiveRtR))
          .MassGrains(JsonToDouble(json, StateType::kMassGrains))
          .InitialVelocityFps(JsonToU16(json, StateType::kInitialVelocityFps))
          .OpticHeightInches(JsonToDouble(json, StateType::kOpticHeightInches))
          .TwistInchesPerTurn(
              JsonToDouble(json, StateType::kTwistInchesPerTurn))
          .ZeroAngleMOA(JsonToDouble(json, StateType::kZeroAngleMOA))
          .ZeroDistanceYds(JsonToDouble(json, StateType::kZeroDistanceYds))
          .ZeroImpactHeightInches(
              JsonToDouble(json, StateType::kZeroImpactHeightInches))
          .AltitudeOfFiringSiteFt(
              JsonToDouble(json, StateType::kAltitudeOfFiringSiteFt))
          .AirPressureInHg(JsonToDouble(json, StateType::kAirPressureInHg))
          .AltitudeOfBarometerFt(
              JsonToDouble(json, StateType::kAltitudeOfBarometerFt))
          .TemperatureDegF(JsonToDouble(json, StateType::kTemperatureDegF))
          .AltitudeOfThermometerFt(
              JsonToDouble(json, StateType::kAltitudeOfThermometerFt))
          .RelativeHumidityPercent(
              JsonToDouble(json, StateType::kRelativeHumidityPercent))
          .WindHeading(JsonToClock(json, StateType::kWindHeading))
          .WindSpeedMph(JsonToDouble(json, StateType::kWindSpeedMph))
          .AzimuthDeg(JsonToDouble(json, StateType::kAzimuthDeg))
          .LatitudeDeg(JsonToDouble(json, StateType::kLatitudeDeg))
          .RangeAngleDeg(JsonToDouble(json, StateType::kRangeAngleDeg))
          .MinimumSpeed(JsonToU16(json, StateType::kMinimumSpeed))
          .MinimumEnergy(JsonToU16(json, StateType::kMinimumEnergy))
          .MaximumTime(JsonToDouble(json, StateType::kMaximumTime))
          .StepSize(100)
          .Build();

  std::vector<uint32_t> ranges =
      json[example::GetStateKeys().at(StateType::kRanges)]
          .get<std::vector<uint32_t>>();
  for (auto& range : ranges) {
    range *= 3;
  }
  std::vector<lob::Output> solutions;
  solutions.resize(ranges.size());

  const size_t kSize =
      lob::Solve(kSolverInput, ranges.data(), solutions.data(), ranges.size());

  example::WriteOutputFile(output_file, json);

  if (output_in_json) {
    nlohmann::json jsolutions = solutions;
    std::cout << jsolutions.dump(4) << "\n";
    return 0;
  }

  example::PrintExtraInfo(kSolverInput);
  example::PrintSolutionTable(solutions.data(), kSize);
  example::PrintGH();
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
