if(PROJECT_IS_TOP_LEVEL)
  set(CMAKE_INSTALL_INCLUDEDIR
      "include/lob-${PROJECT_VERSION}"
      CACHE STRING "")
  set_property(CACHE CMAKE_INSTALL_INCLUDEDIR PROPERTY TYPE PATH)
endif()

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

# find_package(<package>) call for consumers to find this project
set(package lob)

install(
  DIRECTORY include/ "${PROJECT_BINARY_DIR}/export/"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
  COMPONENT lob_Development)

install(
  TARGETS lob_lob
  EXPORT lobTargets
  RUNTIME #
          COMPONENT lob_Runtime
  LIBRARY #
          COMPONENT lob_Runtime NAMELINK_COMPONENT lob_Development
  ARCHIVE #
          COMPONENT lob_Development
  INCLUDES #
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

write_basic_package_version_file("${package}ConfigVersion.cmake"
                                 COMPATIBILITY SameMajorVersion)

# Allow package maintainers to freely override the path for the configs
set(lob_INSTALL_CMAKEDIR
    "${CMAKE_INSTALL_LIBDIR}/cmake/${package}"
    CACHE STRING "CMake package config location relative to the install prefix")
set_property(CACHE lob_INSTALL_CMAKEDIR PROPERTY TYPE PATH)
mark_as_advanced(lob_INSTALL_CMAKEDIR)

install(
  FILES cmake/install-config.cmake
  DESTINATION "${lob_INSTALL_CMAKEDIR}"
  RENAME "${package}Config.cmake"
  COMPONENT lob_Development)

install(
  FILES "${PROJECT_BINARY_DIR}/${package}ConfigVersion.cmake"
  DESTINATION "${lob_INSTALL_CMAKEDIR}"
  COMPONENT lob_Development)

install(
  EXPORT lobTargets
  NAMESPACE lob::
  DESTINATION "${lob_INSTALL_CMAKEDIR}"
  COMPONENT lob_Development)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
