// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#pragma once

#include <cmath>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace lob {

template <typename T = double>
constexpr T NaN() {
  static_assert(std::is_floating_point<T>::value,
                "NaN() only supports floating-point types");
  return std::numeric_limits<T>::quiet_NaN();
}

template <typename T>
constexpr bool AreEqual(T a, T b) {
  return a == b;
}

template <typename T>
constexpr bool AreFloatingPointsEqual(T a, T b) {
  if (std::isinf(a) || std::isinf(b)) {
    return !(a > b) && !(b > a);
  }
  if (std::isnan(a) || std::isnan(b)) {
    return std::isnan(a) && std::isnan(b);
  }
  const T kDiff = std::fabs(a - b);
  const T kAbsTol = std::numeric_limits<T>::epsilon() * static_cast<T>(100);
  if (kDiff <= kAbsTol) {
    return true;
  }
  return (kDiff <= std::numeric_limits<T>::epsilon() *
                       std::fmax(std::fabs(a), std::fabs(b)));
}

constexpr bool AreEqual(double a, double b) {
  return AreFloatingPointsEqual(a, b);
}

constexpr bool AreEqual(float a, float b) {
  return AreFloatingPointsEqual(a, b);
}

template <typename T>
constexpr T Modulo(T a, T b) {
  return a % b;
}

template <typename T>
constexpr T ConstexprFmod(T a, T b) {
  if (AreEqual(b, 0)) {
    return std::numeric_limits<T>::quiet_NaN();
  }
  const T kQuot = a / b;
  if (!std::isnan(kQuot) && (kQuot >= static_cast<T>(9e18) ||
                             kQuot <= static_cast<T>(-9e18))) {
    return std::fmod(a, b);
  }
  if (std::isnan(kQuot) || std::isinf(kQuot)) {
    return std::fmod(a, b);
  }
  const auto kQuotient = static_cast<int64_t>(kQuot);
  return a - (static_cast<T>(kQuotient) * b);
}

constexpr double Modulo(double a, double b) { return ConstexprFmod(a, b); }

constexpr float Modulo(float a, float b) { return ConstexprFmod(a, b); }

}  // namespace lob

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