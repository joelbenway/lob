// Copyright (c) 2026  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

#include "eng_units.hpp"
#include "lob/lob.hpp"
#include "testing.hpp"

namespace tests {

constexpr double kTestBcPsi = 0.372;
constexpr double kTestDiameterIn = 0.224;
constexpr double kTestMassGrains = 77.0;
constexpr uint16_t kTestVelocityFps = 2720;
constexpr double kTestZeroAngleMoa = 4.78;
constexpr double kTestOpticHeightIn = 2.5;
constexpr double kMuzzleWindMph = 5.0;
constexpr double kCrosswindHeadingDeg = 90.0;
constexpr double kTailwindHeadingDeg = 0.0;
constexpr double kWindSpeedMph = 10.0;
constexpr double kLightWindSpeedMph = 5.0;
constexpr double kTestShearExponent = 0.25;
constexpr double kWindReferenceHeightFt = 1.0;
constexpr double kInclineDeg = 15.0;

struct WindProfileBuildFixture : public testing::Test {
  lob::Builder builder;
  void SetUp() override {
    builder.BallisticCoefficientPsi(kTestBcPsi)
        .BCDragFunction(lob::DragFunctionT::kG1)
        .DiameterInch(kTestDiameterIn)
        .MassGrains(kTestMassGrains)
        .InitialVelocityFps(kTestVelocityFps)
        .ZeroAngleMOA(kTestZeroAngleMoa)
        .OpticHeightInches(kTestOpticHeightIn);
  }
};

const std::array<lob::WindPoint, 2> kTwoPoint = {{
    {0.0, kCrosswindHeadingDeg, kMuzzleWindMph,
     std::numeric_limits<double>::quiet_NaN()},
    {1500.0, kCrosswindHeadingDeg, 10.0,
     std::numeric_limits<double>::quiet_NaN()},
}};

namespace {

void ExpectSameSolution(const lob::Context& a, const lob::Context& b) {
  const std::array<uint32_t, 3> kRanges = {900, 1800, 2700};
  std::array<lob::Output, 3> a_outs{};
  std::array<lob::Output, 3> b_outs{};
  ASSERT_EQ(lob::Solve(a, kRanges, &a_outs), kRanges.size());
  ASSERT_EQ(lob::Solve(b, kRanges, &b_outs), kRanges.size());
  for (size_t i = 0; i < kRanges.size(); ++i) {
    EXPECT_DOUBLE_EQ(a_outs.at(i).deflection, b_outs.at(i).deflection);
    EXPECT_DOUBLE_EQ(a_outs.at(i).elevation, b_outs.at(i).elevation);
  }
}

}  // namespace

TEST_F(WindProfileBuildFixture, CopiesProfileAndSetsCount) {
  const lob::Context kCtx = builder.WindProfile(kTwoPoint).Build();
  EXPECT_EQ(kCtx.error, lob::ErrorT::kNone);
  EXPECT_EQ(kCtx.wind_count, 2U);
  EXPECT_DOUBLE_EQ(kCtx.wind_nodes.at(0).x_fps, 0.0);
  EXPECT_DOUBLE_EQ(kCtx.wind_nodes.at(0).z_fps,
                   lob::FpsT(lob::MphT(kMuzzleWindMph)).Value());
  EXPECT_EQ(kCtx.wind_nodes.at(1).range_ft, 1500U);
  EXPECT_DOUBLE_EQ(kCtx.wind_nodes.at(1).z_fps,
                   lob::FpsT(lob::MphT(10.0)).Value());
  EXPECT_DOUBLE_EQ(kCtx.wind_nodes.at(0).y_fps, 0.0);
}

TEST_F(WindProfileBuildFixture, SinglePointEqualsUniform) {
  const std::array<lob::WindPoint, 1> kOne = {{
      {0.0, kCrosswindHeadingDeg, kMuzzleWindMph,
       std::numeric_limits<double>::quiet_NaN()},
  }};
  const lob::Context kProfile = builder.WindProfile(kOne).Build();
  lob::Builder plain;
  plain.BallisticCoefficientPsi(kTestBcPsi)
      .BCDragFunction(lob::DragFunctionT::kG1)
      .DiameterInch(kTestDiameterIn)
      .MassGrains(kTestMassGrains)
      .InitialVelocityFps(kTestVelocityFps)
      .ZeroAngleMOA(kTestZeroAngleMoa)
      .OpticHeightInches(kTestOpticHeightIn)
      .WindHeadingDeg(kCrosswindHeadingDeg)
      .WindSpeedMph(kMuzzleWindMph);
  const lob::Context kUniform = plain.Build();
  EXPECT_DOUBLE_EQ(kProfile.wind_nodes.at(0).x_fps,
                   kUniform.wind_nodes.at(0).x_fps);
  EXPECT_DOUBLE_EQ(kProfile.wind_nodes.at(0).z_fps,
                   kUniform.wind_nodes.at(0).z_fps);
  EXPECT_EQ(kProfile.wind_count, 1U);
}

TEST_F(WindProfileBuildFixture, RejectsTooLong) {
  std::array<lob::WindPoint, lob::kLobWindPoints + 1> many{};
  many.at(0).range_ft = 0.0;
  for (size_t i = 1; i <= lob::kLobWindPoints; ++i) {
    many.at(i).range_ft = 100.0 * static_cast<double>(i);
    many.at(i).speed_mph = 1.0;
  }
  EXPECT_EQ(builder.WindProfile(many).Build().error,
            lob::ErrorT::kWindProfileTooLong);
}

TEST_F(WindProfileBuildFixture, RejectsNonMonotonic) {
  const std::array<lob::WindPoint, 3> kBad = {{
      {0.0, kCrosswindHeadingDeg, 1.0,
       std::numeric_limits<double>::quiet_NaN()},
      {1500.0, kCrosswindHeadingDeg, 2.0,
       std::numeric_limits<double>::quiet_NaN()},
      {1500.0, kCrosswindHeadingDeg, 3.0,
       std::numeric_limits<double>::quiet_NaN()},
  }};
  EXPECT_EQ(builder.WindProfile(kBad).Build().error,
            lob::ErrorT::kWindProfileNotMonotonic);
}

TEST_F(WindProfileBuildFixture, RejectsNonzeroFirstRange) {
  const std::array<lob::WindPoint, 2> kBad = {{
      {100.0, kCrosswindHeadingDeg, 1.0,
       std::numeric_limits<double>::quiet_NaN()},
      {1500.0, kCrosswindHeadingDeg, 2.0,
       std::numeric_limits<double>::quiet_NaN()},
  }};
  EXPECT_EQ(builder.WindProfile(kBad).Build().error,
            lob::ErrorT::kWindProfileNotMonotonic);
}

TEST_F(WindProfileBuildFixture, RejectsEmptyProfile) {
  EXPECT_EQ(builder.WindProfile(kTwoPoint.data(), 0).Build().error,
            lob::ErrorT::kWindProfileInvalid);
}

TEST_F(WindProfileBuildFixture, LastWindCallWinsBothDirections) {
  const lob::Context kProfileLast = builder.WindHeading(lob::ClockAngleT::kIII)
                                        .WindSpeedMph(kLightWindSpeedMph)
                                        .WindProfile(kTwoPoint)
                                        .Build();
  EXPECT_EQ(kProfileLast.wind_count, 2U);
  lob::Builder other;
  other.BallisticCoefficientPsi(kTestBcPsi)
      .BCDragFunction(lob::DragFunctionT::kG1)
      .DiameterInch(kTestDiameterIn)
      .MassGrains(kTestMassGrains)
      .InitialVelocityFps(kTestVelocityFps)
      .ZeroAngleMOA(kTestZeroAngleMoa)
      .OpticHeightInches(kTestOpticHeightIn)
      .WindProfile(kTwoPoint)
      .WindHeading(lob::ClockAngleT::kIII)
      .WindSpeedMph(kLightWindSpeedMph);
  const lob::Context kUniformLast = other.Build();
  EXPECT_EQ(kUniformLast.wind_count, 1U);
  EXPECT_GT(kUniformLast.wind_nodes.at(0).z_fps, 0.0);
}

TEST_F(WindProfileBuildFixture, NormalizesHighMeasurementToReference) {
  const double kHeightFactor =
      std::pow(kWindReferenceHeightFt / 50.0, kTestShearExponent);
  const std::array<lob::WindPoint, 2> kPts = {{
      {0.0, kCrosswindHeadingDeg, kMuzzleWindMph, 1.0},
      {1500.0, kCrosswindHeadingDeg, 10.0, 50.0},
  }};
  const lob::Context kCtx =
      builder.WindProfile(kPts).WindShearExponent(kTestShearExponent).Build();
  EXPECT_EQ(kCtx.error, lob::ErrorT::kNone);
  EXPECT_NEAR(kCtx.wind_nodes.at(1).z_fps,
              lob::FpsT(lob::MphT(10.0)).Value() * kHeightFactor, 1E-9);
  EXPECT_EQ(kCtx.wind_nodes.at(1).range_ft, 1500U);
}

TEST_F(WindProfileBuildFixture, ZeroShearExponentStoresVerbatim) {
  const std::array<lob::WindPoint, 2> kPts = {{
      {0.0, kCrosswindHeadingDeg, kMuzzleWindMph, 50.0},
      {1500.0, kCrosswindHeadingDeg, 10.0, 50.0},
  }};
  const lob::Context kCtx =
      builder.WindProfile(kPts).WindShearExponent(0.0).Build();
  EXPECT_EQ(kCtx.error, lob::ErrorT::kNone);
  EXPECT_DOUBLE_EQ(kCtx.wind_nodes.at(1).z_fps,
                   lob::FpsT(lob::MphT(10.0)).Value());
}

TEST_F(WindProfileBuildFixture, DefaultShearExponentDisablesScaling) {
  const lob::Context kCtx = builder.WindHeading(lob::ClockAngleT::kIII)
                                .WindSpeedMph(kLightWindSpeedMph)
                                .Build();
  ASSERT_EQ(kCtx.error, lob::ErrorT::kNone);
  EXPECT_DOUBLE_EQ(kCtx.wind_shear_exponent, 0.0);
}

TEST_F(WindProfileBuildFixture, RejectsBadShearConfig) {
  EXPECT_EQ(builder.WindShearExponent(-0.5).Build().error,
            lob::ErrorT::kWindProfileInvalid);
  EXPECT_EQ(builder.WindShearExponent(1.5).Build().error,
            lob::ErrorT::kWindProfileInvalid);
  EXPECT_EQ(builder.WindShearExponent(std::numeric_limits<double>::quiet_NaN())
                .Build()
                .error,
            lob::ErrorT::kWindProfileInvalid);
  const std::array<lob::WindPoint, 2> kLow = {{
      {0.0, kCrosswindHeadingDeg, kMuzzleWindMph, 1.0},
      {1500.0, kCrosswindHeadingDeg, 10.0, 0.0},
  }};
  lob::Builder b3;
  b3.BallisticCoefficientPsi(kTestBcPsi)
      .BCDragFunction(lob::DragFunctionT::kG1)
      .DiameterInch(kTestDiameterIn)
      .MassGrains(kTestMassGrains)
      .InitialVelocityFps(kTestVelocityFps)
      .ZeroAngleMOA(kTestZeroAngleMoa)
      .OpticHeightInches(kTestOpticHeightIn)
      .WindProfile(kLow);
  EXPECT_EQ(b3.Build().error, lob::ErrorT::kWindProfileInvalid);
}

TEST_F(WindProfileBuildFixture, MissingHeightsStoreVerbatim) {
  const lob::Context kProfileCtx = builder.WindProfile(kTwoPoint).Build();
  ASSERT_EQ(kProfileCtx.error, lob::ErrorT::kNone);
  EXPECT_DOUBLE_EQ(kProfileCtx.wind_nodes.at(0).z_fps,
                   lob::FpsT(lob::MphT(kMuzzleWindMph)).Value());
  lob::Builder uniform;
  uniform.BallisticCoefficientPsi(kTestBcPsi)
      .BCDragFunction(lob::DragFunctionT::kG1)
      .DiameterInch(kTestDiameterIn)
      .MassGrains(kTestMassGrains)
      .InitialVelocityFps(kTestVelocityFps)
      .ZeroAngleMOA(kTestZeroAngleMoa)
      .OpticHeightInches(kTestOpticHeightIn)
      .WindHeadingDeg(kCrosswindHeadingDeg)
      .WindSpeedMph(kMuzzleWindMph);
  const lob::Context kUniformCtx = uniform.Build();
  ASSERT_EQ(kUniformCtx.error, lob::ErrorT::kNone);
  EXPECT_DOUBLE_EQ(kProfileCtx.wind_nodes.at(0).x_fps,
                   kUniformCtx.wind_nodes.at(0).x_fps);
  EXPECT_DOUBLE_EQ(kProfileCtx.wind_nodes.at(0).z_fps,
                   kUniformCtx.wind_nodes.at(0).z_fps);
}

TEST_F(WindProfileBuildFixture, RejectsUnstorableValues) {
  const std::array<lob::WindPoint, 2> kFractional = {{
      {0.0, kCrosswindHeadingDeg, kMuzzleWindMph,
       std::numeric_limits<double>::quiet_NaN()},
      {1500.5, kCrosswindHeadingDeg, kMuzzleWindMph,
       std::numeric_limits<double>::quiet_NaN()},
  }};
  EXPECT_EQ(builder.WindProfile(kFractional).Build().error,
            lob::ErrorT::kWindProfileInvalid);
  const std::array<lob::WindPoint, 2> kHugeRange = {{
      {0.0, kCrosswindHeadingDeg, kMuzzleWindMph,
       std::numeric_limits<double>::quiet_NaN()},
      {1e300, kCrosswindHeadingDeg, kMuzzleWindMph,
       std::numeric_limits<double>::quiet_NaN()},
  }};
  EXPECT_EQ(builder.WindProfile(kHugeRange).Build().error,
            lob::ErrorT::kWindProfileInvalid);
}

TEST_F(WindProfileBuildFixture, InclineBakesPitchIntoNodes) {
  const std::array<lob::WindPoint, 2> kTailwind = {{
      {0.0, kTailwindHeadingDeg, kMuzzleWindMph,
       std::numeric_limits<double>::quiet_NaN()},
      {1500.0, kTailwindHeadingDeg, kMuzzleWindMph,
       std::numeric_limits<double>::quiet_NaN()},
  }};
  const double kTheta = lob::RadiansT(lob::DegreesT(kInclineDeg)).Value();
  const double kTailFps = lob::FpsT(lob::MphT(kMuzzleWindMph)).Value();
  const lob::Context kCtx =
      builder.WindProfile(kTailwind).RangeAngleDeg(kInclineDeg).Build();
  ASSERT_EQ(kCtx.error, lob::ErrorT::kNone);
  EXPECT_NEAR(kCtx.wind_nodes.at(0).x_fps, kTailFps * std::cos(kTheta), 1E-9);
  EXPECT_NEAR(kCtx.wind_nodes.at(0).y_fps, -kTailFps * std::sin(kTheta), 1E-9);
  EXPECT_NEAR(kCtx.wind_nodes.at(0).z_fps, 0.0, 1E-9);
  // Guard against a vacuous test: the pitched values must differ decisively
  // from the unpitched inputs.
  EXPECT_GT(std::abs(kCtx.wind_nodes.at(0).y_fps), 1.0);
}

TEST_F(WindProfileBuildFixture, ProfileRefinementMatchesCoarseSolve) {
  const std::array<lob::WindPoint, 2> kCoarse = {{
      {0.0, kCrosswindHeadingDeg, 7.0,
       std::numeric_limits<double>::quiet_NaN()},
      {2000.0, kCrosswindHeadingDeg, 20.0,
       std::numeric_limits<double>::quiet_NaN()},
  }};
  const std::array<lob::WindPoint, 3> kFine = {{
      {0.0, kCrosswindHeadingDeg, 7.0,
       std::numeric_limits<double>::quiet_NaN()},
      {1000.0, kCrosswindHeadingDeg, 13.5,
       std::numeric_limits<double>::quiet_NaN()},
      {2000.0, kCrosswindHeadingDeg, 20.0,
       std::numeric_limits<double>::quiet_NaN()},
  }};
  const std::array<uint32_t, 4> kRanges = {500, 1000, 1500, 2000};
  std::array<lob::Output, 4> coarse_outs{};
  std::array<lob::Output, 4> fine_outs{};
  lob::Solve(builder.WindProfile(kCoarse).Build(), kRanges, &coarse_outs);
  lob::Builder fb;
  fb.BallisticCoefficientPsi(kTestBcPsi)
      .BCDragFunction(lob::DragFunctionT::kG1)
      .DiameterInch(kTestDiameterIn)
      .MassGrains(kTestMassGrains)
      .InitialVelocityFps(kTestVelocityFps)
      .ZeroAngleMOA(kTestZeroAngleMoa)
      .OpticHeightInches(kTestOpticHeightIn)
      .WindProfile(kFine);
  lob::Solve(fb.Build(), kRanges, &fine_outs);
  for (size_t i = 0; i < kRanges.size(); ++i) {
    EXPECT_NEAR(fine_outs.at(i).deflection, coarse_outs.at(i).deflection, 1e-6);
    EXPECT_NEAR(fine_outs.at(i).elevation, coarse_outs.at(i).elevation, 1e-6);
  }
}

TEST_F(WindProfileBuildFixture, ClampedTailMatchesExplicitExtension) {
  // Exact: the extension adds kT * 0.0 terms of the same value.
  const std::array<lob::WindPoint, 2> kShort = {{
      {0.0, kCrosswindHeadingDeg, 7.0,
       std::numeric_limits<double>::quiet_NaN()},
      {1500.0, kCrosswindHeadingDeg, 20.0,
       std::numeric_limits<double>::quiet_NaN()},
  }};
  const std::array<lob::WindPoint, 3> kLong = {{
      {0.0, kCrosswindHeadingDeg, 7.0,
       std::numeric_limits<double>::quiet_NaN()},
      {1500.0, kCrosswindHeadingDeg, 20.0,
       std::numeric_limits<double>::quiet_NaN()},
      {3000.0, kCrosswindHeadingDeg, 20.0,
       std::numeric_limits<double>::quiet_NaN()},
  }};
  constexpr size_t kSolutionLength = 6;
  const std::array<uint32_t, kSolutionLength> kRanges = {500,  1000, 1500,
                                                         2000, 2500, 3000};
  std::array<lob::Output, kSolutionLength> short_outs{};
  std::array<lob::Output, kSolutionLength> long_outs{};
  lob::Solve(builder.WindProfile(kShort).Build(), kRanges, &short_outs);
  lob::Builder lb;
  lb.BallisticCoefficientPsi(kTestBcPsi)
      .BCDragFunction(lob::DragFunctionT::kG1)
      .DiameterInch(kTestDiameterIn)
      .MassGrains(kTestMassGrains)
      .InitialVelocityFps(kTestVelocityFps)
      .ZeroAngleMOA(kTestZeroAngleMoa)
      .OpticHeightInches(kTestOpticHeightIn)
      .WindProfile(kLong);
  lob::Solve(lb.Build(), kRanges, &long_outs);
  for (size_t i = 0; i < kRanges.size(); ++i) {
    EXPECT_DOUBLE_EQ(long_outs.at(i).deflection, short_outs.at(i).deflection);
    EXPECT_DOUBLE_EQ(long_outs.at(i).elevation, short_outs.at(i).elevation);
  }
}

TEST_F(WindProfileBuildFixture, PiecewiseWindForwardSolution) {
  const std::array<lob::WindPoint, 3> kPts = {{
      {0.0, 90.0, 10.0, std::numeric_limits<double>::quiet_NaN()},
      {1500.0, 45.0, 15.0, std::numeric_limits<double>::quiet_NaN()},
      {3000.0, 135.0, 5.0, std::numeric_limits<double>::quiet_NaN()},
  }};
  constexpr lob::FpsT kVelocityError{1};
  constexpr lob::FtLbsT kEnergyError{5};
  constexpr lob::MoaT kMoaError{0.1};
  constexpr lob::InchT kInchError{0.1};
  constexpr lob::SecT kTimeOfFlightError{0.01};
  constexpr size_t kSolutionLength = 12;
  const auto kContext = builder.WindProfile(kPts).Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0, 150, 300, 600, 900, 1200, 1500, 1800, 2100, 2400, 2700, 3000};
  const std::vector<lob::Output> kExpected = {
      {0, 2720, 1265, -2.50, 0.00, 0.000},
      {150, 2595, 1151, -0.60, 0.23, 0.056},
      {300, 2473, 1046, 0.00, 0.95, 0.116},
      {600, 2241, 858, -3.20, 4.00, 0.243},
      {900, 2022, 699, -13.34, 9.49, 0.384},
      {1200, 1816, 564, -32.02, 17.85, 0.541},
      {1500, 1627, 453, -61.26, 29.54, 0.715},
      {1800, 1456, 362, -103.64, 44.97, 0.910},
      {2100, 1305, 291, -162.44, 64.10, 1.128},
      {2400, 1180, 238, -241.66, 86.56, 1.370},
      {2700, 1085, 201, -345.82, 111.67, 1.636},
      {3000, 1014, 176, -479.52, 138.62, 1.923}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  const size_t kSize = lob::Solve(kContext, kRanges, &solutions);
  EXPECT_EQ(kSize, kSolutionLength);
  VerifySolutions(solutions, kExpected,
                  {kVelocityError, kEnergyError, kMoaError, kInchError,
                   kTimeOfFlightError});
}

TEST_F(WindProfileBuildFixture, PiecewiseWindScaledForwardSolution) {
  const std::array<lob::WindPoint, 3> kPts = {{
      {0.0, 90.0, 10.0, 5.0},
      {1500.0, 45.0, 15.0, 50.0},
      {3000.0, 135.0, 5.0, 20.0},
  }};
  constexpr lob::FpsT kVelocityError{1};
  constexpr lob::FtLbsT kEnergyError{5};
  constexpr lob::MoaT kMoaError{0.1};
  constexpr lob::InchT kInchError{0.1};
  constexpr lob::SecT kTimeOfFlightError{0.01};
  constexpr size_t kSolutionLength = 12;
  const auto kContext =
      builder.WindProfile(kPts).WindShearExponent(kTestShearExponent).Build();
  const std::array<uint32_t, kSolutionLength> kRanges = {
      0, 150, 300, 600, 900, 1200, 1500, 1800, 2100, 2400, 2700, 3000};
  const std::vector<lob::Output> kExpected = {
      {0, 2720, 1265, -2.50, 0.00, 0.000},
      {150, 2595, 1151, -0.60, 0.15, 0.056},
      {300, 2473, 1046, 0.00, 0.63, 0.116},
      {600, 2240, 858, -3.20, 2.59, 0.243},
      {900, 2020, 697, -13.35, 5.89, 0.384},
      {1200, 1813, 562, -32.05, 10.60, 0.541},
      {1500, 1623, 450, -61.37, 16.75, 0.716},
      {1800, 1450, 359, -103.92, 24.37, 0.912},
      {2100, 1299, 288, -163.06, 33.42, 1.130},
      {2400, 1175, 236, -242.82, 43.78, 1.374},
      {2700, 1080, 200, -347.78, 55.23, 1.641},
      {3000, 1011, 175, -482.50, 67.46, 1.928}};

  std::array<lob::Output, kSolutionLength> solutions = {};
  const size_t kSize = lob::Solve(kContext, kRanges, &solutions);
  EXPECT_EQ(kSize, kSolutionLength);
  VerifySolutions(solutions, kExpected,
                  {kVelocityError, kEnergyError, kMoaError, kInchError,
                   kTimeOfFlightError});
}

TEST_F(WindProfileBuildFixture, InclinedScaledGrowsDrift) {
  const std::array<lob::WindPoint, 2> kPts = {{
      {0.0, kCrosswindHeadingDeg, 10.0,
       std::numeric_limits<double>::quiet_NaN()},
      {3000.0, kCrosswindHeadingDeg, 10.0,
       std::numeric_limits<double>::quiet_NaN()},
  }};
  const std::array<uint32_t, 3> kRanges = {900, 1800, 3000};
  std::array<lob::Output, 3> plain_outs{};
  std::array<lob::Output, 3> scaled_outs{};
  lob::Builder pb;
  pb.BallisticCoefficientPsi(kTestBcPsi)
      .BCDragFunction(lob::DragFunctionT::kG1)
      .DiameterInch(kTestDiameterIn)
      .MassGrains(kTestMassGrains)
      .InitialVelocityFps(kTestVelocityFps)
      .ZeroAngleMOA(kTestZeroAngleMoa)
      .OpticHeightInches(kTestOpticHeightIn)
      .WindProfile(kPts)
      .WindShearExponent(0.0)
      .RangeAngleDeg(kInclineDeg);
  lob::Solve(pb.Build(), kRanges, &plain_outs);
  lob::Builder sb;
  sb.BallisticCoefficientPsi(kTestBcPsi)
      .BCDragFunction(lob::DragFunctionT::kG1)
      .DiameterInch(kTestDiameterIn)
      .MassGrains(kTestMassGrains)
      .InitialVelocityFps(kTestVelocityFps)
      .ZeroAngleMOA(kTestZeroAngleMoa)
      .OpticHeightInches(kTestOpticHeightIn)
      .WindProfile(kPts)
      .WindShearExponent(kTestShearExponent)
      .RangeAngleDeg(kInclineDeg);
  lob::Solve(sb.Build(), kRanges, &scaled_outs);
  for (size_t i = 0; i < kRanges.size(); ++i) {
    EXPECT_GT(scaled_outs.at(i).deflection, plain_outs.at(i).deflection);
  }
}

TEST_F(WindProfileBuildFixture, DownhillScaledSolveCompletes) {
  const std::array<uint32_t, 3> kRanges = {900, 1800, 3000};
  std::array<lob::Output, 3> outs{};
  lob::Builder db;
  db.BallisticCoefficientPsi(kTestBcPsi)
      .BCDragFunction(lob::DragFunctionT::kG1)
      .DiameterInch(kTestDiameterIn)
      .MassGrains(kTestMassGrains)
      .InitialVelocityFps(kTestVelocityFps)
      .ZeroAngleMOA(kTestZeroAngleMoa)
      .OpticHeightInches(kTestOpticHeightIn)
      .WindHeading(lob::ClockAngleT::kIII)
      .WindSpeedMph(kLightWindSpeedMph)
      .WindShearExponent(kTestShearExponent)
      .RangeAngleDeg(-kInclineDeg);
  const size_t kSolved = lob::Solve(db.Build(), kRanges, &outs);
  EXPECT_EQ(kSolved, kRanges.size());
  for (size_t i = 0; i < kSolved; ++i) {
    EXPECT_TRUE(std::isfinite(outs.at(i).deflection));
    EXPECT_GT(outs.at(i).deflection, 0.0);
  }
}

TEST_F(WindProfileBuildFixture, TwoPointFlatProfileMatchesUniformSolve) {
  const std::array<lob::WindPoint, 2> kFlat = {{
      {0.0, kCrosswindHeadingDeg, kWindSpeedMph,
       std::numeric_limits<double>::quiet_NaN()},
      {3000.0, kCrosswindHeadingDeg, kWindSpeedMph,
       std::numeric_limits<double>::quiet_NaN()},
  }};
  const std::array<uint32_t, 4> kRanges = {300, 900, 1800, 3000};
  std::array<lob::Output, 4> profile_outs{};
  std::array<lob::Output, 4> uniform_outs{};
  const size_t kNProfile =
      lob::Solve(builder.WindProfile(kFlat).Build(), kRanges, &profile_outs);
  lob::Builder ub;
  ub.BallisticCoefficientPsi(kTestBcPsi)
      .BCDragFunction(lob::DragFunctionT::kG1)
      .DiameterInch(kTestDiameterIn)
      .MassGrains(kTestMassGrains)
      .InitialVelocityFps(kTestVelocityFps)
      .ZeroAngleMOA(kTestZeroAngleMoa)
      .OpticHeightInches(kTestOpticHeightIn)
      .WindHeadingDeg(kCrosswindHeadingDeg)
      .WindSpeedMph(kWindSpeedMph);
  const size_t kNUniform = lob::Solve(ub.Build(), kRanges, &uniform_outs);
  EXPECT_EQ(kNProfile, kNUniform);
  for (size_t i = 0; i < kNProfile; ++i) {
    EXPECT_DOUBLE_EQ(profile_outs.at(i).deflection,
                     uniform_outs.at(i).deflection);
    EXPECT_DOUBLE_EQ(profile_outs.at(i).elevation,
                     uniform_outs.at(i).elevation);
  }
}

TEST_F(WindProfileBuildFixture, CrosswindBlindToInclineAtSolve) {
  const std::array<uint32_t, 3> kRanges = {900, 1800, 2700};
  std::array<lob::Output, 3> flat_outs{};
  std::array<lob::Output, 3> hill_outs{};
  const size_t kNFlat = lob::Solve(builder.WindHeading(lob::ClockAngleT::kIII)
                                       .WindSpeedMph(kWindSpeedMph)
                                       .WindShearExponent(0.0)
                                       .Build(),
                                   kRanges, &flat_outs);
  lob::Builder hb;
  hb.BallisticCoefficientPsi(kTestBcPsi)
      .BCDragFunction(lob::DragFunctionT::kG1)
      .DiameterInch(kTestDiameterIn)
      .MassGrains(kTestMassGrains)
      .InitialVelocityFps(kTestVelocityFps)
      .ZeroAngleMOA(kTestZeroAngleMoa)
      .OpticHeightInches(kTestOpticHeightIn)
      .WindHeading(lob::ClockAngleT::kIII)
      .WindSpeedMph(kWindSpeedMph)
      .WindShearExponent(0.0)
      .RangeAngleDeg(kInclineDeg);
  const size_t kNHill = lob::Solve(hb.Build(), kRanges, &hill_outs);
  EXPECT_EQ(kNFlat, kNHill);
  for (size_t i = 0; i < kNFlat; ++i) {
    // Residual is pre-existing gravity pitch, not wind (observed ~0.01 @900
    // growing with range; 0.5 catches real wind bugs, which would be inches).
    EXPECT_NEAR(hill_outs.at(i).deflection, flat_outs.at(i).deflection, 0.5);
  }
}

TEST_F(WindProfileBuildFixture, AltitudeScalingGrowsApexDrift) {
  constexpr double kHighArcZeroMoa = 30.0;
  const std::array<lob::WindPoint, 2> kPts = {{
      {0.0, kCrosswindHeadingDeg, 10.0,
       std::numeric_limits<double>::quiet_NaN()},
      {3000.0, kCrosswindHeadingDeg, 10.0,
       std::numeric_limits<double>::quiet_NaN()},
  }};
  const std::array<uint32_t, 3> kRanges = {900, 1800, 3000};
  std::array<lob::Output, 3> plain_outs{};
  std::array<lob::Output, 3> scaled_outs{};
  const lob::Context kPlainCtx = builder.WindProfile(kPts)
                                     .ZeroAngleMOA(kHighArcZeroMoa)
                                     .WindShearExponent(0.0)
                                     .Build();
  ASSERT_EQ(kPlainCtx.error, lob::ErrorT::kNone);
  lob::Builder sb;
  sb.BallisticCoefficientPsi(kTestBcPsi)
      .BCDragFunction(lob::DragFunctionT::kG1)
      .DiameterInch(kTestDiameterIn)
      .MassGrains(kTestMassGrains)
      .InitialVelocityFps(kTestVelocityFps)
      .ZeroAngleMOA(kHighArcZeroMoa)
      .OpticHeightInches(kTestOpticHeightIn)
      .WindProfile(kPts)
      .WindShearExponent(kTestShearExponent);
  const lob::Context kScaledCtx = sb.Build();
  ASSERT_EQ(kScaledCtx.error, lob::ErrorT::kNone);
  lob::Solve(kPlainCtx, kRanges, &plain_outs);
  lob::Solve(kScaledCtx, kRanges, &scaled_outs);
  for (size_t i = 0; i < kRanges.size(); ++i) {
    EXPECT_GT(scaled_outs.at(i).deflection, plain_outs.at(i).deflection);
  }
}

TEST_F(WindProfileBuildFixture, ZeroWindCountSolvesAsCalm) {
  lob::Builder wb;
  wb.BallisticCoefficientPsi(kTestBcPsi)
      .BCDragFunction(lob::DragFunctionT::kG1)
      .DiameterInch(kTestDiameterIn)
      .MassGrains(kTestMassGrains)
      .InitialVelocityFps(kTestVelocityFps)
      .ZeroAngleMOA(kTestZeroAngleMoa)
      .OpticHeightInches(kTestOpticHeightIn)
      .WindHeading(lob::ClockAngleT::kIII)
      .WindSpeedMph(kWindSpeedMph);
  lob::Context zeroed = wb.Build();
  ASSERT_EQ(zeroed.error, lob::ErrorT::kNone);
  zeroed.wind_count = 0;  // hand-packed: nodes still hold wind, count says none
  ExpectSameSolution(zeroed, builder.Build());
}

}  // namespace tests
