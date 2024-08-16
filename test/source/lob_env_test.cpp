// This file is a part of lob, an exterior ballistics calculation library
// Copyright (c) 2024  Joel Benway
// Please see end of file for extended copyright information

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "lob/lob.hpp"

namespace tests {

struct LobEnvTestFixture : public testing::Test {
  friend class lob::Lob;

  // Unit under test
  std::unique_ptr<lob::Lob> puut;

  LobEnvTestFixture() : puut(nullptr) {}

  void SetUp() override {
    if (puut != nullptr) {
      puut = nullptr;
    }

    const double kTestBC = 0.232;
    const lob::DragFunctionT kDragFunction = lob::DragFunctionT::kG7;
    const double kTestDiameter = 0.308;
    const double kTestWeight = 155.0;
    const double kTestMuzzleVelocity = 2800.0;
    const double kTestZero = 100.0;
    const double kTestOpticHeight = 1.5;
    const double kTestTargetDistance = 1000.0;

    puut = lob::Lob::Builder()
               .BallisticCoefficentPsi(kTestBC)
               .BCDragFunction(kDragFunction)
               .BCAtmosphere(lob::AtmosphereReferenceT::kIcao)
               .DiameterInch(kTestDiameter)
               .MassGrains(kTestWeight)
               .InitialVelocityFps(kTestMuzzleVelocity)
               .ZeroDistanceYds(kTestZero)
               .OpticHeightInches(kTestOpticHeight)
               .TargetDistanceYds(kTestTargetDistance)
               .Build();
  }

