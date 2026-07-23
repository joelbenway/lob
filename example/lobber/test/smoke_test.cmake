# Copyright (c) 2025  Joel Benway
# SPDX-License-Identifier: GPL-3.0-or-later
# Please see end of file for extended copyright information

# --version
execute_process(COMMAND "${LOBBER}" --version
  RESULT_VARIABLE RES OUTPUT_VARIABLE OUT ERROR_QUIET
  OUTPUT_STRIP_TRAILING_WHITESPACE)
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
execute_process(COMMAND "${LOBBER}" --help
  RESULT_VARIABLE RES OUTPUT_VARIABLE OUT ERROR_QUIET
  OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT RES EQUAL 0)
  message(FATAL_ERROR "--help exited ${RES}")
endif()
string(REGEX MATCH "Usage:" FOUND "${OUT}")
if(NOT FOUND)
  message(FATAL_ERROR "--help missing 'Usage:'")
endif()
message(STATUS "PASS: --help")

# --json with fixture
execute_process(COMMAND "${LOBBER}" --json
  INPUT_FILE "${FIXTURE}"
  RESULT_VARIABLE RES OUTPUT_VARIABLE OUT ERROR_QUIET
  OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT RES EQUAL 0)
  message(FATAL_ERROR "--json exited ${RES}")
endif()
string(REGEX MATCH "^\\[.*\\]$" FOUND "${OUT}")
if(NOT FOUND)
  message(FATAL_ERROR "--json output not a JSON array")
endif()
foreach(FIELD range velocity energy elevation deflection time_of_flight)
  string(REGEX MATCH "\"${FIELD}\"" FOUND "${OUT}")
  if(NOT FOUND)
    message(FATAL_ERROR "--json output missing field '${FIELD}'")
  endif()
endforeach()
message(STATUS "PASS: --json")
