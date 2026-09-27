# Numerical Validation & Uncertainty Framework — Technical Specification

**Status:** specification only (no implementation).
**Target:** `lob` — open-source C++14 exterior ballistics library
(`https://github.com/joelbenway/lob`, v0.13.0 plus the merged wind-profile
ABI break and its hardening through `d925a09`).
**Deliverable location:** `docs/specs/NUMERICAL_VALIDATION_SPEC.md`

Authoritative wind reference: `docs/superpowers/specs/WIND_INTERFACE_SPEC.md`
(Status: Proposed) is the interface contract for the merged wind-profile
feature (direction+magnitude profile input, 1-ft reference normalization,
shot-parallel altitude scaling, frame-baked pitch). This validation spec does
not restate that contract — it cites it and defines how to *measure* the
numerics, sensitivities, and uncertainties around it (§4.6, §8.7, §9–§13).

> Location note: the repo already has `docs/superpowers/specs/` (dated
> brainstorming design notes, e.g. `2026-08-12-bc-drags-unification-design.md`,
> plus `WIND_INTERFACE_SPEC.md`) and `docs/pages/` (Doxygen technical
> reference, `INPUT` in `docs/Doxyfile.in`). This spec creates `docs/specs/`
> for a different purpose: an **implementation-ready engineering specification**
> with acceptance criteria and phased work items, not a brainstorming note and
> not a Doxygen page. If maintainers prefer, the file moves to
> `docs/superpowers/specs/` with no content change; the `docs/specs/` choice
> keeps validation specs discoverable and versioned separately from dated
> design explorations.

---

## 1. Purpose

Establish a rigorous, incremental methodology for `lob` that separates four
quantities that are routinely conflated:

1. **Numerical error / resolution** — what the solver implementation adds.
2. **Input-parameter sensitivity and propagated uncertainty** — how `u(xi)`
   becomes `u(y)` for `y = f(x)`.
3. **Reference / model discrepancy** — where `lob`'s physics differs from a
   trusted reference or reality.
4. **Prediction uncertainty** — the defensible total uncertainty on a predicted
   output.

The framework must answer, with evidence:

- How much error is attributable to the numerical solver itself?
- Which inputs dominate uncertainty in a given output?
- How much does an uncertain input propagate into the prediction?
- Is a modeled effect large enough to matter against the noise floor?
- Over what operating envelope can `lob` make defensible validation claims?

Monte Carlo (MC) is an **uncertainty-propagation layer around the deterministic
solver** (`uncertain inputs → deterministic lob solve → output distribution`),
never a replacement for the deterministic trajectory solver, and never part of
the embedded core.

No global "`lob` is accurate to ±X%" claim is in scope. The output is a
**validated operating envelope** with per-envelope error floors, sensitivities,
budgets, and provenance.

---

## 2. Scope

In scope:

- Convergence / error-floor methodology for the Heun integrator, step size,
  angle solver, and spline evaluation.
- One-variable-at-a-time (OVAT) sensitivity methodology with central
  differences, noise-floor handling, and ranking of dominant drivers.
- Reference-validation case format, comparison metrics, and envelope reporting.
- Uncertainty-budget schema (linear propagation where justified, flagged
  nonlinear regimes).
- MC propagation design (distributions, seeding, convergence, summaries, cost,
  placement outside the core).
- Signal-versus-uncertainty decision methodology.
- Test/data/artifact architecture, reproducibility rules, CI strategy,
  performance boundary, acceptance criteria, phased plan, docs integration.

Out of scope for this spec: changing the integrator, changing the physics,
acquiring new field data, tuning knot placement, universal accuracy numbers.

---

## 3. Background and terminology

### 3.1 Verification vs validation vs uncertainty (authoritative basis)

- **Verification** — "are we solving the equations correctly?" (code +
  numerical correctness). Follows **ASME V&V 10 (solids) / V&V 20 (fluids)**
  distinction and Oberkampf & Roy *Verification and Validation in Scientific
  Computing*: verification precedes validation; validation cannot fix an
  unverified code.
- **Validation** — "are we solving the correct equations?" Comparison against
  a reference, with all uncertainties characterized. Agreement on one case is
  not validation.
- **Uncertainty quantification** — follows the **JCGM 100 (GUM)** vocabulary:
  standard uncertainty `u(x)`, combined standard uncertainty `u_c(y)`,
  sensitivity coefficients `c_i = ∂y/∂xi`, coverage intervals (not
  "confidence" unless the distributional assumptions are stated and met).
- This spec uses GUM terms (`u`, `u_c`, sensitivity coefficient) and ASME
  `verification / validation / prediction uncertainty` terms consistently.
  Doxygen pages keep their existing names; where they say "accuracy" they mean
  the tolerances cited from tests (see `docs/pages/validation/overview.md`).

### 3.2 Four quantities (never conflate)

```
               ┌──────────────┐
               │ true physics │  (unknown; reality / range truth)
               └──────┬───────┘
                      │ model discrepancy δ_model
               ┌──────▼───────┐     ┌──────────────────┐
               │ math model   │────▶│ reference model  │  (McCoy ISA, BRL tables,
               │ (point-mass  │ δref│ (JBM tables,     │   Huang, papers)
               │  + corrections)    │  borrowed traj.) │
               └──────┬───────┘     └──────────────────┘
                      │ numerical error ε_num (Heun, step, angle tol, spline)
               ┌──────▼───────┐
               │ lob output y │  + input uncertainty u(x) → prediction
               │  = f(x)      │    uncertainty u(y) via sensitivities / MC
               └──────────────┘
```

- `ε_num` — deterministic, reducible by refinement (step, tolerance).
  Measured by convergence studies (§8).
- `δ_model` / `δ_ref` — structural difference between `lob`'s equations and
  reality / a reference implementation. Not reducible by refinement; only by
  model change or envelope restriction (§10).
- `u(x)` — input uncertainty (chronograph, pressure sensor, BC scatter…).
  Supplied by the user/measurement process, never invented by `lob` (§11).
- `u(y)` — propagated prediction uncertainty (§11–§12).

Calling `|y_lob − y_ref|` "numerical error" is forbidden unless `ε_num` was
isolated by a refinement study at fixed inputs and the residual is shown to
shrink with refinement.

### 3.3 Existing `lob` vocabulary (reuse, do not rename)

- `Builder → Context → Solve` (§4). `LobOutput.elevation/deflection` are
  **inches in forward solves, MOA in inverse solves**.
- `FastDsDx / FastSolveStep / FastSolveAngle` = firing-site density fast path;
  `DsDx / SolveStep / SolveAngle` = lapse-scaled path (§4).
- `kDynamicDropThreshold = FeetT(100)` gates the inverse per-range path.
- `CurveView` over `drags[60]`; `kKnots` (16); `kCoefsSize = 60`.

---

## 4. Existing `lob` architecture relevant to validation

