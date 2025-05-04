cmake_minimum_required(VERSION 3.14)

project(lobTests LANGUAGES CXX)

include(../cmake/project-is-top-level.cmake)
include(../cmake/folders.cmake)
include(../cmake/silence.cmake)

# ---- Dependencies ----

if(PROJECT_IS_TOP_LEVEL)
  find_package(lob REQUIRED)
  target_include_directories()
  enable_testing()
endif()

set(DEV_WARNINGS_RECOVER_STATE "$CACHE{CMAKE_SUPPRESS_DEVELOPER_WARNINGS}")
set(DEV_ERRORS_RECOVER_STATE "$CACHE{CMAKE_SUPPRESS_DEVELOPER_ERRORS}")
set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS
    ON
    CACHE INTERNAL "Suppress GoogleBenchmark CMake Warnings" FORCE)
set(CMAKE_SUPPRESS_DEVELOPER_ERRORS
    ON
    CACHE INTERNAL "Suppress GoogleBenchmark CMake Errors" FORCE)

include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.14.0)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt
    ON
    CACHE BOOL "" FORCE)

set(SHARED_LIBS_RECOVER_STATE ${BUILD_SHARED_LIBS})
set(BUILD_SHARED_LIBS
    OFF
    CACHE BOOL "Force static GoogleTest" FORCE)
FetchContent_MakeAvailable(googletest)
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
unset(DEV_ERRORSRECOVER_STATE)

# ---- Tests ----
set(LOB_TEST_SOURCES
    source/lob_test.cpp
    source/lob_api_test.cpp
    source/lob_builder_test.cpp
    source/lob_coriolis_test.cpp
    source/lob_env_test.cpp
    source/lob_spin_test.cpp
    source/lob_wind_test.cpp)

set(HELPER_TEST_SOURCES
    source/calc_test.cpp source/cartesian_test.cpp source/eng_units_test.cpp
    source/helpers_test.cpp source/ode_test.cpp source/tables_test.cpp)

if(BUILD_SHARED_LIBS)
  add_executable(lob_test ${LOB_TEST_SOURCES})
else()
  add_executable(lob_test ${LOB_TEST_SOURCES} ${HELPER_TEST_SOURCES})
endif()

target_include_directories(lob_test
                           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../source)

target_compile_features(lob_test PRIVATE cxx_std_14)

silence(gtest)
silence(gtest_main)
silence(gmock)
silence(gmock_main)

target_link_libraries(lob_test PRIVATE lob::lob GTest::gtest GTest::gtest_main)

include(GoogleTest)

gtest_discover_tests(lob_test)

# ---- End-of-file commands ----

add_folders(Test)
