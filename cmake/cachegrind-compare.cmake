# Copyright (c) 2025  Joel Benway
# SPDX-License-Identifier: GPL-3.0-or-later
# Please see end of file for extended copyright information

# Script-mode: diff base vs head results and gate on regressions; the exit
# code IS the CI gate.
# Usage (-D must precede -P):
#   cmake -D CG_BASE=<base results.cmake> -D CG_HEAD=<head results.cmake>
#         [-D CG_MAX_REGRESSION_BP=<bp>] [-D CG_THRESHOLD_OVERRIDE_<slug>=<bp>]
#         [-D CG_SUMMARY=<path>] -P cachegrind-compare.cmake

cmake_minimum_required(VERSION 3.14)

include("${CMAKE_CURRENT_LIST_DIR}/cachegrind-common.cmake")

if(NOT DEFINED CG_HEAD)
  message(FATAL_ERROR "missing required -D CG_HEAD=<path>")
endif()
if(NOT DEFINED CG_MAX_REGRESSION_BP)
  set(CG_MAX_REGRESSION_BP 100)
endif()

# Load results inside a function so two files' set()s can't collide; re-export under PREFIX.
function(cg_load PREFIX FILE)
  include("${FILE}")
  set(${PREFIX}_CG_RESULT_SLUGS "${CG_RESULT_SLUGS}" PARENT_SCOPE)
  foreach(_slug ${CG_RESULT_SLUGS})
    foreach(_f SLUG IR_PER_OP_MILLI IR_DELTA REPS)
      set(${PREFIX}_CG_${_f}_${_slug} "${CG_${_f}_${_slug}}" PARENT_SCOPE)
    endforeach()
  endforeach()
endfunction()

# No baseline (first PR or a case rename): record only, exit 0.
if(NOT DEFINED CG_BASE OR NOT EXISTS "${CG_BASE}")
  message(STATUS "no baseline at ${CG_BASE}; recording only")
  cg_load(HEAD "${CG_HEAD}")
  foreach(_slug ${HEAD_CG_RESULT_SLUGS})
    cg_milli_to_string(_h "${HEAD_CG_IR_PER_OP_MILLI_${_slug}}")
    message(STATUS "${HEAD_CG_SLUG_${_slug}}: ${_h} Ir/op (new)")
  endforeach()
  return()
endif()

cg_load(BASE "${CG_BASE}")
cg_load(HEAD "${CG_HEAD}")

set(_failures "")
set(_rows "")

# Head cases: new vs compared; regression in bp (100 bp = 1.00%).
foreach(_slug ${HEAD_CG_RESULT_SLUGS})
  set(_name "${HEAD_CG_SLUG_${_slug}}")
  cg_milli_to_string(_h "${HEAD_CG_IR_PER_OP_MILLI_${_slug}}")
  list(FIND BASE_CG_RESULT_SLUGS "${_slug}" _idx)
  if(_idx EQUAL -1)
    list(APPEND _rows "| \`${_name}\` | -- | ${_h} | new |")
    message(STATUS "${_name}: ${_h} Ir/op (new)")
    continue()
  endif()
  cg_milli_to_string(_b "${BASE_CG_IR_PER_OP_MILLI_${_slug}}")
  if(BASE_CG_IR_PER_OP_MILLI_${_slug} EQUAL 0)
    message(FATAL_ERROR
      "base Ir/op for '${_name}' is zero: baseline corrupt (the runner \
rejects non-positive deltas)")
  endif()
  math(EXPR _delta_bp
       "((${HEAD_CG_IR_PER_OP_MILLI_${_slug}} - ${BASE_CG_IR_PER_OP_MILLI_${_slug}}) * 10000) / ${BASE_CG_IR_PER_OP_MILLI_${_slug}}")
  set(_threshold ${CG_MAX_REGRESSION_BP})
  if(DEFINED CG_THRESHOLD_OVERRIDE_${_slug})
    set(_threshold ${CG_THRESHOLD_OVERRIDE_${_slug}})
  endif()
  set(_mark "⚪")
  math(EXPR _neg_threshold "0 - ${_threshold}")
  if(_delta_bp GREATER _threshold)
    set(_mark "🔴")
    list(APPEND _failures "${_name} (+${_delta_bp} bp > ${_threshold} bp)")
  elseif(_delta_bp LESS _neg_threshold)
    set(_mark "🟢")
  endif()
  cg_bp_to_percent(_chg "${_delta_bp}")
  list(APPEND _rows "| \`${_name}\` | ${_b} | ${_h} | ${_mark} ${_chg}% |")
  message(STATUS "${_name}: ${_h} Ir/op (base ${_b}, ${_chg}%)")
endforeach()

# Base-only cases: a rename shows up here rather than silently passing.
foreach(_slug ${BASE_CG_RESULT_SLUGS})
  list(FIND HEAD_CG_RESULT_SLUGS "${_slug}" _idx)
  if(_idx EQUAL -1)
    list(APPEND _rows "| \`${BASE_CG_SLUG_${_slug}}\` | removed | -- | removed |")
    message(STATUS "${BASE_CG_SLUG_${_slug}}: removed")
  endif()
endforeach()

list(JOIN _rows "\n" _table)
set(_md "### Instruction-count comparison\n\n| benchmark | base Ir/op | head Ir/op | change |\n|---|---|---|---|\n${_table}\n")
message("${_md}")
if(DEFINED CG_SUMMARY AND NOT "${CG_SUMMARY}" STREQUAL "")
  file(APPEND "${CG_SUMMARY}" "${_md}\n")
endif()

if(_failures)
  list(JOIN _failures "\n" _fl)
  message("Investigate the regressions with \`cg_annotate --diff base.out head.out\` \
on the uploaded raw artifacts to find which functions moved.")
  message(FATAL_ERROR "regressions:\n${_fl}")
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
