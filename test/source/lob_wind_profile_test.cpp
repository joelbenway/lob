// Copyright (c) 2026  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

#include "eng_units.hpp"
#include "lob/lob.hpp"

namespace tests {

constexpr double kTestBcPsi = 0.372;
constexpr double kTestDiameterIn = 0.224;
constexpr double kTestMassGrains = 77.0;
constexpr uint16_t kTestVelocityFps = 2720;
constexpr double kTestZeroAngleMoa = 4.78;
constexpr double kTestOpticHeightIn = 2.5;
constexpr double kMuzzleWindFps = 7.33;
constexpr double kWindSpeedMph = 10.0;
constexpr double kLightWindSpeedMph = 5.0;
constexpr double kGrassRoughnessFt = 0.1;
constexpr double kBoreHeightFt = 1.0;
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
    {0.0, 0.0, kMuzzleWindFps, std::numeric_limits<double>::quiet_NaN()},
    {1500.0, 0.0, 14.66, std::numeric_limits<double>::quiet_NaN()},
}};

TEST_F(WindProfileBuildFixture, CopiesProfileAndSetsCount) {
  const lob::Context kCtx = builder.WindProfile(kTwoPoint).Build();
  EXPECT_EQ(kCtx.error, lob::ErrorT::kNone);
  EXPECT_EQ(kCtx.wind_count, 2U);
  // Stored nodes hold frame-resolved floats: flat fire leaves x/z untouched
  // up to float quantization (1e-5 covers the ~5e-7 worst case).
  EXPECT_NEAR(kCtx.wind_nodes.at(0).x_fps, 0.0, 1e-5);
  EXPECT_NEAR(kCtx.wind_nodes.at(0).z_fps, kMuzzleWindFps, 1e-5);
  EXPECT_EQ(kCtx.wind_nodes.at(1).range_ft, 1500U);
  EXPECT_NEAR(kCtx.wind_nodes.at(1).z_fps, 14.66, 1e-5);
  EXPECT_DOUBLE_EQ(kCtx.wind_nodes.at(0).y_fps, 0.0);
}

