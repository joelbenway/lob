# Copyright (c) 2026  Joel Benway
# SPDX-License-Identifier: GPL-3.0-or-later
# Please see end of file for extended copyright information

# Provides the Emscripten toolchain file from an installed SDK. Configure
# never downloads a compiler; the emsdk install steps are in the error below.
#
# This file is used as CMAKE_TOOLCHAIN_FILE, so it runs before the project
# is configured and must not depend on the project being set up.

# Emscripten.cmake dereferences CMAKE_C_FLAGS unquoted, which cmake-pedantic
# rejects as uninitialized when the project has no C sources.
if(NOT DEFINED CMAKE_C_FLAGS)
  set(CMAKE_C_FLAGS "")
endif()

if(DEFINED EMSCRIPTEN_TOOLCHAIN_FILE)
  include("${EMSCRIPTEN_TOOLCHAIN_FILE}")
  return()
endif()

# Candidate roots, in priority order: the emsdk and emscripten variables,
# then the directory containing emcc (and its parents).
set(_emscripten_roots "")
if(DEFINED ENV{EMSDK})
  list(APPEND _emscripten_roots "$ENV{EMSDK}")
endif()
if(DEFINED ENV{EMSCRIPTEN})
  list(APPEND _emscripten_roots "$ENV{EMSCRIPTEN}")
endif()
find_program(_emcc emcc)
if(_emcc)
  get_filename_component(_emcc_dir "${_emcc}" DIRECTORY)
  list(APPEND _emscripten_roots "${_emcc_dir}")
  get_filename_component(_emcc_parent "${_emcc_dir}" DIRECTORY)
  list(APPEND _emscripten_roots "${_emcc_parent}")
  get_filename_component(_emcc_grandparent "${_emcc_parent}" DIRECTORY)
  list(APPEND _emscripten_roots "${_emcc_grandparent}")
endif()

set(_emscripten_toolchain "")
foreach(_root IN LISTS _emscripten_roots)
  # Layouts: nix, emsdk, and a plain emscripten checkout.
  foreach(
    _candidate
    "share/emscripten/cmake/Modules/Platform/Emscripten.cmake"
    "upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake"
    "cmake/Modules/Platform/Emscripten.cmake")
    if(EXISTS "${_root}/${_candidate}")
      set(_emscripten_toolchain "${_root}/${_candidate}")
      if(_candidate MATCHES "^upstream/" AND EXISTS "${_root}/.emscripten")
        set(ENV{EM_CONFIG} "${_root}/.emscripten")
      endif()
      break()
    endif()
  endforeach()
  if(_emscripten_toolchain)
    break()
  endif()
endforeach()

if(NOT _emscripten_toolchain)
  message(
    FATAL_ERROR
      "Emscripten not found. Install and activate the emsdk:\n"
      "  git clone https://github.com/emscripten-core/emsdk.git ~/emsdk\n"
      "  cd ~/emsdk && ./emsdk install 3.1.73 && ./emsdk activate 3.1.73\n"
      "  source ./emsdk_env.sh   # emsdk_env.bat on Windows\n"
      "Then configure with: cmake --preset=wasm\n"
      "Or use the nix development shell: nix develop")
endif()

# The try_compile projects need the toolchain path, and the cache carries it
# into them.
set(EMSCRIPTEN_TOOLCHAIN_FILE
    "${_emscripten_toolchain}"
    CACHE INTERNAL "Emscripten toolchain file" FORCE)
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES EMSCRIPTEN_TOOLCHAIN_FILE)

include("${_emscripten_toolchain}")

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
