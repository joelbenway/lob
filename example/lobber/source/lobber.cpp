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

  if (config.parse_error) {
    return 1;
  }

  if (config.show_help) {
    example::PrintHelp();
    return 0;
  }

  if (config.show_version) {
    example::PrintVersion();
    return 0;
  }

  nlohmann::json json;

  // Read ctx from stdin or wizard
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
    std::cerr << "\033[31mError: No ctx data provided.\033[0m\n\n";
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

  // Build Context via lobber_bridge helper
  auto ctx = example::BuildContextFromJson(json);

  if (ctx.error != lob::ErrorT::kNone) {
    std::cerr << "\033[31mBuilder error: " << example::ErrorToString(ctx.error)
              << " (code " << static_cast<int>(ctx.error) << ")\033[0m\n";
    return 1;
  }

  // Resolve solver path, mode, and unit (CLI overrides JSON configuration)
  auto path = config.solver_path;
  if (path == example::SolverPath::kAuto) {
    path = example::JsonToSolverPath(json);
  }
  if (path == example::SolverPath::kAuto) {
    path = example::SolverPath::kFast;
  }

  auto mode = config.solver_mode;
  if (mode == example::SolverMode::kAuto) {
    mode = example::JsonToSolverMode(json);
  }
  if (mode == example::SolverMode::kAuto) {
    mode = example::SolverMode::kForward;
  }

  auto unit = config.angular_unit;
  if (unit == example::AngularUnit::kMoa &&
      (json.contains("AngularUnit") || json.contains("angularUnit"))) {
    unit = example::JsonToAngularUnit(json);
  }

  // Solve
  auto ranges = example::ParseRanges(json);
  if (ranges.empty()) {
    std::cerr << "\033[31mError: No ranges to solve for\033[0m\n";
    return 1;
  }

  std::vector<lob::Output> outputs(ranges.size());
  size_t count = 0;
  if (path == example::SolverPath::kELR) {
    if (mode == example::SolverMode::kInverse) {
      count = lob::SolveInverseELR(ctx, ranges.data(), outputs.data(),
                                   ranges.size());
    } else {
      count = lob::SolveELR(ctx, ranges.data(), outputs.data(), ranges.size());
    }
  } else {
    if (mode == example::SolverMode::kInverse) {
      count =
          lob::SolveInverse(ctx, ranges.data(), outputs.data(), ranges.size());
    } else {
      count = lob::Solve(ctx, ranges.data(), outputs.data(), ranges.size());
    }
  }

  if (count == 0) {
    std::cerr << "\033[31mSolver error: no solutions returned\033[0m\n";
    return 1;
  }

  if (config.json_mode) {
    auto jout = example::OutputsToJson(outputs.data(), count, mode, unit);
    std::cout << jout.dump(4) << "\n";
  } else {
    example::PrintTable(ctx, outputs.data(), count, ranges.size(), mode, unit);
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
