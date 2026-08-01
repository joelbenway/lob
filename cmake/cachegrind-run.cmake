# Copyright (c) 2025  Joel Benway
# SPDX-License-Identifier: GPL-3.0-or-later
# Please see end of file for extended copyright information

# Script-mode runner: measure a binary under Cachegrind, parse the raw output,
# emit machine-readable results plus JSON for github-action-benchmark.
# Usage (-D must precede -P):
#   cmake -D CG_EXE=<binary> -D CG_NAME=<name> -D CG_OUTDIR=<dir>
#         -D CG_VALGRIND=<valgrind> -D CG_CASES=case:reps,... -P cachegrind-run.cmake

cmake_minimum_required(VERSION 3.14)

include("${CMAKE_CURRENT_LIST_DIR}/cachegrind-common.cmake")

foreach(_req CG_EXE CG_NAME CG_OUTDIR CG_VALGRIND CG_CASES)
  if(NOT DEFINED ${_req})
    message(FATAL_ERROR "missing required -D ${_req}=<value>")
  endif()
endforeach()

set(CG_REPS_WIDTH 12)
file(MAKE_DIRECTORY "${CG_OUTDIR}")

# --- ASLR prefix: setarch -R (probe bare and <arch> forms; old util-linux needs the arch).
set(CG_SETARCH_PREFIX "")
find_program(CG_SETARCH setarch)
if(CG_SETARCH)
  execute_process(COMMAND "${CG_SETARCH}" -R true
                  OUTPUT_QUIET ERROR_QUIET RESULT_VARIABLE _r)
  if(_r EQUAL 0)
    set(CG_SETARCH_PREFIX "${CG_SETARCH};-R")
  else()
    execute_process(COMMAND "${CG_SETARCH}" "${CMAKE_HOST_SYSTEM_PROCESSOR}" -R true
                    OUTPUT_QUIET ERROR_QUIET RESULT_VARIABLE _r2)
    if(_r2 EQUAL 0)
      set(CG_SETARCH_PREFIX "${CG_SETARCH};${CMAKE_HOST_SYSTEM_PROCESSOR};-R")
    else()
      message(WARNING "setarch found but -R unsupported; counts may vary with ASLR")
    endif()
  endif()
else()
  message(WARNING "setarch not found; counts may vary with ASLR")
endif()

# --- Scrubbed env via `env -i` (execute_process can't wipe the block); VALGRIND_LIB passthrough for non-FHS installs.
find_program(CG_ENV env)
if(NOT CG_ENV)
  message(FATAL_ERROR "coreutils 'env' not found; cannot build a scrubbed environment")
endif()
set(CG_ENV_PREFIX "${CG_ENV};-i;PATH=/usr/bin:/bin;LC_ALL=C;LANG=C;LD_BIND_NOW=1;HOME=/nonexistent")
if(DEFINED ENV{VALGRIND_LIB})
  list(APPEND CG_ENV_PREFIX "VALGRIND_LIB=$ENV{VALGRIND_LIB}")
endif()
if(DEFINED ENV{VALGRIND_LIB_INNER})
  list(APPEND CG_ENV_PREFIX "VALGRIND_LIB_INNER=$ENV{VALGRIND_LIB_INNER}")
endif()

# --- Parse raw 'events:'/'summary:' lines (stable across Valgrind releases; cg_annotate's format is not).
function(cg_parse_ir OUT FILE)
  file(STRINGS "${FILE}" _events_lines REGEX "^events:")
  file(STRINGS "${FILE}" _summary_lines REGEX "^summary:")
  if(NOT _events_lines)
    message(FATAL_ERROR "could not parse events from ${FILE}")
  endif()
  if(NOT _summary_lines)
    message(FATAL_ERROR "could not parse summary from ${FILE}")
  endif()
  string(REPLACE "events:" "" _ev "${_events_lines}")
  string(REPLACE "summary:" "" _sum "${_summary_lines}")
  string(STRIP "${_ev}" _ev)
  string(STRIP "${_sum}" _sum)
  separate_arguments(_events UNIX_COMMAND "${_ev}")
  separate_arguments(_values UNIX_COMMAND "${_sum}")
  list(FIND _events "Ir" _idx)
  if(_idx EQUAL -1)
    message(FATAL_ERROR "no 'Ir' column in events line of ${FILE}")
  endif()
  list(GET _values ${_idx} _ir)
  set(${OUT} "${_ir}" PARENT_SCOPE)
endfunction()

