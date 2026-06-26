// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#pragma once

#include <cstddef>
#include <nlohmann/json.hpp>
#include <vector>

#include "lob/lob.hpp"

namespace example {

struct BridgeResult {
  lob::Input input;
  std::vector<lob::Output> outputs;
  size_t count;
};

BridgeResult SolveFromJson(const nlohmann::json& j);
void PrintTable(const lob::Input& input, const lob::Output* outputs, size_t count);
nlohmann::json OutputsToJson(const lob::Output* outputs, size_t count);

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
