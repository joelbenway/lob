// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#pragma once

#include <cmath>
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
constexpr T Fabs(T x) {
  if (x < 0) {
    return -x;
  }
  return x;
}

template <typename T>
constexpr T Fmax(T a, T b) {
  if (a > b) {
    return a;
  }
  return b;
}

template <typename T>
constexpr bool IsInf(T x) {
  return x > std::numeric_limits<T>::max() ||
         x < -std::numeric_limits<T>::max();
}

template <typename T>
constexpr bool IsNan(T x) {
  return !(x <= std::numeric_limits<T>::max()) &&
         !(x >= -std::numeric_limits<T>::max());
}

template <typename T>
constexpr bool AreFloatingPointsEqual(T a, T b) {
  if (IsInf(a) || IsInf(b)) {
    return !(a > b) && !(b > a);
  }
  if (IsNan(a) || IsNan(b)) {
    return IsNan(a) && IsNan(b);
  }
  const T kDiff = Fabs(a - b);
  const T kAbsTol = std::numeric_limits<T>::epsilon() * static_cast<T>(100);
  if (kDiff <= kAbsTol) {
    return true;
  }
  return (kDiff <= std::numeric_limits<T>::epsilon() * Fmax(Fabs(a), Fabs(b)));
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
constexpr T Fmod(T x, T y) {
  if (IsNan(x) || IsNan(y)) {
    return NaN<T>();
  }

  const T kAbsX = Fabs(x);
  const T kAbsY = Fabs(y);

  const T kInfinity = std::numeric_limits<T>::infinity();

  if (AreEqual(kAbsY, T(0)) || AreEqual(kAbsX, kInfinity)) {
    return std::numeric_limits<T>::quiet_NaN();
  }
  if (AreEqual(kAbsY, kInfinity)) {
    return x;
  }
  if (AreEqual(x, T(0))) {
    return x;
  }
  T mod = kAbsX;
  while (mod >= kAbsY) {
    T temp = kAbsY;
    while (temp <= mod / 2) {
      temp *= 2;
    }
    mod -= temp;
  }
  if (x < 0) {
    return -mod;
  }

  return mod;
}

constexpr double Modulo(double a, double b) { return Fmod(a, b); }

constexpr float Modulo(float a, float b) { return Fmod(a, b); }

constexpr double Sqrt(double x) {
  if (x < 0.0 || IsNan(x)) {
    return NaN();
  }
  if (AreEqual(x, 0.0)) {
    return 0.0;
  }
  if (x >= std::numeric_limits<double>::infinity()) {
    return std::numeric_limits<double>::infinity();
  }
  double curr = x;
  double prev = 0;
  while (!AreEqual(curr, prev)) {
    prev = curr;
    curr = (curr + x / curr) / 2;
  }
  return curr;
}

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