(Verified against source at v0.13.0. File:line references are the contract;
if code moves, the spec's logic still holds.)

### 4.1 Hourglass API and ABI boundary

- C ABI waist: `include/lob/lob.h` — `LobBuilder` (opaque,
  `LOB_BUILDER_BUFFER_SIZE 320`), `LobContext`, `LobOutput`, `LobSolve`,
  `LobSolveInverse`, `LobFastInverse`, `LobBuilderBuild`, unit conversions.
- C++ wrapper: `include/lob/lob.hpp` — `lob::Builder`, `lob::Context`
  (layout-identical, `static_assert`s), `lob::Solve/SolveInverse/FastInverse`,
  `std::array` overloads. Header-only, inline, forwards to C.
- Implementation once: `source/lob_builder.cpp`, `source/lob_solve.cpp`.
- **Validation rule:** all convergence/sensitivity/reference/MC studies drive
  the **public C/C++ API** (`Builder` + `Solve`/`SolveInverse`). White-box
  step-level tests may include internal headers (`source/solve_step.hpp`,
  `source/solve_angle.hpp`, `source/splines.hpp`, `source/ode.hpp`) but only
  in static test builds (see §14.2); nothing validation-specific ships in the
  library.

### 4.2 Deterministic forward solver

- `source/lob_solve.cpp` `LobSolve`: integrates from muzzle with launch angle
  `zero_angle + aerodynamic_jump`, steps via `FastSolveStep` (firing-site
  `FastDsDx`), clamps the last step to the target (`ComputeStep` in
  `source/solve_step.cpp`), stops at all-ranges-reached / `max_time` /
  `minimum_speed|energy` (linearly interpolated `LerpOutput`) / tumble
  (`|v_y| > 3·v_x`).
- Output quantization floor (load-bearing for convergence, §8.5):
  `OutputAtState` truncates to `range: U32`, `velocity: U16`, `energy: U32`.
  `elevation/deflection/tof` stay `double`. Any study comparing `velocity` or
  `energy` has a ±0.5 LSB rectangular floor that is **not** solver error.
- Strictly-increasing `pranges` guard; `range == 0` muzzle entry allowed.
- No allocation, no exceptions, no RNG, no threads on the hot path.

### 4.3 Integrator and step control

- `source/ode.hpp`: `EulerStep`, `HeunStep` (RK2 predictor-corrector),
  `RungeKuttaStep` (RK4). **Production uses Heun only** (`source/ode.hpp:20`,
  `source/solve_step.cpp:121,136`). Euler/RK4 exist for
  `test/source/ode_test.cpp` demonstration on `dy/dt = sin²(t)·y` and
  `benchmark/ode.cpp` cost-matched comparison. Do not assume RK4 order for
  `lob` trajectories; Heun is globally 2nd-order on smooth problems, reduced
  near discontinuities (Mach-knot crossings, wind-node joints, terminal
  guards).
- BRL basis documented in `docs/pages/numerical_methods/ode.md` (McCoy: 1-step
  2nd-order methods "optimum" for point-mass). The spec takes this as history,
  not proof — the error floor is measured (§8), not quoted from McCoy.
- Step: `ctx.step_size` (`uint16_t` inches, `0` = default 1 yard = 36 in),
  `ComputeStep`: `Δx = min(target − x, step)`. No adaptivity, no embedded
  error estimate. `TOF` integrated as `d(TOF)/dx = 1/vx` inside `DsDxCore`.

### 4.4 Two density paths (must be studied separately)

- `FastDsDx`: `drag = drag_coeff`, `c = speed_of_sound` (firing-site).
- `DsDx`: scales per step by `ρ/ρ0 = 1−u(1−αu)`, `c/c0 = 1−βu`,
  `u = −k_lapse·P·G`, `α = (e−1)/2e`, `β = 1/2e`, `e = kHydrostaticExponent`
  (`source/solve_step.cpp:96-102`, `source/constants.hpp:27-29`).
- Routing (`source/lob_solve.cpp:22`, `kDynamicDropThreshold`):
  forward / zero / Boatright-supersonic integral use `Fast*`; only
  `LobSolveInverse` ranges with forward `drop > 100 ft`
  (`elevation < −1200 in`) use `SolveStep`/`SolveAngle`. Builder zero search
  uses `FastSolveAngle`.
- Implication: a convergence claim on `LobSolve` does **not** transfer to the
  lapse-scaled inverse tail. §8 requires both paths.

### 4.5 Angle solver (shared zero + inverse)

- `source/solve_angle.hpp`: fixed-point
  `θ_{n+1} = FastInverseAngle(θ_n, f(θ_n), R)`,
  `FastInverseAngle = atan(tanθ − f/R)`, tolerance
  `kDefaultAngleTolerance = 0.01 MOA`, cap `kMaxIterations = 10`, bounds
  `±45°`, NaN on unreachable / non-convergence. `FireToTarget` integrates
  step-by-step; `vx ≤ 0` / `TOF ≥ max_time` / speed floor → NaN.
- Seeds: builder zero uses vacuum parabola `0.5·g·R/v0²` clamped to `±45°`
  (`source/lob_builder.cpp` `BuildZeroAngle`); inverse seeds with
  `FastInverseAngle(zero_angle, forward_residual, R)` (usually within
  0.1 MOA, 1–2 iterations).
- `LobSolveInverse` = forward reachability pass + per-range re-solve;
  `LobFastInverse` = one-step geometric warp in place (no integration; fall
  shorts yield meaningless numbers by documented design).
- Tested: seeds `0°, ±30°, 44°`, `±3 in` round-trip to `±0.1 in`, tight
  `0.001 MOA → ±0.01 in`, unreachable NaN, iterative-vs-fast `0.1 MOA`
  (`test/source/solve_angle_test.cpp`, `test/source/lob_inverse_test.cpp`).
- The tolerance parameter already exists on `FastSolveAngle`/`SolveAngle`
  (default `0.01 MOA`) — refinement studies use it directly; no API change
  needed for tolerance tightening. Step refinement goes through public
  `Builder::StepSize`.

### 4.6 Drag, atmosphere, wind, spin (sensitivity inputs, §9)

- Drag: 87-pt BRL tables (`source/tables.hpp`) → 16 PCHIP knots → 60 coefs
  (`source/splines.hpp`), `constexpr` G1/G2/G5/G6/G7/G8. Single-BC path bakes
  `1/(BC·conversion)` into `drags[60]`; custom table / BC-bands / native coefs
  set effective BC=1 (bands keep atmosphere factor). `CurveView::Eval` clamps
  Mach outside `[0, 5]`. Spline budget `5e-3` max abs over 5000-pt truth grid.
  BC-bands: `S(M) = PCHIP(1/BC | M)` flat-padded at 0/5, `Cd = S·Cd_std`,
  exact at band Machs, constant outside (`docs/pages/bc/transformation.md`).
- Atmosphere: ISA + Huang saturation + McCoy humidity corrections, evaluated
  at `Build()` into `speed_of_sound`, `drag_coeff = ρ·π/(8·BC·144)`-family
  (`source/calc.hpp` `CalculateCdCoefficient`), `k_lapse`
  (`source/lob_builder.cpp` `BuildEnvironment`/`BuildDynamicDensity`).
  Constants in `source/constants.hpp` (ISA sea-level, lapse, exponents, Army
  `0.0751265` vs ICAO `0.0764742`, `Ω = 7.292115e-5`).
- Wind: uniform (`WindHeading[Deg]` + `WindSpeed*`) vs direction+magnitude
  profile (`WindProfile` with per-point `heading_deg`/`speed_mph`/`height_ft`,
  `LOB_WIND_POINTS 8`, range 0 first, strictly increasing whole feet,
  `WindShearExponent ∈ [0,1]` default 0 opt-in, Hellmann power-law scaling).
  Authoritative contract: `docs/superpowers/specs/WIND_INTERFACE_SPEC.md`.
  In brief — input (`LobWindPoint`, friendly doubles + measurement heights)
  is split from storage (`LobWindNode`, `uint32` range + frame-resolved
  `double` x/y/z in fps, heights absent);   `Build` resolves heading→Cartesian,
  normalizes each point to the fixed 1-ft reference (`kWindReferenceHeightFt
  = 1.0`, `source/constants.hpp`; `f = (1/h_meas)^α`,
  `NaN` height = measured at reference, factor exactly 1), and bakes range-
  angle pitch once (`Wx=H·cos th, Wy=−H·sin th, Wz=Z`, lateral untouched);
  `GetWind` (`source/wind.hpp` inline fast path + `source/wind.cpp` cold
  path) does a stateless muzzle-anchored scan (≤7 compares, no cached index),
  per-component lerp, end clamping, and per-query altitude scaling against
  the shot-parallel plane (`z_agl = Y/cosθ + 1` via gravity,
  `z_eval = clamp(z_agl, 1, 300)`, `S = z_eval^α ≥ 1`, one `pow`, skipped
  exactly at `α = 0`). Uniform input is stored as a single-point profile
  (`wind_count == 1`, last-wins both directions); `wind_count == 0` at query
  solves as calm. Validation: count 1..8 (0 → `Invalid`, >8 → `TooLong`),
  `[0].range == 0` + strictly increasing else `NotMonotonic`, finite
  range/heading/speed, `NaN`-or-positive heights, whole-foot `uint32` ranges,
  finite shear in `[0,1]` — else `kLobErrorWindProfile*` (`source/
  lob_builder.cpp` `BuildWind`/`ValidateWindProfile`). Tail nodes zeroed so
  identically-built contexts compare equal. Wind enters only as `v−w` in
  drag + muzzle `w_z` (`wind_nodes[0].z_fps`, incline-proof) in jump; the
  zero search integrates through the same query with no special handling.
  Query is `const noexcept`, allocation-free, trig-free. Typical shear
  exponents when enabling: water 0.10, grassland 0.143, farmland 0.20,
  suburban 0.25, forest/urban 0.30 (`include/lob/lob.h`).
- Coriolis: needs both `AzimuthDeg` + `LatitudeDeg`, else zeroed; `2Ω` terms
  in `BuildCoriolis`, applied in `GetDvDt`.
- Spin/jump: automatic Boatright (full geometry + Mach-1.2 supersonic
  Gauss-Legendre integral) → Litz (Miller `sg` only) → zero fallback.
  Drift applied post-solve to `deflection`; jump added to launch angle for
  every solve including zero/inverse (`source/lob_solve.cpp`,
  `source/solve_angle.hpp`, `source/boatright.hpp`, `source/litz.hpp`,
  `source/gauss_legendre.hpp`).
- Gravity rotated by `RangeAngleDeg`; `BuildOptions` merges
  `minimum_speed`/`minimum_energy`; optic default 1.5 in.

### 4.7 Current tests, benchmarks, CI, docs (reuse, do not duplicate)

- Tests: `test/CMakeLists.txt` — `LOB_TEST_SOURCES` (public-API integration:
  `lob_api/builder/coriolis/cwaj/env/inverse/spin_drift/test/wind/...`,
  `c_api`) + `HELPER_TEST_SOURCES` (white-box: `boatright/calc/cartesian/`
  `eng_units/gauss_legendre/helpers/litz/ode/solve_angle/solve_step/splines`).
  **Shared builds compile only `LOB_TEST_SOURCES`** (internal headers
  unavailable); static builds compile both. `gtest_discover_tests`.
  Utilities: `test/source/testing.hpp` — `SolutionTolerances`
  (velocity/energy/MOA/inch/TOF), `OutputNearMatcher` (elevation/deflection
  compared in **MOA space** via `InchToMoa`), `VerifySolutions`,
  `VerifySolutionDifferences`, `SetupTestBuilder` (BC 0.425, Ø0.308 in,
  180 gr, 2700 fps, zero 3.38 MOA).
- Reference trajectories: `test/source/lob_env_test.cpp` — G7 BC 0.232,
  155 gr, 2800 fps, zero 3.66 MOA, 12 ranges 0–3000 ft; ICAO / +4500 ft /
  hot-low-p / barometer-offset / humidity / weather-station cases; tolerances
  `±1 fps, ±5 ft·lbf, 0.1 MOA, ±0.01 s` to 1000 yd.
- Benchmarks: `benchmark/ode.cpp` (Euler/Heun/RK4 cost-matched vs RK4
  `dt=1e-5` reference), `benchmark/loblerp.cpp` (LobLerp vs CurveView),
  `benchmark/cachegrind/lob_bench.cpp` (7 cases: build_basic/full/
  custom_table/boatright/zero_search, solve_basic/inverse; `--verify` gate;
  `CG_MAX_REGRESSION_BP` compare in CI, no hard gate beyond summary).
- CI (`.github/workflows/ci.yml`): `lint` (clang-format, codespell),
  `coverage` (ci-coverage preset → codecov), `sanitize` (ASan+UBSan),
  `test` (macos/ubuntu/windows × shared/static), `lobber`, `docs`
  (Doxygen → gh-pages), `benchmark` + `track` (cachegrind instruction counts).
- Docs: `docs/pages/` Doxygen site (`overview`, `api/*`, `ballistic_model/*`,
  `numerical_methods/*`, `bc/*`, `design/*` incl. `shared_solver`,
  `reference/constants`, `validation/overview`). Validation page already states
  the evidence-before-synthesis policy and lists gaps (gated lapse, no field
  data, no perf gate, knot-derivation note) — this spec fills the methodology;
  it does not restate the physics.
- Example: `example/lobber` JSON stdin/stdout CLI (JSON precedent for artifact
  formats). `result/bin/` is build output, not validation storage.

---

## 5. Design goals

1. **Isolate `ε_num` first.** Every validation number decomposes into
   numerical vs input vs model contributions; numerical floor is measured, not
   assumed.
2. **Rank drivers, don't tabulate trivia.** Sensitivity exists to select the
   inputs that matter for budgets and MC, with a Pareto output.
3. **No invented numbers.** Tolerances come from measured floors and cited
   references; input uncertainties come from the user/measurement process or
   stated distributional assumptions under review (§21).
4. **Deterministic core untouched.** All stochastic/expensive machinery lives
   outside `lob_lob`, behind the public C ABI.
5. **CI-safe by construction.** Fast deterministic gates in CI; expensive /
   stochastic studies offline with curated baselines checked in.
6. **Envelope claims only.** Every accuracy-adjacent statement names the
   envelope (range, regime, atmosphere, wind, model path) and cites the case
   IDs and artifact hashes.
7. **Incremental.** Each phase (§20) delivers standalone value and its own
   acceptance gate; later phases consume earlier artifacts, never redesign
   them.
8. **Reuse first.** Extend `test/source/`, `testing.hpp`, `benchmark/`,
   Doxygen pages, JSON precedent — no parallel test harness, no new units
   system, no new spline code.

---

## 6. Non-goals

- No change to Heun, step control, angle iteration, spline knots, physics, or
  public API in this framework. If studies motivate a change, it becomes a
  separate proposal.
- No universal accuracy percentage, no marketing precision claim.
- No MC inside `lob_lob`, no `<random>` / threads / I/O / heap in the core,
  no new core dependency.
- No adaptive stepper, no higher-order production integrator, no surrogate
  model — unless Phase 1–6 evidence forces a follow-up spec.
- No field-data collection campaign (the spec defines the case format that
  would hold such data, §10.4).
- No complicated data pipeline (§15 keeps CSV + JSON + checked-in baselines).

---

## 7. Validation methodology (overview)

```
Phase 1 (§8)    Phase 2 (§9)     Phase 3 (§10)    Phase 4 (§11)    Phase 5 (§12)
REFINE       →  PERTURB       →  COMPARE       →  BUDGET        →  SAMPLE
step/tol        central diffs     vs reference     GUM linear      MC around
ladder          OVAT + Pareto     per-envelope     + flags         deterministic
   │                │                 │                │               │
   └──── ε_num ─────┴── c_i=∂y/∂xi ───┴── δ_ref ───────┴── u_c(y) ─────┴── u_MC(y)
                                        │
                              Phase 6 (§13): is |Δ_effect| ≫ u_total ?
```

- **Verification chain:** §8 (does the code solve its own equations
  correctly?) → §9 (how does the solution move with inputs?) → §10 (does the
  equation set match a reference?) → §11/§12 (what is the total prediction
  uncertainty?) → §13 (does an effect clear that uncertainty?).
- Each layer records machine-readable artifacts with provenance (§15–§16);
  CI enforces only the deterministic layers (§17).
- Terminology gate: any report using "error" must qualify it as `ε_num`,
  `δ_ref/δ_model`, or `|y − y_ref|` total residual with the decomposition
  stated or explicitly unknown.

---

## 8. Numerical convergence / error-floor methodology

Goal: a statement of the form *"at configuration C, on envelope E, the
numerical contribution is below X (per output, with metric)"* — never *"lob
is accurate to X"*.

### 8.1 Parameters varied (and only these)

| Knob | How | Values | Notes |
|---|---|---|---|
| `StepSize` (public `Builder::StepSize`, `uint16_t` in) | Halving ladder at fixed everything else | `36 → 18 → 9 → 4 → 2 → 1` (`0` = 36 default; ladder uses explicit inches) | Integer-only; 9→4 breaks exact halving — record actual `Δx` per run; cost ∝ `1/Δx`, finest ≈ 36× default → offline only |
| Angle tolerance (existing `SolveAngle`/`FastSolveAngle` param) | Tighten at fixed step | `0.01 → 0.003 → 0.001` MOA | Default path uses 0.01; 0.001 cross-checked to `±0.01 in` in existing tests |
| Density path | Stratify, never mix | `Fast*` (forward/zero) vs `DsDx` (inverse tail) | Inverse studies split at `drop ≷ 100 ft`; report both branches |
| Spline | Isolate, not part of step ladder | Perturb `drags[60]` within `5e-3` budget (§8.6) | Separates drag-table error from integrator error |

Do **not** vary: physics inputs (that's §9), `kMaxIterations`/`±45°` bounds
(fixed design constraints; reaching them = convergence failure, §8.4),
compiler flags (that's §16 platform coverage).

### 8.2 Outputs compared and metrics

- Compare per-range: `elevation` (in, forward; MOA, inverse), `deflection`
  (same), `velocity`, `energy`, `time_of_flight`, plus inverse `elevation/
  deflection` in MOA. Convert linear↔angular only via `LobInchToMoa` at the
  same range (reuse `testing.hpp` convention).
- Metrics per output, per range:
  - **Absolute difference** `|y_h − y_{h/2}|` is primary.
  - **MOA-space difference** for elevation/deflection (range-compensated).
  - Relative difference **only** for strictly-positive far-from-zero
    quantities (`velocity`, `energy`, `TOF` at range > 0); **never** for
    `elevation/deflection` near the zero crossing.
  - Near-zero rule: if `|y| < floor_y`, report absolute difference against
    `floor_y` where `floor_y` = `max(0.5 LSB_q, 0.01 in, 0.01 MOA-equiv)` as
    applicable; state the floor in the artifact. Rationale: at 300 ft the
    reference elevation is `0.00 in` — any relative metric divides by zero.
- Successive-difference (Cauchy) estimator with observed order:
  `p ≈ log2(|y_h − y_{h/2}| / |y_{h/2} − y_{h/4}|)`; Richardson-extrapolated
  reference `y_extrap` when `p` is stable in `[1, 3]`; otherwise report the
  finest-pair difference as a **resolution bound**, not an error.
- Finest solution (1-in step, 0.001 MOA) is the internal reference for the
  ladder — it is a resolution anchor, not truth.

### 8.3 Convergence criteria (per case, per output family)

- **Monotone decrease:** `|Δ_{h/2}| ≤ |Δ_h|` across two successive halvings,
  up to quantization chatter (§8.5).
- **Order plausibility:** observed `p ∈ [1, 3]` for smooth mid-flight outputs
  (Heun theory = 2; allow degradation near Mach-knot crossings, wind joints,
  terminal interpolation).
- **Floor reached:** `|y_{h/2} − y_{h/4}| ≤ max(LSB_q, tol_report)` where
  `tol_report` is the reporting granularity (e.g. existing `0.1 MOA`,
  `±1 fps`); further refinement is declared non-informative for that output.
- A case passes when all primary ranges meet monotone + floor, or deviations
  are dispositioned (e.g. "TOF at 3000 ft still falling at 1-in step —
  envelope capped at 2400 ft for 0.01 s claims").

### 8.4 Detecting convergence failure (vs model disagreement)

Failure signatures (numerical, fix by config/envelope, not by physics debate):

- Non-monotone ladder, `p ≤ 0` or `p > 4`, oscillation with refinement.
- `SolveAngle` returns NaN / hits 10-iteration cap / `±45°` bound at any rung
  (record iterations-to-converge per range; cap-hit = failure, not "large
  adjustment").
- Fall-short / tumble / `max_time` / `minimum_speed` interpolation engages at
  different ranges per rung (branch discontinuity — split the envelope at the
  branch, do not difference across it).
- `Fast*` vs `DsDx` paths diverge on the same inputs beyond the floor (this is
  a **path discrepancy**, reported separately from step convergence; see §13
  for the lapse effect test).
- Model disagreement looks different: ladder converges cleanly (monotone,
  plausible `p`, floor reached) but the converged value disagrees with the
  reference — route to §10, never "refine harder".

### 8.5 Quantization, clamping, and branch hazards (load-bearing)

- `velocity (U16) / energy (U32) / range (U32)` truncation imposes a
  rectangular ±0.5 LSB chatter floor. Ladder differences below 1 LSB on those
  channels are **uninformative** — compare in `double` internally where the
  harness can (white-box `DsDxCore` probes in static builds) or widen the
  reporting floor to 1 LSB and say so.
- Last-step clamping (`ComputeStep`) means ranges that are exact multiples of
  the step show different local error than off-grid ranges — the canonical
  range set (§8.7) deliberately mixes on/off-grid ranges per rung.
- `CurveView` Mach clamp at `[0, 5]`, wind-node joints, `vx ≤ 0` zeroing, and
  the `drop>100 ft` path switch are all non-smooth points: expect local order
  reduction; do not fit a global `p` across a branch switch.

### 8.6 Spline / drag-table resolution (separate from step ladder)

- Reuse the existing `5e-3` budget and 5000-pt truth grid
  (`test/source/splines_test.cpp`, `SplineOptimization::BaselineAccuracyBudget`).
  Convergence artifact records: max-abs `Cd` error per G-curve, plus a
  trajectory-level spot check (reference case at default step with `drags`
  perturbed by `±5e-3` in the transonic knot band) to map `Cd` error → inches/
  MOA. This keeps "drag-table resolution" distinct from "integrator error".

### 8.7 Representative cases (minimum set; all reachable, all deterministic)

Reuse existing fixtures verbatim where possible (no new magic trajectories):

1. **C0 baseline** — `SetupTestBuilder` (BC 0.425, 2700 fps, zero 3.38 MOA):
   short/mid sanity, matches `benchmark/cachegrind` `solve_basic` scale.
2. **C1 ICAO reference** — `lob_env_test.cpp` fixture (G7 BC 0.232, 155 gr,
   2800 fps, zero 3.66 MOA, optic 1.5 in): 12 ranges 0–3000 ft. The primary
   ladder case (covers zero crossing, supersonic→transonic tail).
3. **C2 altitude** — C1 + 4500 ft site; **C3 hot/low-p** — C1 + 100 °F /
   25 inHg; **C4 humid** — 29 inHg / 75 °F / 80%.
4. **C5 wind** — C1 + 10 mph crosswind (kIII) + headwind/tailwind pair
   (uniform path; asserts single-point profile bit-identity per
   `SinglePointEqualsUniform`);
   **C6 profile** — 3-pt direction+magnitude profile
   (`{0,90°,5,NaN},{1500,90°,10,NaN},{3000,90°,10,NaN}`-family, cf.
   `lob_wind_profile_test.cpp` `kTwoPoint`/`ConfiguredBuilder` G1 BC 0.372,
   2720 fps) with (a) shear `α = 0` (verbatim-storage control) and
   (b) shear `α = 0.25` with explicit measurement heights (scaled path);
   **C6b incline+shear** — C6(b) + `RangeAngleDeg = 15°` (pitch-baked nodes,
   tilted-datum scaling); **C6c ceiling** — high-arc scaled solve exercising
   the 300-ft clamp (completion + sign oracle, cf.
   `CeilingClampCompletesHighArcSolve`). Refinement-identity
   (`ProfileRefinementMatchesCoarseSolve`) and clamp-inertness
   (`ClampedTailMatchesExplicitExtension`) bit-exact oracles from the wind
   test file are reused as convergence smoke checks.
5. **C7 Coriolis** — C1 + lat 43.04° × azimuths N/S/E/W.
6. **C8 spin** — Litz path (Ø0.308, L1.215, 168 gr, twist 10) + full Boatright
   geometry (meplat 0.065, base 0.242, nose 0.690, tail 0.140, RtR 0.90);
   non-zero jump required (`assert jump ≠ 0`).
7. **C9 inverse pair** — C1/C5/C8 through `SolveInverse` at 300/600/1000 yd
   (covers `FastSolveAngle` branch) + 2000–3000 yd tail (covers `SolveAngle`
   `drop>100 ft` branch; assert branch taken via forward `elevation < −1200`).
8. **C10 BC-bands/custom** — bands `0.20@2000 / 0.30@2500 / 0.40@3000` fps
   (`bc_transformation.md` values) + G6 custom-table path.
9. **C11 angle-stress** — zero-by-distance 100 yd + tight-tolerance rerun;
   unreachable weak case (BC 0.05, 500 fps @ 4000 ft) reserved for
   failure-handling checks, excluded from ladders.

Each case pins: builder calls verbatim, `step_size` ladder, angle tolerance,
density path, `pranges`, expected branch flags. New cases need human approval
(§21); the list above is the default proposal.

### 8.8 Recording and regression detection

- Per-case artifact: `convergence_{case}.json` (§15 schema) with per-rung
  outputs, pairwise deltas, observed `p`, floor verdict, iterations, cost
  (steps = range/Δx proxy + wall ns where available), provenance.
- CI gate (§17): a **small, fast subset** (C1 @ 36→18→9 in, 4 ranges; C9
  inverse @ 2 ranges; angle-tightening single check) asserts monotone +
  ceiling bounds derived from the first measured run plus margin (e.g. assert
  `|Δ_{18→9}| ≤ |Δ_{36→18}|` and `|Δ_{18→9}| ≤ ceiling_case`, where the ceiling
  is checked in, not invented). Full 36→1 ladders run offline; their curated
  floors become the checked-in ceilings.
- Numerical regression = a previously-passing ladder rung changes beyond its
  ceiling at fixed inputs/version → bisect step vs physics change; never
  "update the golden value" without a §10 disposition.

---

## 9. Sensitivity methodology

Model: `y = f(x_1 … x_n)` where `f` = `Build + Solve` (or `SolveInverse`) at
fixed numerical config (default 36-in step, 0.01 MOA — the deployed config, so
sensitivities include deployed resolution). Goal: **rank dominant drivers**
for budgets (§11) and MC (§12), not a derivative table.

### 9.1 Which parameters are meaningful (perturb these; nothing else)

**Physical (Builder-level; rebuild per evaluation):**

- `InitialVelocityFps`, `BallisticCoefficientPsi` (single-BC path),
  `BCVelocityBands` band values (perturb one band at a time; velocities fixed),
  custom-table `Cd` scale (uniform ±%, not per-knot — per-knot is §8.6).
- `DiameterInch, LengthInch, MassGrains, TwistInchesPerTurn` (via `sg`/drift/
  jump; perturb jointly-aware — see interactions).
- `ZeroAngleMOA` **or** `ZeroDistanceYds` (whichever the case uses; never both),
  `ZeroImpactHeightInches`, `OpticHeightInches`, `RangeAngleDeg`.
- `AltitudeOfFiringSiteFt, AirPressureInHg, AltitudeOfBarometerFt,
  TemperatureDegF, AltitudeOfThermometerFt, RelativeHumidityPercent`.
- Wind (uniform): `WindSpeedMph/Fps` at fixed heading, `WindHeadingDeg` at
  fixed speed (separate the magnitude and direction channels — never a joint
  Cartesian perturb). Wind (profile): per-station `speed_mph` at fixed
  heading and `heading_deg` at fixed speed, one station at a time, muzzle
  first; per-station `height_ft` **only when shear `α ≠ 0`** (at `α = 0` the
  height factor is exactly 1 by construction, so heights are provably inert
  — prune them, do not perturb); `WindShearExponent` (one-sided at 0, §9.2).
  Node `range_ft` stations are **excluded** (placement changes interpolation
  topology — scenario-swap, cf. categorical rule below — not a derivative).
  Clock headings convert losslessly (`30°·c`); profile headings accept any
  finite degrees (shared `HeadingDegToRad` conversion; wrap handled per
  §9.2 step 4).
- `LatitudeDeg, AzimuthDeg` (jointly — Coriolis needs both; single-sided
  perturbation with the other fixed at a non-degenerate value, e.g. 45°/180°).
- `MinimumSpeed/Energy, MaximumTime` — **excluded** from sensitivity (terminal
  guards, not physics); tested for branch stability only (§8.4).
- Categorical inputs (`BCDragFunction` G1…G8, `BCAtmosphere`, drag-source
  selection) are **scenario swaps**, not derivatives — compare C1-G7 vs C1-G1
  as a model-form spot check (§10.5), never `∂y/∂(G7)`.

**Forbidden:** perturbing derived `Context` fields (`drag_coeff`,
`speed_of_sound`, `k_lapse`, `coriolis.*`, `stability_factor`,
`spindrift_factor`, `aerodynamic_jump`, `drags[]`) directly — they are
`Build()` outputs; perturbing them breaks the builder contract and double-
counts. Perturb Builder inputs; rebuild; solve.

**Numerical inputs** (`StepSize`, angle tolerance) belong to §8, never to the
sensitivity Pareto.

### 9.2 Perturbation strategy and magnitude

- **Central differences** as default:
  `∂y/∂x ≈ (f(x+h) − f(x−h)) / 2h`. One-sided only when a bound blocks the
  symmetric step (e.g. humidity at 0/100%, shear at 0/1, altitude ceiling) —
  record asymmetry and use the one-sided value flagged.
- **Step-size selection (procedure, not a constant):** for each `(case, input)`:
  1. Start from a scale-aware guess `h_0 = max(√eps_d · |x|, q_x)` where
     `eps_d ≈ 2.2e-16` and `q_x` is the input quantum (`velocity 1 fps`,
     `BC 1e-6`, angles `1e-3°`, ranges n/a — ranges are the axis, not an
     input).
  2. Evaluate at `h_0, 2·h_0, h_0/2`; require the two derivative estimates to
     agree within 20% **and** `|f(x+h) − f(x−h)| > 10 × noise_floor`, where
     `noise_floor` = the §8 finest-pair `|Δ|` for that output + 1 LSB where
     quantized.
  3. If chatter dominates, grow `h` (×2…×8); if nonlinearity dominates
     (monotone drift of the estimate with `h`), shrink `h`. Record the
     accepted `h` per `(case, input, output)` — the artifact is self-
     documenting.
  4. Angular inputs wrap: uniform/profile heading and azimuth steps stay
     within `±5°` and never cross the `0/360` warrant silently — normalize
     the delta to `(−180, 180]`. Profile headings need no explicit mod: any
     finite value resolves through the shared `HeadingDegToRad` conversion
     (periodicity handles multi-turn values); keep steps small so libm
     argument-reduction never enters the error budget. Height steps (shear
     path only) stay strictly positive and away from the 1-ft floor / 300-ft
     cap — crossing a clamp makes the response piecewise, which is a
     scenario comparison (§13), not a derivative.
- **Canned starting magnitudes** (seeds for the procedure above, not fixed
  constants): velocity `±10 fps`, BC `±1%`, pressure `±0.1 inHg`, temperature
  `±2 °F`, humidity `±5 pp`, wind speed `±1 mph`, wind heading `±2°`,
  measurement height `±1 ft` (shear path only), shear exponent `+0.02`
  (one-sided from 0), zero angle `±0.05 MOA`, optic `±0.1 in`, mass `±1 gr`,
  diameter/length `±0.002 in`. The procedure in the previous bullet adjusts
  from here.

### 9.3 Normalization, outputs, nonlinearity, interactions

- Store **raw** `∂y/∂x` (units: output-unit per input-unit) plus two
  normalized views: **semi-elasticity** `S = (∂y/∂x)·u_ref(x)` (inches/MOA per
  reference input uncertainty — directly budgetable) and **elasticity**
  `E = (x/y)·(∂y/∂x)` only when `|y| > floor_y` (§8.2), else NaN with reason.
- Outputs per evaluation: forward `elevation/deflection` (in **and** MOA),
  `velocity`, `TOF`; inverse `elevation/deflection` (MOA). Report per-range
  curves, not single-range scalars — drivers cross over with range (velocity
  dominates short, BC/wind long).
- Nonlinearity check: recompute each sensitivity at `±h` and `±2h`; flag
  `|S(2h) − S(h)| / |S(h)| > 0.25` as nonlinear-for-budget (route to MC, §12,
  instead of linear propagation).
- Interactions: OVAT screens first; then **targeted 2-factor spot checks**
  only on the top-3 Pareto pairs per case (e.g. velocity×BC, wind×range-angle
  — the pitch bake `Wx=H·cos th, Wy=−H·sin th` makes along-track wind and
  incline a known-coupled pair — shear×measurement-height, pressure×
  temperature) via corner evaluations `f(x±h, z±h)`; full factorial
  is explicitly out of scope. Record interaction residue
  `f(x+h,z+h) − f(x+h) − f(z+h) + f(x)` alongside the OVAT terms.

### 9.4 Noise vs genuine sensitivity; storage; guidance

- Decision rule: a sensitivity is **genuine** iff the symmetric response
  exceeds `10 × noise_floor` at the accepted `h` **and** has consistent sign
  at `h` and `2h`. Below that it is recorded as `below_floor` with the floor
  value — never as zero, never as a small number that downstream code treats
  as informative.
- Artifacts: `sensitivity_{case}.json` (per-input accepted `h`, raw + `S`,
  nonlinearity flag, interaction residues) + `sensitivity_{case}.csv`
  (range × input × output matrix for plotting). Pareto section lists top
  drivers per output family with cumulative-share cutoff (default 90%).
- Downstream contract: §11 consumes `S` and flags; §12 samples the Pareto-top
  inputs as MC dimensions; §13 uses the smallest genuine effect as the
  significance yardstick. Inputs ranked `below_floor` on all outputs are
  excluded from MC dimensions by default (documented, reversible).

---

## 10. Reference-validation and model-discrepancy methodology

### 10.1 What counts as a reference (in priority order)

1. **BRL drag tables via JBM** (already vendored, `source/tables.hpp`) —
   spline reproduction budget `5e-3` (§8.6). Provenance: JBM Ballistics BRL
   tables; record table checksum + knot set.
2. **Borrowed reference trajectories** (`test/source/lob_env_test.cpp`
   expected vectors + McCoy ISA treatment + Huang saturation) — the current
   regression anchor. Provenance per case: McCoy pp. 166–168, Huang
   (J. Appl. Meteor. Climatol.), ISA constants (`ref_constants`).
3. **Paper formulas** — Litz (4th ed., p.422 jump; `TOF^1.83` drift) and
   Boatright & Ruiz (jump + spin-drift precession/nutation). Validated as
   "tests pass against the papers", not as field truth.
4. **External ballistics engines / range truth** — placeholder category.
   No such data is vendored today (acknowledged gap in
   `validation/overview.md`); the case format (§10.4) is ready for it, with
   licensing/provenance fields mandatory before any claim cites it.

### 10.2 Comparison procedure (fixed inputs, fixed config)

- Freeze: builder calls, `step_size`, angle tolerance, density path, `pranges`,
  code version (§16). Solve; compare per-range with the **same metric family
  as §8.2** (absolute + MOA-space; relative only for positive-far-from-zero).
- Report the **total residual** `r = y_lob − y_ref` **and** the decomposition:
  `r = ε_num + δ + η_ref`, where `ε_num` comes from the §8 floor for that
  case/config, `η_ref` is stated reference uncertainty (often "unstated —
  treated as unknown, see §10.6"), and `δ` is the residual discrepancy after
  subtracting the measured floor. If `|r| ≤ ε_floor`, the verdict is
  **"consistent within numerical resolution"** — not "validated accurate".
- Inverse comparisons in MOA; forward in inches + MOA dual view (reuse
  `OutputNearMatcher` MOA-space convention). TOF/velocity/energy compared in
  native units with existing tolerances as the initial reporting granularity,
  tightened only by §8 evidence.

### 10.3 Envelope discipline (anti-overgeneralization)

- Every reference case carries **envelope tags**: range band (e.g. 0–300 /
  300–1000 / 1000+ yd), velocity regime (supersonic / transonic / subsonic at
  target), atmosphere branch (ISA / altitude / hot-low / humid), wind branch
  (calm / uniform / profile-unscaled-α0 / profile-scaled-α>0 + α value /
  incline×wind / ceiling-clamp exercised), spin path (off / Litz / Boatright),
  density path (`Fast*` / lapse-scaled inverse tail), drag source (G* / bands /
  custom).
- A validation claim names the **envelope cell**, the case IDs covering it,
  and the worst residual in the cell. Cells with one case are labeled
  **provisional**; cells with zero cases are **unclaimed** — the report
  renders an explicit coverage matrix with gaps visible. The scaled-profile
  + incline combination is always its own cell (tilted datum + pitch bake
  change the field structurally); unscaled-profile results never cover it.
- Forbidden: extrapolating a short-range ISA result to long-range humid, or a
  `Fast*` forward result to the lapse-scaled inverse tail, or a G7 result to
  G1, or an `α = 0` profile result to a sheared atmosphere. The `drop>100 ft`
  path gate is an envelope boundary by construction.

### 10.4 Case record format (source-controlled, small)

Each `reference_{id}.json` holds: `id`, `provenance` (source, page/URL,
access date, checksum where applicable), `builder` (verbatim setter list +
units), `solver_config` (step, tolerance, density path), `ranges_ft`,
`expected[]` (range/velocity/energy/elevation/deflection/tof + per-field
`u_ref` or `"unknown"`), `envelope_tags[]`, `status`
(`anchor | provisional | deprecated`), `supersedes`. Expected vectors for the
existing env tests are transcribed verbatim (mechanical move, reviewed diff);
no values are re-tuned in the transcription.

### 10.5 Model-form spot checks (planned discrepancies)

- G7-vs-G1 mismatch run, single-BC vs BC-bands on the same bands, Litz vs
  Boatright on full geometry, lapse on/off (`Fast*` vs `DsDx` same inputs),
  Coriolis/spin/jump on/off. These quantify **known structural choices** and
  feed §13 effect sizes; they are reported as `δ_structural`, never as
  `ε_num`.

### 10.6 Missing-evidence rules

- Reference uncertainty unstated → `η_ref = unknown`; the budget (§11) carries
  it as an explicit unknown row, and the envelope cell caps at **provisional**.
- No field/radar data → long-range cells stay provisional regardless of
  residuals; the spec's gap statement in `validation/overview.md` remains
  until data arrives.
- Per-step lapse gating stays visible: any cell whose ranges cross `drop =
  100 ft` reports both sides of the gate.

---

## 11. Uncertainty-budget methodology

### 11.1 Combined-uncertainty model (GUM, with stated limits)

For output `y = f(x)`, with sensitivity coefficients `c_i = ∂y/∂x_i` from §9
at the accepted `h`, and standard uncertainties `u(x_i)` supplied per §11.3:

```
u_c²(y) = Σ c_i²·u²(x_i) + 2·Σ_{i<j} c_i·c_j·u(x_i,x_j) + u²(ε_num) + u²(δ) + u²(η_ref)
```

- Covariance terms default to zero (independence) **only** when inputs come
  from independent measurements; pressure/temperature/altitude triples and
  band-BC sets carry documented correlations or an explicit
  "independence-assumed — see risk" flag (§22).
- `u(ε_num)` = rectangular/Normal treatment of the §8 floor (default:
  rectangular `floor/√3`, stated in the artifact).
- `u(δ)`, `u(η_ref)` = discrepancy/reference rows from §10 (often
  "unknown" — carried visibly, never zero-filled silently).
- Output: `budget_{case}_{output}.json` with per-source rows
  (`source, u(x_i), c_i, contribution, share%`), combined `u_c`, effective
  degrees of freedom where known, and the assumption list. No coverage factor
  `k` is applied without stating it (`k=1` default; `k=2` intervals only with
  the normality justification recorded).

### 11.2 Where linear propagation fails (explicit flags → MC)

Linear budgets are **not used** (MC §12 required instead) when any holds:

- Nonlinearity flag from §9.3 tripped on a top-3 contributor.
- Output near a branch: zero crossing, fall-short/tumble/`max_time`/
  `minimum_speed` interpolation active within the `±u` band, angle-solver
  iteration count changes across the band, `±45°`/cap proximity.
- Transonic target Mach (drag slope discontinuity across knots) with a driver
  that moves Mach across a knot.
- Quantized channels (`velocity`/`energy`) where `u_c` < 1 LSB (report
  "below reporting resolution").
- Categorical swaps (G-curve, drag source, density path) — budgeted as
  discrete scenarios, never linearized.

### 11.3 Which `u(x_i)` are known vs assumed (no invented values)

- **Measurably known (user/supplier provides):** chronograph velocity spread,
  pressure/temperature/humidity sensor specs, optic-height tolerance, range
  measurement error, wind-meter error at the station. The budget schema
  **requires** a `u(x_i)` + provenance per row; missing rows block the
  combined number and render as "incomplete budget".
- **Assumptions requiring human sign-off (§21):** BC scatter / band
  uncertainties, custom-table `Cd` uncertainty beyond the `5e-3` spline
  budget, wind-aloft vs station error, profile-interpolation error (incl.
  node-placement as scenario, not `u(range_ft)`), shear exponent uncertainty,
  measurement-height uncertainty (applies **only** when `α ≠ 0`; at `α = 0`
  heights are inert and the row is omitted with reason, not zero-filled),
  heading-wrap correlation for multi-station profiles drawn from one sensor,
  transonic drag-form error. The spec defines the rows
  and the elicitation format; it does **not** fill them. Example placeholders
  are banned from checked-in artifacts — templates ship with `"u": "TBD —
  human input required"` and fail closed.
- **Numerically known:** `u(ε_num)` from §8; angle-tolerance contribution via
  the §8.1 tightening pair (0.01 vs 0.001 MOA mapped to inches/MOA).

---

## 12. Monte Carlo uncertainty-propagation methodology

Architecture (non-negotiable):

```
   input distributions  (user assumptions, §12.1, versioned JSON)
            │
            ▼
   deterministic lob solve per sample (public API only, fixed config)
            │
            ▼
   output distribution + summaries (mean/σ/percentiles/correlations)
```

MC never modifies the solver, never lives in `lob_lob`, never runs on
embedded targets.

### 12.1 Input distributions and determinism contract

- Supported families: `normal(μ,σ)`, `uniform(a,b)`, `triangular(a,c,b)`,
  `fixed(v)` (degenerate — for toggling dimensions), `categorical({v:p})`
  for scenario weights only (G-curve choice as a weighted scenario, never a
  continuous perturb). Truncation at Builder OOR bounds is mandatory
  (resample or clip-and-record — record which; default resample, cap 100
  retries then fail the sample loudly).
- Default mapping guidance (assumptions, human-approved per study): sensor
  errors → normal (or uniform when only bounds are known — state it);
  environmental variability over a session → normal/uniform per the data
  sheet; BC scatter → lognormal-or-normal per supplier evidence (no default
  imposed by this spec); wind: sample in **input space** — `speed_mph`
  magnitude plus `heading_deg` direction (wrapped normal / von Mises reduced
  mod 360, never Cartesian x/z, so draws respect the heading convention and
  the zero-speed trig skip) — station-anchored profiles jitter per-station
  with stated cross-station correlation (one sensor = correlated); heights
  jitter only when `α ≠ 0`; `α` itself sampled on `[0,1]` (truncated,
  one-sided mass at 0 allowed as a scenario weight).
- Deterministic components: solver config, drag tables, constants, envelope
  tags — fixed per MC run and hashed into the run ID. Stochastic components:
  only the declared input draws. One sample = one `Build + Solve(+Inverse)`
  through the public API; build failures (OOR draws) are counted and reported,
  not silently dropped.

### 12.2 Sampling, RNG, seeding, reproducibility

- RNG: `std::mt19937_64` (C++14 `<random>`, no new dependency), one engine per
  run, seed `uint64_t` recorded; distributions via `<random>` types only.
  Alternative engines require a spec amendment (no `rand()`, no time-seeded
  runs, no thread-local unsynchronized engines).
- Reproducibility: seed + input-distribution JSON + code version + solver
  config fully determine the run. Parallel workers use pre-derived subseeds
  (`seed ^ worker_id` via splitmix64, documented) so worker count does not
  change the sequence.
- Pilot-then-scale for `N`: pilot `N=1024` estimates `σ` per primary output;
  scale to `N ≥ (z·σ / E_target)²` for target half-width `E_target`
  (default `E_target = 0.1·σ`, i.e. ~400 samples minimum for means; percentiles
  need more — see §12.3). CI smoke uses fixed `N=256` with a fixed seed and
  asserts only run-integrity + summary-schema, never tight statistical values.

### 12.3 Convergence of MC statistics and summaries

- Track running mean/`σ` + batch-means standard error; declare mean-converged
  when `SE < E_target` for all primary outputs. Percentiles `P5/P50/P95`
  (plus `P2.5/P97.5` for long-tail checks) via order statistics with
  binomial CI widths reported; tails (beyond P1/P99) are reported as
  "unresolved at N" unless `N ≥ 10⁵` offline runs justify them.
- Summaries per output: `n, seed, mean, sd, min, max, P2.5/P5/P50/P95/P97.5,
  P(out-of-envelope), P(build-fail), rank-correlations` (Spearman between each
  sampled input and each output — the MC cross-check on the §9 Pareto).
- Multimodality/branch check: histogram + branch counters (fall-short, tumble,
  cap-hit, path-switch counts). Any nonzero branch count splits the summary
  by branch — pooled statistics across a branch discontinuity are forbidden.

### 12.4 Cost, parallelization, placement (embedded separation)

- Cost is linear: `N × cost_single_solve` (measure `cost_single` from the
  cachegrind harness scale, `benchmark/cachegrind/lob_bench.cpp`). A 7-range
  forward solve at 36-in step is microseconds-to-milliseconds; `10⁵` samples
  is desktop-minutes — acceptable offline, absurd on MCU. State the measured
  `cost_single` in each MC artifact.
- Parallelize across samples (embarrassingly parallel): threads/processes on
  desktop only, deterministic subseeds (§12.2). No threading in the library;
  the MC runner links `lob::lob` as a client.
- **Placement:** new desktop-only component (proposed: `tools/lob_mc/` or
  `validation/mc/` — decision in §21; default recommendation `tools/lob_mc/`
  so it never confuses `test/` CI discovery), C++14, links installed
  `lob::lob`, reads distribution JSON, writes summaries (§15). Optional
  Python plotting consumes CSV/JSON outputs — no Python in the build, no new
  library dependency. The core `lob_lob` target gains zero sources, zero
  dependencies, zero `#include <random>`.

---

## 13. Signal-versus-uncertainty methodology

Question: does effect `E` (a model choice, correction, or refinement) produce
a change distinguishable from numerical resolution and prediction uncertainty?

### 13.1 Effect-size protocol

For fixed case + config, compute the paired difference:

```
Δ_effect(R) = y_with(R) − y_without(R)   per range, same seed/config
```

Candidate effects (minimum set): step refinement (36→9 in); angle tolerance
(0.01→0.001 MOA); lapse path (`Fast*` vs `DsDx` on the inverse tail);
Coriolis on/off; spin off/Litz/Boatright; jump on/off; single-BC vs bands;
G7 vs G1 (model-form reference); spline `±5e-3` nudge; single-point profile
vs uniform (expect `R_sig ≪ 1` — bit-identity oracle, guards the storage
merge); multi-point profile vs uniform; shear off/on (`α = 0` vs `α > 0`,
same heights); measured vs `NaN` heights at fixed `α > 0` (normalization
effect); tilted-datum vs flat-`h` scaling (model-form check on the
shot-parallel assumption); incline with/without pitch bake (unavailable as a
runtime toggle — compare inclined profile solve vs flat-fire solve corrected
for gravity only, cf. `CrosswindBlindToInclineAtSolve`); 300-ft ceiling clamp
on/off for high-arc cases (completion + sign as the oracle).

### 13.2 Significance comparison (no prescribed threshold)

Compare against the total yardstick for that case/output/range:

```
u_total(R) = sqrt( u_num²(R) + u_c²(R) )     (§8 floor + §11 budget; MC σ where linear fails)
R_sig(R)   = |Δ_effect(R)| / u_total(R)
```

- `R_sig ≫ 1` (sustained across the range band): distinguishable — the effect
  earns its complexity.
- `R_sig ≈ 1`: marginal — report as such; the decision belongs to the
  application (a 0.05 MOA effect is noise for hunting, signal for ELR).
- `R_sig ≪ 1`: indistinguishable on this envelope — complexity is not
  justified by evidence here; keep the simpler path and say so
  (`// ponytail:`-style one-line rationale in the report).
- No universal `k` is prescribed. Thresholds derive per application from
  (§8 floors + §9 Pareto + §11 budgets + §10 residuals + stated app
  requirement, e.g. the existing `0.1 MOA` reporting granularity as an
  *example* floor, not a significance criterion). Each report states the `k`
  it used and why.

### 13.3 Engineering-decision output

Per effect: range-resolved `Δ(R)`, `u_total(R)`, `R_sig(R)`, verdict
(`keep / drop / envelope-restrict`), and the artifact IDs backing it. This
section is the mechanism that lets `lob` say "Boatright matters past X00 yd
on envelope E within `u_total`, and is indistinguishable inside Y00 yd" —
with numbers, not adjectives.

---

## 14. Test architecture

### 14.1 Principle: extend, don't parallel-build

The task sketch (`tests/unit/integration/validation/...`) is **not adopted**:
the repo convention is `test/source/` flat + `test/CMakeLists.txt` split +
`gtest_discover_tests`. Imposing a second tree orphans the shared/static
split and CI discovery. Instead:

```
test/
  CMakeLists.txt            (extend LOB_TEST_SOURCES / HELPER_TEST_SOURCES + new VALIDATION_* sets)
  source/
    <existing *_test.cpp>   (untouched)
    testing.hpp             (extend: ladder/sensitivity/artifact helpers)
    validation_convergence_test.cpp   (CI-fast subset, §8.8)
    validation_sensitivity_test.cpp   (CI-fast: 1 case × 2 inputs, schema + Pareto smoke)
    validation_reference_test.cpp     (transcribed env vectors → reference_*.json loaders)
    validation_budget_test.cpp        (budget math on fixtures; incomplete-budget fail-closed)
    validation_mc_smoke_test.cpp      (N=256 fixed-seed integrity + schema)
    validation_signal_test.cpp        (lapse on/off + jump on/off R_sig smoke)
  validation/               (NEW, only if test/source/ gets crowded — decision §21)
    cases/                  (reference_*.json, distribution templates)
    baselines/              (checked-in ceilings/floors/Paretos)
```

If `test/validation/` is created, it registers as a separate CTest target
with the same GTest dependency — not a competing framework.

Template for validation fixtures: `test/source/lob_wind_profile_test.cpp`
(`ConfiguredBuilder` canonical-case factory + `ExpectSameSolution`
bit-exact solve comparator). New validation tests copy that shape —
one factory per envelope case, one comparator for identity oracles
(refinement-identity, clamp-inertness, single-point==uniform) — rather than
inventing new harness styles.

### 14.2 Shared vs static discipline (load-bearing)

- Anything including only `lob/lob.h` + `lob/lob.hpp` → `LOB_TEST_SOURCES`
  (runs in shared **and** static CI).
- Anything including `source/*.hpp` internals (`ode/splines/solve_step/
  solve_angle/wind`) → `HELPER_TEST_SOURCES` (static-only). Convergence ladders
  driving public `StepSize` + `Solve` belong in the first group (preferred —
  they run everywhere); white-box step probes (`DsDxCore`, `CurveView.Seek`,
  direct `GetWind` queries) belong in the second and are marked static-only
  in the file header.
- No test includes `lob_builder.cpp` internals (`Impl`/`Pimpl`) — Builder
  inputs are the perturbation surface (§9.1).

### 14.3 What runs where

| Layer | Location | Deterministic? | CI? | Budget |
|---|---|---|---|---|
| Unit/integration (existing + extended) | `test/source/*_test.cpp` | yes | yes (all OS × shared/static) | seconds |
| Convergence fast subset | `validation_convergence_test.cpp` | yes | yes | < 30 s (C1 36→18→9, ≤4 ranges) |
| Sensitivity/reference/budget/MC/signal smoke | `validation_*_test.cpp` | yes (fixed seeds) | yes | each < 30 s |
| Full ladders, Pareto sweeps, MC ≥10⁴, envelope reports | offline runner + `tools/lob_mc/` | yes (seeded) | no — nightly/manual only | minutes–hours |
| Benchmarks | `benchmark/*` (existing) + cachegrind compare | no gate change | compare-only | existing |
| Generated artifacts | `build/validation/` (gitignored) + curated `baselines/` (checked in) | — | baselines asserted, raw not | — |

Nondeterministic or >60 s tests in CI are rejected at review. MC in CI is
integrity-only (schema + branch counters), never a statistical assertion
tighter than `3·SE_pilot`.

---

## 15. Data / artifact architecture

### 15.1 Formats (reuse precedent)

- **JSON** for structured records (case definitions, convergence summaries,
  budgets, MC run manifests) — precedent: `lobber` JSON stdin/stdout.
- **CSV** for tabular sweeps (range × rung/input/sample-index matrices for
  plotting). One header row, SI/native units named in headers
  (`range_ft,elevation_in_36,elevation_in_18,...`), NaN as empty field with
  `branches.csv` sidecar explaining non-finite entries.
- No new format, no database, no pipeline tool: files + schemas + a
  ≤200-line Python plot helper (offline, optional) that reads the CSV/JSON.
  If the helper grows past plotting, it becomes a separate proposal.

### 15.2 Artifact list (per case/run)

```
convergence_{case}.json/csv | sensitivity_{case}.json/csv
reference_{id}.json         | budget_{case}_{output}.json
mc_run_{id}.json (+ samples.csv.gz for N≥10⁴) | signal_{effect}_{case}.json
envelope_report_{cell}.json (coverage matrix + worst residuals + verdicts)
```

Every JSON artifact starts with a provenance header:

```json
{
  "provenance": {
    "lob_version": "0.13.0", "git_sha": "<12+>",
    "compiler": "gcc-14 -O2", "platform": "x86_64-linux",
    "solver_config": {"step_in": 36, "angle_tol_moa": 0.01, "density_path": "fast"},
    "inputs_hash": "sha256:<of canonical builder JSON>",
    "seed": 12345, "rng": "mt19937_64",
    "created_utc": "2026-..", "artifact_schema": 1
  },
  "results": { "...": "..." }
}
```

### 15.3 Raw vs curated

- **Source-controlled (small):** `reference_*.json` inputs, distribution
  templates, curated baselines (`floors.json`, `pareto.json`,
  `envelope_claims.json`) — reviewed diffs, versioned with the code.
- **Generated (gitignored):** full ladders, sweep CSVs, MC samples under
  `build/validation/`. Reproducible from checked-in inputs + code + seed;
  CI uploads them as job artifacts, never commits them.
- Existing `test/source/*_test.cpp` expected vectors migrate to
  `reference_*.json` mechanically (same numbers, added provenance); the
  GTest wrappers load them so tests and reports share one source of truth.

---

## 16. Reproducibility

1. **Deterministic tests:** fixed builder calls, fixed `pranges`, fixed step/
   tolerance, no RNG, no wall-clock, no threads, no test-order dependence
   (no shared mutable `CurveView` across cases — construct per evaluation).
2. **Fixed inputs:** canonical case builders checked in as JSON + C++
   factory; any deviation changes `inputs_hash` and fails baseline comparison
   with a "config drift" message, not a silent pass.
3. **Solver config:** step, tolerance, density path, iteration cap, bounds are
   artifact fields, never ambient defaults relied upon silently.
4. **Version:** `LobVersion()` + git SHA + `artifact_schema` in every file;
   baseline comparison rejects cross-version equality claims (reports
   "version skew — re-baseline per §17.3").
5. **MC seeds/RNG:** `mt19937_64` + `uint64_t` seed + subseed derivation
   recorded; reseed-per-sample forbidden; worker-count independence required.
6. **Provenance:** reference data cites source + page/URL + access date +
   checksum; supplier assumptions cite the data sheet or `"TBD — human input
   required"`.
7. **Platform/compiler:** CI matrix (macos/ubuntu/windows × shared/static,
   gcc/clang/msvc, hardened flags in `CMakePresets.json`) is the portability
   evidence. Artifacts record compiler + flags + platform.
8. **Floating-point limits (stated honestly):** bit-for-bit reproducibility is
   guaranteed **only** for the same binary + inputs on the same platform.
   Cross-platform / cross-compiler / cross-optimization comparisons use
   tolerance windows from §8 floors (wider than the finest rung delta);
   `EXPECT_DOUBLE_EQ` across platforms is banned in validation tests — use
   `EXPECT_NEAR` with the artifact floor. Shear-on runs (`α ≠ 0`) add one
   `pow` per query whose libm implementation may differ across platforms:
   shear-on envelopes carry their own (wider-or-equal) floors, never shared
   with the `α = 0` cell.

---

## 17. CI strategy

- **Gate (every PR):** `lint` + existing unit/integration + §14.3 smoke subset
  (convergence monotone/ceiling, sensitivity schema+Pareto smoke, reference
  loader round-trip, budget fail-closed on TBD, MC N=256 integrity, signal
  smoke). Total added CI time < 3 min on the existing matrix. ASan/UBSan
  covers new test code; coverage may dip only with justification.
- **Compare-only (existing, unchanged):** cachegrind `CG_MAX_REGRESSION_BP`
  summary; validation artifacts uploaded as job artifacts for inspection.
- **Nightly/manual (not PR-blocking):** full ladders (36→1), full Pareto,
  `N ≥ 10⁴` MC, envelope reports. Failures file issues with artifact links;
  promotion to CI-gated baselines follows §17.3.
- **Baseline promotion:** new ceilings/floors check in only with (a) the
  generating artifact attached, (b) two-platform confirmation (linux + one
  more), (c) a note if any number moved and why (code change vs config vs
  reference update). "Update the golden file to make CI green" without (a–c)
  is a review-blocking violation.
- **Flakiness guard:** any validation CI test failing once per 50 runs on
  retry without code change is quarantined to nightly with a tracking issue —
  statistical assertions stay out of PR gates (§12.2 `3·SE` rule).

---

## 18. Performance considerations

### 18.1 Core library (untouched constraints)

- Deterministic, predictable, allocation-free (`LOB_BUILDER_BUFFER_SIZE 320`
  inline buffer; borrowed table pointers; 60-float spline; two-`CartesianT` +
  `SecT` state), no exceptions, C++14, no `<random>`, no threads, no I/O.
- Hot path stays branch-light: 7-FLOP `PolyVal`, `O(1)` monotonic `Seek`,
  `FastDsDx` firing-site fast path; cost linear in `1/Δx` (measured in
  `benchmark/ode.cpp`). Wind query costs one node copy on the uniform /
  no-shear fast path (`wind.hpp` inline, `kCount == 1 && !kShear`) and a
  ≤7-compare scan + lerp + mults on the profile path (`wind.cpp` cold TU,
  outlined to preserve `DsDxCore` inlining), plus one `pow` iff shear is
  nonzero; no trig, no pitch arithmetic, no allocation at query time.
- Validation adds **zero** sources to `lob_lob`, zero public API changes
  (tolerance already parameterized; step already builder-settable), zero
  hot-path branches. White-box probes are test-only.

### 18.2 Validation / analysis infrastructure (desktop-oriented)

- May be expensive: 36× step refinement, `10⁴–10⁶` MC samples, multi-case
  Pareto sweeps, threaded runners, CSV/JSON I/O, plotting. Lives in
  `test/` (CI smoke) + `tools/lob_mc/` (offline runner), links `lob::lob` as
  a client, never ships to embedded consumers.
- Cost model per study: `ranges × rungs|inputs|samples × steps(∝ 1/Δx)`.
  Publish measured `cost_single` in each artifact; size CI subsets by it.
- Cachegrind stays the perf-regression instrument; validation studies must
  not perturb benchmark cases (shared `BuildBasic/BuildFull/BuildBoatright`
  builders are fixtures, not tuning targets).

---

## 19. Acceptance criteria

Implementation is accepted iff **all** hold (per-phase gates in §20 refine
these; no criterion invents a numeric threshold — thresholds are the measured
floors/ceilings checked into `baselines/`):

1. **Convergence:** full ladder (36→1 in, 3 tolerances) runs offline for
   C1–C9; monotone + order-plausibility verdicts recorded per output family;
   `ε_num` floors checked in per envelope cell; CI fast subset green on all
   OS × shared/static; branch-split cases (fall-short/tumble/path-switch)
   dispositioned, not differenced across.
2. **Error floor:** each envelope cell states its per-output floor with
   metric + rung pair + config; floors for quantized channels note the LSB
   bound; spline-trajectory mapping recorded.
3. **Sensitivity:** OVAT central-difference harness with adaptive-`h`
   procedure (§9.2) runs for C1/C5/C8; accepted `h`, raw + `S`, nonlinearity
   flags, and Pareto (90% cutoff) checked in; `below_floor` entries carry
   floors, never zeros.
4. **Reference:** `reference_*.json` set covers all env-test vectors with
   provenance; comparison reports total residual + `ε_num`-subtracted `δ`
   per cell; coverage matrix renders provisional/unclaimed cells honestly.
5. **Budget:** GUM-schema budgets for C1 elevation/deflection/TOF with
   per-row `u(x)` + provenance; incomplete budgets fail closed; nonlinear
   regimes route to MC with the flag visible.
6. **MC:** offline runner reproduces a fixed-seed `N=256` smoke bit-identical
   on one platform; pilot-scale `N` selection, batch-means SE, P5/P50/P95
   with widths, branch-split summaries, and Spearman cross-check vs §9 Pareto
   all demonstrated on one case; core `lob_lob` diff is empty.
7. **Reproducibility:** same-binary rerun reproduces artifacts hash-identical
   (excl. timestamps); cross-platform runs agree within stated floors;
   every artifact carries the §15.2 header; no `TBD` uncertainties in
   checked-in combined numbers.
8. **Regression:** a synthetic `ε_num` inflation (e.g. forced 72-in step in a
   fixture) trips the CI ceiling test; a reference-vector typo trips the
   loader test with a provenance-pointing message.
9. **Docs:** `validation/overview.md` + `num_ode` extended per §23; no new
   unsupported claim; envelope page lists cells + verdicts + artifact links.
10. **CI/perf:** added PR-gate time < 3 min; no new core dependency; no heap/
    exception/RNG/thread in `lob_lob` (verified by the existing sanitize +
    a `grep`-level source guard in review); benchmark compare output unchanged
    in shape.

---

## 20. Implementation phases

### Phase 1 — Numerical convergence and error-floor infrastructure

- **Files:** new `test/source/validation_convergence_test.cpp` (+ helpers in
  `testing.hpp`: ladder runner, pairwise-delta + order estimator, floor
  verdict, JSON/CSV writers to `build/validation/`); curated
  `test/validation/baselines/floors.json`; `test/CMakeLists.txt` (new sources
  in `LOB_TEST_SOURCES` — public-API only); docs touch-up to
  `docs/pages/validation/overview.md` (method pointer, no numbers yet).
- **Deps:** GTest only (already vendored/fetched). No new dependency.
- **Tests/artifacts:** CI fast subset (C1 36→18→9, C9 2-range inverse,
  angle-tightening check); offline full ladders C1–C9 → `convergence_*.json`.
- **Acceptance:** §19.1 + §19.2 for the CI subset; offline floors recorded.
- **CI:** yes (fast subset). **Cost:** CI seconds; offline ~36× solve cost per
  finest rung (minutes total, single-threaded).

### Phase 2 — Sensitivity analysis

- **Files:** `test/source/validation_sensitivity_test.cpp` (central-diff
  harness, adaptive-`h` loop, noise-floor import from Phase 1 baselines,
  Pareto writer); `baselines/pareto.json` (top drivers per case/output).
- **Deps:** Phase 1 floors (noise reference). No new dep.
- **Tests/artifacts:** CI smoke (C1 × velocity + wind); offline full Pareto
  C1/C5/C8 → `sensitivity_*.json/csv`.
- **Acceptance:** §19.3. **CI:** smoke only. **Cost:** CI seconds; offline
  `inputs × 2 × ranges` solves per case (minutes).

### Phase 3 — Reference-validation infrastructure

- **Files:** `test/validation/cases/reference_*.json` (transcribed env
  vectors + provenance) + loader; `validation_reference_test.cpp`
  (residual + decomposition + coverage-matrix generator);
  `envelope_report_*.json`.
- **Deps:** Phases 1–2 (floors + metrics). Needs human provenance review
  (§21).
- **Tests/artifacts:** CI loader + decomposition smoke on C1-ICAO;
  offline full matrix.
- **Acceptance:** §19.4. **CI:** loader/decomposition only. **Cost:** CI
  seconds; offline negligible beyond solves.

### Phase 4 — Uncertainty-budget infrastructure

- **Files:** `validation_budget_test.cpp` (GUM combiner, covariance table,
  fail-closed TBD handling, nonlinear-route flags); budget templates with
  `TBD` rows; `budget_*.json` for completed cells.
- **Deps:** Phases 1–3 (c_i, floors, δ/η rows). Human `u(x)` inputs (§21).
- **Tests/artifacts:** CI math-on-fixtures + fail-closed test; offline real
  budgets once `u(x)` approved.
- **Acceptance:** §19.5. **CI:** math + fail-closed only. **Cost:** trivial
  compute; dominant cost is eliciting `u(x)`.

### Phase 5 — Monte Carlo propagation

- **Files:** new `tools/lob_mc/` (runner: distribution-JSON parser,
  `mt19937_64` sampler with subseeds, threaded worker pool, summary writer);
  `validation_mc_smoke_test.cpp` (N=256 integrity); distribution templates.
- **Deps:** Phases 1–4 (dimensions from Pareto, `E_target` from floors,
  branch list). No core changes; new tool links installed `lob`.
- **Tests/artifacts:** CI smoke (integrity + schema); offline pilot→scale
  runs → `mc_run_*.json`.
- **Acceptance:** §19.6. **CI:** smoke only, fixed seed. **Cost:** CI
  seconds (256 solves); offline minutes–hours at 10⁴–10⁶ (threaded).

### Phase 6 — Technical-reference integration and envelope reporting

- **Files:** Doxygen updates (§23), `envelope_claims.json` (cell verdicts +
  artifact links), `signal_{effect}_{case}.json` + `validation_signal_test.cpp`
  smoke (lapse + jump).
- **Deps:** Phases 1–5 artifacts. Human sign-off on every claimed cell (§21).
- **Tests/artifacts:** CI signal smoke; offline full effect matrix + envelope
  report.
- **Acceptance:** §19.8–§19.10. **CI:** smoke only. **Cost:** small; mostly
  writing.

---

## 21. Open questions / decisions requiring human review

1. **Spec location:** accept `docs/specs/` (this file) vs move under
   `docs/superpowers/specs/`? Content is location-independent.
2. **`test/source/` vs new `test/validation/` dir:** default = extend
   `test/source/` until crowding forces the split. Confirm threshold.
   **Decided 2026-09-27:** tests in `test/source/`, baselines in
   `test/validation/baselines/` (Phase 1 plan as written).
3. **MC runner home:** `tools/lob_mc/` (recommended) vs `validation/mc/` vs
   `benchmark/`-adjacent. Confirm before Phase 5.
4. **Canonical MC defaults:** pilot `N=1024`, smoke `N=256`, `E_target=0.1σ`,
   `mt19937_64` + splitmix64 subseeds — approve or amend.
5. **Step ladder exact rungs:** `36→18→9→4→2→1` in (9→4 inexact halving
   recorded) — approve; alternative `36→12→6→3→1` if exact thirds preferred.
   **Decided 2026-09-27:** `36→18→9→4→2→1` with actual `Δx` recorded per rung.
6. **Envelope cell granularity:** range bands × regimes proposed in §10.3 —
   approve boundaries (esp. transonic definition and the `drop=100 ft` split).
   **Decided 2026-09-27:** `0–300/300–1000/1000+ yd` bands as written.
7. **Input uncertainties `u(x_i)`:** no defaults shipped; approve the
   elicitation template and the first completed budget's sources (supplier
   sheets vs engineering judgment, each row labeled).
8. **Reference transcription:** approve mechanical move of env-test vectors
   into `reference_*.json` (reviewer diffs each number) and the provenance
   text for McCoy/Huang/JBM/BRL/Litz/Boatright entries.
9. **Categorical treatment:** G-curve / drag-source / density-path as scenario
   weights — approve families and weights before any MC uses them.
10. **Output quantization:** keep `U16/U32` truncation (report LSB floors) vs
    expose double-precision validation accessors (test-only, static builds)?
    Default = keep ABI, document floors. Changing the ABI is out of scope
    without a separate proposal.
    **Decided 2026-09-27:** keep ABI, LSB floors.
11. **Field/radar data:** pursue acquisition? Until yes, long-range cells stay
    provisional by policy — confirm.
12. **Significance `k`:** per-report stated, no global default — confirm no
    project-wide `R_sig` threshold is wanted.
13. **Wind envelope cells:** approve the split (uniform / profile-α0 /
    profile-scaled / incline×wind / ceiling-clamp) and the rule that scaled
    results never inherit unscaled floors (§10.3, §16.8).
    **Decided 2026-09-27:** five-way split as written.
14. **Wind MC sampling:** approve input-space (speed, heading) sampling with
    wrapped headings and α-on-`[0,1]` (§12.1), plus the height-jitter-only-
    when-sheared rule.
15. **Wind `u(x)` elicitation:** approve rows for station error, cross-station
    correlation, height error (shear path), and α uncertainty (§11.3) before
    the first wind budget is combined.

---

## 22. Risks and limitations

- **Overgeneralization** from few cases → mitigated by envelope matrix with
  explicit unclaimed cells (§10.3); residual risk if readers quote a cell
  number out of context — docs carry the envelope with every number (§23).
- **MC hiding bad inputs** (wide priors masking model error) → priors are
  versioned assumptions under review; budgets show rows; fail-closed TBDs.
- **Precision ≠ accuracy**: tight ladders + wide `δ` is the expected
  long-range outcome — reports lead with the decomposition, never the
  smallest number.
- **Nondeterministic CI**: MC/PR-gate statistics capped at `3·SE` integrity
  checks; quarantine rule (§17).
- **Cost blowup** (36× finest rung × Pareto × MC) → CI subsets sized by
  measured `cost_single`; offline-only full runs; threaded MC with subseeds.
- **Platform FP divergence** → tolerance windows, never cross-platform exact
  equality; provenance records platform (§16).
- **Branch discontinuities** (fall-short/tumble/cap/bounds/path-switch) →
  split envelopes, branch counters, no pooled stats across branches.
- **Quantization masking** (U16/U32) → LSB floors stated; sub-LSB claims
  rejected at review.
- **Angular wrap** (uniform/profile headings, azimuth) and **joint
  perturbations** (pressure/temperature/altitude, band BCs, multi-station
  profiles from one sensor) → wrap normalization + documented
  independence/correlation flags.
- **Tilted-datum assumption**: shot-parallel constant-slope plane is exact at
  muzzle and target footing for ground targets but a documented limit in
  broken terrain (terrain-driven flow dominates there); remedy is denser
  downrange sensing through the same profile API, and envelope cells never
  mix flat/inclined wind claims (§10.3).
- **Reference licensing** (JBM/BRL tables, paper excerpts): vendored tables
  already in-tree; new external data needs license check before check-in.
- **Lapse-gate validity**: forward `Fast*` vs inverse-tail `DsDx` split means
  two truths per long-range case — the spec treats this as designed, visible,
  and envelope-defining, not as a bug to hide.

---

## 23. Documentation integration

Doxygen (`docs/Doxyfile.in` `INPUT = include + docs/pages`) gains methodology
pointers, not duplicated physics. Planned edits (Phase 6; each cites tests +
artifact IDs, keeps the evidence-before-synthesis policy):

- `docs/pages/validation/overview.md` — add subsections: error-floor table
  (per envelope cell, with config + artifact link), sensitivity Pareto pointer,
  budget summary, MC method pointer, envelope coverage matrix, limitations
  (unchanged gaps + new branch/quantization notes).
- `docs/pages/numerical_methods/ode.md` — link the ladder procedure + observed
  order/floor results; keep the McCoy history paragraph intact.
- `docs/pages/numerical_methods/splines.md` + `ballistic_model/drag.md` —
  link the `5e-3 → inches/MOA` trajectory mapping (§8.6).
- `docs/pages/numerical_methods/zero_angle.md` + `inverse.md` + `api/*` —
  link tolerance-tightening evidence and the `Fast*` vs lapse branch table.
- `docs/pages/ballistic_model/wind_coriolis.md` — the usage/limitations
  sections already describe the profile query, but the vector section still
  documents the removed `ctx.wind = {x,z}` storage and the internal-radian
  heading mapping. Phase 6 rewrites it against `wind_nodes[]`, the
  direction+magnitude input convention, and the 1-ft/tilted-datum semantics,
  with `WIND_INTERFACE_SPEC.md` cited as the contract (not duplicated).
- `docs/pages/overview.md` — add the validation-spec page to the map
  (whether via `@page` include or external link depends on whether
  `docs/specs/` joins the Doxygen `INPUT`; default = link, not include, to
  keep the spec readable on GitHub without Doxygen processing).
- Every envelope-adjacent number in the reference states its cell + case IDs;
  the words "error", "uncertainty", and "validated" appear only with their
  §3 qualifier (`ε_num`, `u_c`, cell ID) or not at all.

---

## Appendix A — Implementation-ready checklists (per phase kickoff)

Each phase opens with a tracking issue copying its §20 row plus: exact file
paths, test names, artifact filenames, baseline diffs under review, CI time
measurement, and the §19 gate it closes. No phase starts without the prior
phase's baselines merged.

## Appendix B — Banned phrases (review gate)

- "`lob` is accurate to X" (use: "on envelope E at config C, `|r| ≤ …`
  with `ε_num ≤ …`, cases …, artifact …").
- "Numerical error" for a reference difference without a ladder citation.
- "Validated" without an envelope cell + case IDs.
- "Confidence interval" without the distributional assumption stated.
- Any `u(x)` value without provenance or `TBD` marking.
- MC results without seed + N + convergence widths + branch counts.

---

*End of specification. Next action: human review of §21, then Phase 1 issue
per Appendix A. No implementation code is authorized by this document.*
