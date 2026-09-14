// Copyright (c) 2026  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>

#include "cartesian.hpp"
#include "constants.hpp"
#include "eng_units.hpp"
#include "lob/lob.h"
#include "lob/lob.hpp"
#include "ode.hpp"
#include "solve_step.hpp"

TEST(WindProfileAbi, CapacityConstant) { EXPECT_EQ(LOB_WIND_POINTS, 8); }

TEST(WindProfileAbi, PointLayout) {
  EXPECT_EQ(sizeof(LobWindPoint), 4 * sizeof(double));
  EXPECT_EQ(offsetof(LobWindPoint, range_ft), 0U);
  EXPECT_EQ(offsetof(LobWindPoint, x_fps), sizeof(double));
  EXPECT_EQ(offsetof(LobWindPoint, z_fps), 2 * sizeof(double));
  EXPECT_EQ(offsetof(LobWindPoint, height_ft_agl), 3 * sizeof(double));
}

TEST(WindProfileAbi, ContextAppendsPreserveHistory) {
  // New members sit after every historical member.
  EXPECT_GT(offsetof(LobContext, wind_count), offsetof(LobContext, error));
  EXPECT_GT(offsetof(LobContext, wind_points), offsetof(LobContext, error));
  EXPECT_EQ(sizeof(LobContext::wind_points),
            static_cast<size_t>(LOB_WIND_POINTS - 1) * sizeof(LobWindPoint));
  // LobWind itself is untouched.
  EXPECT_EQ(sizeof(LobWind), 2 * sizeof(double));
  EXPECT_EQ(offsetof(LobContext, wind), offsetof(lob::Context, wind));
}

namespace {

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
constexpr int kGarbageByte = 0xAB;

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

const std::array<LobWindPoint, 2> kTwoPoint = {{
    {0.0, 0.0, kMuzzleWindFps, std::numeric_limits<double>::quiet_NaN()},
    {1500.0, 0.0, 14.66, std::numeric_limits<double>::quiet_NaN()},
}};

}  // namespace

TEST_F(WindProfileBuildFixture, CopiesProfileAndSetsCount) {
  const lob::Context kCtx = builder.WindProfile(kTwoPoint).Build();
  EXPECT_EQ(kCtx.error, lob::ErrorT::kNone);
  EXPECT_EQ(kCtx.wind_count, 2U);
  EXPECT_DOUBLE_EQ(kCtx.wind.x, 0.0);
  EXPECT_DOUBLE_EQ(kCtx.wind.z, 7.33);
  EXPECT_DOUBLE_EQ(kCtx.wind_points[0].range_ft, 1500.0);
  EXPECT_DOUBLE_EQ(kCtx.wind_points[0].z_fps, 14.66);
  EXPECT_DOUBLE_EQ(kCtx.wind_cos, 1.0);
  EXPECT_DOUBLE_EQ(kCtx.wind_sin, 0.0);
}

