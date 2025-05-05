// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <initializer_list>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "lob/lob.hpp"
#include "version.hpp"

namespace example {
namespace {
void PrintGH() {
  std::cout << "Report bugs or give feedback here: "
               "https://github.com/joelbenway/lob\n";
}

void PrintVersion() {
  std::cout << "Lobber version: " << kProjectVersion << "\n"
            << "Lob version:    " << lob::Version() << "\n\n";
  PrintGH();
}

void PrintHelp() {
  std::cout << "Usage: lobber [options]\n"
            << "Options:\n"
            << "  --h, --help     Show this help message\n"
            << "  --v, --version  Show version information\n"
            << "  --if=FILE       Input file containing data to use instead of "
               "user prompts\n"
            << "  --of=FILE       Output file where data is saved for future "
               "use as an input file\n"
            << "Example:\n"
            << "  lobber --of=my_rifle_load.txt\n\n";
  PrintGH();
}

void PrintGreeting() {
  std::cout
      << "Welcome to lobber, a minimal example program included with the lob "
         "ballistics library. Follow the prompts to enter data.\n\n";
}

double Prompt(const std::string& prompt) {
  bool is_valid = false;
  double input = 0;
  std::string str;

  while (!is_valid) {
    std::cout << prompt << '\n' << '>';
    std::getline(std::cin, str);
    if (str.empty()) {
      input = std::numeric_limits<double>::quiet_NaN();
      is_valid = true;
    } else {
      std::istringstream iss(str);
      if ((iss >> input) || (iss >> std::ws).eof()) {
        is_valid = true;
      } else {
        std::cerr << "Invalid input. Enter a number or omit to skip.\n";
      }
    }
  }

  return input;
}

double Read(std::ifstream* pfile) {
  std::string line;
  std::getline(*pfile, line);
  double input = std::numeric_limits<double>::quiet_NaN();

  if (!line.empty() && line != "nan") {
    std::istringstream iss(line);
    iss >> input;
  }
  return input;
}

void GetInput(const std::string& prompt, std::vector<double>* inputs,
              std::ifstream* pfile) {
  if (!*pfile) {
    inputs->push_back(Prompt(prompt));
  } else {
    inputs->push_back(Read(pfile));
  }
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

bool WriteOutputFile(const std::string& file_name,
                     const std::vector<double>& inputs) {
  std::ofstream output_file(file_name);

  if (!output_file.is_open()) {
    std::cerr << "Error opening output file: " << file_name << '\n';
    return false;
  }

  for (const auto kValue : inputs) {
    output_file << kValue << '\n';
  }

  output_file.close();
  return true;
}

enum class BuildState : uint8_t {
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
  kMachVsDragTable,
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
  kRangeAngleDeg
};

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
lob::Builder BuildHelper(std::ifstream* pinfile, std::vector<double>* pinputs) {
  lob::Builder builder;
  BuildState state = BuildState::kBallisticCoefficientPsi;
  std::string prompt = "Enter Ballistic Coefficient in PSI";
  while (true) {
    switch (state) {
      case BuildState::kBallisticCoefficientPsi:
        GetInput(prompt, pinputs, pinfile);
        builder.BallisticCoefficientPsi(pinputs->back());
        state = std::isnan(pinputs->back())
                    ? BuildState::kBallisticCoefficientPsi
                    : BuildState::kBCAtmosphere;
        break;
      case BuildState::kBCAtmosphere:
        prompt =
            "Enter 1 for Army Standard Metro or 2 for ICAO reference "
            "atmosphere";
        GetInput(prompt, pinputs, pinfile);
        builder.BCAtmosphere(ConvertAR(pinputs->back()));
        state = BuildState::kBCDragFunction;
        break;
      case BuildState::kBCDragFunction:
        prompt = "Enter 1, 2, 5, 6, 7, or 8 for associated drag function";
        GetInput(prompt, pinputs, pinfile);
        builder.BCDragFunction(ConvertDF(pinputs->back()));
        state = BuildState::kDiameterInch;
        break;
      case BuildState::kDiameterInch:
        prompt = "Enter projectile diameter in inches";
        GetInput(prompt, pinputs, pinfile);
        builder.DiameterInch(pinputs->back());
        state = std::isnan(pinputs->back()) ? BuildState::kMassGrains
                                            : BuildState::kLengthInch;
        break;
      case BuildState::kMeplatDiameterInch:
        prompt = "Enter projectile meplat diameter in inches";
        GetInput(prompt, pinputs, pinfile);
        builder.MeplatDiameterInch(pinputs->back());
        state = std::isnan(pinputs->back()) ? BuildState::kTwistInchesPerTurn
                                            : BuildState::kBaseDiameterInch;
        break;
      case BuildState::kBaseDiameterInch:
        prompt = "Enter projectile base diameter in inches";
        GetInput(prompt, pinputs, pinfile);
        builder.BaseDiameterInch(pinputs->back());
        state = std::isnan(pinputs->back()) ? BuildState::kTwistInchesPerTurn
                                            : BuildState::kTailLengthInch;
        break;
      case BuildState::kLengthInch:
        prompt = "Enter projectile length in inches";
        GetInput(prompt, pinputs, pinfile);
        builder.LengthInch(pinputs->back());
        state = std::isnan(pinputs->back()) ? BuildState::kMassGrains
                                            : BuildState::kNoseLengthInch;
        break;
      case BuildState::kNoseLengthInch:
        prompt = "Enter projectile ogive (nose) length in inches";
        GetInput(prompt, pinputs, pinfile);
        builder.NoseLengthInch(pinputs->back());
        state = std::isnan(pinputs->back()) ? BuildState::kTwistInchesPerTurn
                                            : BuildState::kMeplatDiameterInch;
        break;
      case BuildState::kTailLengthInch:
        prompt = "Enter projectile boat tail length in inches";
        GetInput(prompt, pinputs, pinfile);
        builder.TailLengthInch(pinputs->back());
        state = std::isnan(pinputs->back()) ? BuildState::kTwistInchesPerTurn
                                            : BuildState::kOgiveRtR;
        break;
      case BuildState::kOgiveRtR:
        prompt = "Enter ogive Rt/R ratio";
        GetInput(prompt, pinputs, pinfile);
        builder.OgiveRtR(pinputs->back());
        state = BuildState::kTwistInchesPerTurn;
        break;
      case BuildState::kMachVsDragTable:
        break;
      case BuildState::kMassGrains:
        prompt = "Enter projectile mass in grains";
        GetInput(prompt, pinputs, pinfile);
        builder.MassGrains(pinputs->back());
        state = BuildState::kInitialVelocityFps;
        break;
      case BuildState::kInitialVelocityFps:
        prompt = "Enter initial velocity of projectile in FPS";
        GetInput(prompt, pinputs, pinfile);
        builder.InitialVelocityFps(
            static_cast<uint16_t>(std::round(pinputs->back())));
        state = std::isnan(pinputs->back()) ? BuildState::kInitialVelocityFps
                                            : BuildState::kOpticHeightInches;
        break;
      case BuildState::kOpticHeightInches:
        prompt = "Enter the rifle's optic height above bore in inches";
        GetInput(prompt, pinputs, pinfile);
        builder.OpticHeightInches(pinputs->back());
        state = BuildState::kZeroAngleMOA;
        break;
      case BuildState::kTwistInchesPerTurn:
        prompt = "Enter rifle's barrel twist rate in inches per turn";
        GetInput(prompt, pinputs, pinfile);
        builder.TwistInchesPerTurn(pinputs->back());
        state = BuildState::kMassGrains;
        break;
      case BuildState::kZeroAngleMOA:
        prompt = "Enter angle in MOA between optic's line of sight and bore";
        GetInput(prompt, pinputs, pinfile);
        builder.ZeroAngleMOA(pinputs->back());
        state = std::isnan(pinputs->back())
                    ? BuildState::kZeroDistanceYds
                    : BuildState::kAltitudeOfFiringSiteFt;
        break;
      case BuildState::kZeroDistanceYds:
        prompt = "Enter range in yards of the rifle's zero";
        GetInput(prompt, pinputs, pinfile);
        builder.ZeroDistanceYds(pinputs->back());
        state = std::isnan(pinputs->back())
                    ? BuildState::kZeroAngleMOA
                    : BuildState::kZeroImpactHeightInches;
        break;
      case BuildState::kZeroImpactHeightInches:
        prompt =
            "Enter height in inches for zero impact above zero aiming point";
        GetInput(prompt, pinputs, pinfile);
        builder.ZeroImpactHeightInches(pinputs->back());
        state = BuildState::kAltitudeOfFiringSiteFt;
        break;
      case BuildState::kAltitudeOfFiringSiteFt:
        prompt = "Enter altitude in feet of firing site";
        GetInput(prompt, pinputs, pinfile);
        builder.AltitudeOfFiringSiteFt(pinputs->back());
        state = BuildState::kAirPressureInHg;
        break;
      case BuildState::kAirPressureInHg:
        prompt = "Enter air pressure in inches of mercury";
        GetInput(prompt, pinputs, pinfile);
        builder.AirPressureInHg(pinputs->back());
        state = std::isnan(pinputs->back())
                    ? BuildState::kTemperatureDegF
                    : BuildState::kAltitudeOfBarometerFt;
        break;
      case BuildState::kAltitudeOfBarometerFt:
        prompt = "Enter altitude in feet of air pressure measurement";
        GetInput(prompt, pinputs, pinfile);
        builder.AltitudeOfBarometerFt(pinputs->back());
        state = BuildState::kTemperatureDegF;
        break;
      case BuildState::kTemperatureDegF:
        prompt = "Enter temperature in degrees F";
        GetInput(prompt, pinputs, pinfile);
        builder.TemperatureDegF(pinputs->back());
        state = std::isnan(pinputs->back())
                    ? BuildState::kRelativeHumidityPercent
                    : BuildState::kAltitudeOfThermometerFt;
        break;
      case BuildState::kAltitudeOfThermometerFt:
        prompt = "Enter altitude in feet of temperature measurement";
        GetInput(prompt, pinputs, pinfile);
        builder.AltitudeOfThermometerFt(pinputs->back());
        state = BuildState::kRelativeHumidityPercent;
        break;
      case BuildState::kRelativeHumidityPercent:
        prompt = "Enter relative humidity percent";
        GetInput(prompt, pinputs, pinfile);
        builder.RelativeHumidityPercent(pinputs->back());
        state = BuildState::kWindSpeedMph;
        break;
      case BuildState::kWindHeading:
        prompt = "Enter wind heading as a clock direction 1-12";
        GetInput(prompt, pinputs, pinfile);
        builder.WindHeading(ConvertCA(pinputs->back()));
        state = BuildState::kAzimuthDeg;
        break;
      case BuildState::kWindSpeedMph:
        prompt = "Enter wind speed as Mph";
        GetInput(prompt, pinputs, pinfile);
        builder.WindSpeedFps(pinputs->back());
        if (std::isnan(pinputs->back()) ||
            pinputs->back() < std::numeric_limits<double>::epsilon()) {
          state = BuildState::kAzimuthDeg;
        } else {
          state = BuildState::kWindHeading;
        }
        break;
      case BuildState::kAzimuthDeg:
        prompt = "Enter azimuth in degrees";
        GetInput(prompt, pinputs, pinfile);
        builder.AzimuthDeg(pinputs->back());
        state = std::isnan(pinputs->back()) ? BuildState::kRangeAngleDeg
                                            : BuildState::kLatitudeDeg;
        break;
      case BuildState::kLatitudeDeg:
        prompt = "Enter latitude in degrees";
        GetInput(prompt, pinputs, pinfile);
        builder.LatitudeDeg(pinputs->back());
        state = BuildState::kRangeAngleDeg;
        break;
      case BuildState::kRangeAngleDeg:
        prompt = "Enter the angle of incline (or decline) in degrees";
        GetInput(prompt, pinputs, pinfile);
        builder.RangeAngleDeg(pinputs->back());
        return builder;
    }
  }
}

std::vector<uint32_t> RangeHelper(std::ifstream* pinfile,
                                  std::vector<double>* pinputs) {
  std::vector<uint32_t> ranges_ft;
  const static std::string kPrompt = "Enter a range in yards to solve for";
  if (!*pinfile) {
    while (true) {
      const double kRangeYds = Prompt(kPrompt);
      if (std::isnan(kRangeYds)) {
        break;
      }
      ranges_ft.push_back(static_cast<uint32_t>(std::round(kRangeYds)) * 3);
    }
  } else {
    const auto kSize = static_cast<size_t>(std::round(Read(pinfile)));
    ranges_ft.resize(kSize);
    for (auto& range_ft : ranges_ft) {
      range_ft = static_cast<uint32_t>(std::round(Read(pinfile)));
    }
  }

  if (ranges_ft.empty()) {
    const std::initializer_list<uint32_t> kRangesFt = {
        0U,     150U,   300U,   600U,   900U,   1'200U,
        1'500U, 1'800U, 2'100U, 2'400U, 2'700U, 3'000U};
    ranges_ft = kRangesFt;
  } else {
    std::sort(ranges_ft.begin(), ranges_ft.end());
    ranges_ft.erase(std::unique(ranges_ft.begin(), ranges_ft.end()),
                    ranges_ft.end());
  }

  const auto kSize = static_cast<double>(ranges_ft.size());
  pinputs->push_back(kSize);
  for (auto range_ft : ranges_ft) {
    pinputs->push_back(static_cast<double>(range_ft));
  }
  return ranges_ft;
}

lob::Options OptionsHelper(std::ifstream* pinfile,
                           std::vector<double>* pinputs) {
  lob::Options options;

  std::string prompt = "Enter minimum speed threshold in feet per second";
  GetInput(prompt, pinputs, pinfile);
  if (!std::isnan(pinputs->back())) {
    options.min_speed = static_cast<uint16_t>(std::round(pinputs->back()));
  }

  prompt = "Enter minimum energy threshold in foot-pounds";
  GetInput(prompt, pinputs, pinfile);
  if (!std::isnan(pinputs->back())) {
    options.min_energy = static_cast<uint16_t>(std::round(pinputs->back()));
  }

  prompt = "Enter maximum time of flight in seconds";
  GetInput(prompt, pinputs, pinfile);
  if (!std::isnan(pinputs->back())) {
    options.max_time = pinputs->back();
  }

  const uint16_t kStepSize = 100U;
  options.step_size = kStepSize;

  return options;
}

struct SolverData {
  lob::Input input;
  std::vector<uint32_t> ranges;
  lob::Options options;
  std::vector<lob::Output> solutions;
};

std::unique_ptr<SolverData> Wizard(const std::string& infile,
                                   const std::string& outfile) {
  std::ifstream file(infile);
  auto* pfile = &file;
  std::vector<double> inputs;
  auto* pinputs = &inputs;
  std::unique_ptr<SolverData> pwizard(new SolverData);

  if (!file) {
    PrintGreeting();
  }
  lob::Builder builder = BuildHelper(pfile, pinputs);
  pwizard->input = builder.Build();
  pwizard->ranges = RangeHelper(pfile, pinputs);
  pwizard->solutions.resize(pwizard->ranges.size());
  pwizard->options = OptionsHelper(pfile, pinputs);

  if (!outfile.empty() && !file) {
    WriteOutputFile(outfile, inputs);
  }
  return pwizard;
}

void PrintExtraInfo(const lob::Input& input) {
  const uint8_t kExtra = 3U;
  const std::string kZA("Zero Angle");
  const auto kZAw = static_cast<int>(kZA.length() + kExtra);
  const std::string kSF("Stability Factor");
  const auto kSFw = static_cast<int>(kSF.length() + kExtra);
  const std::string kSS("Speed of Sound");
  const auto kSSw = static_cast<int>(kSS.length() + kExtra);

  std::cout << std::left << std::setw(kZAw) << kZA << std::setw(kSFw) << kSF
            << std::setw(kSSw) << kSS << "\n";
  std::cout << std::left << std::setw(kZAw) << std::fixed
            << std::setprecision(2) << input.zero_angle << std::setw(kSFw)
            << input.stability_factor << std::setw(kSSw) << input.speed_of_sound
            << "\n\n";
}

void PrintSolutionTable(const lob::Output* psolutions, size_t size) {
  // Column widths for better formatting
  const int kWidth = 12;

  // Print table header
  std::cout << std::left << std::setw(kWidth) << "Yards" << std::setw(kWidth)
            << "FPS" << std::setw(kWidth) << "FtLbs" << std::setw(kWidth)
            << "Elev Inch" << std::setw(kWidth) << "Elev MOA"
            << std::setw(kWidth) << "Elev MIL" << std::setw(kWidth)
            << "Wind Inch" << std::setw(kWidth) << "Wind MOA"
            << std::setw(kWidth) << "Wind MIL" << std::setw(kWidth) << "Seconds"
            << "\n";

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

int main(int argc, char* argv[]) {
  const std::string kHelp = "--help";
  const std::string kH = "--h";
  const std::string kVersion = "--version";
  const std::string kV = "--v";
  const std::string kIf = "--if=";
  const std::string kOf = "--of=";

  std::string input_file;
  std::string output_file;

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
      input_file = kArg.substr(kIf.length());
      continue;
    }
    if (kArg.substr(0, kIf.length()) == kOf) {
      output_file = kArg.substr(kOf.length());
      continue;
    }
  }

  auto psolver_data = example::Wizard(input_file, output_file);

  while (std::isnan(psolver_data->input.table_coefficient)) {
    psolver_data.reset();
    input_file.clear();  // prevent bad file loop
    std::cout << "\nOOPS! INVALID DATA, let's start over.\n";
    psolver_data = example::Wizard(input_file, output_file);
  }

  example::PrintExtraInfo(psolver_data->input);

  const auto kSize =
      lob::Solve(psolver_data->input, psolver_data->ranges.data(),
                 psolver_data->solutions.data(), psolver_data->ranges.size(),
                 psolver_data->options);

  example::PrintSolutionTable(psolver_data->solutions.data(), kSize);
  example::PrintGH();
  std::cout << "Thanks for using lobber! Goodbye.";

  psolver_data.reset();
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