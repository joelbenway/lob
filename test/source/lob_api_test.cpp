// This file is a part of lob, an exterior ballistics calculation library
// Copyright (c) 2025  Joel Benway
// Please see end of file for extended copyright information

#include <gtest/gtest.h>

#include "eng_units.hpp"
#include "lob/lob.hpp"

namespace tests {

TEST(LobAPITest, Version) {
  const char* version_string = lob::Version();
  EXPECT_NE(version_string, nullptr);
  uint8_t dot_count = 0U;
  for (uint8_t i = 0U; version_string[i] != '\0'; i++) {
    if (version_string[i] == '.') {
      dot_count++;
    }
  }
  EXPECT_EQ(2, dot_count) << version_string;
}

TEST(LobAPITest, MoaToMil) {
  const auto kA = lob::MoaT(5);
  const auto kB = lob::MilT(lob::MoaToMil(kA.Value()));
  EXPECT_FLOAT_EQ(kA.Value(), lob::MoaT(kB).Value());
}

TEST(LobAPITest, MoaToDeg) {
  const auto kA = lob::MoaT(5);
  const auto kB = lob::DegreesT(lob::MoaToDeg(kA.Value()));
  EXPECT_FLOAT_EQ(kA.Value(), lob::MoaT(kB).Value());
}

TEST(LobAPITest, MoaToIphy) {
  const auto kA = lob::MoaT(5);
  const auto kB = lob::IphyT(lob::MoaToIphy(kA.Value()));
  EXPECT_FLOAT_EQ(kA.Value(), lob::MoaT(kB).Value());
}

TEST(LobAPITest, MoaToInch) {
  const auto kA = lob::MoaT(5);
  const auto kB = lob::MoaToInch(kA.Value(), 300);
  EXPECT_FLOAT_EQ(lob::IphyT(kA).Value(), kB);
}

}  // namespace tests

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.