# Phase 2 — Sensitivity Analysis — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build the one-variable-at-a-time sensitivity harness (central differences with adaptive step selection, noise-floor gating from Phase 1 floors, Pareto ranking) that identifies the dominant uncertainty drivers per case and output.

**Architecture:** Pure math helpers (solver-independent, unit-tested on analytic functions) plus thin OVAT plumbing (clone-and-tweak `lob::Builder`, rebuild, solve, difference) driving the unchanged public API. A CI smoke test ranks 2 inputs on C1; an env-gated offline driver runs the full Pareto (C1/C5/C8 + shear extension), flags nonlinearity, spot-checks top-3 interactions, and writes artifacts plus a checked-in `pareto.json`.

**Tech Stack:** C++14, GTest/GMock 1.14 (existing), CMake 3.14+, hand-rolled JSON/CSV extension of the Phase 1 writer (zero new dependencies).

## Global Constraints

- C++14 only; no `std::filesystem`, no structured bindings, no new third-party dependency.
- `lob_lob` library diff is empty: no `source/*.cpp|hpp` production change, no public API change, no new core dependency.
- All new test code uses the PUBLIC API only (`lob/lob.h`, `lob/lob.hpp`) → registered in `LOB_TEST_SOURCES` (shared+static CI). No internal headers anywhere in Phase 2.
- No test includes `lob_builder.cpp` internals (`Impl`/`Pimpl`). Perturbation surface is Builder inputs exclusively (§9.1); derived `Context` fields are never touched directly.
- Deterministic: fixed builders, fixed ranges, fixed default 36-in step, `0.01` MOA angle tolerance, no RNG, no wall-clock asserts, no threads.
- Numerical inputs (`StepSize`, angle tolerance) are NOT sensitivity dimensions (spec §9.1, §9.4); minimum-speed/energy/maximum-time are excluded (terminal guards, not physics).
- CI additions stay under 30 s; offline driver is env-gated (`LOB_FULL_PARETO=1`, else `GTEST_SKIP`), consistent with `LOB_FULL_LADDER`/`LOB_FLOOR_SURVEY`.
- Canned magnitudes (spec §9.2 seeds, not fixed constants): velocity `±10 fps`, BC `±1%`, pressure `±0.1 inHg`, temperature `±2 °F`, humidity `±5 pp`, wind speed `±1 mph`, wind heading `±2°`, measurement height `±1 ft` (shear path only), shear exponent `+0.02` one-sided from 0, zero angle `±0.05 MOA`, optic `±0.1 in`, mass `±1 gr`, diameter/length `±0.002 in`, twist `±0.5 in/turn` (C8 only).
- Plan location note: `docs/superpowers/plans/` is gitignored in this repo, so this plan lives versioned at `docs/specs/PHASE2_SENSITIVITY_PLAN.md` next to the spec and the Phase 1 plan.

---

## Scope

Phase 2 of `docs/specs/NUMERICAL_VALIDATION_SPEC.md` §20 ONLY: sensitivity methodology (§9), its CI smoke (§14.3, §17), Pareto artifacts (§9.4, §15), and the §19.3 acceptance slice. No §21 item blocks Phase 2 (all Phase-2-relevant decisions were recorded 2026-09-27).

