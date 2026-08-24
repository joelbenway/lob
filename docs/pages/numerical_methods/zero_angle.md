@page num_zero_angle Zero-Angle Solver

# Zero-Angle Solver

@section num-zero-problem Problem

Find the bore-vs-sight angle `θ` that makes the trajectory hit a desired
impact height at range `R`:

```text
f(θ) = y(R; θ) − optic_height − impact_height = 0
```

`y(R;θ)` is the height reached by integrating from `x=0` at
`v = v0·(cos(θ+aj), sin(θ+aj), 0)` with `aj = aerodynamic_jump`
(`source/solve_angle.hpp`).  `R` is the zero distance (or any range in
the inverse path), `impact_height` is usually `0` or the caller's
`zero_impact_height`.  The solver must be robust to poor seeds and report
unreachability rather than a spurious angle.

@section num-zero-method Method

`SolveAngle` (`source/solve_angle.hpp`) is a fixed-point iteration with a
geometric one-step:

```text
FastInverseAngle(θ, f, R) = atan(tan θ − f/R)
```

(`source/solve_angle.hpp`).  The fixed point is `θ_{n+1}=FastInverseAngle(θ_n, f(θ_n), R)`,
which is the angle that would zero the residual if the trajectory were a
straight line — then corrected by re-integrating.

Pseudocode:

```text
θ = seed
f = fire_to_target(θ)          // integrate to R, NaN if vx≤0 / TOF≥max / vacuum
for iter in 0..9
    θ' = FastInverseAngle(θ, f, R)
    if θ'∉[−45°,45°] or NaN → return NaN
    if |θ'−θ| ≤ tol           → return θ' (as MOA)
    θ = θ';  f = fire_to_target(θ)
return NaN                      // no convergence
```

Constants (`source/solve_angle.hpp`):

- `kMaxIterations = 10`
- `kDefaultAngleTolerance = 0.01 MOA` (≈ 2.9 µrad)
- `kMaxAngle = 45°`, `kMinAngle = −45°` — lob targets flat-fire trajectories;
  above 45° a high (mortar) and a low direct-fire trajectory can both reach `R`.
  The solver restricts search to the flat-fire branch, treating >45° as
  out-of-bounds (returns NaN / `kLobErrorZeroUnreachable`).

`fire_to_target` integrates step-by-step via `SolveStep` until `x ≥ R`
and returns the vertical miss; any step where `vx ≤ 0`, `TOF ≥ max_time`, or
speed ≤ `minimum_speed` yields `NaN` and the iteration becomes non-finite,
propagating to a NaN return (unreachable).

@section num-zero-seed Seed

`BuildZeroAngle` (`source/lob_builder.cpp`) supplies the seed when the
caller gave `ZeroDistanceYds` instead of `ZeroAngleMOA`:

```text
θ_vac = 0.5·g·R / v0²           (vacuum parabola angle)
seed  = clamp(θ_vac, −45°, 45°)
```

The same seed construction is used in `test/source/solve_angle_test.cpp`.
A poor seed (e.g. `0` or even `−30°`) still converges for the validated
ranges (`test/source/solve_angle_test.cpp`), but the vacuum seed cuts
typical iterations to 2–4.

@section num-zero-validation Validation

- `test/source/solve_angle_test.cpp` checks convergence from `0°, ±30°, 44°`
  seeds, round-trip with `±3 in` impact heights, very short range, tight
  `0.001 MOA` tolerance, and unreachable detection.
- `test/source/solve_angle_test.cpp` cross-checks the builder's zero angle
  against a direct `SolveAngle` call (`BuilderZeroAngleTest`), while
  `test/source/lob_api_test.cpp` verifies weak `BC=0.1, v=600 fps` at 2000 yd
  exhausts iterations and returns NaN via `kLobErrorZeroUnreachable`.

@section num-zero-reuse Reuse

The same solver backs inverse adjustments (@ref num_inverse) and is the point
of the @ref design_shared_solver decision.  `SolveAngle` is `inline` in a
header so both builder and solve translation units share one definition.
