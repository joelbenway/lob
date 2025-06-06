// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <vector>

#include "constants.hpp"
#include "lob/lob.hpp"

namespace tests {

struct LobEnvTestFixture : public testing::Test {
  // Unit under test
  std::unique_ptr<lob::Builder> puut;

  LobEnvTestFixture() : puut(nullptr) {}

  void SetUp() override {
    ASSERT_EQ(puut, nullptr);
    puut = std::make_unique<lob::Builder>();
    ASSERT_NE(puut, nullptr);

    const double kTestBC = 0.232;
    const lob::DragFunctionT kDragFunction = lob::DragFunctionT::kG7;
    const double kTestDiameter = 0.308;
    const double kTestWeight = 155.0;
    const uint16_t kTestMuzzleVelocity = 2800;
    const double kTestZeroAngle = 3.66;
    const double kTestOpticHeight = 1.5;
    const uint16_t kStep = 100U;

    puut->BallisticCoefficientPsi(kTestBC)
        .BCDragFunction(kDragFunction)
        .BCAtmosphere(lob::AtmosphereReferenceT::kIcao)
        .DiameterInch(kTestDiameter)
        .MassGrains(kTestWeight)
        .InitialVelocityFps(kTestMuzzleVelocity)
        .ZeroAngleMOA(kTestZeroAngle)
        .OpticHeightInches(kTestOpticHeight)
        .StepSize(kStep);
  }

