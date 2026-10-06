# Copyright (c) 2025  Joel Benway
# SPDX-License-Identifier: GPL-3.0-or-later
# Please see end of file for extended copyright information

find_package(Threads REQUIRED)

include(${CMAKE_SOURCE_DIR}/cmake/nlohmann-json.cmake)

include(${CMAKE_SOURCE_DIR}/cmake/folders.cmake)

add_executable(lob_mc lob_mc/main.cpp)

# Keep the runner at tools/lob_mc/lob_mc (sources also live in tools/lob_mc/).
set_target_properties(lob_mc PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                        ${CMAKE_CURRENT_BINARY_DIR}/lob_mc)

target_include_directories(lob_mc PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lob_mc)

target_compile_features(lob_mc PRIVATE cxx_std_14)

target_link_libraries(lob_mc PRIVATE lob::lob nlohmann_json::nlohmann_json
                                     Threads::Threads)

execute_process(COMMAND git rev-parse --short=12 HEAD
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" OUTPUT_VARIABLE LOB_GIT_SHA
  OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
if(NOT LOB_GIT_SHA)
  set(LOB_GIT_SHA "unknown")
endif()
target_compile_definitions(lob_mc PRIVATE LOB_GIT_SHA="${LOB_GIT_SHA}")
target_compile_definitions(
  lob_mc PRIVATE LOB_COMPILER="${CMAKE_CXX_COMPILER_ID}-${CMAKE_CXX_COMPILER_VERSION}"
                 LOB_PLATFORM="${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")

add_folders(Tools)

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
