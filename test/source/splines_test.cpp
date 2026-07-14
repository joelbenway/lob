// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include "splines.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <cstddef>

#include "tables.hpp"

namespace {

constexpr auto kEps = 1.0e-5F;
constexpr auto kEpsLoose = 1.0e-3F;

constexpr std::array<float, 5> kLinearX{0.0F, 1.0F, 2.0F, 3.0F, 4.0F};
constexpr std::array<float, 5> kLinearY{1.0F, 3.0F, 5.0F, 7.0F, 9.0F};

constexpr std::array<float, 4> kQuadX{0.0F, 1.0F, 2.0F, 3.0F};
constexpr std::array<float, 4> kQuadY{0.0F, 1.0F, 4.0F, 9.0F};

constexpr std::array<float, 5> kFcX{0.0F, 1.0F, 2.0F, 3.0F, 4.0F};
constexpr std::array<float, 5> kFcY{0.0F, 1.0F, 0.0F, -1.0F, 0.0F};

constexpr std::array<float, 5> kMonotonicX{0.0F, 0.5F, 1.0F, 2.0F, 5.0F};
constexpr std::array<float, 5> kMonotonicY{0.0F, 1.0F, 2.0F, 4.0F, 10.0F};

constexpr auto SecantRef(const float* x, const float* y, size_t i) {
  return (*(y + i + 1) - *(y + i)) / (*(x + i + 1) - *(x + i));
}

constexpr auto LinearY(float x) { return (2 * x) + 1.0F; }

}  // namespace

namespace tests {

TEST(SplinesDetailSecantTest, ReturnsSlopeBetweenAdjacentPoints) {
  EXPECT_FLOAT_EQ(
      lob::spline::detail::Secant<float>(kLinearX.data(), kLinearY.data(), 0),
      2);
  EXPECT_FLOAT_EQ(
      lob::spline::detail::Secant<float>(kLinearX.data(), kLinearY.data(), 3),
      2);
  EXPECT_FLOAT_EQ(
      lob::spline::detail::Secant<float>(kQuadX.data(), kQuadY.data(), 0), 1);
  EXPECT_FLOAT_EQ(
      lob::spline::detail::Secant<float>(kQuadX.data(), kQuadY.data(), 2), 5);
  EXPECT_FLOAT_EQ(lob::spline::detail::Secant<float>(
                      kQuadX.data(), kQuadY.data(), kQuadX.size() - 2),
                  SecantRef(kQuadX.data(), kQuadY.data(), kQuadX.size() - 2));
}

TEST(SplinesDetailEndTangentTest, ReturnsWeightedSlopeForMonotonic) {
  constexpr auto kH0 = 1;
  constexpr auto kH1 = 1;
  constexpr auto kD0 = 2;
  constexpr auto kD1 = 2;
  const auto kExpected = (((2 * kH0) + kH1) * kD0 - kH0 * kD1) / (kH0 + kH1);
  EXPECT_FLOAT_EQ(lob::spline::detail::EndTangent<float>(kH0, kH1, kD0, kD1),
                  kExpected);
}

TEST(SplinesDetailEndTangentTest, ReturnsZeroWhenMTimesD0IsNonPositive) {
  constexpr auto kH0 = 1;
  constexpr auto kH1 = 1;
  constexpr auto kD0 = 1;
  constexpr auto kD1 = 5;
  const auto kM = (((2 * kH0) + kH1) * kD0 - kH0 * kD1) / (kH0 + kH1);
  ASSERT_TRUE(kM * kD0 <= 0);
  EXPECT_FLOAT_EQ(lob::spline::detail::EndTangent<float>(kH0, kH1, kD0, kD1),
                  0);
}

TEST(SplinesDetailEndTangentTest, ClampsToThreeD0WhenSignChangeAndSteep) {
  constexpr auto kH0 = 0.01F;
  constexpr auto kH1 = 1;
  constexpr auto kD0 = 1;
  constexpr auto kD1 = -1000.0F;
  const auto kM = (((2 * kH0) + kH1) * kD0 - kH0 * kD1) / (kH0 + kH1);
  ASSERT_TRUE(kM * kD0 > 0);
  ASSERT_TRUE(kD0 * kD1 < 0);
  ASSERT_TRUE(lob::Fabs(kM) > (3 * lob::Fabs(kD0)));
  EXPECT_FLOAT_EQ(lob::spline::detail::EndTangent<float>(kH0, kH1, kD0, kD1),
                  (3 * kD0));
}

TEST(SplinesDetailEndTangentTest, ReturnsMWhenConditionsRelaxed) {
  constexpr auto kH0 = 1;
  constexpr auto kH1 = 1;
  constexpr auto kD0 = 1;
  constexpr auto kD1 = 0.5;
  const auto kM = (((2 * kH0) + kH1) * kD0 - kH0 * kD1) / (kH0 + kH1);
  ASSERT_TRUE(kM * kD0 > 0);
  ASSERT_FALSE((kD0 * kD1 < 0) && (lob::Fabs(kM) > (3 * lob::Fabs(kD0))));
  EXPECT_FLOAT_EQ(lob::spline::detail::EndTangent<float>(kH0, kH1, kD0, kD1),
                  kM);
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
      kLinearX.data(), kLinearY.data(), kLinearX.size(), 0);
  EXPECT_GT(kResult, 0);
  EXPECT_NEAR(kResult, 2, kEpsLoose);
}

