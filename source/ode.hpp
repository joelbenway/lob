// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#pragma once

#include "cartesian.hpp"
#include "eng_units.hpp"

namespace lob {

// Generic implementation of Euler's method
template <typename T, typename Y, typename F>
constexpr Y EulerStep(const T& t_i, const Y& y_i, T dt, const F& f) {
  return y_i + (f(t_i, y_i) * dt);
}

// Generic implementation of Heun's method
template <typename T, typename Y, typename F>
constexpr Y HeunStep(const T& t_i, const Y& y_i, T dt, const F& f) {
  const T kQuanta = dt / 2;
  const Y k1 = f(t_i, y_i);
  const Y k2 = f(t_i + dt, y_i + (k1 * dt));
  return y_i + ((k1 + k2) * kQuanta);
}

// Generic implementation of fourth order Runge-Kutta method
template <typename T, typename Y, typename F>
constexpr Y RungeKuttaStep(const T& t_i, const Y& y_i, T dt, const F& f) {
  const T kHalfStep = dt / 2;
  const T kQuanta = dt / 6;
  const T kDouble = static_cast<T>(2);
  const Y k1 = f(t_i, y_i);
  const Y k2 = f(t_i + kHalfStep, y_i + (k1 * kHalfStep));
  const Y k3 = f(t_i + kHalfStep, y_i + (k2 * kHalfStep));
  const Y k4 = f(t_i + dt, y_i + (k3 * dt));
  return y_i + ((k1 + k2 * kDouble + k3 * kDouble + k4) * kQuanta);
}

// Numerical method friendly container for velocity, position, and time of
// flight
class TrajectoryStateT {
 public:
  constexpr TrajectoryStateT() : time_of_flight_(SecT(0)) {}

  constexpr TrajectoryStateT(CartesianT<FeetT> p, CartesianT<FpsT> v,
                             SecT tof = SecT(0))
      : position_(std::move(p)),
        velocity_(std::move(v)),
        time_of_flight_(std::move(tof)) {}

  constexpr TrajectoryStateT(const TrajectoryStateT&) = default;
  constexpr TrajectoryStateT(TrajectoryStateT&&) noexcept = default;
  constexpr TrajectoryStateT& operator=(const TrajectoryStateT&) = default;
  constexpr TrajectoryStateT& operator=(TrajectoryStateT&&) noexcept = default;
  ~TrajectoryStateT() = default;

  constexpr TrajectoryStateT operator+(const TrajectoryStateT& rhs) const {
    return TrajectoryStateT{position_ + rhs.position_,
                            velocity_ + rhs.velocity_,
                            time_of_flight_ + rhs.time_of_flight_};
  }

  constexpr TrajectoryStateT operator-(const TrajectoryStateT& rhs) const {
    return TrajectoryStateT{position_ - rhs.position_,
                            velocity_ - rhs.velocity_,
                            time_of_flight_ - rhs.time_of_flight_};
  }

  template <typename T>
  constexpr TrajectoryStateT operator+(const T& rhs) const {
    return TrajectoryStateT{position_ + FeetT(ToDouble(rhs)),
                            velocity_ + FpsT(ToDouble(rhs)),
                            time_of_flight_ + SecT(ToDouble(rhs))};
  }

  template <typename T>
  constexpr TrajectoryStateT operator-(const T& rhs) const {
    return TrajectoryStateT{position_ - FeetT(ToDouble(rhs)),
                            velocity_ - FpsT(ToDouble(rhs)),
                            time_of_flight_ - SecT(ToDouble(rhs))};
  }

  template <typename T>
  constexpr TrajectoryStateT operator*(const T& rhs) const {
    return TrajectoryStateT{position_ * FeetT(ToDouble(rhs)),
                            velocity_ * FpsT(ToDouble(rhs)),
                            time_of_flight_ * ToDouble(rhs)};
  }

  template <typename T>
  constexpr TrajectoryStateT operator/(const T& rhs) const {
    return TrajectoryStateT{position_ / FeetT(ToDouble(rhs)),
                            velocity_ / FpsT(ToDouble(rhs)),
                            time_of_flight_ / ToDouble(rhs)};
  }

  template <typename T>
  friend constexpr TrajectoryStateT operator+(const T& lhs,
                                              const TrajectoryStateT& rhs) {
    return rhs + lhs;
  }

  template <typename T>
  friend constexpr TrajectoryStateT operator*(const T& lhs,
                                              const TrajectoryStateT& rhs) {
    return rhs * lhs;
  }

  constexpr CartesianT<FeetT> P() const { return position_; }
  constexpr void P(FeetT input) { position_ = CartesianT<FeetT>(input); }
  constexpr void P(double input) {
    position_ = CartesianT<FeetT>(FeetT(input));
  }

  constexpr CartesianT<FpsT> V() const { return velocity_; }
  constexpr void V(FpsT input) { velocity_ = CartesianT<FpsT>(input); }
  constexpr void V(double input) { velocity_ = CartesianT<FpsT>(FpsT(input)); }

  constexpr SecT TOF() const { return time_of_flight_; }
  constexpr void TOF(SecT input) { time_of_flight_ = input; }

 private:
  struct PriorityLow {};
  struct PriorityHigh : PriorityLow {};

  template <typename T>
  static constexpr auto ToDoubleImpl(const T& val, PriorityHigh tag) noexcept
      -> decltype(val.Value()) {
    (void)tag;
    return val.Value();
  }

  template <typename T>
  static constexpr double ToDoubleImpl(const T& val, PriorityLow tag) noexcept {
    (void)tag;
    return static_cast<double>(val);
  }

  template <typename T>
  static constexpr double ToDouble(const T& val) noexcept {
    return static_cast<double>(ToDoubleImpl(val, PriorityHigh{}));
  }

  CartesianT<FeetT> position_;
  CartesianT<FpsT> velocity_;
  SecT time_of_flight_;
};

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