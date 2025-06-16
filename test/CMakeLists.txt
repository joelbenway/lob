# Copyright (c) 2025  Joel Benway SPDX-License-Identifier: GPL-3.0-or-later
# Please see end of file for extended copyright information
cmake_minimum_required(VERSION 3.14)

project(lobTests LANGUAGES CXX)

include(../cmake/project-is-top-level.cmake)
include(../cmake/folders.cmake)

# ---- Dependencies ----

if(PROJECT_IS_TOP_LEVEL)
  find_package(lob REQUIRED)
  target_include_directories()
  enable_testing()
endif()

find_package(GTest CONFIG)

if(NOT GTest_FOUND)
  message(STATUS "GTest not found, FetchContent instead...")
  set(DEV_WARNINGS_RECOVER_STATE "$CACHE{CMAKE_SUPPRESS_DEVELOPER_WARNINGS}")
  set(DEV_ERRORS_RECOVER_STATE "$CACHE{CMAKE_SUPPRESS_DEVELOPER_ERRORS}")
  set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS
      ON
      CACHE INTERNAL "Suppress third-party CMake warnings" FORCE)
  set(CMAKE_SUPPRESS_DEVELOPER_ERRORS
      ON
      CACHE INTERNAL "Suppress third-party CMake errors" FORCE)

  include(FetchContent)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0)

  # For Windows: Prevent overriding the parent project's compiler/linker
  # settings
  set(gtest_force_shared_crt
      ON
      CACHE BOOL "" FORCE)

  set(SHARED_LIBS_RECOVER_STATE ${BUILD_SHARED_LIBS})
  set(BUILD_SHARED_LIBS
      OFF
      CACHE BOOL "Force static GoogleTest" FORCE)
  FetchContent_MakeAvailable(googletest)
  set(BUILD_SHARED_LIBS
      ${SHARED_LIBS_RECOVER_STATE}
      CACHE BOOL "Restore original setting" FORCE)
  unset(SHARED_LIBS_RECOVER_STATE)

  set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS
      ${DEV_WARNINGS_RECOVER_STATE}
      CACHE INTERNAL "Restore original setting" FORCE)
  set(CMAKE_SUPPRESS_DEVELOPER_ERRORS
      ${DEV_ERRORS_RECOVER_STATE}
      CACHE INTERNAL "Restore original setting" FORCE)
  unset(DEV_WARNINGS_RECOVER_STATE)
  unset(DEV_ERRORS_RECOVER_STATE)

  include(../cmake/silence.cmake)
  silence(gtest)
  silence(gtest_main)
  silence(gmock)
  silence(gmock_main)
else()
  message(STATUS "Found GTest: ${GTest_DIR}")
endif()

# ---- Tests ----
# cmake-format: off
set(LOB_TEST_SOURCES
    source/lob_test.cpp
    source/lob_api_test.cpp
    source/lob_cwaj_test.cpp
    source/lob_builder_test.cpp
    source/lob_coriolis_test.cpp
    source/lob_env_test.cpp
    source/lob_spin_drift_test.cpp
    source/lob_wind_test.cpp)

set(HELPER_TEST_SOURCES
    source/boatright_test.cpp
    source/calc_test.cpp
    source/cartesian_test.cpp
    source/eng_units_test.cpp
    source/helpers_test.cpp
    source/litz_test.cpp
    source/ode_test.cpp)
# cmake-format: on

if(BUILD_SHARED_LIBS)
  add_executable(lob_test ${LOB_TEST_SOURCES})
else()
  add_executable(lob_test ${LOB_TEST_SOURCES} ${HELPER_TEST_SOURCES})
endif()

target_include_directories(lob_test
                           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../source)

target_compile_features(lob_test PRIVATE cxx_std_14)

target_link_libraries(lob_test PRIVATE lob::lob GTest::gtest GTest::gtest_main)

include(GoogleTest)

gtest_discover_tests(lob_test)

# ---- End-of-file commands ----

add_folders(Test)

# This file is part of lob.
#
# lob is free software: you can redistribute it and/or modify it under the terms
# of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# lob is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
# A PARTICULAR PURPOSE. See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with
# lob. If not, see <https://www.gnu.org/licenses/>.