Deliberate deferrals (recorded so reviewers don't re-litigate):
- Semi-elasticity `S = (∂y/∂x)·u_ref(x)` (§9.3) needs `u(x)` values that don't exist until Phase 4. Phase 2 stores raw derivatives + canned-response magnitudes (directly comparable, no invented uncertainties); the artifact schema reserves the `S` field for Phase 4 to fill.
- BC-band perturbation (one band at a time) needs its own floor cell, which doesn't exist. Single-BC perturbation covers the BC dimension; bands follow when a bands-context floor is measured (ledger note, not this phase).
- JSON *reader* is still Phase 3: noise floors enter as named constants cross-linked to `floors.json`, same trick as Phase 1 ceilings.
- CI smoke asserts the genuine path (velocity→elevation, wind→deflection) and the ranking mechanism. The `below_floor` path is unit-tested on synthetic data (Task 1), NOT asserted on live solver output in CI (a live below-floor assert would couple CI to modeling minutiae).

---

## File Structure

```
test/
  CMakeLists.txt                      MODIFY: append validation_sensitivity_test.cpp
                                      to LOB_TEST_SOURCES (1 line, no new defines;
                                      LOB_VALIDATION_DIR from Phase 1 is reused)
  source/
    testing.hpp                       UNTOUCHED (Phase 1 helpers reused as-is)
    validation_io.hpp                 MODIFY: ADD SensitivityArtifact struct
                                      alongside ConvergenceArtifact (existing
                                      struct byte-untouched)
    validation_sensitivity.hpp        CREATE: pure math (CentralDifference,
                                      SnapH, WrapDelta180, SelectH,
                                      GenuineCheck) + CannedInput table
                                      (~120 lines, test-only, stdlib only)
    validation_sensitivity_test.cpp   CREATE: math unit tests + OVAT plumbing
                                      tests + CI smoke + offline Pareto driver
  validation/
    baselines/
      floors.json                     UNTOUCHED (consumed as noise reference)
      pareto.json                     CREATE: curated driver ranking per
                                      case/output (checked in, from real runs)
docs/
  pages/validation/overview.md        MODIFY: append 3-line sensitivity pointer
                                      (no numbers)
  specs/PHASE2_SENSITIVITY_PLAN.md    THIS FILE
build/validation/                     GENERATED (gitignored): sensitivity_C1/
                                      C5/C8.json+csv from offline driver
```

Why this shape: math without solver dependence is unit-testable without builds of contexts; the writer extension is additive (Phase 1 artifact bytes stable); one test file holds smoke + driver because both consume the same plumbing; `pareto.json` is data, reviewed independently.

---

### Task 1: Pure sensitivity math + analytic unit tests

**Files:**
- Create: `test/source/validation_sensitivity.hpp`
- Test: `test/source/validation_sensitivity_test.cpp` (created here as scaffolding with the math tests; CMake registration happens in Task 3 with the smoke test — see note below)

**Interfaces:**
- Consumes: nothing (stdlib only: `<cmath>`, `<cstddef>`, `<functional>`, `<limits>`, `<string>`).
- Produces (used by Tasks 2, 3, 5): `tests::CentralDifference`, `tests::SnapH`, `tests::WrapDelta180`, `tests::HSelection`, `tests::SelectH`, `tests::GenuineCheck`, `tests::CannedInput`, `tests::kCannedTable`.

Semantics (spec §9.2, exact):
- `SnapH(h, quantum)`: quantum `≤ 0` → `h` unchanged (continuous input); else snap to grid with floor at one quantum: `max(quantum, round(h/quantum)*quantum)` via ordered comparisons (float-equal safe: test `!(quantum > 0.0)` for the continuous branch).
- `SelectH(f, x, h_seed, quantum)`: evaluates central-difference derivatives at snapped `h_seed`, `2·h_seed`, `h_seed/2` (re-snapped, requiring three DISTINCT snapped values — if snapping collides, e.g. tiny seed on integer grid, grow seed ×2 until distinct, max 8 doublings, else report `ok=false`); returns accepted `h` (the seed rung), `deriv`, `rel_spread` (max pairwise relative deviation, NaN-guarded), `ok = rel_spread ≤ 0.2`.
- `GenuineCheck(response, noise_floor, sign_h, sign_2h)`: `response > 10·noise_floor && signs agree && response > 0`. Signs are `±1` ints computed by callers via ordered comparisons (never pass raw doubles).
- `CannedInput { const char* name; double h_canned; double quantum; }` with the Global Constraints table; velocity quantum `1.0` (fps, integer setter), everything else quantum `0.0` except heights `0.0` (continuous — clamped positive by callers).

- [ ] **Step 1: Create the test file with analytic unit tests (fails: header missing)**

Create `test/source/validation_sensitivity_test.cpp`:

```cpp
// Copyright (c) 2026  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include <cmath>
#include <functional>
#include <limits>
#include <string>

#include "validation_sensitivity.hpp"

namespace tests {

TEST(SensitivityMath, CentralDifferenceQuadraticIsExact) {
  auto f = [](double x) { return x * x; };
  const DiffResult kR = CentralDifference(f, 3.0, 0.5);
  EXPECT_NEAR(kR.deriv, 6.0, 1e-9);
  EXPECT_DOUBLE_EQ(kR.f_plus, 12.25);
  EXPECT_DOUBLE_EQ(kR.f_minus, 6.25);
}

TEST(SensitivityMath, ConstantFunctionHasZeroDerivative) {
  auto f = [](double) { return 5.0; };
  const DiffResult kR = CentralDifference(f, 3.0, 0.5);
  EXPECT_DOUBLE_EQ(kR.deriv, 0.0);
  EXPECT_FALSE(GenuineCheck(0.0, 0.01, 1, 1));
}

TEST(SensitivityMath, SnapHRespectsIntegerGrid) {
  EXPECT_DOUBLE_EQ(SnapH(10.4, 1.0), 10.0);
  EXPECT_DOUBLE_EQ(SnapH(0.3, 1.0), 1.0);
  EXPECT_DOUBLE_EQ(SnapH(2.5, 0.0), 2.5);
}

TEST(SensitivityMath, SelectHAcceptsLinearFunction) {
  auto f = [](double x) { return 2.0 * x + 1.0; };
  const HSelection kS = SelectH(f, 3.0, 0.5, 0.0);
  ASSERT_TRUE(kS.ok);
  EXPECT_NEAR(kS.deriv, 2.0, 1e-9);
  EXPECT_DOUBLE_EQ(kS.h, 0.5);
}

TEST(SensitivityMath, SelectHFlagsSharpNonlinearity) {
  auto f = [](double x) { return x * x * x; };
  const HSelection kS = SelectH(f, 1.0, 0.5, 0.0);
  EXPECT_FALSE(kS.ok);
}

TEST(SensitivityMath, GenuineNeedsMarginAndSignAgreement) {
  EXPECT_TRUE(GenuineCheck(0.5, 0.01, 1, 1));
  EXPECT_FALSE(GenuineCheck(0.05, 0.01, 1, 1));
  EXPECT_FALSE(GenuineCheck(0.5, 0.01, 1, -1));
}

TEST(SensitivityMath, WrapDeltaNormalizesCrossWarrant) {
  EXPECT_DOUBLE_EQ(WrapDelta180(1.0, 359.0), 2.0);
  EXPECT_DOUBLE_EQ(WrapDelta180(359.0, 1.0), -2.0);
  EXPECT_DOUBLE_EQ(WrapDelta180(10.0, 20.0), -10.0);
}

TEST(SensitivityMath, CannedTableHasExpectedEntries) {
  EXPECT_STREQ(kCannedTable[0].name, "velocity_fps");
  EXPECT_DOUBLE_EQ(kCannedTable[0].h_canned, 10.0);
  EXPECT_DOUBLE_EQ(kCannedTable[0].quantum, 1.0);
}

}  // namespace tests
```

(`x³` at `x=1, h=0.5`: derivatives at 0.5/1.0/0.25 rungs are 3.25/4.0/3.0625 — spread >20% → `ok=false`. Verified by hand; if the implementer's arithmetic disagrees, recompute rather than loosen.)

- [ ] **Step 2: Run to verify it fails (header absent)**

Run: `cmake --build --preset=dev --target lob_test 2>&1 | head`
Expected: FAIL — `validation_sensitivity_test.cpp` is not yet registered in CMake, so nothing happens (no failure either). The honest RED for this task: `g++ -std=c++14 -fsyntax-only` on the test file fails on the missing header. Run: `g++ -std=c++14 -fsyntax-only -I test/source -I source -I include -I build/dev/include -I build/dev/export test/source/validation_sensitivity_test.cpp`
Expected: FAIL — `validation_sensitivity.hpp: No such file or directory`. (Include paths mirror `test/CMakeLists.txt` + generated headers; adjust only if the repo layout differs.)

- [ ] **Step 3: Implement the header (exact code)**

Create `test/source/validation_sensitivity.hpp`:

```cpp
// Copyright (c) 2026  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Test-only sensitivity math (Phase 2). Pure functions, no solver contact:
// unit-testable on analytic functions. C++14, stdlib only.

#pragma once

#include <cmath>
#include <cstddef>
#include <functional>
#include <limits>
#include <string>

namespace tests {

struct DiffResult {
  double deriv = 0.0;
  double f_plus = 0.0;
  double f_minus = 0.0;
};

template <typename F>
DiffResult CentralDifference(F f, double x, double h) {
  const double kFp = f(x + h);
  const double kFm = f(x - h);
  DiffResult r;
  r.f_plus = kFp;
  r.f_minus = kFm;
  r.deriv = (kFp - kFm) / (2.0 * h);
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
  double d = a - b;
  while (d > 180.0) {
    d -= 360.0;
  }
  while (d <= -180.0) {
    d += 360.0;
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
  double h = SnapH(h_seed, quantum);
  for (int i = 0; i < 8; ++i) {
    const double kH2 = SnapH(2.0 * h, quantum);
    const double kHh = SnapH(0.5 * h, quantum);
    const bool kDistinct =
        (kH2 > h || h > kH2) && (h > kHh || kHh > h) && (kH2 > kHh || kHh > kH2);
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
      s.ok = s.rel_spread <= 0.2;
      return s;
    }
    h = 2.0 * h;
  }
  return HSelection();
}

inline bool GenuineCheck(double response, double noise_floor, int sign_h,
                         int sign_2h) {
  return (response > 10.0 * noise_floor) && (sign_h == sign_2h) &&
         (response > 0.0);
}

struct CannedInput {
  const char* name;
  double h_canned;
  double quantum;
};

constexpr CannedInput kCannedTable[] = {
    {"velocity_fps", 10.0, 1.0},
    {"bc_psi", 0.00425, 0.0},  // ±1% of the C1 0.425-scale BC; per-case BC scaling is applied by callers, see Task 3
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
};

}  // namespace tests
```

Notes the implementer must respect: `bc_psi` canned `0.00425` is 1% of the SetupTestBuilder-scale BC (0.425) — callers scale per case (C1: 1% of 0.232 = 0.00232); the Task 3 smoke test does this scaling explicitly. `(kH2 > h || h > kH2)` is the float-equal-safe inequality idiom used across this repo (NaN → false → not distinct → grow, which is the safe direction).

- [ ] **Step 4: Syntax-check to verify GREEN (registration lands in Task 3)**

Run the Step-2 `g++ -fsyntax-only` command again.
Expected: PASS (no output). Full-suite proof waits for Task 3 registration — state that in the report.

- [ ] **Step 5: Commit**

```bash
git add test/source/validation_sensitivity.hpp test/source/validation_sensitivity_test.cpp
git commit -m "test: add pure sensitivity math with analytic unit tests"
```

---

### Task 2: OVAT plumbing (appliers, channels, sanity tests)

**Files:**
- Modify: `test/source/validation_sensitivity_test.cpp` (append plumbing + `SensitivityPlumbing.*` tests; NO CMake change — file still unregistered, still syntax-checked standalone)

**Interfaces:**
- Consumes: Task 1 header, `lob::Builder` copy semantics (copy ctor via `LobBuilderCopy`), `lob::Solve` array overload, Phase 1 `SolveN`.
- Produces (used by Tasks 3, 5): `tests::PerturbScalar` pattern (documented by example, not abstracted — see below), `tests::SolveChannelAt` helper, channel-extractor convention.

Deliberate non-abstraction (YAGNI, reviewable): there is NO generic applier framework. Each input kind gets one documented 5-line pattern in the test file, because a `std::function`-based generic applier would hide the integer-snap and borrow-lifetime subtleties the plan needs visible:
- scalar setter: copy base builder, call setter with perturbed value, `Build()`.
- integer setter (velocity): `static_cast<uint16_t>(std::llround(v))` inside the applier.
- profile station: local `std::array<lob::WindPoint, N>` copy, tweak element, `WindProfile(arr)` + synchronous `Build()` (borrow safe).
- shear: one-sided `WindShearExponent(a + h)` (never below 0).

- [ ] **Step 1: Write plumbing + sanity tests (fail: helpers absent — same-file, so RED is the syntax check)**

Append to the test file:

```cpp
namespace {
// Reuses the C1-ICAO point from tests::MakeC1IcaoBuilder (testing.hpp).
// Channel convention for all Phase 2 work: extractors are tiny lambdas at
// call sites over const lob::Output&, e.g.
//   [](const lob::Output& o) { return o.elevation; }
// for forward inches, lob::InchToMoa(o.elevation, o.range) for MOA channels.
template <size_t N>
lob::Output SolveChannelAt(const lob::Context& ctx,
                           const std::array<uint32_t, N>& ranges, size_t idx) {
  std::array<lob::Output, N> outs{};
  const size_t kSolved = SolveN(ctx, ranges, &outs);
  EXPECT_EQ(kSolved, N);
  return outs[idx];
}
}  // namespace

TEST(SensitivityPlumbing, VelocityApplierBuildsCleanContexts) {
  const std::array<uint32_t, 2> kRanges = {900U, 1800U};
  lob::Builder base = MakeC1IcaoBuilder();
  for (const double kDv : {-10.0, 0.0, 10.0}) {
    lob::Builder p = base;
    p.InitialVelocityFps(
        static_cast<uint16_t>(std::llround(2800.0 + kDv)));
    const lob::Context kCtx = p.Build();
    ASSERT_EQ(kCtx.error, lob::ErrorT::kNone);
    EXPECT_EQ(kCtx.velocity, static_cast<uint16_t>(2800 + static_cast<int>(kDv)));
  }
  const lob::Context kBase = base.Build();
  const double kE0 = SolveChannelAt(kBase, kRanges, 1).elevation;
  lob::Builder p = base;
  p.InitialVelocityFps(2810U);
  const double kE1 = SolveChannelAt(p.Build(), kRanges, 1).elevation;
  // Measured 0.859 in (drop∝t² predicts ≈0.65); bound 0.5 keeps channel
  // consistency with the Task 3 smoke tests at ~12,000× the noise floor.
  EXPECT_GT(std::fabs(kE1 - kE0), 0.5);
}

TEST(SensitivityPlumbing, WindApplierBuildsCleanContexts) {
  lob::Builder base = MakeC1IcaoBuilder();
  for (const double kDw : {-1.0, 1.0}) {
    lob::Builder p = base;
    p.WindHeadingDeg(90.0).WindSpeedMph(kDw > 0.0 ? kDw : -kDw);
    // NOTE: speed is magnitude-only; direction comes from heading. Negative
    // perturbation at zero baseline is expressed as heading 270 vs 90, see
    // Task 3 smoke test — this sanity check only proves clean builds.
    const lob::Context kCtx = p.Build();
    ASSERT_EQ(kCtx.error, lob::ErrorT::kNone);
  }
}
```

The wind NOTE records a real subtlety the implementer must get right in Task 3: `WindSpeedMph` takes magnitude (negative speeds normalize per the wind spec), so the `x−h` wind evaluation at zero baseline is heading `270°` at `1 mph`, NOT speed `−1 mph`. Task 3's smoke test implements exactly that; this plumbing test only asserts clean builds.

- [ ] **Step 2: Syntax-check (RED: SolveChannelAt undefined → GREEN after append)**

Run the Task-1 `g++ -fsyntax-only` command before adding the helper (expect fail on `SolveChannelAt`), then after (expect pass). Two-command TDD evidence for the report.

- [ ] **Step 3: Commit**

```bash
git add test/source/validation_sensitivity_test.cpp
git commit -m "test: add OVAT plumbing patterns with sanity tests"
```

---

### Task 3: CI smoke (C1 × velocity + wind) + noise-floor constants

**Files:**
- Modify: `test/source/validation_sensitivity_test.cpp` (append smoke test)
- Modify: `test/CMakeLists.txt` (append file to `LOB_TEST_SOURCES` — FIRST build integration; public API only)

**Interfaces:**
- Consumes: Tasks 1–2, Phase 1 `floors.json` C1 cell (values copied as constants with provenance comments — no JSON reader until Phase 3).
- Produces: the CI gate for sensitivity; 2-item Pareto mechanism reused by Task 5.

Noise constants (mirror `floors.json` C1-ICAO `floors_18_9`, comment cross-links the file + cell):
`kNoiseElevIn = 4.15814e-05`, `kNoiseElevMoa = 3.97148e-06`, `kNoiseDeflMoa = 1e-12` — wait: deflection floor is exactly `0.0` with ceiling `1e-12`. For the genuine-response test (`response > 10·noise`), noise `0.0` makes ANY nonzero response genuine — correct per §9.4 (the floor IS zero; the epsilon ceiling is a CI guard, not the noise model). Use floor `0.0` for deflection with a comment explaining the distinction. TOF noise `6.12488e-08`.

Smoke design (hermetic, in-memory, default 36-in step, ranges {300, 900, 1800, 3000}):
- velocity→elevation at 1800 ft: full `SelectH` at canned ±10 (snapped integer rungs 10/20/5), assert `ok`, assert `GenuineCheck` true with `kNoiseElevIn`, assert sign consistency. Expected: genuine with feet-scale response (tens of inches for ±10 fps at 3000 ft — if the measured response is below 1.0 in, STOP and report NEEDS_CONTEXT: something structural changed).
- wind-speed→deflection at 1800 ft: evaluations at headings 90°/270° × 1 mph (the ±h pair around the zero-wind baseline) plus 2h pair at 2 mph for the agreement check; assert genuine with `kNoiseDeflMoa` floor `0.0` (any consistent nonzero response passes — the assert that matters is sign agreement + finiteness, proving the deflection channel moves with wind).
- Pareto smoke: shares from canned-response magnitudes on the elevation channel {velocity, wind-speed}: `share_v = |Δv| / (|Δv| + |Δw|)`, assert `share_v > 0.9` (crosswind elevation response is second-order/even → central diff ≈ 0; velocity ≈ 100%) and `|share_v + share_w − 1.0| < 1e-9`.
- BC scaling note for future tasks: C1 BC is 0.232, so the offline BC canned step is `0.00232` (1%), NOT the table's `0.00425` — Task 5 must scale per case. This smoke test does not perturb BC.

- [ ] **Step 1: Register in CMake + write smoke test (RED: new test fails on missing constants? No — honest RED: write test, build, run; if genuine-asserts fail, the failure IS the finding)**

CMake edit first (1 line in `LOB_TEST_SOURCES`, after `validation_convergence_test.cpp`), then append the smoke test per the design above (full code at implementation time following Tasks 1–2 patterns; ~60 lines).

- [ ] **Step 2: Build + run smoke**

Run: `cmake --build --preset=dev --target lob_test && ./build/dev/test/lob_test --gtest_filter='SensitivitySmoke.*:SensitivityMath.*:SensitivityPlumbing.*'`
Expected: PASS. On genuine-assert failure: do NOT loosen — measure, compare against the Task-4-style margin logic, and escalate NEEDS_CONTEXT with numbers (a failed genuine assert means the solver stopped responding to velocity, which is structural news).

- [ ] **Step 3: Commit**

```bash
git add test/source/validation_sensitivity_test.cpp test/CMakeLists.txt
git commit -m "test: add C1 velocity/wind sensitivity smoke with Pareto check"
```

---

### Task 4: Sensitivity artifact writer + round-trip tests

**Files:**
- Modify: `test/source/validation_io.hpp` (ADD `SensitivityArtifact`; existing `ConvergenceArtifact` byte-untouched)
- Test: `test/source/validation_sensitivity_test.cpp` (append `SensitivityIo.*` round-trip tests on synthetic rows)

**Interfaces:**
- Consumes: Task 1 row semantics. Produces (used by Task 5): `tests::SensitivityArtifact` with `AddRow(input, range_ft, output, h, raw_deriv, canned_response, nonlinear_flag, status)` + `ToJson`/`ToCsv`/`WriteFiles`.

Row schema (S reserved for Phase 4 — present as `"sensitivity_coefficient_S": null` with a code comment citing spec §9.3/§11):
```json
{"input":"velocity_fps","range_ft":1800,"output":"elevation_in","h_accepted":10.0,"raw_deriv":2.31,"canned_response":23.1,"nonlinear":false,"status":"genuine","sensitivity_coefficient_S":null}
```
CSV header: `input,range_ft,output,h_accepted,raw_deriv,canned_response,nonlinear,status` (S column omitted from CSV until Phase 4 fills it — JSON carries the null placeholder). `status ∈ {genuine, below_floor}` exactly (spec §9.4 vocabulary).

- [ ] **Step 1: Write round-trip tests on synthetic rows (fail: type absent)**

```cpp
TEST(SensitivityIo, ArtifactRoundTripsSyntheticRows) {
  SensitivityArtifact artifact;
  artifact.provenance_lob_version = "0.13.0-test";
  artifact.provenance_git_sha = "deadbee";
  artifact.solver_config = "step_in=36,angle_tol_moa=0.01,density_path=fast";
  artifact.AddRow("velocity_fps", 1800U, "elevation_in", 10.0, 2.31, 23.1,
                  false, "genuine");
  artifact.AddRow("wind_speed_mph", 1800U, "elevation_in", 1.0, 0.0, 0.0,
                  false, "below_floor");
  const std::string kJson = artifact.ToJson();
  EXPECT_NE(kJson.find("\"input\":\"velocity_fps\""), std::string::npos);
  EXPECT_NE(kJson.find("\"status\":\"below_floor\""), std::string::npos);
  EXPECT_NE(kJson.find("\"sensitivity_coefficient_S\":null"), std::string::npos);
  const std::string kCsv = artifact.ToCsv();
  EXPECT_NE(kCsv.find("input,range_ft,output,h_accepted,raw_deriv,"),
            std::string::npos);
}
```

- [ ] **Step 2: Run to verify it fails (type + header additions absent)**
- [ ] **Step 3: Implement `SensitivityArtifact` in `validation_io.hpp`** (same style as `ConvergenceArtifact`: `JsonEscape`/`JsonDouble` reuse, `uint32_t range_ft`, `bool nonlinear` → `true`/`false`, `WriteFiles(dir, stem)` identical shape)
- [ ] **Step 4: Run to verify it passes** (round-trip + full `ctest -R Validation` green)
- [ ] **Step 5: Commit**

```bash
git add test/source/validation_io.hpp test/source/validation_sensitivity_test.cpp
git commit -m "test: add sensitivity artifact writer with reserved S field"
```

---

### Task 5: Offline full Pareto driver (C1/C5/C8 + shear extension) + `pareto.json`

**Files:**
- Modify: `test/source/validation_sensitivity_test.cpp` (append env-gated driver)
- Create: `test/validation/baselines/pareto.json` (from real runs — no placeholders)

**Interfaces:**
- Consumes: Tasks 1–4, Phase 1b `floors.json` cells (C1/C5/C6/C8 noise constants, hardcoded with cross-links — C9 tail excluded: no inverse-channel Pareto in Phase 2 scope).
- Produces: `build/validation/sensitivity_{C1,C5,C8}.json+csv`, `pareto.json`, Phase 3/4 input (ranked dimensions + flags).

Driver design (`LOB_FULL_PARETO=1`, else `GTEST_SKIP`; no file I/O unless gated; writes via `WriteFiles(LOB_VALIDATION_DIR, stem)` reusing Task 8's compile definitions — no CMake change):
- Input lists (all at default 36-in step; BC canned scaled per case: C1 `0.00232`, C5/C8 per their builders):
  - C1 (MakeC1IcaoBuilder, ranges {300,900,1800,3000}): velocity, BC, zero_angle, optic, pressure, temp, humidity, mass, diameter, length, wind_speed, wind_heading (12).
  - C5 (C1 + kIII 5 mph uniform — the EXACT C5-uniform survey builder; cite the survey test): same 12.
  - C8 (Litz spin context — the EXACT C8-Litz survey builder; cite): same 12 + twist (13).
  - C6-shear extension (the EXACT C6-scaled survey builder: two-point profile + α=0.25): shear_exponent (one-sided) + height_ft of station 2 only (2 extra inputs — heights pruned everywhere else per §9.1 inert-at-α0 rule).
- Per (case, input, output-channel, range): `SelectH` → if `!ok`, record `nonlinear=true` with the seed-rung derivative (route-to-MC flag, not a failure); `GenuineCheck` vs the cell's noise floor → `genuine`/`below_floor`; canned response `|f(x+h)−f(x−h)|/2`… precisely: `canned_response = |f(x+h) − f(x−h)|` (full symmetric swing at accepted h — document the definition in the artifact).
- Output channels: forward `elevation_in`, `elevation_moa`, `deflection_moa`, `velocity_fps`, `tof_s` at every ladder range; inverse MOA only for the C1-900/1800 pair (Fast branch, reusing the Task 5 precondition pattern).
- Interactions: rank Pareto per (case, output) by canned response; take top-3 pairs; corner evaluations `f(x±h, z±h)` (4 extra solves per pair); record residue `f(x+h,z+h) − f(x+h) − f(z+h) + f(x)` in the artifact's `interactions[]` array. Full factorial explicitly out of scope.
- `pareto.json` (checked in): per (cell, output-channel, range): ordered drivers with shares (share = canned_response/Σ), `cutoff_90` list, `nonlinear_flags[]`, `below_floor[]` (with floor values, never zeros). Numbers from the actual gated run; provenance block like `floors.json` plus `driver: ValidationFullPareto`.

- [ ] **Step 1: Implement driver + run gated** (`LOB_FULL_PARETO=1 ./build/dev/test/lob_test --gtest_filter='SensitivityFullPareto.*'`), iterate on runtime (target: minutes, single-threaded; ~12 inputs × 3 rungs × 2 evals ≈ 72+ solves per case-channel set — if wall time exceeds 10 min, halve the range set to {900, 1800} and record the reduction in the commit message, never silently).
- [ ] **Step 2: Transcribe `pareto.json`** from artifacts (reviewed numbers, zero placeholders).
- [ ] **Step 3: Prove skip-by-default** (unset env → SKIPPED, suite green) + `ctest -R Validation` green both ways.
- [ ] **Step 4: Commit** (test file + pareto.json; artifacts under `build/` stay untracked)

```bash
git add test/source/validation_sensitivity_test.cpp test/validation/baselines/pareto.json
git commit -m "test: add offline full-Pareto driver with curated ranking"
```

---

### Task 6: Docs pointer + full gate + lint

**Files:**
- Modify: `docs/pages/validation/overview.md` (append 3-line sensitivity pointer under the convergence paragraph from Phase 1 Task 9 — method pointer, no numbers)
- No code changes unless the gate finds regressions.

- [ ] **Step 1: Append pointer text**

```markdown
Sensitivity methodology and driver ranking live in
`docs/specs/NUMERICAL_VALIDATION_SPEC.md` (§9) with the harness in
`test/source/validation_sensitivity_test.cpp` (public API) and curated
ranking in `test/validation/baselines/pareto.json`. CI smokes
C1 × velocity/wind with Pareto-share checks; full C1/C5/C8 Pareto runs
offline behind `LOB_FULL_PARETO=1`. Semi-elasticities await Phase 4 input
uncertainties; the artifact schema reserves the field.
```

- [ ] **Step 2: Full local gate** — `cmake --build --preset=dev && ctest --preset=dev --output-on-failure -j 4` → 100% PASS (full-Pareto SKIPPED); `LOB_FULL_PARETO=1` focused run → PASS + artifacts with provenance.
- [ ] **Step 3: Linters** — `cmake -D FORMAT_COMMAND=clang-format -P cmake/lint.cmake`, `cmake -P cmake/spell.cmake` (via nix per HACKING.md if local); fix in place, `style:` commit if needed.
- [ ] **Step 4: Acceptance snapshot** (§19.3): adaptive-h harness runs C1/C5/C8; accepted h + raw + S-reserved + nonlinearity flags + 90% Pareto checked in; below_floor entries carry floors; empty `lob_lob` diff proof (`git status --short | grep -v '^?$' | grep -E 'source/|include/'` prints nothing). Commit docs.

```bash
git add docs/pages/validation/overview.md
git commit -m "docs: point validation page at sensitivity methodology"
```

---

## Self-Review

**1. Spec coverage (§9 / §14 / §15 / §17 / §19.3 / §20 Phase 2 row):**
- §9.1 meaningful inputs → Task 2 patterns + Task 5 lists (categoricals excluded as scenario-swaps; node ranges excluded with rationale; derived Context fields forbidden — the plan never touches them; numerical inputs excluded).
- §9.2 central diffs + adaptive-h + wrap + canned seeds → Tasks 1–2 (integer snapping is the added rigor the spec's formula needs for the `uint16_t` velocity setter; heading wrap helper + magnitude-only wind subtlety documented).
- §9.3 raw + canned-response + elasticity-where-defined + S-reserved → Tasks 4–5 (S explicitly deferred to Phase 4 with schema reservation, not silently dropped).
- §9.3 nonlinearity (±h vs ±2h flag) + top-3 interactions → Task 5 (free 2h evals reused; corners recorded; no factorial).
- §9.4 genuine rule + below_floor + Pareto + downstream contract → Tasks 1 (rule), 3 (smoke), 5 (Pareto + MC-dimension contract in pareto.json).
- §14 (extend test/source/, LOB-only, no Impl) → all test code public-API; single CMake line.
- §15 (JSON+CSV, provenance, raw-vs-curated) → Task 4 writer + Task 5 artifacts/build vs pareto.json checked in.
- §17 (fast gate, offline runner, promotion) → Tasks 3 (hermetic CI) + 5 (gated offline) + margin procedure inherited from Phase 1 (measure-then-record, no silent loosening).
- §19.3 acceptance → Task 6 snapshot. §20 Phase 2 row → files/tests/artifacts/CI/cost all covered.
- GAP CHECK: §9.1 "drag-curve parameters" — covered via BC single-path + custom-table scale? Custom-table Cd-scale perturb is listed in spec §9.1 ("custom-table Cd scale (uniform ±%, not per-knot)"). The Task 5 lists omit it (no custom-table floor cell exists). FIX APPLIED in plan: same deferral class as BC bands — single-BC covers the drag-magnitude dimension; custom-table and bands variants wait on their floor cells. Stating here so Phase 2 stays honest: drag-*source* sensitivity is scenario-swap territory (§10.5), already covered by model-form checks, not derivatives.

**2. Placeholder scan:** no TBD/TODO/later in deliverables; `S: null` is a schema reservation with a named filling phase, not a placeholder; `pareto.json` filled from real runs in-task; ceiling/floor constants mirror reviewed `floors.json` values.

**3. Type consistency:** `uint16_t` velocity handled via llround+cast in appliers and quantum-1.0 snapping in SelectH (consistent); `WindSpeedMph(double)` magnitude-only with heading-pair trick consistent between Task 2 NOTE and Task 3 design; `WindPoint` array borrow synchronous in lambdas (established Phase 1 pattern); `LOB_VALIDATION_DIR`/`LOB_GIT_SHA` defines already exist on the `lob_test` target (no CMake define work needed); `GTEST_SKIP` 1.14-valid; `std::function` unused by final design (documented patterns instead — the `<functional>` include in Task 1 test file is unnecessary; drop it at implementation time — recorded here, not a defect).

---

*End of plan. Phase 3 (reference validation) plan is written after Phase 2 baselines merge, per spec Appendix A.*
