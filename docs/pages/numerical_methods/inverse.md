@page num_inverse Inverse Solver

# Inverse Solver

@section num-inverse-problem Problem

Given a desired range `R`, find the sight adjustment that makes the trajectory
hit:

```
Δ_elev = θ*(R) − zero_angle     (MOA)
Δ_wind = InchToMoa(−deflection(R), R)   (MOA)
```

where `θ*(R)` solves `f(θ)=0` as in @ref num_zero_angle.  The difficulty is
doing this for many `R` without repeating the full trajectory integration more
than necessary and handling unreachable or zero ranges correctly.

@section num-inverse-two Two APIs, one solver

lob exposes two APIs over the same core (`source/lob_solve.cpp`,
`include/lob/lob.h`):

**`LobSolveInverse` / `lob::SolveInverse` — iterative, authoritative.**

```
n = LobSolve(ctx, ranges, outs, size)          // forward pass, reachability (Fast* fast path)
for i in 0..n-1
    if ranges[i]==0 { outs[i].{elev,defl}=0; continue }
    seed = FastInverseAngle(zero_angle, elevation_inches, R)
    θ*   = (elevation < −1200in) ? SolveAngle(ctx,R,0,seed)  // drop>100ft → lapse-scaled DsDx
                                 : FastSolveAngle(ctx,R,0,seed) // else firing-site
    if θ* is NaN → return i                     // prefix only; shortfall is not counted
    outs[i].elevation = MOA(θ* − zero_angle)
    outs[i].deflection = InchToMoa(−deflection, R)
return n
```

Seeding with `FastInverseAngle` applied to the forward residual gives a
starting angle usually within `0.1 MOA` of the solution
(`test/source/solve_angle_test.cpp`), so the inner iteration typically
converges in 1–2 steps.

**`LobFastInverse` / `lob::FastInverse` — one-step, in-place.**

```
for each out
    if range==0 or !finite(elev/defl) → skip
    elevation = MOA(FastInverseAngle(θ0+aj, elevation, R) − (θ0+aj))
    deflection = InchToMoa(−deflection, R)
```

No integration is performed; the call just warps the stored forward height
via the geometric formula.  Hence the doc warning: only convert outputs whose
forward solve *reached* `R` — fall-short residuals are meaningless
(`source/lob_solve.cpp`).

@section num-inverse-shared Shared angle solver

Both the builder's zero finding (`FastSolveAngle`) and inverse solving
(`FastSolveAngle` or `SolveAngle` per-range gated on `drop>100ft`) route
through `SolveAngle`/`FastSolveAngle`/`FastInverseAngle` in
`source/solve_angle.hpp`:

```
zero-angle calculation (FastSolveAngle)
        │
        ├── shared angle solver (FastSolveAngle/SolveAngle + FastInverseAngle, gated per-range)
        │
inverse trajectory solving (FastSolveAngle or SolveAngle)
```

Zero-angle uses a vacuum parabola seed; inverse uses the forward residual
seed.  They converge with the same tolerance `0.01 MOA`, bounds `±45°`, and
iteration cap `10`.  Sharing the mechanism avoids divergence between “how we
zero” and “how we compute adjustments” (@ref design_shared_solver).

@section num-inverse-edges Edges and guarantees

- `range==0` entries: `LobSolveInverse` counts them with `0` MOA;
  `LobFastInverse` skips them and does not count them
  (`source/lob_solve.cpp` `LobFastInverse`).
- `LobSolveInverse` stops at the first unreachable range (where `SolveAngle`
  returns NaN) and reports only the reachable prefix; `LobFastInverse` does not
  stop — it would silently give a wrong number for a fall-short, hence the
  asymmetry is intentional.  Note: a fall-short `LobSolve` still produces an
  output at the tumble/max-time point; `LobSolveInverse` only discards it if the
  angle solve for that achieved range fails.
- `LobSolveInverse` requires strictly increasing `pranges`; otherwise `0` is
  returned (same guard as `LobSolve`).  `LobFastInverse` operates in-place and
  does not check `pranges`.
- Non-finite forward elevations/deflections are skipped by `FastInverse` and
  not counted.

Validation in `test/source/lob_inverse_test.cpp`:

- single and array solves, sign flip vs forward, monotonic growth with range,
- zero-range and unreachable handling, raw-pointer and `std::array` overloads,
- agreement between `FastInverse` and iterative inverse within `0.1 MOA`
  when jump is non-zero (cross-checks the jump addition inside both paths),
- `SolveAngle` tight-tolerance and unreachable tests.

@section num-inverse-which Which to call

- Use `SolveInverse` for published adjustment tables.
- Use `FastInverse` when you already have forward outputs and know they are
  reachable and want to avoid a second integration.  Spot-check a few ranges
  against `SolveInverse` to confirm `FastInverse` error is acceptable for your
  trajectory.
