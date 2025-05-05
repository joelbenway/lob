# Copyright (c) 2025  Joel Benway
# SPDX-License-Identifier: GPL-3.0-or-later
# Please see end of file for extended copyright information

set_property(GLOBAL PROPERTY USE_FOLDERS YES)

# Call this function at the end of a directory scope to assign a folder to
# targets created in that directory. Utility targets will be assigned to the
# UtilityTargets folder, otherwise to the ${name}Targets folder. If a target
# already has a folder assigned, then that target will be skipped.
function(add_folders name)
  get_property(
    targets
    DIRECTORY
    PROPERTY BUILDSYSTEM_TARGETS)
  foreach(target IN LISTS targets)
    get_property(
      folder
      TARGET "${target}"
      PROPERTY FOLDER)
    if(DEFINED folder)
      continue()
    endif()
    set(folder Utility)
    get_property(
      type
      TARGET "${target}"
      PROPERTY TYPE)
    if(NOT type STREQUAL "UTILITY")
      set(folder "${name}")
    endif()
    set_property(TARGET "${target}" PROPERTY FOLDER "${folder}Targets")
  endforeach()
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