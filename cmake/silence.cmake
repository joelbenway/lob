# Copyright (c) 2025  Joel Benway
# SPDX-License-Identifier: GPL-3.0-or-later
# Please see end of file for extended copyright information

# Use this function to temporarily silence warnings for a target. This is useful
# for third-party libraries that we don't want to modify.
function(silence target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W0 /WX-)
  else()
    target_compile_options(${target} PRIVATE -w -Wno-error)
  endif()
  set_target_properties(${target} PROPERTIES CXX_CLANG_TIDY "")
  set_target_properties(${target} PROPERTIES CXX_CPPCHECK "")
  set_target_properties(${target} PROPERTIES CXX_CPPLINT "")
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