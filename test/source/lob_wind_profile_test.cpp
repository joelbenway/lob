// Copyright (c) 2026  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

#include "constants.hpp"
#include "lob/lob.h"
#include "lob/lob.hpp"
#include "solve_step.hpp"

TEST(WindProfileAbi, CapacityConstant) { EXPECT_EQ(LOB_WIND_POINTS, 8); }

TEST(WindProfileAbi, PointLayout) {
  EXPECT_EQ(sizeof(LobWindPoint), 4 * sizeof(double));
  EXPECT_EQ(offsetof(LobWindPoint, range_ft), 0u);
  EXPECT_EQ(offsetof(LobWindPoint, x_fps), sizeof(double));
  EXPECT_EQ(offsetof(LobWindPoint, z_fps), 2 * sizeof(double));
  EXPECT_EQ(offsetof(LobWindPoint, height_ft_agl), 3 * sizeof(double));
}

TEST(WindProfileAbi, ContextAppendsPreserveHistory) {
  // New members sit after every historical member.
  EXPECT_GT(offsetof(LobContext, wind_count), offsetof(LobContext, error));
  EXPECT_GT(offsetof(LobContext, wind_points), offsetof(LobContext, error));
  EXPECT_EQ(sizeof(((LobContext*)nullptr)->wind_points),
            (size_t)(LOB_WIND_POINTS - 1) * sizeof(LobWindPoint));
  // LobWind itself is untouched.
  EXPECT_EQ(sizeof(LobWind), 2 * sizeof(double));
  EXPECT_EQ(offsetof(LobContext, wind), offsetof(lob::Context, wind));
}

namespace {

struct WindProfileBuildFixture : public testing::Test {
  lob::Builder builder;
  void SetUp() override {
    builder.BallisticCoefficientPsi(0.372)
        .BCDragFunction(lob::DragFunctionT::kG1)
        .DiameterInch(0.224)
        .MassGrains(77.0)
        .InitialVelocityFps(2720)
        .ZeroAngleMOA(4.78)
        .OpticHeightInches(2.5);
  }
};

const LobWindPoint kTwoPoint[2] = {
    {0.0, 0.0, 7.33, std::numeric_limits<double>::quiet_NaN()},
    {1500.0, 0.0, 14.66, std::numeric_limits<double>::quiet_NaN()},
};

}  // namespace

TEST_F(WindProfileBuildFixture, CopiesProfileAndSetsCount) {
  const lob::Context kCtx = builder.WindProfile(kTwoPoint, 2).Build();
  EXPECT_EQ(kCtx.error, lob::ErrorT::kNone);
  EXPECT_EQ(kCtx.wind_count, 2u);
  EXPECT_DOUBLE_EQ(kCtx.wind.x, 0.0);
  EXPECT_DOUBLE_EQ(kCtx.wind.z, 7.33);
  EXPECT_DOUBLE_EQ(kCtx.wind_points[0].range_ft, 1500.0);
  EXPECT_DOUBLE_EQ(kCtx.wind_points[0].z_fps, 14.66);
  EXPECT_DOUBLE_EQ(kCtx.wind_cos, 1.0);
  EXPECT_DOUBLE_EQ(kCtx.wind_sin, 0.0);
}

TEST_F(WindProfileBuildFixture, SinglePointEqualsUniform) {
  const LobWindPoint kOne[1] = {
      {0.0, 0.0, 7.33, std::numeric_limits<double>::quiet_NaN()}};
  const lob::Context kProfile = builder.WindProfile(kOne, 1).Build();
  lob::Builder plain;
  plain.BallisticCoefficientPsi(0.372)
      .BCDragFunction(lob::DragFunctionT::kG1)
      .DiameterInch(0.224)
      .MassGrains(77.0)
      .InitialVelocityFps(2720)
      .ZeroAngleMOA(4.78)
      .OpticHeightInches(2.5)
      .WindHeading(lob::ClockAngleT::kIII)
      .WindSpeedFps(7.33);
  const lob::Context kUniform = plain.Build();
  // ponytail: kIII heading is 2π rad; libm sin leaves ~1.8e-15 residue.
  EXPECT_NEAR(kProfile.wind.x, kUniform.wind.x, 1e-12);
  EXPECT_DOUBLE_EQ(kProfile.wind.z, kUniform.wind.z);
  EXPECT_EQ(kProfile.wind_count, 1u);
}

