#pragma once

#include <cstddef>
#include <utility>

#include "constants.hpp"
#include "helpers.hpp"

namespace lob {

constexpr std::size_t kGaussLegendreOrder = 8;

namespace detail {

constexpr void EvaluateLegendre(std::size_t n, double x, double* pp,
                                double* pdp) noexcept {
  double p_prev = 1.0;
  double p_curr = x;

  for (std::size_t k = 1; k < n; ++k) {
    const double kPNext = (((2 * static_cast<double>(k)) + 1.0) * x * p_curr -
                     static_cast<double>(k) * p_prev) /
                    static_cast<double>(k + 1);
    p_prev = p_curr;
    p_curr = kPNext;
  }

  *pp = p_curr;
  *pdp = (static_cast<double>(n) / (x * x - 1.0)) * (x * p_curr - p_prev);
}

}  // namespace detail

template <std::size_t N>
struct GaussLegendreRule {
  double nodes[N]{};  // NOLINT
  double weights[N]{};  // NOLINT
};

template <std::size_t N>
constexpr GaussLegendreRule<N> MakeGaussLegendreRule() noexcept {
  GaussLegendreRule<N> rule{};
  const std::size_t kM = (N + 1) / 2;

  for (std::size_t i = 0; i < kM; ++i) {
    const auto kK = static_cast<double>(i + 1);
    const double kQuarter = 0.25;
    const double kHalf = 0.5;
    const size_t kIter = 10;
    double z = Cos(kPi * (kK - kQuarter) / (static_cast<double>(N) + kHalf));

    double p = 0.0;
    double dp = 0.0;

    for (std::size_t j = 0; j < kIter; j++) {
      detail::EvaluateLegendre(N, z, &p, &dp);
      z -= p / dp;
    }

    detail::EvaluateLegendre(N, z, &p, &dp);

    rule.nodes[i] = -z;
    rule.nodes[N - 1 - i] = z;

    const double kW = 2.0 / ((1.0 - z * z) * dp * dp);
    rule.weights[i] = kW;
    rule.weights[N - 1 - i] = kW;
  }

  return rule;
}

template <std::size_t N>
constexpr bool IsValidGaussLegendreRule(
    const GaussLegendreRule<N>& r) noexcept {
  for (std::size_t i = 0; i < N; ++i) {
    if (IsNan(r.weights[i]) || IsInf(r.weights[i]) || r.weights[i] <= 0.0) {
      return false;
    }
    if (IsNan(r.nodes[i]) || IsInf(r.nodes[i]) || r.nodes[i] < -1.0 ||
        r.nodes[i] > 1.0) {
      return false;
    }
  }
  return true;
}

template <std::size_t N = kGaussLegendreOrder, typename BoundsT, typename Func>
constexpr auto IntegrateGaussLegendre(BoundsT a, BoundsT b, Func&& f) noexcept {
  constexpr auto kRule = MakeGaussLegendreRule<N>();
  static_assert(IsValidGaussLegendreRule(kRule), "Gauss-Legendre rule invalid");

  const auto kMid = (a + b) * 0.5;
  const auto kHalfRange = (b - a) * 0.5;

  auto sum = kRule.weights[0] *
             std::forward<Func>(f)(kMid + (kHalfRange * kRule.nodes[0]));
  for (std::size_t i = 1; i < N; ++i) {
    sum += kRule.weights[i] *
           std::forward<Func>(f)(kMid + (kHalfRange * kRule.nodes[i]));
  }

  return kHalfRange * sum;
}

}  // namespace lob