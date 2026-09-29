@page model_wind_coriolis Wind and Coriolis

# Wind and Coriolis

Wind and Coriolis are the two velocity-coupled corrections to the point-mass
trajectory. Both are built once as vectors/scalars in `LobContext` and applied
every integration step in `DsDx`. They are grouped because neither changes
the drag curve or atmosphere — they only add `v`-dependent accelerations in
`DsDx` (`source/solve_step.cpp` drag + Coriolis).

@section model-wind-vector Wind vector

Wind enters the equations only as `v − w` in the drag term and as `w_z` in the
jump models. `Build` stores the profile once as frame-resolved nodes
`ctx.wind_nodes[]` (`LobWindNode`, `include/lob/lob.h`): each caller-supplied
`LobWindPoint` gives direction (`heading_deg`) + magnitude (`speed_mph`) with
a measurement height, resolved to horizontal fps at `Build` and pitched once
into shooting-frame components. A single-point profile stores bit-identically
with the uniform setters (`test/source/validation_signal_test.cpp`,
`SinglePointEqualsUniform`).

Heights normalize to the fixed 1-ft reference via the Hellmann power law at
`Build`; per-query `GetWind` (`source/solve_step.cpp`) lerps nodes downrange
and scales by the configured shear exponent (0 = uniform wind, the default —
scaling is opt-in). Under inclined fire the height datum is the tilted
shot-parallel plane, not flat muzzle level. Jump reads the lateral muzzle
node `wind_nodes[0].z`, which incline cannot touch (lateral is the pitch
axis).

Heading convention (unchanged): 12 o'clock / 0° = tailwind, 6 o'clock / 180° =
headwind, 3 o'clock = pure crosswind. Speed is `WindSpeedFps` or
`WindSpeedMph` (`MphT → FpsT`); default 0 fps. Out-of-range headings are
rejected (`kLobErrorWindHeadingOOR`); profile shape errors are
`kLobErrorWindProfile*` (`include/lob/lob.h`).

Contract: `docs/superpowers/specs/WIND_INTERFACE_SPEC.md` (cited, not
duplicated); profile behavior is pinned in
`test/source/lob_wind_profile_test.cpp`.

@section model-wind-usage Usage in the solver

`DsDx` forms `v − w` with the frame-resolved wind from the profile query (`GetWind` in `source/solve_step.cpp`) — downrange-lerped nodes with optional power-law altitude scaling — and uses `|v−w|` to scale drag. No vertical wind input; uniform wind is the single-point case. Jump consumes only `w_z` via `CalculateCrosswindAngleGamma(MphT(w_z), v)` and the Litz/Boatright formulas (@ref model_spin).

@section model-wind-limitations Limitations

- No vertical wind input; altitude/range gradients are opt-in (profiles plus shear exponent).
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

