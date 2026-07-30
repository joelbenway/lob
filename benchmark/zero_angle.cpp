// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include <benchmark/benchmark.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

#include "lob/lob.hpp"
#include "cartesian.hpp"
#include "constants.hpp"
#include "eng_units.hpp"
#include "helpers.hpp"
#include "ode.hpp"
#include "solve_step.hpp"
#include "splines.hpp"

namespace benchmark {
namespace {

using lob::CartesianT;
using lob::DegreesT;
using lob::FeetT;
using lob::FpsT;
using lob::InchT;
using lob::MoaT;
using lob::NaN;
using lob::RadiansT;
using lob::SecT;
using lob::SolveStep;
using lob::TrajectoryStateT;
using lob::convert::kInchPerFoot;
using lob::kStandardGravityFtPerSecSq;
using lob::spline::CurveView;
using lob::spline::kKnots;

constexpr double kBC = 0.436;
constexpr uint16_t kVelocity = 3100U;
constexpr double kZeroHeight = 3.0;
constexpr double kZero100 = 100.0;
constexpr double kZero500 = 500.0;
constexpr double kZero1000 = 1000.0;
constexpr double kZeroAnglePreset = 6.11;

constexpr RadiansT kZeroAngleError = MoaT(0.01);
constexpr RadiansT kMaxZeroAngle = DegreesT(45);
constexpr RadiansT kMinZeroAngle = DegreesT(-45.0);

FeetT FireToRange(::LobContext* ctx, FeetT zero_distance_ft,
                  RadiansT launch_angle, int* count) {
  ++(*count);
  const FpsT kVel = FpsT(ctx->velocity);
  TrajectoryStateT s(
      CartesianT<FeetT>(FeetT(0.0)),
      CartesianT<FpsT>(kVel * std::cos(launch_angle.Value()),
                       kVel * std::sin(launch_angle.Value()), FpsT(0.0)));
  CurveView zero_drag_curve(kKnots.data(), static_cast<const float*>(ctx->drags));
  while (s.P().X() < zero_distance_ft) {
    if (s.V().X() <= FpsT(0)) {
      ctx->error = kLobErrorInternalError;
      return FeetT(0);
    }
    SolveStep(*ctx, &s, &zero_drag_curve, FeetT(zero_distance_ft));
  }
  return s.P().Y();
}

bool HasFatalError(const ::LobContext* ctx) {
  return ctx->error != kLobErrorNone && ctx->error != kLobErrorNotFormed;
}

FeetT FireToTarget(::LobContext* ctx, FeetT zero_distance_ft,
                   RadiansT theta, FeetT target_height,
                   double aero_jump, int* count) {
  ctx->error = kLobErrorNone;
  const RadiansT kLaunch = RadiansT(theta.Value() + aero_jump);
  return FireToRange(ctx, zero_distance_ft, kLaunch, count) - target_height;
}

RadiansT SecantStep(RadiansT theta, RadiansT theta_prev, FeetT f,
                    FeetT f_prev, RadiansT lo, RadiansT hi) {
  constexpr double kEpsilon = 1e-10;
  const RadiansT kDTheta = theta - theta_prev;
  const FeetT kDF = f - f_prev;
  if (std::abs(kDF.Value()) < kEpsilon) {
    return (lo + hi) / 2;
  }
  const RadiansT kNext =
      theta - RadiansT(f.Value() / (kDF.Value() / kDTheta.Value()));
  if (kNext < lo || kNext > hi || std::isnan(kNext.Value())) {
    return (lo + hi) / 2;
  }
  return kNext;
}

void NarrowBracket(RadiansT* lo, RadiansT* hi, RadiansT theta_prev,
                   FeetT f_prev, FeetT flo, FeetT fhi) {
  if (flo.Value() < fhi.Value()) {
    if (f_prev.Value() < flo.Value()) {
      *lo = theta_prev;
    } else if (f_prev.Value() > fhi.Value()) {
      *hi = theta_prev;
    }
  } else {
    if (f_prev.Value() < fhi.Value()) {
      *hi = theta_prev;
    } else if (f_prev.Value() > flo.Value()) {
      *lo = theta_prev;
    }
  }
}

double SearchBinary(::LobContext* ctx, FeetT zero_distance_ft,
                    FeetT target_height, int* count) {
  const double kAeroJump = RadiansT(MoaT(ctx->aerodynamic_jump)).Value();
  double lo = kMinZeroAngle.Value();
  double hi = kMaxZeroAngle.Value();
  while (hi - lo > kZeroAngleError.Value()) {
    const double kMid = (lo + hi) / 2.0;
    const FeetT kImpact = FireToTarget(ctx, zero_distance_ft,
                                       RadiansT(kMid), target_height,
                                       kAeroJump, count);
    if (HasFatalError(ctx)) {
      return std::numeric_limits<double>::quiet_NaN();
    }
    if (kImpact < target_height) {
      lo = kMid;
    } else {
      hi = kMid;
    }
  }
  return (lo + hi) / 2;
}

double SearchHuman(::LobContext* ctx, FeetT zero_distance_ft,
                   FeetT target_height, int* count) {
  const double kVSq =
      static_cast<double>(ctx->velocity) * static_cast<double>(ctx->velocity);
  const double kRawSeed =
      kStandardGravityFtPerSecSq * zero_distance_ft.Value() / kVSq;
  const double kClampedSeed =
      std::max(kMinZeroAngle.Value(),
               std::min(kMaxZeroAngle.Value(), kRawSeed));
  const RadiansT kThetaSeed = RadiansT(kClampedSeed);
  const double kAeroJump = RadiansT(MoaT(ctx->aerodynamic_jump)).Value();

  RadiansT theta_prev = kThetaSeed;
  FeetT f_prev = FireToTarget(ctx, zero_distance_ft, theta_prev, target_height,
                              kAeroJump, count);
  if (HasFatalError(ctx)) {
    return NaN();
  }

  constexpr size_t kMaxIterations = 10;
  for (size_t iter = 0; iter < kMaxIterations; ++iter) {
    const RadiansT kDTheta =
        RadiansT(-(f_prev.Value() / zero_distance_ft.Value()));
    const RadiansT kThetaNext = theta_prev + kDTheta;
    if (kThetaNext < kMinZeroAngle || kThetaNext > kMaxZeroAngle ||
        std::isnan(kThetaNext.Value())) {
      return NaN();
    }
    if (std::abs((kThetaNext - theta_prev).Value()) <=
        kZeroAngleError.Value()) {
      return MoaT(kThetaNext).Value();
    }
    theta_prev = kThetaNext;
    f_prev = FireToTarget(ctx, zero_distance_ft, theta_prev, target_height,
                           kAeroJump, count);
    if (HasFatalError(ctx)) {
      return NaN();
    }
  }
  return NaN();
}

void EnsureBracketEval(::LobContext* ctx, FeetT zero_distance_ft,
                       FeetT target_height, double aero_jump, int* count,
                       FeetT* f_val, RadiansT angle) {
  if (!std::isnan(*f_val)) {
    return;
  }
  *f_val = FireToTarget(ctx, zero_distance_ft, angle, target_height, aero_jump,
                       count);
}

double SearchSecant(::LobContext* ctx, FeetT zero_distance_ft,
                    FeetT target_height, int* count) {
  const double kVSq =
      static_cast<double>(ctx->velocity) * static_cast<double>(ctx->velocity);
  const double kRawSeed =
      kStandardGravityFtPerSecSq * zero_distance_ft.Value() / kVSq;
  const double kClampedSeed =
      std::max(kMinZeroAngle.Value(),
               std::min(kMaxZeroAngle.Value(), kRawSeed));
  const RadiansT kThetaSeed = RadiansT(kClampedSeed);
  const double kAeroJump = RadiansT(MoaT(ctx->aerodynamic_jump)).Value();

  RadiansT lo = kMinZeroAngle;
  RadiansT hi = kMaxZeroAngle;
  FeetT flo = FeetT(NaN());
  FeetT fhi = FeetT(NaN());

  RadiansT theta_prev = kThetaSeed;
  FeetT f_prev = FireToTarget(ctx, zero_distance_ft, theta_prev,
                              target_height, kAeroJump, count);
  if (HasFatalError(ctx)) {
    return NaN();
  }

  constexpr RadiansT kDeltaSeed = MoaT(0.1);
  RadiansT theta = theta_prev + kDeltaSeed;
  FeetT f = FireToTarget(ctx, zero_distance_ft, theta, target_height,
                         kAeroJump, count);
  if (HasFatalError(ctx)) {
    return NaN();
  }

  constexpr size_t kMaxIterations = 5;
  for (size_t iter = 0; iter < kMaxIterations; ++iter) {
    RadiansT theta_next = SecantStep(theta, theta_prev, f, f_prev, lo, hi);

    if (theta_next < lo || theta_next > hi || std::isnan(theta_next)) {
      EnsureBracketEval(ctx, zero_distance_ft, target_height, kAeroJump,
                        count, &flo, RadiansT(lo.Value()));
      if (HasFatalError(ctx)) {
    return NaN();
  }
      EnsureBracketEval(ctx, zero_distance_ft, target_height, kAeroJump,
                        count, &fhi, RadiansT(hi.Value()));
      if (HasFatalError(ctx)) {
    return NaN();
  }
      theta_next = (lo + hi) / 2;
    }

    if (std::abs((theta_next - theta).Value()) <= kZeroAngleError.Value()) {
      return MoaT(theta_next).Value();
    }

    NarrowBracket(&lo, &hi, theta_prev, f_prev, flo, fhi);

    theta_prev = theta;
    f_prev = f;
    theta = theta_next;
    f = FireToTarget(ctx, zero_distance_ft, theta, target_height,
                     kAeroJump, count);
    if (HasFatalError(ctx)) {
    return NaN();
  }
  }

  return NaN();
}

double SearchRidders(::LobContext* ctx, FeetT zero_distance_ft,
                     FeetT target_height, int* count) {
  const double kAeroJump = RadiansT(MoaT(ctx->aerodynamic_jump)).Value();
  auto fired_angle = [&](RadiansT za) {
    return RadiansT(za.Value() + kAeroJump);
  };
  auto f_eval = [&](double theta_rad) {
    ctx->error = kLobErrorNone;
    const FeetT kImpact = FireToRange(ctx, zero_distance_ft,
                                      fired_angle(RadiansT(theta_rad)), count);
    return kImpact.Value() - target_height.Value();
  };

  double x1 = kMinZeroAngle.Value();
  double x2 = kMaxZeroAngle.Value();
  double f1 = f_eval(x1);
  if (ctx->error != kLobErrorNone && ctx->error != kLobErrorNotFormed) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  double f2 = f_eval(x2);
  if (ctx->error != kLobErrorNone && ctx->error != kLobErrorNotFormed) {
    return std::numeric_limits<double>::quiet_NaN();
  }

  constexpr size_t kMaxIter = 20;
  for (size_t i = 0; i < kMaxIter; ++i) {
    const double kXm = (x1 + x2) / 2.0;
    const double kFm = f_eval(kXm);
    if (ctx->error != kLobErrorNone && ctx->error != kLobErrorNotFormed) {
      return std::numeric_limits<double>::quiet_NaN();
    }

    const double kS = std::sqrt((kFm * kFm) - (f1 * f2));
    double x_new = {};
    if (lob::AreEqual(kS, 0)) {
      x_new = kXm;
    } else {
      x_new = kXm + (kXm - x1) * std::copysign(1.0, f1 - f2) * kFm / kS;
    }

    const double kFNew = f_eval(x_new);
    if (ctx->error != kLobErrorNone && ctx->error != kLobErrorNotFormed) {
      return std::numeric_limits<double>::quiet_NaN();
    }

    if (std::abs(x2 - x1) <= kZeroAngleError.Value()) {
      return x_new;
    }

    if (f1 * kFNew < 0.0) {
      x2 = x_new;
      f2 = kFNew;
    } else if (f2 * kFNew < 0.0) {
      x1 = x_new;
      f1 = kFNew;
    } else {
      x1 = kXm;
      f1 = kFm;
      x2 = x_new;
      f2 = kFNew;
    }
  }
  return (x1 + x2) / 2;
}

lob::Context BuildPresetContext(double zero_distance_yds) {
  return lob::Builder()
      .BallisticCoefficientPsi(kBC)
      .BCAtmosphere(lob::AtmosphereReferenceT::kArmyStandardMetro)
      .InitialVelocityFps(kVelocity)
      .ZeroDistanceYds(zero_distance_yds)
      .ZeroImpactHeightInches(kZeroHeight)
      .ZeroAngleMOA(kZeroAnglePreset)
      .Build();
}

FeetT TargetHeight(const ::LobContext& ctx, double impact_height_inches) {
  return FeetT(ctx.optic_height + (impact_height_inches / kInchPerFoot));
}

void ZeroSearchPreset(benchmark::State& state) {
  for (auto _ : state) {
    const lob::Context kResult = BuildPresetContext(kZero100);
    benchmark::DoNotOptimize(&kResult);
  }
}

void SearchBinary100(benchmark::State& state) {
  const lob::Context kBase = BuildPresetContext(kZero100);
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  const auto* base_ctx = reinterpret_cast<const ::LobContext*>(&kBase);
  const FeetT kZeroDist = FeetT(kZero100 * 3.0);
  const FeetT kTargetH = TargetHeight(*base_ctx, kZeroHeight);
  for (auto _ : state) {
    ::LobContext ctx = *base_ctx;
    int count = 0;
    double angle = SearchBinary(&ctx, kZeroDist, kTargetH, &count);
    benchmark::DoNotOptimize(angle);
    state.counters["trajectories"] += count;
  }
}

void SearchBinary500(benchmark::State& state) {
  const lob::Context kBase = BuildPresetContext(kZero500);
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  const auto* base_ctx = reinterpret_cast<const ::LobContext*>(&kBase);
  const FeetT kZeroDist = FeetT(kZero500 * 3.0);
  const FeetT kTargetH = TargetHeight(*base_ctx, kZeroHeight);
  for (auto _ : state) {
    ::LobContext ctx = *base_ctx;
    int count = 0;
    double angle = SearchBinary(&ctx, kZeroDist, kTargetH, &count);
    benchmark::DoNotOptimize(angle);
    state.counters["trajectories"] += count;
  }
}

void SearchBinary1000(benchmark::State& state) {
  const lob::Context kBase = BuildPresetContext(kZero1000);
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  const auto* base_ctx = reinterpret_cast<const ::LobContext*>(&kBase);
  const FeetT kZeroDist = FeetT(kZero1000 * 3.0);
  const FeetT kTargetH = TargetHeight(*base_ctx, kZeroHeight);
  for (auto _ : state) {
    ::LobContext ctx = *base_ctx;
    int count = 0;
    double angle = SearchBinary(&ctx, kZeroDist, kTargetH, &count);
    benchmark::DoNotOptimize(angle);
    state.counters["trajectories"] += count;
  }
}

void SearchSecant100(benchmark::State& state) {
  const lob::Context kBase = BuildPresetContext(kZero100);
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  const auto* base_ctx = reinterpret_cast<const ::LobContext*>(&kBase);
  const FeetT kZeroDist = FeetT(kZero100 * 3.0);
  const FeetT kTargetH = TargetHeight(*base_ctx, kZeroHeight);
  for (auto _ : state) {
    ::LobContext ctx = *base_ctx;
    int count = 0;
    double angle = SearchSecant(&ctx, kZeroDist, kTargetH, &count);
    benchmark::DoNotOptimize(angle);
    state.counters["trajectories"] += count;
  }
}

void SearchSecant500(benchmark::State& state) {
  const lob::Context kBase = BuildPresetContext(kZero500);
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  const auto* base_ctx = reinterpret_cast<const ::LobContext*>(&kBase);
  const FeetT kZeroDist = FeetT(kZero500 * 3.0);
  const FeetT kTargetH = TargetHeight(*base_ctx, kZeroHeight);
  for (auto _ : state) {
    ::LobContext ctx = *base_ctx;
    int count = 0;
    double angle = SearchSecant(&ctx, kZeroDist, kTargetH, &count);
    benchmark::DoNotOptimize(angle);
    state.counters["trajectories"] += count;
  }
}

void SearchSecant1000(benchmark::State& state) {
  const lob::Context kBase = BuildPresetContext(kZero1000);
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  const auto* base_ctx = reinterpret_cast<const ::LobContext*>(&kBase);
  const FeetT kZeroDist = FeetT(kZero1000 * 3.0);
  const FeetT kTargetH = TargetHeight(*base_ctx, kZeroHeight);
  for (auto _ : state) {
    ::LobContext ctx = *base_ctx;
    int count = 0;
    double angle = SearchSecant(&ctx, kZeroDist, kTargetH, &count);
    benchmark::DoNotOptimize(angle);
    state.counters["trajectories"] += count;
  }
}

void SearchHuman100(benchmark::State& state) {
  const lob::Context kBase = BuildPresetContext(kZero100);
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  const auto* base_ctx = reinterpret_cast<const ::LobContext*>(&kBase);
  const FeetT kZeroDist = FeetT(kZero100 * 3.0);
  const FeetT kTargetH = TargetHeight(*base_ctx, kZeroHeight);
  for (auto _ : state) {
    ::LobContext ctx = *base_ctx;
    int count = 0;
    double angle = SearchHuman(&ctx, kZeroDist, kTargetH, &count);
    benchmark::DoNotOptimize(angle);
    state.counters["trajectories"] += count;
  }
}

void SearchHuman500(benchmark::State& state) {
  const lob::Context kBase = BuildPresetContext(kZero500);
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  const auto* base_ctx = reinterpret_cast<const ::LobContext*>(&kBase);
  const FeetT kZeroDist = FeetT(kZero500 * 3.0);
  const FeetT kTargetH = TargetHeight(*base_ctx, kZeroHeight);
  for (auto _ : state) {
    ::LobContext ctx = *base_ctx;
    int count = 0;
    double angle = SearchHuman(&ctx, kZeroDist, kTargetH, &count);
    benchmark::DoNotOptimize(angle);
    state.counters["trajectories"] += count;
  }
}

void SearchHuman1000(benchmark::State& state) {
  const lob::Context kBase = BuildPresetContext(kZero1000);
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  const auto* base_ctx = reinterpret_cast<const ::LobContext*>(&kBase);
  const FeetT kZeroDist = FeetT(kZero1000 * 3.0);
  const FeetT kTargetH = TargetHeight(*base_ctx, kZeroHeight);
  for (auto _ : state) {
    ::LobContext ctx = *base_ctx;
    int count = 0;
    double angle = SearchHuman(&ctx, kZeroDist, kTargetH, &count);
    benchmark::DoNotOptimize(angle);
    state.counters["trajectories"] += count;
  }
}

void SearchRidders100(benchmark::State& state) {
  const lob::Context kBase = BuildPresetContext(kZero100);
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  const auto* base_ctx = reinterpret_cast<const ::LobContext*>(&kBase);
  const FeetT kZeroDist = FeetT(kZero100 * 3.0);
  const FeetT kTargetH = TargetHeight(*base_ctx, kZeroHeight);
  for (auto _ : state) {
    ::LobContext ctx = *base_ctx;
    int count = 0;
    double angle = SearchRidders(&ctx, kZeroDist, kTargetH, &count);
    benchmark::DoNotOptimize(angle);
    state.counters["trajectories"] += count;
  }
}

void SearchRidders500(benchmark::State& state) {
  const lob::Context kBase = BuildPresetContext(kZero500);
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  const auto* base_ctx = reinterpret_cast<const ::LobContext*>(&kBase);
  const FeetT kZeroDist = FeetT(kZero500 * 3.0);
  const FeetT kTargetH = TargetHeight(*base_ctx, kZeroHeight);
  for (auto _ : state) {
    ::LobContext ctx = *base_ctx;
    int count = 0;
    double angle = SearchRidders(&ctx, kZeroDist, kTargetH, &count);
    benchmark::DoNotOptimize(angle);
    state.counters["trajectories"] += count;
  }
}

void SearchRidders1000(benchmark::State& state) {
  const lob::Context kBase = BuildPresetContext(kZero1000);
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  const auto* base_ctx = reinterpret_cast<const ::LobContext*>(&kBase);
  const FeetT kZeroDist = FeetT(kZero1000 * 3.0);
  const FeetT kTargetH = TargetHeight(*base_ctx, kZeroHeight);
  for (auto _ : state) {
    ::LobContext ctx = *base_ctx;
    int count = 0;
    double angle = SearchRidders(&ctx, kZeroDist, kTargetH, &count);
    benchmark::DoNotOptimize(angle);
    state.counters["trajectories"] += count;
  }
}

}  // namespace

BENCHMARK(ZeroSearchPreset);
BENCHMARK(SearchBinary100);
BENCHMARK(SearchBinary500);
BENCHMARK(SearchBinary1000);
BENCHMARK(SearchSecant100);
BENCHMARK(SearchSecant500);
BENCHMARK(SearchSecant1000);
BENCHMARK(SearchHuman100);
BENCHMARK(SearchHuman500);
BENCHMARK(SearchHuman1000);
BENCHMARK(SearchRidders100);
BENCHMARK(SearchRidders500);
BENCHMARK(SearchRidders1000);

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
