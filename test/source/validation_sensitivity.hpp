// Copyright (c) 2026  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

// Test-only sensitivity math (Phase 2). Pure functions, no solver contact:
// unit-testable on analytic functions. C++14, stdlib only.

#pragma once

#include <array>
#include <cmath>

namespace tests {

struct DiffResult {
  double deriv = 0.0;
  double f_plus = 0.0;
  double f_minus = 0.0;
};

template <typename F>
DiffResult CentralDifference(F f, double x, double h) {
  constexpr double kTwo = 2.0;
  const double kFp = f(x + h);
  const double kFm = f(x - h);
  DiffResult r;
  r.f_plus = kFp;
  r.f_minus = kFm;
  r.deriv = (kFp - kFm) / (kTwo * h);
  return r;
}

inline double SnapH(double h, double quantum) {
  if (!(quantum > 0.0)) {
    return h;
  }
  const double kSnapped = std::round(h / quantum) * quantum;
  return (kSnapped > quantum) ? kSnapped : quantum;
}

inline double WrapDelta180(double a, double b) {
  constexpr double kHalfCircleDeg = 180.0;
  constexpr double kFullCircleDeg = 360.0;
  double d = a - b;
  while (d > kHalfCircleDeg) {
    d -= kFullCircleDeg;
  }
  while (d <= -kHalfCircleDeg) {
    d += kFullCircleDeg;
  }
  return d;
}

struct HSelection {
  double h = 0.0;
  double deriv = 0.0;
  double rel_spread = 0.0;
  bool ok = false;
};

template <typename F>
HSelection SelectH(F f, double x, double h_seed, double quantum) {
  constexpr int kMaxDoublings = 8;
  constexpr double kTwo = 2.0;
  constexpr double kMaxRelSpread = 0.2;
  double h = SnapH(h_seed, quantum);
  for (int i = 0; i < kMaxDoublings; ++i) {
    const double kH2 = SnapH(2.0 * h, quantum);
    const double kHh = SnapH(0.5 * h, quantum);
    const bool kDistinct = (kH2 > h || h > kH2) && (h > kHh || kHh > h) &&
                           (kH2 > kHh || kHh > kH2);
    if (kDistinct) {
      const double kD1 = CentralDifference(f, x, h).deriv;
      const double kD2 = CentralDifference(f, x, kH2).deriv;
      const double kD3 = CentralDifference(f, x, kHh).deriv;
      const double kMean = (kD1 + kD2 + kD3) / 3.0;
      HSelection s;
      s.h = h;
      s.deriv = kD1;
      if (!(std::fabs(kMean) > 0.0)) {
        s.rel_spread = 0.0;
        s.ok = true;
        return s;
      }
      const double kS12 = std::fabs(kD2 - kD1) / std::fabs(kMean);
      const double kS13 = std::fabs(kD3 - kD1) / std::fabs(kMean);
      s.rel_spread = (kS12 > kS13) ? kS12 : kS13;
      s.ok = s.rel_spread <= kMaxRelSpread;
      return s;
    }
    h = kTwo * h;
  }
  return {};
}

inline bool GenuineCheck(double response, double noise_floor, int sign_h,
                         int sign_2h) {
  constexpr double kMinSignalToNoise = 10.0;
  return (response > kMinSignalToNoise * noise_floor) && (sign_h == sign_2h) &&
         (response > 0.0);
}

struct CannedInput {
  const char* name;
  double h_canned;
  double quantum;
};

constexpr std::array<CannedInput, 15> kCannedTable = {{
    {"velocity_fps", 10.0, 1.0},
    {"bc_psi", 0.00425, 0.0},  // ±1% of the C1 0.425-scale BC; per-case BC
                               // scaling is applied by callers, see Task 3
    {"zero_angle_moa", 0.05, 0.0},
    {"optic_height_in", 0.1, 0.0},
    {"pressure_inhg", 0.1, 0.0},
    {"temperature_degf", 2.0, 0.0},
    {"humidity_pp", 5.0, 0.0},
    {"wind_speed_mph", 1.0, 0.0},
    {"wind_heading_deg", 2.0, 0.0},
    {"height_ft", 1.0, 0.0},
    {"shear_exponent", 0.02, 0.0},
    {"mass_grains", 1.0, 0.0},
    {"diameter_in", 0.002, 0.0},
    {"length_in", 0.002, 0.0},
    {"twist_in_per_turn", 0.5, 0.0},
}};

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
