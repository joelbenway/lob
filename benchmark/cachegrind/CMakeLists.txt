# Copyright (c) 2025  Joel Benway
# SPDX-License-Identifier: GPL-3.0-or-later
# Please see end of file for extended copyright information

include(../../cmake/cachegrind.cmake)

# Reps tuned so the loop dominates fixed startup cost (signal_ratio > 90%);
# raise any case the runner warns about.
add_cachegrind_benchmark(lob_bench
  SOURCES lob_bench.cpp
  CASES
    build_basic:32000
    build_full:14000
    build_custom_table:3500
    build_boatright:500
    build_zero_search:400
    solve_basic:200
    solve_boatright:200)

# Cheap gate so a broken builder chain fails the test job instead of
# silently reporting a stable, meaningless number.
if(BUILD_TESTING)
  add_test(NAME cg_bench_verify COMMAND lob_bench --verify)
endif()

add_folders(Benchmark)

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