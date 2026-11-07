# Copyright (c) 2026  Joel Benway
# SPDX-License-Identifier: GPL-3.0-or-later
# Please see end of file for extended copyright information

# Locates doxygen-awesome-css assets, preferring a system package and falling
# back to FetchContent. Defines DOXYGEN_AWESOME_CSS_DIR pointing to the
# directory containing doxygen-awesome.css and its companion scripts.
include(${CMAKE_CURRENT_LIST_DIR}/find-or-fetch.cmake)

lob_find_or_fetch(
  doxygen_awesome_css
  PATH_VAR DOXYGEN_AWESOME_CSS_DIR
  PATH_NAMES doxygen-awesome.css
  PATH_SUFFIXES share/doxygen-awesome-css doxygen-awesome-css
  DISPLAY_NAME "doxygen-awesome-css"
  GIT_REPOSITORY https://github.com/jothepro/doxygen-awesome-css.git
  GIT_TAG v2.5.0
  FETCH_NAME doxygen-awesome-css)

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
