# Copyright (c) 2025  Joel Benway
# SPDX-License-Identifier: GPL-3.0-or-later
# Please see end of file for extended copyright information

# ---- Dependencies ----

find_package(Doxygen REQUIRED)

# ---- Declare documentation target ----

set(DOXYGEN_OUTPUT_DIRECTORY
    "${PROJECT_BINARY_DIR}/docs"
    CACHE PATH "Path for the generated Doxygen documentation")

set(working_dir "${PROJECT_BINARY_DIR}/docs")

configure_file("docs/Doxyfile.in" "${working_dir}/Doxyfile" @ONLY)

set(DOXYGEN_CMD)
if(TARGET Doxygen::doxygen)
  set(DOXYGEN_CMD "$<TARGET_FILE:Doxygen::doxygen>")
elseif(DEFINED DOXYGEN_EXECUTABLE)
  set(DOXYGEN_CMD "${DOXYGEN_EXECUTABLE}")
else()
  message(
    WARNING
      "Neither the Doxygen::doxygen import target nor DOXYGEN_EXECUTABLE are defined. Documentation target might not work."
  )
  set(DOXYGEN_CMD "doxygen")
endif()

add_custom_target(
  docs
  COMMAND "${CMAKE_COMMAND}" -E remove_directory
          "${DOXYGEN_OUTPUT_DIRECTORY}/html"
  COMMAND "${DOXYGEN_CMD}" "${working_dir}/Doxyfile"
  COMMENT "Building documentation using Doxygen"
  WORKING_DIRECTORY "${working_dir}"
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
