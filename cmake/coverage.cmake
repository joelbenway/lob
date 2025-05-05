# Copyright (c) 2025  Joel Benway
# SPDX-License-Identifier: GPL-3.0-or-later
# Please see end of file for extended copyright information

# ---- Variables ----

# We use variables separate from what CTest uses, because those have
# customization issues
set(COVERAGE_TRACE_COMMAND
    lcov -c -q -o "${PROJECT_BINARY_DIR}/coverage.info" -d
    "${PROJECT_BINARY_DIR}" --include "${PROJECT_SOURCE_DIR}/include/lob/*"
    --include "${PROJECT_SOURCE_DIR}/source/*"
    CACHE STRING
          "; separated command to generate a trace for the 'coverage' target")

set(COVERAGE_HTML_COMMAND
    genhtml --legend -f -q "${PROJECT_BINARY_DIR}/coverage.info" -p
    "${PROJECT_SOURCE_DIR}" -o "${PROJECT_BINARY_DIR}/coverage_html"
    CACHE
      STRING
      "; separated command to generate an HTML report for the 'coverage' target"
)

# ---- Coverage target ----

add_custom_target(
  coverage
  COMMAND ${COVERAGE_TRACE_COMMAND}
  COMMAND ${COVERAGE_HTML_COMMAND}
  COMMENT "Generating coverage report"
  VERBATIM)

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
