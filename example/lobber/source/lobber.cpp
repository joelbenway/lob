// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <string>
#include <vector>

#include "lob/lob.hpp"
#include "lobber_bridge.hpp"
#include "lobber_cli.hpp"
#include "lobber_wizard.hpp"

int main(int argc, char* argv[]) try {
  auto config = example::ParseArgs(argc, argv);

  if (config.show_help) {
    example::PrintHelp();
    return 0;
  }

  if (config.show_version) {
    example::PrintVersion();
    return 0;
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

  // Build Context via lob::Builder
  auto input =
      lob::Builder()
          .BallisticCoefficientPsi(
              example::JsonToDouble(json, "BallisticCoefficientPsi"))
          .BCAtmosphere(example::JsonToAtmosphere(json, "BCAtmosphere"))
          .BCDragFunction(example::JsonToDragFunction(json, "BCDragFunction"))
          .DiameterInch(example::JsonToDouble(json, "DiameterInch"))
          .MeplatDiameterInch(example::JsonToDouble(json, "MeplatDiameterInch"))
          .BaseDiameterInch(example::JsonToDouble(json, "BaseDiameterInch"))
          .LengthInch(example::JsonToDouble(json, "LengthInch"))
          .NoseLengthInch(example::JsonToDouble(json, "NoseLengthInch"))
          .TailLengthInch(example::JsonToDouble(json, "TailLengthInch"))
          .OgiveRtR(example::JsonToDouble(json, "OgiveRtR"))
          .MassGrains(example::JsonToDouble(json, "MassGrains"))
          .InitialVelocityFps(example::JsonToU16(json, "InitialVelocityFps"))
          .OpticHeightInches(example::JsonToDouble(json, "OpticHeightInches"))
          .TwistInchesPerTurn(example::JsonToDouble(json, "TwistInchesPerTurn"))
          .ZeroAngleMOA(example::JsonToDouble(json, "ZeroAngleMOA"))
          .ZeroDistanceYds(example::JsonToDouble(json, "ZeroDistanceYds"))
          .ZeroImpactHeightInches(
              example::JsonToDouble(json, "ZeroImpactHeightInches"))
          .AltitudeOfFiringSiteFt(
              example::JsonToDouble(json, "AltitudeOfFiringSiteFt"))
          .AirPressureInHg(example::JsonToDouble(json, "AirPressureInHg"))
          .AltitudeOfBarometerFt(
              example::JsonToDouble(json, "AltitudeOfBarometerFt"))
          .TemperatureDegF(example::JsonToDouble(json, "TemperatureDegF"))
          .AltitudeOfThermometerFt(
              example::JsonToDouble(json, "AltitudeOfThermometerFt"))
          .RelativeHumidityPercent(
              example::JsonToDouble(json, "RelativeHumidityPercent"))
          .WindHeading(example::JsonToClockAngle(json, "WindHeading"))
          .WindSpeedMph(example::JsonToDouble(json, "WindSpeedMph"))
          .AzimuthDeg(example::JsonToDouble(json, "AzimuthDeg"))
          .LatitudeDeg(example::JsonToDouble(json, "LatitudeDeg"))
          .RangeAngleDeg(example::JsonToDouble(json, "RangeAngleDeg"))
          .MinimumSpeed(example::JsonToU16(json, "MinimumSpeed"))
          .MinimumEnergy(example::JsonToU16(json, "MinimumEnergy"))
          .MaximumTime(example::JsonToDouble(json, "MaximumTime"))
          .Build();

  if (input.error != kLobErrorNone) {
    std::cerr << "\033[31mBuilder error: code " << static_cast<int>(input.error)
              << "\033[0m\n";
    return 1;
  }

  // Solve
  auto ranges = example::ParseRanges(json);
  std::vector<lob::Output> outputs(ranges.size());
  const size_t kCount =
      lob::Solve(input, ranges.data(), outputs.data(), ranges.size());

  if (kCount == 0) {
    std::cerr << "\033[31mSolver error: no solutions returned\033[0m\n";
    return 1;
  }

  if (config.json_mode) {
    auto jout = example::OutputsToJson(outputs.data(), kCount);
    std::cout << jout.dump(4) << "\n";
  } else {
    example::PrintTable(input, outputs.data(), kCount);
    example::PrintGH();
  }

  return 0;
} catch (...) {
  std::cerr << "\033[31mUnexpected error\033[0m\n";
  return 1;
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
