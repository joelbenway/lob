// This file is a part of lob, an exterior ballistics calculation library
// Copyright (c) 2025  Joel Benway
// Please see end of file for extended copyright information

#pragma once

#include <cmath>
#include <limits>
#include <type_traits>

namespace lob {

template <typename T>
constexpr bool AreFloatingPointsEqual(T a, T b) {
  if (std::isinf(a) || std::isinf(b)) {
    return !(a > b) && !(b > a);
  }
  if (std::isnan(a) || std::isnan(b)) {
    return std::isnan(a) && std::isnan(b);
  }
  return (std::fabs(a - b) <= std::numeric_limits<T>::epsilon() *
                                  std::fmax(std::fabs(a), std::fabs(b)));
}

template <typename T>
constexpr bool AreEqual(T a, T b) {
  return a == b;
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

constexpr double Modulo(double a, double b) { return std::fmod(a, b); }

constexpr float Modulo(float a, float b) { return std::fmod(a, b); }

}  // namespace lob

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.