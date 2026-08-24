@page model_wind_coriolis Wind and Coriolis

# Wind and Coriolis

Wind and Coriolis are the two velocity-coupled corrections to the point-mass
trajectory. Both are built once as vectors/scalars in `LobContext` and applied
every integration step in `DsDx`. They are grouped because neither changes
the drag curve or atmosphere — they only add `v`-dependent accelerations in
`DsDx` (`source/solve_step.cpp` drag + Coriolis).

@section model-wind-vector Wind vector

Wind enters the equations only as `v − w` in the drag term and as `w_z` in the
jump models. `BuildWind` (`source/lob_builder.cpp`) produces
`ctx.wind = {x,z}` in fps:

```
w_x = |w|·sin(heading)
w_z = |w|·cos(heading)
```

`heading` is the user angle (0° = tailwind) mapped to the internal math
angle used in `BuildWind`:

- `WindHeading(ClockAngleT)` — 12 o'clock = tailwind, 6 = headwind. Stored as
  `(3 − clock)·30°` wrapped to `[0,360)` so internal 90° = tailwind.
- `WindHeadingDeg(value)` — degrees with same 0°=tailwind convention. Stored as
  `−value + 90°` wrapped to `[0,360)` (`source/lob_builder.cpp`), so internal
  90° = tailwind, 270° = headwind.

Both satisfy "12 or 0 is tailwind, 6 or 180 is headwind" (`include/lob/lob.h`).
Speed is `WindSpeedFps` or `WindSpeedMph` (`MphT → FpsT`). Default 0 fps. Validation: internal radian value checked against `±360°` → `kLobErrorWindHeadingOOR`.

@section model-wind-usage Usage in the solver

`DsDx` forms `v − w` as `CartesianT<FpsT>(ctx.wind.x, 0, ctx.wind.z)` and uses `|v−w|` to scale drag (`source/solve_step.cpp`). No vertical wind; uniform and constant. Jump consumes only `w_z` via `CalculateCrosswindAngleGamma(MphT(w_z), v)` and the Litz/Boatright formulas (@ref model_spin).

@section model-wind-limitations Limitations

- No gradient with altitude/range, no vertical wind.
- Headwind/tailwind scales `|v−w|`; crosswind affects drag only via Boatright's `CDa` (`source/lob_builder.cpp`).

@section model-coriolis Coriolis

When the shooter supplies both azimuth and latitude, lob adds McCoy's rotating-Earth correction. `BuildCoriolis` (`source/lob_builder.cpp`) requires both; otherwise terms are zeroed.

Validation:
```
|azimuth| ≤ 360° else kLobErrorAzimuthOOR
|latitude| ≤ 90° else kLobErrorLatitudeOOR
```
With `Ω = 7.292115e-5 rad/s` (`source/constants.hpp`):
```
cos_l_sin_a = 2·Ω·cos(lat)·sin(az)
sin_l       = 2·Ω·sin(lat)
cos_l_cos_a = 2·Ω·cos(lat)·cos(az)
```
stored as `ctx.coriolis.{cos_l_sin_a, sin_l, cos_l_cos_a}`.

`DsDx` (`source/solve_step.cpp`) adds:
```
dvx -= vy·cos_l_sin_a + vz·sin_l
dvy += vx·cos_l_sin_a + vz·cos_l_cos_a
dvz += vx·sin_l       − vy·cos_l_cos_a
```
after drag, before gravity. No Eötvös variation; effect folded into `deflection`/`elevation`. Tests in `test/source/lob_coriolis_test.cpp` cover N/S/E/W azimuths.