  void TearDown() override {
    if (puut == nullptr) {
      return;
    }

    puut = nullptr;
  }
};

TEST_F(LobEnvTestFixture, ZeroAngleSearch) {
  ASSERT_NE(puut, nullptr);
  const float kExpectedZA = 3.66F;
  const float kZA = puut->GetZeroAngleMOA();
  const float kError = 0.1F;
  EXPECT_NEAR(kZA, kExpectedZA, kError);
}

TEST_F(LobEnvTestFixture, GetAirDensityLbsPerCuFt) {
  ASSERT_NE(puut, nullptr);
  const float kExpectedFps = 0.0764742;
  const float kError = 0.001;
  EXPECT_NEAR(puut->GetAirDensityLbsPerCuFt(), kExpectedFps, kError);
}

TEST_F(LobEnvTestFixture, GetSpeedOfSoundFps) {
  ASSERT_NE(puut, nullptr);
  const float kExpectedFps = 1116.45;
  const float kError = 0.001;
  EXPECT_NEAR(puut->GetSpeedOfSoundFps(), kExpectedFps, kError);
}

TEST_F(LobEnvTestFixture, SolveAtICAOAtmosphere) {
  ASSERT_NE(puut, nullptr);
  // NOLINTNEXTLINE c-style array
  const uint16_t kRanges[] = {0,   50,  100, 200, 300, 400,
                              500, 600, 700, 800, 900, 1000};
  const std::vector<lob::Lob::Solution> kExpected = {
      {0, 2800, 2696, -1.5, 0, 0, 0, 0},
      {50, 2699, 2505, -0.2, -0.37, 0, 0, 0.055},
      {100, 2600, 2325, 0, 0, 0, 0, 0.111},
      {200, 2409, 1995, -3.6, -1.72, 0, 0, 0.231},
      {300, 2225, 1703, -13.3, -4.23, 0, 0, 0.361},
      {400, 2051, 1446, -30, -7.16, 0, 0, 0.501},
      {500, 1883, 1220, -55, -10.5, 0, 0, 0.654},
      {600, 1723, 1021, -89.8, -14.29, 0, 0, 0.82},
      {700, 1569, 846, -136.3, -18.59, 0, 0, 1.003},
      {800, 1421, 694, -197, -23.52, 0, 0, 1.204},
      {900, 1280, 564, -275.1, -29.19, 0, 0, 1.426},
      {1000, 1149, 454, -374.4, -35.75, 0, 0, 1.674}};
  constexpr size_t kSolutionLength = 12;
  lob::Lob::Solution solutions[kSolutionLength] = {0};  // NOLINT c-style array
  // NOLINTNEXTLINE implicitly decay an array into a pointer
  const size_t kWritten = puut->Solve(solutions, kRanges, kSolutionLength);
  EXPECT_EQ(kWritten, kSolutionLength);
  for (size_t i = 0; i < kSolutionLength; i++) {
    EXPECT_NEAR(solutions[i].range, kExpected[i].range, 1);
    EXPECT_NEAR(solutions[i].velocity, kExpected[i].velocity, 1);
    EXPECT_NEAR(solutions[i].energy, kExpected[i].energy, 5);
    EXPECT_NEAR(solutions[i].elevation_adjustments,
                kExpected[i].elevation_adjustments, 0.1);
    EXPECT_NEAR(solutions[i].time_of_flight, kExpected[i].time_of_flight, .001);
  }
}

TEST_F(LobEnvTestFixture, SolveWithAltitude4500ft) {
  ASSERT_NE(puut, nullptr);
  const int32_t kAltitude = 4500;
  const int32_t kWindSpeed = 10;
  auto puut2 = lob::Lob::Builder(*puut)
                   .AltitudeOfFiringSiteFt(kAltitude)
                   .WindHeading(lob::ClockAngleT::kIII)
                   .WindSpeedMph(kWindSpeed)
                   .Build();
  ASSERT_NE(puut2, nullptr);
  // NOLINTNEXTLINE c-style array
  const uint16_t kRanges[] = {0,   50,  100, 200, 300, 400,
                              500, 600, 700, 800, 900, 1000};
  const std::vector<lob::Lob::Solution> kExpected = {
      {0, 2800, 2696, -1.5, 0, 0, 0, 0},
      {50, 2712, 2529, -0.1, -0.19, 0.2, 0.38, 0.054},
      {100, 2626, 2371, 0, 0, 0.6, 0.57, 0.111},
      {200, 2458, 2078, -3.5, -1.67, 2.5, 1.19, 0.229},
      {300, 2297, 1813, -12.8, -4.07, 5.9, 1.88, 0.355},
      {400, 2141, 1576, -28.7, -6.85, 10.9, 2.6, 0.49},
      {500, 1992, 1364, -52.2, -9.97, 17.6, 3.36, 0.635},
      {600, 1849, 1175, -84.4, -13.43, 26.2, 4.17, 0.792},
      {700, 1711, 1006, -126.9, -17.31, 37.1, 5.06, 0.96},
      {800, 1577, 855, -181.3, -21.64, 50.4, 6.02, 1.143},
      {900, 1448, 721, -249.6, -26.48, 66.4, 7.05, 1.342},
      {1000, 1324, 603, -334.6, -31.95, 85.7, 8.18, 1.558}};
  constexpr size_t kSolutionLength = 12;
  lob::Lob::Solution solutions[kSolutionLength] = {0};  // NOLINT c-style array
  // NOLINTNEXTLINE implicitly decay an array into a pointer
  const size_t kWritten = puut2->Solve(solutions, kRanges, kSolutionLength);
  EXPECT_EQ(kWritten, kSolutionLength);
  for (size_t i = 0; i < kSolutionLength; i++) {
    EXPECT_EQ(solutions[i].range, kExpected[i].range);
    EXPECT_NEAR(solutions[i].velocity, kExpected[i].velocity, 1);
    EXPECT_NEAR(solutions[i].energy, kExpected[i].energy, 5);
    EXPECT_NEAR(solutions[i].elevation_adjustments,
                kExpected[i].elevation_adjustments, 0.1);
    EXPECT_NEAR(solutions[i].windage_adjustments,
                kExpected[i].windage_adjustments, 0.1);
    EXPECT_NEAR(solutions[i].time_of_flight, kExpected[i].time_of_flight, .001);
  }
}

TEST_F(LobEnvTestFixture, SolveWithTempAndAirPressure) {
  ASSERT_NE(puut, nullptr);
  const int32_t kTemperature = 100;
  const int32_t kAirPressure = 25;
  const int32_t kWindSpeed = 10;
  auto puut2 = lob::Lob::Builder(*puut)
                   .TemperatureDegF(kTemperature)
                   .AirPressureInHg(kAirPressure)
                   .WindHeading(lob::ClockAngleT::kIX)
                   .WindSpeedMph(kWindSpeed)
                   .Build();
  ASSERT_NE(puut2, nullptr);
  // NOLINTNEXTLINE implicitly decay an array into a pointer
  const uint16_t kRanges[] = {0,   50,  100, 200, 300, 400,
                              500, 600, 700, 800, 900, 1000};
  const std::vector<lob::Lob::Solution> kExpected = {
      {0, 2800, 2696, -1.5, 0, 0, 0, 0},
      {50, 2720, 2544, -0.1, -0.19, -0.1, -0.19, 0.054},
      {100, 2642, 2400, 0, 0, -0.6, -0.57, 0.11},
      {200, 2489, 2130, -3.4, -1.62, -2.3, -1.1, 0.227},
      {300, 2341, 1885, -12.5, -3.98, -5.3, -1.69, 0.351},
      {400, 2199, 1662, -27.9, -6.66, -9.7, -2.32, 0.484},
      {500, 2061, 1461, -50.6, -9.66, -15.7, -3, 0.625},
      {600, 1929, 1279, -81.4, -12.96, -23.3, -3.71, 0.775},
      {700, 1800, 1114, -121.6, -16.59, -32.8, -4.47, 0.936},
      {800, 1675, 965, -172.5, -20.59, -44.3, -5.29, 1.109},
      {900, 1554, 830, -235.8, -25.02, -58.2, -6.18, 1.295},
      {1000, 1437, 710, -313.6, -29.95, -74.7, -7.13, 1.496}};
  constexpr size_t kSolutionLength = 12;
  lob::Lob::Solution solutions[kSolutionLength] = {0};  // NOLINT c-style array
  // NOLINTNEXTLINE implicitly decay an array into a pointer
  const size_t kWritten = puut2->Solve(solutions, kRanges, kSolutionLength);
  EXPECT_EQ(kWritten, kSolutionLength);
  for (size_t i = 0; i < kSolutionLength; i++) {
    EXPECT_EQ(solutions[i].range, kExpected[i].range);
    EXPECT_NEAR(solutions[i].velocity, kExpected[i].velocity, 1);
    EXPECT_NEAR(solutions[i].energy, kExpected[i].energy, 5);
    EXPECT_NEAR(solutions[i].elevation_adjustments,
                kExpected[i].elevation_adjustments, 0.1);
    EXPECT_NEAR(solutions[i].windage_adjustments,
                kExpected[i].windage_adjustments, 0.1);
    EXPECT_NEAR(solutions[i].time_of_flight, kExpected[i].time_of_flight, .001);
  }
}

TEST_F(LobEnvTestFixture, SolveWithBarometricPressure) {
  ASSERT_NE(puut, nullptr);
  const double kAltitude = 5'280;
  const double kAirPressure = 30;
  const double kTemperature = 59;
  const double kWindSpeed = 11;
  auto puut2 = lob::Lob::Builder(*puut)
                   .AltitudeOfFiringSiteFt(kAltitude)
                   .AirPressureInHg(kAirPressure)
                   .AltitudeOfBarometerFt(0)
                   .TemperatureDegF(kTemperature)
                   .WindHeading(lob::ClockAngleT::kIX)
                   .WindSpeedMph(kWindSpeed)
                   .Build();
  ASSERT_NE(puut2, nullptr);
  // NOLINTNEXTLINE implicitly decay an array into a pointer
  const uint16_t kRanges[] = {0,   50,  100, 200, 300, 400,
                              500, 600, 700, 800, 900, 1000};
  const std::vector<lob::Lob::Solution> kExpected = {
      {0, 2800, 2696, -1.5, 0, 0, 0, 0},
      {50, 2716, 2537, -0.1, -0.19, -0.2, -0.38, 0.054},
      {100, 2634, 2385, 0, 0, -0.6, -0.57, 0.11},
      {200, 2472, 2102, -3.5, -1.67, -2.7, -1.29, 0.228},
      {300, 2317, 1846, -12.7, -4.04, -6.2, -1.97, 0.353},
      {400, 2168, 1615, -28.3, -6.76, -11.4, -2.72, 0.487},
      {500, 2024, 1408, -51.4, -9.82, -18.3, -3.5, 0.63},
      {600, 1885, 1222, -83, -13.21, -27.3, -4.34, 0.784},
      {700, 1752, 1055, -124.4, -16.97, -38.6, -5.27, 0.949},
      {800, 1622, 905, -177.1, -21.14, -52.3, -6.24, 1.127},
      {900, 1497, 770, -243.1, -25.79, -68.8, -7.3, 1.32},
      {1000, 1376, 651, -324.6, -31, -88.5, -8.45, 1.529}};
  constexpr size_t kSolutionLength = 12;
  lob::Lob::Solution solutions[kSolutionLength] = {0};  // NOLINT c-style array
  // NOLINTNEXTLINE implicitly decay an array into a pointer
  const size_t kWritten = puut2->Solve(solutions, kRanges, kSolutionLength);
  EXPECT_EQ(kWritten, kSolutionLength);
  for (size_t i = 0; i < kSolutionLength; i++) {
    EXPECT_EQ(solutions[i].range, kExpected[i].range);
    EXPECT_NEAR(solutions[i].velocity, kExpected[i].velocity, 1);
    EXPECT_NEAR(solutions[i].energy, kExpected[i].energy, 5);
    EXPECT_NEAR(solutions[i].elevation_adjustments,
                kExpected[i].elevation_adjustments, 0.1);
    EXPECT_NEAR(solutions[i].windage_adjustments,
                kExpected[i].windage_adjustments, 0.1);
    EXPECT_NEAR(solutions[i].time_of_flight, kExpected[i].time_of_flight, .001);
  }
}

TEST_F(LobEnvTestFixture, SolveWithPressureTempHumidity) {
  ASSERT_NE(puut, nullptr);
  const double kAirPressure = 29;
  const double kTemperature = 75;
  const double kRelativeHumidity = 80;
  auto puut2 = lob::Lob::Builder(*puut)
                   .AirPressureInHg(kAirPressure)
                   .TemperatureDegF(kTemperature)
                   .RelativeHumidityPercent(kRelativeHumidity)
                   .Build();
  ASSERT_NE(puut2, nullptr);
  // NOLINTNEXTLINE implicitly decay an array into a pointer
  const uint16_t kRanges[] = {0,   50,  100, 200, 300, 400,
                              500, 600, 700, 800, 900, 1000};
  const std::vector<lob::Lob::Solution> kExpected = {
      {0, 2800, 2696, -1.5, 0, 0, 0, 0},
      {50, 2705, 2516, -0.1, -0.19, 0, 0, 0.055},
      {100, 2612, 2346, 0, 0, 0, 0, 0.111},
      {200, 2431, 2033, -3.6, -1.72, 0, 0, 0.23},
      {300, 2258, 1753, -13.1, -4.17, 0, 0, 0.358},
      {400, 2092, 1505, -29.4, -7.02, 0, 0, 0.496},
      {500, 1934, 1285, -53.6, -10.24, 0, 0, 0.645},
      {600, 1781, 1090, -87.2, -13.88, 0, 0, 0.807},
      {700, 1633, 917, -131.8, -17.98, 0, 0, 0.983},
      {800, 1491, 765, -189.5, -22.62, 0, 0, 1.175},
      {900, 1355, 632, -262.8, -27.88, 0, 0, 1.386},
      {1000, 1227, 517, -355.2, -33.92, 0, 0, 1.619}};
  constexpr size_t kSolutionLength = 12;
  lob::Lob::Solution solutions[kSolutionLength] = {0};  // NOLINT c-style array
  // NOLINTNEXTLINE implicitly decay an array into a pointer
  const size_t kWritten = puut2->Solve(solutions, kRanges, kSolutionLength);
  EXPECT_EQ(kWritten, kSolutionLength);
  for (size_t i = 0; i < kSolutionLength; i++) {
    EXPECT_EQ(solutions[i].range, kExpected[i].range);
    EXPECT_NEAR(solutions[i].velocity, kExpected[i].velocity, 1);
    EXPECT_NEAR(solutions[i].energy, kExpected[i].energy, 5);
    EXPECT_NEAR(solutions[i].elevation_adjustments,
                kExpected[i].elevation_adjustments, 0.1);
    EXPECT_NEAR(solutions[i].windage_adjustments,
                kExpected[i].windage_adjustments, 0.1);
    EXPECT_NEAR(solutions[i].time_of_flight, kExpected[i].time_of_flight, .001);
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