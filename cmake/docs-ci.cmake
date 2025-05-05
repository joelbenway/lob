# Copyright (c) 2025  Joel Benway
# SPDX-License-Identifier: GPL-3.0-or-later
# Please see end of file for extended copyright information

cmake_minimum_required(VERSION 3.14)

foreach(var IN ITEMS PROJECT_BINARY_DIR PROJECT_SOURCE_DIR)
  if(NOT DEFINED "${var}")
    message(FATAL_ERROR "${var} must be defined")
  endif()
endforeach()
set(bin "${PROJECT_BINARY_DIR}")
set(src "${PROJECT_SOURCE_DIR}")

# ---- Dependencies ----

find_program(DOXYGEN_EXECUTABLE NAMES doxygen)
if(NOT DOXYGEN_EXECUTABLE)
  message(FATAL_ERROR "Doxygen executable was not found")
endif()

# ---- Process project() call in CMakeLists.txt ----

file(READ "${src}/CMakeLists.txt" content)

string(FIND "${content}" "project(" index)
if(index EQUAL "-1")
  message(FATAL_ERROR "Could not find \"project(\"")
endif()
string(SUBSTRING "${content}" "${index}" -1 content)

string(FIND "${content}" ")\n" index)
if(index EQUAL "-1")
  message(FATAL_ERROR "Could not find \"\)\\n\"")
endif()
string(SUBSTRING "${content}" 0 "${index}" content)

file(WRITE "${bin}/docs-ci.project.cmake" "docs_${content}\n)\n")

macro(list_pop_front list out)
  list(GET "${list}" 0 "${out}")
  list(REMOVE_AT "${list}" 0)
endmacro()

function(docs_project name)
  cmake_parse_arguments(PARSE_ARGV 1 "" "" "VERSION;DESCRIPTION;HOMEPAGE_URL"
                        LANGUAGES)
  set(PROJECT_NAME
      "${name}"
      PARENT_SCOPE)
  if(DEFINED _VERSION)
    set(PROJECT_VERSION
        "${_VERSION}"
        PARENT_SCOPE)
    string(REGEX MATCH "^.*" is_version "${_VERSION}")
    if(is_version)
      set(versions "${_VERSION}")
      string(REPLACE "." ";" versions "${versions}")
      set(suffixes MAJOR MINOR PATCH TWEAK)
      while(NOT versions STREQUAL "" AND NOT suffixes STREQUAL "")
        list_pop_front(versions version)
        list_pop_front(suffixes suffix)
        set("PROJECT_VERSION_${suffix}"
            "${version}"
            PARENT_SCOPE)
      endwhile()
    else()
      message(
        WARNING "Project version '${_VERSION}' does not match expected format.")
    endif()
  endif()
  if(DEFINED _DESCRIPTION)
    set(PROJECT_DESCRIPTION
        "${_DESCRIPTION}"
        PARENT_SCOPE)
  endif()
  if(DEFINED _HOMEPAGE_URL)
    set(PROJECT_HOMEPAGE_URL
        "${_HOMEPAGE_URL}"
        PARENT_SCOPE)
  endif()
endfunction()

include("${bin}/docs-ci.project.cmake")

# ---- Generate docs ----

if(NOT DEFINED DOXYGEN_OUTPUT_DIRECTORY)
  set(DOXYGEN_OUTPUT_DIRECTORY "${bin}/docs")
endif()
set(out "${DOXYGEN_OUTPUT_DIRECTORY}")
set(doxyfile "${bin}/docs/Doxyfile")

configure_file("${src}/docs/Doxyfile.in" "${doxyfile}" @ONLY)

file(REMOVE_RECURSE "${out}/html")

execute_process(
  COMMAND ${DOXYGEN_EXECUTABLE} "${doxyfile}"
  WORKING_DIRECTORY "${bin}/docs"
  RESULT_VARIABLE result)
if(NOT result EQUAL "0")
  message(FATAL_ERROR "Doxygen returned with ${result}")
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