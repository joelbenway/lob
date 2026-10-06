# Copyright (c) 2026  Joel Benway
# SPDX-License-Identifier: GPL-3.0-or-later
# Please see end of file for extended copyright information

# Provides the nlohmann_json::nlohmann_json target, preferring a system
# package and falling back to FetchContent. Include from any directory scope
# that links the target instead of repeating this block.
find_package(nlohmann_json CONFIG)

if(NOT nlohmann_json_FOUND)
  message(STATUS "nlohmann/json not found, FetchContent instead...")
  set(DEV_WARNINGS_RECOVER_STATE "$CACHE{CMAKE_SUPPRESS_DEVELOPER_WARNINGS}")
  set(DEV_ERRORS_RECOVER_STATE "$CACHE{CMAKE_SUPPRESS_DEVELOPER_ERRORS}")
  set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS
      ON
      CACHE INTERNAL "Suppress third-party CMake warnings" FORCE)
  set(CMAKE_SUPPRESS_DEVELOPER_ERRORS
      ON
      CACHE INTERNAL "Suppress third-party CMake errors" FORCE)

  include(FetchContent)
  FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.12.0)
  FetchContent_MakeAvailable(json)

  set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS
      ${DEV_WARNINGS_RECOVER_STATE}
      CACHE INTERNAL "Restore original setting" FORCE)
  set(CMAKE_SUPPRESS_DEVELOPER_ERRORS
      ${DEV_ERRORS_RECOVER_STATE}
      CACHE INTERNAL "Restore original setting" FORCE)
  unset(DEV_WARNINGS_RECOVER_STATE)
  unset(DEV_ERRORS_RECOVER_STATE)
else()
  message(STATUS "Found nlohmann/json: ${nlohmann_json_DIR}")
endif()

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
