# Copyright (c) 2025  Joel Benway
# SPDX-License-Identifier: GPL-3.0-or-later
# Please see end of file for extended copyright information

# --version
execute_process(
  COMMAND "${LOBBER}" --version
  RESULT_VARIABLE RES
  OUTPUT_VARIABLE OUT
  ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT RES EQUAL 0)
  message(FATAL_ERROR "--version exited ${RES}")
endif()
string(REGEX MATCH "Lobber version:" FOUND "${OUT}")
if(NOT FOUND)
  message(FATAL_ERROR "--version missing 'Lobber version:'")
endif()
string(REGEX MATCH "Lob version:" FOUND "${OUT}")
if(NOT FOUND)
  message(FATAL_ERROR "--version missing 'Lob version:'")
endif()
message(STATUS "PASS: --version")

# --help
execute_process(
  COMMAND "${LOBBER}" --help
  RESULT_VARIABLE RES
  OUTPUT_VARIABLE OUT
  ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT RES EQUAL 0)
  message(FATAL_ERROR "--help exited ${RES}")
endif()
string(REGEX MATCH "Usage:" FOUND "${OUT}")
if(NOT FOUND)
  message(FATAL_ERROR "--help missing 'Usage:'")
endif()
message(STATUS "PASS: --help")

# --json with fixture
execute_process(
  COMMAND "${LOBBER}" --json
  INPUT_FILE "${FIXTURE}"
  RESULT_VARIABLE RES
  OUTPUT_VARIABLE OUT
  ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT RES EQUAL 0)
  message(FATAL_ERROR "--json exited ${RES}")
endif()
string(REGEX MATCH "^\\[.*\\]$" FOUND "${OUT}")
if(NOT FOUND)
  message(FATAL_ERROR "--json output not a JSON array")
endif()
foreach(
  FIELD
  range velocity energy elevation
  deflection time_of_flight)
  string(REGEX MATCH "\"${FIELD}\"" FOUND "${OUT}")
  if(NOT FOUND)
    message(FATAL_ERROR "--json output missing field '${FIELD}'")
  endif()
endforeach()
message(STATUS "PASS: --json")

# forward table mode
execute_process(
  COMMAND "${LOBBER}"
  INPUT_FILE "${FIXTURE}"
  RESULT_VARIABLE RES
  OUTPUT_VARIABLE OUT
  ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT RES EQUAL 0)
  message(FATAL_ERROR "forward solve exited ${RES}")
endif()
string(REGEX MATCH "Forward Solution" FOUND "${OUT}")
if(NOT FOUND)
  message(FATAL_ERROR "forward solve missing 'Forward Solution'")
endif()
message(STATUS "PASS: forward solve table")

# --inverse mode
execute_process(
  COMMAND "${LOBBER}" --inverse
  INPUT_FILE "${FIXTURE}"
  RESULT_VARIABLE RES
  OUTPUT_VARIABLE OUT
  ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT RES EQUAL 0)
  message(FATAL_ERROR "--inverse exited ${RES}")
endif()
string(REGEX MATCH "Inverse Solution" FOUND "${OUT}")
if(NOT FOUND)
  message(FATAL_ERROR "--inverse missing 'Inverse Solution'")
endif()
string(REGEX MATCH "Hold MOA" FOUND "${OUT}")
if(NOT FOUND)
  message(FATAL_ERROR "--inverse missing 'Hold MOA'")
endif()
message(STATUS "PASS: --inverse solve")

# --inverse --mil mode
execute_process(
  COMMAND "${LOBBER}" --inverse --mil
  INPUT_FILE "${FIXTURE}"
  RESULT_VARIABLE RES
  OUTPUT_VARIABLE OUT
  ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT RES EQUAL 0)
  message(FATAL_ERROR "--inverse --mil exited ${RES}")
endif()
string(REGEX MATCH "Hold MIL" FOUND "${OUT}")
if(NOT FOUND)
  message(FATAL_ERROR "--inverse --mil missing 'Hold MIL'")
endif()
message(STATUS "PASS: --inverse --mil solve")

# --elr mode
execute_process(
  COMMAND "${LOBBER}" --elr
  INPUT_FILE "${FIXTURE}"
  RESULT_VARIABLE RES
  OUTPUT_VARIABLE OUT
  ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT RES EQUAL 0)
  message(FATAL_ERROR "--elr exited ${RES}")
endif()
string(REGEX MATCH "Forward Solution" FOUND "${OUT}")
if(NOT FOUND)
  message(FATAL_ERROR "--elr missing 'Forward Solution'")
endif()
message(STATUS "PASS: --elr solve")

# This file is part of lob.
#
# lob is free software: you can redistribute it and/or modify it under the terms
# of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# lob is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
# A PARTICULAR PURPOSE. See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with
# lob. If not, see <https://www.gnu.org/licenses/>.
