# Copyright (c) 2026  Joel Benway
# SPDX-License-Identifier: GPL-3.0-or-later
# Please see end of file for extended copyright information

add_executable(lob_wasm wasm.cpp)
target_link_libraries(lob_wasm PRIVATE lob::lob)
target_compile_features(lob_wasm PRIVATE cxx_std_14)

if(EMSCRIPTEN)
  set_target_properties(
    lob_wasm
    PROPERTIES SUFFIX ".mjs"
               OUTPUT_NAME lob
               RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

  file(STRINGS "${PROJECT_SOURCE_DIR}/include/lob/lob.h" _lob_export_lines
       REGEX "^LOB_EXPORT")
  set(_lob_exports "")
  foreach(_line IN LISTS _lob_export_lines)
    string(REGEX MATCHALL "Lob[A-Za-z0-9_]+\\(" _names "${_line}")
    foreach(_name IN LISTS _names)
      string(REGEX REPLACE "\\(" "" _name "${_name}")
      list(APPEND _lob_exports "_${_name}")
    endforeach()
  endforeach()
  list(
    APPEND
    _lob_exports
    _malloc
    _free
    _LobWasmBuilderSize
    _LobWasmContextSize
    _LobWasmWindPointSize
    _LobWasmOutputSize
    _LobWasmContextError
    _LobWasmOutputsToDoubles)
  list(JOIN _lob_exports "," _lob_exports_list)
  target_link_options(lob_wasm PRIVATE
                      "-sEXPORTED_FUNCTIONS=${_lob_exports_list}")

  add_custom_target(
    lob_solver ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/solver"
    COMMAND
      ${CMAKE_COMMAND} -E copy_if_different
      "${PROJECT_SOURCE_DIR}/example/solver/index.html"
      "${PROJECT_SOURCE_DIR}/example/solver/style.css"
      "${PROJECT_SOURCE_DIR}/example/solver/app.mjs"
      "${PROJECT_SOURCE_DIR}/example/solver/inputs.mjs"
      "${CMAKE_BINARY_DIR}/index.mjs" "${CMAKE_BINARY_DIR}/lob.mjs"
      "${CMAKE_BINARY_DIR}/lob.wasm" "${CMAKE_BINARY_DIR}/solver"
    DEPENDS lob_wasm
    COMMENT "Assembling the browser solver example")

  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/package.json.in"
                 "${CMAKE_BINARY_DIR}/package.json" @ONLY)
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/index.mjs"
                 "${CMAKE_BINARY_DIR}/index.mjs" COPYONLY)
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/README.md"
                 "${CMAKE_BINARY_DIR}/README.md" COPYONLY)
  configure_file("${PROJECT_SOURCE_DIR}/COPYING.md"
                 "${CMAKE_BINARY_DIR}/COPYING.md" COPYONLY)
endif()

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
