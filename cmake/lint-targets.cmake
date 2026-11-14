# Copyright (c) 2025  Joel Benway
# SPDX-License-Identifier: GPL-3.0-or-later
# Please see end of file for extended copyright information

set(FORMAT_PATTERNS_DEFAULT
    source/*.cpp
    source/*.hpp
    include/*.h
    include/*.hpp
    test/*.cpp
    test/*.hpp
    example/*.cpp
    example/*.hpp
    wasm/*.cpp)

set(FORMAT_PATTERNS_PREV_DEFAULT
    source/*.cpp
    source/*.hpp
    include/*.hpp
    test/*.cpp
    test/*.hpp
    example/*.cpp
    example/*.hpp
    wasm/*.cpp)

if(DEFINED CACHE{FORMAT_PATTERNS})
  if("$CACHE{FORMAT_PATTERNS}" STREQUAL "${FORMAT_PATTERNS_PREV_DEFAULT}")
    set(FORMAT_PATTERNS
        "${FORMAT_PATTERNS_DEFAULT}"
        CACHE STRING
              "; separated patterns relative to the project source dir to format"
              FORCE)
  endif()
endif()

set(FORMAT_PATTERNS
    "${FORMAT_PATTERNS_DEFAULT}"
    CACHE STRING
          "; separated patterns relative to the project source dir to format")

set(FORMAT_COMMAND
    clang-format
    CACHE STRING "Formatter to use")

find_program(NPM_COMMAND NAMES npm npm.cmd)
if(NOT NPM_COMMAND)
  set(NPM_COMMAND npm)
endif()
set(NPM_COMMAND
    "${NPM_COMMAND}"
    CACHE STRING "npm command to use")

add_custom_target(
  format-check
  COMMAND
    "${CMAKE_COMMAND}" -D "FORMAT_COMMAND=${FORMAT_COMMAND}" -D
    "PATTERNS=${FORMAT_PATTERNS}" -P "${PROJECT_SOURCE_DIR}/cmake/lint.cmake"
  COMMAND
    "${NPM_COMMAND}" run format:check
  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
  COMMENT "Linting the code"
  VERBATIM)

add_custom_target(
  format-fix
  COMMAND
    "${CMAKE_COMMAND}" -D "FORMAT_COMMAND=${FORMAT_COMMAND}" -D
    "PATTERNS=${FORMAT_PATTERNS}" -D FIX=YES -P
    "${PROJECT_SOURCE_DIR}/cmake/lint.cmake"
  COMMAND
    "${NPM_COMMAND}" run format
  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
  COMMENT "Fixing the code"
  VERBATIM)

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
