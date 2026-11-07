# Copyright (c) 2026  Joel Benway
# SPDX-License-Identifier: GPL-3.0-or-later
# Please see end of file for extended copyright information

include_guard(GLOBAL)
include(FetchContent)
include(${CMAKE_CURRENT_LIST_DIR}/silence.cmake)

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

    if(DEFINED CACHE{CMAKE_SUPPRESS_DEVELOPER_WARNINGS})
      set(DEV_WARNINGS_RECOVER_STATE
          "$CACHE{CMAKE_SUPPRESS_DEVELOPER_WARNINGS}")
    else()
      set(DEV_WARNINGS_RECOVER_STATE "__UNSET__")
    endif()
    if(DEFINED CACHE{CMAKE_SUPPRESS_DEVELOPER_ERRORS})
      set(DEV_ERRORS_RECOVER_STATE "$CACHE{CMAKE_SUPPRESS_DEVELOPER_ERRORS}")
    else()
      set(DEV_ERRORS_RECOVER_STATE "__UNSET__")
    endif()
    set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS
        ON
        CACHE INTERNAL "Suppress third-party CMake warnings" FORCE)
    set(CMAKE_SUPPRESS_DEVELOPER_ERRORS
        ON
        CACHE INTERNAL "Suppress third-party CMake errors" FORCE)

    if(CMAKE_SCRIPT_MODE_FILE)
      set(_subbuild_dir "${bin}")
      if(NOT _subbuild_dir AND DEFINED PROJECT_BINARY_DIR)
        set(_subbuild_dir "${PROJECT_BINARY_DIR}")
      endif()
      if(NOT _subbuild_dir)
        set(_subbuild_dir "${CMAKE_BINARY_DIR}")
      endif()
      if(NOT _subbuild_dir)
        set(_subbuild_dir "${CMAKE_CURRENT_BINARY_DIR}/_fetch_${ARG_FETCH_NAME}")
      else()
        set(_subbuild_dir "${_subbuild_dir}/_fetch_${ARG_FETCH_NAME}")
      endif()

      file(MAKE_DIRECTORY "${_subbuild_dir}")
      file(
        WRITE "${_subbuild_dir}/CMakeLists.txt"
        "cmake_minimum_required(VERSION 3.14)\n"
        "project(fetch_${ARG_FETCH_NAME} NONE)\n"
        "include(FetchContent)\n"
        "FetchContent_Declare(${ARG_FETCH_NAME} GIT_REPOSITORY ${ARG_GIT_REPOSITORY} GIT_TAG ${ARG_GIT_TAG})\n"
        "FetchContent_MakeAvailable(${ARG_FETCH_NAME})\n"
        "file(WRITE \"\${CMAKE_CURRENT_BINARY_DIR}/src_dir.txt\" \"\${${ARG_FETCH_NAME}_SOURCE_DIR}\")\n"
      )
      execute_process(
        COMMAND "${CMAKE_COMMAND}" -S "${_subbuild_dir}" -B
                "${_subbuild_dir}/build"
        OUTPUT_QUIET
        ERROR_VARIABLE _fetch_err
        RESULT_VARIABLE _fetch_res)
      if(NOT _fetch_res EQUAL 0)
        message(
          FATAL_ERROR "Failed to fetch ${ARG_DISPLAY_NAME}: ${_fetch_err}")
      endif()
      file(READ "${_subbuild_dir}/build/src_dir.txt" _src_dir)
      string(STRIP "${_src_dir}" _src_dir)
      if(ARG_PATH_VAR)
        set(${ARG_PATH_VAR}
            "${_src_dir}"
            CACHE PATH "Path to ${ARG_DISPLAY_NAME} directory" FORCE)
        set(${ARG_PATH_VAR} "${_src_dir}" PARENT_SCOPE)
      endif()
      return()
    endif()

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

    if(DEV_WARNINGS_RECOVER_STATE STREQUAL "__UNSET__")
      unset(CMAKE_SUPPRESS_DEVELOPER_WARNINGS CACHE)
    else()
      set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS
          ${DEV_WARNINGS_RECOVER_STATE}
          CACHE INTERNAL "Restore original setting" FORCE)
    endif()
    if(DEV_ERRORS_RECOVER_STATE STREQUAL "__UNSET__")
      unset(CMAKE_SUPPRESS_DEVELOPER_ERRORS CACHE)
    else()
      set(CMAKE_SUPPRESS_DEVELOPER_ERRORS
          ${DEV_ERRORS_RECOVER_STATE}
          CACHE INTERNAL "Restore original setting" FORCE)
    endif()
    unset(DEV_WARNINGS_RECOVER_STATE)
    unset(DEV_ERRORS_RECOVER_STATE)

    if(ARG_SILENCE_TARGETS)
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