TEST_F(WindProfileBuildFixture, SinglePointEqualsUniform) {
  const std::array<lob::WindPoint, 1> kOne = {{
      {0.0, 0.0, kMuzzleWindFps, std::numeric_limits<double>::quiet_NaN()},
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
      .WindHeading(lob::ClockAngleT::kIII)
      .WindSpeedFps(kMuzzleWindFps);
  const lob::Context kUniform = plain.Build();
  // ponytail: kIII heading is 2π rad; libm sin leaves ~1.8e-15 residue,
  // identical through the shared float storage on both sides.
  EXPECT_NEAR(kProfile.wind_nodes.at(0).x_fps, kUniform.wind_nodes.at(0).x_fps,
              1e-12);
  EXPECT_DOUBLE_EQ(kProfile.wind_nodes.at(0).z_fps,
                   kUniform.wind_nodes.at(0).z_fps);
  EXPECT_EQ(kProfile.wind_count, 1U);
}

TEST_F(WindProfileBuildFixture, RejectsTooLong) {
  std::array<lob::WindPoint, lob::kLobWindPoints + 1> many{};
  many.at(0).range_ft = 0.0;
  for (size_t i = 1; i <= lob::kLobWindPoints; ++i) {
    many.at(i).range_ft = 100.0 * static_cast<double>(i);
    many.at(i).z_fps = 1.0;
  }
  EXPECT_EQ(builder.WindProfile(many).Build().error,
            lob::ErrorT::kWindProfileTooLong);
}

TEST_F(WindProfileBuildFixture, RejectsNonMonotonic) {
  const std::array<lob::WindPoint, 3> kBad = {{
      {0.0, 0.0, 1.0, std::numeric_limits<double>::quiet_NaN()},
      {1500.0, 0.0, 2.0, std::numeric_limits<double>::quiet_NaN()},
      {1500.0, 0.0, 3.0, std::numeric_limits<double>::quiet_NaN()},
  }};
  EXPECT_EQ(builder.WindProfile(kBad).Build().error,
            lob::ErrorT::kWindProfileNotMonotonic);
}

TEST_F(WindProfileBuildFixture, RejectsNonzeroFirstRange) {
  const std::array<lob::WindPoint, 2> kBad = {{
      {100.0, 0.0, 1.0, std::numeric_limits<double>::quiet_NaN()},
      {1500.0, 0.0, 2.0, std::numeric_limits<double>::quiet_NaN()},
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
  EXPECT_GT(kUniformLast.wind_nodes.at(0).z_fps, 0.0F);
}

TEST_F(WindProfileBuildFixture, NormalizesHighMeasurementToReference) {
  // z0 = grass roughness, muzzle at bore height, drone reads at 50 ft AGL.
  const double kF = std::log(kBoreHeightFt / kGrassRoughnessFt) /
                    std::log(50.0 / kGrassRoughnessFt);  // ≈ 0.37052
  const std::array<lob::WindPoint, 2> kPts = {{
      {0.0, 0.0, kMuzzleWindFps, kBoreHeightFt},
      {1500.0, 0.0, 14.66, 50.0},
  }};
  const lob::Context kCtx = builder.WindProfile(kPts)
                                .WindRoughnessLengthFt(kGrassRoughnessFt)
                                .HeightOfBoreAboveGroundFt(kBoreHeightFt)
                                .Build();
  EXPECT_EQ(kCtx.error, lob::ErrorT::kNone);
  // Stored nodes hold frame-resolved floats; compare against the inputs
  // within float quantization (1e-5 covers the ~1e-6 worst case here).
  // Measurement heights are Build inputs only and are not stored.
  EXPECT_NEAR(kCtx.wind_nodes.at(1).z_fps, 14.66 * kF, 1e-5);
  EXPECT_EQ(kCtx.wind_nodes.at(1).range_ft, 1500U);
  EXPECT_DOUBLE_EQ(kCtx.wind_muzzle_height_ft, kBoreHeightFt);
}

TEST_F(WindProfileBuildFixture, RoughnessUnsetLeavesValuesIdentical) {
  const std::array<lob::WindPoint, 2> kPts = {{
      {0.0, 0.0, kMuzzleWindFps, 50.0},
      {1500.0, 0.0, 14.66, 50.0},
  }};
  const lob::Context kCtx = builder.WindProfile(kPts).Build();
  EXPECT_EQ(kCtx.error, lob::ErrorT::kNone);
  EXPECT_NEAR(kCtx.wind_nodes.at(1).z_fps, 14.66, 1e-5);
  EXPECT_TRUE(std::isnan(kCtx.wind_roughness_ft));
}

TEST_F(WindProfileBuildFixture, BoreHeightDefaultsToOneFootWhenScaling) {
  const lob::Context kCtx = builder.WindProfile(kTwoPoint)
                                .WindRoughnessLengthFt(kGrassRoughnessFt)
                                .Build();
  EXPECT_EQ(kCtx.error, lob::ErrorT::kNone);
  EXPECT_DOUBLE_EQ(kCtx.wind_muzzle_height_ft, kBoreHeightFt);
}

TEST_F(WindProfileBuildFixture, RejectsBadAltitudeConfig) {
  // Non-positive roughness.
  EXPECT_EQ(builder.WindRoughnessLengthFt(0.0).Build().error,
            lob::ErrorT::kWindProfileInvalid);
  // Bore height at/below roughness.
  lob::Builder b2;
  b2.BallisticCoefficientPsi(kTestBcPsi)
      .BCDragFunction(lob::DragFunctionT::kG1)
      .DiameterInch(kTestDiameterIn)
      .MassGrains(kTestMassGrains)
      .InitialVelocityFps(kTestVelocityFps)
      .ZeroAngleMOA(kTestZeroAngleMoa)
      .OpticHeightInches(kTestOpticHeightIn)
      .WindRoughnessLengthFt(kGrassRoughnessFt)
      .HeightOfBoreAboveGroundFt(kGrassRoughnessFt);
  EXPECT_EQ(b2.Build().error, lob::ErrorT::kWindProfileInvalid);
  // Measurement height at/below roughness while scaling is on.
  const std::array<lob::WindPoint, 2> kLow = {{
      {0.0, 0.0, kMuzzleWindFps, kBoreHeightFt},
      {1500.0, 0.0, 14.66, 0.05},
  }};
  lob::Builder b3;
  b3.BallisticCoefficientPsi(kTestBcPsi)
      .BCDragFunction(lob::DragFunctionT::kG1)
      .DiameterInch(kTestDiameterIn)
      .MassGrains(kTestMassGrains)
      .InitialVelocityFps(kTestVelocityFps)
      .ZeroAngleMOA(kTestZeroAngleMoa)
      .OpticHeightInches(kTestOpticHeightIn)
      .WindProfile(kLow)
      .WindRoughnessLengthFt(kGrassRoughnessFt)
      .HeightOfBoreAboveGroundFt(kBoreHeightFt);
  EXPECT_EQ(b3.Build().error, lob::ErrorT::kWindProfileInvalid);
  // A roughness above the head-height default leaves NaN heights (and
  // uniform inputs, which carry none) with no valid configuration.
  constexpr double kTallRoughnessFt = 6.0;
  constexpr double kHighBoreFt = 10.0;
  EXPECT_EQ(builder.WindRoughnessLengthFt(kTallRoughnessFt).Build().error,
            lob::ErrorT::kWindProfileInvalid);
  EXPECT_EQ(builder.WindProfile(kTwoPoint)
                .WindRoughnessLengthFt(kTallRoughnessFt)
                .HeightOfBoreAboveGroundFt(kHighBoreFt)
                .Build()
                .error,
            lob::ErrorT::kWindProfileInvalid);
}

TEST_F(WindProfileBuildFixture, MissingHeightsDefaultToHeadHeight) {
  // NaN heights assume a head-height measurement per the Kestrel convention,
  // so the stored muzzle wind scales down to the bore reference — identically
  // for profile and uniform inputs.
  constexpr double kHeadHeightFt = 5.0;
  const double kExpectedFactor = std::log(kBoreHeightFt / kGrassRoughnessFt) /
                                 std::log(kHeadHeightFt / kGrassRoughnessFt);
  EXPECT_LT(kExpectedFactor, 1.0);
  const lob::Context kProfileCtx = builder.WindProfile(kTwoPoint)
                                       .WindRoughnessLengthFt(kGrassRoughnessFt)
                                       .HeightOfBoreAboveGroundFt(kBoreHeightFt)
                                       .Build();
  ASSERT_EQ(kProfileCtx.error, lob::ErrorT::kNone);
  EXPECT_NEAR(kProfileCtx.wind_nodes.at(0).z_fps,
              kMuzzleWindFps * kExpectedFactor, 1e-5);
  lob::Builder uniform;
  uniform.BallisticCoefficientPsi(kTestBcPsi)
      .BCDragFunction(lob::DragFunctionT::kG1)
      .DiameterInch(kTestDiameterIn)
      .MassGrains(kTestMassGrains)
      .InitialVelocityFps(kTestVelocityFps)
      .ZeroAngleMOA(kTestZeroAngleMoa)
      .OpticHeightInches(kTestOpticHeightIn)
      .WindHeading(lob::ClockAngleT::kIII)
      .WindSpeedFps(kMuzzleWindFps)
      .WindRoughnessLengthFt(kGrassRoughnessFt)
      .HeightOfBoreAboveGroundFt(kBoreHeightFt);
  const lob::Context kUniformCtx = uniform.Build();
  ASSERT_EQ(kUniformCtx.error, lob::ErrorT::kNone);
  EXPECT_NEAR(kProfileCtx.wind_nodes.at(0).x_fps,
              kUniformCtx.wind_nodes.at(0).x_fps, 1e-12);
  EXPECT_DOUBLE_EQ(kProfileCtx.wind_nodes.at(0).z_fps,
                   kUniformCtx.wind_nodes.at(0).z_fps);
}

TEST_F(WindProfileBuildFixture, RejectsUnstorableValues) {
  // Nodes store whole-foot uint32 ranges and float winds; values that cannot
  // round-trip are rejected instead of silently quantized.
  const std::array<lob::WindPoint, 2> kFractional = {{
      {0.0, 0.0, kMuzzleWindFps, std::numeric_limits<double>::quiet_NaN()},
      {1500.5, 0.0, kMuzzleWindFps, std::numeric_limits<double>::quiet_NaN()},
  }};
  EXPECT_EQ(builder.WindProfile(kFractional).Build().error,
            lob::ErrorT::kWindProfileInvalid);
  const std::array<lob::WindPoint, 2> kHugeRange = {{
      {0.0, 0.0, kMuzzleWindFps, std::numeric_limits<double>::quiet_NaN()},
      {1e300, 0.0, kMuzzleWindFps, std::numeric_limits<double>::quiet_NaN()},
  }};
  EXPECT_EQ(builder.WindProfile(kHugeRange).Build().error,
            lob::ErrorT::kWindProfileInvalid);
  const std::array<lob::WindPoint, 2> kHugeWind = {{
      {0.0, 0.0, 1e300, std::numeric_limits<double>::quiet_NaN()},
      {1500.0, 0.0, 1e300, std::numeric_limits<double>::quiet_NaN()},
  }};
  EXPECT_EQ(builder.WindProfile(kHugeWind).Build().error,
            lob::ErrorT::kWindProfileInvalid);
}

TEST_F(WindProfileBuildFixture, InclineBakesPitchIntoNodes) {
  // Frame pitch resolves once at Build: an uphill tailwind tips downward in
  // frame coordinates while lateral wind is untouched.
  const std::array<lob::WindPoint, 2> kTailwind = {{
      {0.0, kMuzzleWindFps, 0.0, std::numeric_limits<double>::quiet_NaN()},
      {1500.0, kMuzzleWindFps, 0.0, std::numeric_limits<double>::quiet_NaN()},
  }};
  const double kTheta = lob::RadiansT(lob::DegreesT(kInclineDeg)).Value();
  const lob::Context kCtx =
      builder.WindProfile(kTailwind).RangeAngleDeg(kInclineDeg).Build();
  ASSERT_EQ(kCtx.error, lob::ErrorT::kNone);
  EXPECT_NEAR(kCtx.wind_nodes.at(0).x_fps, kMuzzleWindFps * std::cos(kTheta),
              1e-5);
  EXPECT_NEAR(kCtx.wind_nodes.at(0).y_fps, -kMuzzleWindFps * std::sin(kTheta),
              1e-5);
  EXPECT_DOUBLE_EQ(kCtx.wind_nodes.at(0).z_fps, 0.0);
  // Guard against a vacuous test: the pitched values must differ decisively
  // from the unpitched inputs.
  EXPECT_GT(std::abs(kCtx.wind_nodes.at(0).y_fps), 1.0F);
}

TEST_F(WindProfileBuildFixture, TwoPointFlatProfileMatchesUniformSolve) {
  // Constant-valued 2-point profile must reproduce the uniform solution.
  // Convert through the strong types so the test shares the library's own
  // mph->fps factor instead of hand-spelling one.
  const double kFps = lob::FpsT(lob::MphT(kWindSpeedMph)).Value();
  const std::array<lob::WindPoint, 2> kFlat = {{
      {0.0, 0.0, kFps, std::numeric_limits<double>::quiet_NaN()},
      {3000.0, 0.0, kFps, std::numeric_limits<double>::quiet_NaN()},
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
      .WindHeading(lob::ClockAngleT::kIII)
      .WindSpeedFps(kFps);
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
  // Solve-level: gravity pitches on incline so TOF differs slightly.
  const std::array<uint32_t, 3> kRanges = {900, 1800, 2700};
  std::array<lob::Output, 3> flat_outs{};
  std::array<lob::Output, 3> hill_outs{};
  const size_t kNFlat = lob::Solve(builder.WindHeading(lob::ClockAngleT::kIII)
                                       .WindSpeedMph(kWindSpeedMph)
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
  // Same profile with/without grass roughness. Heights sit exactly at the
  // bore reference, so normalization is identity on both sides and only the
  // solver-side altitude factor differs.
  const std::array<lob::WindPoint, 2> kPts = {{
      {0.0, 0.0, 14.66, kBoreHeightFt},
      {3000.0, 0.0, 14.66, kBoreHeightFt},
  }};
  const std::array<uint32_t, 3> kRanges = {300, 1500, 3000};
  std::array<lob::Output, 3> plain_outs{};
  std::array<lob::Output, 3> scaled_outs{};
  lob::Solve(builder.WindProfile(kPts).Build(), kRanges, &plain_outs);
  lob::Builder sb;
  sb.BallisticCoefficientPsi(kTestBcPsi)
      .BCDragFunction(lob::DragFunctionT::kG1)
      .DiameterInch(kTestDiameterIn)
      .MassGrains(kTestMassGrains)
      .InitialVelocityFps(kTestVelocityFps)
      .ZeroAngleMOA(kTestZeroAngleMoa)
      .OpticHeightInches(kTestOpticHeightIn)
      .WindProfile(kPts)
      .WindRoughnessLengthFt(kGrassRoughnessFt)
      .HeightOfBoreAboveGroundFt(kBoreHeightFt);
  lob::Solve(sb.Build(), kRanges, &scaled_outs);
  // Apex-region growth, the feature's intent (S>1 above muzzle).
  EXPECT_GT(scaled_outs.at(0).deflection, plain_outs.at(0).deflection);
  // Sub-bore floor attenuation — the 100-yd-zero fixture drops far below bore
  // level by 3000 ft, where the floor clamps wind to ~2%.
  EXPECT_LT(scaled_outs.at(2).deflection, plain_outs.at(2).deflection);
}

}  // namespace tests