TEST_F(WindProfileBuildFixture, SinglePointEqualsUniform) {
  const std::array<LobWindPoint, 1> kOne = {{
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
  // ponytail: kIII heading is 2π rad; libm sin leaves ~1.8e-15 residue.
  EXPECT_NEAR(kProfile.wind.x, kUniform.wind.x, 1e-12);
  EXPECT_DOUBLE_EQ(kProfile.wind.z, kUniform.wind.z);
  EXPECT_EQ(kProfile.wind_count, 1U);
}

TEST_F(WindProfileBuildFixture, RejectsTooLong) {
  std::array<LobWindPoint, LOB_WIND_POINTS + 1> many{};
  many.at(0).range_ft = 0.0;
  for (size_t i = 1; i <= LOB_WIND_POINTS; ++i) {
    many.at(i).range_ft = 100.0 * static_cast<double>(i);
    many.at(i).z_fps = 1.0;
  }
  EXPECT_EQ(builder.WindProfile(many).Build().error,
            lob::ErrorT::kWindProfileTooLong);
}

TEST_F(WindProfileBuildFixture, RejectsNonMonotonic) {
  const std::array<LobWindPoint, 3> kBad = {{
      {0.0, 0.0, 1.0, std::numeric_limits<double>::quiet_NaN()},
      {1500.0, 0.0, 2.0, std::numeric_limits<double>::quiet_NaN()},
      {1500.0, 0.0, 3.0, std::numeric_limits<double>::quiet_NaN()},
  }};
  EXPECT_EQ(builder.WindProfile(kBad).Build().error,
            lob::ErrorT::kWindProfileNotMonotonic);
}

TEST_F(WindProfileBuildFixture, RejectsNonzeroFirstRange) {
  const std::array<LobWindPoint, 2> kBad = {{
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
  EXPECT_GT(kUniformLast.wind.z, 0.0);
}

TEST(WindProfileNullSafety, NullBuilderIsNoOp) {
  EXPECT_EQ(LobBuilderWindProfile(nullptr, kTwoPoint.data(), 2), nullptr);
}

TEST(WindProfileAbi, ErrorCodesAppended) {
  EXPECT_EQ(kLobErrorWindProfileTooLong, kLobErrorNumberOfErrors - 3);
  EXPECT_EQ(kLobErrorWindProfileNotMonotonic, kLobErrorNumberOfErrors - 2);
  EXPECT_EQ(kLobErrorWindProfileInvalid, kLobErrorNumberOfErrors - 1);
  EXPECT_EQ(static_cast<LobErrorT>(lob::ErrorT::kWindProfileTooLong),
            kLobErrorWindProfileTooLong);
  EXPECT_EQ(static_cast<LobErrorT>(lob::ErrorT::kWindProfileNotMonotonic),
            kLobErrorWindProfileNotMonotonic);
  EXPECT_EQ(static_cast<LobErrorT>(lob::ErrorT::kWindProfileInvalid),
            kLobErrorWindProfileInvalid);
}

TEST_F(WindProfileBuildFixture, NormalizesHighMeasurementToReference) {
  // z0 = grass roughness, muzzle at bore height, drone reads at 50 ft AGL.
  const double kF = std::log(kBoreHeightFt / kGrassRoughnessFt) /
                    std::log(50.0 / kGrassRoughnessFt);  // ≈ 0.37052
  const std::array<LobWindPoint, 2> kPts = {{
      {0.0, 0.0, kMuzzleWindFps, kBoreHeightFt},
      {1500.0, 0.0, 14.66, 50.0},
  }};
  const lob::Context kCtx = builder.WindProfile(kPts)
                                .WindRoughnessLengthFt(kGrassRoughnessFt)
                                .HeightOfBoreAboveGroundFt(kBoreHeightFt)
                                .Build();
  EXPECT_EQ(kCtx.error, lob::ErrorT::kNone);
  EXPECT_NEAR(kCtx.wind_points[0].z_fps, 14.66 * kF, 1E-9);
  EXPECT_NEAR(kCtx.wind_points[0].height_ft_agl, 50.0, 0.0);
  EXPECT_NEAR(kCtx.wind_inv_ln_denom,
              1.0 / std::log(kBoreHeightFt / kGrassRoughnessFt), 1E-12);
  EXPECT_DOUBLE_EQ(kCtx.wind_muzzle_height_ft, kBoreHeightFt);
}

TEST_F(WindProfileBuildFixture, RoughnessUnsetLeavesValuesIdentical) {
  const std::array<LobWindPoint, 2> kPts = {{
      {0.0, 0.0, kMuzzleWindFps, 50.0},
      {1500.0, 0.0, 14.66, 50.0},
  }};
  const lob::Context kCtx = builder.WindProfile(kPts).Build();
  EXPECT_EQ(kCtx.error, lob::ErrorT::kNone);
  EXPECT_DOUBLE_EQ(kCtx.wind_points[0].z_fps, 14.66);
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
  const std::array<LobWindPoint, 2> kLow = {{
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
}

namespace {

using lob::CartesianT;
using lob::FeetT;
using lob::FpsT;
using lob::TrajectoryStateT;

constexpr double kQueryWindZFps = 10.0;
constexpr double kQuerySpeedFps = 2000.0;
constexpr double kTailwindXFps = 10.0;
constexpr double kCrosswindZFps = 4.0;
constexpr double kPitchDeg = 8.0;

LobContext MakeWindQueryCtx() {
  LobContext ctx{};
  ctx.gravity.x = 0.0;
  ctx.gravity.y = -lob::kStandardGravityFtPerSecSq;
  ctx.wind.x = 0.0;
  ctx.wind.z = kQueryWindZFps;
  ctx.wind_count = 1;
  ctx.wind_cos = 1.0;
  ctx.wind_sin = 0.0;
  ctx.wind_roughness_ft = std::numeric_limits<double>::quiet_NaN();
  ctx.wind_muzzle_height_ft = std::numeric_limits<double>::quiet_NaN();
  ctx.wind_inv_ln_denom = std::numeric_limits<double>::quiet_NaN();
  return ctx;
}

TrajectoryStateT MakeStateAt(double x_ft, double y_ft) {
  return {CartesianT<FeetT>(FeetT(x_ft), FeetT(y_ft), FeetT(0.0)),
          CartesianT<FpsT>(FpsT(kQuerySpeedFps), FpsT(0.0), FpsT(0.0))};
}

// Builds a uniform-wind LobContext through the C API, so query tests can
// hold the C type directly instead of casting out of lob::Context.
LobContext BuildUniformWindCtx(
    double speed_mph, double range_angle_deg,
    double roughness_ft = std::numeric_limits<double>::quiet_NaN(),
    double bore_height_ft = std::numeric_limits<double>::quiet_NaN()) {
  LobBuilder builder;
  LobBuilderInit(&builder);
  LobBuilderBallisticCoefficientPsi(&builder, kTestBcPsi);
  LobBuilderBCDragFunction(&builder, kLobDragFunctionG1);
  LobBuilderDiameterInch(&builder, kTestDiameterIn);
  LobBuilderMassGrains(&builder, kTestMassGrains);
  LobBuilderInitialVelocityFps(&builder, kTestVelocityFps);
  LobBuilderZeroAngleMOA(&builder, kTestZeroAngleMoa);
  LobBuilderOpticHeightInches(&builder, kTestOpticHeightIn);
  LobBuilderWindHeading(&builder, kLobClockAngleIII);
  LobBuilderWindSpeedMph(&builder, speed_mph);
  LobBuilderRangeAngleDeg(&builder, range_angle_deg);
  if (!std::isnan(roughness_ft)) {
    LobBuilderWindRoughnessLengthFt(&builder, roughness_ft);
    LobBuilderHeightOfBoreAboveGroundFt(&builder, bore_height_ft);
  }
  LobContext ctx{};
  LobBuilderBuild(&builder, &ctx);
  LobBuilderDestroy(&builder);
  return ctx;
}

void ExpectWindTailZeroed(const LobContext& ctx) {
  const LobWindPoint* points = &ctx.wind_points[0];
  for (size_t i = 1; i < LOB_WIND_POINTS - 1; ++i) {
    EXPECT_DOUBLE_EQ(points[i].range_ft, 0.0);
    EXPECT_DOUBLE_EQ(points[i].x_fps, 0.0);
    EXPECT_DOUBLE_EQ(points[i].z_fps, 0.0);
    EXPECT_DOUBLE_EQ(points[i].height_ft_agl, 0.0);
  }
}

const LobWindPoint kLerpLow{1000.0, 0.0, kQueryWindZFps,
                            std::numeric_limits<double>::quiet_NaN()};
const LobWindPoint kLerpHigh{2000.0, 0.0, 30.0,
                             std::numeric_limits<double>::quiet_NaN()};

}  // namespace

TEST(WindProfileQuery, UniformFlatIsHistoricalPath) {
  const LobContext kCtx = MakeWindQueryCtx();
  const CartesianT<FpsT> kW = lob::GetWind(kCtx, MakeStateAt(500.0, 0.0));
  EXPECT_DOUBLE_EQ(kW.X().Value(), 0.0);
  EXPECT_DOUBLE_EQ(kW.Y().Value(), 0.0);
  EXPECT_DOUBLE_EQ(kW.Z().Value(), 10.0);
}

TEST(WindProfileQuery, LerpsMidpointAndClampsEnds) {
  LobContext ctx = MakeWindQueryCtx();
  ctx.wind_count = 3;
  ctx.wind_points[0] = kLerpLow;
  ctx.wind_points[1] = kLerpHigh;
  EXPECT_DOUBLE_EQ(lob::GetWind(ctx, MakeStateAt(1500.0, 0.0)).Z().Value(),
                   20.0);
  EXPECT_DOUBLE_EQ(lob::GetWind(ctx, MakeStateAt(1000.0, 0.0)).Z().Value(),
                   10.0);
  EXPECT_DOUBLE_EQ(lob::GetWind(ctx, MakeStateAt(5000.0, 0.0)).Z().Value(),
                   30.0);
  EXPECT_DOUBLE_EQ(lob::GetWind(ctx, MakeStateAt(-10.0, 0.0)).Z().Value(),
                   10.0);
}

TEST(WindProfileQuery, PitchesAlongTrackIntoFrame) {
  LobContext ctx = MakeWindQueryCtx();
  ctx.wind.x = kTailwindXFps;  // pure horizontal tailwind
  ctx.wind.z = kCrosswindZFps;
  const double kTheta = lob::RadiansT(lob::DegreesT(kPitchDeg)).Value();
  ctx.wind_cos = std::cos(kTheta);
  ctx.wind_sin = std::sin(kTheta);
  const CartesianT<FpsT> kW = lob::GetWind(ctx, MakeStateAt(100.0, 0.0));
  EXPECT_NEAR(kW.X().Value(), kTailwindXFps * std::cos(kTheta), 1E-9);
  EXPECT_NEAR(kW.Y().Value(), -kTailwindXFps * std::sin(kTheta), 1E-9);
  EXPECT_DOUBLE_EQ(kW.Z().Value(), kCrosswindZFps);  // lateral untouched
}

TEST(WindProfileQuery, AltitudeScalesAboutMuzzleReference) {
  // z0 = grass roughness, muzzle at bore height: at true height 9 ft,
  // z_agl = 10, S = 2 exactly.
  LobContext ctx = MakeWindQueryCtx();
  ctx.wind_roughness_ft = kGrassRoughnessFt;
  ctx.wind_muzzle_height_ft = kBoreHeightFt;
  ctx.wind_inv_ln_denom = 1.0 / std::log(kBoreHeightFt / kGrassRoughnessFt);
  const CartesianT<FpsT> kAtMuzzle = lob::GetWind(ctx, MakeStateAt(0.0, 0.0));
  EXPECT_NEAR(kAtMuzzle.Z().Value(), 10.0, 1E-9);  // S = 1 at muzzle
  const CartesianT<FpsT> kHigh = lob::GetWind(ctx, MakeStateAt(0.0, 9.0));
  EXPECT_NEAR(kHigh.Z().Value(), 20.0, 1E-9);  // S = ln(100)/ln(10) = 2
}

TEST(WindProfileQuery, SubBoreFloorKeepsLogDefined) {
  LobContext ctx = MakeWindQueryCtx();
  ctx.wind_roughness_ft = kGrassRoughnessFt;
  ctx.wind_muzzle_height_ft = kBoreHeightFt;
  ctx.wind_inv_ln_denom = 1.0 / std::log(kBoreHeightFt / kGrassRoughnessFt);
  const CartesianT<FpsT> kW = lob::GetWind(ctx, MakeStateAt(2500.0, -40.0));
  EXPECT_TRUE(std::isfinite(kW.X().Value()));
  EXPECT_TRUE(std::isfinite(kW.Y().Value()));
  EXPECT_TRUE(std::isfinite(kW.Z().Value()));
  EXPECT_GE(kW.Z().Value(), 0.0);
}

TEST_F(WindProfileBuildFixture, TwoPointFlatProfileMatchesUniformSolve) {
  // Constant-valued 2-point profile must reproduce the uniform solution.
  // Convert through the strong types so the test shares the library's own
  // mph->fps factor instead of hand-spelling one.
  const double kFps = lob::FpsT(lob::MphT(kWindSpeedMph)).Value();
  const std::array<LobWindPoint, 2> kFlat = {{
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

TEST(WindProfileQuery, CrosswindResolvesIdenticallyWithIncline) {
  // Spec section 9: the wind query resolves pure crosswind identically;
  // pitch cannot touch pure-crosswind output.
  const LobContext kFlatCtx = BuildUniformWindCtx(kWindSpeedMph, 0.0);
  const LobContext kHillCtx = BuildUniformWindCtx(kWindSpeedMph, kInclineDeg);
  const std::array<double, 3> kXs = {100.0, 900.0, 2700.0};
  for (const double kX : kXs) {
    const CartesianT<FpsT> kFlatW =
        lob::GetWind(kFlatCtx, MakeStateAt(kX, 0.0));
    const CartesianT<FpsT> kHillW =
        lob::GetWind(kHillCtx, MakeStateAt(kX, 0.0));
    // ponytail: kIII heading is 2π rad; libm sin leaves ~3.6e-15 in
    // wind.x which pitch then scales — same 1e-12 tolerance as
    // SinglePointEqualsUniform. The crosswind (Z) component is bit-identical.
    EXPECT_NEAR(kFlatW.X().Value(), kHillW.X().Value(), 1e-12);
    EXPECT_NEAR(kFlatW.Y().Value(), kHillW.Y().Value(), 1e-12);
    EXPECT_DOUBLE_EQ(kFlatW.Z().Value(), kHillW.Z().Value());
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
  // Same profile with/without grass roughness: muzzle output identical,
  // far output drifts further with scaling on.
  const std::array<LobWindPoint, 2> kPts = {{
      {0.0, 0.0, 14.66, std::numeric_limits<double>::quiet_NaN()},
      {3000.0, 0.0, 14.66, std::numeric_limits<double>::quiet_NaN()},
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

TEST(WindProfileQuery, InclinedScalingUsesTrueVertical) {
  // Regression: altitude scaling must resolve height through the gravity
  // vector, never frame-Y. At 15° incline and state (1500, 0), frame-Y says
  // height 0 (S = 1) while true height is x*sin(15°) ≈ 388 ft (S ≈ 3.59).
  const LobContext kCtx = BuildUniformWindCtx(kWindSpeedMph, kInclineDeg,
                                              kGrassRoughnessFt, kBoreHeightFt);
  const CartesianT<FpsT> kW = lob::GetWind(kCtx, MakeStateAt(1500.0, 0.0));
  const double kG = std::sqrt((kCtx.gravity.x * kCtx.gravity.x) +
                              (kCtx.gravity.y * kCtx.gravity.y));
  const double kH = -((1500.0 * kCtx.gravity.x) / kG);
  const double kS = std::log((kH + kBoreHeightFt) / kGrassRoughnessFt) /
                    std::log(kBoreHeightFt / kGrassRoughnessFt);
  // Guard against a vacuous test: the gravity-vector answer must differ
  // decisively from the frame-Y answer (S = 1).
  EXPECT_GT(std::abs(kS - 1.0), 2.0);
  EXPECT_NEAR(kW.Z().Value(), kCtx.wind.z * kS, 1e-9);
}

TEST(WindProfileBuild, ZeroesUnusedTailOverGarbage) {
  // Build writes only wind_points[0..count-2]; the rest must be zeroed so
  // identically-built contexts compare equal. Fill the output with garbage
  // first — the C++ Builder zero-inits and would mask the bug.
  LobBuilder cbuilder;
  LobBuilderInit(&cbuilder);
  LobBuilderBallisticCoefficientPsi(&cbuilder, kTestBcPsi);
  LobBuilderInitialVelocityFps(&cbuilder, kTestVelocityFps);
  LobBuilderZeroAngleMOA(&cbuilder, kTestZeroAngleMoa);
  const std::array<LobWindPoint, 2> kPts = {{
      {0.0, 0.0, 7.0, std::numeric_limits<double>::quiet_NaN()},
      {1500.0, 0.0, 8.0, std::numeric_limits<double>::quiet_NaN()},
  }};
  LobBuilderWindProfile(&cbuilder, kPts.data(), kPts.size());
  LobContext ctx;
  std::memset(&ctx, kGarbageByte, sizeof(ctx));
  LobBuilderBuild(&cbuilder, &ctx);
  LobBuilderDestroy(&cbuilder);
  ASSERT_EQ(ctx.error, kLobErrorNone);
  ASSERT_EQ(ctx.wind_count, 2U);
  ExpectWindTailZeroed(ctx);
}
