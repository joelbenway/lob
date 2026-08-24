@page design_shared_solver Shared Angle Solver

# Shared Angle Solver

@section design-shared-problem The two places an angle is solved

```
Builder:  ZeroDistanceYds(R)  →  SolveAngle(R, 0) → zero_angle
Inverse:  range R             →  SolveAngle(R, 0) → θ* → adjustment = θ* − zero_angle
```

Both are the same mathematical problem: find `θ` with

```
f(θ) = y(R; θ) − optic − impact = 0
```

(`source/solve_angle.hpp`, `source/lob_builder.cpp`,
`source/lob_solve.cpp`).  Historically a ballistics library might have
two implementations — one for zero and one for inverse — and they can diverge
subtly in tolerance, bounds, jump handling, or iteration cap.

@section design-shared-solution One solver

lob factors the iteration into `source/solve_angle.hpp`:

- `FastInverseAngle(θ, f, R) = atan(tan θ − f/R)` — geometric one-step.
- `SolveAngle(ctx, R, impact, seed, tol)` — fixed-point over `FastInverseAngle`,
  `tol = 0.01 MOA`, `±45°`, `≤10` iterations, `NaN` on unreachable.

Both call sites use it:

- `BuildZeroAngle` seeds with the vacuum parabola `0.5·g·R/v0²` clamped to
  `±45°` (`source/lob_builder.cpp`).
- `LobSolveInverse` seeds with `FastInverseAngle(zero_angle, forward_residual, R)`
  (`source/lob_solve.cpp`) and calls the same `SolveAngle`.  The comment
  `// Seed represents mechanical launch angle. SolveAngle adds aerodynamic_jump
  internally` records the subtlety that both paths add `jump` inside
  `fire_to_target`/`SolveStep`, not outside.

@section design-shared-why Why sharing matters

- **Consistency**: the zero that zeros the rifle and the adjustments that
  assume that zero are computed with identical tolerance, bounds and jump
  handling — otherwise a rifle zeroed with one solver would be off by the
  other's bias.
- **Maintenance**: a fix to bounds or convergence (e.g. the `±45°` clamp)
  applies to both features at once; `test/source/solve_angle_test.cpp`
  cross-checks `FastInverse` vs `SolveAngle` and `BuilderZeroAngleTest`
  cross-checks builder vs `SolveAngle`.
- **Diff size**: one guard in the shared function is a smaller, safer diff
  than a guard in every caller.

@section design-shared-validation Validation of sharing

- `SolveAngle` round-trips heights `−3…+3 in` at 300 yd to `±0.1 in`
  (`test/source/solve_angle_test.cpp`).
- Builder zero matches a direct `SolveAngle` call to `1e-6 MOA`
  (`test/source/solve_angle_test.cpp`).
- Iterative inverse vs one-step `FastInverse` agree within `0.1 MOA` even
  with non-zero `aerodynamic_jump`
  (`test/source/lob_inverse_test.cpp`).
