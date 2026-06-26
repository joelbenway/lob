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
  if (config.save_input_path.has_value()) {
    std::ofstream file(*config.save_input_path);
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
    auto jout = example::OutputsToJson(result.outputs.data(), result.outputs.size());
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
