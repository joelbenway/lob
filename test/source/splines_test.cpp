// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include "splines.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <random>
#include <sstream>

#include "tables.hpp"

namespace {

constexpr auto kEps = 1.0e-5F;
constexpr auto kEpsLoose = 1.0e-3F;

constexpr std::array<float, 5> kLinearX{0.0F, 1.0F, 2.0F, 3.0F, 4.0F};
constexpr std::array<float, 5> kLineFn{1.0F, 3.0F, 5.0F, 7.0F, 9.0F};

constexpr std::array<float, 4> kQuadX{0.0F, 1.0F, 2.0F, 3.0F};
constexpr std::array<float, 4> kQuadY{0.0F, 1.0F, 4.0F, 9.0F};

constexpr std::array<float, 5> kFcX{0.0F, 1.0F, 2.0F, 3.0F, 4.0F};
constexpr std::array<float, 5> kFcY{0.0F, 1.0F, 0.0F, -1.0F, 0.0F};

constexpr std::array<float, 5> kMonotonicX{0.0F, 0.5F, 1.0F, 2.0F, 5.0F};
constexpr std::array<float, 5> kMonotonicY{0.0F, 1.0F, 2.0F, 4.0F, 10.0F};

constexpr auto SecantRef(const float* x, const float* y, size_t i) {
  return (*(y + i + 1) - *(y + i)) / (*(x + i + 1) - *(x + i));
}

constexpr auto LineFn(float x) { return (2 * x) + 1.0F; }

constexpr size_t kTruthSize = 2000;
constexpr size_t kTargetKnotSize = lob::spline::kKnotCount;

// Absolute accuracy budget for the checked-in kKnots, in Cd units. This is
// what CI actually enforces. After the first run, read the recorded
// BaselineMaxError property and tighten this to ~1.2x that value so a Pchip
// regression or corrupted kKnots array fails loudly.

constexpr float kMaxAllowedErr = 5e-3F;
struct TruthTables {
  std::array<float, kTruthSize> machs{};
  std::array<float, kTruthSize> g1{};
  std::array<float, kTruthSize> g7{};
};

TruthTables BuildTruthTables() {
  TruthTables t;
  const float kMinMach = lob::dragtable::kMachs.front();
  const float kMaxMach = lob::dragtable::kMachs.back();
  const float kStepMach = (kMaxMach - kMinMach) / (kTruthSize - 1);

  for (size_t i = 0; i < kTruthSize; ++i) {
    const float kM = kMinMach + (static_cast<float>(i) * kStepMach);
    t.machs.at(i) = kM;

    t.g1.at(i) =
        lob::spline::detail::EvalWithDeriv(lob::dragtable::kMachs.data(),
                                           lob::dragtable::kG1Drags.data(),
                                           lob::dragtable::kTableSize, kM)
            .y;

    t.g7.at(i) =
        lob::spline::detail::EvalWithDeriv(lob::dragtable::kMachs.data(),
                                           lob::dragtable::kG7Drags.data(),
                                           lob::dragtable::kTableSize, kM)
            .y;
  }
  return t;
}

// Snap x to the NEAREST truth-grid index. (lower_bound alone always rounds
// up, biasing every knot slightly right of its true value.)

unsigned int FindNearestGridIndex(const std::array<float, kTruthSize>& grid,
                                  float x) {
  const auto* const it = std::lower_bound(grid.begin(), grid.end(), x);

  if (it == grid.end()) {
    return static_cast<unsigned int>(kTruthSize - 1);
  }

  auto idx = static_cast<unsigned int>(std::distance(grid.begin(), it));
  if (idx > 0 && std::abs(grid.at(idx - 1) - x) < std::abs(grid.at(idx) - x)) {
    --idx;
  }
  return idx;
}

// Max abs error (worst of G1/G7) of a spline over the whole truth grid.
// Optionally writes the per-point errors.

float EvalMaxError(const TruthTables& t,
                   const std::array<float, kTargetKnotSize>& sample_machs,
                   const std::array<float, kTargetKnotSize>& sample_g1,
                   const std::array<float, kTargetKnotSize>& sample_g7,
                   std::array<float, kTruthSize>* errors_out = nullptr) {
  float max_err = 0.0F;
  for (size_t i = 0; i < kTruthSize; ++i) {
    const float kEvalG1 = lob::spline::detail::EvalWithDeriv(
                              sample_machs.data(), sample_g1.data(),
                              kTargetKnotSize, t.machs.at(i))
                              .y;

    const float kEvalG7 = lob::spline::detail::EvalWithDeriv(
                              sample_machs.data(), sample_g7.data(),
                              kTargetKnotSize, t.machs.at(i))
                              .y;

    const float kErr = std::max(std::abs(kEvalG1 - t.g1.at(i)),
                                std::abs(kEvalG7 - t.g7.at(i)));
    if (errors_out != nullptr) {
      errors_out->at(i) = kErr;
    }
    max_err = std::max(max_err, kErr);
  }
  return max_err;
}

// Error of the ACTUAL checked-in kKnots (exact mach values, not snapped to
// the truth grid), so the reported baseline is faithful to production.

float BaselineError(const TruthTables& t) {
  std::array<float, kTargetKnotSize> sample_machs{};
  std::array<float, kTargetKnotSize> sample_g1{};
  std::array<float, kTargetKnotSize> sample_g7{};

  for (size_t i = 0; i < kTargetKnotSize; ++i) {
    const float kMach = lob::spline::kKnots.at(i);
    sample_machs.at(i) = kMach;
    sample_g1.at(i) =
        lob::spline::detail::EvalWithDeriv(lob::dragtable::kMachs.data(),
                                           lob::dragtable::kG1Drags.data(),
                                           lob::dragtable::kTableSize, kMach)
            .y;

    sample_g7.at(i) =
        lob::spline::detail::EvalWithDeriv(lob::dragtable::kMachs.data(),
                                           lob::dragtable::kG7Drags.data(),
                                           lob::dragtable::kTableSize, kMach)
            .y;
  }
  return EvalMaxError(t, sample_machs, sample_g1, sample_g7);
}

}  // namespace

