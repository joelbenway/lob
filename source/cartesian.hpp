// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#pragma once

#include <cmath>
#include <utility>

#include "eng_units.hpp"

namespace lob {

template <typename T>
class CartesianT {
 public:
  constexpr CartesianT() = default;
  constexpr CartesianT(T x, T y, T z)
      : x_(std::move(x)), y_(std::move(y)), z_(std::move(z)) {}
  constexpr explicit CartesianT(T value) : x_(value), y_(value), z_(value) {}
  constexpr CartesianT(const CartesianT& other) = default;
  constexpr CartesianT(CartesianT&& other) noexcept = default;
  ~CartesianT() = default;
  constexpr CartesianT& operator=(const CartesianT& rhs) {
    if (this != &rhs) {
      x_ = rhs.x_;
      y_ = rhs.y_;
      z_ = rhs.z_;
    }
    return *this;
  }
  constexpr CartesianT& operator=(CartesianT&& rhs) noexcept {
    if (this != &rhs) {
      x_ = rhs.x_;
      y_ = rhs.y_;
      z_ = rhs.z_;
      rhs.x_ = static_cast<T>(0);
      rhs.y_ = static_cast<T>(0);
      rhs.z_ = static_cast<T>(0);
    }
    return *this;
  }

  // Arithmetic operators
  constexpr CartesianT operator+(const CartesianT& rhs) const {
    return CartesianT{x_ + rhs.x_, y_ + rhs.y_, z_ + rhs.z_};
  }
  constexpr CartesianT operator+(const T& rhs) const {
    return CartesianT{x_ + rhs, y_ + rhs, z_ + rhs};
  }
  constexpr CartesianT operator-(const CartesianT& rhs) const {
    return CartesianT{x_ - rhs.x_, y_ - rhs.y_, z_ - rhs.z_};
  }
  constexpr CartesianT operator-(const T& rhs) const {
    return CartesianT{x_ - rhs, y_ - rhs, z_ - rhs};
  }
  constexpr CartesianT operator*(const CartesianT& rhs) const {
    return CartesianT{x_ * rhs.x_, y_ * rhs.y_, z_ * rhs.z_};
  }
  constexpr CartesianT operator*(const T& rhs) const {
    return CartesianT{x_ * rhs, y_ * rhs, z_ * rhs};
  }
  constexpr CartesianT operator/(const CartesianT& rhs) const {
    return CartesianT{x_ / rhs.x_, y_ / rhs.y_, z_ / rhs.z_};
  }
  constexpr CartesianT operator/(const T& rhs) const {
    return CartesianT{x_ / rhs, y_ / rhs, z_ / rhs};
  }

  constexpr T X() const { return x_; }
  constexpr T Y() const { return y_; }
  constexpr T Z() const { return z_; }
  constexpr void X(const T& x_in) { x_ = x_in; }
  constexpr void Y(const T& y_in) { y_ = y_in; }
  constexpr void Z(const T& z_in) { z_ = z_in; }

  constexpr T Magnitude() const {
    return std::sqrt(std::pow(x_, 2) + std::pow(y_, 2) + std::pow(z_, 2));
  }

 private:
  T x_{0}, y_{0}, z_{0};
};

template <>
constexpr FeetT CartesianT<FeetT>::Magnitude() const {
  return FeetT(std::sqrt(std::pow(x_.Value(), 2) + std::pow(y_.Value(), 2) +
                         std::pow(z_.Value(), 2)));
}

template <>
constexpr FpsT CartesianT<FpsT>::Magnitude() const {
  return FpsT(std::sqrt(std::pow(x_.Value(), 2) + std::pow(y_.Value(), 2) +
                        std::pow(z_.Value(), 2)));
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