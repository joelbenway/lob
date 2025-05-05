# Copyright (c) 2025  Joel Benway
# SPDX-License-Identifier: GPL-3.0-or-later
# Please see end of file for extended copyright information

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
