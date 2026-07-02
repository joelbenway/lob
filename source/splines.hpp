// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#pragma once

#include <array>
#include <cstddef>

#include "helpers.hpp"
#include "tables.hpp"

namespace lob {
namespace spline {

constexpr static int kNumSplineKnots = 14;
constexpr static int kNumSplinePieces = 13;
// Universal knot positions optimized for G7, shared by all drag functions.
constexpr std::array<size_t, kNumSplineKnots> kSplineKnots = {
    0, 6, 13, 18, 22, 23, 26, 27, 31, 38, 52, 60, 79, 84};

constexpr double kHermiteCoeff = 3.0;
constexpr double kDerivCoeff = 2.0;

template <typename T, size_t N>
struct Arr {
  // NOLINTNEXTLINE
  T data[N]{};
  // NOLINTNEXTLINE (cppcoreguidelines-pro-bounds-constant-array-index)
  constexpr T& operator[](size_t i) { return data[i]; }
  // NOLINTNEXTLINE (cppcoreguidelines-pro-bounds-constant-array-index)
  constexpr const T& operator[](size_t i) const { return data[i]; }
};

struct CubicCoeffs {
  double a, b, c, d;
};

template <size_t N, size_t P>
struct SplineData {
  Arr<double, N> x0{};
  Arr<CubicCoeffs, P> coeffs{};
};

template <size_t N, size_t P>
constexpr void AccumulateSystem(
    const std::array<uint16_t, lob::kTableSize>& drag_table,
    const Arr<double, N>& x0, const Arr<double, N>& yk, const Arr<double, P>& h,
    Arr<Arr<double, N>, N>& a, Arr<double, N>& bb) noexcept {
  for (size_t pi = 0; pi < lob::kTableSize; ++pi) {
    const auto kXi = static_cast<double>(lob::kMachs.at(pi));
    const auto kYi = static_cast<double>(drag_table.at(pi));
    if (kXi < x0[0] || kXi >= x0[P]) {
      continue;
    }

    size_t j = 0;
    while (j < P && kXi >= x0[j + 1]) {
      ++j;
    }
    if (j >= P) {
      continue;
    }

    const double kT = (kXi - x0[j]) / h[j];
    const double kT2 = kT * kT;
    const double kT3 = kT2 * kT;
    const double kH10 = (kT3 - (kDerivCoeff * kT2) + kT) * h[j];
    const double kH11 = (kT3 - kT2) * h[j];
    const double kH00 = (kDerivCoeff * kT3) - (kHermiteCoeff * kT2) + 1.0;
    const double kH01 = (-kDerivCoeff * kT3) + (kHermiteCoeff * kT2);
    const double kBase = (yk[j] * kH00) + (yk[j + 1] * kH01);
    const double kResid = kYi - kBase;

    a[j][j] += kH10 * kH10;
    a[j][j + 1] += kH10 * kH11;
    a[j + 1][j] = a[j][j + 1];
    a[j + 1][j + 1] += kH11 * kH11;
    bb[j] += kResid * kH10;
    bb[j + 1] += kResid * kH11;
  }
}

template <size_t N, size_t P>
constexpr Arr<double, N> CholeskySolve(const Arr<Arr<double, N>, N>& a,
                                       const Arr<double, N>& bb) noexcept {
  auto l = Arr<Arr<double, N>, N>{};
  for (size_t i = 0; i < N; ++i) {
    for (size_t j = 0; j <= i; ++j) {
      double sum = a[i][j];
      for (size_t k = 0; k < j; ++k) {
        sum -= l[i][k] * l[j][k];
      }
      l[i][j] = (i == j) ? Sqrt(sum) : sum / l[j][j];
    }
  }

  auto y = Arr<double, N>{};
  for (size_t i = 0; i < N; ++i) {
    double sum = bb[i];
    for (size_t j = 0; j < i; ++j) {
      sum -= l[i][j] * y[j];
    }
    y[i] = sum / l[i][i];
  }

  auto d = Arr<double, N>{};
  for (size_t i = P + 1; i > 0; --i) {
    const size_t kIdx = i - 1;
    double sum = y[kIdx];
    for (size_t j = kIdx + 1; j < N; ++j) {
      sum -= l[j][kIdx] * d[j];
    }
    d[kIdx] = sum / l[kIdx][kIdx];
  }

  return d;
}

template <const std::array<uint16_t, lob::kTableSize>& kDragTable,
          const std::array<size_t, kNumSplineKnots>& kKnots>
constexpr auto Fit() noexcept {
  constexpr auto kNumKnots = kNumSplineKnots;
  constexpr auto kNumPieces = kNumSplinePieces;
  SplineData<kNumKnots, kNumPieces> result{};

  for (size_t i = 0; i < kNumKnots; ++i) {
    result.x0[i] = static_cast<double>(lob::kMachs.at(kKnots.at(i)));
  }

  Arr<double, kNumKnots> yk{};
  for (size_t i = 0; i < kNumKnots; ++i) {
    yk[i] = static_cast<double>(kDragTable.at(kKnots.at(i)));
  }

  Arr<double, kNumPieces> h{};
  for (size_t i = 0; i < kNumPieces; ++i) {
    h[i] = result.x0[i + 1] - result.x0[i];
  }

  auto a = Arr<Arr<double, kNumKnots>, kNumKnots>{};
  auto bb = Arr<double, kNumKnots>{};
  AccumulateSystem<kNumKnots, kNumPieces>(kDragTable, result.x0, yk, h, a, bb);

  constexpr double kLambda = 1e-7;
  for (size_t i = 0; i < kNumKnots; ++i) {
    a[i][i] += kLambda;
  }

  auto d = CholeskySolve<kNumKnots, kNumPieces>(a, bb);

  for (size_t i = 0; i < kNumPieces; ++i) {
    const double kDy = yk[i + 1] - yk[i];
    const double kHi = h[i];
    const double kHi2 = kHi * kHi;
    const double kHi3 = kHi2 * kHi;
    result.coeffs[i].a = yk[i];
    result.coeffs[i].b = d[i];
    result.coeffs[i].c = ((kHermiteCoeff * kDy) - (kDerivCoeff * d[i] * kHi) -
                          (d[i + 1] * kHi)) /
                         kHi2;
    result.coeffs[i].d =
        ((-kDerivCoeff * kDy) + (d[i] * kHi) + (d[i + 1] * kHi)) / kHi3;
  }

  return result;
}

template <size_t N, size_t P>
constexpr double Eval(const Arr<double, N>& x0,
                      const Arr<CubicCoeffs, P>& coeffs,
                      const double x_in) noexcept {
  constexpr auto kLast = P - 1;

  if (x_in <= x0[0]) {
    return coeffs[0].a;
  }
  if (x_in >= x0[P]) {
    const double kH = x0[kLast + 1] - x0[kLast];
    const auto& p = coeffs[kLast];
    return p.a + (kH * (p.b + (kH * (p.c + (kH * p.d)))));
  }

  size_t idx = 0;
  for (size_t i = 1; i < P; ++i) {
    idx += (x_in >= x0[i]) ? size_t{1} : size_t{0};
  }

  const auto& p = coeffs[idx];
  const double kT = x_in - x0[idx];
  return p.a + (kT * (p.b + (kT * (p.c + (kT * p.d)))));
}

template <size_t N, size_t P>
constexpr double Eval(const SplineData<N, P>& data,
                      const double x_in) noexcept {
  return Eval(data.x0, data.coeffs, x_in);
}

}  // namespace spline
}  // namespace lob