TEST_F(WindProfileBuildFixture, RejectsTooLong) {
  LobWindPoint many[LOB_WIND_POINTS + 1] = {};
  many[0].range_ft = 0.0;
  for (size_t i = 1; i <= LOB_WIND_POINTS; ++i) {
    many[i].range_ft = 100.0 * static_cast<double>(i);
    many[i].z_fps = 1.0;
  }
  EXPECT_EQ(builder.WindProfile(many, LOB_WIND_POINTS + 1).Build().error,
            lob::ErrorT::kWindProfileTooLong);
}

TEST_F(WindProfileBuildFixture, RejectsNonMonotonic) {
  const LobWindPoint kBad[3] = {
      {0.0, 0.0, 1.0, std::numeric_limits<double>::quiet_NaN()},
      {1500.0, 0.0, 2.0, std::numeric_limits<double>::quiet_NaN()},
      {1500.0, 0.0, 3.0, std::numeric_limits<double>::quiet_NaN()}};
  EXPECT_EQ(builder.WindProfile(kBad, 3).Build().error,
            lob::ErrorT::kWindProfileNotMonotonic);
}

TEST_F(WindProfileBuildFixture, RejectsNonzeroFirstRange) {
  const LobWindPoint kBad[2] = {
      {100.0, 0.0, 1.0, std::numeric_limits<double>::quiet_NaN()},
      {1500.0, 0.0, 2.0, std::numeric_limits<double>::quiet_NaN()}};
  EXPECT_EQ(builder.WindProfile(kBad, 2).Build().error,
            lob::ErrorT::kWindProfileNotMonotonic);
}

TEST_F(WindProfileBuildFixture, RejectsEmptyProfile) {
  EXPECT_EQ(builder.WindProfile(kTwoPoint, 0).Build().error,
            lob::ErrorT::kWindProfileInvalid);
}

TEST_F(WindProfileBuildFixture, LastWindCallWinsBothDirections) {
  const lob::Context kProfileLast = builder.WindHeading(lob::ClockAngleT::kIII)
                                        .WindSpeedMph(5.0)
                                        .WindProfile(kTwoPoint, 2)
                                        .Build();
  EXPECT_EQ(kProfileLast.wind_count, 2u);
  lob::Builder other;
  other.BallisticCoefficientPsi(0.372)
      .BCDragFunction(lob::DragFunctionT::kG1)
      .DiameterInch(0.224)
      .MassGrains(77.0)
      .InitialVelocityFps(2720)
      .ZeroAngleMOA(4.78)
      .OpticHeightInches(2.5)
      .WindProfile(kTwoPoint, 2)
      .WindHeading(lob::ClockAngleT::kIII)
      .WindSpeedMph(5.0);
  const lob::Context kUniformLast = other.Build();
  EXPECT_EQ(kUniformLast.wind_count, 1u);
  EXPECT_GT(kUniformLast.wind.z, 0.0);
}

TEST(WindProfileNullSafety, NullBuilderIsNoOp) {
  EXPECT_EQ(LobBuilderWindProfile(nullptr, kTwoPoint, 2), nullptr);
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
  // z0 = 0.1 ft grass, muzzle 1 ft, drone reads 14.66 fps at 50 ft AGL.
  const double kF = std::log(1.0 / 0.1) / std::log(50.0 / 0.1);  // ≈ 0.37052
  const LobWindPoint kPts[2] = {
      {0.0, 0.0, 7.33, 1.0},
      {1500.0, 0.0, 14.66, 50.0},
  };
  const lob::Context kCtx = builder.WindProfile(kPts, 2)
                                .WindRoughnessLengthFt(0.1)
                                .HeightOfBoreAboveGroundFt(1.0)
                                .Build();
  EXPECT_EQ(kCtx.error, lob::ErrorT::kNone);
  EXPECT_NEAR(kCtx.wind_points[0].z_fps, 14.66 * kF, 1E-9);
  EXPECT_NEAR(kCtx.wind_points[0].height_ft_agl, 50.0, 0.0);
  EXPECT_NEAR(kCtx.wind_inv_ln_denom, 1.0 / std::log(1.0 / 0.1), 1E-12);
  EXPECT_DOUBLE_EQ(kCtx.wind_muzzle_height_ft, 1.0);
}

TEST_F(WindProfileBuildFixture, RoughnessUnsetLeavesValuesIdentical) {
  const LobWindPoint kPts[2] = {
      {0.0, 0.0, 7.33, 50.0},
      {1500.0, 0.0, 14.66, 50.0},
  };
  const lob::Context kCtx = builder.WindProfile(kPts, 2).Build();
  EXPECT_EQ(kCtx.error, lob::ErrorT::kNone);
  EXPECT_DOUBLE_EQ(kCtx.wind_points[0].z_fps, 14.66);
  EXPECT_TRUE(std::isnan(kCtx.wind_roughness_ft));
}

