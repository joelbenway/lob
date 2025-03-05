// This file is a part of lob, an exterior ballistics calculation library
// Copyright (c) 2025  Joel Benway
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
#include <vector>

#include "constants.hpp"

namespace tests {

TEST(TableTests, DeriveMachDragTableMachs) {
  // Data is directly from JBM Ballistics
  // https://jbmballistics.com/ballistics/downloads/text/mcg1.txt
  // https://jbmballistics.com/ballistics/downloads/text/mcg7.txt
  const size_t kG1MachsSize = 79U;
  const std::array<float, kG1MachsSize> kG1Machs = {
      0.00,  0.05, 0.10,  0.15, 0.20,  0.25, 0.30,  0.35, 0.40,  0.45,
      0.50,  0.55, 0.60,  0.70, 0.725, 0.75, 0.775, 0.80, 0.825, 0.85,
      0.875, 0.90, 0.925, 0.95, 0.975, 1.0,  1.025, 1.05, 1.075, 1.10,
      1.125, 1.15, 1.20,  1.25, 1.30,  1.35, 1.40,  1.45, 1.50,  1.55,
      1.60,  1.65, 1.70,  1.75, 1.80,  1.85, 1.90,  1.95, 2.00,  2.05,
      2.10,  2.15, 2.20,  2.25, 2.30,  2.35, 2.40,  2.45, 2.50,  2.60,
      2.70,  2.80, 2.90,  3.00, 3.10,  3.20, 3.30,  3.40, 3.50,  3.60,
      3.70,  3.80, 3.90,  4.00, 4.20,  4.40, 4.60,  4.80, 5.00};
  const size_t kG7MachsSize = 84U;
  const std::array<float, kG7MachsSize> kG7Machs = {
      0.00, 0.05,  0.10, 0.15,  0.20,  0.25,  0.30,  0.35,  0.40,  0.45,  0.50,
      0.55, 0.60,  0.65, 0.70,  0.725, 0.75,  0.775, 0.80,  0.825, 0.85,  0.875,
      0.90, 0.925, 0.95, 0.975, 1.00,  1.025, 1.05,  1.075, 1.10,  1.125, 1.15,
      1.20, 1.25,  1.30, 1.35,  1.40,  1.50,  1.55,  1.60,  1.65,  1.70,  1.75,
      1.80, 1.85,  1.90, 1.95,  2.00,  2.05,  2.10,  2.15,  2.20,  2.25,  2.30,
      2.35, 2.40,  2.45, 2.50,  2.55,  2.60,  2.65,  2.70,  2.75,  2.80,  2.85,
      2.90, 2.95,  3.00, 3.10,  3.20,  3.30,  3.40,  3.50,  3.60,  3.70,  3.80,
      3.90, 4.00,  4.20, 4.40,  4.60,  4.80,  5.00};
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
  EXPECT_TRUE(kTableIsCorrectSize);
  bool tables_are_equal = true;
  for (size_t i = 0; i < lob::kMachs.size(); i++) {
    if (machs.at(i) != lob::kMachs.at(i)) {
      tables_are_equal = false;
      break;
    }
  }
  EXPECT_TRUE(tables_are_equal);

  if (!kTableIsCorrectSize || !tables_are_equal) {
    for (auto mach : machs) {
      std::cout << std::fixed << std::setprecision(0) << mach << "U ,";
    }
    std::cout << "\n";
  }
}

TEST(TableTests, DeriveMachDragTableG1) {
  // Data is directly from JBM Ballistics
  // https://jbmballistics.com/ballistics/downloads/text/mcg1.txt
  const size_t kG1MachsSize = 79U;
  const std::array<float, kG1MachsSize> kG1Machs = {
      0.00,  0.05, 0.10,  0.15, 0.20,  0.25, 0.30,  0.35, 0.40,  0.45,
      0.50,  0.55, 0.60,  0.70, 0.725, 0.75, 0.775, 0.80, 0.825, 0.85,
      0.875, 0.90, 0.925, 0.95, 0.975, 1.0,  1.025, 1.05, 1.075, 1.10,
      1.125, 1.15, 1.20,  1.25, 1.30,  1.35, 1.40,  1.45, 1.50,  1.55,
      1.60,  1.65, 1.70,  1.75, 1.80,  1.85, 1.90,  1.95, 2.00,  2.05,
      2.10,  2.15, 2.20,  2.25, 2.30,  2.35, 2.40,  2.45, 2.50,  2.60,
      2.70,  2.80, 2.90,  3.00, 3.10,  3.20, 3.30,  3.40, 3.50,  3.60,
      3.70,  3.80, 3.90,  4.00, 4.20,  4.40, 4.60,  4.80, 5.00};
  const std::array<float, kG1MachsSize> kG1DragCoefficents = {
      0.2629, 0.2558, 0.2487, 0.2413, 0.2344, 0.2278, 0.2214, 0.2155, 0.2104,
      0.2061, 0.2032, 0.2020, 0.2034, 0.2165, 0.2230, 0.2313, 0.2417, 0.2546,
      0.2706, 0.2901, 0.3136, 0.3415, 0.3734, 0.4084, 0.4448, 0.4805, 0.5136,
      0.5427, 0.5677, 0.5883, 0.6053, 0.6191, 0.6393, 0.6518, 0.6589, 0.6621,
      0.6625, 0.6607, 0.6573, 0.6528, 0.6474, 0.6413, 0.6347, 0.6280, 0.6210,
      0.6141, 0.6072, 0.6003, 0.5934, 0.5867, 0.5804, 0.5743, 0.5685, 0.5630,
      0.5577, 0.5527, 0.5481, 0.5438, 0.5397, 0.5325, 0.5264, 0.5211, 0.5168,
      0.5133, 0.5105, 0.5084, 0.5067, 0.5054, 0.5040, 0.5030, 0.5022, 0.5016,
      0.5010, 0.5006, 0.4998, 0.4995, 0.4992, 0.4990, 0.4988};

  std::vector<uint16_t> drags;
  drags.reserve(lob::kTableSize);
  for (size_t i = 0; i < lob::kTableSize; i++) {
    const double kMach =
        static_cast<double>(lob::kMachs.at(i)) / lob::kTableScale;
    const double kDrag = lob::LobQerp(kG1Machs, kG1DragCoefficents, kMach);
    drags.push_back(
        static_cast<uint16_t>(std::round(kDrag * lob::kTableScale)));
  }

  const bool kTableIsCorrectSize = drags.size() == lob::kTableSize;
  EXPECT_TRUE(kTableIsCorrectSize);
  bool tables_are_equal = true;
  for (size_t i = 0; i < lob::kTableSize; i++) {
    if (drags.at(i) != lob::kG1Drags.at(i)) {
      tables_are_equal = false;
      std::cout << drags.at(i) << " != " << lob::kG1Drags.at(i) << "\n";
      break;
    }
  }
  EXPECT_TRUE(tables_are_equal);

  if (!kTableIsCorrectSize || !tables_are_equal) {
    for (auto drag : drags) {
      std::cout << std::fixed << std::setprecision(0) << drag << "U ,";
    }
    std::cout << "\n";
  } else {
    for (size_t i = 0; i < kG1MachsSize; i++) {
      const auto kMach =
          static_cast<uint16_t>(std::round(kG1Machs.at(i) * lob::kTableScale));
      const auto kDrag = static_cast<uint16_t>(
          std::round(lob::LobLerp(lob::kMachs, lob::kG1Drags, kMach)));
      const auto kExpectedDrag = static_cast<uint16_t>(
          std::round(kG1DragCoefficents.at(i) * lob::kTableScale));
      EXPECT_EQ(kDrag, kExpectedDrag)
          << kDrag << " != " << kExpectedDrag << " at " << i << "\n";
    }
  }
}

TEST(TableTests, DeriveMachDragTableG2) {
  // Data is directly from JBM Ballistics
  // https://jbmballistics.com/ballistics/downloads/text/mcg2.txt
  const size_t kG2MachsSize = 85U;
  const std::array<float, kG2MachsSize> kG2Machs = {
      0,     0.05, 0.1,   0.15, 0.2,   0.25,  0.3,   0.35,  0.4,   0.45,  0.5,
      0.55,  0.6,  0.65,  0.7,  0.75,  0.775, 0.8,   0.825, 0.85,  0.875, 0.9,
      0.925, 0.95, 0.975, 1,    1.025, 1.05,  1.075, 1.1,   1.125, 1.15,  1.175,
      1.2,   1.25, 1.3,   1.35, 1.4,   1.45,  1.5,   1.55,  1.6,   1.65,  1.7,
      1.75,  1.8,  1.85,  1.9,  1.95,  2,     2.05,  2.1,   2.15,  2.2,   2.25,
      2.3,   2.35, 2.4,   2.45, 2.5,   2.55,  2.6,   2.65,  2.7,   2.75,  2.8,
      2.85,  2.9,  2.95,  3,    3.1,   3.2,   3.3,   3.4,   3.5,   3.6,   3.7,
      3.8,   3.9,  4,     4.2,  4.4,   4.6,   4.80,  5};
  const std::array<float, kG2MachsSize> kG2DragCoefficents = {
      0.2303, 0.2298, 0.2287, 0.2271, 0.2251, 0.2227, 0.2196, 0.2156, 0.2107,
      0.2048, 0.198,  0.1905, 0.1828, 0.1758, 0.1702, 0.1669, 0.1664, 0.1667,
      0.1682, 0.1711, 0.1761, 0.1831, 0.2004, 0.2589, 0.3492, 0.3983, 0.4075,
      0.4103, 0.4114, 0.4106, 0.4089, 0.4068, 0.4046, 0.4021, 0.3966, 0.3904,
      0.3835, 0.3759, 0.3678, 0.3594, 0.3512, 0.3432, 0.3356, 0.3282, 0.3213,
      0.3149, 0.3089, 0.3033, 0.2982, 0.2933, 0.2889, 0.2846, 0.2806, 0.2768,
      0.2731, 0.2696, 0.2663, 0.2632, 0.2602, 0.2572, 0.2543, 0.2515, 0.2487,
      0.246,  0.2433, 0.2408, 0.2382, 0.2357, 0.2333, 0.2309, 0.2262, 0.2217,
      0.2173, 0.2132, 0.2091, 0.2052, 0.2014, 0.1978, 0.1944, 0.1912, 0.1851,
      0.1794, 0.1741, 0.1693, 0.1648};

  std::vector<uint16_t> drags;
  drags.reserve(lob::kTableSize);
  for (size_t i = 0; i < lob::kTableSize; i++) {
    const double kMach =
        static_cast<double>(lob::kMachs.at(i)) / lob::kTableScale;
    const double kDrag = lob::LobQerp(kG2Machs, kG2DragCoefficents, kMach);
    drags.push_back(
        static_cast<uint16_t>(std::round(kDrag * lob::kTableScale)));
  }

  const bool kTableIsCorrectSize = drags.size() == lob::kTableSize;
  EXPECT_TRUE(kTableIsCorrectSize);
  bool tables_are_equal = true;
  for (size_t i = 0; i < lob::kTableSize; i++) {
    if (drags.at(i) != lob::kG2Drags.at(i)) {
      tables_are_equal = false;
      std::cout << drags.at(i) << " != " << lob::kG2Drags.at(i) << "\n";
      break;
    }
  }
  EXPECT_TRUE(tables_are_equal);

  if (!kTableIsCorrectSize || !tables_are_equal) {
    for (auto drag : drags) {
      std::cout << std::fixed << std::setprecision(0) << drag << "U ,";
    }
    std::cout << "\n";
  } else {
    /*for (size_t i = 0; i < kG2MachsSize; i++) {
      const auto kMach =
          static_cast<uint16_t>(std::round(kG2Machs.at(i) * lob::kTableScale));
      const auto kDrag = static_cast<uint16_t>(
          std::round(lob::LobLerp(lob::kMachs, lob::kG2Drags, kMach)));
      const auto kExpectedDrag = static_cast<uint16_t>(
          std::round(kG2DragCoefficents.at(i) * lob::kTableScale));
      EXPECT_EQ(kDrag, kExpectedDrag)
          << kDrag << " != " << kExpectedDrag << " at " << i << "\n";
    }*/
  }
}

TEST(TableTests, DeriveMachDragTableG5) {
  // Data is directly from JBM Ballistics
  // https://jbmballistics.com/ballistics/downloads/text/mcg5.txt
  const size_t kG5MachsSize = 76U;
  const std::array<float, kG5MachsSize> kG5Machs = {
      0,     0.05, 0.1,   0.15, 0.2,   0.25, 0.3,  0.35,  0.4,  0.45,  0.5,
      0.55,  0.6,  0.65,  0.7,  0.75,  0.8,  0.85, 0.875, 0.9,  0.925, 0.95,
      0.975, 1,    1.025, 1.05, 1.075, 1.1,  1.15, 1.2,   1.25, 1.3,   1.35,
      1.4,   1.45, 1.5,   1.55, 1.6,   1.65, 1.7,  1.75,  1.8,  1.85,  1.9,
      1.95,  2,    2.05,  2.1,  2.15,  2.2,  2.25, 2.3,   2.35, 2.4,   2.45,
      2.5,   2.6,  2.7,   2.8,  2.9,   3,    3.1,  3.2,   3.3,  3.4,   3.5,
      3.6,   3.7,  3.8,   3.9,  4,     4.2,  4.4,  4.6,   4.8,  5};
  const std::array<float, kG5MachsSize> kG5DragCoefficents = {
      0.171,  0.1719, 0.1727, 0.1732, 0.1734, 0.173,  0.1718, 0.1696, 0.1668,
      0.1637, 0.1603, 0.1566, 0.1529, 0.1497, 0.1473, 0.1463, 0.1489, 0.1583,
      0.1672, 0.1815, 0.2051, 0.2413, 0.2884, 0.3379, 0.3785, 0.4032, 0.4147,
      0.4201, 0.4278, 0.4338, 0.4373, 0.4392, 0.4403, 0.4406, 0.4401, 0.4386,
      0.4362, 0.4328, 0.4286, 0.4237, 0.4182, 0.4121, 0.4057, 0.3991, 0.3926,
      0.3861, 0.38,   0.3741, 0.3684, 0.363,  0.3578, 0.3529, 0.3481, 0.3435,
      0.3391, 0.3349, 0.3269, 0.3194, 0.3125, 0.306,  0.2999, 0.2942, 0.2889,
      0.2838, 0.279,  0.2745, 0.2703, 0.2662, 0.2624, 0.2588, 0.2553, 0.2488,
      0.2429, 0.2376, 0.2326, 0.228};

  std::vector<uint16_t> drags;
  drags.reserve(lob::kTableSize);
  for (size_t i = 0; i < lob::kTableSize; i++) {
    const double kMach =
        static_cast<double>(lob::kMachs.at(i)) / lob::kTableScale;
    const double kDrag = lob::LobQerp(kG5Machs, kG5DragCoefficents, kMach);
    drags.push_back(
        static_cast<uint16_t>(std::round(kDrag * lob::kTableScale)));
  }

  const bool kTableIsCorrectSize = drags.size() == lob::kTableSize;
  EXPECT_TRUE(kTableIsCorrectSize);
  bool tables_are_equal = true;
  for (size_t i = 0; i < lob::kTableSize; i++) {
    if (drags.at(i) != lob::kG5Drags.at(i)) {
      tables_are_equal = false;
      std::cout << drags.at(i) << " != " << lob::kG5Drags.at(i) << "\n";
      break;
    }
  }
  EXPECT_TRUE(tables_are_equal);

  if (!kTableIsCorrectSize || !tables_are_equal) {
    for (auto drag : drags) {
      std::cout << std::fixed << std::setprecision(0) << drag << "U ,";
    }
    std::cout << "\n";
  } else {
    /*for (size_t i = 0; i < kG5MachsSize; i++) {
      const auto kMach =
          static_cast<uint16_t>(std::round(kG5Machs.at(i) * lob::kTableScale));
      const auto kDrag = static_cast<uint16_t>(
          std::round(lob::LobLerp(lob::kMachs, lob::kG5Drags, kMach)));
      const auto kExpectedDrag = static_cast<uint16_t>(
          std::round(kG5DragCoefficents.at(i) * lob::kTableScale));
      EXPECT_EQ(kDrag, kExpectedDrag)
          << kDrag << " != " << kExpectedDrag << " at " << i << "\n";
    }*/
  }
}

TEST(TableTests, DeriveMachDragTableG6) {
  // Data is directly from JBM Ballistics
  // https://jbmballistics.com/ballistics/downloads/text/mcg6.txt
  const size_t kG6MachsSize = 79U;
  const std::array<float, kG6MachsSize> kG6Machs = {
      0,     0.05, 0.1,   0.15, 0.2,   0.25, 0.3,   0.35, 0.4,   0.45,
      0.5,   0.55, 0.6,   0.65, 0.7,   0.75, 0.8,   0.85, 0.875, 0.9,
      0.925, 0.95, 0.975, 1,    1.025, 1.05, 1.075, 1.1,  1.125, 1.15,
      1.175, 1.2,  1.225, 1.25, 1.3,   1.35, 1.4,   1.45, 1.5,   1.55,
      1.6,   1.65, 1.7,   1.75, 1.8,   1.85, 1.9,   1.95, 2,     2.05,
      2.1,   2.15, 2.2,   2.25, 2.3,   2.35, 2.4,   2.45, 2.5,   2.6,
      2.7,   2.8,  2.9,   3,    3.1,   3.2,  3.3,   3.4,  3.5,   3.6,
      3.7,   3.8,  3.9,   4,    4.2,   4.4,  4.6,   4.8,  5};
  const std::array<float, kG6MachsSize> kG6DragCoefficents = {
      0.2617, 0.2553, 0.2491, 0.2432, 0.2376, 0.2324, 0.2278, 0.2238, 0.2205,
      0.2177, 0.2155, 0.2138, 0.2126, 0.2121, 0.2122, 0.2132, 0.2154, 0.2194,
      0.2229, 0.2297, 0.2449, 0.2732, 0.3141, 0.3597, 0.3994, 0.4261, 0.4402,
      0.4465, 0.449,  0.4497, 0.4494, 0.4482, 0.4464, 0.4441, 0.439,  0.4336,
      0.4279, 0.4221, 0.4162, 0.4102, 0.4042, 0.3981, 0.3919, 0.3855, 0.3788,
      0.3721, 0.3652, 0.3583, 0.3515, 0.3447, 0.3381, 0.3314, 0.3249, 0.3185,
      0.3122, 0.306,  0.3,    0.2941, 0.2883, 0.2772, 0.2668, 0.2574, 0.2487,
      0.2407, 0.2333, 0.2265, 0.2202, 0.2144, 0.2089, 0.2039, 0.1991, 0.1947,
      0.1905, 0.1866, 0.1794, 0.173,  0.1673, 0.1621, 0.1574};

  std::vector<uint16_t> drags;
  drags.reserve(lob::kTableSize);
  for (size_t i = 0; i < lob::kTableSize; i++) {
    const double kMach =
        static_cast<double>(lob::kMachs.at(i)) / lob::kTableScale;
    const double kDrag = lob::LobQerp(kG6Machs, kG6DragCoefficents, kMach);
    drags.push_back(
        static_cast<uint16_t>(std::round(kDrag * lob::kTableScale)));
  }

  const bool kTableIsCorrectSize = drags.size() == lob::kTableSize;
  EXPECT_TRUE(kTableIsCorrectSize);
  bool tables_are_equal = true;
  for (size_t i = 0; i < lob::kTableSize; i++) {
    if (drags.at(i) != lob::kG6Drags.at(i)) {
      tables_are_equal = false;
      std::cout << drags.at(i) << " != " << lob::kG6Drags.at(i) << "\n";
      break;
    }
  }
  EXPECT_TRUE(tables_are_equal);

  if (!kTableIsCorrectSize || !tables_are_equal) {
    for (auto drag : drags) {
      std::cout << std::fixed << std::setprecision(0) << drag << "U ,";
    }
    std::cout << "\n";
  } else {
    /*for (size_t i = 0; i < kG6MachsSize; i++) {
      const auto kMach =
          static_cast<uint16_t>(std::round(kG6Machs.at(i) * lob::kTableScale));
      const auto kDrag = static_cast<uint16_t>(
          std::round(lob::LobLerp(lob::kMachs, lob::kG6Drags, kMach)));
      const auto kExpectedDrag = static_cast<uint16_t>(
          std::round(kG6DragCoefficents.at(i) * lob::kTableScale));
      EXPECT_EQ(kDrag, kExpectedDrag)
          << kDrag << " != " << kExpectedDrag << " at " << i << "\n";
    }*/
  }
}

TEST(TableTests, DeriveMachDragTableG7) {
  // Data is directly from JBM Ballistics
  // https://jbmballistics.com/ballistics/downloads/text/mcg7.txt
  const size_t kG7MachsSize = 84U;
  const std::array<float, kG7MachsSize> kG7Machs = {
      0.00, 0.05,  0.10, 0.15,  0.20,  0.25,  0.30,  0.35,  0.40,  0.45,  0.50,
      0.55, 0.60,  0.65, 0.70,  0.725, 0.75,  0.775, 0.80,  0.825, 0.85,  0.875,
      0.90, 0.925, 0.95, 0.975, 1.00,  1.025, 1.05,  1.075, 1.10,  1.125, 1.15,
      1.20, 1.25,  1.30, 1.35,  1.40,  1.50,  1.55,  1.60,  1.65,  1.70,  1.75,
      1.80, 1.85,  1.90, 1.95,  2.00,  2.05,  2.10,  2.15,  2.20,  2.25,  2.30,
      2.35, 2.40,  2.45, 2.50,  2.55,  2.60,  2.65,  2.70,  2.75,  2.80,  2.85,
      2.90, 2.95,  3.00, 3.10,  3.20,  3.30,  3.40,  3.50,  3.60,  3.70,  3.80,
      3.90, 4.00,  4.20, 4.40,  4.60,  4.80,  5.00};
  const std::array<float, kG7MachsSize> kG7DragCoefficents = {
      0.1198, 0.1197, 0.1196, 0.1194, 0.1193, 0.1194, 0.1194, 0.1194, 0.1193,
      0.1193, 0.1194, 0.1193, 0.1194, 0.1197, 0.1202, 0.1207, 0.1215, 0.1226,
      0.1242, 0.1266, 0.1306, 0.1368, 0.1464, 0.1660, 0.2054, 0.2993, 0.3803,
      0.4015, 0.4043, 0.4034, 0.4014, 0.3987, 0.3955, 0.3884, 0.3810, 0.3732,
      0.3657, 0.3580, 0.3440, 0.3376, 0.3315, 0.3260, 0.3209, 0.3160, 0.3117,
      0.3078, 0.3042, 0.3010, 0.2980, 0.2951, 0.2922, 0.2892, 0.2864, 0.2835,
      0.2807, 0.2779, 0.2752, 0.2725, 0.2697, 0.2670, 0.2643, 0.2615, 0.2588,
      0.2561, 0.2533, 0.2506, 0.2479, 0.2451, 0.2424, 0.2368, 0.2313, 0.2258,
      0.2205, 0.2154, 0.2106, 0.2060, 0.2017, 0.1975, 0.1935, 0.1861, 0.1793,
      0.1730, 0.1672, 0.1618};

  std::vector<uint16_t> drags;
  drags.reserve(lob::kTableSize);
  for (size_t i = 0; i < lob::kTableSize; i++) {
    const double kMach =
        static_cast<double>(lob::kMachs.at(i)) / lob::kTableScale;
    const double kDrag = lob::LobQerp(kG7Machs, kG7DragCoefficents, kMach);
    drags.push_back(
        static_cast<uint16_t>(std::round(kDrag * lob::kTableScale)));
  }

  const bool kTableIsCorrectSize = drags.size() == lob::kTableSize;
  EXPECT_TRUE(kTableIsCorrectSize);
  bool tables_are_equal = true;
  for (size_t i = 0; i < lob::kTableSize; i++) {
    if (drags.at(i) != lob::kG7Drags.at(i)) {
      tables_are_equal = false;
      std::cout << drags.at(i) << " != " << lob::kG7Drags.at(i) << "\n";
      break;
    }
  }
  EXPECT_TRUE(tables_are_equal);

  if (!kTableIsCorrectSize || !tables_are_equal) {
    for (auto drag : drags) {
      std::cout << std::fixed << std::setprecision(0) << drag << "U ,";
    }
    std::cout << "\n";
  } else {
    for (size_t i = 0; i < kG7MachsSize; i++) {
      const auto kMach =
          static_cast<uint16_t>(std::round(kG7Machs.at(i) * lob::kTableScale));
      const auto kDrag = static_cast<uint16_t>(
          std::round(lob::LobLerp(lob::kMachs, lob::kG7Drags, kMach)));
      const auto kExpectedDrag = static_cast<uint16_t>(
          std::round(kG7DragCoefficents.at(i) * lob::kTableScale));
      EXPECT_EQ(kDrag, kExpectedDrag)
          << kDrag << " != " << kExpectedDrag << " at " << i << "\n";
    }
  }
}

TEST(TableTests, DeriveMachDragTableG8) {
  // Data is directly from JBM Ballistics
  // https://jbmballistics.com/ballistics/downloads/text/mcg8.txt
  const size_t kG8MachsSize = 78U;
  const std::array<float, kG8MachsSize> kG8Machs = {
      0,    0.05,  0.1,  0.15,  0.2,  0.25,  0.3,  0.35,  0.4,  0.45,
      0.5,  0.55,  0.6,  0.65,  0.7,  0.75,  0.8,  0.825, 0.85, 0.875,
      0.9,  0.925, 0.95, 0.975, 1,    1.025, 1.05, 1.075, 1.1,  1.125,
      1.15, 1.2,   1.25, 1.3,   1.35, 1.4,   1.45, 1.5,   1.55, 1.6,
      1.65, 1.7,   1.75, 1.8,   1.85, 1.9,   1.95, 2,     2.05, 2.1,
      2.15, 2.2,   2.25, 2.3,   2.35, 2.4,   2.45, 2.5,   2.6,  2.7,
      2.8,  2.9,   3,    3.1,   3.2,  3.3,   3.4,  3.5,   3.6,  3.7,
      3.8,  3.9,   4,    4.2,   4.4,  4.6,   4.8,  5};
  const std::array<float, kG8MachsSize> kG8DragCoefficents = {
      0.2105, 0.2105, 0.2104, 0.2104, 0.2103, 0.2103, 0.2103, 0.2103, 0.2103,
      0.2102, 0.2102, 0.2102, 0.2102, 0.2102, 0.2103, 0.2103, 0.2104, 0.2104,
      0.2105, 0.2106, 0.2109, 0.2183, 0.2571, 0.3358, 0.4068, 0.4378, 0.4476,
      0.4493, 0.4477, 0.445,  0.4419, 0.4353, 0.4283, 0.4208, 0.4133, 0.4059,
      0.3986, 0.3915, 0.3845, 0.3777, 0.371,  0.3645, 0.3581, 0.3519, 0.3458,
      0.34,   0.3343, 0.3288, 0.3234, 0.3182, 0.3131, 0.3081, 0.3032, 0.2983,
      0.2937, 0.2891, 0.2845, 0.2802, 0.272,  0.2642, 0.2569, 0.2499, 0.2432,
      0.2368, 0.2308, 0.2251, 0.2197, 0.2147, 0.2101, 0.2058, 0.2019, 0.1983,
      0.195,  0.189,  0.1837, 0.1791, 0.175,  0.1713};

  std::vector<uint16_t> drags;
  drags.reserve(lob::kTableSize);
  for (size_t i = 0; i < lob::kTableSize; i++) {
    const double kMach =
        static_cast<double>(lob::kMachs.at(i)) / lob::kTableScale;
    const double kDrag = lob::LobQerp(kG8Machs, kG8DragCoefficents, kMach);
    drags.push_back(
        static_cast<uint16_t>(std::round(kDrag * lob::kTableScale)));
  }

  const bool kTableIsCorrectSize = drags.size() == lob::kTableSize;
  EXPECT_TRUE(kTableIsCorrectSize);
  bool tables_are_equal = true;
  for (size_t i = 0; i < lob::kTableSize; i++) {
    if (drags.at(i) != lob::kG8Drags.at(i)) {
      tables_are_equal = false;
      std::cout << drags.at(i) << " != " << lob::kG8Drags.at(i) << "\n";
      break;
    }
  }
  EXPECT_TRUE(tables_are_equal);

  if (!kTableIsCorrectSize || !tables_are_equal) {
    for (auto drag : drags) {
      std::cout << std::fixed << std::setprecision(0) << drag << "U ,";
    }
    std::cout << "\n";
  } else {
    /*for (size_t i = 0; i < kG8MachsSize; i++) {
      const auto kMach =
          static_cast<uint16_t>(std::round(kG8Machs.at(i) * lob::kTableScale));
      const auto kDrag = static_cast<uint16_t>(
          std::round(lob::LobLerp(lob::kMachs, lob::kG8Drags, kMach)));
      const auto kExpectedDrag = static_cast<uint16_t>(
          std::round(kG8DragCoefficents.at(i) * lob::kTableScale));
      EXPECT_EQ(kDrag, kExpectedDrag)
          << kDrag << " != " << kExpectedDrag << " at " << i << "\n";
    }*/
  }
}

TEST(TableTests, ResizeToEqualTable) {
  static const size_t kNewTableSize = lob::kTableSize;
  std::array<uint16_t, kNewTableSize> new_table_machs = {};
  std::array<uint16_t, kNewTableSize> new_table_drags = {};
  lob::ResizeMachDragTable(lob::kMachs, lob::kG1Drags, &new_table_machs,
                           &new_table_drags);
  for (auto mach : lob::kMachs) {
    const double kOldResult = lob::LobLerp(lob::kMachs, lob::kG1Drags, mach);
    const double kNewResult =
        lob::LobLerp(new_table_machs, new_table_drags, mach);
    EXPECT_DOUBLE_EQ(kOldResult, kNewResult);
  }
}

TEST(TableTests, ResizeToLargerTable) {
  static const uint16_t kError = 100;
  static const size_t kNewTableSize = 200;
  std::array<uint16_t, kNewTableSize> new_table_machs = {};
  std::array<uint16_t, kNewTableSize> new_table_drags = {};
  lob::ResizeMachDragTable(lob::kMachs, lob::kG1Drags, &new_table_machs,
                           &new_table_drags);

  for (auto mach : new_table_machs) {
    const double kOldResult = lob::LobLerp(lob::kMachs, lob::kG1Drags, mach);
    const double kNewResult =
        lob::LobLerp(new_table_machs, new_table_drags, mach);
    EXPECT_NEAR(kOldResult, kNewResult, kError);
  }
}

TEST(TableTests, ResizeToSmallerTable) {
  static const uint16_t kError = 100;
  static const size_t kNewTableSize = 60;
  std::array<uint16_t, kNewTableSize> new_table_machs = {};
  std::array<uint16_t, kNewTableSize> new_table_drags = {};
  lob::ResizeMachDragTable(lob::kMachs, lob::kG1Drags, &new_table_machs,
                           &new_table_drags);
  for (auto mach : lob::kMachs) {
    const double kOldResult = lob::LobLerp(lob::kMachs, lob::kG1Drags, mach);
    const double kNewResult =
        lob::LobLerp(new_table_machs, new_table_drags, mach);
    EXPECT_NEAR(kOldResult, kNewResult, kError);
  }
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