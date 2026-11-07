# Copyright (c) 2026  Joel Benway
# SPDX-License-Identifier: GPL-3.0-or-later
# Please see end of file for extended copyright information

# Provides the nlohmann_json::nlohmann_json target, preferring a system
# package and falling back to FetchContent. Include from any directory scope
# that links the target instead of repeating this block.
include(${CMAKE_CURRENT_LIST_DIR}/find-or-fetch.cmake)

lob_find_or_fetch(
  nlohmann_json
  PACKAGE nlohmann_json
  DISPLAY_NAME "nlohmann/json"
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.12.0
  FETCH_NAME json)

# This file is part of lob.
#
# lob is free software: you can redistribute it and/or modify it under the
# terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# lob is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
# A PARTICULAR PURPOSE. See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with
# lob. If not, see <https://www.gnu.org/licenses/>.