TEST(SplinesDetailTangentTest, RightEndCallsEndTangent) {
  const auto kResult = lob::spline::detail::Tangent<float>(
      kLinearX.data(), kLinearY.data(), kLinearX.size(), kLinearX.size() - 1);
  EXPECT_GT(kResult, 0);
  EXPECT_NEAR(kResult, 2, kEpsLoose);
}

TEST(SplinesDetailTangentTest, InteriorReturnsHarmonicMeanForLinear) {
  const auto kN = kLinearX.size();
  for (size_t i = 1; i + 1 < kN; ++i) {
    const auto kResult = lob::spline::detail::Tangent<float>(
        kLinearX.data(), kLinearY.data(), kN, i);
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
        kLinearX.data(), kLinearY.data(), kN, *(kLinearX.data() + i));
    EXPECT_FLOAT_EQ(kResult.y, *(kLinearY.data() + i));
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
        kLinearX.data(), kLinearY.data(), kN, kMid);
    EXPECT_NEAR(kResult.y, LinearY(kMid), kEpsLoose);
  }
}

TEST(SplinesDetailEvalWithDerivTest, ReturnsExactLinearDerivForLinearData) {
  const auto kN = kLinearX.size();
  for (size_t i = 1; i + 1 < kN; ++i) {
    const auto kResult = lob::spline::detail::EvalWithDeriv<float>(
        kLinearX.data(), kLinearY.data(), kN, *(kLinearX.data() + i));
    EXPECT_NEAR(kResult.dy, 2, kEpsLoose) << "i=" << i;
  }
}

TEST(SplinesDetailEvalWithDerivTest, BelowFirstKnotFindsFirstSegment) {
  constexpr auto kTen = 10.0F;
  const auto kN = kLinearX.size();
  EXPECT_EQ(
      lob::spline::detail::FindInterval<float>(kLinearX.data(), kN, -kTen), 0U);
  const auto kResult = lob::spline::detail::EvalWithDeriv<float>(
      kLinearX.data(), kLinearY.data(), kN, -kTen);
  EXPECT_TRUE(std::isfinite(kResult.y));
  EXPECT_TRUE(std::isfinite(kResult.dy));
}

TEST(SplinesDetailEvalWithDerivTest, AboveLastKnotClampsToLastInterval) {
  const auto kN = kLinearX.size();
  const auto kResult = lob::spline::detail::EvalWithDeriv<float>(
      kLinearX.data(), kLinearY.data(), kN, 1000.0F);
  EXPECT_GT(kResult.y, *(kLinearY.data() + kN - 2));
}

TEST(SplinesSegmentTest, ProducesCoefsReproducingDataAtKnots) {
  std::array<float, 4> coefs{};
  lob::spline::Segment<float>(kLinearX.data(), kLinearY.data(), kLinearX.size(),
                              0, 1, coefs.data());
  EXPECT_FLOAT_EQ(lob::spline::detail::PolyVal<float>(coefs.data(), 0),
                  *(kLinearY.data() + 0));
  EXPECT_NEAR(lob::spline::detail::PolyVal<float>(coefs.data(), 1),
              *(kLinearY.data() + 1), kEpsLoose);
}

