// This file is a part of lob, an exterior ballistics calculation library
// Copyright (c) 2024  Joel Benway
// Please see end of file for extended copyright information

#pragma once

#include "cartesian.hpp"
#include "eng_units.hpp"

namespace lob {

// Generic implementation of Euler's method
template <typename T, typename Y, typename F>
Y EulerStep(const T& t_i, const Y& y_i, T dt, const F& f) {
  return y_i + f(t_i, y_i) * dt;
}

// Generic implementation of Heun's method
template <typename T, typename Y, typename F>
Y HeunStep(const T& t_i, const Y& y_i, T dt, const F& f) {
  const T kQuanta = dt / 2;
  const Y k1 = f(t_i, y_i);
  const Y k2 = f(t_i + dt, y_i + k1 * dt);
  return y_i + (k1 + k2) * kQuanta;
}

// Generic implementation of fourth order Runge-Kutta method
template <typename T, typename Y, typename F>
Y RungeKuttaStep(const T& t_i, const Y& y_i, T dt, const F& f) {
  const T kHalfStep = dt / 2;
  const T kQuanta = dt / 6;
  const Y k1 = f(t_i, y_i);
  const Y k2 = f(t_i + kHalfStep, y_i + k1 * kHalfStep);
  const Y k3 = f(t_i + kHalfStep, y_i + k2 * kHalfStep);
  const Y k4 = f(t_i + dt, y_i + k3 * dt);
  return y_i + (k1 + k2 * 2 + k3 * 2 + k4) * kQuanta;
}

// Numerical method friendly container for velocity and posiiton
struct SpvT {
  SpvT();
  SpvT(CartesianT<FeetT> p, CartesianT<FpsT> v);
  SpvT(const SpvT& other);
  SpvT(SpvT&& other) noexcept;
  ~SpvT() = default;
  SpvT& operator=(const SpvT& rhs);
  SpvT& operator=(SpvT&& rhs) noexcept;

  SpvT operator+(const SpvT& rhs) const;
  SpvT operator+(const double& rhs) const;
  SpvT operator*(const SpvT& rhs) const;
  SpvT operator*(const double& rhs) const;

  CartesianT<FeetT> P() const;
  void P(FeetT input);
  void P(double input);
  CartesianT<FpsT> V() const;
  void V(FpsT input);
  void V(double input);

  CartesianT<FeetT> position;
  CartesianT<FpsT> velocity;
};

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