# --- One measured run: pad reps so argv (and stack layout) is byte-identical across passes.
function(cg_run_once OUT_IR OUT_FILE CASE REPS)
  cg_pad(_reps_padded "${REPS}" ${CG_REPS_WIDTH})
  set(_out_file "${CG_OUTDIR}/cg.${CASE}.${_reps_padded}.out")
  set(_cmd "${CG_ENV_PREFIX};${CG_SETARCH_PREFIX};${CG_VALGRIND}")
  list(APPEND _cmd "--tool=cachegrind" "--cache-sim=no" "--branch-sim=no"
                   "--cachegrind-out-file=${_out_file}" "--" "${CG_EXE}"
                   "${CASE}" "${_reps_padded}")
  string(REPLACE ";" " " _dbg "${_cmd}")
  message(STATUS "cachegrind: ${_dbg}")
  execute_process(COMMAND ${_cmd}
                  RESULT_VARIABLE _rc
                  OUTPUT_VARIABLE _stdout ERROR_VARIABLE _stderr)
  if(NOT _rc EQUAL 0)
    message("${_stdout}")
    message("${_stderr}")
    message(FATAL_ERROR "valgrind exited ${_rc} for case '${CASE}' reps ${REPS}")
  endif()
  cg_parse_ir(_ir "${_out_file}")
  set(${OUT_IR} "${_ir}" PARENT_SCOPE)
  set(${OUT_FILE} "${_out_file}" PARENT_SCOPE)
endfunction()

# --- Main loop: two-run subtraction cancels startup/teardown; a non-positive delta means the optimizer ate the loop.
string(REPLACE "," ";" CG_CASE_LIST "${CG_CASES}")

set(CG_RESULT_SLUGS "")
foreach(_spec ${CG_CASE_LIST})
  if(NOT _spec MATCHES "^([^:]+):([0-9]+)$")
    message(FATAL_ERROR "malformed case spec: '${_spec}' (expected name:reps)")
  endif()
  set(_case "${CMAKE_MATCH_1}")
  set(_reps "${CMAKE_MATCH_2}")

  cg_run_once(_ir_base _base_file "${_case}" 0)
  cg_run_once(_ir_meas _meas_file "${_case}" "${_reps}")

  math(EXPR _delta "${_ir_meas} - ${_ir_base}")
  if(_delta LESS_EQUAL 0)
    message(FATAL_ERROR
      "non-positive delta (${_delta}) for case '${_case}': loop likely optimized \
away -- check the volatile sink in the harness")
  endif()

  math(EXPR _per_op_milli "(${_delta} * 1000) / ${_reps}")
  math(EXPR _signal_bp "(${_delta} * 10000) / ${_ir_meas}")
  if(_signal_bp LESS 9000)
    message(WARNING
      "low signal ratio for '${_case}' (${_signal_bp} bp < 9000 bp): raise its reps")
  endif()

  cg_slug(_slug "${_case}")
  list(APPEND CG_RESULT_SLUGS "${_slug}")
  set(CG_SLUG_${_slug} "${_case}")
  set(CG_IR_PER_OP_MILLI_${_slug} "${_per_op_milli}")
  set(CG_IR_DELTA_${_slug} "${_delta}")
  set(CG_REPS_${_slug} "${_reps}")

  cg_milli_to_string(_per_op_str "${_per_op_milli}")
  message(STATUS "${_case}: ${_per_op_str} Ir/op (delta ${_delta} over ${_reps} reps)")
endforeach()

# --- Emit results.cmake (set() file, since string(JSON) needs CMake 3.19+).
set(_results_path "${CG_OUTDIR}/${CG_NAME}.results.cmake")
set(_results "")
string(APPEND _results "set(CG_RESULT_SLUGS \"${CG_RESULT_SLUGS}\")\n")
foreach(_slug ${CG_RESULT_SLUGS})
  string(APPEND _results "set(CG_SLUG_${_slug} \"${CG_SLUG_${_slug}}\")\n")
  string(APPEND _results "set(CG_IR_PER_OP_MILLI_${_slug} ${CG_IR_PER_OP_MILLI_${_slug}})\n")
  string(APPEND _results "set(CG_IR_DELTA_${_slug} ${CG_IR_DELTA_${_slug}})\n")
  string(APPEND _results "set(CG_REPS_${_slug} ${CG_REPS_${_slug}})\n")
endforeach()
file(WRITE "${_results_path}" "${_results}")

# --- Emit JSON for github-action-benchmark (customSmallerIsBetter shape).
set(_json "[\n")
set(_first TRUE)
foreach(_slug ${CG_RESULT_SLUGS})
  if(NOT _first)
    string(APPEND _json ",\n")
  endif()
  set(_first FALSE)
  cg_milli_to_string(_val "${CG_IR_PER_OP_MILLI_${_slug}}")
  string(APPEND _json "  {\"name\": \"${CG_NAME}/${CG_SLUG_${_slug}}\", \"unit\": \"Ir/op\", \"value\": ${_val}}")
endforeach()
string(APPEND _json "\n]\n")
file(WRITE "${CG_OUTDIR}/${CG_NAME}.json" "${_json}")

message(STATUS "cachegrind: wrote ${CG_OUTDIR}/${CG_NAME}.results.cmake and ${CG_NAME}.json")

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
