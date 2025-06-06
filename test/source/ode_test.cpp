// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include "ode.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <memory>

#include "cartesian.hpp"
#include "eng_units.hpp"

namespace tests {

TEST(OdeTests, EulerStep) {
  const double kY0 = 1.0;
  const double kT0 = 0.0;
  const double kTFinal = 5.0;
  const double kDT = 0.001;

  // This is ordinary differential equation to solve
  auto ode = [](double t, double y) { return std::pow(std::sin(t), 2) * y; };

  // The solved ode will provide an expected solution
  auto y_exact = [](double t_0, double y_0, double t) {
    auto exponent = ((t - t_0) - (std::sin(t) * std::cos(t) -
                                  std::sin(t_0) * std::cos(t_0))) /
                    2;
    return y_0 * std::exp(exponent);
  };

  double y = kY0;
  double t = kT0;
  const double kError = 0.1;
  while (t < kTFinal) {
    y = lob::EulerStep(t, y, kDT, ode);
    t += kDT;
    const double kExpectedSolution = y_exact(kT0, kY0, t);
    EXPECT_NEAR(y, kExpectedSolution, kError);
  }
}

TEST(OdeTests, HeunStep) {
  const double kY0 = 1.0;
  const double kT0 = 0.0;
  const double kTFinal = 5.0;
  const double kDT = 0.1;

  // This is ordinary differential equation to solve
  auto ode = [](double t, double y) { return std::pow(std::sin(t), 2) * y; };

  // The solved ode will provide an expected solution
  auto y_exact = [](double t_0, double y_0, double t) {
    auto exponent = ((t - t_0) - (std::sin(t) * std::cos(t) -
                                  std::sin(t_0) * std::cos(t_0))) /
                    2;
    return y_0 * std::exp(exponent);
  };

  double y = kY0;
  double t = kT0;
  const double kError = 0.1;
  while (t < kTFinal) {
    y = lob::HeunStep(t, y, kDT, ode);
    t += kDT;
    const double kExpectedSolution = y_exact(kT0, kY0, t);
    EXPECT_NEAR(y, kExpectedSolution, kError);
  }
}

TEST(OdeTests, RungeKuttaStep) {
  const double kY0 = 1.0;
  const double kT0 = 0.0;
  const double kTFinal = 5.0;
  const double kDT = 0.5;

  // This is ordinary differential equation to solve
  auto ode = [](double t, double y) { return std::pow(std::sin(t), 2) * y; };

  // The solved ode will provide an expected solution
  auto y_exact = [](double t_0, double y_0, double t) {
    auto exponent = ((t - t_0) - (std::sin(t) * std::cos(t) -
                                  std::sin(t_0) * std::cos(t_0))) /
                    2;
    return y_0 * std::exp(exponent);
  };

  double y = kY0;
  double t = kT0;
  const double kError = 0.1;
  while (t < kTFinal) {
    y = lob::RungeKuttaStep(t, y, kDT, ode);
    t += kDT;
    const double kExpectedSolution = y_exact(kT0, kY0, t);
    EXPECT_NEAR(y, kExpectedSolution, kError);
  }
}

TEST(OdeTests, TrajectoryStateTConstrutor) {
  const auto kP =
      lob::CartesianT<lob::FeetT>(lob::FeetT(3), lob::FeetT(4), lob::FeetT(0));
  const auto kV =
      lob::CartesianT<lob::FpsT>(lob::FpsT(3), lob::FpsT(4), lob::FpsT(0));
  std::unique_ptr<lob::TrajectoryStateT> pa = nullptr;
  std::unique_ptr<lob::TrajectoryStateT> pb = nullptr;
  pa = std::make_unique<lob::TrajectoryStateT>();
  pb = std::make_unique<lob::TrajectoryStateT>(kP, kV);
  ASSERT_NE(pa.get(), nullptr);
  ASSERT_NE(pb.get(), nullptr);
  pa.reset();
  pb.reset();
}

TEST(OdeTests, TrajectoryStateTCopyAssignment) {
  const auto kP =
      lob::CartesianT<lob::FeetT>(lob::FeetT(3), lob::FeetT(4), lob::FeetT(0));
  const auto kV =
      lob::CartesianT<lob::FpsT>(lob::FpsT(3), lob::FpsT(4), lob::FpsT(0));
  lob::TrajectoryStateT a = lob::TrajectoryStateT(kP, kV);
  EXPECT_DOUBLE_EQ(a.P().Magnitude().Value(), 5.0);
}

TEST(OdeTests, Addition) {
  const auto kP =
      lob::CartesianT<lob::FeetT>(lob::FeetT(3), lob::FeetT(4), lob::FeetT(0));
  const auto kV =
      lob::CartesianT<lob::FpsT>(lob::FpsT(3), lob::FpsT(4), lob::FpsT(0));
  lob::TrajectoryStateT a = lob::TrajectoryStateT(kP, kV);
  lob::TrajectoryStateT b;
  b = b + lob::SecT(1);
  EXPECT_DOUBLE_EQ(b.P().X().Value(), 1.0);
  a = a + b;
  EXPECT_DOUBLE_EQ(a.P().X().Value(), kP.X().Value() + b.P().X().Value());
}

TEST(OdeTests, Multiplication) {
  const auto kP =
      lob::CartesianT<lob::FeetT>(lob::FeetT(3), lob::FeetT(4), lob::FeetT(0));
  const auto kV =
      lob::CartesianT<lob::FpsT>(lob::FpsT(3), lob::FpsT(4), lob::FpsT(0));
  lob::TrajectoryStateT a = lob::TrajectoryStateT(kP, kV);
  a = a * lob::SecT(2);
  EXPECT_DOUBLE_EQ(a.P().X().Value(), kP.X().Value() * 2);
}

TEST(OdeTests, Input) {
  const double kA = 3.0;
  const double kB = 4.0;
  const auto kP = lob::CartesianT<lob::FeetT>(lob::FeetT(kA), lob::FeetT(kB),
                                              lob::FeetT(0));
  const auto kV =
      lob::CartesianT<lob::FpsT>(lob::FpsT(kA), lob::FpsT(kB), lob::FpsT(0));
  lob::TrajectoryStateT a = lob::TrajectoryStateT(kP, kV);
  lob::TrajectoryStateT b = lob::TrajectoryStateT(kP, kV);
  a.P(lob::FeetT(kB));
  a.V(lob::FpsT(kA));
  b.P(kB);
  b.V(kA);
  EXPECT_DOUBLE_EQ(a.P().X().Value(), kB);
  EXPECT_DOUBLE_EQ(a.V().X().Value(), kA);
  EXPECT_DOUBLE_EQ(b.P().X().Value(), kB);
  EXPECT_DOUBLE_EQ(b.V().X().Value(), kA);
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