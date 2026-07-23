// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#pragma once

#include <string>

namespace example {

struct CliConfig {
  bool show_help = false;
  bool show_version = false;
  bool json_mode = false;
  bool has_save_input_path = false;
  std::string save_input_path;
};

CliConfig ParseArgs(int argc, char* argv[]);  // NOLINT

void PrintHelp();
void PrintVersion();
void PrintGH();

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
