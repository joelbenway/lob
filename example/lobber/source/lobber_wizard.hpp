// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#pragma once

#include <nlohmann/json_fwd.hpp>

namespace example {

/**
 * @brief Runs the interactive 8-phase terminal wizard to build a ballistics
 * configuration JSON.
 * @return Fully populated JSON object representing the user's ballistic
 * configuration.
 */
nlohmann::json RunWizard();

/**
 * @brief Prints the ASCII art banner and introductory welcome message for
 * Lobber.
 */
void PrintGreeting();

/**
 * @brief Determines whether standard input is connected to an interactive
 * terminal.
 * @return True if standard input is an interactive terminal/tty, false if piped
 * or redirected.
 */
bool IsInteractive();

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