TEST(SplinesSegmentTest, TangentsAtKnotsMatchEndEndTangents) {
  std::array<float, 4> coefs{};
  lob::spline::Segment<float>(kLinearX.data(), kLinearY.data(), kLinearX.size(),
                              0, 1, coefs.data());
  const auto* const cp = coefs.data();
  const auto kM0 = *(cp + 1);
  const auto kM1 =
      *(cp + 1) + ((1 - 0) * ((2 * *(cp + 2)) + (3 * *(cp + 3) * (1 - 0))));
  const auto kExpectedM0 = lob::spline::detail::Tangent<float>(
      kLinearX.data(), kLinearY.data(), kLinearX.size(), 0);
  const auto kExpectedM1 = lob::spline::detail::Tangent<float>(
      kLinearX.data(), kLinearY.data(), kLinearX.size(), 1);
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
  const auto kCount = lob::spline::Build<float>(
      kLinearX.data(), kLinearY.data(), kLinearX.size(), knots.data(), kN,
      coefs.data());
  EXPECT_EQ(kCount, kN - 1);
}

TEST(SplinesBuildTest, BuiltCoefsReproduceSourceDataAtKnots) {
  constexpr auto kN = size_t{5};
  std::array<float, kN> knots{0, 1, 2, 3, 4};
  std::array<float, 4 * (kN - 1)> coefs{};
  lob::spline::Build<float>(kLinearX.data(), kLinearY.data(), kLinearX.size(),
                            knots.data(), kN, coefs.data());
  for (size_t i = 0; i + 1 < kN; ++i) {
    const auto* const seg = coefs.data() + (4 * i);
    const auto kT1 = *(knots.data() + i + 1) - *(knots.data() + i);
    EXPECT_NEAR(lob::spline::detail::PolyVal<float>(seg, 0),
                LinearY(*(knots.data() + i)), kEpsLoose)
        << "segment i=" << i << " left endpoint";
    EXPECT_NEAR(lob::spline::detail::PolyVal<float>(seg, kT1),
                LinearY(*(knots.data() + i + 1)), kEpsLoose)
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
  lob::spline::View<float> view{lob::spline::kKnots.data(), coefs.data(),
                                lob::spline::kKnots.size()};
  for (size_t i = 0; i < lob::spline::kKnots.size(); ++i) {
    const auto kMach = *(lob::spline::kKnots.data() + i);
    const auto* const lo = std::lower_bound(
        lob::dragtable::kMachs.begin(), lob::dragtable::kMachs.end(), kMach);
    if (lo == lob::dragtable::kMachs.end() || !lob::AreEqual(*lo, kMach)) {
      continue;
    }
    const auto kIdx = static_cast<size_t>(lo - lob::dragtable::kMachs.begin());
    const auto kExpected = *(lob::dragtable::kG1Drags.data() + kIdx);
    EXPECT_FLOAT_EQ(view.Eval(kMach), kExpected)
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

TEST(SplinesMakeCoefsTest, RuntimeRobustAcrossMachsUsingRealCoefs) {
  lob::spline::View<float> view{lob::spline::kKnots.data(),
                                lob::spline::kG1Coefs.data(),
                                lob::spline::kKnotCount};
  for (size_t i = 0; i < lob::dragtable::kTableSize; ++i) {
    const auto kMach = *(lob::dragtable::kMachs.data() + i);
    const auto kResult = view.Eval(kMach);
    EXPECT_GT(kResult, 0);
    EXPECT_TRUE(std::isfinite(kResult));
  }
}

TEST(SplinesViewSegmentIndexTest, ReturnsZeroBeforeFirstKnot) {
  lob::spline::View<float> v{lob::spline::kKnots.data(),
                             lob::spline::kG1Coefs.data(),
                             lob::spline::kKnotCount};
  EXPECT_EQ(v.SegmentIndex(-1), 0U);
  EXPECT_EQ(v.SegmentIndex(0), 0U);
  EXPECT_EQ(v.SegmentIndex(0.3F), 0U);
}

TEST(SplinesViewSegmentIndexTest, ReturnsIAtKnotI) {
  lob::spline::View<float> v{lob::spline::kKnots.data(),
                             lob::spline::kG1Coefs.data(),
                             lob::spline::kKnotCount};
  for (size_t i = 0; i + 1 < lob::spline::kKnotCount; ++i) {
    EXPECT_EQ(v.SegmentIndex(*(lob::spline::kKnots.data() + i)), i);
  }
}

TEST(SplinesViewSegmentIndexTest, ReturnsLastSegmentAboveLastKnot) {
  constexpr auto kTen = 10.0F;
  lob::spline::View<float> v{lob::spline::kKnots.data(),
                             lob::spline::kG1Coefs.data(),
                             lob::spline::kKnotCount};
  EXPECT_EQ(v.SegmentIndex(5), lob::spline::kKnotCount - 2);
  EXPECT_EQ(v.SegmentIndex(kTen), lob::spline::kKnotCount - 2);
}

TEST(SplinesViewSegmentIndexTest, WalksEveryInterval) {
  lob::spline::View<float> v{lob::spline::kKnots.data(),
                             lob::spline::kG1Coefs.data(),
                             lob::spline::kKnotCount};
  for (size_t i = 0; i + 1 < lob::spline::kKnotCount; ++i) {
    const float kMid = 0.5F * (*(lob::spline::kKnots.data() + i) +
                             *(lob::spline::kKnots.data() + i + 1));
    EXPECT_EQ(v.SegmentIndex(kMid), i) << "interval i=" << i;
  }
}

TEST(SplinesViewEvalTest, MatchesTableAtKnotMachsExactly) {
  lob::spline::View<float> v{lob::spline::kKnots.data(),
                             lob::spline::kG1Coefs.data(),
                             lob::spline::kKnotCount};
  for (size_t i = 0; i < lob::spline::kKnots.size(); ++i) {
    const auto kMach = *(lob::spline::kKnots.data() + i);
    const auto* const lo = std::lower_bound(
        lob::dragtable::kMachs.begin(), lob::dragtable::kMachs.end(), kMach);
    if (lo == lob::dragtable::kMachs.end() || !lob::AreEqual(*lo, kMach)) {
      continue;
    }
    const auto kIdx = static_cast<size_t>(lo - lob::dragtable::kMachs.begin());
    const auto kExpected = *(lob::dragtable::kG1Drags.data() + kIdx);
    EXPECT_FLOAT_EQ(v.Eval(kMach), kExpected)
        << "knot i=" << i << " = " << kMach;
  }
}

TEST(SplinesViewEvalTest, ClampsBelowRangeToFirstValue) {
  lob::spline::View<float> v{lob::spline::kKnots.data(),
                             lob::spline::kG1Coefs.data(),
                             lob::spline::kKnotCount};
  const auto kResult = v.Eval(-1.0e9F);
  EXPECT_FLOAT_EQ(kResult, v.Eval(0));
  EXPECT_FLOAT_EQ(kResult, *(lob::dragtable::kG1Drags.data() + 0));
}

TEST(SplinesViewEvalTest, ClampsAboveRangeToLastValue) {
  lob::spline::View<float> v{lob::spline::kKnots.data(),
                             lob::spline::kG1Coefs.data(),
                             lob::spline::kKnotCount};
  const auto kResult = v.Eval(1.0e9F);
  EXPECT_FLOAT_EQ(kResult, v.Eval(5));
}

TEST(SplinesViewEvalTest, IsFiniteForAllTableMachs) {
  lob::spline::View<float> v{lob::spline::kKnots.data(),
                             lob::spline::kG1Coefs.data(),
                             lob::spline::kKnotCount};
  for (size_t i = 0; i < lob::dragtable::kTableSize; ++i) {
    const auto kMach = *(lob::dragtable::kMachs.data() + i);
    EXPECT_TRUE(std::isfinite(v.Eval(kMach))) << "mach #" << i;
  }
}

TEST(SplinesViewEvalTest, IsWellDefinedAtEndsToMatchTableEnds) {
  lob::spline::View<float> v{lob::spline::kKnots.data(),
                             lob::spline::kG1Coefs.data(),
                             lob::spline::kKnotCount};
  const auto kAtEnd = v.Eval(lob::spline::kKnots.back());
  const auto kAtStart = v.Eval(lob::spline::kKnots.front());
  EXPECT_FLOAT_EQ(kAtStart, lob::dragtable::kG1Drags.front());
  EXPECT_FLOAT_EQ(kAtEnd, lob::dragtable::kG1Drags.back());
}

TEST(SplinesViewDerivTest, DerivOfPolyValMatchesPolyDerivAtMidSpan) {
  lob::spline::View<float> v{lob::spline::kKnots.data(),
                             lob::spline::kG1Coefs.data(),
                             lob::spline::kKnotCount};
  for (size_t i = 0; i + 1 < lob::spline::kKnotCount; ++i) {
    const auto kLo = *(lob::spline::kKnots.data() + i);
    const auto kHi = *(lob::spline::kKnots.data() + i + 1);
    const auto kMid = 0.5F * (kLo + kHi);
    const auto kT = kMid - kLo;
    const auto* const seg = lob::spline::kG1Coefs.data() + (4 * i);
    const auto kExpected = lob::spline::detail::PolyDeriv<float>(seg, kT);
    EXPECT_NEAR(v.Deriv(kMid), kExpected, kEps) << "span i=" << i;
  }
}

TEST(SplinesViewDerivTest, ClampsBelowRangeToFirstSegDeriv) {
  lob::spline::View<float> v{lob::spline::kKnots.data(),
                             lob::spline::kG1Coefs.data(),
                             lob::spline::kKnotCount};
  const auto kBelow = v.Deriv(-1);
  const auto kAtFirst = v.Deriv(lob::spline::kKnots.front());
  EXPECT_FLOAT_EQ(kBelow, kAtFirst);
}

TEST(SplinesViewDerivTest, ClampsAboveRangeToLastSegDeriv) {
  constexpr auto kTen = 10.0F;
  lob::spline::View<float> v{lob::spline::kKnots.data(),
                             lob::spline::kG1Coefs.data(),
                             lob::spline::kKnotCount};
  const auto kAbove = v.Deriv(kTen);
  const auto kAtLast = v.Deriv(lob::spline::kKnots.back());
  EXPECT_FLOAT_EQ(kAbove, kAtLast);
}

TEST(SplinesViewDerivTest, IsFiniteForAllMachsInTable) {
  lob::spline::View<float> v{lob::spline::kKnots.data(),
                             lob::spline::kG1Coefs.data(),
                             lob::spline::kKnotCount};
  for (size_t i = 0; i < lob::dragtable::kTableSize; ++i) {
    EXPECT_TRUE(std::isfinite(v.Deriv(*(lob::dragtable::kMachs.data() + i))));
  }
}

TEST(SplinesViewDerivTest, MagnitudesReasonableAboveZero) {
  constexpr auto kMaxDeriv = 100.0F;
  lob::spline::View<float> v{lob::spline::kKnots.data(),
                             lob::spline::kG1Coefs.data(),
                             lob::spline::kKnotCount};
  for (size_t i = 0; i < lob::dragtable::kTableSize; ++i) {
    EXPECT_TRUE(lob::Fabs(v.Deriv(*(lob::dragtable::kMachs.data() + i))) <
                kMaxDeriv);
  }
}

TEST(SplinesCursorTest, StartsAtIdxZeroAndEvaluatesAtStart) {
  lob::spline::Cursor<float, lob::spline::kKnotCount> cursor(
      lob::spline::kKnots.data(), lob::spline::kG1Coefs.data());
  const auto kResult = cursor.Eval(0);
  EXPECT_FLOAT_EQ(kResult, *(lob::dragtable::kG1Drags.data() + 0));
  EXPECT_EQ(cursor.idx, 0U);
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

TEST(SplinesCursorTest, EvalsEqualViewEvalsAtRandomMachs) {
  lob::spline::View<float> view{lob::spline::kKnots.data(),
                                lob::spline::kG1Coefs.data(),
                                lob::spline::kKnotCount};
  lob::spline::Cursor<float, lob::spline::kKnotCount> cursor(
      lob::spline::kKnots.data(), lob::spline::kG1Coefs.data());
  const std::array<float, 9> kPeskyMachs{0.3F, 0.6F, 1.1F, 1.7F, 2.5F,
                                         3.5F, 4,    4.9F, 5};
  for (const auto kMach : kPeskyMachs) {
    EXPECT_FLOAT_EQ(cursor.Eval(kMach), view.Eval(kMach)) << "mach=" << kMach;
  }
}

TEST(SplinesCursorTest, ForwardBackwardMotionKeepsIdxInBounds) {
  lob::spline::Cursor<float, lob::spline::kKnotCount> cursor(
      lob::spline::kKnots.data(), lob::spline::kG1Coefs.data());
  cursor.Eval(5);  // NOLINT
  EXPECT_TRUE(cursor.idx < lob::spline::kKnotCount - 1);
  for (size_t i = 0; i < lob::spline::kKnotCount; ++i) {
    cursor.Eval(*(lob::spline::kKnots.data() + i));
    ASSERT_LE(cursor.idx, lob::spline::kKnotCount - 2);
  }
  cursor.Eval(0);
  EXPECT_EQ(cursor.idx, 0U);
}

TEST(SplinesCursorTest, SeeksContinuousUpwardFromStartToEndAndBack) {
  lob::spline::Cursor<float, lob::spline::kKnotCount> cursor(
      lob::spline::kKnots.data(), lob::spline::kG1Coefs.data());
  for (size_t i = 0; i < lob::spline::kKnots.size(); ++i) {
    cursor.Eval(*(lob::spline::kKnots.data() + i));
  }
  EXPECT_EQ(cursor.idx, lob::spline::kKnotCount - 2);
  for (size_t i = lob::spline::kKnots.size(); i-- > 0;) {
    cursor.Eval(*(lob::spline::kKnots.data() + i));
  }
  EXPECT_EQ(cursor.idx, 0);
}

TEST(SplinesCursorTest, DerivMatchesViewDerivAtRandomMachs) {
  lob::spline::View<float> view{lob::spline::kKnots.data(),
                                lob::spline::kG1Coefs.data(),
                                lob::spline::kKnotCount};
  lob::spline::Cursor<float, lob::spline::kKnotCount> cursor(
      lob::spline::kKnots.data(), lob::spline::kG1Coefs.data());
  const std::array<float, 9> kPeskyMachs{0.3F, 0.6F, 1.1F, 1.7F, 2.5F,
                                         3.5F, 4,    4.9F, 5};
  for (const auto kMach : kPeskyMachs) {
    EXPECT_FLOAT_EQ(cursor.Deriv(kMach), view.Deriv(kMach)) << "mach=" << kMach;
  }
}

TEST(SplinesCursorTest, DerivIsFiniteAcrossTable) {
  lob::spline::Cursor<float, lob::spline::kKnotCount> cursor(
      lob::spline::kKnots.data(), lob::spline::kG1Coefs.data());
  for (size_t i = 0; i < lob::dragtable::kTableSize; ++i) {
    EXPECT_TRUE(
        std::isfinite(cursor.Deriv(*(lob::dragtable::kMachs.data() + i))));
  }
}

TEST(SplinesCursorTest, IdxZeroAfterResetByEvaluatingAtStart) {
  lob::spline::Cursor<float, lob::spline::kKnotCount> cursor(
      lob::spline::kKnots.data(), lob::spline::kG1Coefs.data());
  cursor.Eval(5);  // NOLINT
  cursor.idx = 0;
  EXPECT_EQ(cursor.idx, 0U);
  EXPECT_FLOAT_EQ(cursor.Eval(0), *(lob::dragtable::kG1Drags.data() + 0));
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
      lob::spline::detail::Secant<float>(kLinearX.data(), kLinearY.data(), 0);
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
      kLinearX.data(), kLinearY.data(), kLinearX.size(), 0);
  EXPECT_GT(kTan, 0);
  const auto kEt = lob::spline::detail::EndTangent<float>(1, 1, 2, 2);
  EXPECT_GT(kEt, 0);
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

TEST(SplinesRuntimeContextsTest, ViewEvalsAtCompileTimeConstants) {
  lob::spline::View<float> v{lob::spline::kKnots.data(),
                             lob::spline::kG1Coefs.data(),
                             lob::spline::kKnotCount};
  const auto kResult = v.Eval(*(lob::spline::kKnots.data() + 0));
  EXPECT_FLOAT_EQ(kResult, *(lob::dragtable::kG1Drags.data() + 0));
}

TEST(SplinesRuntimeContextsTest, ViewDerivAtCompileTimeConstants) {
  lob::spline::View<float> v{lob::spline::kKnots.data(),
                             lob::spline::kG1Coefs.data(),
                             lob::spline::kKnotCount};
  const auto kResult = v.Deriv(*(lob::spline::kKnots.data() + 0));
  EXPECT_TRUE(std::isfinite(kResult));
  EXPECT_FLOAT_EQ(kResult, *(lob::spline::kG1Coefs.data() + 1));
}

TEST(SplinesRuntimeContextsTest, ViewSegIdxAtCompileTimeConstants) {
  lob::spline::View<float> v{lob::spline::kKnots.data(),
                             lob::spline::kG1Coefs.data(),
                             lob::spline::kKnotCount};
  const auto kIdx = v.SegmentIndex(2.6F);
  EXPECT_EQ(kIdx, 11U);
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
