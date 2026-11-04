// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include <cstdint>
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

namespace {

constexpr int kIndent = 4;

bool LoadJsonInput(nlohmann::json* json) {
  if (example::IsInteractive()) {
    *json = example::RunWizard();
    return !json->empty();
  }

  if (std::cin.peek() == std::char_traits<char>::eof()) {
    return false;
  }

  try {
    std::cin >> *json;
    return !json->empty();
  } catch (const nlohmann::json::parse_error& e) {
    std::cerr << "\033[31mError parsing JSON: \033[0m" << e.what() << "\n";
    return false;
  }
}

bool SaveJsonInput(const std::string& path, const nlohmann::json& json) {
  std::ofstream file(path);
  if (!file.is_open()) {
    std::cerr << "\033[31mError: Could not open save file!\033[0m\n";
    return false;
  }
  file << json.dump(kIndent);
  return true;
}

example::SolverPath ResolveSolverPath(example::SolverPath cli_path,
                                      const nlohmann::json& json) {
  if (cli_path != example::SolverPath::kAuto) {
    return cli_path;
  }
  const auto kJsonPath = example::JsonToSolverPath(json);
  return (kJsonPath != example::SolverPath::kAuto) ? kJsonPath
                                                   : example::SolverPath::kFast;
}

example::SolverMode ResolveSolverMode(example::SolverMode cli_mode,
                                      const nlohmann::json& json) {
  if (cli_mode != example::SolverMode::kAuto) {
    return cli_mode;
  }
  const auto kJsonMode = example::JsonToSolverMode(json);
  return (kJsonMode != example::SolverMode::kAuto)
             ? kJsonMode
             : example::SolverMode::kForward;
}

example::AngularUnit ResolveAngularUnit(example::AngularUnit cli_unit,
                                        const nlohmann::json& json) {
  if (cli_unit != example::AngularUnit::kMoa) {
    return cli_unit;
  }
  if (json.contains("AngularUnit") || json.contains("angularUnit")) {
    return example::JsonToAngularUnit(json);
  }
  return cli_unit;
}

size_t SolveTrajectory(example::SolverPath path, example::SolverMode mode,
                       const lob::Context& ctx,
                       const std::vector<uint32_t>& ranges,
                       std::vector<lob::Output>* outputs) {
  if (path == example::SolverPath::kELR) {
    if (mode == example::SolverMode::kInverse) {
      return lob::SolveInverseELR(ctx, ranges.data(), outputs->data(),
                                  ranges.size());
    }
    return lob::SolveELR(ctx, ranges.data(), outputs->data(), ranges.size());
  }

  if (mode == example::SolverMode::kInverse) {
    return lob::SolveInverse(ctx, ranges.data(), outputs->data(),
                             ranges.size());
  }
  return lob::Solve(ctx, ranges.data(), outputs->data(), ranges.size());
}

void EmitResults(const example::CliConfig& config, example::SolverMode mode,
                 example::AngularUnit unit, const lob::Context& ctx,
                 const std::vector<lob::Output>& outputs, size_t count,
                 size_t total_ranges) {
  if (config.json_mode) {
    const auto kJout =
        example::OutputsToJson(outputs.data(), count, mode, unit);
    std::cout << kJout.dump(kIndent) << "\n";
  } else {
    example::PrintTable(ctx, outputs.data(), count, total_ranges, mode, unit);
    example::PrintGH();
  }
}

}  // namespace

int main(int argc, char* argv[]) try {
  const auto kConfig = example::ParseArgs(argc, argv);

  if (kConfig.parse_error) {
    return 1;
  }

  if (kConfig.show_help) {
    example::PrintHelp();
    return 0;
  }

  if (kConfig.show_version) {
    example::PrintVersion();
    return 0;
  }

  nlohmann::json json;
  if (!LoadJsonInput(&json)) {
    if (example::IsInteractive()) {
      return 0;
    }
    std::cerr << "\033[31mError: No ctx data provided.\033[0m\n\n";
    example::PrintHelp();
    return 1;
  }

  if (kConfig.has_save_input_path &&
      !SaveJsonInput(kConfig.save_input_path, json)) {
    return 1;
  }

  const auto kCtx = example::BuildContextFromJson(json);
  if (kCtx.error != lob::ErrorT::kNone) {
    std::cerr << "\033[31mBuilder error: " << example::ErrorToString(kCtx.error)
              << " (code " << static_cast<int>(kCtx.error) << ")\033[0m\n";
    return 1;
  }

  const auto kPath = ResolveSolverPath(kConfig.solver_path, json);
  const auto kMode = ResolveSolverMode(kConfig.solver_mode, json);
  const auto kUnit = ResolveAngularUnit(kConfig.angular_unit, json);

  const auto kRanges = example::ParseRanges(json);
  if (kRanges.empty()) {
    std::cerr << "\033[31mError: No ranges to solve for\033[0m\n";
    return 1;
  }

  std::vector<lob::Output> outputs(kRanges.size());
  const size_t kCount = SolveTrajectory(kPath, kMode, kCtx, kRanges, &outputs);

  if (kCount == 0) {
    std::cerr << "\033[31mSolver error: no solutions returned\033[0m\n";
    return 1;
  }

  EmitResults(kConfig, kMode, kUnit, kCtx, outputs, kCount, kRanges.size());
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
