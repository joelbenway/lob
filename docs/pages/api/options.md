@page api_options Solver Options

# Solver Options

Termination and stepping options live in `LobContext` and are set via the
builder. All default to “off” unless noted — the solver then uses its
1-yard Heun grid and runs to the last requested range unless another stop
fires. (`max_time` defaults to `NaN` for “no limit”; see below.)

@section api-options-step Step size

- **Builder:** `StepSize(uint16_t inches)` / `LobBuilderStepSize`
- **Context:** `step_size` inches, `0` = default 1 yard (36 in)
- **Code:** `source/solve_step.cpp` `SolveStep` — `Δx = min(target−x, step_size)` (last step clamped), Heun `k1+k2` + trapezoidal `TOF += 2·Δx/(vx_old+vx_new)`.

Smaller `Δx` reduces truncation error monotonically; `benchmark/ode.cpp` shows
linear cost in `1/Δx`. Default 1 yd meets the `validation_overview` tolerances
out to 1000 yd — tighten only when you need it and can pay for it.

@section api-options-time Maximum time

- **Builder:** `MaximumTime(double seconds)` — `≥ 0` else `kMaximumTimeOOR`
- **Context:** `max_time` seconds, `NaN` = no limit (builder leaves `NaN`, `BuildOptions` checks `<0` only)
- **Solver:** `source/lob_solve.cpp` `LobSolve` — if `s.TOF ≥ max_time` and previous `< max_time`, the last `Output` is linearly interpolated to `max_time` via `LerpOutput`. No further ranges are attempted.

@section api-options-speed Minimum speed / energy

- **Builder:** `MinimumSpeed(uint16_t fps)`, `MinimumEnergy(uint16_t ft·lbf)`
- **Context:** `minimum_speed` fps — `BuildOptions` (`source/lob_builder.cpp`) computes `speed_from_energy = CalculateVelocityFromKineticEnergy(minimum_energy, mass)` (`source/calc.hpp`) and stores `max(explicit, from_energy)`.
- **Solver:** if `|v| ≤ minimum_speed` and previous `> minimum_speed`, last `Output` is interpolated to that speed (`source/lob_solve.cpp`). Both are floors — whistling past either stops the trajectory.

`max_time`, `minimum_speed`, `minimum_energy`, and tumble are forward-stop “stop and interpolate” conditions: the output at the stop is a `LerpOutput` between the two Heun steps straddling the limit, and `LobSolve` returns only up to that point. Any of those stops may therefore shorten `kForwardSolves` independently of angle solving. Separately, `LobSolveInverse` stops when `SolveAngle` returns `NaN` for an unreachable angle (@ref api_inverse).

@section api-options-optic Optic and zero helpers

- `OpticHeightInches(double)` — feet internally (`FeetT(InchT(...))`), default 1.5 in (`source/lob_builder.cpp` `BuildOpticHeight`).
- `ZeroAngleMOA` vs `ZeroDistanceYds (+ ZeroImpactHeightInches)` — exactly one is required; the distance path is solved via the shared angle solver (@ref num_zero_angle, @ref design_shared_solver) with vacuum-parabola seed `0.5·g·R/v0²` clamped to ±45°.
- `RangeAngleDeg` — inclination, gravity rotated `gx=−g·sin`, `gy=−g·cos` (`source/lob_builder.cpp` `BuildEnvironment`); validated `|angle| < 90°`.

@section api-options-atm Atmosphere, wind, Coriolis

- Atmosphere inputs (`AltitudeOfFiringSiteFt`, `AirPressureInHg`, `AltitudeOfBarometerFt`, `TemperatureDegF`, `AltitudeOfThermometerFt`, `RelativeHumidityPercent`) only affect `speed_of_sound` and `drag_coeff` at `Build()` time — see @ref model_atmosphere.
- Wind (`WindHeading`/`WindHeadingDeg` + `WindSpeed*`) only enters as `v−w` in `DsDx` and `w_z` in jump — see @ref model_wind_coriolis.
- Coriolis needs both `AzimuthDeg` and `LatitudeDeg`; otherwise zeroed.

None of these allocate; `Context` is self-contained after `Build()` (@ref api_context).
