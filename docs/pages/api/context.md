@page api_context Context and Results

# Context and Results

@section api-context-context Context

`LobContext` (`include/lob/lob.h`) is the solver's immutable input.
`lob::Context` (`include/lob/lob.hpp`) is layout-identical; a set of
`static_assert`s guarantee no drift.

Selected fields (units in the comments in the header):

- `drag_coeff` — density term `ρ·π / 8` applied to `Cd(Mach)` inside `DsDx`
  (`source/solve_step.cpp`).  For standard and BC-band paths the `1/BC`
  scaling is already baked into the spline `drags[60]`; for custom tables
  `drag_coeff` scales density only.
- `speed_of_sound` — fps, local value including humidity correction
  (`source/calc.hpp`).
- `mass` — pounds, `LbsT(mass).Value()`.
- `optic_height` — feet.
- `gravity` — `{x,y}` ft/s² rotated by range angle (`source/lob_builder.cpp`).
- `wind` — `{x,z}` fps from heading+speed.
- `coriolis` — three scalars `2Ω cos(lat) sin(az)` etc.
- `zero_angle` — MOA bore-vs-sight angle that zeros at the requested distance.
- `stability_factor` — Miller sg, NaN if not computable.
- `aerodynamic_jump` — MOA, 0 if not computed.
- `spindrift_factor` — NaN if Litz path should be used instead.
- `max_time`, `minimum_speed`, `step_size`, `error`, `drags[60]` (spline coefs).

`Context` is built only via `Builder`; do not fill it by hand.

@section api-context-output Output

`LobOutput` (`include/lob/lob.h`) / `lob::Output`:

```
uint32_t range;         // feet actually reached
uint16_t velocity;      // fps
uint32_t energy;        // ft·lbf  (½·m·v²)
double elevation;       // inches (forward) or MOA (inverse)
double deflection;      // inches (forward) or MOA (inverse)
double time_of_flight;  // seconds
```

All outputs are produced by `LobSolve` / `LobSolveInverse`; callers provide
the storage.  No allocation occurs.

@section api-context-errors Error handling

`Context::error` (`LobErrorT`) is checked at the top of every solve.  Any
non-`kLobErrorNone` value causes the solve to return `0` immediately
(`source/lob_solve.cpp`).  `Builder` never throws; errors are values.

@section api-context-lifetime Lifetime

`Builder` and the drag tables it points to must outlive `Build()` only.
After `Build()`, `Context` is self-contained (coefficients copied).  `Context`
and `Output` are plain aggregates suitable for `memcpy`.
