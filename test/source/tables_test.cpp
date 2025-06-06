// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include "tables.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

namespace tests {

TEST(TableTests, DeriveMachDragTableMachs) {
  // Data is from Robert McCoy/BRL via JBM Ballistics
  // https://jbmballistics.com/ballistics/downloads/text/mcg1.txt
  // https://jbmballistics.com/ballistics/downloads/text/mcg7.txt
  const size_t kG1MachsSize = 79U;
  const std::array<float, kG1MachsSize> kG1Machs = {
      0.00F,  0.05F, 0.10F,  0.15F, 0.20F,  0.25F, 0.30F,  0.35F, 0.40F,  0.45F,
      0.50F,  0.55F, 0.60F,  0.70F, 0.725F, 0.75F, 0.775F, 0.80F, 0.825F, 0.85F,
      0.875F, 0.90F, 0.925F, 0.95F, 0.975F, 1.0F,  1.025F, 1.05F, 1.075F, 1.10F,
      1.125F, 1.15F, 1.20F,  1.25F, 1.30F,  1.35F, 1.40F,  1.45F, 1.50F,  1.55F,
      1.60F,  1.65F, 1.70F,  1.75F, 1.80F,  1.85F, 1.90F,  1.95F, 2.00F,  2.05F,
      2.10F,  2.15F, 2.20F,  2.25F, 2.30F,  2.35F, 2.40F,  2.45F, 2.50F,  2.60F,
      2.70F,  2.80F, 2.90F,  3.00F, 3.10F,  3.20F, 3.30F,  3.40F, 3.50F,  3.60F,
      3.70F,  3.80F, 3.90F,  4.00F, 4.20F,  4.40F, 4.60F,  4.80F, 5.00F};
  const size_t kG7MachsSize = 84U;
  const std::array<float, kG7MachsSize> kG7Machs = {
      0.00F, 0.05F,  0.10F, 0.15F,  0.20F, 0.25F,  0.30F, 0.35F,  0.40F, 0.45F,
      0.50F, 0.55F,  0.60F, 0.65F,  0.70F, 0.725F, 0.75F, 0.775F, 0.80F, 0.825F,
      0.85F, 0.875F, 0.90F, 0.925F, 0.95F, 0.975F, 1.00F, 1.025F, 1.05F, 1.075F,
      1.10F, 1.125F, 1.15F, 1.20F,  1.25F, 1.30F,  1.35F, 1.40F,  1.50F, 1.55F,
      1.60F, 1.65F,  1.70F, 1.75F,  1.80F, 1.85F,  1.90F, 1.95F,  2.00F, 2.05F,
      2.10F, 2.15F,  2.20F, 2.25F,  2.30F, 2.35F,  2.40F, 2.45F,  2.50F, 2.55F,
      2.60F, 2.65F,  2.70F, 2.75F,  2.80F, 2.85F,  2.90F, 2.95F,  3.00F, 3.10F,
      3.20F, 3.30F,  3.40F, 3.50F,  3.60F, 3.70F,  3.80F, 3.90F,  4.00F, 4.20F,
      4.40F, 4.60F,  4.80F, 5.00F};
  std::vector<uint16_t> machs;
  machs.reserve(kG1MachsSize + kG7MachsSize);
  for (auto mach : kG1Machs) {
    machs.push_back(static_cast<uint16_t>(std::floor(mach * lob::kTableScale)));
  }
  for (auto mach : kG7Machs) {
    machs.push_back(static_cast<uint16_t>(std::floor(mach * lob::kTableScale)));
  }
  std::sort(machs.begin(), machs.end());
  auto it = std::unique(machs.begin(), machs.end());
  machs.erase(it, machs.end());

  const bool kTableIsCorrectSize = machs.size() == lob::kMachs.size();
  bool tables_are_equal = true;
  for (size_t i = 0; i < lob::kMachs.size(); i++) {
    if (machs.at(i) != lob::kMachs.at(i)) {
      tables_are_equal = false;
      break;
    }
  }
  std::stringstream ss;
  for (auto mach : machs) {
    ss << std::fixed << std::setprecision(0) << mach << "U ,";
  }
  ss << "\n";
  EXPECT_TRUE(tables_are_equal && kTableIsCorrectSize) << ss.str();
}

TEST(TableTests, DeriveMachDragTableG1) {
  // Data is from Robert McCoy/BRL via JBM Ballistics
  // https://jbmballistics.com/ballistics/downloads/text/mcg1.txt
  const size_t kG1MachsSize = 79U;
  const std::array<float, kG1MachsSize> kG1Machs = {
      0.00F,  0.05F, 0.10F,  0.15F, 0.20F,  0.25F, 0.30F,  0.35F, 0.40F,  0.45F,
      0.50F,  0.55F, 0.60F,  0.70F, 0.725F, 0.75F, 0.775F, 0.80F, 0.825F, 0.85F,
      0.875F, 0.90F, 0.925F, 0.95F, 0.975F, 1.0F,  1.025F, 1.05F, 1.075F, 1.10F,
      1.125F, 1.15F, 1.20F,  1.25F, 1.30F,  1.35F, 1.40F,  1.45F, 1.50F,  1.55F,
      1.60F,  1.65F, 1.70F,  1.75F, 1.80F,  1.85F, 1.90F,  1.95F, 2.00F,  2.05F,
      2.10F,  2.15F, 2.20F,  2.25F, 2.30F,  2.35F, 2.40F,  2.45F, 2.50F,  2.60F,
      2.70F,  2.80F, 2.90F,  3.00F, 3.10F,  3.20F, 3.30F,  3.40F, 3.50F,  3.60F,
      3.70F,  3.80F, 3.90F,  4.00F, 4.20F,  4.40F, 4.60F,  4.80F, 5.00F};
  const std::array<float, kG1MachsSize> kG1DragCoefficients = {
      0.2629F, 0.2558F, 0.2487F, 0.2413F, 0.2344F, 0.2278F, 0.2214F, 0.2155F,
      0.2104F, 0.2061F, 0.2032F, 0.2020F, 0.2034F, 0.2165F, 0.2230F, 0.2313F,
      0.2417F, 0.2546F, 0.2706F, 0.2901F, 0.3136F, 0.3415F, 0.3734F, 0.4084F,
      0.4448F, 0.4805F, 0.5136F, 0.5427F, 0.5677F, 0.5883F, 0.6053F, 0.6191F,
      0.6393F, 0.6518F, 0.6589F, 0.6621F, 0.6625F, 0.6607F, 0.6573F, 0.6528F,
      0.6474F, 0.6413F, 0.6347F, 0.6280F, 0.6210F, 0.6141F, 0.6072F, 0.6003F,
      0.5934F, 0.5867F, 0.5804F, 0.5743F, 0.5685F, 0.5630F, 0.5577F, 0.5527F,
      0.5481F, 0.5438F, 0.5397F, 0.5325F, 0.5264F, 0.5211F, 0.5168F, 0.5133F,
      0.5105F, 0.5084F, 0.5067F, 0.5054F, 0.5040F, 0.5030F, 0.5022F, 0.5016F,
      0.5010F, 0.5006F, 0.4998F, 0.4995F, 0.4992F, 0.4990F, 0.4988F};

  std::vector<uint16_t> drags;
  drags.reserve(lob::kTableSize);
  for (size_t i = 0; i < lob::kTableSize; i++) {
    const double kMach =
        static_cast<double>(lob::kMachs.at(i)) / lob::kTableScale;
    const double kDrag =
        lob::tables::LobQerp(kG1Machs, kG1DragCoefficients, kMach);
    drags.push_back(
        static_cast<uint16_t>(std::round(kDrag * lob::kTableScale)));
  }

  bool table_is_valid = drags.size() == lob::kTableSize;
  for (size_t i = 0; i < lob::kTableSize; i++) {
    if (drags.at(i) != lob::kG1Drags.at(i)) {
      table_is_valid = false;
      break;
    }
  }
  std::stringstream ss;
  for (auto drag : drags) {
    ss << std::fixed << std::setprecision(0) << drag << "U ,";
  }
  ss << "\n";
  EXPECT_TRUE(table_is_valid) << ss.str();

  for (size_t i = 0; i < kG1MachsSize && table_is_valid; i++) {
    const auto kMach =
        static_cast<uint16_t>(std::round(kG1Machs.at(i) * lob::kTableScale));
    const auto kDrag = static_cast<uint16_t>(
        std::round(lob::LobLerp(lob::kMachs, lob::kG1Drags, kMach)));
    const auto kExpectedDrag = static_cast<uint16_t>(
        std::round(kG1DragCoefficients.at(i) * lob::kTableScale));
    EXPECT_EQ(kDrag, kExpectedDrag)
        << kDrag << " != " << kExpectedDrag << " at " << i << "\n";
  }
}

TEST(TableTests, DeriveMachDragTableG2) {
  // Data is from Robert McCoy/BRL via JBM Ballistics
  // https://jbmballistics.com/ballistics/downloads/text/mcg2.txt
  const size_t kG2MachsSize = 85U;
  const std::array<float, kG2MachsSize> kG2Machs = {
      0.0F,   0.05F, 0.1F,   0.15F, 0.2F,   0.25F, 0.3F,   0.35F, 0.4F,   0.45F,
      0.5F,   0.55F, 0.6F,   0.65F, 0.7F,   0.75F, 0.775F, 0.8F,  0.825F, 0.85F,
      0.875F, 0.9F,  0.925F, 0.95F, 0.975F, 1.0F,  1.025F, 1.05F, 1.075F, 1.1F,
      1.125F, 1.15F, 1.175F, 1.2F,  1.25F,  1.3F,  1.35F,  1.4F,  1.45F,  1.5F,
      1.55F,  1.6F,  1.65F,  1.7F,  1.75F,  1.8F,  1.85F,  1.9F,  1.95F,  2.0F,
      2.05F,  2.1F,  2.15F,  2.2F,  2.25F,  2.3F,  2.35F,  2.4F,  2.45F,  2.5F,
      2.55F,  2.6F,  2.65F,  2.7F,  2.75F,  2.8F,  2.85F,  2.9F,  2.95F,  3.0F,
      3.1F,   3.2F,  3.3F,   3.4F,  3.5F,   3.6F,  3.7F,   3.8F,  3.9F,   4.0F,
      4.2F,   4.4F,  4.6F,   4.80F, 5.0F};
  const std::array<float, kG2MachsSize> kG2DragCoefficients = {
      0.2303F, 0.2298F, 0.2287F, 0.2271F, 0.2251F, 0.2227F, 0.2196F, 0.2156F,
      0.2107F, 0.2048F, 0.198F,  0.1905F, 0.1828F, 0.1758F, 0.1702F, 0.1669F,
      0.1664F, 0.1667F, 0.1682F, 0.1711F, 0.1761F, 0.1831F, 0.2004F, 0.2589F,
      0.3492F, 0.3983F, 0.4075F, 0.4103F, 0.4114F, 0.4106F, 0.4089F, 0.4068F,
      0.4046F, 0.4021F, 0.3966F, 0.3904F, 0.3835F, 0.3759F, 0.3678F, 0.3594F,
      0.3512F, 0.3432F, 0.3356F, 0.3282F, 0.3213F, 0.3149F, 0.3089F, 0.3033F,
      0.2982F, 0.2933F, 0.2889F, 0.2846F, 0.2806F, 0.2768F, 0.2731F, 0.2696F,
      0.2663F, 0.2632F, 0.2602F, 0.2572F, 0.2543F, 0.2515F, 0.2487F, 0.246F,
      0.2433F, 0.2408F, 0.2382F, 0.2357F, 0.2333F, 0.2309F, 0.2262F, 0.2217F,
      0.2173F, 0.2132F, 0.2091F, 0.2052F, 0.2014F, 0.1978F, 0.1944F, 0.1912F,
      0.1851F, 0.1794F, 0.1741F, 0.1693F, 0.1648F};

  std::vector<uint16_t> drags;
  drags.reserve(lob::kTableSize);
  for (size_t i = 0; i < lob::kTableSize; i++) {
    const double kMach =
        static_cast<double>(lob::kMachs.at(i)) / lob::kTableScale;
    const double kDrag =
        lob::tables::LobQerp(kG2Machs, kG2DragCoefficients, kMach);
    drags.push_back(
        static_cast<uint16_t>(std::round(kDrag * lob::kTableScale)));
  }

  bool table_is_valid = drags.size() == lob::kTableSize;
  for (size_t i = 0; i < lob::kTableSize; i++) {
    if (drags.at(i) != lob::kG2Drags.at(i)) {
      table_is_valid = false;
      break;
    }
  }
  std::stringstream ss;
  for (auto drag : drags) {
    ss << std::fixed << std::setprecision(0) << drag << "U ,";
  }
  ss << "\n";
  EXPECT_TRUE(table_is_valid) << ss.str();
}

TEST(TableTests, DeriveMachDragTableG5) {
  // Data is from Robert McCoy/BRL via JBM Ballistics
  // https://jbmballistics.com/ballistics/downloads/text/mcg5.txt
  const size_t kG5MachsSize = 76U;
  const std::array<float, kG5MachsSize> kG5Machs = {
      0.0F,   0.05F, 0.1F,   0.15F, 0.2F,   0.25F, 0.3F,   0.35F, 0.4F,   0.45F,
      0.5F,   0.55F, 0.6F,   0.65F, 0.7F,   0.75F, 0.8F,   0.85F, 0.875F, 0.9F,
      0.925F, 0.95F, 0.975F, 1.0F,  1.025F, 1.05F, 1.075F, 1.1F,  1.15F,  1.2F,
      1.25F,  1.3F,  1.35F,  1.4F,  1.45F,  1.5F,  1.55F,  1.6F,  1.65F,  1.7F,
      1.75F,  1.8F,  1.85F,  1.9F,  1.95F,  2.0F,  2.05F,  2.1F,  2.15F,  2.2F,
      2.25F,  2.3F,  2.35F,  2.4F,  2.45F,  2.5F,  2.6F,   2.7F,  2.8F,   2.9F,
      3.0F,   3.1F,  3.2F,   3.3F,  3.4F,   3.5F,  3.6F,   3.7F,  3.8F,   3.9F,
      4.0F,   4.2F,  4.4F,   4.6F,  4.8F,   5.0F};
  const std::array<float, kG5MachsSize> kG5DragCoefficients = {
      0.171F,  0.1719F, 0.1727F, 0.1732F, 0.1734F, 0.173F,  0.1718F, 0.1696F,
      0.1668F, 0.1637F, 0.1603F, 0.1566F, 0.1529F, 0.1497F, 0.1473F, 0.1463F,
      0.1489F, 0.1583F, 0.1672F, 0.1815F, 0.2051F, 0.2413F, 0.2884F, 0.3379F,
      0.3785F, 0.4032F, 0.4147F, 0.4201F, 0.4278F, 0.4338F, 0.4373F, 0.4392F,
      0.4403F, 0.4406F, 0.4401F, 0.4386F, 0.4362F, 0.4328F, 0.4286F, 0.4237F,
      0.4182F, 0.4121F, 0.4057F, 0.3991F, 0.3926F, 0.3861F, 0.38F,   0.3741F,
      0.3684F, 0.363F,  0.3578F, 0.3529F, 0.3481F, 0.3435F, 0.3391F, 0.3349F,
      0.3269F, 0.3194F, 0.3125F, 0.306F,  0.2999F, 0.2942F, 0.2889F, 0.2838F,
      0.279F,  0.2745F, 0.2703F, 0.2662F, 0.2624F, 0.2588F, 0.2553F, 0.2488F,
      0.2429F, 0.2376F, 0.2326F, 0.228F};

  std::vector<uint16_t> drags;
  drags.reserve(lob::kTableSize);
  for (size_t i = 0; i < lob::kTableSize; i++) {
    const double kMach =
        static_cast<double>(lob::kMachs.at(i)) / lob::kTableScale;
    const double kDrag =
        lob::tables::LobQerp(kG5Machs, kG5DragCoefficients, kMach);
    drags.push_back(
        static_cast<uint16_t>(std::round(kDrag * lob::kTableScale)));
  }

  bool table_is_valid = drags.size() == lob::kTableSize;
  for (size_t i = 0; i < lob::kTableSize; i++) {
    if (drags.at(i) != lob::kG5Drags.at(i)) {
      table_is_valid = false;
      break;
    }
  }
  std::stringstream ss;
  for (auto drag : drags) {
    ss << std::fixed << std::setprecision(0) << drag << "U ,";
  }
  ss << "\n";
  EXPECT_TRUE(table_is_valid) << ss.str();
}

TEST(TableTests, DeriveMachDragTableG6) {
  // Data is from Robert McCoy/BRL via JBM Ballistics
  // https://jbmballistics.com/ballistics/downloads/text/mcg6.txt
  const size_t kG6MachsSize = 79U;
  const std::array<float, kG6MachsSize> kG6Machs = {
      0.0F,   0.05F, 0.1F,   0.15F, 0.2F,   0.25F, 0.3F,   0.35F, 0.4F,   0.45F,
      0.5F,   0.55F, 0.6F,   0.65F, 0.7F,   0.75F, 0.8F,   0.85F, 0.875F, 0.9F,
      0.925F, 0.95F, 0.975F, 1.0F,  1.025F, 1.05F, 1.075F, 1.1F,  1.125F, 1.15F,
      1.175F, 1.2F,  1.225F, 1.25F, 1.3F,   1.35F, 1.4F,   1.45F, 1.5F,   1.55F,
      1.6F,   1.65F, 1.7F,   1.75F, 1.8F,   1.85F, 1.9F,   1.95F, 2.0F,   2.05F,
      2.1F,   2.15F, 2.2F,   2.25F, 2.3F,   2.35F, 2.4F,   2.45F, 2.5F,   2.6F,
      2.7F,   2.8F,  2.9F,   3.0F,  3.1F,   3.2F,  3.3F,   3.4F,  3.5F,   3.6F,
      3.7F,   3.8F,  3.9F,   4.0F,  4.2F,   4.4F,  4.6F,   4.8F,  5.0F};
  const std::array<float, kG6MachsSize> kG6DragCoefficients = {
      0.2617F, 0.2553F, 0.2491F, 0.2432F, 0.2376F, 0.2324F, 0.2278F, 0.2238F,
      0.2205F, 0.2177F, 0.2155F, 0.2138F, 0.2126F, 0.2121F, 0.2122F, 0.2132F,
      0.2154F, 0.2194F, 0.2229F, 0.2297F, 0.2449F, 0.2732F, 0.3141F, 0.3597F,
      0.3994F, 0.4261F, 0.4402F, 0.4465F, 0.449F,  0.4497F, 0.4494F, 0.4482F,
      0.4464F, 0.4441F, 0.439F,  0.4336F, 0.4279F, 0.4221F, 0.4162F, 0.4102F,
      0.4042F, 0.3981F, 0.3919F, 0.3855F, 0.3788F, 0.3721F, 0.3652F, 0.3583F,
      0.3515F, 0.3447F, 0.3381F, 0.3314F, 0.3249F, 0.3185F, 0.3122F, 0.306F,
      0.3F,    0.2941F, 0.2883F, 0.2772F, 0.2668F, 0.2574F, 0.2487F, 0.2407F,
      0.2333F, 0.2265F, 0.2202F, 0.2144F, 0.2089F, 0.2039F, 0.1991F, 0.1947F,
      0.1905F, 0.1866F, 0.1794F, 0.173F,  0.1673F, 0.1621F, 0.1574F};

  std::vector<uint16_t> drags;
  drags.reserve(lob::kTableSize);
  for (size_t i = 0; i < lob::kTableSize; i++) {
    const double kMach =
        static_cast<double>(lob::kMachs.at(i)) / lob::kTableScale;
    const double kDrag =
        lob::tables::LobQerp(kG6Machs, kG6DragCoefficients, kMach);
    drags.push_back(
        static_cast<uint16_t>(std::round(kDrag * lob::kTableScale)));
  }

  bool table_is_valid = drags.size() == lob::kTableSize;
  for (size_t i = 0; i < lob::kTableSize; i++) {
    if (drags.at(i) != lob::kG6Drags.at(i)) {
      table_is_valid = false;
      break;
    }
  }
  std::stringstream ss;
  for (auto drag : drags) {
    ss << std::fixed << std::setprecision(0) << drag << "U ,";
  }
  ss << "\n";
  EXPECT_TRUE(table_is_valid) << ss.str();
}

TEST(TableTests, DeriveMachDragTableG7) {
  // Data is from Robert McCoy/BRL via JBM Ballistics
  // https://jbmballistics.com/ballistics/downloads/text/mcg7.txt
  const size_t kG7MachsSize = 84U;
  const std::array<float, kG7MachsSize> kG7Machs = {
      0.00F, 0.05F,  0.10F, 0.15F,  0.20F, 0.25F,  0.30F, 0.35F,  0.40F, 0.45F,
      0.50F, 0.55F,  0.60F, 0.65F,  0.70F, 0.725F, 0.75F, 0.775F, 0.80F, 0.825F,
      0.85F, 0.875F, 0.90F, 0.925F, 0.95F, 0.975F, 1.00F, 1.025F, 1.05F, 1.075F,
      1.10F, 1.125F, 1.15F, 1.20F,  1.25F, 1.30F,  1.35F, 1.40F,  1.50F, 1.55F,
      1.60F, 1.65F,  1.70F, 1.75F,  1.80F, 1.85F,  1.90F, 1.95F,  2.00F, 2.05F,
      2.10F, 2.15F,  2.20F, 2.25F,  2.30F, 2.35F,  2.40F, 2.45F,  2.50F, 2.55F,
      2.60F, 2.65F,  2.70F, 2.75F,  2.80F, 2.85F,  2.90F, 2.95F,  3.00F, 3.10F,
      3.20F, 3.30F,  3.40F, 3.50F,  3.60F, 3.70F,  3.80F, 3.90F,  4.00F, 4.20F,
      4.40F, 4.60F,  4.80F, 5.00F};
  const std::array<float, kG7MachsSize> kG7DragCoefficients = {
      0.1198F, 0.1197F, 0.1196F, 0.1194F, 0.1193F, 0.1194F, 0.1194F, 0.1194F,
      0.1193F, 0.1193F, 0.1194F, 0.1193F, 0.1194F, 0.1197F, 0.1202F, 0.1207F,
      0.1215F, 0.1226F, 0.1242F, 0.1266F, 0.1306F, 0.1368F, 0.1464F, 0.1660F,
      0.2054F, 0.2993F, 0.3803F, 0.4015F, 0.4043F, 0.4034F, 0.4014F, 0.3987F,
      0.3955F, 0.3884F, 0.3810F, 0.3732F, 0.3657F, 0.3580F, 0.3440F, 0.3376F,
      0.3315F, 0.3260F, 0.3209F, 0.3160F, 0.3117F, 0.3078F, 0.3042F, 0.3010F,
      0.2980F, 0.2951F, 0.2922F, 0.2892F, 0.2864F, 0.2835F, 0.2807F, 0.2779F,
      0.2752F, 0.2725F, 0.2697F, 0.2670F, 0.2643F, 0.2615F, 0.2588F, 0.2561F,
      0.2533F, 0.2506F, 0.2479F, 0.2451F, 0.2424F, 0.2368F, 0.2313F, 0.2258F,
      0.2205F, 0.2154F, 0.2106F, 0.2060F, 0.2017F, 0.1975F, 0.1935F, 0.1861F,
      0.1793F, 0.1730F, 0.1672F, 0.1618F};

  std::vector<uint16_t> drags;
  drags.reserve(lob::kTableSize);
  for (size_t i = 0; i < lob::kTableSize; i++) {
    const double kMach =
        static_cast<double>(lob::kMachs.at(i)) / lob::kTableScale;
    const double kDrag =
        lob::tables::LobQerp(kG7Machs, kG7DragCoefficients, kMach);
    drags.push_back(
        static_cast<uint16_t>(std::round(kDrag * lob::kTableScale)));
  }

  bool table_is_valid = drags.size() == lob::kTableSize;
  for (size_t i = 0; i < lob::kTableSize; i++) {
    if (drags.at(i) != lob::kG7Drags.at(i)) {
      table_is_valid = false;
      break;
    }
  }
  std::stringstream ss;
  for (auto drag : drags) {
    ss << std::fixed << std::setprecision(0) << drag << "U ,";
  }
  ss << "\n";
  EXPECT_TRUE(table_is_valid) << ss.str();

  for (size_t i = 0; i < kG7MachsSize && table_is_valid; i++) {
    const auto kMach =
        static_cast<uint16_t>(std::round(kG7Machs.at(i) * lob::kTableScale));
    const auto kDrag = static_cast<uint16_t>(
        std::round(lob::LobLerp(lob::kMachs, lob::kG7Drags, kMach)));
    const auto kExpectedDrag = static_cast<uint16_t>(
        std::round(kG7DragCoefficients.at(i) * lob::kTableScale));
    EXPECT_EQ(kDrag, kExpectedDrag)
        << kDrag << " != " << kExpectedDrag << " at " << i << "\n";
  }
}

TEST(TableTests, DeriveMachDragTableG8) {
  // Data is from Robert McCoy/BRL via JBM Ballistics
  // https://jbmballistics.com/ballistics/downloads/text/mcg8.txt
  const size_t kG8MachsSize = 78U;
  const std::array<float, kG8MachsSize> kG8Machs = {
      0.0F,  0.05F,  0.1F,  0.15F,  0.2F,  0.25F,  0.3F,  0.35F,  0.4F,  0.45F,
      0.5F,  0.55F,  0.6F,  0.65F,  0.7F,  0.75F,  0.8F,  0.825F, 0.85F, 0.875F,
      0.9F,  0.925F, 0.95F, 0.975F, 1.0F,  1.025F, 1.05F, 1.075F, 1.1F,  1.125F,
      1.15F, 1.2F,   1.25F, 1.3F,   1.35F, 1.4F,   1.45F, 1.5F,   1.55F, 1.6F,
      1.65F, 1.7F,   1.75F, 1.8F,   1.85F, 1.9F,   1.95F, 2.0F,   2.05F, 2.1F,
      2.15F, 2.2F,   2.25F, 2.3F,   2.35F, 2.4F,   2.45F, 2.5F,   2.6F,  2.7F,
      2.8F,  2.9F,   3.0F,  3.1F,   3.2F,  3.3F,   3.4F,  3.5F,   3.6F,  3.7F,
      3.8F,  3.9F,   4.0F,  4.2F,   4.4F,  4.6F,   4.8F,  5.0};
  const std::array<float, kG8MachsSize> kG8DragCoefficients = {
      0.2105F, 0.2105F, 0.2104F, 0.2104F, 0.2103F, 0.2103F, 0.2103F, 0.2103F,
      0.2103F, 0.2102F, 0.2102F, 0.2102F, 0.2102F, 0.2102F, 0.2103F, 0.2103F,
      0.2104F, 0.2104F, 0.2105F, 0.2106F, 0.2109F, 0.2183F, 0.2571F, 0.3358F,
      0.4068F, 0.4378F, 0.4476F, 0.4493F, 0.4477F, 0.445F,  0.4419F, 0.4353F,
      0.4283F, 0.4208F, 0.4133F, 0.4059F, 0.3986F, 0.3915F, 0.3845F, 0.3777F,
      0.371F,  0.3645F, 0.3581F, 0.3519F, 0.3458F, 0.34F,   0.3343F, 0.3288F,
      0.3234F, 0.3182F, 0.3131F, 0.3081F, 0.3032F, 0.2983F, 0.2937F, 0.2891F,
      0.2845F, 0.2802F, 0.272F,  0.2642F, 0.2569F, 0.2499F, 0.2432F, 0.2368F,
      0.2308F, 0.2251F, 0.2197F, 0.2147F, 0.2101F, 0.2058F, 0.2019F, 0.1983F,
      0.195F,  0.189F,  0.1837F, 0.1791F, 0.175F,  0.1713F};

  std::vector<uint16_t> drags;
  drags.reserve(lob::kTableSize);
  for (size_t i = 0; i < lob::kTableSize; i++) {
    const double kMach =
        static_cast<double>(lob::kMachs.at(i)) / lob::kTableScale;
    const double kDrag =
        lob::tables::LobQerp(kG8Machs, kG8DragCoefficients, kMach);
    drags.push_back(
        static_cast<uint16_t>(std::round(kDrag * lob::kTableScale)));
  }

  bool table_is_valid = drags.size() == lob::kTableSize;
  for (size_t i = 0; i < lob::kTableSize; i++) {
    if (drags.at(i) != lob::kG8Drags.at(i)) {
      table_is_valid = false;
      break;
    }
  }
  std::stringstream ss;
  for (auto drag : drags) {
    ss << std::fixed << std::setprecision(0) << drag << "U ,";
  }
  ss << "\n";
  EXPECT_TRUE(table_is_valid) << ss.str();

  for (size_t i = 0; i < kG8MachsSize && table_is_valid; i++) {
    const auto kMach =
        static_cast<uint16_t>(std::round(kG8Machs.at(i) * lob::kTableScale));
    const auto kDrag = static_cast<uint16_t>(
        std::round(lob::LobLerp(lob::kMachs, lob::kG8Drags, kMach)));
    const auto kExpectedDrag = static_cast<uint16_t>(
        std::round(kG8DragCoefficients.at(i) * lob::kTableScale));
    EXPECT_EQ(kDrag, kExpectedDrag)
        << kDrag << " != " << kExpectedDrag << " at " << i << "\n";
  }
}

}  // namespace tests

// This file is part of lob.
//
// lob is free software: you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version.
//
// lob is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
// A PARTICULAR PURPOSE. See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along with
// lob. If not, see <https://www.gnu.org/licenses/>.