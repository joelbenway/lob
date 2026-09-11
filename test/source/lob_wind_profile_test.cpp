// Copyright (c) 2026  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

#include "lob/lob.h"
#include "lob/lob.hpp"

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
  const lob::Context kCtx =
      builder.WindProfile(kTwoPoint, 2).Build();
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
  const LobWindPoint kOne[1] = {{0.0, 0.0, 7.33,
                                 std::numeric_limits<double>::quiet_NaN()}};
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
    many[i].range_ft = 100.0 * i;
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
  const lob::Context kProfileLast =
      builder.WindHeading(lob::ClockAngleT::kIII)
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
