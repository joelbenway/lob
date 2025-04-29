cmake_minimum_required(VERSION 3.14)

project(
    lobExamples
    VERSION 1.0.1
    DESCRIPTION "Examples using the lob library"
    HOMEPAGE_URL "https://github.com/joelbenway/lob"
    LANGUAGES CXX
)

set(PROJECT_VERSION "${PROJECT_VERSION}")

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/version.hpp.in"
  "${CMAKE_CURRENT_BINARY_DIR}/include/version.hpp"
)

include(../cmake/project-is-top-level.cmake)
include(../cmake/folders.cmake)

if(PROJECT_IS_TOP_LEVEL)
  find_package(lob REQUIRED)
endif()

add_custom_target(run-examples)

function(add_example NAME)
  add_executable("${NAME}" "${NAME}.cpp")
  target_include_directories(
    "${NAME}" PUBLIC 
    ${CMAKE_CURRENT_BINARY_DIR}/include
  )
  target_link_libraries(
    "${NAME}" PRIVATE
    lob::lob)
  target_compile_features("${NAME}" PRIVATE cxx_std_14)
  add_custom_target("run_${NAME}" COMMAND "${NAME}" VERBATIM)
  add_dependencies("run_${NAME}" "${NAME}")
  add_dependencies(run-examples "run_${NAME}")
endfunction()

add_example(lobber)

add_folders(Example)