namespace tests {

TEST(SplinesDetailSecantTest, ReturnsSlopeBetweenAdjacentPoints) {
  EXPECT_FLOAT_EQ(
      lob::spline::detail::Secant<float>(kLinearX.data(), kLineFn.data(), 0),
      2);
  EXPECT_FLOAT_EQ(
      lob::spline::detail::Secant<float>(kLinearX.data(), kLineFn.data(), 3),
      2);
  EXPECT_FLOAT_EQ(
      lob::spline::detail::Secant<float>(kQuadX.data(), kQuadY.data(), 0), 1);
  EXPECT_FLOAT_EQ(
      lob::spline::detail::Secant<float>(kQuadX.data(), kQuadY.data(), 2), 5);
  EXPECT_FLOAT_EQ(lob::spline::detail::Secant<float>(
                      kQuadX.data(), kQuadY.data(), kQuadX.size() - 2),
                  SecantRef(kQuadX.data(), kQuadY.data(), kQuadX.size() - 2));
}

TEST(SplinesDetailTangentTest, TwoPointInputReturnsSecantOfFirstSpan) {
  const std::array<float, 2> kX{0, 4};
  const std::array<float, 2> kY{0, 8};
  const auto kExpected = (*(kY.data() + 1) - *(kY.data() + 0)) /
                         (*(kX.data() + 1) - *(kX.data() + 0));
  EXPECT_FLOAT_EQ(
      lob::spline::detail::Tangent<float>(kX.data(), kY.data(), kX.size(), 0),
      kExpected);
  EXPECT_FLOAT_EQ(
      lob::spline::detail::Tangent<float>(kX.data(), kY.data(), kX.size(), 1),
      kExpected);
}

TEST(SplinesDetailTangentTest, LeftEndCallsEndTangent) {
  const auto kResult = lob::spline::detail::Tangent<float>(
      kLinearX.data(), kLineFn.data(), kLinearX.size(), 0);
  EXPECT_GT(kResult, 0);
  EXPECT_NEAR(kResult, 2, kEpsLoose);
}

TEST(SplinesDetailTangentTest, RightEndCallsEndTangent) {
  const auto kResult = lob::spline::detail::Tangent<float>(
      kLinearX.data(), kLineFn.data(), kLinearX.size(), kLinearX.size() - 1);
  EXPECT_GT(kResult, 0);
  EXPECT_NEAR(kResult, 2, kEpsLoose);
}

TEST(SplinesDetailTangentTest, InteriorReturnsHarmonicMeanForLinear) {
  const auto kN = kLinearX.size();
  for (size_t i = 1; i + 1 < kN; ++i) {
    const auto kResult = lob::spline::detail::Tangent<float>(
        kLinearX.data(), kLineFn.data(), kN, i);
    EXPECT_NEAR(kResult, 2, kEpsLoose) << "i=" << i;
  }
}

TEST(SplinesDetailTangentTest, ReturnsZeroWhenAdjacentSecantsDisagree) {
  const auto kTan = lob::spline::detail::Tangent<float>(
      kFcX.data(), kFcY.data(), kFcX.size(), 1);
  EXPECT_FLOAT_EQ(kTan, 0);
}

TEST(SplinesDetailTangentTest, InteriorMonotonicIsAverageSecantApproximately) {
  const auto kN = kMonotonicX.size();
  for (size_t i = 1; i + 1 < kN; ++i) {
    const auto kPrev = lob::spline::detail::Secant<float>(
        kMonotonicX.data(), kMonotonicY.data(), i - 1);
    const auto kNext = lob::spline::detail::Secant<float>(
        kMonotonicX.data(), kMonotonicY.data(), i);
    ASSERT_TRUE(kPrev * kNext > 0);
    const auto kTan = lob::spline::detail::Tangent<float>(
        kMonotonicX.data(), kMonotonicY.data(), kN, i);
    const auto kLo = std::min(kPrev, kNext);
    const auto kHi = std::max(kPrev, kNext);
    EXPECT_GE(kTan, kLo - kEpsLoose);
    EXPECT_LE(kTan, kHi + kEpsLoose);
  }
}

TEST(SplinesDetailHermiteTest, LinearDataProducesLinearCoefs) {
  constexpr auto kSample5 = size_t{5};
  std::array<float, 4> coefs{};
  // NOLINTNEXTLINE
  lob::spline::detail::Hermite<float>(1, 3, 2, 6, 2, 2, coefs.data());
  EXPECT_FLOAT_EQ(*(coefs.data() + 0), 2);
  EXPECT_FLOAT_EQ(*(coefs.data() + 1), 2);
  EXPECT_NEAR(*(coefs.data() + 2), 0, kEps);
  EXPECT_NEAR(*(coefs.data() + 3), 0, kEps);
  for (size_t i = 0; i < kSample5; ++i) {
    const auto kT = static_cast<float>(i);
    const auto kExpectedY = (2 * kT) + 2;
    EXPECT_FLOAT_EQ(lob::spline::detail::PolyVal<float>(coefs.data(), kT),
                    kExpectedY);
  }
}

TEST(SplinesDetailHermiteTest, EndpointsExactAndTangentsPreserved) {
  std::array<float, 4> coefs{};
  // NOLINTNEXTLINE
  lob::spline::detail::Hermite<float>(0, 2, -1, 5, 0.5, -0.5, coefs.data());
  EXPECT_FLOAT_EQ(lob::spline::detail::PolyVal<float>(coefs.data(), 0), -1);
  EXPECT_FLOAT_EQ(lob::spline::detail::PolyVal<float>(coefs.data(), 2), 5);
  EXPECT_FLOAT_EQ(lob::spline::detail::PolyDeriv<float>(coefs.data(), 0), 0.5);
  EXPECT_FLOAT_EQ(lob::spline::detail::PolyDeriv<float>(coefs.data(), 2), -0.5);
}

TEST(SplinesDetailHermiteTest, SecantOfSplineMatchesAverageSlope) {
  std::array<float, 4> coefs{};
  // NOLINTNEXTLINE
  lob::spline::detail::Hermite<float>(0, 4, 1, 9, 1, 2, coefs.data());
  const auto kY0Got = lob::spline::detail::PolyVal<float>(coefs.data(), 0);
  const auto kY1Got = lob::spline::detail::PolyVal<float>(coefs.data(), 4);
  EXPECT_FLOAT_EQ((kY1Got - kY0Got) / 4, static_cast<float>(9 - 1) / 4);
}

TEST(SplinesDetailHermiteTest, WritesAllFourCoefsEvenWhenZero) {
  std::array<float, 4> coefs{1, 1, 1, 1};
  lob::spline::detail::Hermite<float>(0, 1, 0, 0, 0, 0, coefs.data());
  EXPECT_FLOAT_EQ(*(coefs.data() + 0), 0);
  EXPECT_FLOAT_EQ(*(coefs.data() + 1), 0);
  EXPECT_FLOAT_EQ(*(coefs.data() + 2), 0);
  EXPECT_FLOAT_EQ(*(coefs.data() + 3), 0);
}

TEST(SplinesDetailPolyValTest, ReturnsConstantWhenOnlyC0) {
  constexpr auto kC0 = 3.14F;
  constexpr auto kSample5 = size_t{5};
  std::array<float, 4> c{kC0, 0, 0, 0};
  for (size_t i = 0; i < kSample5; ++i) {
    const auto kT = static_cast<float>(i);
    EXPECT_FLOAT_EQ(lob::spline::detail::PolyVal<float>(c.data(), kT), kC0);
  }
}

TEST(SplinesDetailPolyValTest, ReturnsLinearWhenOnlyC0C1) {
  constexpr auto kStep01 = 0.01F;
  constexpr auto kLoop = 200U;
  constexpr auto kC0 = -2;
  constexpr auto kC1 = 0.5F;
  std::array<float, 4> c{kC0, kC1, 0, 0};
  for (size_t i = 0; i < kLoop; ++i) {
    const auto kT = static_cast<float>(i) * kStep01;
    EXPECT_FLOAT_EQ(lob::spline::detail::PolyVal<float>(c.data(), kT),
                    kC0 + (kC1 * kT));
  }
}

TEST(SplinesDetailPolyValTest, ReturnsQuadraticWhenOnlyC0C1C2) {
  constexpr auto kStep01 = 0.01F;
  constexpr auto kLoop = 200U;
  std::array<float, 4> c{1, 0, 2, 0};
  for (size_t i = 0; i < kLoop; ++i) {
    const auto kT = static_cast<float>(i) * kStep01;
    EXPECT_FLOAT_EQ(lob::spline::detail::PolyVal<float>(c.data(), kT),
                    1 + (2 * kT * kT));
  }
}

TEST(SplinesDetailPolyValTest, ReturnsFullCubicForCompleteInput) {
  constexpr auto kStep01 = 0.01F;
  constexpr auto kLoop = 200U;
  std::array<float, 4> c{1, 2, 3, 4};
  for (size_t i = 0; i < kLoop; ++i) {
    const auto kT = static_cast<float>(i) * kStep01;
    const auto kExpected =
        ((1 + (2 * kT)) + (3 * kT * kT)) + (4 * kT * kT * kT);
    EXPECT_FLOAT_EQ(lob::spline::detail::PolyVal<float>(c.data(), kT),
                    kExpected);
  }
}

TEST(SplinesDetailPolyDerivTest, ConstantSplineHasZeroDeriv) {
  constexpr auto kSize = size_t{10};
  std::array<float, 4> c{3, 0, 0, 0};
  for (size_t i = 0; i < kSize; ++i) {
    const auto kT = static_cast<float>(i);
    EXPECT_FLOAT_EQ(lob::spline::detail::PolyDeriv<float>(c.data(), kT), 0);
  }
}

TEST(SplinesDetailPolyDerivTest, LinearSplineHasConstantDeriv) {
  constexpr auto kLoop = 100U;
  const std::array<float, 4> kC{0, 0.5, 0, 0};
  for (size_t i = 0; i < kLoop; ++i) {
    const auto kT = static_cast<float>(i) * 0.1F;
    EXPECT_FLOAT_EQ(lob::spline::detail::PolyDeriv<float>(kC.data(), kT), 0.5);
  }
}

TEST(SplinesDetailPolyDerivTest, QuadraticSplineHasLinearDeriv) {
  constexpr auto kLoop = 100U;
  const std::array<float, 4> kC{1, 0, 2, 0};
  for (size_t i = 0; i < kLoop; ++i) {
    const auto kT = static_cast<float>(i) * 0.1F;
    EXPECT_FLOAT_EQ(lob::spline::detail::PolyDeriv<float>(kC.data(), kT),
                    (2 * 2 * kT));
  }
}

TEST(SplinesDetailPolyDerivTest, CubicSplineHasQuadraticDeriv) {
  constexpr auto kLoop = 100U;
  const std::array<float, 4> kC{0, 2, 3, 4};
  for (size_t i = 0; i < kLoop; ++i) {
    const auto kT = static_cast<float>(i) * 0.1F;
    EXPECT_FLOAT_EQ(lob::spline::detail::PolyDeriv<float>(kC.data(), kT),
                    ((2 + ((2 * 3) * kT)) + ((3 * 4) * kT * kT)));
  }
}

TEST(SplinesDetailPolyDerivTest, DerivAndValAtTimeZeroMatchesC1AndC0) {
  constexpr auto kC0 = 5.5F;
  constexpr auto kC1 = -1.5F;
  std::array<float, 4> c{kC0, kC1, 0, 0};
  EXPECT_FLOAT_EQ(lob::spline::detail::PolyVal<float>(c.data(), 0), kC0);
  EXPECT_FLOAT_EQ(lob::spline::detail::PolyDeriv<float>(c.data(), 0), kC1);
}

TEST(SplinesDetailFindIntervalTest, ReturnsZeroAtOrBeforeFirstKnot) {
  EXPECT_EQ(lob::spline::detail::FindInterval<float>(kLinearX.data(),
                                                     kLinearX.size(), 0),
            0U);
  EXPECT_EQ(lob::spline::detail::FindInterval<float>(kLinearX.data(),
                                                     kLinearX.size(), -1),
            0U);
  EXPECT_EQ(lob::spline::detail::FindInterval<float>(kLinearX.data(),
                                                     kLinearX.size(), 0.5),
            0U);
}

TEST(SplinesDetailFindIntervalTest, ReturnsLastSegmentForTopKnot) {
  EXPECT_EQ(lob::spline::detail::FindInterval<float>(kLinearX.data(),
                                                     kLinearX.size(), 4),
            kLinearX.size() - 2);
  EXPECT_EQ(lob::spline::detail::FindInterval<float>(kLinearX.data(),
                                                     kLinearX.size(), 4),
            3U);
}

TEST(SplinesDetailFindIntervalTest, ReturnsLastSegmentAboveTop) {
  constexpr auto kTen = 10.0F;
  EXPECT_EQ(lob::spline::detail::FindInterval<float>(kLinearX.data(),
                                                     kLinearX.size(), kTen),
            kLinearX.size() - 2);
}

TEST(SplinesDetailFindIntervalTest, WalksEachInteriorInterval) {
  const auto kN = kLinearX.size();
  for (size_t i = 0; i + 1 < kN; ++i) {
    const float kV =
        (*(kLinearX.data() + i) + *(kLinearX.data() + i + 1)) * 0.5F;
    ASSERT_EQ(lob::spline::detail::FindInterval<float>(kLinearX.data(), kN, kV),
              i);
  }
  for (size_t i = 1; i + 1 < kN; ++i) {
    ASSERT_EQ(lob::spline::detail::FindInterval<float>(kLinearX.data(), kN,
                                                       *(kLinearX.data() + i)),
              i);
  }
}

TEST(SplinesDetailFindIntervalTest, HandlesNonUniformSpacing) {
  const auto kN = kMonotonicX.size();
  for (size_t i = 0; i + 1 < kN; ++i) {
    const float kV =
        (*(kMonotonicX.data() + i) + *(kMonotonicX.data() + i + 1)) * 0.5F;
    ASSERT_EQ(
        lob::spline::detail::FindInterval<float>(kMonotonicX.data(), kN, kV),
        i);
  }
}

TEST(SplinesDetailFindIntervalTest, ReturnsZeroForTwoPointInput) {
  constexpr auto kHigh = 10.0F;
  constexpr auto kMid = 5.0F;
  constexpr auto kNearHigh = 9.9F;
  std::array<float, 2> x{0, kHigh};
  EXPECT_EQ(lob::spline::detail::FindInterval<float>(x.data(), x.size(), kMid),
            0U);
  EXPECT_EQ(
      lob::spline::detail::FindInterval<float>(x.data(), x.size(), kNearHigh),
      0U);
}

TEST(SplinesDetailEvalWithDerivTest, ValueMatchesInputAtKnotsForLinear) {
  const auto kN = kLinearX.size();
  for (size_t i = 0; i < kN; ++i) {
    const auto kResult = lob::spline::detail::EvalWithDeriv<float>(
        kLinearX.data(), kLineFn.data(), kN, *(kLinearX.data() + i));
    EXPECT_FLOAT_EQ(kResult.y, *(kLineFn.data() + i));
  }
}

TEST(SplinesDetailEvalWithDerivTest, ValueMatchesInputAtKnotsForQuadratic) {
  const auto kN = kQuadX.size();
  for (size_t i = 0; i < kN; ++i) {
    const auto kResult = lob::spline::detail::EvalWithDeriv<float>(
        kQuadX.data(), kQuadY.data(), kN, *(kQuadX.data() + i));
    EXPECT_NEAR(kResult.y, *(kQuadY.data() + i), kEpsLoose) << "i=" << i;
  }
}

TEST(SplinesDetailEvalWithDerivTest, ApproximatesMidspanWithinTolerance) {
  const auto kN = kLinearX.size();
  for (size_t i = 0; i + 1 < kN; ++i) {
    const float kMid =
        0.5F * (*(kLinearX.data() + i) + *(kLinearX.data() + i + 1));
    const auto kResult = lob::spline::detail::EvalWithDeriv<float>(
        kLinearX.data(), kLineFn.data(), kN, kMid);
    EXPECT_NEAR(kResult.y, LineFn(kMid), kEpsLoose);
  }
}

TEST(SplinesDetailEvalWithDerivTest, ReturnsExactLinearDerivForLinearData) {
  const auto kN = kLinearX.size();
  for (size_t i = 1; i + 1 < kN; ++i) {
    const auto kResult = lob::spline::detail::EvalWithDeriv<float>(
        kLinearX.data(), kLineFn.data(), kN, *(kLinearX.data() + i));
    EXPECT_NEAR(kResult.dy, 2, kEpsLoose) << "i=" << i;
  }
}

TEST(SplinesDetailEvalWithDerivTest, BelowFirstKnotFindsFirstSegment) {
  constexpr auto kTen = 10.0F;
  const auto kN = kLinearX.size();
  EXPECT_EQ(
      lob::spline::detail::FindInterval<float>(kLinearX.data(), kN, -kTen), 0U);
  const auto kResult = lob::spline::detail::EvalWithDeriv<float>(
      kLinearX.data(), kLineFn.data(), kN, -kTen);
  EXPECT_TRUE(std::isfinite(kResult.y));
  EXPECT_TRUE(std::isfinite(kResult.dy));
}

TEST(SplinesDetailEvalWithDerivTest, AboveLastKnotClampsToLastInterval) {
  const auto kN = kLinearX.size();
  const auto kResult = lob::spline::detail::EvalWithDeriv<float>(
      kLinearX.data(), kLineFn.data(), kN, 1000.0F);
  EXPECT_GT(kResult.y, *(kLineFn.data() + kN - 2));
}

TEST(SplinesSegmentTest, ProducesCoefsReproducingDataAtKnots) {
  std::array<float, 4> coefs{};
  lob::spline::Segment<float>(kLinearX.data(), kLineFn.data(), kLinearX.size(),
                              0, 1, coefs.data());
  EXPECT_FLOAT_EQ(lob::spline::detail::PolyVal<float>(coefs.data(), 0),
                  *(kLineFn.data() + 0));
  EXPECT_NEAR(lob::spline::detail::PolyVal<float>(coefs.data(), 1),
              *(kLineFn.data() + 1), kEpsLoose);
}

TEST(SplinesSegmentTest, TangentsAtKnotsMatchEndEndTangents) {
  std::array<float, 4> coefs{};
  lob::spline::Segment<float>(kLinearX.data(), kLineFn.data(), kLinearX.size(),
                              0, 1, coefs.data());
  const auto* const cp = coefs.data();
  const auto kM0 = *(cp + 1);
  const auto kM1 =
      *(cp + 1) + ((1 - 0) * ((2 * *(cp + 2)) + (3 * *(cp + 3) * (1 - 0))));
  const auto kExpectedM0 = lob::spline::detail::Tangent<float>(
      kLinearX.data(), kLineFn.data(), kLinearX.size(), 0);
  const auto kExpectedM1 = lob::spline::detail::Tangent<float>(
      kLinearX.data(), kLineFn.data(), kLinearX.size(), 1);
  EXPECT_NEAR(kM0, kExpectedM0, kEpsLoose);
  EXPECT_NEAR(kM1, kExpectedM1, kEpsLoose);
}

TEST(SplinesBuildTest, ReturnsKnotsMinusOneForTwoKnotInput) {
  std::array<float, 2> machs{0, 1};
  std::array<float, 2> drags{1, 2};
  std::array<float, 2> knots{0, 1};
  std::array<float, 4> coefs{};
  const auto kCount =
      lob::spline::Build<float>(machs.data(), drags.data(), machs.size(),
                                knots.data(), knots.size(), coefs.data());
  EXPECT_EQ(kCount, knots.size() - 1);
}

TEST(SplinesBuildTest, ReturnsKnotsMinusOneForMultipleKnots) {
  constexpr auto kN = size_t{5};
  std::array<float, kN> knots{0, 1, 2, 3, 4};
  std::array<float, 4 * (kN - 1)> coefs{};
  const auto kCount = lob::spline::Build<float>(kLinearX.data(), kLineFn.data(),
                                                kLinearX.size(), knots.data(),
                                                kN, coefs.data());
  EXPECT_EQ(kCount, kN - 1);
}

TEST(SplinesBuildTest, BuiltCoefsReproduceSourceDataAtKnots) {
  constexpr auto kN = size_t{5};
  std::array<float, kN> knots{0, 1, 2, 3, 4};
  std::array<float, 4 * (kN - 1)> coefs{};
  lob::spline::Build<float>(kLinearX.data(), kLineFn.data(), kLinearX.size(),
                            knots.data(), kN, coefs.data());
  for (size_t i = 0; i + 1 < kN; ++i) {
    const auto* const seg = coefs.data() + (4 * i);
    const auto kT1 = *(knots.data() + i + 1) - *(knots.data() + i);
    EXPECT_NEAR(lob::spline::detail::PolyVal<float>(seg, 0),
                LineFn(*(knots.data() + i)), kEpsLoose)
        << "segment i=" << i << " left endpoint";
    EXPECT_NEAR(lob::spline::detail::PolyVal<float>(seg, kT1),
                LineFn(*(knots.data() + i + 1)), kEpsLoose)
        << "segment i=" << i << " right endpoint";
  }
}

TEST(SplinesBuildTest, BuiltCoefsOnRealKnotsReproduceDragAtKnotsInTable) {
  std::array<float, lob::spline::kCoefsSize> coefs{};
  lob::spline::Build<float>(
      lob::dragtable::kMachs.data(), lob::dragtable::kG1Drags.data(),
      lob::dragtable::kMachs.size(), lob::spline::kKnots.data(),
      lob::spline::kKnots.size(), coefs.data());
  for (size_t i = 0; i + 1 < lob::spline::kKnots.size(); ++i) {
    const auto kMach = *(lob::spline::kKnots.data() + i);
    const auto* const lo = std::lower_bound(
        lob::dragtable::kMachs.begin(), lob::dragtable::kMachs.end(), kMach);
    if (lo == lob::dragtable::kMachs.end() || !lob::AreEqual(*lo, kMach)) {
      continue;
    }
    const auto kIdx = static_cast<size_t>(lo - lob::dragtable::kMachs.begin());
    const auto kExpected = *(lob::dragtable::kG1Drags.data() + kIdx);
    const auto* const seg = coefs.data() + (4 * i);
    EXPECT_NEAR(lob::spline::detail::PolyVal<float>(seg, 0), kExpected,
                kEpsLoose);
  }
}

TEST(SplinesBuildTest, RuntimeBuildMatchesCompileTimeMakeCoefs) {
  std::array<float, lob::spline::kCoefsSize> rt{};
  lob::spline::Build<float>(
      lob::dragtable::kMachs.data(), lob::dragtable::kG1Drags.data(),
      lob::dragtable::kMachs.size(), lob::spline::kKnots.data(),
      lob::spline::kKnots.size(), rt.data());
  for (size_t i = 0; i < lob::spline::kCoefsSize; ++i) {
    EXPECT_FLOAT_EQ(*(rt.data() + i), *(lob::spline::kG1Coefs.data() + i))
        << "coef i=" << i;
  }
}

TEST(SplinesBuildTest, BuiltSplineMatchesTableAtKnotMachsExactly) {
  std::array<float, lob::spline::kCoefsSize> coefs{};
  lob::spline::Build<float>(
      lob::dragtable::kMachs.data(), lob::dragtable::kG1Drags.data(),
      lob::dragtable::kMachs.size(), lob::spline::kKnots.data(),
      lob::spline::kKnots.size(), coefs.data());
  lob::spline::Cursor<float, lob::spline::kKnotCount> cursor{
      lob::spline::kKnots.data(), coefs.data()};
  for (size_t i = 0; i < lob::spline::kKnots.size(); ++i) {
    const auto kMach = *(lob::spline::kKnots.data() + i);
    const auto* const lo = std::lower_bound(
        lob::dragtable::kMachs.begin(), lob::dragtable::kMachs.end(), kMach);
    if (lo == lob::dragtable::kMachs.end() || !lob::AreEqual(*lo, kMach)) {
      continue;
    }
    const auto kIdx = static_cast<size_t>(lo - lob::dragtable::kMachs.begin());
    const auto kExpected = *(lob::dragtable::kG1Drags.data() + kIdx);
    EXPECT_FLOAT_EQ(cursor.Eval(kMach), kExpected)
        << "knot mach #" << i << " = " << kMach;
  }
}

TEST(SplinesConstantsTest, KnotCountIs16AndSegmentCountIs15) {
  EXPECT_EQ(lob::spline::kKnotCount, 16U);
  EXPECT_EQ(lob::spline::kSegmentCount, 15U);
  EXPECT_EQ(lob::spline::kCoefsSize, 4U * 15U);
}

TEST(SplinesConstantsTest, KnotsAreSortedAndBracketRealRange) {
  const auto& k = lob::spline::kKnots;
  EXPECT_FLOAT_EQ(k.front(), 0);
  EXPECT_FLOAT_EQ(k.back(), 5);
  for (size_t i = 1; i < k.size(); ++i) {
    ASSERT_GT(*(k.data() + i), *(k.data() + i - 1)) << "knot i=" << i;
  }
}

TEST(SplinesConstantsTest, KnotsAreBoundedByDragTableRange) {
  for (size_t i = 0; i < lob::spline::kKnots.size(); ++i) {
    const auto kKnot = *(lob::spline::kKnots.data() + i);
    ASSERT_GE(kKnot, lob::dragtable::kMachs.front());
    ASSERT_LE(kKnot, lob::dragtable::kMachs.back());
  }
}

TEST(SplinesConstantsTest, AllPrecomputedCoefsAreFinite) {
  for (const auto* name : {&lob::spline::kG1Coefs, &lob::spline::kG2Coefs,
                           &lob::spline::kG5Coefs, &lob::spline::kG6Coefs,
                           &lob::spline::kG7Coefs, &lob::spline::kG8Coefs}) {
    for (size_t i = 0; i < name->size(); ++i) {
      ASSERT_TRUE(std::isfinite(*(name->data() + i)));
    }
  }
}

TEST(SplinesCursorTest, StartsAtIdxZeroAndEvaluatesAtStart) {
  lob::spline::Cursor<float, lob::spline::kKnotCount> cursor(
      lob::spline::kKnots.data(), lob::spline::kG1Coefs.data());
  const auto kResult = cursor.Eval(0);
  EXPECT_FLOAT_EQ(kResult, *(lob::dragtable::kG1Drags.data() + 0));
  EXPECT_EQ(cursor.GetSegment(), 0U);
}

TEST(SplinesCursorTest, MatchesTableAtKnotMachsExactly) {
  lob::spline::Cursor<float, lob::spline::kKnotCount> cursor(
      lob::spline::kKnots.data(), lob::spline::kG1Coefs.data());
  for (size_t i = 0; i < lob::spline::kKnots.size(); ++i) {
    const auto kMach = *(lob::spline::kKnots.data() + i);
    const auto* const lo = std::lower_bound(
        lob::dragtable::kMachs.begin(), lob::dragtable::kMachs.end(), kMach);
    if (lo == lob::dragtable::kMachs.end() || !lob::AreEqual(*lo, kMach)) {
      continue;
    }
    const auto kIdx = static_cast<size_t>(lo - lob::dragtable::kMachs.begin());
    const auto kExpected = *(lob::dragtable::kG1Drags.data() + kIdx);
    EXPECT_FLOAT_EQ(cursor.Eval(kMach), kExpected) << "knot i=" << i;
  }
}

TEST(SplinesCursorTest, ClampsBelowRangeToFirstDrag) {
  lob::spline::Cursor<float, lob::spline::kKnotCount> cursor(
      lob::spline::kKnots.data(), lob::spline::kG1Coefs.data());
  EXPECT_FLOAT_EQ(cursor.Eval(-1.0e3F), *(lob::dragtable::kG1Drags.data() + 0));
}

TEST(SplinesCursorTest, ClampsAboveRangeToLastDrag) {
  lob::spline::Cursor<float, lob::spline::kKnotCount> cursor(
      lob::spline::kKnots.data(), lob::spline::kG1Coefs.data());
  EXPECT_FLOAT_EQ(cursor.Eval(1.0e3F), lob::dragtable::kG1Drags.back());
}

TEST(SplinesCursorTest, ForwardBackwardMotionKeepsIdxInBounds) {
  lob::spline::Cursor<float, lob::spline::kKnotCount> cursor(
      lob::spline::kKnots.data(), lob::spline::kG1Coefs.data());
  cursor.Eval(5);  // NOLINT
  EXPECT_TRUE(cursor.GetSegment() < lob::spline::kKnotCount - 1);
  for (size_t i = 0; i < lob::spline::kKnotCount; ++i) {
    cursor.Eval(*(lob::spline::kKnots.data() + i));
    ASSERT_LE(cursor.GetSegment(), lob::spline::kKnotCount - 2);
  }
  cursor.Eval(0);
  EXPECT_EQ(cursor.GetSegment(), 0U);
}

TEST(SplinesCursorTest, SeeksContinuousUpwardFromStartToEndAndBack) {
  lob::spline::Cursor<float, lob::spline::kKnotCount> cursor(
      lob::spline::kKnots.data(), lob::spline::kG1Coefs.data());
  for (size_t i = 0; i < lob::spline::kKnots.size(); ++i) {
    cursor.Eval(*(lob::spline::kKnots.data() + i));
  }
  EXPECT_EQ(cursor.GetSegment(), lob::spline::kKnotCount - 2);
  for (size_t i = lob::spline::kKnots.size(); i-- > 0;) {
    cursor.Eval(*(lob::spline::kKnots.data() + i));
  }
  EXPECT_EQ(cursor.GetSegment(), 0);
}

TEST(SplinesCursorTest, SeekStaysAtLastSegmentStrictlyBelowLastKnot) {
  using lob::spline::kKnotCount;
  lob::spline::Cursor<float, kKnotCount> cursor(lob::spline::kKnots.data(),
                                                lob::spline::kG1Coefs.data());
  const float kMidLast = 0.5F * (lob::spline::kKnots[kKnotCount - 2] +
                                 lob::spline::kKnots[kKnotCount - 1]);
  cursor.Eval(kMidLast);
  EXPECT_EQ(cursor.GetSegment(), kKnotCount - 2);
  cursor.Eval(lob::spline::kKnots[kKnotCount - 1]);
  EXPECT_EQ(cursor.GetSegment(), kKnotCount - 2);
}

TEST(SplinesCursorTest, DerivIsFiniteAcrossTable) {
  lob::spline::Cursor<float, lob::spline::kKnotCount> cursor(
      lob::spline::kKnots.data(), lob::spline::kG1Coefs.data());
  for (size_t i = 0; i < lob::dragtable::kTableSize; ++i) {
    EXPECT_TRUE(
        std::isfinite(cursor.Deriv(*(lob::dragtable::kMachs.data() + i))));
  }
}

TEST(SplinesCursorTest, DerivOfPolyValMatchesPolyDerivAtMidSpan) {
  lob::spline::Cursor<float, lob::spline::kKnotCount> cursor(
      lob::spline::kKnots.data(), lob::spline::kG1Coefs.data());
  for (size_t i = 0; i + 1 < lob::spline::kKnotCount; ++i) {
    const auto kLo = *(lob::spline::kKnots.data() + i);
    const auto kHi = *(lob::spline::kKnots.data() + i + 1);
    const auto kMid = 0.5F * (kLo + kHi);
    const auto kT = kMid - kLo;
    const auto* const seg = lob::spline::kG1Coefs.data() + (4 * i);
    const auto kExpected = lob::spline::detail::PolyDeriv<float>(seg, kT);
    EXPECT_NEAR(cursor.Deriv(kMid), kExpected, kEps) << "span i=" << i;
  }
}

TEST(SplinesCursorTest, DerivClampsAtRangeEdges) {
  constexpr auto kTen = 10.0F;
  lob::spline::Cursor<float, lob::spline::kKnotCount> below(
      lob::spline::kKnots.data(), lob::spline::kG1Coefs.data());
  EXPECT_FLOAT_EQ(below.Deriv(-1), below.Deriv(lob::spline::kKnots.front()));

  lob::spline::Cursor<float, lob::spline::kKnotCount> above(
      lob::spline::kKnots.data(), lob::spline::kG1Coefs.data());
  EXPECT_FLOAT_EQ(above.Deriv(kTen), above.Deriv(lob::spline::kKnots.back()));
}

TEST(SplinesCursorTest, DerivMagnitudesReasonable) {
  constexpr auto kMaxDeriv = 100.0F;
  lob::spline::Cursor<float, lob::spline::kKnotCount> cursor(
      lob::spline::kKnots.data(), lob::spline::kG1Coefs.data());
  for (size_t i = 0; i < lob::dragtable::kTableSize; ++i) {
    EXPECT_TRUE(lob::Fabs(cursor.Deriv(*(lob::dragtable::kMachs.data() + i))) <
                kMaxDeriv);
  }
}

TEST(SplinesCursorTest, IdxZeroAfterResetByEvaluatingAtStart) {
  lob::spline::Cursor<float, lob::spline::kKnotCount> cursor(
      lob::spline::kKnots.data(), lob::spline::kG1Coefs.data());
  cursor.Eval(lob::spline::kKnots.back());  // NOLINT
  EXPECT_EQ(cursor.GetSegment(), lob::spline::kSegmentCount - 1);
  cursor.Eval(lob::spline::kKnots.front());
  EXPECT_EQ(cursor.GetSegment(), 0);
}

TEST(SplinesCursorTest, AllCoefsRobustAcrossMachs) {
  const std::array<const std::array<float, lob::spline::kCoefsSize>*, 6> kCoefs{
      &lob::spline::kG1Coefs, &lob::spline::kG2Coefs, &lob::spline::kG5Coefs,
      &lob::spline::kG6Coefs, &lob::spline::kG7Coefs, &lob::spline::kG8Coefs};
  for (const auto* coefs : kCoefs) {
    lob::spline::Cursor<float, lob::spline::kKnotCount> cursor{
        lob::spline::kKnots.data(), coefs->data()};
    for (size_t i = 0; i < lob::dragtable::kTableSize; ++i) {
      const auto kMach = *(lob::dragtable::kMachs.data() + i);
      const auto kResult = cursor.Eval(kMach);
      ASSERT_GT(kResult, 0);
      ASSERT_TRUE(std::isfinite(kResult));
    }
  }
}

TEST(SplinesSizesTest, CoefsSizeCoversExpectedArrayBytes) {
  EXPECT_EQ(lob::spline::kCoefsSize, 60U);
  EXPECT_EQ(lob::spline::kG1Coefs.size(), 60U);
  EXPECT_EQ(lob::spline::kG2Coefs.size(), 60U);
  EXPECT_EQ(lob::spline::kG5Coefs.size(), 60U);
  EXPECT_EQ(lob::spline::kG6Coefs.size(), 60U);
  EXPECT_EQ(lob::spline::kG7Coefs.size(), 60U);
  EXPECT_EQ(lob::spline::kG8Coefs.size(), 60U);
}

TEST(SplinesToArrayTest, RoundTripsDataPointForPoint) {
  constexpr auto kS1 = 1.5F;
  constexpr auto kS2 = -2.5F;
  constexpr auto kS3 = 3.5F;
  constexpr auto kS4 = 4.5F;
  std::array<float, 4> src{kS1, kS2, kS3, kS4};
  const auto kTarget =
      lob::spline::ToArray(src.data(), std::make_index_sequence<4>{});
  EXPECT_EQ(kTarget.size(), 4U);
  for (size_t i = 0; i < src.size(); ++i) {
    EXPECT_FLOAT_EQ(*(kTarget.data() + i), *(src.data() + i));
  }
}

TEST(SplinesRuntimeContextsTest, DetailFunctionsAreUsableAtRuntime) {
  constexpr auto kExpectedFabs = 3.5F;
  constexpr auto kExpectedIdx = size_t{1};
  const auto kSecant =
      lob::spline::detail::Secant<float>(kLinearX.data(), kLineFn.data(), 0);
  EXPECT_FLOAT_EQ(kSecant, 2);
  const auto kFabs = lob::Fabs(-kExpectedFabs);
  EXPECT_FLOAT_EQ(kFabs, kExpectedFabs);
  const auto kIdx = lob::spline::detail::FindInterval<float>(
      kLinearX.data(), kLinearX.size(), 1.5F);
  EXPECT_EQ(kIdx, kExpectedIdx);
  std::array<float, 4> c{1, 2, 0, 0};
  const auto kVal = lob::spline::detail::PolyVal<float>(c.data(), 0);
  EXPECT_FLOAT_EQ(kVal, 1);
  const auto kDeriv = lob::spline::detail::PolyDeriv<float>(c.data(), 0);
  EXPECT_FLOAT_EQ(kDeriv, 2);
  const auto kTan = lob::spline::detail::Tangent<float>(
      kLinearX.data(), kLineFn.data(), kLinearX.size(), 0);
  EXPECT_GT(kTan, 0);
}

TEST(SplinesRuntimeContextsTest, HermiteProducesExpectedEndpoints) {
  std::array<float, 4> cr{};
  // NOLINTNEXTLINE
  lob::spline::detail::Hermite<float>(0, 2, 1, 5, 2, 1, cr.data());
  EXPECT_FLOAT_EQ(lob::spline::detail::PolyVal<float>(cr.data(), 0), 1);
  EXPECT_FLOAT_EQ(lob::spline::detail::PolyVal<float>(cr.data(), 2), 5);
}

TEST(SplinesRuntimeContextsTest, BuildProducesMatchingEndpoints) {
  std::array<float, 2> machs{0, 1};
  std::array<float, 2> drags{1, 3};
  std::array<float, 2> knots{0, 1};
  std::array<float, 4> built{};
  lob::spline::Build<float>(machs.data(), drags.data(), machs.size(),
                            knots.data(), knots.size(), built.data());
  EXPECT_FLOAT_EQ(lob::spline::detail::PolyVal<float>(built.data(), 0), 1);
  EXPECT_FLOAT_EQ(lob::spline::detail::PolyVal<float>(built.data(), 1), 3);
}

TEST(SplinesRuntimeContextsTest, MakeCoefsMatchesCompileTimeGlobal) {
  const auto kRT = lob::spline::MakeCoefs(lob::dragtable::kG1Drags);
  EXPECT_EQ(kRT.size(), lob::spline::kCoefsSize);
  for (size_t i = 0; i < kRT.size(); ++i) {
    EXPECT_FLOAT_EQ(*(kRT.data() + i), *(lob::spline::kG1Coefs.data() + i));
  }
}

TEST(SplineOptimization, BaselineAccuracyBudget) {
  for (size_t i = 1; i < kTargetKnotSize; ++i) {
    ASSERT_GT(lob::spline::kKnots.at(i), lob::spline::kKnots.at(i - 1))
        << "kKnots not strictly increasing at index " << i;
  }

  // Span check with a small tolerance: knot machs are often printed from
  // rounded floats, and the evaluator clamps at the table edges anyway.

  constexpr float kSpanTolerance = 1e-3F;
  ASSERT_GE(lob::spline::kKnots.front(),
            lob::dragtable::kMachs.front() - kSpanTolerance);
  ASSERT_LE(lob::spline::kKnots.back(),
            lob::dragtable::kMachs.back() + kSpanTolerance);

  const TruthTables kTruth = BuildTruthTables();
  const float kBaselineMaxErr = BaselineError(kTruth);
  testing::Test::RecordProperty("BaselineMaxError",
                                std::to_string(kBaselineMaxErr));

  EXPECT_LT(kBaselineMaxErr, kMaxAllowedErr)
      << "Checked-in kKnots exceed the accuracy budget. Either the spline "
         "evaluator regressed, kKnots was corrupted, or the budget needs "
         "revisiting. Run SplineOptimization.DISABLED_OptimizeKnots to "
         "search for a better knot placement.";
}

// ---------------------------------------------------------------------------
// Tuning tool, not a CI gate. Run manually with:
//   --gtest_also_run_disabled_tests --gtest_filter='*OptimizeKnots*'
// Fails (intentionally, to be loud) only when it finds a meaningfully better
// configuration, and prints the drop-in replacement array.
// ---------------------------------------------------------------------------

TEST(SplineOptimization, DISABLED_OptimizeKnots) {
  const TruthTables kTruth = BuildTruthTables();

  // --- Baseline: error of the exact checked-in knots ----------------------
  const float kBaseLineErrorMax = BaselineError(kTruth);

  // --- Initialize SA state on the truth grid ------------------------------
  // Snap each knot to the nearest grid point, then repair collisions so the
  // index sequence stays strictly increasing — two nearby knots can
  // otherwise land on the same grid point and hand the spline evaluator
  // duplicate x-values.
  std::array<unsigned int, kTargetKnotSize> current_idxs{};

  for (size_t i = 0; i < kTargetKnotSize; ++i) {
    current_idxs.at(i) =
        FindNearestGridIndex(kTruth.machs, lob::spline::kKnots.at(i));
  }
  current_idxs.front() = 0;
  current_idxs.back() = static_cast<unsigned int>(kTruthSize - 1);
  for (size_t i = 1; i < kTargetKnotSize; ++i) {  // forward repair
    current_idxs.at(i) =
        std::max(current_idxs.at(i), current_idxs.at(i - 1) + 1);
  }

  for (size_t i = kTargetKnotSize - 1; i-- > 0;) {  // backward repair
    current_idxs.at(i) =
        std::min(current_idxs.at(i), current_idxs.at(i + 1) - 1);
  }

  for (size_t i = 1; i < kTargetKnotSize; ++i) {
    ASSERT_GT(current_idxs.at(i), current_idxs.at(i - 1))
        << "Could not place strictly-increasing knots on the truth grid";
  }

  // Rebuild the sample arrays so they EXACTLY match the given indices.
  // Every code path that changes current_idxs must keep these in sync;
  // a desync here silently poisons best_err (measured on one spline,
  // attributed to another).

  std::array<float, kTargetKnotSize> sample_machs{};
  std::array<float, kTargetKnotSize> sample_g1{};
  std::array<float, kTargetKnotSize> sample_g7{};

  auto load_samples = [&kTruth](
                          const std::array<unsigned int, kTargetKnotSize>& idxs,
                          std::array<float, kTargetKnotSize>& machs,
                          std::array<float, kTargetKnotSize>& g1,
                          std::array<float, kTargetKnotSize>& g7) {
    for (size_t i = 0; i < kTargetKnotSize; ++i) {
      machs.at(i) = kTruth.machs.at(idxs.at(i));
      g1.at(i) = kTruth.g1.at(idxs.at(i));
      g7.at(i) = kTruth.g7.at(idxs.at(i));
    }
  };

  std::array<float, kTruthSize> current_errors{};
  auto best_idxs = current_idxs;
  load_samples(current_idxs, sample_machs, sample_g1, sample_g7);

  float best_err =
      EvalMaxError(kTruth, sample_machs, sample_g1, sample_g7, &current_errors);

  // --- Annealing schedule --------------------------------------------------
  // T0 scales with the problem: a quarter of the starting error means early
  // proposals that worsen error "on the order of the error itself" are
  // often accepted, and by T ~ T0/100 we are effectively hill climbing.
  // Step size decays on its OWN geometric schedule (decoupled from T) so
  // the walk can still make long jumps mid-run while acceptance tightens.

  const float kStartingTemp = std::max(0.25F * best_err, 1e-5F);
  const float kTMin = kStartingTemp * 1e-3F;
  const float kCoolingRate = 0.9975F;  // ~2760 temperature steps
  const int kIterationsPerTemp = 20;   // ~55k proposals per restart
  const int kInitialStep = 100;        // in truth-grid indices, decays to 1
  const int kNumRestarts = 3;          // reheat from best-so-far each time
  const float kLogTempRange = std::log(kStartingTemp / kTMin);

  std::uniform_real_distribution<float> prob_dist(0.0F, 1.0F);
  std::uniform_int_distribution<size_t> knot_dist(1, kTargetKnotSize - 2);

  for (int restart = 0; restart < kNumRestarts; ++restart) {
    // NOLINTNEXTLINE(cert-msc51-cpp, cert-msc32-c)
    std::mt19937 rng(static_cast<std::mt19937::result_type>(
        42 + (1000 * restart)));  // NOLINT

    // Start (or reheat) from the best configuration found so far.
    // FIX: rebuild sample arrays AND per-point errors together with the
    // indices — restoring indices alone leaves the spline state stale,
    // which is exactly the desync that produced unreproducible results.

    current_idxs = best_idxs;

    load_samples(current_idxs, sample_machs, sample_g1, sample_g7);
    float current_max_err = EvalMaxError(kTruth, sample_machs, sample_g1,
                                         sample_g7, &current_errors);

    for (float temperature = kStartingTemp; temperature > kTMin;
         // NOLINTNEXTLINE(cert-flp30-c)
         temperature *= kCoolingRate) {
      // Fraction of the schedule elapsed, 0 -> 1, drives step-size decay.
      const float kFrac = std::log(kStartingTemp / temperature) / kLogTempRange;
      const int kMaxStep = std::max(
          1, static_cast<int>(std::lround(
                 kInitialStep *
                 std::pow(1.0F / static_cast<float>(kInitialStep), kFrac))));

      std::uniform_int_distribution<int> step_dist(-kMaxStep, kMaxStep);
      for (int i = 0; i < kIterationsPerTemp; ++i) {
        const size_t kK = knot_dist(rng);
        const int kOldIdx = static_cast<int>(current_idxs.at(kK));
        const int kMinAllowed = static_cast<int>(current_idxs.at(kK - 1)) + 1;
        const int kMaxAllowed = static_cast<int>(current_idxs.at(kK + 1)) - 1;

        if (kMinAllowed > kMaxAllowed) {
          continue;
        }

        int new_idx_signed = kOldIdx + step_dist(rng);
        new_idx_signed =
            std::max(kMinAllowed, std::min(new_idx_signed, kMaxAllowed));

        const auto kNewIdx = static_cast<unsigned int>(new_idx_signed);
        if (kNewIdx == static_cast<unsigned int>(kOldIdx)) {
          continue;
        }

        current_idxs.at(kK) = kNewIdx;
        sample_machs.at(kK) = kTruth.machs.at(kNewIdx);
        sample_g1.at(kK) = kTruth.g1.at(kNewIdx);
        sample_g7.at(kK) = kTruth.g7.at(kNewIdx);

        // Guard the invariant that makes windowed evaluation valid: the
        // sample x-array must stay sorted. An unsorted array here means a
        // state-sync bug upstream, and the evaluator's output is garbage.

        ASSERT_TRUE(std::is_sorted(sample_machs.begin(), sample_machs.end()))
            << "sample_machs desynced from current_idxs";

        // Windowed re-evaluation: moving knot k changes secant slopes of
        // segments k-1..k, hence derivatives at knots k-1..k+1, hence
        // spline values only on segments k-2..k+1. Truth points outside
        // [knot k-2, knot k+2] are untouched. (The earlier "errors leak
        // outside the window" symptom was this same desync bug, not a
        // flaw in the windowed update.)

        const size_t kLeftKnot = (kK >= 2) ? kK - 2 : 0;
        const size_t kRightKnot = std::min(kTargetKnotSize - 1, kK + 2);
        const size_t kStartTruthIdx = current_idxs.at(kLeftKnot);
        const size_t kEndTruthIdx = current_idxs.at(kRightKnot);

        auto candidate_errors = current_errors;

        for (size_t j = kStartTruthIdx; j <= kEndTruthIdx; ++j) {
          const float kEvalG1 = lob::spline::detail::EvalWithDeriv(
                                    sample_machs.data(), sample_g1.data(),
                                    kTargetKnotSize, kTruth.machs.at(j))
                                    .y;

          const float kEvalG7 = lob::spline::detail::EvalWithDeriv(
                                    sample_machs.data(), sample_g7.data(),
                                    kTargetKnotSize, kTruth.machs.at(j))
                                    .y;

          candidate_errors.at(j) =
              std::max(std::abs(kEvalG1 - kTruth.g1.at(j)),
                       std::abs(kEvalG7 - kTruth.g7.at(j)));
        }

        const float kCandidateMaxErr =
            *std::max_element(candidate_errors.begin(), candidate_errors.end());

        const float kDelta = kCandidateMaxErr - current_max_err;

        if (kDelta < 0.0F || std::exp(-kDelta / temperature) > prob_dist(rng)) {
          current_max_err = kCandidateMaxErr;
          current_errors = candidate_errors;

          if (current_max_err < best_err) {
            best_err = current_max_err;
            best_idxs = current_idxs;
          }

        } else {
          current_idxs.at(kK) = static_cast<unsigned int>(kOldIdx);
          sample_machs.at(kK) = kTruth.machs.at(static_cast<size_t>(kOldIdx));
          sample_g1.at(kK) = kTruth.g1.at(static_cast<size_t>(kOldIdx));
          sample_g7.at(kK) = kTruth.g7.at(static_cast<size_t>(kOldIdx));
        }
      }
    }
  }

  // --- Verify before reporting ---------------------------------------------
  // Recompute the error of best_idxs from scratch. If this does not match
  // the running best_err, optimizer state desynced somewhere and the printed
  // knots would NOT reproduce the reported number after paste-back.

  load_samples(best_idxs, sample_machs, sample_g1, sample_g7);

  const float kVerifiedErr =
      EvalMaxError(kTruth, sample_machs, sample_g1, sample_g7);

  ASSERT_NEAR(kVerifiedErr, best_err, 1e-6F)
      << "Optimizer state desync: recorded best_err does not match the "
         "true error of best_idxs. Do NOT trust this run's output.";

  best_err = kVerifiedErr;  // report the independently verified number

  // --- Report ---------------------------------------------------------------
  testing::Test::RecordProperty("BaselineMaxError",
                                std::to_string(kBaseLineErrorMax));

  testing::Test::RecordProperty("OptimizedMaxError", std::to_string(best_err));

  // NOTE: the emitted machs come from the truth grid, and their drag values
  // are assumed to be spline evaluations of the full drag table at those
  // machs. If production stores measured values at knots instead, re-derive
  // the sample values accordingly before committing.

  std::ostringstream oss;
  oss << "constexpr std::array<float, kKnotCount> kKnots = {\n";
  constexpr int kPrecision = std::numeric_limits<float>::max_digits10;
  oss << std::fixed << std::setprecision(kPrecision);

  for (size_t i = 0; i < kTargetKnotSize; ++i) {
    oss << "    " << kTruth.machs.at(best_idxs.at(i)) << "F"
        << (i < kTargetKnotSize - 1 ? ",\n" : "\n");
  }

  oss << "};";
  constexpr float kMinMeaningfulImprovement = 1e-5F;
  if (best_err < (kBaseLineErrorMax - kMinMeaningfulImprovement)) {
    FAIL() << "\n=============================================================="
              "==========\n"
           << " SUCCESS: Found a significantly better knot configuration!\n"
           << "================================================================"
              "========\n"
           << " Supplied kKnots Max Error:  " << kBaseLineErrorMax << "\n"
           << " Optimized kKnots Max Error: " << best_err << " (verified)\n"
           << " Net Improvement:            " << (kBaseLineErrorMax - best_err)
           << "\n\n"
           << " Replace your current kKnots array definition with the "
              "following:\n\n"
           << oss.str() << "\n"
           << "================================================================"
              "========\n";
  }
}

#if 1  // NOLINT
TEST(SplinesDesmosOutputTest, GeneratesMachDragTableForG1AndG7) {
  constexpr size_t kPoints = 2000;
  constexpr float kMachMin = 0.0F;
  constexpr float kMachMax = 5.0F;
  constexpr float kStep =
      (kMachMax - kMachMin) / static_cast<float>(kPoints - 1);

  lob::spline::Cursor<float, lob::spline::kKnotCount> g1_cursor{
      lob::spline::kKnots.data(), lob::spline::kG1Coefs.data()};
  lob::spline::Cursor<float, lob::spline::kKnotCount> g7_cursor{
      lob::spline::kKnots.data(), lob::spline::kG7Coefs.data()};

  std::cout << "=== G1 Drag Table ===\n";
  std::cout << "Mach,G1_Spline,G1_RuntimePchip,G1_LobLerp\n";
  for (size_t i = 0; i < kPoints; ++i) {
    const float kMach = kMachMin + (static_cast<float>(i) * kStep);
    const auto kG1Runtime =
        lob::spline::detail::EvalWithDeriv<float>(
            lob::dragtable::kMachs.data(), lob::dragtable::kG1Drags.data(),
            lob::dragtable::kTableSize, kMach)
            .y;
    const auto kG1Lerp = lob::dragtable::LobLerp(
        lob::dragtable::kMachs.data(), lob::dragtable::kG1Drags.data(),
        lob::dragtable::kTableSize, static_cast<double>(kMach));
    const float kG1Spline = g1_cursor.Eval(kMach);
    std::cout << kMach << ',' << kG1Spline << ',' << kG1Runtime << ','
              << kG1Lerp << '\n';
  }

  std::cout << "\n=== G7 Drag Table ===\n";
  std::cout << "Mach,G7_Spline,G7_RuntimePchip,G7_LobLerp\n";
  for (size_t i = 0; i < kPoints; ++i) {
    const float kMach = kMachMin + (static_cast<float>(i) * kStep);
    const auto kG7Runtime =
        lob::spline::detail::EvalWithDeriv<float>(
            lob::dragtable::kMachs.data(), lob::dragtable::kG7Drags.data(),
            lob::dragtable::kTableSize, kMach)
            .y;
    const auto kG7Lerp = lob::dragtable::LobLerp(
        lob::dragtable::kMachs.data(), lob::dragtable::kG7Drags.data(),
        lob::dragtable::kTableSize, static_cast<double>(kMach));
    const float kG7Spline = g7_cursor.Eval(kMach);
    std::cout << kMach << ',' << kG7Spline << ',' << kG7Runtime << ','
              << kG7Lerp << '\n';
  }
  SUCCEED()
      << "Desmos CSV output complete. Copy each table separately to Desmos.";
}
#endif

}  // namespace tests

// This file is part of lob.
//
// lob is free software: you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version.
//
// lob is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
// A PARTICULAR PURPOSE. See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along with
// lob. If not, see <https://www.gnu.org/licenses/>.
