// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include "splines.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <iterator>
#include <string>
#include <utility>

#include "helpers.hpp"
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

constexpr size_t kTruthSize = 5000;
constexpr size_t kTargetKnotSize = lob::spline::kKnotCount;
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

// Max abs error (worst of G1/G7) of a spline over the whole truth grid.
float EvalMaxError(const TruthTables& t,
                   const std::array<float, kTargetKnotSize>& sample_machs,
                   const std::array<float, kTargetKnotSize>& sample_g1,
                   const std::array<float, kTargetKnotSize>& sample_g7) {
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
  const auto* const pcp = coefs.data();
  const auto kM0 = *(pcp + 1);
  const auto kM1 =
      *(pcp + 1) + ((1 - 0) * ((2 * *(pcp + 2)) + (3 * *(pcp + 3) * (1 - 0))));
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
    const auto* const pseg = coefs.data() + (4 * i);
    const auto kT1 = *(knots.data() + i + 1) - *(knots.data() + i);
    EXPECT_NEAR(lob::spline::detail::PolyVal<float>(pseg, 0),
                LineFn(*(knots.data() + i)), kEpsLoose)
        << "segment i=" << i << " left endpoint";
    EXPECT_NEAR(lob::spline::detail::PolyVal<float>(pseg, kT1),
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
    // NOLINTNEXTLINE(readability-qualified-auto, llvm-qualified-auto)
    const auto lo = std::lower_bound(lob::dragtable::kMachs.begin(),
                                     lob::dragtable::kMachs.end(), kMach);
    if (lo == lob::dragtable::kMachs.end() || !lob::AreEqual(*lo, kMach)) {
      continue;
    }
    const auto kIdx =
        static_cast<size_t>(std::distance(lob::dragtable::kMachs.begin(), lo));
    const auto kExpected = *(lob::dragtable::kG1Drags.data() + kIdx);
    const auto* const pseg = coefs.data() + (4 * i);
    EXPECT_NEAR(lob::spline::detail::PolyVal<float>(pseg, 0), kExpected,
                kEpsLoose);
  }
}

TEST(SplinesBuildTest, RuntimeBuildMatchesCompileTimeMakeCoefs) {
  constexpr float kRelEps = 5.0e-4F;
  std::array<float, lob::spline::kCoefsSize> rt{};
  lob::spline::Build<float>(
      lob::dragtable::kMachs.data(), lob::dragtable::kG1Drags.data(),
      lob::dragtable::kMachs.size(), lob::spline::kKnots.data(),
      lob::spline::kKnots.size(), rt.data());
  for (size_t i = 0; i < lob::spline::kCoefsSize; ++i) {
    const float kGot = *(rt.data() + i);
    const float kWant = *(lob::spline::kG1Coefs.data() + i);
    const float kTol = std::max(std::fabs(kGot), std::fabs(kWant)) * kRelEps;
    EXPECT_NEAR(kGot, kWant, std::max(kTol, kEpsLoose)) << "coef i=" << i;
  }
}

TEST(SplinesBuildTest, MakeCoefsReproducesCustomTableAtKnots) {
  const float kSlope = 0.1F;
  const float kIntercept = 0.5F;
  std::array<float, lob::dragtable::kMachs.size()> drags{};
  for (size_t i = 0; i < lob::dragtable::kMachs.size(); ++i) {
    const float kMach = *(lob::dragtable::kMachs.data() + i);
    *(drags.data() + i) = kIntercept + (kSlope * kMach);
  }
  const auto kCoefs = lob::spline::MakeCoefs(drags);
  lob::spline::CurveView curve(lob::spline::kKnots, kCoefs);
  for (size_t i = 0; i < lob::spline::kKnotCount; ++i) {
    const float kMach = *(lob::spline::kKnots.data() + i);
    const float kWant = kIntercept + (kSlope * kMach);
    EXPECT_NEAR(curve.Eval(kMach), kWant, kEpsLoose)
        << "knot mach #" << i << " = " << kMach;
  }
}

