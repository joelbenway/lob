// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include <benchmark/benchmark.h>

#include <cmath>
#include <cstddef>

#include "ode.hpp"

namespace benchmark {
namespace {

// 2D point-mass ballistic state: position (ft) and velocity (fps)
struct Ball2 {
  double x, y, vx, vy;
  Ball2 operator+(const Ball2& r) const {
    return {x + r.x, y + r.y, vx + r.vx, vy + r.vy};
  }
  Ball2 operator*(double s) const {
    return {x * s, y * s, vx * s, vy * s};
  }
};

constexpr double kG = 32.174;
constexpr double kDrag = 1e-3;
constexpr Ball2 kInit{0.0, 0.0, 3000.0, 0.0};
constexpr double kTFinal = 1.0;

auto MakeBallisticOde() {
  return [](double /*t*/, const Ball2& s) -> Ball2 {
    const double kV = std::sqrt((s.vx * s.vx) + (s.vy * s.vy));
    const double kAx = -(kDrag * kV * s.vx);
    const double kAy = -(kG + (kDrag * kV * s.vy));
    return {s.vx, s.vy, kAx, kAy};
  };
}

// Reference: RK4 at dt=1e-5 (100k steps), computed once at startup
Ball2 Reference() {
  constexpr double kDt = 1e-5;
  constexpr auto kSteps = static_cast<size_t>(kTFinal / kDt);
  Ball2 y = kInit;
  double t = 0.0;
  for (size_t i = 0; i < kSteps; ++i) {
    auto ode = MakeBallisticOde();
    y = lob::RungeKuttaStep<double, Ball2, decltype(ode)>(t, y, kDt, ode);
    t += kDt;
  }
  return y;
}
const Ball2& Ref() {
  static const Ball2 kRef = Reference();
  return kRef;
}

void EulerStepBM(benchmark::State& state) {
  constexpr double kDt = 1e-4;
  constexpr auto kStep = static_cast<size_t>(kTFinal / kDt);
  for (auto _ : state) {
    auto ode = MakeBallisticOde();
    Ball2 y = kInit;
    double t = 0.0;
    for (size_t i = 0; i < kStep; ++i) {
      y = lob::EulerStep(t, y, kDt, ode);
      t += kDt;
    }
    benchmark::DoNotOptimize(y);
    const auto& ref = Ref();
    const double kErr = std::hypot(y.x - ref.x, y.y - ref.y);
    state.counters["error_ft"] = kErr;
  }
}

void HeunStepBM(benchmark::State& state) {
  constexpr double kDt = 1e-3;
  constexpr auto kStep = static_cast<size_t>(kTFinal / kDt);
  for (auto _ : state) {
    auto ode = MakeBallisticOde();
    Ball2 y = kInit;
    double t = 0.0;
    for (size_t i = 0; i < kStep; ++i) {
      y = lob::HeunStep(t, y, kDt, ode);
      t += kDt;
    }
    benchmark::DoNotOptimize(y);
    const auto& ref = Ref();
    const double kErr = std::hypot(y.x - ref.x, y.y - ref.y);
    state.counters["error_ft"] = kErr;
  }
}

void RungeKuttaStepBM(benchmark::State& state) {
  constexpr double kDt = 1e-2;
  constexpr auto kStep = static_cast<size_t>(kTFinal / kDt);
  for (auto _ : state) {
    auto ode = MakeBallisticOde();
    Ball2 y = kInit;
    double t = 0.0;
    for (size_t i = 0; i < kStep; ++i) {
      y = lob::RungeKuttaStep(t, y, kDt, ode);
      t += kDt;
    }
    benchmark::DoNotOptimize(y);
    const auto& ref = Ref();
    const double kErr = std::hypot(y.x - ref.x, y.y - ref.y);
    state.counters["error_ft"] = kErr;
  }
}

}  // namespace

BENCHMARK(EulerStepBM);
BENCHMARK(HeunStepBM);
BENCHMARK(RungeKuttaStepBM);

}  // namespace benchmark

BENCHMARK_MAIN();

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
