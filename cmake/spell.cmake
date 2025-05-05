# Copyright (c) 2025  Joel Benway
# SPDX-License-Identifier: GPL-3.0-or-later
# Please see end of file for extended copyright information

cmake_minimum_required(VERSION 3.14)

macro(default name)
  if(NOT DEFINED "${name}")
    set("${name}" "${ARGN}")
  endif()
endmacro()

default(SPELL_COMMAND codespell)
default(FIX NO)

set(flag "")
if(FIX)
  set(flag -w)
endif()

execute_process(
  COMMAND "${SPELL_COMMAND}" ${flag}
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  RESULT_VARIABLE result)

if(result EQUAL "65")
  message(FATAL_ERROR "Run again with FIX=YES to fix these errors.")
elseif(result EQUAL "64")
  message(FATAL_ERROR "Spell checker printed the usage info. Bad arguments?")
elseif(NOT result EQUAL "0")
  message(FATAL_ERROR "Spell checker returned with ${result}")
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