TEST(SplinesBuildTest, MakeCoefsRuntimeWithCustomDragTable) {
  const std::array<float, 2> kDrags = {0.5F, 0.3F};
  const auto kCoefs = lob::spline::MakeCoefs<2>(kDrags);
  EXPECT_EQ(kCoefs.size(), lob::spline::kCoefsSize);
  for (const auto kCoef : kCoefs) {
    EXPECT_TRUE(std::isfinite(kCoef));
  }
}

TEST(SplinesBuildTest, BuiltSplineMatchesTableAtKnotMachsExactly) {
  std::array<float, lob::spline::kCoefsSize> coefs{};
  lob::spline::Build<float>(
      lob::dragtable::kMachs.data(), lob::dragtable::kG1Drags.data(),
      lob::dragtable::kMachs.size(), lob::spline::kKnots.data(),
      lob::spline::kKnots.size(), coefs.data());
  lob::spline::CurveView curve(lob::spline::kKnots, coefs);
  for (size_t i = 0; i < lob::spline::kKnots.size(); ++i) {
    const auto kMach = *(lob::spline::kKnots.data() + i);
    // NOLINTNEXTLINE(readability-qualified-auto, llvm-qualified-auto)
    const auto lo = std::lower_bound(lob::dragtable::kMachs.begin(),
                                     lob::dragtable::kMachs.end(), kMach);
    if (lo == lob::dragtable::kMachs.end() || !lob::AreEqual(*lo, kMach)) {
      continue;
    }
    const auto kIdx =
        static_cast<size_t>(std::distance(lob::dragtable::kMachs.begin(), lo));
    const auto kExpected = *(lob::dragtable::kG1Drags.data() + kIdx);
    EXPECT_FLOAT_EQ(curve.Eval(kMach), kExpected)
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

TEST(SplinesCurveViewTest, StartsAtIdxZeroAndEvaluatesAtStart) {
  lob::spline::CurveView curve(lob::spline::kKnots, lob::spline::kG1Coefs);
  const auto kResult = curve.Eval(0);
  EXPECT_FLOAT_EQ(kResult, *(lob::dragtable::kG1Drags.data() + 0));
  EXPECT_EQ(curve.GetSegment(), 0U);
}

TEST(SplinesCurveViewTest, MatchesTableAtKnotMachsExactly) {
  lob::spline::CurveView curve(lob::spline::kKnots, lob::spline::kG1Coefs);
  for (size_t i = 0; i < lob::spline::kKnots.size(); ++i) {
    const auto kMach = *(lob::spline::kKnots.data() + i);
    // NOLINTNEXTLINE(readability-qualified-auto, llvm-qualified-auto)
    const auto lo = std::lower_bound(lob::dragtable::kMachs.begin(),
                                     lob::dragtable::kMachs.end(), kMach);
    if (lo == lob::dragtable::kMachs.end() || !lob::AreEqual(*lo, kMach)) {
      continue;
    }
    const auto kIdx =
        static_cast<size_t>(std::distance(lob::dragtable::kMachs.begin(), lo));
    const auto kExpected = *(lob::dragtable::kG1Drags.data() + kIdx);
    EXPECT_FLOAT_EQ(curve.Eval(kMach), kExpected) << "knot i=" << i;
  }
}

TEST(SplinesCurveViewTest, ClampsBelowRangeToFirstDrag) {
  lob::spline::CurveView curve(lob::spline::kKnots, lob::spline::kG1Coefs);
  EXPECT_FLOAT_EQ(curve.Eval(-1.0e3F), *(lob::dragtable::kG1Drags.data() + 0));
}

TEST(SplinesCurveViewTest, ClampsAboveRangeToLastDrag) {
  lob::spline::CurveView curve(lob::spline::kKnots, lob::spline::kG1Coefs);
  EXPECT_FLOAT_EQ(curve.Eval(1.0e3F), lob::dragtable::kG1Drags.back());
}

TEST(SplinesCurveViewTest, ForwardBackwardMotionKeepsIdxInBounds) {
  lob::spline::CurveView curve(lob::spline::kKnots, lob::spline::kG1Coefs);
  curve.Eval(5);  // NOLINT
  EXPECT_TRUE(curve.GetSegment() < lob::spline::kKnotCount - 1);
  for (size_t i = 0; i < lob::spline::kKnotCount; ++i) {
    curve.Eval(*(lob::spline::kKnots.data() + i));
    ASSERT_LE(curve.GetSegment(), lob::spline::kKnotCount - 2);
  }
  curve.Eval(0);
  EXPECT_EQ(curve.GetSegment(), 0U);
}

TEST(SplinesCurveViewTest, SeeksContinuousUpwardFromStartToEndAndBack) {
  lob::spline::CurveView curve(lob::spline::kKnots, lob::spline::kG1Coefs);
  for (size_t i = 0; i < lob::spline::kKnots.size(); ++i) {
    curve.Eval(*(lob::spline::kKnots.data() + i));
  }
  EXPECT_EQ(curve.GetSegment(), lob::spline::kKnotCount - 2);
  for (size_t i = lob::spline::kKnots.size(); i-- > 0;) {
    curve.Eval(*(lob::spline::kKnots.data() + i));
  }
  EXPECT_EQ(curve.GetSegment(), 0);
}

TEST(SplinesCurveViewTest, SeekStaysAtLastSegmentStrictlyBelowLastKnot) {
  using lob::spline::kKnotCount;
  lob::spline::CurveView curve(lob::spline::kKnots, lob::spline::kG1Coefs);
  const float kMidLast = 0.5F * (lob::spline::kKnots[kKnotCount - 2] +
                                 lob::spline::kKnots[kKnotCount - 1]);
  curve.Eval(kMidLast);
  EXPECT_EQ(curve.GetSegment(), kKnotCount - 2);
  curve.Eval(lob::spline::kKnots[kKnotCount - 1]);
  EXPECT_EQ(curve.GetSegment(), kKnotCount - 2);
}

TEST(SplinesCurveViewTest, DerivIsFiniteAcrossTable) {
  lob::spline::CurveView curve(lob::spline::kKnots, lob::spline::kG1Coefs);
  for (size_t i = 0; i < lob::dragtable::kTableSize; ++i) {
    EXPECT_TRUE(
        std::isfinite(curve.Deriv(*(lob::dragtable::kMachs.data() + i))));
  }
}

TEST(SplinesCurveViewTest, DerivOfPolyValMatchesPolyDerivAtMidSpan) {
  lob::spline::CurveView curve(lob::spline::kKnots, lob::spline::kG1Coefs);
  for (size_t i = 0; i + 1 < lob::spline::kKnotCount; ++i) {
    const auto kLo = *(lob::spline::kKnots.data() + i);
    const auto kHi = *(lob::spline::kKnots.data() + i + 1);
    const auto kMid = 0.5F * (kLo + kHi);
    const auto kT = kMid - kLo;
    const auto* const pseg = lob::spline::kG1Coefs.data() + (4 * i);
    const auto kExpected = lob::spline::detail::PolyDeriv<float>(pseg, kT);
    EXPECT_NEAR(curve.Deriv(kMid), kExpected, kEps) << "span i=" << i;
  }
}

TEST(SplinesCurveViewTest, DerivClampsAtRangeEdges) {
  constexpr auto kTen = 10.0F;
  lob::spline::CurveView below(lob::spline::kKnots, lob::spline::kG1Coefs);
  EXPECT_FLOAT_EQ(below.Deriv(-1), below.Deriv(lob::spline::kKnots.front()));

  lob::spline::CurveView above(lob::spline::kKnots, lob::spline::kG1Coefs);
  EXPECT_FLOAT_EQ(above.Deriv(kTen), above.Deriv(lob::spline::kKnots.back()));
}

TEST(SplinesCurveViewTest, DerivMagnitudesReasonable) {
  constexpr auto kMaxDeriv = 100.0F;
  lob::spline::CurveView curve(lob::spline::kKnots, lob::spline::kG1Coefs);
  for (size_t i = 0; i < lob::dragtable::kTableSize; ++i) {
    EXPECT_TRUE(lob::Fabs(curve.Deriv(*(lob::dragtable::kMachs.data() + i))) <
                kMaxDeriv);
  }
}

TEST(SplinesCurveViewTest, IdxZeroAfterResetByEvaluatingAtStart) {
  lob::spline::CurveView curve(lob::spline::kKnots, lob::spline::kG1Coefs);
  curve.Eval(lob::spline::kKnots.back());  // NOLINT
  EXPECT_EQ(curve.GetSegment(), lob::spline::kSegmentCount - 1);
  curve.Eval(lob::spline::kKnots.front());
  EXPECT_EQ(curve.GetSegment(), 0);
}

TEST(SplinesCurveViewTest, AllCoefsRobustAcrossMachs) {
  const std::array<const std::array<float, lob::spline::kCoefsSize>*, 6> kCoefs{
      &lob::spline::kG1Coefs, &lob::spline::kG2Coefs, &lob::spline::kG5Coefs,
      &lob::spline::kG6Coefs, &lob::spline::kG7Coefs, &lob::spline::kG8Coefs};
  for (const auto* coefs : kCoefs) {
    lob::spline::CurveView curve{lob::spline::kKnots, *coefs};
    for (size_t i = 0; i < lob::dragtable::kTableSize; ++i) {
      const auto kMach = *(lob::dragtable::kMachs.data() + i);
      const auto kResult = curve.Eval(kMach);
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
  const float kExpectedFabs = 3.5F;
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

TEST(SplinesFormFactorTest, ConstantBcProducesFlatCurve) {
  constexpr float kSd = 0.250F;
  constexpr float kBc = 0.300F;
  const std::array<float, 3> kMachs{0.5F, 1.0F, 2.0F};
  const std::array<float, 3> kBcs{kBc, kBc, kBc};
  std::array<float, lob::spline::kCoefsSize> coefs{};
  lob::spline::MakeFormFactorCoefs(kSd, kMachs.data(), kBcs.data(),
                                   kMachs.size(), coefs.data());
  lob::spline::CurveView curve(lob::spline::kKnots, coefs);
  for (size_t i = 0; i < lob::spline::kKnotCount; ++i) {
    EXPECT_NEAR(curve.Eval(lob::spline::kKnots.at(i)), kSd / kBc, kEpsLoose)
        << "knot i=" << i;
  }
}

TEST(SplinesFormFactorTest, ExtrapolatesFlatOutsideBandRange) {
  constexpr float kSd = 0.250F;
  const std::array<float, 2> kMachs{2.5F, 3.5F};
  const std::array<float, 2> kBcs{0.300F, 0.200F};
  std::array<float, lob::spline::kCoefsSize> coefs{};
  lob::spline::MakeFormFactorCoefs(kSd, kMachs.data(), kBcs.data(),
                                   kMachs.size(), coefs.data());
  lob::spline::CurveView curve(lob::spline::kKnots, coefs);
  EXPECT_NEAR(curve.Eval(0.0F), kSd / kBcs[0], kEpsLoose);
  EXPECT_NEAR(curve.Eval(5.0F), kSd / kBcs[1], kEpsLoose);
}

TEST(SplinesFormFactorTest, RuntimeMatchesCompileTimeVariant) {
  constexpr size_t kN = 4;
  constexpr float kSd = 0.250F;
  const std::array<float, kN> kMachs{0.4F, 0.8F, 1.2F, 3.0F};
  const std::array<float, kN> kBcs{0.35F, 0.30F, 0.25F, 0.20F};
  const auto kExpected =
      lob::spline::MakeFormFactorCoefs<float, kN>(kSd, kMachs, kBcs);
  std::array<float, lob::spline::kCoefsSize> got{};
  lob::spline::MakeFormFactorCoefs(kSd, kMachs.data(), kBcs.data(), kN,
                                   got.data());
  for (size_t i = 0; i < got.size(); ++i) {
    EXPECT_FLOAT_EQ(got.at(i), kExpected.at(i)) << "coef i=" << i;
  }
}

TEST(SplineOptimization, BaselineAccuracyBudget) {
  for (size_t i = 1; i < kTargetKnotSize; ++i) {
    ASSERT_GT(lob::spline::kKnots.at(i), lob::spline::kKnots.at(i - 1))
        << "kKnots not strictly increasing at index " << i;
  }

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
         "revisiting.";
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
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
// A PARTICULAR PURPOSE. See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along with
// lob. If not, see <https://www.gnu.org/licenses/>.
