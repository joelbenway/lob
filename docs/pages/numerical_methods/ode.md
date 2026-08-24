@page num_ode ODE Integration

# ODE Integration

@section num-ode-why Problem

The point-mass dynamics (@ref model_point_mass) are

```
dy/dx = f(x, y)   with y = (p, v, TOF)
```

integrated in down-range distance `x` rather than time `t` because the shooter
queries by distance. Integrating over distance with a configurable step size
provides flexibility across diverse applications — intercepting a drone at
50 yards within a millisecond requires different spatial resolution than
computing a multi-mile long-range shot. Time of flight is advanced as a
dependent variable via the velocity history.

@section num-ode-method Method

`source/ode.hpp` provides four steppers; lob uses **Heun's method**
(RK2 predictor-corrector) in `SolveStep` (`source/solve_step.cpp`):

```
k1 = f(x_n, y_n)
k2 = f(x_n+Δx, y_n + k1·Δx)
y_{n+1} = y_n + (k1+k2)/2 · Δx
```

`Δx` is one yard (36 in) by default or `ctx.step_size` inches if the caller
set it (`source/solve_step.cpp`).  The last step to a requested target `R`
is clamped: `Δx = min(R − x_n, step)`.

`DsDx` (`source/solve_step.cpp`) evaluates `f`:

```
kDtDx = 1/vx   (guarded: if vx ≤ 0 → zero derivative)
wind  = (ctx.wind.x, 0, ctx.wind.z)
Mach  = |v|/c
Cd    = curve.Eval(Mach)·drag_coeff
dp/dt = v
dv/dt = −Cd·|v−w|·(v−w)  − Coriolis(v) + gravity
return (dp/dt·kDtDx, dv/dt·kDtDx)
```

`TOF` is **not** integrated via `f`; it is advanced by the trapezoidal
estimate `2·Δx/(vx_old+vx_new)` after the Heun step (`source/solve_step.cpp`),
which is exact for a linear velocity profile over the step.

`source/ode.hpp` also implements
`EulerStep`, `RungeKuttaStep` (RK4) and `IterativeHeunStep`; they are not used
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
- TOF via trapezoidal velocity average, not via integrating `dt/dx`.
- No per-step altitude stratification — density/c are firing-site values (per-step atmospheric lapse is planned).
