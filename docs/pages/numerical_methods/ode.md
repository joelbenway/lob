@page num_ode ODE Integration

# ODE Integration

@section num-ode-why Problem

The point-mass dynamics (@ref model_point_mass) are

```text
dy/dx = f(x, y)   with y = (p, v, TOF)
```

integrated in down-range distance `x` rather than time `t` because the shooter
queries by distance. Integrating over distance with a configurable step size
provides flexibility across diverse applications — intercepting a drone at
50 yards within a millisecond requires different spatial resolution than
computing a multi-mile long-range shot. Time of flight is advanced as a
dependent variable via the velocity history.

@section num-ode-method Method

`source/ode.hpp` provides three steppers; lob uses **Heun's method**
(RK2 predictor-corrector) in `SolveStep` (`source/solve_step.cpp`):

```text
k1 = f(x_n, y_n)
k2 = f(x_n+Δx, y_n + k1·Δx)
y_{n+1} = y_n + (k1+k2)/2 · Δx
```

`Δx` is one yard (36 in) by default or `ctx.step_size` inches if the caller
set it (`source/solve_step.cpp`).  The last step to a requested target `R`
is clamped: `Δx = min(R − x_n, step)`.

`DsDx` / `FastDsDx` (`source/solve_step.cpp`) evaluates `f` via
`DsDxCore` helpers (`GetDtDx`/`GetWind`/`GetMach`/`GetCd`/`GetDpDt`/`GetDvDt`):

```text
kDtDx = 1/vx                    (guarded: if vx ≤ 0 → zero derivative)
wind  = (ctx.wind.x, 0, ctx.wind.z)
u     = −k_lapse·P·G            (only DsDx; FastDsDx skips)
drag  = drag_coeff·(1−u(1−αu))   (only DsDx; FastDsDx = drag_coeff)
c     = c·(1−βu)                (only DsDx; FastDsDx = c)
Mach  = |v|/c
Cd    = curve.Eval(Mach)·drag
dp/dt = v
dv/dt = −Cd·|v−w|·(v−w) − Coriolis(v) + gravity
d(TOF)/dt = 1
return (dp/dt·kDtDx, dv/dt·kDtDx, 1/vx)
```

`TOF` is integrated as the third component of `f` via `d(TOF)/dx = 1/vx`
(`DsDxCore` `SecT(kDtDx)`), integrated by `HeunStep` — `FastSolveStep`
(`FastDsDx`) for forward/zero/Boatright and `SolveStep` (`DsDx`) for
`drop>100ft` inverse ranges.

`source/ode.hpp` also implements
`EulerStep` and `RungeKuttaStep` (RK4); they are not used
in production but are exercised in `test/source/ode_test.cpp` to demonstrate
convergence on the test ODE `dy/dt = sin²(t)·y`.

@section num-ode-accuracy Observed accuracy

No universal accuracy claim is made.  The method choice follows BRL
experience — "The cumulative experience of the Ballistic Research Laboratory has
shown that the one-step, second-order methods are, in the practical sense,
optimum solutions of the point-mass trajectory problem." (McCoy, *Modern
Exterior Ballistics — The Launch and Flight Dynamics of Symmetric Projectiles*,
2nd ed).  lob uses Heun (RK2) on that basis, then validates empirically:

- `LobSolve` with the default 1-yard step reproduces the reference
  trajectories in `test/source/lob_env_test.cpp` within `±1 fps`,
  `±5 ft·lbf`, `±0.1 MOA`-equivalent and `±0.01 s` out to 1000 yd.
- Reducing the step via `Builder::StepSize` reduces error monotonically but
  with diminishing returns beyond ~6 in for the tested trajectories; the
  benchmark in `benchmark/ode.cpp` shows linear time cost in `1/Δx` (each
  method is run at its own `dt` to reach ~1 ft error vs an RK4 `dt=1e-5`
  reference — see `benchmark/ode.cpp`).
- The `_solve_step_test` and `_calc_test` lock in per-step invariants.

@section num-ode-limits Limitations

- Fixed step size; no embedded error estimate or adaptive stepping.
- Per-step lapse is gated: `LobSolve`/`BuildBoatright`/`BuildZeroAngle` use `Fast*`
  (firing-site `ρ`/`c`); only `LobSolveInverse` ranges with forward `drop>100ft`
  (`elevation < −1200in`) use `DsDx`/`SolveStep`/`SolveAngle` lapse.
