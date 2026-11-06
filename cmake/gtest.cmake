# Copyright (c) 2026  Joel Benway
# SPDX-License-Identifier: GPL-3.0-or-later
# Please see end of file for extended copyright information

# Provides the GTest::gtest and GTest::gtest_main targets, preferring a system
# package and falling back to FetchContent. Include from any directory scope
# that links the targets instead of repeating this block.
include(${CMAKE_CURRENT_LIST_DIR}/find-or-fetch.cmake)

set(gtest_force_shared_crt
    ON
    CACHE BOOL "" FORCE)

lob_find_or_fetch(
  GTest
  PACKAGE GTest
  DISPLAY_NAME "GTest"
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.14.0
  FETCH_NAME googletest
  FORCE_STATIC
  SILENCE_TARGETS gtest gtest_main gmock gmock_main)

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
