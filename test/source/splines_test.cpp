// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include "splines.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "tables.hpp"

namespace {

using lob::spline::Arr;
using lob::spline::CubicCoeffs;
using lob::spline::Eval;
using lob::spline::Fit;
using lob::spline::kNumSplineKnots;
using lob::spline::kNumSplinePieces;
using lob::spline::kSplineKnots;

struct SplineInfo {
  const char* name;
  const std::array<uint16_t, lob::kTableSize>* drag_table;
  Arr<double, kNumSplineKnots> x0;
  Arr<CubicCoeffs, kNumSplinePieces> coeffs;
  double max_err;
};

template <const std::array<uint16_t, lob::kTableSize>& kDragTable,
          const std::array<size_t, kNumSplineKnots>& kKnots>
SplineInfo MakeInfo(const char* name) noexcept {
  constexpr auto kSpline = Fit<kDragTable, kKnots>();
  double max_err = 0.0;
  for (size_t i = 0; i < lob::kTableSize; ++i) {
    const auto kXi = static_cast<double>(lob::kMachs.at(i));
    const auto kActual = static_cast<double>(kDragTable.at(i));
    const double kVal = Eval(kSpline.x0, kSpline.coeffs, kXi);
    const double kErr = std::abs(kVal - kActual) / kActual * 100.0;
    max_err = std::max(max_err, kErr);
  }
  return {name, &kDragTable, kSpline.x0, kSpline.coeffs, max_err};
}

const auto kInfos = std::array<SplineInfo, 6>{
    MakeInfo<lob::kG1Drags, kSplineKnots>("G1"),
    MakeInfo<lob::kG2Drags, kSplineKnots>("G2"),
    MakeInfo<lob::kG5Drags, kSplineKnots>("G5"),
    MakeInfo<lob::kG6Drags, kSplineKnots>("G6"),
    MakeInfo<lob::kG7Drags, kSplineKnots>("G7"),
    MakeInfo<lob::kG8Drags, kSplineKnots>("G8"),
};

}  // namespace

namespace tests {

TEST(SplineTest, MaxErrorBelow1Pct) {
  for (const auto& info : kInfos) {
    EXPECT_LT(info.max_err, 1.0) << info.name << " max error exceeds 1%";
  }
}

TEST(SplineTest, KnotValuesMatchExactly) {
  for (const auto& info : kInfos) {
    for (size_t i = 0; i < kNumSplineKnots; ++i) {
      const size_t kKi = kSplineKnots.at(i);
      const auto kExpected = static_cast<double>(info.drag_table->at(kKi));
      const double kActual = Eval(info.x0, info.coeffs, info.x0[i]);
      EXPECT_DOUBLE_EQ(kActual, kExpected)
          << info.name << " knot " << i << " (table idx " << kKi << ")";
    }
  }
}

TEST(SplineTest, BelowRangeClamps) {
  const auto& info = kInfos.at(0);
  const double kVal = Eval(info.x0, info.coeffs, -1.0);
  EXPECT_DOUBLE_EQ(kVal, static_cast<double>(lob::kG1Drags.at(0)));
}

TEST(SplineTest, AboveRangeClamps) {
  const auto& info = kInfos.at(0);
  const double kVal = Eval(info.x0, info.coeffs, 1e9);
  const size_t kLast = kSplineKnots.back();
  EXPECT_DOUBLE_EQ(kVal, static_cast<double>(lob::kG1Drags.at(kLast)));
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
// warranty; even the implied warranty of MERCHANTABILITY or FITNESS FOR A
// PARTICULAR PURPOSE. See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along with
// lob. If not, see <https://www.gnu.org/licenses/>.
