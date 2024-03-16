// This file is a part of lob, an exterior ballistics calculation library
// Copyright (c) 2024  Joel Benway
// Please see end of file for extended copyright information

#pragma once

#include <cmath>
#include <utility>

#include "eng_units.hpp"

namespace lob {

template <typename T>
class CartesianT {
 public:
  CartesianT() = default;
  CartesianT(T x, T y, T z)  // NOLINT
      : x_(std::move(x)), y_(std::move(y)), z_(std::move(z)) {}
  explicit CartesianT(T value) : x_(value), y_(value), z_(value) {}
  CartesianT(const CartesianT& other) = default;
  CartesianT(CartesianT&& other) noexcept = default;
  ~CartesianT() = default;
  CartesianT& operator=(const CartesianT& rhs) {
    if (this != &rhs) {
      x_ = rhs.x_;
      y_ = rhs.y_;
      z_ = rhs.z_;
    }
    return *this;
  }
  CartesianT& operator=(CartesianT&& rhs) noexcept {
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
  CartesianT operator+(const CartesianT& rhs) const {
    return CartesianT{x_ + rhs.x_, y_ + rhs.y_, z_ + rhs.z_};
  }
  CartesianT operator+(const T& rhs) const {
    return CartesianT{x_ + rhs, y_ + rhs, z_ + rhs};
  }
  CartesianT operator-(const CartesianT& rhs) const {
    return CartesianT{x_ - rhs.x_, y_ - rhs.y_, z_ - rhs.z_};
  }
  CartesianT operator-(const T& rhs) const {
    return CartesianT{x_ - rhs, y_ - rhs, z_ - rhs};
  }
  CartesianT operator*(const CartesianT& rhs) const {
    return CartesianT{x_ * rhs.x_, y_ * rhs.y_, z_ * rhs.z_};
  }
  CartesianT operator*(const T& rhs) const {
    return CartesianT{x_ * rhs, y_ * rhs, z_ * rhs};
  }
  CartesianT operator/(const CartesianT& rhs) const {
    return CartesianT{x_ / rhs.x_, y_ / rhs.y_, z_ / rhs.z_};
  }
  CartesianT operator/(const T& rhs) const {
    return CartesianT{x_ / rhs, y_ / rhs, z_ / rhs};
  }

  T X() const { return x_; }
  T Y() const { return y_; }
  T Z() const { return z_; }
  void X(const T& x_in) { x_ = x_in; }
  void Y(const T& y_in) { y_ = y_in; }
  void Z(const T& z_in) { z_ = z_in; }

  T Magnitude() const {
    return std::sqrt(std::pow(x_, 2) + std::pow(y_, 2) + std::pow(z_, 2));
  }

 private:
  T x_{0}, y_{0}, z_{0};
};

template <>
FeetT CartesianT<FeetT>::Magnitude() const;
template <>
FpsT CartesianT<FpsT>::Magnitude() const;

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