  void TearDown() override { puut.reset(); }
};

TEST_F(LobEnvTestFixture, ZeroAngleSearch) {
  ASSERT_NE(puut, nullptr);
  auto input1 = puut->Build();
  const double kZeroRange = 100.0;
  auto input2 = puut->ZeroAngleMOA(std::numeric_limits<double>::quiet_NaN())
                    .ZeroDistanceYds(kZeroRange)
                    .Build();
  const double kError = 0.01;
  EXPECT_NEAR(input1.zero_angle, input2.zero_angle, kError);
}

TEST_F(LobEnvTestFixture, GetSpeedOfSoundFps) {
  ASSERT_NE(puut, nullptr);
  const auto kInput = puut->Build();
  const double kExpectedFps = 1116.45;
  const double kError = 0.001;
  EXPECT_NEAR(kInput.speed_of_sound, kExpectedFps, kError);
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
TEST_F(LobEnvTestFixture, SolveAtICAOAtmosphere) {
  ASSERT_NE(puut, nullptr);
  constexpr uint16_t kVelocityError = 1;
  constexpr uint16_t kEnergyError = 5;
  constexpr double kMoaError = 0.1;
  constexpr double kTimeOfFlightError = 0.01;
  constexpr size_t kSolutionLength = 12;
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0, 150, 300, 600, 900, 1200, 1500, 1800, 2100, 2400, 2700, 3000};
  const std::vector<lob::Output> kExpected = {
      {0, 2800, 2696, -1.50, 0.00, 0.000},
      {150, 2699, 2505, -0.15, 0.00, 0.055},
      {300, 2600, 2325, 0.00, 0.00, 0.111},
      {600, 2409, 1995, -3.64, 0.00, 0.231},
      {900, 2225, 1703, -13.30, 0.00, 0.361},
      {1200, 2051, 1446, -29.98, 0.00, 0.501},
      {1500, 1883, 1220, -54.96, 0.00, 0.654},
      {1800, 1723, 1021, -89.76, 0.00, 0.820},
      {2100, 1569, 846, -136.31, 0.00, 1.003},
      {2400, 1421, 694, -197.03, 0.00, 1.204},
      {2700, 1280, 564, -275.05, 0.00, 1.426},
      {3000, 1149, 454, -374.36, 0.00, 1.674}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  lob::Solve(puut->Build(), kRanges, solutions);
  for (size_t i = 0; i < kSolutionLength; i++) {
    EXPECT_EQ(solutions.at(i).range, kExpected.at(i).range);
    EXPECT_NEAR(solutions.at(i).velocity, kExpected.at(i).velocity,
                kVelocityError);
    EXPECT_NEAR(solutions.at(i).energy, kExpected.at(i).energy, kEnergyError);
    const double kSolutionElevationMoa =
        lob::InchToMoa(solutions.at(i).elevation, solutions.at(i).range);
    const double kExpectedElevationMoa =
        lob::InchToMoa(kExpected.at(i).elevation, kExpected.at(i).range);
    EXPECT_NEAR(kSolutionElevationMoa, kExpectedElevationMoa, kMoaError);

    const double kSolutionDeflectionMoa =
        lob::InchToMoa(solutions.at(i).deflection, solutions.at(i).range);
    const double kExpectedDeflectionMoa =
        lob::InchToMoa(kExpected.at(i).deflection, kExpected.at(i).range);
    EXPECT_NEAR(kSolutionDeflectionMoa, kExpectedDeflectionMoa, kMoaError);
    EXPECT_NEAR(solutions.at(i).time_of_flight, kExpected.at(i).time_of_flight,
                kTimeOfFlightError);
  }
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
TEST_F(LobEnvTestFixture, SolveWithAltitude4500ft) {
  const int32_t kAltitude = 4500;
  const auto kInput = puut->AltitudeOfFiringSiteFt(kAltitude)
                          .TemperatureDegF(lob::kIsaSeaLevelDegF)
                          .Build();
  constexpr uint16_t kVelocityError = 1;
  constexpr uint16_t kEnergyError = 5;
  constexpr double kMoaError = 0.1;
  constexpr double kTimeOfFlightError = 0.01;
  constexpr size_t kSolutionLength = 12;
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0, 150, 300, 600, 900, 1200, 1500, 1800, 2100, 2400, 2700, 3000};
  const std::vector<lob::Output> kExpected = {
      {0, 2800, 2696, -1.50, 0.00, 0.000},
      {150, 2714, 2533, -0.15, 0.00, 0.054},
      {300, 2630, 2379, 0.02, 0.00, 0.111},
      {600, 2466, 2091, -3.49, 0.00, 0.228},
      {900, 2308, 1831, -12.72, 0.00, 0.354},
      {1200, 2156, 1598, -28.49, 0.00, 0.488},
      {1500, 2010, 1389, -51.76, 0.00, 0.633},
      {1800, 1869, 1201, -83.62, 0.00, 0.787},
      {2100, 1733, 1033, -125.48, 0.00, 0.954},
      {2400, 1602, 882, -178.94, 0.00, 1.134},
      {2700, 1475, 748, -245.97, 0.00, 1.329},
      {3000, 1353, 629, -329.05, 0.00, 1.542}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  lob::Solve(kInput, kRanges, solutions);
  for (size_t i = 0; i < kSolutionLength; i++) {
    EXPECT_EQ(solutions.at(i).range, kExpected.at(i).range);
    EXPECT_NEAR(solutions.at(i).velocity, kExpected.at(i).velocity,
                kVelocityError);
    EXPECT_NEAR(solutions.at(i).energy, kExpected.at(i).energy, kEnergyError);
    const double kSolutionElevationMoa =
        lob::InchToMoa(solutions.at(i).elevation, solutions.at(i).range);
    const double kExpectedElevationMoa =
        lob::InchToMoa(kExpected.at(i).elevation, kExpected.at(i).range);
    EXPECT_NEAR(kSolutionElevationMoa, kExpectedElevationMoa, kMoaError);
    const double kSolutionDeflectionMoa =
        lob::InchToMoa(solutions.at(i).deflection, solutions.at(i).range);
    const double kExpectedDeflectionMoa =
        lob::InchToMoa(kExpected.at(i).deflection, kExpected.at(i).range);
    EXPECT_NEAR(kSolutionDeflectionMoa, kExpectedDeflectionMoa, kMoaError);
    EXPECT_NEAR(solutions.at(i).time_of_flight, kExpected.at(i).time_of_flight,
                kTimeOfFlightError);
  }
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
TEST_F(LobEnvTestFixture, SolveWithTempAndAirPressure) {
  const int32_t kTemperature = 100;
  const int32_t kAirPressure = 25;
  const auto kInput =
      puut->TemperatureDegF(kTemperature).AirPressureInHg(kAirPressure).Build();
  constexpr uint16_t kVelocityError = 1;
  constexpr uint16_t kEnergyError = 5;
  constexpr double kMoaError = 0.1;
  constexpr double kTimeOfFlightError = 0.01;
  constexpr size_t kSolutionLength = 12;
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0, 150, 300, 600, 900, 1200, 1500, 1800, 2100, 2400, 2700, 3000};
  const std::vector<lob::Output> kExpected = {
      {0, 2800, 2696, -1.50, 0.00, 0.000},
      {150, 2720, 2544, -0.15, 0.00, 0.054},
      {300, 2642, 2400, 0.03, 0.00, 0.110},
      {600, 2489, 2130, -3.43, 0.00, 0.227},
      {900, 2341, 1885, -12.50, 0.00, 0.351},
      {1200, 2199, 1662, -27.94, 0.00, 0.484},
      {1500, 2061, 1461, -50.56, 0.00, 0.625},
      {1800, 1929, 1279, -81.36, 0.00, 0.775},
      {2100, 1800, 1114, -121.56, 0.00, 0.936},
      {2400, 1675, 965, -172.49, 0.00, 1.109},
      {2700, 1554, 830, -235.82, 0.00, 1.295},
      {3000, 1437, 710, -313.59, 0.00, 1.496}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  lob::Solve(kInput, kRanges, solutions);
  for (size_t i = 0; i < kSolutionLength; i++) {
    EXPECT_EQ(solutions.at(i).range, kExpected.at(i).range);
    EXPECT_NEAR(solutions.at(i).velocity, kExpected.at(i).velocity,
                kVelocityError);
    EXPECT_NEAR(solutions.at(i).energy, kExpected.at(i).energy, kEnergyError);
    const double kSolutionElevationMoa =
        lob::InchToMoa(solutions.at(i).elevation, solutions.at(i).range);
    const double kExpectedElevationMoa =
        lob::InchToMoa(kExpected.at(i).elevation, kExpected.at(i).range);
    EXPECT_NEAR(kSolutionElevationMoa, kExpectedElevationMoa, kMoaError);
    const double kSolutionDeflectionMoa =
        lob::InchToMoa(solutions.at(i).deflection, solutions.at(i).range);
    const double kExpectedDeflectionMoa =
        lob::InchToMoa(kExpected.at(i).deflection, kExpected.at(i).range);
    EXPECT_NEAR(kSolutionDeflectionMoa, kExpectedDeflectionMoa, kMoaError);
    EXPECT_NEAR(solutions.at(i).time_of_flight, kExpected.at(i).time_of_flight,
                kTimeOfFlightError);
  }
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
TEST_F(LobEnvTestFixture, SolveWithBarometricPressure) {
  const int32_t kAltitude = 5'280;
  const int32_t kAirPressure = 30;
  const int32_t kTemperature = 59;
  const auto kInput = puut->AltitudeOfFiringSiteFt(kAltitude)
                          .AirPressureInHg(kAirPressure)
                          .AltitudeOfBarometerFt(0)
                          .TemperatureDegF(kTemperature)
                          .Build();
  constexpr uint16_t kVelocityError = 1;
  constexpr uint16_t kEnergyError = 5;
  constexpr double kMoaError = 0.1;
  constexpr double kTimeOfFlightError = 0.01;
  constexpr size_t kSolutionLength = 12;
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0, 150, 300, 600, 900, 1200, 1500, 1800, 2100, 2400, 2700, 3000};
  const std::vector<lob::Output> kExpected = {
      {0, 2800, 2696, -1.50, 0.00, 0.000},
      {150, 2716, 2537, -0.15, 0.00, 0.054},
      {300, 2634, 2385, 0.02, 0.00, 0.110},
      {600, 2472, 2102, -3.47, 0.00, 0.228},
      {900, 2317, 1846, -12.66, 0.00, 0.353},
      {1200, 2168, 1615, -28.33, 0.00, 0.487},
      {1500, 2024, 1408, -51.41, 0.00, 0.630},
      {1800, 1885, 1222, -83.00, 0.00, 0.784},
      {2100, 1752, 1055, -124.38, 0.00, 0.949},
      {2400, 1622, 905, -177.12, 0.00, 1.127},
      {2700, 1497, 770, -243.08, 0.00, 1.320},
      {3000, 1376, 651, -324.63, 0.00, 1.529}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  lob::Solve(kInput, kRanges, solutions);
  for (size_t i = 0; i < kSolutionLength; i++) {
    EXPECT_EQ(solutions.at(i).range, kExpected.at(i).range);
    EXPECT_NEAR(solutions.at(i).velocity, kExpected.at(i).velocity,
                kVelocityError);
    EXPECT_NEAR(solutions.at(i).energy, kExpected.at(i).energy, kEnergyError);
    const double kSolutionElevationMoa =
        lob::InchToMoa(solutions.at(i).elevation, solutions.at(i).range);
    const double kExpectedElevationMoa =
        lob::InchToMoa(kExpected.at(i).elevation, kExpected.at(i).range);
    EXPECT_NEAR(kSolutionElevationMoa, kExpectedElevationMoa, kMoaError);
    const double kSolutionDeflectionMoa =
        lob::InchToMoa(solutions.at(i).deflection, solutions.at(i).range);
    const double kExpectedDeflectionMoa =
        lob::InchToMoa(kExpected.at(i).deflection, kExpected.at(i).range);
    EXPECT_NEAR(kSolutionDeflectionMoa, kExpectedDeflectionMoa, kMoaError);
    EXPECT_NEAR(solutions.at(i).time_of_flight, kExpected.at(i).time_of_flight,
                kTimeOfFlightError);
  }
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
TEST_F(LobEnvTestFixture, SolveWithPressureTempHumidity) {
  const int32_t kAirPressure = 29;
  const int32_t kTemperature = 75;
  const int32_t kRelativeHumidity = 80;
  const auto kInput = puut->AirPressureInHg(kAirPressure)
                          .TemperatureDegF(kTemperature)
                          .RelativeHumidityPercent(kRelativeHumidity)
                          .Build();
  constexpr uint16_t kVelocityError = 1;
  constexpr uint16_t kEnergyError = 5;
  constexpr double kMoaError = 0.1;
  constexpr double kTimeOfFlightError = 0.01;
  constexpr size_t kSolutionLength = 12;
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0, 150, 300, 600, 900, 1200, 1500, 1800, 2100, 2400, 2700, 3000};
  const std::vector<lob::Output> kExpected = {
      {0, 2800, 2696, -1.50, 0.00, 0.000},
      {150, 2705, 2516, -0.15, 0.00, 0.055},
      {300, 2612, 2346, 0.01, 0.00, 0.111},
      {600, 2431, 2033, -3.58, 0.00, 0.230},
      {900, 2258, 1753, -13.06, 0.00, 0.358},
      {1200, 2092, 1505, -29.38, 0.00, 0.496},
      {1500, 1934, 1285, -53.64, 0.00, 0.645},
      {1800, 1781, 1090, -87.24, 0.00, 0.807},
      {2100, 1633, 917, -131.81, 0.00, 0.983},
      {2400, 1491, 765, -189.49, 0.00, 1.175},
      {2700, 1355, 632, -262.81, 0.00, 1.386},
      {3000, 1227, 517, -355.17, 0.00, 1.619}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  const auto kSize = lob::Solve(kInput, kRanges, solutions);
  EXPECT_EQ(kSize, kSolutionLength);
  for (size_t i = 0; i < kSolutionLength; i++) {
    EXPECT_EQ(solutions.at(i).range, kExpected.at(i).range);
    EXPECT_NEAR(solutions.at(i).velocity, kExpected.at(i).velocity,
                kVelocityError);
    EXPECT_NEAR(solutions.at(i).energy, kExpected.at(i).energy, kEnergyError);
    const double kSolutionElevationMoa =
        lob::InchToMoa(solutions.at(i).elevation, solutions.at(i).range);
    const double kExpectedElevationMoa =
        lob::InchToMoa(kExpected.at(i).elevation, kExpected.at(i).range);
    EXPECT_NEAR(kSolutionElevationMoa, kExpectedElevationMoa, kMoaError);
    const double kSolutionDeflectionMoa =
        lob::InchToMoa(solutions.at(i).deflection, solutions.at(i).range);
    const double kExpectedDeflectionMoa =
        lob::InchToMoa(kExpected.at(i).deflection, kExpected.at(i).range);
    EXPECT_NEAR(kSolutionDeflectionMoa, kExpectedDeflectionMoa, kMoaError);
    EXPECT_NEAR(solutions.at(i).time_of_flight, kExpected.at(i).time_of_flight,
                kTimeOfFlightError);
  }
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
TEST_F(LobEnvTestFixture, SolveWithWeatherStationData) {
  const double kAltitudeOfFiringSite = 5'280.0;
  const double kAirPressure = 30.0;
  const double kAltitudeOfBarometer = 0.0;
  const double kTemperature = 65.0;
  const double kAltitudeOfThermometer = 3'598.0;
  const auto kInput = puut->AltitudeOfFiringSiteFt(kAltitudeOfFiringSite)
                          .AirPressureInHg(kAirPressure)
                          .AltitudeOfBarometerFt(kAltitudeOfBarometer)
                          .TemperatureDegF(kTemperature)
                          .AltitudeOfThermometerFt(kAltitudeOfThermometer)
                          .Build();
  constexpr uint16_t kVelocityError = 1;
  constexpr uint16_t kEnergyError = 5;
  constexpr double kMoaError = 0.1;
  constexpr double kTimeOfFlightError = 0.01;
  constexpr size_t kSolutionLength = 12;
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0, 150, 300, 600, 900, 1200, 1500, 1800, 2100, 2400, 2700, 3000};
  const std::vector<lob::Output> kExpected = {
      {0, 2800, 2696, -1.50, 0.00, 0.000},
      {150, 2716, 2537, -0.15, 0.00, 0.054},
      {300, 2634, 2385, 0.02, 0.00, 0.110},
      {600, 2472, 2102, -3.47, 0.00, 0.228},
      {900, 2317, 1846, -12.66, 0.00, 0.353},
      {1200, 2168, 1615, -28.33, 0.00, 0.487},
      {1500, 2024, 1408, -51.41, 0.00, 0.630},
      {1800, 1885, 1222, -83.00, 0.00, 0.784},
      {2100, 1752, 1055, -124.38, 0.00, 0.949},
      {2400, 1622, 905, -177.12, 0.00, 1.127},
      {2700, 1497, 770, -243.08, 0.00, 1.320},
      {3000, 1376, 651, -324.63, 0.00, 1.529}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  lob::Solve(kInput, kRanges, solutions);
  for (size_t i = 0; i < kSolutionLength; i++) {
    EXPECT_EQ(solutions.at(i).range, kExpected.at(i).range);
    EXPECT_NEAR(solutions.at(i).velocity, kExpected.at(i).velocity,
                kVelocityError);
    EXPECT_NEAR(solutions.at(i).energy, kExpected.at(i).energy, kEnergyError);
    const double kSolutionElevationMoa =
        lob::InchToMoa(solutions.at(i).elevation, solutions.at(i).range);
    const double kExpectedElevationMoa =
        lob::InchToMoa(kExpected.at(i).elevation, kExpected.at(i).range);
    EXPECT_NEAR(kSolutionElevationMoa, kExpectedElevationMoa, kMoaError);
    const double kSolutionDeflectionMoa =
        lob::InchToMoa(solutions.at(i).deflection, solutions.at(i).range);
    const double kExpectedDeflectionMoa =
        lob::InchToMoa(kExpected.at(i).deflection, kExpected.at(i).range);
    EXPECT_NEAR(kSolutionDeflectionMoa, kExpectedDeflectionMoa, kMoaError);
    EXPECT_NEAR(solutions.at(i).time_of_flight, kExpected.at(i).time_of_flight,
                kTimeOfFlightError);
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