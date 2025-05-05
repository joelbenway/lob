# Copyright (c) 2025  Joel Benway
# SPDX-License-Identifier: GPL-3.0-or-later
# Please see end of file for extended copyright information

cmake_minimum_required(VERSION 3.14)

macro(default name)
  if(NOT DEFINED "${name}")
    set("${name}" "${ARGN}")
  endif()
endmacro()

default(FORMAT_COMMAND clang-format)
default(
  PATTERNS
  source/*.cpp
  source/*.hpp
  include/*.hpp
  test/*.cpp
  test/*.hpp
  example/*.cpp
  example/*.hpp)
default(FIX NO)

set(flag --output-replacements-xml)
set(args OUTPUT_VARIABLE output)
if(FIX)
  set(flag -i)
  set(args "")
endif()

file(GLOB_RECURSE files ${PATTERNS})
set(badly_formatted "")
set(output "")
string(LENGTH "${CMAKE_SOURCE_DIR}/" path_prefix_length)

foreach(file IN LISTS files)
  execute_process(
    COMMAND "${FORMAT_COMMAND}" --style=file "${flag}" "${file}"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    RESULT_VARIABLE result ${args})
  if(NOT result EQUAL "0")
    message(FATAL_ERROR "'${file}': formatter returned with ${result}")
  endif()
  if(NOT FIX AND output MATCHES "\n<replacement offset")
    string(SUBSTRING "${file}" "${path_prefix_length}" -1 relative_file)
    list(APPEND badly_formatted "${relative_file}")
  endif()
  set(output "")
endforeach()

if(NOT badly_formatted STREQUAL "")
  list(JOIN badly_formatted "\n" bad_list)
  message("The following files are badly formatted:\n\n${bad_list}\n")
  message(FATAL_ERROR "Run again with FIX=YES to fix these files.")
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
# lob. If not, see <https:#www.gnu.org/licenses/>.