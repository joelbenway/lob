# Copyright (c) 2025  Joel Benway
# SPDX-License-Identifier: GPL-3.0-or-later
# Please see end of file for extended copyright information

# Shared helpers for the run/compare scripts; no floats in CMake, so values
# are scaled: "milli" = *1000, "bp" (basis points) = *10000.

# Zero-pad VALUE to WIDTH characters.
function(cg_pad OUT VALUE WIDTH)
  set(_s "${VALUE}")
  string(LENGTH "${_s}" _len)
  while(_len LESS WIDTH)
    set(_s "0${_s}")
    string(LENGTH "${_s}" _len)
  endwhile()
  set(${OUT} "${_s}" PARENT_SCOPE)
endfunction()

# Render a milli-unit integer as a decimal string: 12481300 -> "12481.300"
function(cg_milli_to_string OUT MILLI)
  set(_v "${MILLI}")
  set(_sign "")
  if(_v LESS 0)
    set(_sign "-")
    math(EXPR _v "0 - (${_v})")
  endif()
  math(EXPR _whole "${_v} / 1000")
  math(EXPR _frac "${_v} % 1000")
  cg_pad(_frac_s "${_frac}" 3)
  set(${OUT} "${_sign}${_whole}.${_frac_s}" PARENT_SCOPE)
endfunction()

# Render basis points as a signed percentage: -125 -> "-1.25", 250 -> "+2.50"
function(cg_bp_to_percent OUT BP)
  set(_v "${BP}")
  set(_sign "+")
  if(_v LESS 0)
    set(_sign "-")
    math(EXPR _v "0 - (${_v})")
  endif()
  math(EXPR _whole "${_v} / 100")
  math(EXPR _frac "${_v} % 100")
  cg_pad(_frac_s "${_frac}" 2)
  set(${OUT} "${_sign}${_whole}.${_frac_s}" PARENT_SCOPE)
endfunction()

# Sanitize a case name into something legal as a CMake variable suffix.
function(cg_slug OUT NAME)
  string(REGEX REPLACE "[^A-Za-z0-9_]" "_" _s "${NAME}")
  set(${OUT} "${_s}" PARENT_SCOPE)
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
