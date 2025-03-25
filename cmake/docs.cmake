# ---- Dependencies ----

find_package(Doxygen REQUIRED)

# ---- Declare documentation target ----

set(
    DOXYGEN_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/docs"
    CACHE PATH "Path for the generated Doxygen documentation"
)

set(working_dir "${PROJECT_BINARY_DIR}/docs")

configure_file("docs/Doxyfile.in" "${working_dir}/Doxyfile" @ONLY)

set(DOXYGEN_CMD)
if(TARGET Doxygen::doxygen)
  set(DOXYGEN_CMD "$<TARGET_FILE:Doxygen::doxygen>")
elseif(DEFINED DOXYGEN_EXECUTABLE)
  set(DOXYGEN_CMD "${DOXYGEN_EXECUTABLE}")
else()
  message(WARNING "Neither the Doxygen::doxygen import target nor DOXYGEN_EXECUTABLE are defined. Documentation target might not work.")
  set(DOXYGEN_CMD "doxygen")
endif()

add_custom_target(
    docs
    COMMAND "${CMAKE_COMMAND}" -E remove_directory
    "${DOXYGEN_OUTPUT_DIRECTORY}/html"
    COMMAND "${DOXYGEN_CMD}" "${working_dir}/Doxyfile"
    COMMENT "Building documentation using Doxygen"
    WORKING_DIRECTORY "${working_dir}"
    VERBATIM
)