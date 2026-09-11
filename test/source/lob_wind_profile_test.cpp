// Copyright (c) 2026  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>

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
