// This file is a part of lob, an exterior ballistics calculation library
// Copyright (c) 2025  Joel Benway
// Please see end of file for extended copyright information

#include <gtest/gtest.h>

#include <cstddef>

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
  EXPECT_DOUBLE_EQ(kA.Value(), lob::MoaT(kB).Value());
}

TEST(LobAPITest, MoaToDeg) {
  const auto kA = lob::MoaT(5);
  const auto kB = lob::DegreesT(lob::MoaToDeg(kA.Value()));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::MoaT(kB).Value());
}

TEST(LobAPITest, MoaToIphy) {
  const auto kA = lob::MoaT(5);
  const auto kB = lob::IphyT(lob::MoaToIphy(kA.Value()));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::MoaT(kB).Value());
}

TEST(LobAPITest, MoaToInch) {
  const auto kA = lob::MoaT(5);
  const auto kB = lob::MoaToInch(kA.Value(), 300);
  EXPECT_DOUBLE_EQ(lob::IphyT(kA).Value(), kB);
}

TEST(LobAPITest, MilToMoa) {
  const auto kA = lob::MilT(10);
  const auto kB = lob::MoaT(lob::MilToMoa(kA.Value()));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::MilT(kB).Value());
}

TEST(LobAPITest, MilToDeg) {
  const auto kA = lob::MilT(10);
  const auto kB = lob::DegreesT(lob::MilToDeg(kA.Value()));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::MilT(kB).Value());
}

TEST(LobAPITest, MilToIphy) {
  const auto kA = lob::MilT(10);
  const auto kB = lob::IphyT(lob::MilToIphy(kA.Value()));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::MilT(kB).Value());
}

TEST(LobAPITest, MilToInch) {
  const auto kA = lob::MilT(10);
  const auto kB = lob::MilToInch(kA.Value(), 300);
  EXPECT_DOUBLE_EQ(lob::IphyT(kA).Value(), kB);
}

TEST(LobAPITest, DegToMoa) {
  const auto kA = lob::DegreesT(0.2);
  const auto kB = lob::MoaT(lob::DegToMoa(kA.Value()));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::DegreesT(kB).Value());
}

TEST(LobAPITest, DegToMil) {
  const auto kA = lob::DegreesT(0.2);
  const auto kB = lob::MilT(lob::DegToMil(kA.Value()));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::DegreesT(kB).Value());
}

TEST(LobAPITest, InchToMoa) {
  const auto kA = lob::IphyT(5);
  const auto kB = lob::MoaT(lob::InchToMoa(kA.Value(), 300));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::IphyT(kB).Value());
}

TEST(LobAPITest, InchToDeg) {
  const auto kA = lob::IphyT(5);
  const auto kB = lob::DegreesT(lob::InchToDeg(kA.Value(), 300));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::IphyT(kB).Value());
}

TEST(LobAPITest, JToFtLbs) {
  const auto kA = lob::JouleT(100);
  const auto kB = lob::FtLbsT(lob::JToFtLbs(kA.Value()));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::JouleT(kB).Value());
}

TEST(LobAPITest, FtLbsToJ) {
  const auto kA = lob::FtLbsT(100);
  const auto kB = lob::JouleT(lob::FtLbsToJ(kA.Value()));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::FtLbsT(kB).Value());
}

TEST(LobAPITest, MtoYd) {
  const auto kA = lob::MeterT(100);
  const auto kB = lob::YardT(lob::MtoYd(kA.Value()));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::MeterT(kB).Value());
}

TEST(LobAPITest, YdToFt) {
  const auto kA = lob::YardT(100);
  const auto kB = lob::FeetT(lob::YdToFt(kA.Value()));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::YardT(kB).Value());
}

TEST(LobAPITest, MToFt) {
  const auto kA = lob::MeterT(100);
  const auto kB = lob::FeetT(lob::MToFt(kA.Value()));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::MeterT(kB).Value());
}

TEST(LobAPITest, FtToIn) {
  const auto kA = lob::FeetT(100);
  const auto kB = lob::InchT(lob::FtToIn(kA.Value()));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::FeetT(kB).Value());
}

TEST(LobAPITest, MmToIn) {
  const auto kA = lob::MmT(100);
  const auto kB = lob::InchT(lob::MmToIn(kA.Value()));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::MmT(kB).Value());
}

TEST(LobAPITest, CmToIn) {
  const auto kA = lob::CmT(100);
  const auto kB = lob::InchT(lob::CmToIn(kA.Value()));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::CmT(kB).Value());
}

TEST(LobAPITest, YdToM) {
  const auto kA = lob::YardT(100);
  const auto kB = lob::MeterT(lob::YdToM(kA.Value()));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::YardT(kB).Value());
}

TEST(LobAPITest, FtToM) {
  const auto kA = lob::FeetT(100);
  const auto kB = lob::MeterT(lob::FtToM(kA.Value()));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::FeetT(kB).Value());
}

TEST(LobAPITest, FtToYd) {
  const auto kA = lob::FeetT(100);
  const auto kB = lob::YardT(lob::FtToYd(kA.Value()));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::FeetT(kB).Value());
}

TEST(LobAPITest, InToMm) {
  const auto kA = lob::InchT(100);
  const auto kB = lob::MmT(lob::InToMm(kA.Value()));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::InchT(kB).Value());
}

TEST(LobAPITest, InToCm) {
  const auto kA = lob::InchT(100);
  const auto kB = lob::CmT(lob::InToCm(kA.Value()));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::InchT(kB).Value());
}

TEST(LobAPITest, InToFt) {
  const auto kA = lob::InchT(100);
  const auto kB = lob::FeetT(lob::InToFt(kA.Value()));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::InchT(kB).Value());
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