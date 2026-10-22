// Copyright (c) 2026  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later

#include <cstddef>
#include <cstdint>

#include "splines.hpp"
#include "tables.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#define LOB_KEEPALIVE EMSCRIPTEN_KEEPALIVE
#else
#define LOB_KEEPALIVE
#endif

namespace {
constexpr double kMinMach = 0.0;
constexpr double kMaxMach = 5.0;

double ClampMach(double m) {
  if (m < kMinMach) {
    return kMinMach;
  }
  if (m > kMaxMach) {
    return kMaxMach;
  }
  return m;
}
}  // namespace

extern "C" {

LOB_KEEPALIVE int LobExhibitsVersion() { return 1; }

// Drag coefficient of a standard curve at a Mach number.
LOB_KEEPALIVE double LobExhibitDragEvalG1(double mach) {
  using namespace lob::spline;
  CurveView curve(kKnots.data(), kG1Coefs.data());
  return curve.Eval(static_cast<float>(ClampMach(mach)));
}

LOB_KEEPALIVE double LobExhibitDragEvalG7(double mach) {
  using namespace lob::spline;
  CurveView curve(kKnots.data(), kG7Coefs.data());
  return curve.Eval(static_cast<float>(ClampMach(mach)));
}

}  // extern "C"