TEST_F(WindProfileBuildFixture, BoreHeightDefaultsToOneFootWhenScaling) {
  const lob::Context kCtx =
      builder.WindProfile(kTwoPoint, 2).WindRoughnessLengthFt(0.1).Build();
  EXPECT_EQ(kCtx.error, lob::ErrorT::kNone);
  EXPECT_DOUBLE_EQ(kCtx.wind_muzzle_height_ft, 1.0);
}

TEST_F(WindProfileBuildFixture, RejectsBadAltitudeConfig) {
  // Non-positive roughness.
  EXPECT_EQ(builder.WindRoughnessLengthFt(0.0).Build().error,
            lob::ErrorT::kWindProfileInvalid);
  // Bore height at/below roughness.
  lob::Builder b2;
  b2.BallisticCoefficientPsi(0.372)
      .BCDragFunction(lob::DragFunctionT::kG1)
      .DiameterInch(0.224)
      .MassGrains(77.0)
      .InitialVelocityFps(2720)
      .ZeroAngleMOA(4.78)
      .OpticHeightInches(2.5)
      .WindRoughnessLengthFt(0.1)
      .HeightOfBoreAboveGroundFt(0.1);
  EXPECT_EQ(b2.Build().error, lob::ErrorT::kWindProfileInvalid);
  // Measurement height at/below roughness while scaling is on.
  const LobWindPoint kLow[2] = {
      {0.0, 0.0, 7.33, 1.0},
      {1500.0, 0.0, 14.66, 0.05},
  };
  lob::Builder b3;
  b3.BallisticCoefficientPsi(0.372)
      .BCDragFunction(lob::DragFunctionT::kG1)
      .DiameterInch(0.224)
      .MassGrains(77.0)
      .InitialVelocityFps(2720)
      .ZeroAngleMOA(4.78)
      .OpticHeightInches(2.5)
      .WindProfile(kLow, 2)
      .WindRoughnessLengthFt(0.1)
      .HeightOfBoreAboveGroundFt(1.0);
  EXPECT_EQ(b3.Build().error, lob::ErrorT::kWindProfileInvalid);
}

namespace {

using lob::CartesianT;
using lob::FeetT;
using lob::FpsT;
using lob::TrajectoryStateT;

LobContext MakeWindQueryCtx() {
  LobContext ctx{};
  ctx.gravity.x = 0.0;
  ctx.gravity.y = -lob::kStandardGravityFtPerSecSq;
  ctx.wind.x = 0.0;
  ctx.wind.z = 10.0;
  ctx.wind_count = 1;
  ctx.wind_cos = 1.0;
  ctx.wind_sin = 0.0;
  ctx.wind_roughness_ft = std::numeric_limits<double>::quiet_NaN();
  ctx.wind_muzzle_height_ft = std::numeric_limits<double>::quiet_NaN();
  ctx.wind_inv_ln_denom = std::numeric_limits<double>::quiet_NaN();
  return ctx;
}

TrajectoryStateT MakeStateAt(double x_ft, double y_ft) {
  return TrajectoryStateT(
      CartesianT<FeetT>(FeetT(x_ft), FeetT(y_ft), FeetT(0.0)),
      CartesianT<FpsT>(FpsT(2000.0), FpsT(0.0), FpsT(0.0)));
}

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
  ctx.wind_points[0] = {1000.0, 0.0, 10.0,
                        std::numeric_limits<double>::quiet_NaN()};
  ctx.wind_points[1] = {2000.0, 0.0, 30.0,
                        std::numeric_limits<double>::quiet_NaN()};
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
  ctx.wind.x = 10.0;  // pure horizontal tailwind
  ctx.wind.z = 4.0;
  const double kTheta = lob::RadiansT(lob::DegreesT(8.0)).Value();
  ctx.wind_cos = std::cos(kTheta);
  ctx.wind_sin = std::sin(kTheta);
  const CartesianT<FpsT> kW = lob::GetWind(ctx, MakeStateAt(100.0, 0.0));
  EXPECT_NEAR(kW.X().Value(), 10.0 * std::cos(kTheta), 1E-9);
  EXPECT_NEAR(kW.Y().Value(), -10.0 * std::sin(kTheta), 1E-9);
  EXPECT_DOUBLE_EQ(kW.Z().Value(), 4.0);  // lateral untouched
}

