// This file is a part of lob, an exterior ballistics calculation library
// Copyright (c) 2025  Joel Benway
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

    const float kTestBC = 0.232F;
    const lob::DragFunctionT kDragFunction = lob::DragFunctionT::kG7;
    const float kTestDiameter = 0.308F;
    const float kTestWeight = 155.0F;
    const uint16_t kTestMuzzleVelocity = 2800;
    const float kTestZeroAngle = 3.66F;
    const float kTestOpticHeight = 1.5F;

    puut->BallisticCoefficentPsi(kTestBC)
        .BCDragFunction(kDragFunction)
        .BCAtmosphere(lob::AtmosphereReferenceT::kIcao)
        .DiameterInch(kTestDiameter)
        .MassGrains(kTestWeight)
        .InitialVelocityFps(kTestMuzzleVelocity)
        .ZeroAngleMOA(kTestZeroAngle)
        .OpticHeightInches(kTestOpticHeight);
  }

  void TearDown() override { puut.reset(); }
};

TEST_F(LobEnvTestFixture, ZeroAngleSearch) {
  ASSERT_NE(puut, nullptr);
  auto input1 = puut->Build();
  const float kZeroRange = 100.0F;
  auto input2 = puut->ZeroAngleMOA(std::numeric_limits<double>::quiet_NaN())
                    .ZeroDistanceYds(kZeroRange)
                    .Build();
  const float kError = 0.01F;
  EXPECT_NEAR(input1.zero_angle, input2.zero_angle, kError);
}

TEST_F(LobEnvTestFixture, GetSpeedOfSoundFps) {
  ASSERT_NE(puut, nullptr);
  const auto kInput = puut->Build();
  const float kExpectedFps = 1116.45F;
  const float kError = 0.001F;
  EXPECT_NEAR(kInput.speed_of_sound, kExpectedFps, kError);
}

