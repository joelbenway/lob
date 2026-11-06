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
file(READ "${FIXTURE}" FIXTURE_CONTENT)
string(FIND "${FIXTURE_CONTENT}" "WindProfile" HAS_WIND)
if(NOT HAS_WIND EQUAL -1)
  string(REPLACE "\"WindProfile\"" "\"DisabledWindProfile\"" NO_WIND_JSON "${FIXTURE_CONTENT}")
  file(WRITE "no_wind_temp.json" "${NO_WIND_JSON}")
  execute_process(
    COMMAND "${LOBBER}" --json
    INPUT_FILE "no_wind_temp.json"
    RESULT_VARIABLE NO_WIND_RES
    OUTPUT_VARIABLE NO_WIND_OUT
    ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
  file(REMOVE "no_wind_temp.json")
  if(NOT NO_WIND_RES EQUAL 0)
    message(FATAL_ERROR "no-wind solve exited ${NO_WIND_RES}")
  endif()
  if("${OUT}" STREQUAL "${NO_WIND_OUT}")
    message(FATAL_ERROR "WindProfile had no effect on trajectory output")
  endif()
endif()
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
set(OUT_FORWARD "${OUT}")
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
set(OUT_INVERSE "${OUT}")
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
string(REGEX MATCH "FtLbs.*Hold MIL.*Hold MOA" FOUND "${OUT}")
if(NOT FOUND)
  message(FATAL_ERROR "--inverse --mil missing ordered 'FtLbs', 'Hold MIL', 'Hold MOA' headers")
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
if("${OUT}" STREQUAL "${OUT_FORWARD}")
  message(FATAL_ERROR "--elr produced identical output to forward solve")
endif()
set(OUT_ELR "${OUT}")
message(STATUS "PASS: --elr solve")

# --inverse --elr mode
execute_process(
  COMMAND "${LOBBER}" --inverse --elr
  INPUT_FILE "${FIXTURE}"
  RESULT_VARIABLE RES
  OUTPUT_VARIABLE OUT
  ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT RES EQUAL 0)
  message(FATAL_ERROR "--inverse --elr exited ${RES}")
endif()
string(REGEX MATCH "Inverse Solution" FOUND "${OUT}")
if(NOT FOUND)
  message(FATAL_ERROR "--inverse --elr missing 'Inverse Solution'")
endif()
string(REGEX MATCH "Hold MOA" FOUND "${OUT}")
if(NOT FOUND)
  message(FATAL_ERROR "--inverse --elr missing 'Hold MOA'")
endif()
if("${OUT}" STREQUAL "${OUT_INVERSE}")
  message(FATAL_ERROR "--inverse --elr produced identical output to --inverse solve")
endif()
if("${OUT}" STREQUAL "${OUT_ELR}")
  message(FATAL_ERROR "--inverse --elr did not distinguish from forward ELR solve")
endif()
message(STATUS "PASS: --inverse --elr solve")

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
