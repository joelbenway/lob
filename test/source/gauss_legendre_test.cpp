// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include "gauss_legendre.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <functional>
#include <limits>
#include <memory>
#include <utility>

#include "constants.hpp"
#include "eng_units.hpp"
#include "ode.hpp"

namespace tests {

namespace {

constexpr double kTolLoose = 1.0e-9;
constexpr double kTolTight = 1.0e-12;
constexpr double kTolRefNodes = 1.0e-12;
constexpr double kTwo = 2.0;
constexpr double kThree = 3.0;
constexpr double kEight = 8.0;
constexpr double kSeventeen = 17.0;
constexpr int kMaxDegree = 15;
constexpr int kBeyondDegree = 16;
constexpr std::size_t kOrder2 = 2;
constexpr std::size_t kOrder3 = 3;
constexpr std::size_t kOrder5 = 5;
constexpr std::size_t kOrder8 = 8;
constexpr std::size_t kLargeOrder = 16;
constexpr std::size_t kMaxTestOrder = 5;
constexpr double kLowerBound = -1.0;
constexpr double kUpperBound = 1.0;

constexpr std::array<double, 2> kNodes2{-0.5773502691896257,
                                       0.5773502691896257};
constexpr std::array<double, 2> kWeights2{1.0, 1.0};

constexpr std::array<double, 8> kNodes8{
    -0.9602898564975363, -0.7966664774136267, -0.5255324099163290,
    -0.1834346424956498, 0.1834346424956498,  0.5255324099163290,
    0.7966664774136267,  0.9602898564975363};
constexpr std::array<double, 8> kWeights8{
    0.10122853629037626, 0.22238103445337448, 0.31370664587788727,
    0.3626837833783620,  0.3626837833783620,  0.31370664587788727,
    0.22238103445337448, 0.10122853629037626};

double LegendreP(std::size_t n, double x) {
  if (n == 0) {
    return 1.0;
  }
  if (n == 1) {
    return x;
  }
  double p0 = 1.0;
  double p1 = x;
  for (std::size_t k = 1; k < n; ++k) {
    const double kP2 = ((kTwo * static_cast<double>(k) + 1.0) * x * p1 -
                       static_cast<double>(k) * p0) /
                      static_cast<double>(k + 1);
    p0 = p1;
    p1 = kP2;
  }
  return p1;
}

}  // namespace

TEST(GaussLegendreEvaluateTest, MatchesRecurrence) {
  const std::array<double, 5> kXs{-0.8, -0.3, 0.0, 0.3, 0.8};
  for (const double kX : kXs) {
    for (std::size_t order = 1; order <= kOrder8; ++order) {
      double p = 0.0;
      double dp = 0.0;
      lob::detail::EvaluateLegendre(order, kX, &p, &dp);
      EXPECT_NEAR(p, LegendreP(order, kX), 1.0e-12) << "n=" << order << " x=" << kX;
    }
  }
}

TEST(GaussLegendreEvaluateTest, Derivative) {
  const double kX = 0.3;
  const double kH = 1e-6;
  for (std::size_t order = 1; order <= kMaxTestOrder; ++order) {
    double p = 0.0;
    double dp = 0.0;
    lob::detail::EvaluateLegendre(order, kX, &p, &dp);
    const double kFd = (LegendreP(order, kX + kH) - LegendreP(order, kX - kH)) / (kTwo * kH);
    EXPECT_NEAR(dp, kFd, 1e-5) << "n=" << order;
  }
  const double kX2 = 0.5;
  double p = 0.0;
  double dp = 0.0;
  lob::detail::EvaluateLegendre(2, kX2, &p, &dp);
  EXPECT_NEAR(p, (kThree * 0.25 - 1.0) / kTwo, kTolTight);
  EXPECT_NEAR(dp, 1.5, kTolTight);
}

TEST(MakeGaussLegendreRuleTest, ReferenceValuesN2) {
  constexpr auto kR2 = lob::MakeGaussLegendreRule<kOrder2>();
  EXPECT_NEAR(kR2.nodes[0], kNodes2[0], kTolRefNodes);
  EXPECT_NEAR(kR2.nodes[1], kNodes2[1], kTolRefNodes);
  EXPECT_NEAR(kR2.weights[0], kWeights2[0], kTolRefNodes);
  EXPECT_NEAR(kR2.weights[1], kWeights2[1], kTolRefNodes);
}

TEST(MakeGaussLegendreRuleTest, ReferenceValuesN8) {
  const auto kR8 = lob::MakeGaussLegendreRule<kOrder8>();
  EXPECT_NEAR(kR8.nodes[0], kNodes8[0], 1e-12);
  EXPECT_NEAR(kR8.nodes[1], kNodes8[1], 1e-12);
  EXPECT_NEAR(kR8.nodes[2], kNodes8[2], 1e-12);
  EXPECT_NEAR(kR8.nodes[3], kNodes8[3], 1e-12);
  EXPECT_NEAR(kR8.nodes[4], kNodes8[4], 1e-12);
  EXPECT_NEAR(kR8.nodes[5], kNodes8[5], 1e-12);
  EXPECT_NEAR(kR8.nodes[6], kNodes8[6], 1e-12);
  EXPECT_NEAR(kR8.nodes[7], kNodes8[7], 1e-12);
  EXPECT_NEAR(kR8.weights[0], kWeights8[0], 1e-12);
  EXPECT_NEAR(kR8.weights[1], kWeights8[1], 1e-12);
  EXPECT_NEAR(kR8.weights[2], kWeights8[2], 1e-12);
  EXPECT_NEAR(kR8.weights[3], kWeights8[3], 1e-12);
  EXPECT_NEAR(kR8.weights[4], kWeights8[4], 1e-12);
  EXPECT_NEAR(kR8.weights[5], kWeights8[5], 1e-12);
  EXPECT_NEAR(kR8.weights[6], kWeights8[6], 1e-12);
  EXPECT_NEAR(kR8.weights[7], kWeights8[7], 1e-12);
}

TEST(MakeGaussLegendreRuleTest, Symmetry) {
  const auto kR = lob::MakeGaussLegendreRule<kOrder8>();
  EXPECT_NEAR(kR.nodes[0], -kR.nodes[7], kTolTight);
  EXPECT_NEAR(kR.nodes[1], -kR.nodes[6], kTolTight);
  EXPECT_NEAR(kR.nodes[2], -kR.nodes[5], kTolTight);
  EXPECT_NEAR(kR.nodes[3], -kR.nodes[4], kTolTight);
  EXPECT_NEAR(kR.weights[0], kR.weights[7], kTolTight);
  EXPECT_NEAR(kR.weights[1], kR.weights[6], kTolTight);
  EXPECT_NEAR(kR.weights[2], kR.weights[5], kTolTight);
  EXPECT_NEAR(kR.weights[3], kR.weights[4], kTolTight);
  EXPECT_LT(kR.nodes[0], kR.nodes[1]);
  EXPECT_LT(kR.nodes[1], kR.nodes[2]);
  EXPECT_LT(kR.nodes[2], kR.nodes[3]);
  EXPECT_LT(kR.nodes[3], kR.nodes[4]);
}

TEST(MakeGaussLegendreRuleTest, Invariants) {
  const auto kR = lob::MakeGaussLegendreRule<kOrder8>();
  double sum = 0.0;
  for (const auto kW : kR.weights) {
    sum += kW;
  }
  EXPECT_NEAR(sum, kTwo, kTolTight);
  EXPECT_GT(kR.nodes[0], kLowerBound);
  EXPECT_LT(kR.nodes[7], kUpperBound);
  EXPECT_GT(kR.weights[0], 0.0);
  EXPECT_NEAR(lob::MakeGaussLegendreRule<kOrder3>().nodes[1], 0.0, kTolTight);
  EXPECT_NEAR(lob::MakeGaussLegendreRule<kOrder5>().nodes[2], 0.0, kTolTight);
}

TEST(IsValidGaussLegendreRuleTest, Valid) {
  EXPECT_TRUE(
      lob::IsValidGaussLegendreRule(lob::MakeGaussLegendreRule<1>()));
  EXPECT_TRUE(lob::IsValidGaussLegendreRule(
      lob::MakeGaussLegendreRule<kOrder8>()));
}

TEST(IsValidGaussLegendreRuleTest, RejectsBadWeight) {
  lob::GaussLegendreRule<kOrder2> r{};
  r.nodes[0] = kNodes2[0];
  r.nodes[1] = kNodes2[1];
  r.weights[0] = std::numeric_limits<double>::quiet_NaN();
  r.weights[1] = 1.0;
  EXPECT_FALSE(lob::IsValidGaussLegendreRule(r));
  r.weights[0] = std::numeric_limits<double>::infinity();
  EXPECT_FALSE(lob::IsValidGaussLegendreRule(r));
  const double kNegativeWeight = -0.5;
  r.weights[0] = kNegativeWeight;
  EXPECT_FALSE(lob::IsValidGaussLegendreRule(r));
  r.weights[0] = 0.0;
  EXPECT_FALSE(lob::IsValidGaussLegendreRule(r));
}

TEST(IsValidGaussLegendreRuleTest, RejectsBadNode) {
  lob::GaussLegendreRule<kOrder2> r{};
  r.weights[0] = 1.0;
  r.weights[1] = 1.0;
  r.nodes[0] = std::numeric_limits<double>::quiet_NaN();
  r.nodes[1] = kNodes2[1];
  EXPECT_FALSE(lob::IsValidGaussLegendreRule(r));
  r.nodes[0] = kNodes2[0];
  r.nodes[1] = std::numeric_limits<double>::infinity();
  EXPECT_FALSE(lob::IsValidGaussLegendreRule(r));
  const double kNode = 1.5;
  r.nodes[0] = -kNode;
  r.nodes[1] = kNode;
  EXPECT_FALSE(lob::IsValidGaussLegendreRule(r));
}

TEST(IsValidGaussLegendreRuleTest, AllowsBoundary) {
  lob::GaussLegendreRule<kOrder2> r{};
  r.nodes[0] = -1.0;
  r.nodes[1] = 1.0;
  r.weights[0] = 1.0;
  r.weights[1] = 1.0;
  EXPECT_TRUE(lob::IsValidGaussLegendreRule(r));
  constexpr lob::GaussLegendreRule<kOrder2> kGood{{-0.5, 0.5}, {1.0, 1.0}};
  constexpr lob::GaussLegendreRule<kOrder2> kBad{{-2.0, 0.0}, {1.0, 1.0}};
  EXPECT_TRUE(lob::IsValidGaussLegendreRule(kGood));
  EXPECT_FALSE(lob::IsValidGaussLegendreRule(kBad));
}

TEST(IntegrateGaussLegendreTest, PolynomialExactness) {
  for (int deg = 0; deg <= kMaxDegree; ++deg) {
    auto f = [deg](double x) { return std::pow(x, deg); };
    const double kExpected = (deg % 2 == 1) ? 0.0 : kTwo / (deg + 1);
    EXPECT_NEAR(lob::IntegrateGaussLegendre<kOrder8>(-1.0, 1.0, f), kExpected,
                1e-11)
        << "deg=" << deg;
  }
  EXPECT_EQ(lob::kGaussLegendreOrder, kOrder8);
  const auto kF15 = [](double x) { return std::pow(x, kMaxDegree); };
  EXPECT_NEAR(lob::IntegrateGaussLegendre(-1.0, 1.0, kF15), 0.0, kTolTight);
}

TEST(IntegrateGaussLegendreTest, BeyondExactness) {
  auto f = [](double x) { return std::pow(x, kBeyondDegree); };
  const double kGot = lob::IntegrateGaussLegendre<kOrder8>(-1.0, 1.0, f);
  constexpr double kExact = kTwo / kSeventeen;
  EXPECT_NEAR(kGot, kExact, 1e-4);
  EXPECT_GT(std::abs(kGot - kExact), 1e-12);
}

TEST(IntegrateGaussLegendreTest, NonPolynomial) {
  EXPECT_NEAR(lob::IntegrateGaussLegendre(
                  -1.0, 1.0, [](double x) { return std::sin(x); }),
              0.0, kTolTight);
  EXPECT_NEAR(lob::IntegrateGaussLegendre(
                  0.0, lob::kPi, [](double x) { return std::sin(x); }),
              kTwo, 1e-11);
  const double kExpExpected = std::exp(1.0) - 1.0;
  EXPECT_NEAR(lob::IntegrateGaussLegendre(
                  0.0, 1.0, [](double x) { return std::exp(x); }),
              kExpExpected, 1e-11);
}

TEST(IntegrateGaussLegendreTest, EdgeIntervals) {
  auto f = [](double x) { return x * x; };
  EXPECT_NEAR(lob::IntegrateGaussLegendre(1.0, 0.0, f),
              -lob::IntegrateGaussLegendre(0.0, 1.0, f), kTolTight);
  EXPECT_NEAR(lob::IntegrateGaussLegendre(3.0, 3.0, f), 0.0, kTolTight);
}

TEST(IntegrateGaussLegendreTest, CallableAndForwarding) {
  auto lam = [](double x) { return x + 1; };
  EXPECT_NEAR(lob::IntegrateGaussLegendre(0.0, 1.0, lam), 1.5, kTolTight);
  struct Functor {
    double operator()(double x) const { return x * kTwo; }
  };
  EXPECT_NEAR(lob::IntegrateGaussLegendre(0.0, 1.0, Functor{}), 1.0, kTolTight);
  const std::function<double(double)> kSf = [](double x) { return x * x; };
  EXPECT_NEAR(lob::IntegrateGaussLegendre(0.0, 1.0, kSf), 1.0 / kThree,
              kTolTight);
  int count = 0;
  auto counting = [&count](double x) {
    ++count;
    return x;
  };
  lob::IntegrateGaussLegendre<4>(0.0, 1.0, counting);
  EXPECT_EQ(count, 4);
  count = 0;
  lob::IntegrateGaussLegendre(0.0, 1.0, counting);
  EXPECT_EQ(count, 8);
  auto uniq = [p = std::make_unique<double>(kTwo)](double x) {
    return (*p) * x;
  };
  EXPECT_NEAR(lob::IntegrateGaussLegendre(0.0, 1.0, std::move(uniq)), 1.0,
              kTolTight);
}

TEST(IntegrateGaussLegendreTest, StrongTAndLargeN) {
  auto f = [](lob::FeetT x) { return x.Value() * x.Value(); };
  const auto kRes =
      lob::IntegrateGaussLegendre(lob::FeetT(0.0), lob::FeetT(kTwo), f);
  EXPECT_NEAR(kRes.Value(), kEight / kThree, kTolLoose);
  auto s = [](double x) { return std::sin(x); };
  const double kR8 = lob::IntegrateGaussLegendre<kOrder8>(0.0, lob::kPi, s);
  const double kR16 = lob::IntegrateGaussLegendre<kLargeOrder>(0.0, lob::kPi, s);
  EXPECT_NEAR(kR8, kTwo, 1e-11);
  EXPECT_NEAR(kR16, kTwo, 1e-12);
}

TEST(IntegrateGaussLegendreTest, MatchesRungeKutta) {
  const auto kRkIntegrate = [](auto f, double a, double b) {
    double y = 0.0;
    double t = a;
    const double kDt = 1e-4;
    while (t < b) {
      const double kStep = std::min(kDt, b - t);
      y = lob::RungeKuttaStep(t, y, kStep,
                              [&](double tt, double) { return f(tt); });
      t += kStep;
    }
    return y;
  };
  const auto kExpF = [](double x) { return std::exp(x); };
  EXPECT_NEAR(lob::IntegrateGaussLegendre(0.0, 1.0, kExpF),
              kRkIntegrate(kExpF, 0.0, 1.0), 1e-6);
  const auto kSinF = [](double x) { return std::sin(x); };
  EXPECT_NEAR(lob::IntegrateGaussLegendre(0.0, lob::kPi, kSinF),
              kRkIntegrate(kSinF, 0.0, lob::kPi), 1e-6);
  const auto kQuadF = [](double x) { return x * x; };
  EXPECT_NEAR(lob::IntegrateGaussLegendre(-1.0, 1.0, kQuadF),
              kRkIntegrate(kQuadF, -1.0, 1.0), 1e-6);
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
