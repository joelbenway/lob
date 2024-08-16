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

struct LobWindTestFixture : public testing::Test {
  friend class lob::Lob;

  // Unit under test
  std::unique_ptr<lob::Lob> puut;

  LobWindTestFixture() : puut(nullptr) {}

  void SetUp() override {
    if (puut != nullptr) {
      puut = nullptr;
    }

    const double kTestBC = 0.425;
    const double kTestDiameter = 0.308;
    const double kTestWeight = 180.0;
    const double kTestMuzzleVelocity = 3000.0;
    const double kTestZero = 100.0;
    const double kTestOpticHeight = 1.5;
    const double kTestTargetDistance = 1000.0;

    puut = lob::Lob::Builder()
               .BallisticCoefficentPsi(kTestBC)
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

TEST_F(LobWindTestFixture, ZeroAngleSearch) {
  ASSERT_NE(puut, nullptr);
  const float kExpectedZA = 3.38F;
  const float kZA = puut->GetZeroAngleMOA();
  const float kError = 0.1F;
  EXPECT_NEAR(kZA, kExpectedZA, kError);
}

TEST_F(LobWindTestFixture, GetAirDensityLbsPerCuFt) {
  ASSERT_NE(puut, nullptr);
  const float kExpectedFps = 0.0764742;
  const float kError = 0.001;
  EXPECT_NEAR(puut->GetAirDensityLbsPerCuFt(), kExpectedFps, kError);
}

TEST_F(LobWindTestFixture, GetSpeedOfSoundFps) {
  ASSERT_NE(puut, nullptr);
  const float kExpectedFps = 1116.45;
  const float kError = 0.001;
  EXPECT_NEAR(puut->GetSpeedOfSoundFps(), kExpectedFps, kError);
}

TEST_F(LobWindTestFixture, SolveAtICAOAtmosphere) {
  ASSERT_NE(puut, nullptr);
  // NOLINTNEXTLINE c-style array
  const uint16_t kRanges[] = {0,   50,  100, 200, 300, 400,
                              500, 600, 700, 800, 900, 1000};
  const std::vector<lob::Lob::Solution> kExpected = {
      {0, 3000, 3594, -1.5, 0, 0, 0, 0},
      {50, 2885, 3324, -0.2, -0.38, 0, 0, 0.051},
      {100, 2773, 3071, 0, 0, 0, 0, 0.104},
      {200, 2558, 2612, -3, -1.43, 0, 0, 0.217},
      {300, 2352, 2209, -11.4, -3.63, 0, 0, 0.339},
      {400, 2156, 1857, -26, -6.21, 0, 0, 0.472},
      {500, 1970, 1549, -48.2, -9.21, 0, 0, 0.618},
      {600, 1794, 1285, -79.3, -12.62, 0, 0, 0.777},
      {700, 1629, 1060, -121.3, -16.55, 0, 0, 0.953},
      {800, 1478, 872, -176.4, -21.06, 0, 0, 1.146},
      {900, 1343, 720, -247.5, -26.26, 0, 0, 1.359},
      {1000, 1226, 601, -337.9, -32.27, 0, 0, 1.594}};
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

TEST_F(LobWindTestFixture, SolveWithClockWindIII) {
  ASSERT_NE(puut, nullptr);
  const int32_t kWindSpeed = 10;
  auto puut2 = lob::Lob::Builder(*puut)
                   .WindHeading(lob::ClockAngleT::kIII)
                   .WindSpeedMph(kWindSpeed)
                   .Build();
  ASSERT_NE(puut2, nullptr);
  // NOLINTNEXTLINE c-style array
  const uint16_t kRanges[] = {0,   50,  100, 200, 300, 400,
                              500, 600, 700, 800, 900, 1000};
  const std::vector<lob::Lob::Solution> kExpected = {
      {0, 3000, 3594, -1.5, 0, 0, 0, 0},
      {50, 2885, 3324, -0.2, -0.38, 0.2, 0.38, 0.051},
      {100, 2773, 3071, 0, 0, 0.7, 0.67, 0.104},
      {200, 2558, 2612, -3, -1.43, 2.9, 1.38, 0.217},
      {300, 2352, 2209, -11.4, -3.63, 6.9, 2.2, 0.339},
      {400, 2156, 1856, -26, -6.21, 12.7, 3.03, 0.472},
      {500, 1970, 1549, -48.2, -9.21, 20.7, 3.95, 0.618},
      {600, 1794, 1285, -79.3, -12.62, 31.2, 4.97, 0.777},
      {700, 1629, 1060, -121.3, -16.55, 44.5, 6.07, 0.953},
      {800, 1478, 872, -176.4, -21.06, 61, 7.28, 1.146},
      {900, 1343, 720, -247.5, -26.26, 80.9, 8.58, 1.359},
      {1000, 1226, 601, -337.9, -32.27, 104.5, 9.98, 1.594}};
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

TEST_F(LobWindTestFixture, SolveWithClockWindIV) {
  ASSERT_NE(puut, nullptr);
  const int32_t kWindSpeed = 10;
  auto puut2 = lob::Lob::Builder(*puut)
                   .WindHeading(lob::ClockAngleT::kIV)
                   .WindSpeedMph(kWindSpeed)
                   .Build();
  ASSERT_NE(puut2, nullptr);
  // NOLINTNEXTLINE c-style array
  const uint16_t kRanges[] = {0,   50,  100, 200, 300, 400,
                              500, 600, 700, 800, 900, 1000};
  const std::vector<lob::Lob::Solution> kExpected = {
      {0, 3000, 3594, -1.5, 0, 0, 0, 0},
      {50, 2885, 3322, -0.2, -0.38, 0.2, 0.38, 0.051},
      {100, 2772, 3068, 0, 0, 0.6, 0.57, 0.104},
      {200, 2555, 2608, -3, -1.43, 2.5, 1.19, 0.217},
      {300, 2349, 2203, -11.4, -3.63, 6, 1.91, 0.339},
      {400, 2152, 1849, -26.1, -6.23, 11, 2.63, 0.473},
      {500, 1964, 1541, -48.3, -9.22, 18, 3.44, 0.618},
      {600, 1787, 1275, -79.5, -12.65, 27.2, 4.33, 0.779},
      {700, 1622, 1050, -121.7, -16.6, 38.8, 5.29, 0.955},
      {800, 1470, 863, -177.2, -21.15, 53.1, 6.34, 1.149},
      {900, 1334, 711, -248.9, -26.41, 70.5, 7.48, 1.364},
      {1000, 1218, 593, -340.1, -32.48, 91.2, 8.71, 1.599}};
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

TEST_F(LobWindTestFixture, SolveWithClockWindV) {
  ASSERT_NE(puut, nullptr);
  const int32_t kWindSpeed = 10;
  auto puut2 = lob::Lob::Builder(*puut)
                   .WindHeading(lob::ClockAngleT::kV)
                   .WindSpeedMph(kWindSpeed)
                   .Build();
  ASSERT_NE(puut2, nullptr);
  // NOLINTNEXTLINE c-style array
  const uint16_t kRanges[] = {0,   50,  100, 200, 300, 400,
                              500, 600, 700, 800, 900, 1000};
  const std::vector<lob::Lob::Solution> kExpected = {
      {0, 3000, 3594, -1.5, 0, 0, 0, 0},
      {50, 2884, 3321, -0.2, -0.38, 0.1, 0.19, 0.051},
      {100, 2771, 3066, 0, 0, 0.4, 0.38, 0.104},
      {200, 2554, 2604, -3, -1.43, 1.5, 0.72, 0.217},
      {300, 2346, 2198, -11.4, -3.63, 3.5, 1.11, 0.339},
      {400, 2149, 1843, -26.1, -6.23, 6.4, 1.53, 0.473},
      {500, 1960, 1534, -48.4, -9.24, 10.4, 1.99, 0.619},
      {600, 1782, 1268, -79.7, -12.68, 15.7, 2.5, 0.78},
      {700, 1616, 1043, -122.1, -16.66, 22.5, 3.07, 0.956},
      {800, 1464, 856, -177.8, -21.22, 30.8, 3.68, 1.152},
      {900, 1328, 704, -249.9, -26.52, 40.9, 4.34, 1.367},
      {1000, 1212, 587, -341.8, -32.64, 52.9, 5.05, 1.604}};
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

TEST_F(LobWindTestFixture, SolveWithClockWindVI) {
  ASSERT_NE(puut, nullptr);
  const int32_t kWindSpeed = 10;
  auto puut2 = lob::Lob::Builder(*puut)
                   .WindHeading(lob::ClockAngleT::kVI)
                   .WindSpeedMph(kWindSpeed)
                   .Build();
  ASSERT_NE(puut2, nullptr);
  // NOLINTNEXTLINE c-style array
  const uint16_t kRanges[] = {0,   50,  100, 200, 300, 400,
                              500, 600, 700, 800, 900, 1000};
  const std::vector<lob::Lob::Solution> kExpected = {
      {0, 3000, 3594, -1.5, 0, 0, 0, 0},
      {50, 2884, 3321, -0.2, -0.38, 0, 0, 0.051},
      {100, 2771, 3066, 0, 0, 0, 0, 0.104},
      {200, 2553, 2603, -3, -1.43, 0, 0, 0.217},
      {300, 2346, 2197, -11.4, -3.63, 0, 0, 0.339},
      {400, 2147, 1841, -26.1, -6.23, 0, 0, 0.473},
      {500, 1959, 1532, -48.4, -9.24, 0, 0, 0.619},
      {600, 1781, 1266, -79.8, -12.7, 0, 0, 0.78},
      {700, 1614, 1040, -122.2, -16.67, 0, 0, 0.957},
      {800, 1462, 853, -178.1, -21.26, 0, 0, 1.152},
      {900, 1326, 702, -250.3, -26.56, 0, 0, 1.368},
      {1000, 1210, 585, -342.4, -32.7, 0, 0, 1.605}};
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

TEST_F(LobWindTestFixture, SolveWithClockWindVII) {
  ASSERT_NE(puut, nullptr);
  const int32_t kWindSpeed = 10;
  auto puut2 = lob::Lob::Builder(*puut)
                   .WindHeading(lob::ClockAngleT::kVII)
                   .WindSpeedMph(kWindSpeed)
                   .Build();
  ASSERT_NE(puut2, nullptr);
  // NOLINTNEXTLINE c-style array
  const uint16_t kRanges[] = {0,   50,  100, 200, 300, 400,
                              500, 600, 700, 800, 900, 1000};
  const std::vector<lob::Lob::Solution> kExpected = {
      {0, 3000, 3594, -1.5, 0, 0, 0, 0},
      {50, 2884, 3321, -0.2, -0.38, -0.1, -0.19, 0.051},
      {100, 2771, 3066, 0, 0, -0.4, -0.38, 0.104},
      {200, 2554, 2604, -3, -1.43, -1.5, -0.72, 0.217},
      {300, 2346, 2198, -11.4, -3.63, -3.5, -1.11, 0.339},
      {400, 2149, 1843, -26.1, -6.23, -6.4, -1.53, 0.473},
      {500, 1960, 1534, -48.4, -9.24, -10.4, -1.99, 0.619},
      {600, 1782, 1268, -79.7, -12.68, -15.7, -2.5, 0.78},
      {700, 1616, 1043, -122.1, -16.66, -22.5, -3.07, 0.956},
      {800, 1464, 856, -177.8, -21.22, -30.8, -3.68, 1.152},
      {900, 1328, 704, -249.9, -26.52, -40.9, -4.34, 1.367},
      {1000, 1212, 587, -341.8, -32.64, -52.9, -5.05, 1.604}};
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

TEST_F(LobWindTestFixture, SolveWithClockWindVIII) {
  ASSERT_NE(puut, nullptr);
  const int32_t kWindSpeed = 10;
  auto puut2 = lob::Lob::Builder(*puut)
                   .WindHeading(lob::ClockAngleT::kVIII)
                   .WindSpeedMph(kWindSpeed)
                   .Build();
  ASSERT_NE(puut2, nullptr);
  // NOLINTNEXTLINE c-style array
  const uint16_t kRanges[] = {0,   50,  100, 200, 300, 400,
                              500, 600, 700, 800, 900, 1000};
  const std::vector<lob::Lob::Solution> kExpected = {
      {0, 3000, 3594, -1.5, 0, 0, 0, 0},
      {50, 2885, 3322, -0.2, -0.38, -0.2, -0.38, 0.051},
      {100, 2772, 3068, 0, 0, -0.6, -0.57, 0.104},
      {200, 2555, 2608, -3, -1.43, -2.5, -1.19, 0.217},
      {300, 2349, 2203, -11.4, -3.63, -6, -1.91, 0.339},
      {400, 2152, 1849, -26.1, -6.23, -11, -2.63, 0.473},
      {500, 1964, 1541, -48.3, -9.22, -18, -3.44, 0.618},
      {600, 1787, 1275, -79.5, -12.65, -27.2, -4.33, 0.779},
      {700, 1622, 1050, -121.7, -16.6, -38.8, -5.29, 0.955},
      {800, 1470, 863, -177.2, -21.15, -53.1, -6.34, 1.149},
      {900, 1334, 711, -248.9, -26.41, -70.5, -7.48, 1.364},
      {1000, 1218, 593, -340.1, -32.48, -91.2, -8.71, 1.599}};
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

TEST_F(LobWindTestFixture, SolveWithClockWindIX) {
  ASSERT_NE(puut, nullptr);
  const int32_t kWindSpeed = 10;
  auto puut2 = lob::Lob::Builder(*puut)
                   .WindHeading(lob::ClockAngleT::kIX)
                   .WindSpeedMph(kWindSpeed)
                   .Build();
  ASSERT_NE(puut2, nullptr);
  // NOLINTNEXTLINE c-style array
  const uint16_t kRanges[] = {0,   50,  100, 200, 300, 400,
                              500, 600, 700, 800, 900, 1000};
  const std::vector<lob::Lob::Solution> kExpected = {
      {0, 3000, 3594, -1.5, 0, 0, 0, 0},
      {50, 2885, 3324, -0.2, -0.38, -0.2, -0.38, 0.051},
      {100, 2773, 3071, 0, 0, -0.7, -0.67, 0.104},
      {200, 2558, 2612, -3, -1.43, -2.9, -1.38, 0.217},
      {300, 2352, 2209, -11.4, -3.63, -6.9, -2.2, 0.339},
      {400, 2156, 1856, -26, -6.21, -12.7, -3.03, 0.472},
      {500, 1970, 1549, -48.2, -9.21, -20.7, -3.95, 0.618},
      {600, 1794, 1285, -79.3, -12.62, -31.2, -4.97, 0.777},
      {700, 1629, 1060, -121.3, -16.55, -44.5, -6.07, 0.953},
      {800, 1478, 872, -176.4, -21.06, -61, -7.28, 1.146},
      {900, 1343, 720, -247.5, -26.26, -80.9, -8.58, 1.359},
      {1000, 1226, 601, -337.9, -32.27, -104.5, -9.98, 1.594}};
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

TEST_F(LobWindTestFixture, SolveWithClockWindX) {
  ASSERT_NE(puut, nullptr);
  const int32_t kWindSpeed = 10;
  auto puut2 = lob::Lob::Builder(*puut)
                   .WindHeading(lob::ClockAngleT::kX)
                   .WindSpeedMph(kWindSpeed)
                   .Build();
  ASSERT_NE(puut2, nullptr);
  // NOLINTNEXTLINE c-style array
  const uint16_t kRanges[] = {0,   50,  100, 200, 300, 400,
                              500, 600, 700, 800, 900, 1000};
  const std::vector<lob::Lob::Solution> kExpected = {
      {0, 3000, 3594, -1.5, 0, 0, 0, 0},
      {50, 2886, 3325, -0.2, -0.38, -0.2, -0.38, 0.051},
      {100, 2774, 3073, 0, 0, -0.6, -0.57, 0.104},
      {200, 2560, 2617, -3, -1.43, -2.5, -1.19, 0.216},
      {300, 2356, 2215, -11.4, -3.63, -5.9, -1.88, 0.339},
      {400, 2161, 1864, -26, -6.21, -11, -2.63, 0.472},
      {500, 1975, 1558, -48.1, -9.19, -17.9, -3.42, 0.617},
      {600, 1800, 1294, -79.1, -12.59, -26.9, -4.28, 0.776},
      {700, 1636, 1069, -120.8, -16.48, -38.3, -5.22, 0.951},
      {800, 1486, 882, -175.6, -20.96, -52.5, -6.27, 1.143},
      {900, 1351, 729, -246.1, -26.11, -69.6, -7.38, 1.355},
      {1000, 1235, 609, -335.6, -32.05, -89.8, -8.58, 1.588}};
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

TEST_F(LobWindTestFixture, SolveWithClockWindXI) {
  ASSERT_NE(puut, nullptr);
  const int32_t kWindSpeed = 10;
  auto puut2 = lob::Lob::Builder(*puut)
                   .WindHeading(lob::ClockAngleT::kXI)
                   .WindSpeedMph(kWindSpeed)
                   .Build();
  ASSERT_NE(puut2, nullptr);
  // NOLINTNEXTLINE c-style array
  const uint16_t kRanges[] = {0,   50,  100, 200, 300, 400,
                              500, 600, 700, 800, 900, 1000};
  const std::vector<lob::Lob::Solution> kExpected = {
      {0, 3000, 3594, -1.5, 0, 0, 0, 0},
      {50, 2886, 3326, -0.2, -0.38, -0.1, -0.19, 0.051},
      {100, 2775, 3075, 0, 0, -0.4, -0.38, 0.104},
      {200, 2561, 2620, -3, -1.43, -1.5, -0.72, 0.216},
      {300, 2358, 2220, -11.4, -3.63, -3.4, -1.08, 0.338},
      {400, 2164, 1870, -25.9, -6.18, -6.3, -1.5, 0.471},
      {500, 1979, 1564, -48, -9.17, -10.3, -1.97, 0.616},
      {600, 1805, 1301, -78.9, -12.56, -15.5, -2.47, 0.775},
      {700, 1642, 1076, -120.5, -16.44, -22, -3, 0.949},
      {800, 1492, 889, -175, -20.89, -30.2, -3.6, 1.141},
      {900, 1357, 735, -245.1, -26.01, -40, -4.24, 1.352},
      {1000, 1241, 615, -334, -31.89, -51.6, -4.93, 1.584}};
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

TEST_F(LobWindTestFixture, SolveWithClockWindXII) {
  ASSERT_NE(puut, nullptr);
  const int32_t kWindSpeed = 10;
  auto puut2 = lob::Lob::Builder(*puut)
                   .WindHeading(lob::ClockAngleT::kXII)
                   .WindSpeedMph(kWindSpeed)
                   .Build();
  ASSERT_NE(puut2, nullptr);
  // NOLINTNEXTLINE c-style array
  const uint16_t kRanges[] = {0,   50,  100, 200, 300, 400,
                              500, 600, 700, 800, 900, 1000};
  const std::vector<lob::Lob::Solution> kExpected = {
      {0, 3000, 3594, -1.5, 0, 0, 0, 0},
      {50, 2886, 3326, -0.2, -0.38, 0, 0, 0.051},
      {100, 2775, 3076, 0, 0, 0, 0, 0.104},
      {200, 2562, 2621, -3, -1.43, 0, 0, 0.216},
      {300, 2359, 2222, -11.4, -3.63, 0, 0, 0.338},
      {400, 2165, 1872, -25.9, -6.18, 0, 0, 0.471},
      {500, 1981, 1566, -47.9, -9.15, 0, 0, 0.616},
      {600, 1807, 1303, -78.8, -12.54, 0, 0, 0.775},
      {700, 1644, 1079, -120.4, -16.42, 0, 0, 0.949},
      {800, 1494, 891, -174.8, -20.87, 0, 0, 1.14},
      {900, 1359, 738, -244.7, -25.96, 0, 0, 1.351},
      {1000, 1243, 617, -333.4, -31.84, 0, 0, 1.582}};
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

TEST_F(LobWindTestFixture, SolveWithClockWindI) {
  ASSERT_NE(puut, nullptr);
  const int32_t kWindSpeed = 10;
  auto puut2 = lob::Lob::Builder(*puut)
                   .WindHeading(lob::ClockAngleT::kI)
                   .WindSpeedMph(kWindSpeed)
                   .Build();
  ASSERT_NE(puut2, nullptr);
  // NOLINTNEXTLINE c-style array
  const uint16_t kRanges[] = {0,   50,  100, 200, 300, 400,
                              500, 600, 700, 800, 900, 1000};
  const std::vector<lob::Lob::Solution> kExpected = {
      {0, 3000, 3594, -1.5, 0, 0, 0, 0},
      {50, 2886, 3326, -0.2, -0.38, 0.1, 0.19, 0.051},
      {100, 2775, 3075, 0, 0, 0.4, 0.38, 0.104},
      {200, 2561, 2620, -3, -1.43, 1.5, 0.72, 0.216},
      {300, 2358, 2220, -11.4, -3.63, 3.4, 1.08, 0.338},
      {400, 2164, 1870, -25.9, -6.18, 6.3, 1.5, 0.471},
      {500, 1979, 1564, -48, -9.17, 10.3, 1.97, 0.616},
      {600, 1805, 1301, -78.9, -12.56, 15.5, 2.47, 0.775},
      {700, 1642, 1076, -120.5, -16.44, 22, 3, 0.949},
      {800, 1492, 889, -175, -20.89, 30.2, 3.6, 1.141},
      {900, 1357, 735, -245.1, -26.01, 40, 4.24, 1.352},
      {1000, 1241, 615, -334, -31.89, 51.6, 4.93, 1.584}};
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

TEST_F(LobWindTestFixture, SolveWithClockWindII) {
  ASSERT_NE(puut, nullptr);
  const int32_t kWindSpeed = 10;
  auto puut2 = lob::Lob::Builder(*puut)
                   .WindHeading(lob::ClockAngleT::kII)
                   .WindSpeedMph(kWindSpeed)
                   .Build();
  ASSERT_NE(puut2, nullptr);
  // NOLINTNEXTLINE c-style array
  const uint16_t kRanges[] = {0,   50,  100, 200, 300, 400,
                              500, 600, 700, 800, 900, 1000};
  const std::vector<lob::Lob::Solution> kExpected = {
      {0, 3000, 3594, -1.5, 0, 0, 0, 0},
      {50, 2886, 3325, -0.2, -0.38, 0.2, 0.38, 0.051},
      {100, 2774, 3073, 0, 0, 0.6, 0.57, 0.104},
      {200, 2560, 2617, -3, -1.43, 2.5, 1.19, 0.216},
      {300, 2356, 2215, -11.4, -3.63, 5.9, 1.88, 0.339},
      {400, 2161, 1864, -26, -6.21, 11, 2.63, 0.472},
      {500, 1975, 1558, -48.1, -9.19, 17.9, 3.42, 0.617},
      {600, 1800, 1294, -79.1, -12.59, 26.9, 4.28, 0.776},
      {700, 1636, 1069, -120.8, -16.48, 38.3, 5.22, 0.951},
      {800, 1486, 882, -175.6, -20.96, 52.5, 6.27, 1.143},
      {900, 1351, 729, -246.1, -26.11, 69.6, 7.38, 1.355},
      {1000, 1235, 609, -335.6, -32.05, 89.8, 8.58, 1.588}};
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

TEST_F(LobWindTestFixture, SolveWithAngleWind150) {
  ASSERT_NE(puut, nullptr);
  const int32_t kWindAngle = 150;
  const int32_t kWindSpeed = 20;
  auto puut2 = lob::Lob::Builder(*puut)
                   .WindHeadingDeg(kWindAngle)
                   .WindSpeedMph(kWindSpeed)
                   .Build();
  ASSERT_NE(puut2, nullptr);
  const std::vector<lob::Lob::Solution> kExpected = {
      {100, 2769, 3062, 0.0, 0.0, 0.7, 0.7, 0.104},
      {200, 2550, 2596, -3.0, -1.4, 3.0, 1.4, 0.217},
      {300, 2341, 2188, -11.4, -3.6, 6.9, 2.2, 0.340},
      {400, 2141, 1830, -26.2, -6.3, 12.9, 3.1, 0.474},
      {500, 1951, 1519, -48.6, -9.3, 21.1, 4.0, 0.621},
      {600, 1771, 1252, -80.2, -12.8, 31.8, 5.1, 0.782},
      {700, 1603, 1026, -123.0, -16.8, 45.5, 6.2, 0.960},
      {800, 1450, 839, -179.4, -21.4, 62.3, 7.4, 1.157},
      {900, 1314, 689, -252.5, -26.8, 82.8, 8.8, 1.374},
      {1000, 1198, 573, -345.9, -33.0, 107.2, 10.2, 1.614}};
  constexpr size_t kSolutionLength = 10;
  lob::Lob::Solution solutions[kSolutionLength] = {0};  // NOLINT c-style array
  const size_t kWritten = puut2->Solve(
      static_cast<lob::Lob::Solution*>(solutions), nullptr, kSolutionLength);
  EXPECT_EQ(kWritten, kSolutionLength);
  for (size_t i = 0; i < kSolutionLength; i++) {
    EXPECT_NEAR(solutions[i].range, kExpected[i].range, 1);
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