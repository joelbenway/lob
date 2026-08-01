# Copyright (c) 2025  Joel Benway
# SPDX-License-Identifier: GPL-3.0-or-later
# Please see end of file for extended copyright information

# add_cachegrind_benchmark(); include from benchmark/cachegrind/CMakeLists.txt.

find_program(VALGRIND_PROGRAM valgrind)

# CMAKE_BINARY_DIR: PROJECT_BINARY_DIR here is benchmark/'s (it calls project()).
set(CACHEGRIND_OUTPUT_DIR "${CMAKE_BINARY_DIR}/cachegrind"
    CACHE PATH "Where cachegrind results land")

if(NOT TARGET cachegrind-bench)
  add_custom_target(cachegrind-bench
                    COMMENT "Run all instruction-count benchmarks")
endif()

# add_cachegrind_benchmark(<name> SOURCES <files...> CASES <case>:<reps>...)
function(add_cachegrind_benchmark NAME)
  cmake_parse_arguments(CG "" "" "SOURCES;CASES" ${ARGN})

  add_executable("${NAME}" ${CG_SOURCES})
  target_link_libraries("${NAME}" PRIVATE lob::lob)
  target_compile_features("${NAME}" PRIVATE cxx_std_14)
  target_include_directories("${NAME}"
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../source")

  # -g gives cg_annotate line attribution at no runtime cost.
  target_compile_options("${NAME}" PRIVATE -g)

  if(NOT VALGRIND_PROGRAM)
    message(STATUS "cachegrind: valgrind missing; cg_${NAME} not runnable")
    return()
  endif()

  # Comma-join: a semicolon list would be re-split into separate COMMAND args.
  string(REPLACE ";" "," _cases_csv "${CG_CASES}")

  add_custom_target("cg_${NAME}"
    COMMAND "${CMAKE_COMMAND}"
            -D "CG_EXE=$<TARGET_FILE:${NAME}>"
            -D "CG_NAME=${NAME}"
            -D "CG_OUTDIR=${CACHEGRIND_OUTPUT_DIR}"
            -D "CG_VALGRIND=${VALGRIND_PROGRAM}"
            -D "CG_CASES=${_cases_csv}"
            -P "${CMAKE_SOURCE_DIR}/cmake/cachegrind-run.cmake"
    DEPENDS "${NAME}"
    USES_TERMINAL VERBATIM
    COMMENT "cachegrind: ${NAME}")

  add_dependencies(cachegrind-bench "cg_${NAME}")
endfunction()

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