TEST(WindProfileQuery, AltitudeScalesAboutMuzzleReference) {
  // z0 = 0.1 ft, muzzle 1 ft: at true height 9 ft, z_agl = 10, S = 2 exactly.
  LobContext ctx = MakeWindQueryCtx();
  ctx.wind_roughness_ft = 0.1;
  ctx.wind_muzzle_height_ft = 1.0;
  ctx.wind_inv_ln_denom = 1.0 / std::log(1.0 / 0.1);
  const CartesianT<FpsT> kAtMuzzle = lob::GetWind(ctx, MakeStateAt(0.0, 0.0));
  EXPECT_NEAR(kAtMuzzle.Z().Value(), 10.0, 1E-9);  // S = 1 at muzzle
  const CartesianT<FpsT> kHigh = lob::GetWind(ctx, MakeStateAt(0.0, 9.0));
  EXPECT_NEAR(kHigh.Z().Value(), 20.0, 1E-9);  // S = ln(100)/ln(10) = 2
}

TEST(WindProfileQuery, SubBoreFloorKeepsLogDefined) {
  LobContext ctx = MakeWindQueryCtx();
  ctx.wind_roughness_ft = 0.1;
  ctx.wind_muzzle_height_ft = 1.0;
  ctx.wind_inv_ln_denom = 1.0 / std::log(1.0 / 0.1);
  const CartesianT<FpsT> kW = lob::GetWind(ctx, MakeStateAt(2500.0, -40.0));
  EXPECT_TRUE(std::isfinite(kW.X().Value()));
  EXPECT_TRUE(std::isfinite(kW.Y().Value()));
  EXPECT_TRUE(std::isfinite(kW.Z().Value()));
  EXPECT_GE(kW.Z().Value(), 0.0);
}

TEST_F(WindProfileBuildFixture, TwoPointFlatProfileMatchesUniformSolve) {
  // Constant-valued 2-point profile must reproduce the uniform solution.
  const double kFps = 10.0 * 22.0 / 15.0;
  const LobWindPoint kFlat[2] = {
      {0.0, 0.0, kFps, std::numeric_limits<double>::quiet_NaN()},
      {3000.0, 0.0, kFps, std::numeric_limits<double>::quiet_NaN()}};
  const std::array<uint32_t, 4> kRanges = {300, 900, 1800, 3000};
  std::array<lob::Output, 4> profile_outs{};
  std::array<lob::Output, 4> uniform_outs{};
  const size_t kNProfile =
      lob::Solve(builder.WindProfile(kFlat, 2).Build(), kRanges, &profile_outs);
  lob::Builder ub;
  ub.BallisticCoefficientPsi(0.372)
      .BCDragFunction(lob::DragFunctionT::kG1)
      .DiameterInch(0.224)
      .MassGrains(77.0)
      .InitialVelocityFps(2720)
      .ZeroAngleMOA(4.78)
      .OpticHeightInches(2.5)
      .WindHeading(lob::ClockAngleT::kIII)
      .WindSpeedFps(kFps);
  const size_t kNUniform = lob::Solve(ub.Build(), kRanges, &uniform_outs);
  EXPECT_EQ(kNProfile, kNUniform);
  for (size_t i = 0; i < kNProfile; ++i) {
    EXPECT_DOUBLE_EQ(profile_outs[i].deflection, uniform_outs[i].deflection);
    EXPECT_DOUBLE_EQ(profile_outs[i].elevation, uniform_outs[i].elevation);
  }
}

