# Copyright (c) 2025  Joel Benway
# SPDX-License-Identifier: GPL-3.0-or-later
# Please see end of file for extended copyright information

cmake_minimum_required(VERSION 3.14)

project(
  lobExamples
  VERSION 1.0.1
  DESCRIPTION "Examples using the lob library"
  HOMEPAGE_URL "https://github.com/joelbenway/lob"
  LANGUAGES CXX)

set(PROJECT_VERSION "${PROJECT_VERSION}")

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/version.hpp.in"
               "${CMAKE_CURRENT_BINARY_DIR}/include/version.hpp")

include(../cmake/project-is-top-level.cmake)
include(../cmake/folders.cmake)

if(PROJECT_IS_TOP_LEVEL)
  find_package(lob REQUIRED)
endif()

add_custom_target(run-examples)

function(add_example NAME)
  add_executable("${NAME}" "${NAME}.cpp")
  target_include_directories("${NAME}"
                             PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/include)
  target_link_libraries("${NAME}" PRIVATE lob::lob)
  target_compile_features("${NAME}" PRIVATE cxx_std_14)
  add_custom_target(
    "run_${NAME}"
    COMMAND "${NAME}"
    VERBATIM)
  add_dependencies("run_${NAME}" "${NAME}")
  add_dependencies(run-examples "run_${NAME}")
endfunction()

add_example(lobber)

add_folders(Example)

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