TEST_F(LobEnvTestFixture, SolveAtICAOAtmosphere) {
  ASSERT_NE(puut, nullptr);
  constexpr uint16_t kTestStepSize = 100;
  constexpr uint16_t kVelocityError = 1;
  constexpr uint16_t kEnergyError = 5;
  constexpr double kMoaError = 0.1;
  constexpr double kTimeOfFlightError = 0.01;
  constexpr size_t kSolutionLength = 12;
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0, 150, 300, 600, 900, 1200, 1500, 1800, 2100, 2400, 2700, 3000};
  const std::vector<lob::Output> kExpected = {
      {0, 2800, 2696, -1.50F, 0.00F, 0.000F},
      {150, 2699, 2505, -0.15F, 0.00F, 0.055F},
      {300, 2600, 2325, 0.00F, 0.00F, 0.111F},
      {600, 2409, 1995, -3.64F, 0.00F, 0.231F},
      {900, 2225, 1703, -13.30F, 0.00F, 0.361F},
      {1200, 2051, 1446, -29.98F, 0.00F, 0.501F},
      {1500, 1883, 1220, -54.96F, 0.00F, 0.654F},
      {1800, 1723, 1021, -89.76F, 0.00F, 0.820F},
      {2100, 1569, 846, -136.31F, 0.00F, 1.003F},
      {2400, 1421, 694, -197.03F, 0.00F, 1.204F},
      {2700, 1280, 564, -275.05F, 0.00F, 1.426F},
      {3000, 1149, 454, -374.36F, 0.00F, 1.674F}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  const lob::Options kOptions = {0, 0, lob::kNaN, kTestStepSize};
  lob::Solve(puut->Build(), &kRanges, &solutions, kOptions);
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

TEST_F(LobEnvTestFixture, SolveWithAltitude4500ft) {
  const int32_t kAltitude = 4500;
  const auto kInput = puut->AltitudeOfFiringSiteFt(kAltitude)
                          .TemperatureDegF(lob::kIsaSeaLevelDegF)
                          .Build();
  constexpr uint16_t kTestStepSize = 100;
  constexpr uint16_t kVelocityError = 1;
  constexpr uint16_t kEnergyError = 5;
  constexpr double kMoaError = 0.1;
  constexpr double kTimeOfFlightError = 0.01;
  constexpr size_t kSolutionLength = 12;
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0, 150, 300, 600, 900, 1200, 1500, 1800, 2100, 2400, 2700, 3000};
  const std::vector<lob::Output> kExpected = {
      {0, 2800, 2696, -1.50F, 0.00F, 0.000F},
      {150, 2714, 2533, -0.15F, 0.00F, 0.054F},
      {300, 2630, 2379, 0.02F, 0.00F, 0.111F},
      {600, 2466, 2091, -3.49F, 0.00F, 0.228F},
      {900, 2308, 1831, -12.72F, 0.00F, 0.354F},
      {1200, 2156, 1598, -28.49F, 0.00F, 0.488F},
      {1500, 2010, 1389, -51.76F, 0.00F, 0.633F},
      {1800, 1869, 1201, -83.62F, 0.00F, 0.787F},
      {2100, 1733, 1033, -125.48F, 0.00F, 0.954F},
      {2400, 1602, 882, -178.94F, 0.00F, 1.134F},
      {2700, 1475, 748, -245.97F, 0.00F, 1.329F},
      {3000, 1353, 629, -329.05F, 0.00F, 1.542F}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  const lob::Options kOptions = {0, 0, lob::kNaN, kTestStepSize};
  lob::Solve(kInput, &kRanges, &solutions, kOptions);
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

TEST_F(LobEnvTestFixture, SolveWithTempAndAirPressure) {
  const int32_t kTemperature = 100;
  const int32_t kAirPressure = 25;
  const auto kInput =
      puut->TemperatureDegF(kTemperature).AirPressureInHg(kAirPressure).Build();
  constexpr uint16_t kTestStepSize = 100;
  constexpr uint16_t kVelocityError = 1;
  constexpr uint16_t kEnergyError = 5;
  constexpr double kMoaError = 0.1;
  constexpr double kTimeOfFlightError = 0.01;
  constexpr size_t kSolutionLength = 12;
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0, 150, 300, 600, 900, 1200, 1500, 1800, 2100, 2400, 2700, 3000};
  const std::vector<lob::Output> kExpected = {
      {0, 2800, 2696, -1.50F, 0.00F, 0.000F},
      {150, 2720, 2544, -0.15F, 0.00F, 0.054F},
      {300, 2642, 2400, 0.03F, 0.00F, 0.110F},
      {600, 2489, 2130, -3.43F, 0.00F, 0.227F},
      {900, 2341, 1885, -12.50F, 0.00F, 0.351F},
      {1200, 2199, 1662, -27.94F, 0.00F, 0.484F},
      {1500, 2061, 1461, -50.56F, 0.00F, 0.625F},
      {1800, 1929, 1279, -81.36F, 0.00F, 0.775F},
      {2100, 1800, 1114, -121.56F, 0.00F, 0.936F},
      {2400, 1675, 965, -172.49F, 0.00F, 1.109F},
      {2700, 1554, 830, -235.82F, 0.00F, 1.295F},
      {3000, 1437, 710, -313.59F, 0.00F, 1.496F}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  const lob::Options kOptions = {0, 0, lob::kNaN, kTestStepSize};
  lob::Solve(kInput, &kRanges, &solutions, kOptions);
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

TEST_F(LobEnvTestFixture, SolveWithBarometricPressure) {
  const int32_t kAltitude = 5'280;
  const int32_t kAirPressure = 30;
  const int32_t kTemperature = 59;
  const auto kInput = puut->AltitudeOfFiringSiteFt(kAltitude)
                          .AirPressureInHg(kAirPressure)
                          .AltitudeOfBarometerFt(0)
                          .TemperatureDegF(kTemperature)
                          .Build();
  constexpr uint16_t kTestStepSize = 100;
  constexpr uint16_t kVelocityError = 1;
  constexpr uint16_t kEnergyError = 5;
  constexpr double kMoaError = 0.1;
  constexpr double kTimeOfFlightError = 0.01;
  constexpr size_t kSolutionLength = 12;
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0, 150, 300, 600, 900, 1200, 1500, 1800, 2100, 2400, 2700, 3000};
  const std::vector<lob::Output> kExpected = {
      {0, 2800, 2696, -1.50F, 0.00F, 0.000F},
      {150, 2716, 2537, -0.15F, 0.00F, 0.054F},
      {300, 2634, 2385, 0.02F, 0.00F, 0.110F},
      {600, 2472, 2102, -3.47F, 0.00F, 0.228F},
      {900, 2317, 1846, -12.66F, 0.00F, 0.353F},
      {1200, 2168, 1615, -28.33F, 0.00F, 0.487F},
      {1500, 2024, 1408, -51.41F, 0.00F, 0.630F},
      {1800, 1885, 1222, -83.00F, 0.00F, 0.784F},
      {2100, 1752, 1055, -124.38F, 0.00F, 0.949F},
      {2400, 1622, 905, -177.12F, 0.00F, 1.127F},
      {2700, 1497, 770, -243.08F, 0.00F, 1.320F},
      {3000, 1376, 651, -324.63F, 0.00F, 1.529F}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  const lob::Options kOptions = {0, 0, lob::kNaN, kTestStepSize};
  lob::Solve(kInput, &kRanges, &solutions, kOptions);
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

TEST_F(LobEnvTestFixture, SolveWithPressureTempHumidity) {
  const int32_t kAirPressure = 29;
  const int32_t kTemperature = 75;
  const int32_t kRelativeHumidity = 80;
  const auto kInput = puut->AirPressureInHg(kAirPressure)
                          .TemperatureDegF(kTemperature)
                          .RelativeHumidityPercent(kRelativeHumidity)
                          .Build();
  constexpr uint16_t kTestStepSize = 100;
  constexpr uint16_t kVelocityError = 1;
  constexpr uint16_t kEnergyError = 5;
  constexpr double kMoaError = 0.1;
  constexpr double kTimeOfFlightError = 0.01;
  constexpr size_t kSolutionLength = 12;
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0, 150, 300, 600, 900, 1200, 1500, 1800, 2100, 2400, 2700, 3000};
  const std::vector<lob::Output> kExpected = {
      {0, 2800, 2696, -1.50F, 0.00F, 0.000F},
      {150, 2705, 2516, -0.15F, 0.00F, 0.055F},
      {300, 2612, 2346, 0.01F, 0.00F, 0.111F},
      {600, 2431, 2033, -3.58F, 0.00F, 0.230F},
      {900, 2258, 1753, -13.06F, 0.00F, 0.358F},
      {1200, 2092, 1505, -29.38F, 0.00F, 0.496F},
      {1500, 1934, 1285, -53.64F, 0.00F, 0.645F},
      {1800, 1781, 1090, -87.24F, 0.00F, 0.807F},
      {2100, 1633, 917, -131.81F, 0.00F, 0.983F},
      {2400, 1491, 765, -189.49F, 0.00F, 1.175F},
      {2700, 1355, 632, -262.81F, 0.00F, 1.386F},
      {3000, 1227, 517, -355.17F, 0.00F, 1.619F}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  const lob::Options kOptions = {0, 0, lob::kNaN, kTestStepSize};
  const auto kSize = lob::Solve(kInput, &kRanges, &solutions, kOptions);
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