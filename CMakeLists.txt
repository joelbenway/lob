cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    lob
    VERSION 0.2.0
    DESCRIPTION "an exterior balistics calculation library"
    HOMEPAGE_URL "https://github.com/joelbenway/lob"
    LANGUAGES CXX
)

set(PROJECT_VERSION "${PROJECT_VERSION}")

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/source/version.hpp.in"
  "${CMAKE_CURRENT_BINARY_DIR}/include/version.hpp"
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Declare library ----

add_library(
    lob_lob
    source/lob.cpp
    source/calc.cpp
    source/tables.cpp
)
add_library(lob::lob ALIAS lob_lob)

include(GenerateExportHeader)
generate_export_header(
    lob_lob
    BASE_NAME lob
    EXPORT_FILE_NAME export/lob/lob_export.hpp
    CUSTOM_CONTENT_FROM_VARIABLE pragma_suppress_c4251
)

if(NOT BUILD_SHARED_LIBS)
  target_compile_definitions(lob_lob PUBLIC LOB_STATIC_DEFINE)
endif()

set_target_properties(
    lob_lob PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
    VERSION "${PROJECT_VERSION}"
    SOVERSION "${PROJECT_VERSION_MAJOR}"
    EXPORT_NAME lob
    OUTPUT_NAME lob
)

target_include_directories(
    lob_lob ${warning_guard}
    PUBLIC
    "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
)

target_include_directories(
    lob_lob ${warning_guard}
    PUBLIC
    "\$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>"
)

target_include_directories(
    lob_lob SYSTEM
    PUBLIC
    "\$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/export>"
)

target_compile_features(lob_lob PUBLIC cxx_std_14)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Examples ----

if(PROJECT_IS_TOP_LEVEL)
  option(BUILD_EXAMPLES "Build examples tree." "${lob_DEVELOPER_MODE}")
  if(BUILD_EXAMPLES)
    add_subdirectory(example)
  endif()
endif()

# ---- Developer mode ----

if(NOT lob_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of lob"
  )
endif()

include(cmake/dev-mode.cmake)
