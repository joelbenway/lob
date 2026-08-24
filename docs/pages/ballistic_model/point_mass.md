@page model_point_mass Point-Mass Model

# Point-Mass Model

@section model-pm-equations Equations of motion

lob integrates the classic 3-DOF point-mass equations (McCoy, Modern Exterior
Ballistics, Ch. 3, 10).  The state is `s = (p, v, TOF)` with
`p ∈ FeetT³`, `v ∈ FpsT³` (`source/ode.hpp`, `source/cartesian.hpp`).

Down-range `x` is the independent variable.  Let `w` be the wind vector,
`c` the Coriolis vector, `g` gravity, `ρ` air density, `Cd(M)` the Mach-
dependent drag coefficient and

```
k = Cd(M) · drag_coeff     (see below)
```

Then in the time domain

```
dp/dt = v
dv/dt = −k·|v − w|·(v − w)  −  Coriolis(v)  +  g
d(TOF)/dt = 1
```

Dividing by `vx` gives the down-range form actually integrated
(`source/solve_step.cpp`):

```
dp/dx = v / vx
dv/dx = dv/dt / vx
```

`DsDx` returns `(dp/dx, dv/dx)`.  When `vx ≤ 0` the derivative is zeroed
(`source/solve_step.cpp` `DsDx`) and the step clamps `vx` to zero — the
projectile is falling straight down (`source/solve_step.cpp` `SolveStep`).

`drag_coeff` is `ρ·π / 8` (density term) for all paths; for the single-BC
path the BC is divided into the spline coefficients `drags[60] = coefs/(BC·conversion)`
(`source/calc.hpp`, `source/lob_builder.cpp`).  For custom tables and BC bands
the spline already embeds `1/BC` and `drag_coeff` remains `ρ·π / 8` at `BC=1`.

Coriolis is expanded as

```
dvx/dt −= vy·2Ω cos(lat) sin(az) + vz·2Ω sin(lat)
dvy/dt += vx·2Ω cos(lat) sin(az) + vz·2Ω cos(lat) cos(az)
dvz/dt += vx·2Ω sin(lat) − vy·2Ω cos(lat) cos(az)
```

with the three scalars precomputed in `BuildCoriolis`
(`source/lob_builder.cpp`).  Gravity is rotated by the range (inclination)
angle: `gx = −g sin(rangeAngle)`, `gy = −g cos(rangeAngle)`
(`source/lob_builder.cpp`).

@section model-pm-integration How it is integrated

The integrator is Heun's method (RK2 predictor-corrector, `source/ode.hpp`):
`y_{n+1}= y_n + (k1+k2)/2 · Δx` with `k1=f(x_n,y_n)`, `k2=f(x_n+Δx, y_n+k1·Δx)`.
`Δx` is one yard by default (36 in) or `ctx.step_size` inches if non-zero,
clamped to not over-shoot the requested target `x` (`source/solve_step.cpp`).
Each `SolveStep` also advances `TOF` by the trapezoidal estimate
`2·Δx/(vx_old+vx_new)` (`source/solve_step.cpp`).

This is the same step used by both forward and inverse solves and by the
zero-angle seed search.  See @ref num_ode for accuracy notes.

@section model-pm-assumptions Assumptions

- Point mass: no lift except via the Boatright/Litz spin corrections applied
  as post-factors to `deflection` and as `aerodynamic_jump` added to the
  launch angle.
- Flat Earth within the integration horizon; altitude-dependent density is
  evaluated only at the firing site (no per-step density lapse).
- Wind is uniform and constant.
