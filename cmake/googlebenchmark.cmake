# Copyright (c) 2026  Joel Benway
# SPDX-License-Identifier: GPL-3.0-or-later
# Please see end of file for extended copyright information

# Provides the benchmark::benchmark and benchmark::benchmark_main targets,
# preferring a system package and falling back to FetchContent. Include from any
# directory scope that links the targets instead of repeating this block.
include(${CMAKE_CURRENT_LIST_DIR}/find-or-fetch.cmake)

set(BENCHMARK_ENABLE_TESTING NO)

lob_find_or_fetch(
  benchmark
  PACKAGE benchmark
  DISPLAY_NAME "google/benchmark"
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG v1.9.2
  FETCH_NAME googlebenchmark
  FORCE_STATIC
  SILENCE_TARGETS benchmark benchmark_main)

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
