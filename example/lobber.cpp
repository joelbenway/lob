// This file is a part of lob, an exterior ballistics calculation library
// Copyright (c) 2025  Joel Benway
// Please see end of file for extended copyright information

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#include "lob/lob.hpp"
#include "version.hpp"

namespace example {
namespace {
void PrintVersion() {
  std::cout << "Lobber version: " << kProjectVersion << '\n'
            << "Lob version:    " << lob::Version() << '\n';
}

void PrintHelp() {
  std::cout
      << "Usage: lobber [options]\n"
      << "Options:\n"
      << "  --h, --help     Show this help message\n"
      << "  --v, --version  Show version information\n"
      << "  --if=FILE       Input file containing input data for calculation\n"
      << "  --of=FILE       Output file where input data may be saved for "
         "future use as an input file\n";
}

float Prompt(const std::string& prompt) {
  bool is_valid = false;
  float input = 0;
  std::string str;

  while (!is_valid) {
    std::cout << prompt << '\n' << '>';
    std::getline(std::cin, str);
    if (str.empty()) {
      input = std::numeric_limits<float>::quiet_NaN();
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

float Read(std::ifstream* pfile) {
  std::string line;
  std::getline(*pfile, line);
  float input = std::numeric_limits<float>::quiet_NaN();

  if (!line.empty() && line != "nan") {
    std::istringstream iss(line);
    iss >> input;
  }
  return input;
}

lob::DragFunctionT ConvertDF(float input) {
  switch (static_cast<int>(std::round(input))) {
    case 2:  // NOLINT magic number
      return lob::DragFunctionT::kG2;
    case 5:  // NOLINT magic number
      return lob::DragFunctionT::kG5;
    case 6:  // NOLINT magic number
      return lob::DragFunctionT::kG6;
    case 7:  // NOLINT magic number
      return lob::DragFunctionT::kG7;
    case 8:  // NOLINT magic number
      return lob::DragFunctionT::kG8;
    case 1:  // NOLINT magic number
    default:
      return lob::DragFunctionT::kG1;
  }
}

lob::AtmosphereReferenceT ConvertAR(float input) {
  return 2 == static_cast<int>(std::round(input))
             ? lob::AtmosphereReferenceT::kIcao
             : lob::AtmosphereReferenceT::kArmyStandardMetro;
}

lob::ClockAngleT ConvertCA(float input) {
  switch (static_cast<int>(std::round(input))) {
    case 1:  // NOLINT magic number
      return lob::ClockAngleT::kI;
    case 2:  // NOLINT magic number
      return lob::ClockAngleT::kII;
    case 3:  // NOLINT magic number
      return lob::ClockAngleT::kIII;
    case 4:  // NOLINT magic number
      return lob::ClockAngleT::kIV;
    case 5:  // NOLINT magic number
      return lob::ClockAngleT::kV;
    case 6:  // NOLINT magic number
      return lob::ClockAngleT::kVI;
    case 7:  // NOLINT magic number
      return lob::ClockAngleT::kVII;
    case 8:  // NOLINT magic number
      return lob::ClockAngleT::kVIII;
    case 9:  // NOLINT magic number
      return lob::ClockAngleT::kIX;
    case 10:  // NOLINT magic number
      return lob::ClockAngleT::kX;
    case 11:  // NOLINT magic number
      return lob::ClockAngleT::kXI;
    case 12:  // NOLINT magic number
    default:
      return lob::ClockAngleT::kXII;
  }
}

bool WriteOutputFile(const std::string& file_name,
                     const std::vector<float>& inputs) {
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

lob::Builder BuildHelper(const std::string& infile,
                         const std::string& outfile) {
  lob::Builder builder;
  std::string prompt;
  auto* pprompt = &prompt;
  std::vector<float> inputs;
  auto* pinputs = &inputs;
  std::ifstream file(infile);
  auto* pfile = &file;

  auto collect_inputs = [pinputs, pfile, pprompt]() {
    pinputs->push_back(!*pfile ? Prompt(*pprompt) : Read(pfile));
  };

  prompt = "Enter Ballistic Coefficient in PSI";
  collect_inputs();
  builder.BallisticCoefficentPsi(inputs.back());

  prompt = "Enter 1 for Army Standard Metro or 2 for ICAO reference atmosphere";
  collect_inputs();
  builder.BCAtmosphere(ConvertAR(inputs.back()));

  prompt = "Enter 1, 2, 5, 6, 7, or 8 for associated drag function";
  collect_inputs();
  builder.BCDragFunction(ConvertDF(inputs.back()));

  prompt = "Enter projectile diameter in inches";
  collect_inputs();
  builder.DiameterInch(inputs.back());

  prompt = "Enter projectile length in inches";
  collect_inputs();
  builder.LengthInch(inputs.back());

  prompt = "Enter projectile mass in grains";
  collect_inputs();
  builder.MassGrains(inputs.back());

  prompt = "Enter initial velocity of projectile in FPS";
  collect_inputs();
  builder.InitialVelocityFps(static_cast<uint16_t>(std::round(inputs.back())));

  prompt = "Enter the rifle's optic height above bore in inches";
  collect_inputs();
  builder.OpticHeightInches(inputs.back());

  prompt = "Enter rifle's barrel twist rate in inches per turn";
  collect_inputs();
  builder.TwistInchesPerTurn(inputs.back());

  prompt = "Enter angle in MOA between optic's line of sight and bore";
  collect_inputs();
  builder.ZeroAngleMOA(inputs.back());

  prompt = "Enter range in yards of the rifle's zero";
  collect_inputs();
  builder.ZeroDistanceYds(inputs.back());

  prompt = "Enter height in inches for zero impact above zero aiming point";
  collect_inputs();
  builder.ZeroImpactHeightInches(inputs.back());

  prompt = "Enter altitude of firing site in feet";
  collect_inputs();
  builder.AltitudeOfFiringSiteFt(inputs.back());

  prompt = "Enter air pressure in inches of mercury";
  collect_inputs();
  builder.AirPressureInHg(inputs.back());

  prompt = "Enter altitude in feet associated with air pressure value";
  collect_inputs();
  builder.AltitudeOfBarometerFt(inputs.back());

  prompt = "Enter temperature in degrees F";
  collect_inputs();
  builder.TemperatureDegF(inputs.back());

  prompt = "Enter altitude in feet associated with temperature value";
  collect_inputs();
  builder.AltitudeOfThermometerFt(inputs.back());

  prompt = "Enter relative humidity percent";
  collect_inputs();
  builder.RelativeHumidityPercent(inputs.back());

  prompt = "Enter wind heading as a clock direction 1-12";
  collect_inputs();
  builder.WindHeading(ConvertCA(inputs.back()));

  prompt = "Enter wind speed as Mph";
  collect_inputs();
  builder.WindSpeedFps(inputs.back());

  prompt = "Enter azimuth in degrees";
  collect_inputs();
  builder.AzimuthDeg(inputs.back());

  prompt = "Enter latitude in degrees";
  collect_inputs();
  builder.LatitudeDeg(inputs.back());

  if (!outfile.empty() && !file) {
    WriteOutputFile(outfile, inputs);
  }

  return builder;
}

// NOLINTNEXTLINE c-style arrays
void PlotSolution(const lob::Output solutions[], size_t size) {
  std::vector<uint32_t> x;
  std::vector<float> y;
  for (size_t i = 0; i < size; i++) {
    x.push_back(solutions[i].range);
    y.push_back(solutions[i].elevation);
  }
  constexpr int32_t kWidth = 92;
  constexpr int32_t kHeight = kWidth / 10;
  const auto kXminmax = std::minmax_element(x.begin(), x.end());
  const auto kYminmax = std::minmax_element(y.begin(), y.end());
  const auto kXmin = *kXminmax.first;
  const auto kXmax = *kXminmax.second;
  const auto kYmin = *kYminmax.first;
  const auto kYmax = *kYminmax.second;

  std::vector<std::vector<char>> plot(kHeight, std::vector<char>(kWidth, ' '));

  for (size_t i = 0; i < x.size(); i++) {
    const unsigned int kXpos =
        (x.at(i) - kXmin) * (kWidth - 1) / (kXmax - kXmin);
    int y_pos =
        static_cast<int>((y.at(i) - kYmin) * (kHeight - 1) / (kYmax - kYmin));
    y_pos = kHeight - 1 - y_pos;  // Invert y-axis

    if (kXpos < kWidth && y_pos >= 0 && y_pos < kHeight) {
      plot.at(static_cast<size_t>(y_pos)).at(static_cast<size_t>(kXpos)) = '*';
    }
  }

  const std::string kTitle = "Range vs Drop\n";
  std::cout << std::string((kWidth - kTitle.length()) / 2, ' ') << kTitle;
  for (const auto& row : plot) {
    for (const char kC : row) {
      std::cout << kC;
    }
    std::cout << '\n';
  }

  std::cout << kXmin;
  std::cout << std::string(
      kWidth - 2 - std::to_string(static_cast<uint16_t>(kXmin)).length() -
          std::to_string(static_cast<uint16_t>(kXmax)).length(),
      ' ');
  std::cout << kXmax << '\n';
}

// NOLINTNEXTLINE c-style array
void PrintSolutionTable(const lob::Output solutions[], size_t size) {
  // Column widths for better formatting
  const int kWidth = 12;

  // Print table header
  std::cout << std::left << std::setw(kWidth) << "Yards" << std::setw(kWidth)
            << "FPS" << std::setw(kWidth) << "FtLbs" << std::setw(kWidth)
            << "Elev Inch" << std::setw(kWidth) << "Elev MOA"
            << std::setw(kWidth) << "Wind Inch" << std::setw(kWidth)
            << "Wind MOA" << std::setw(kWidth) << "Seconds"
            << "\n";

  // Print table content
  for (size_t i = 0; i < size; ++i) {
    std::cout << std::left << std::setw(kWidth) << solutions[i].range
              << std::setw(kWidth) << solutions[i].velocity << std::setw(kWidth)
              << solutions[i].energy << std::setw(kWidth) << std::fixed
              << std::setprecision(2) << solutions[i].elevation
              << std::setw(kWidth)
              << lob::InchToMoa(solutions[i].elevation, solutions[i].range)
              << std::setw(kWidth) << solutions[i].deflection
              << std::setw(kWidth)
              << lob::InchToMoa(solutions[i].deflection, solutions[i].range)
              << std::setw(kWidth) << std::setprecision(3)
              << solutions[i].time_of_flight << "\n";
  }
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

  lob::Builder builder = example::BuildHelper(input_file, output_file);
  const auto kInput = builder.Build();

  constexpr size_t kSolutionSize = 12;
  constexpr std::array<uint32_t, kSolutionSize> kRanges = {
      0, 50, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000};
  std::array<lob::Output, kSolutionSize> solutions = {};
  const auto kSize = lob::Solve(kInput, &kRanges, &solutions);

  example::PlotSolution(solutions.data(), kSize);
  std::cout << '\n';
  example::PrintSolutionTable(solutions.data(), kSize);
  return 0;
}

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.