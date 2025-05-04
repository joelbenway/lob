cmake_minimum_required(VERSION 3.14)

project(
  lobBenchmarks
  VERSION 1.0.0
  DESCRIPTION "Benchmarks for lob library"
  HOMEPAGE_URL "https://github.com/joelbenway/lob"
  LANGUAGES CXX)

set(PROJECT_VERSION "${PROJECT_VERSION}")

include(../cmake/project-is-top-level.cmake)
include(../cmake/folders.cmake)
include(../cmake/silence.cmake)

if(PROJECT_IS_TOP_LEVEL)
  find_package(lob REQUIRED)
endif()

set(DEV_WARNINGS_RECOVER_STATE "$CACHE{CMAKE_SUPPRESS_DEVELOPER_WARNINGS}")
set(DEV_ERRORS_RECOVER_STATE "$CACHE{CMAKE_SUPPRESS_DEVELOPER_ERRORS}")
set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS
    ON
    CACHE INTERNAL "Suppress GoogleBenchmark CMake Warnings" FORCE)
set(CMAKE_SUPPRESS_DEVELOPER_ERRORS
    ON
    CACHE INTERNAL "Suppress GoogleBenchmark CMake Errors" FORCE)

set(BENCHMARK_ENABLE_TESTING NO)

include(FetchContent)
FetchContent_Declare(
  googlebenchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG v1.9.2)

set(SHARED_LIBS_RECOVER_STATE ${BUILD_SHARED_LIBS})
set(BUILD_SHARED_LIBS
    OFF
    CACHE BOOL "Force static GoogleBenchmark" FORCE)
FetchContent_MakeAvailable(googlebenchmark)
set(BUILD_SHARED_LIBS
    ${SHARED_LIBS_RECOVER_STATE}
    CACHE BOOL "Restore original setting" FORCE)
unset(SHARED_LIBS_RECOVER_STATE)

set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS
    ${DEV_WARNINGS_RECOVER_STATE}
    CACHE INTERNAL "Restore original setting" FORCE)
set(CMAKE_SUPPRESS_DEVELOPER_ERRORS
    ${DEV_ERRORS_RECOVER_STATE}
    CACHE INTERNAL "Restore original setting" FORCE)
unset(DEV_WARNINGS_RECOVER_STATE)
unset(DEV_ERRORS_RECOVER_STATE)

silence(benchmark)
silence(benchmark_main)

add_custom_target(run-benchmarks)

function(add_benchmark NAME)
  add_executable("${NAME}" "${NAME}.cpp")
  target_include_directories(
    "${NAME}" PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/include
                     ${CMAKE_CURRENT_SOURCE_DIR}/../source)
  target_compile_features("${NAME}" PRIVATE cxx_std_14)
  target_link_libraries("${NAME}" PRIVATE lob::lob benchmark::benchmark
                                          benchmark::benchmark_main)
  add_custom_target(
    "run_${NAME}"
    COMMAND "${NAME}"
    VERBATIM)
  add_dependencies("run_${NAME}" "${NAME}")
  add_dependencies(run-benchmarks "run_${NAME}")
endfunction()

add_benchmark(loblerp)

add_folders(Benchmark)