TEST_F(WindProfileBuildFixture, CrosswindBlindToIncline) {
  // Spec section 9: pure crosswind resolves identically in the query;
  // pitch cannot touch pure-crosswind output.
  lob::Builder qb;
  qb.BallisticCoefficientPsi(0.372)
      .BCDragFunction(lob::DragFunctionT::kG1)
      .DiameterInch(0.224)
      .MassGrains(77.0)
      .InitialVelocityFps(2720)
      .ZeroAngleMOA(4.78)
      .OpticHeightInches(2.5)
      .WindHeading(lob::ClockAngleT::kIII)
      .WindSpeedMph(10.0);
  const lob::Context kFlatCtx = qb.Build();
  lob::Builder qh;
  qh.BallisticCoefficientPsi(0.372)
      .BCDragFunction(lob::DragFunctionT::kG1)
      .DiameterInch(0.224)
      .MassGrains(77.0)
      .InitialVelocityFps(2720)
      .ZeroAngleMOA(4.78)
      .OpticHeightInches(2.5)
      .WindHeading(lob::ClockAngleT::kIII)
      .WindSpeedMph(10.0)
      .RangeAngleDeg(15.0);
  const lob::Context kHillCtx = qh.Build();
  const LobContext& kFlatRaw = reinterpret_cast<const LobContext&>(kFlatCtx);
  const LobContext& kHillRaw = reinterpret_cast<const LobContext&>(kHillCtx);
  const double kXs[3] = {100.0, 900.0, 2700.0};
  for (double x : kXs) {
    const CartesianT<FpsT> kFlatW = lob::GetWind(kFlatRaw, MakeStateAt(x, 0.0));
    const CartesianT<FpsT> kHillW = lob::GetWind(kHillRaw, MakeStateAt(x, 0.0));
    // ponytail: kIII heading is 2π rad; libm sin leaves ~3.6e-15 in
    // wind.x which pitch then scales — same 1e-12 tolerance as
    // SinglePointEqualsUniform. The crosswind (Z) component is bit-identical.
    EXPECT_NEAR(kFlatW.X().Value(), kHillW.X().Value(), 1e-12);
    EXPECT_NEAR(kFlatW.Y().Value(), kHillW.Y().Value(), 1e-12);
    EXPECT_DOUBLE_EQ(kFlatW.Z().Value(), kHillW.Z().Value());
  }
  // Solve-level: gravity pitches on incline so TOF differs slightly.
  const std::array<uint32_t, 3> kRanges = {900, 1800, 2700};
  std::array<lob::Output, 3> flat_outs{};
  std::array<lob::Output, 3> hill_outs{};
  const size_t kNFlat = lob::Solve(
      builder.WindHeading(lob::ClockAngleT::kIII).WindSpeedMph(10.0).Build(),
      kRanges, &flat_outs);
  lob::Builder hb;
  hb.BallisticCoefficientPsi(0.372)
      .BCDragFunction(lob::DragFunctionT::kG1)
      .DiameterInch(0.224)
      .MassGrains(77.0)
      .InitialVelocityFps(2720)
      .ZeroAngleMOA(4.78)
      .OpticHeightInches(2.5)
      .WindHeading(lob::ClockAngleT::kIII)
      .WindSpeedMph(10.0)
      .RangeAngleDeg(15.0);
  const size_t kNHill = lob::Solve(hb.Build(), kRanges, &hill_outs);
  EXPECT_EQ(kNFlat, kNHill);
  for (size_t i = 0; i < kNFlat; ++i) {
    // Residual is pre-existing gravity pitch, not wind (observed ~0.01 @900
    // growing with range; 0.5 catches real wind bugs, which would be inches).
    EXPECT_NEAR(hill_outs[i].deflection, flat_outs[i].deflection, 0.5);
  }
}

TEST_F(WindProfileBuildFixture, AltitudeScalingGrowsApexDrift) {
  // Same profile with/without grass roughness: muzzle output identical,
  // far output drifts further with scaling on.
  const LobWindPoint kPts[2] = {
      {0.0, 0.0, 14.66, std::numeric_limits<double>::quiet_NaN()},
      {3000.0, 0.0, 14.66, std::numeric_limits<double>::quiet_NaN()}};
  const std::array<uint32_t, 3> kRanges = {300, 1500, 3000};
  std::array<lob::Output, 3> plain_outs{};
  std::array<lob::Output, 3> scaled_outs{};
  lob::Solve(builder.WindProfile(kPts, 2).Build(), kRanges, &plain_outs);
  lob::Builder sb;
  sb.BallisticCoefficientPsi(0.372)
      .BCDragFunction(lob::DragFunctionT::kG1)
      .DiameterInch(0.224)
      .MassGrains(77.0)
      .InitialVelocityFps(2720)
      .ZeroAngleMOA(4.78)
      .OpticHeightInches(2.5)
      .WindProfile(kPts, 2)
      .WindRoughnessLengthFt(0.1)
      .HeightOfBoreAboveGroundFt(1.0);
  lob::Solve(sb.Build(), kRanges, &scaled_outs);
  // Apex-region growth, the feature's intent (S>1 above muzzle).
  EXPECT_GT(scaled_outs[0].deflection, plain_outs[0].deflection);
  // Sub-bore floor attenuation — the 100-yd-zero fixture drops far below bore
  // level by 3000 ft, where the floor clamps wind to ~2%.
  EXPECT_LT(scaled_outs[2].deflection, plain_outs[2].deflection);
}
