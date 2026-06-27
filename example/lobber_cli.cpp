// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include "lobber_cli.hpp"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

#include "lob/lob.hpp"
#include "version.hpp"

namespace example {
namespace {

constexpr const char* kHelp = "--help";
constexpr const char* kH = "--h";
constexpr const char* kVersion = "--version";
constexpr const char* kV = "--v";
constexpr const char* kJson = "--json";
constexpr const char* kSaveInput = "--save-input=";

}  // namespace

CliConfig ParseArgs(int argc, char* argv[]) {  // NOLINT
  CliConfig config;
  for (int i = 1; i < argc; ++i) {
    const std::string kArg(argv[i]);
    if (kArg == kHelp || kArg == kH) {
      config.show_help = true;
    } else if (kArg == kVersion || kArg == kV) {
      config.show_version = true;
    } else if (kArg == kJson) {
      config.json_mode = true;
    } else if (kArg.compare(0, std::strlen(kSaveInput), kSaveInput) == 0) {
      config.has_save_input_path = true;
      config.save_input_path = kArg.substr(std::strlen(kSaveInput));
    }
  }
  return config;
}

void PrintHelp() {
  std::cout
      << "Usage: lobber [options] [< input.json]\n"
      << "Options:\n"
      << "  --h, --help            Show this help message\n"
      << "  --v, --version         Show version information\n"
      << "  --json                 Output results to stdout in json format\n"
      << "  --save-input=FILE      Save input configuration JSON to FILE\n"
      << "\n"
      << "Note: When run interactively, a wizard prompts for input.\n"
      << "      When stdin is redirected, JSON data is read from stdin.\n"
      << "Example:\n"
      << "\033[33m  lobber --save-input=my_rifle_load.json\n\033[0m"
      << "\n";
  PrintGH();
}

void PrintGH() {
  std::cout << "Report bugs or give feedback here: "
            << "\033[34mhttps://github.com/joelbenway/lob\033[0m\n";
}

void PrintVersion() {
  std::cout << "Lobber version: " << kProjectVersion << "\n"
            << "Lob version:    " << lob::Version() << "\n\n";
  PrintGH();
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
