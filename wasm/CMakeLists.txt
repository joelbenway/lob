# Copyright (c) 2026  Joel Benway
# SPDX-License-Identifier: GPL-3.0-or-later

add_executable(lob_exhibits wasm.cpp)
target_link_libraries(lob_exhibits PRIVATE lob::lob)
target_include_directories(lob_exhibits PRIVATE "${PROJECT_SOURCE_DIR}/source"
                                         "${PROJECT_SOURCE_DIR}/include"
                                         "${PROJECT_BINARY_DIR}/export")

target_compile_definitions(lob_exhibits PRIVATE LOB_EXPORT=)
target_compile_features(lob_exhibits PRIVATE cxx_std_14)

if(EMSCRIPTEN)
  set_target_properties(
    lob_exhibits
    PROPERTIES SUFFIX ".mjs"
               COMPILE_FLAGS "-O3"
               LINK_FLAGS
               "-O3 -flto --no-entry -sENVIRONMENT=web,node -sMODULARIZE=1 -sEXPORT_ES6=1 -sALLOW_MEMORY_GROWTH=1 -sEXPORTED_FUNCTIONS=['_LobExhibitsVersion','_LobExhibitDragEvalG1','_LobExhibitDragEvalG7']"
  )
endif()
