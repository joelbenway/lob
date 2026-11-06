# Copyright (c) 2026  Joel Benway
# SPDX-License-Identifier: GPL-3.0-or-later
# Please see end of file for extended copyright information

include_guard(GLOBAL)
include(FetchContent)

set(_LOB_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}")

function(lob_find_or_fetch name)
  cmake_parse_arguments(
    ARG
    "FORCE_STATIC"
    "PACKAGE;DISPLAY_NAME;GIT_REPOSITORY;GIT_TAG;FETCH_NAME;PATH_VAR"
    "PATH_NAMES;PATH_SUFFIXES;SILENCE_TARGETS"
    ${ARGN})

  if(NOT ARG_DISPLAY_NAME)
    set(ARG_DISPLAY_NAME "${name}")
  endif()
  if(NOT ARG_FETCH_NAME)
    set(ARG_FETCH_NAME "${name}")
  endif()

  set(_found FALSE)

  if(ARG_PACKAGE)
    find_package(${ARG_PACKAGE} CONFIG QUIET)
    if(${ARG_PACKAGE}_FOUND OR ${name}_FOUND)
      set(_found TRUE)
      set(_dir "${${ARG_PACKAGE}_DIR}")
      if(NOT _dir)
        set(_dir "${${name}_DIR}")
      endif()
      message(STATUS "Found ${ARG_DISPLAY_NAME}: ${_dir}")
    endif()
  elseif(ARG_PATH_VAR)
    find_path(
      ${ARG_PATH_VAR}
      NAMES ${ARG_PATH_NAMES}
      PATH_SUFFIXES ${ARG_PATH_SUFFIXES})
    if(${ARG_PATH_VAR})
      set(_found TRUE)
      set(${ARG_PATH_VAR} "${${ARG_PATH_VAR}}" PARENT_SCOPE)
      message(STATUS "Found ${ARG_DISPLAY_NAME}: ${${ARG_PATH_VAR}}")
    endif()
  endif()

  if(NOT _found)
    message(STATUS "${ARG_DISPLAY_NAME} not found, FetchContent instead...")

    set(DEV_WARNINGS_RECOVER_STATE "$CACHE{CMAKE_SUPPRESS_DEVELOPER_WARNINGS}")
    set(DEV_ERRORS_RECOVER_STATE "$CACHE{CMAKE_SUPPRESS_DEVELOPER_ERRORS}")
    set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS
        ON
        CACHE INTERNAL "Suppress third-party CMake warnings" FORCE)
    set(CMAKE_SUPPRESS_DEVELOPER_ERRORS
        ON
        CACHE INTERNAL "Suppress third-party CMake errors" FORCE)

    FetchContent_Declare(
      ${ARG_FETCH_NAME}
      GIT_REPOSITORY ${ARG_GIT_REPOSITORY}
      GIT_TAG ${ARG_GIT_TAG})

    if(ARG_FORCE_STATIC)
      set(SHARED_LIBS_RECOVER_STATE ${BUILD_SHARED_LIBS})
      set(BUILD_SHARED_LIBS
          OFF
          CACHE BOOL "Force static dependency build" FORCE)
    endif()

    FetchContent_MakeAvailable(${ARG_FETCH_NAME})

    if(ARG_FORCE_STATIC)
      set(BUILD_SHARED_LIBS
          ${SHARED_LIBS_RECOVER_STATE}
          CACHE BOOL "Restore original setting" FORCE)
      unset(SHARED_LIBS_RECOVER_STATE)
    endif()

    set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS
        ${DEV_WARNINGS_RECOVER_STATE}
        CACHE INTERNAL "Restore original setting" FORCE)
    set(CMAKE_SUPPRESS_DEVELOPER_ERRORS
        ${DEV_ERRORS_RECOVER_STATE}
        CACHE INTERNAL "Restore original setting" FORCE)
    unset(DEV_WARNINGS_RECOVER_STATE)
    unset(DEV_ERRORS_RECOVER_STATE)

    if(ARG_SILENCE_TARGETS)
      include(${_LOB_CMAKE_DIR}/silence.cmake)
      foreach(tgt IN LISTS ARG_SILENCE_TARGETS)
        if(TARGET ${tgt})
          silence(${tgt})
        endif()
      endforeach()
    endif()

    if(ARG_PATH_VAR)
      set(${ARG_PATH_VAR}
          "${${ARG_FETCH_NAME}_SOURCE_DIR}"
          CACHE PATH "Path to ${ARG_DISPLAY_NAME} directory" FORCE)
      set(${ARG_PATH_VAR} "${${ARG_FETCH_NAME}_SOURCE_DIR}" PARENT_SCOPE)
    endif()
  endif()
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
