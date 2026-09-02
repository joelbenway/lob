@page model_atmosphere Atmospheric Model

# Atmospheric Model

@section model-atm-overview Overview

The atmosphere determines two quantities the integrator needs: air density
(through `drag_coeff`) and speed of sound (for Mach).  lob implements the
International Standard Atmosphere (ISA) with humidity corrections (McCoy
pp. 166–168; Huang saturation-pressure formula).  Lapse, humidity and
barometric corrections are evaluated once at `Build()` time from the firing-
site conditions, and then optionally stratified per step via `ctx.k_lapse`
(`source/constants.hpp` `kHydrostaticExponent`/`kBarometricExponent`,
`source/lob_builder.cpp` `BuildDynamicDensity`, `source/solve_step.cpp`
`DsDx`/`FastDsDx`).

@section model-atm-inputs Inputs

Builder inputs that affect the atmosphere (`source/lob_builder.cpp`):

- `AltitudeOfFiringSiteFt` — feet, `|alt| < kIsaStratosphereAltitudeFt`
  (65 617 ft).  Outside that → `kLobErrorAltitudeOfFiringSiteOOR`.
- `AirPressureInHg` — inHg, `≥ 0`.  If absent, ISA sea-level 29.92 inHg
  propagated via the barometric formula from the site altitude.
- `AltitudeOfBarometerFt` — where the pressure was read; defaults to the
  firing-site altitude.  The pressure is barometrically shifted to the firing
  site (`source/lob_builder.cpp`).
- `TemperatureDegF` — degrees F.  If absent, `kIsaSeaLevelDegF` (59 °F) at
  sea level lapsed to the site.  If a thermometer altitude is given,
  `AltitudeOfThermometerFt`, the temperature is shifted similarly
  (`source/lob_builder.cpp`).
- `RelativeHumidityPercent` — 0–100, defaults to 0.

All altitudes are validated against the same ISA ceiling.

@section model-atm-formulas Formulas

Lapse and barometric formula (`source/calc.hpp`, `source/calc.hpp`):

```
T(h) = max(T0 − L·h, T_min)               with L = 0.00356616 °F/ft, T_min = −69.7 °F
p(h) = p0·(1 − L·h / T_R)^e · exp(...)     e = g/(R·L), with tropopause/stratosphere branch
```

`BarometricFormula` handles the tropopause at 36 090 ft and then an
isothermal exponential above it, matching the implementation.

Humidity enters via Huang's saturation-pressure formula
(`source/calc.hpp`):

```
p_sat(T) = exp(A − B/(T_C+D1) − C·ln(T_C+D2))   distinct A,B,C,D1,D2 for water vs ice
```

and two linear corrections (McCoy p. 167–168, with `h = RelativeHumidityPercent` ∈ [0, 100] as a percent value, not a 0–1 fraction; `0.00378` and `0.00140` already include the percent scaling):

```
ρ/ρ0 = (p/p0)·(T0_R/T_R)
ρ_corr = 1 − 0.00378·h·p_sat/p0
c_corr = 1 + 0.00140·h·p_sat/p0
ρ = ρ0_ISA · (ρ/ρ0) · ρ_corr
c = c(T)·c_corr    with c(T)=49.0223·√T_R
```

`ρ0_ISA = 0.0764742 lb/ft³`, `c0_ISA = 1116.45 fps`.  Army Standard Metro uses
`0.0751265 lb/ft³` and is converted at the BC level, not here.

@section model-atm-output Output in Context

`BuildEnvironment` writes `ctx.speed_of_sound` (fps) and the intermediate
`Impl::air_density_lbs_per_cu_ft`, which is converted to `ctx.drag_coeff`
via `CalculateCdCoefficient` using the solver's ρ·π/8 relationship at BC=1
(`source/lob_builder.cpp`, `source/calc.hpp`).  It also writes
`ctx.k_lapse` (`(1/R−L/g)/T_R`, `source/lob_builder.cpp`
`BuildDynamicDensity`) — the projected linear density lapse coefficient used
per step.

Per-step stratification (`source/solve_step.cpp`):
```
u = −k_lapse·P·G          // dimensionless altitude projected onto G
ρ/ρ0 = 1 − u·(1−α·u)      // α=(e−1)/2e, e=kHydrostaticExponent
c/c0 = 1 − β·u            // β=1/2e
Cd = curve(Mach)·drag·(ρ/ρ0),  Mach=|v|/c·(c0/c)
```
`DsDx` scales `drag_coeff`/`speed_of_sound` by `ρ/ρ0`/`c/c0`; `FastDsDx`
uses firing-site values.  `LobSolve` (forward) and `BuildBoatright`/`BuildZeroAngle`
use `Fast*` fast path; `LobSolveInverse` uses `FastSolveAngle` or `SolveAngle`
per-range gated on forward `drop>100ft` (`elevation < −1200in`,
`source/lob_solve.cpp` `kDynamicDropThresholdIn`).

@section model-atm-limitations Limitations

- Per-step lapse is gated: forward solves and `drop≤100ft` inverse ranges stay
  on `Fast*` (firing-site density) for speed; only `drop>100ft` inverse ranges
  pay `DsDx`/`SolveStep`/`SolveAngle`.  For the ranges lob is validated against
  (`test/source/lob_env_test.cpp`) the error is bounded by the tests' tolerances.
- Temperature is lapsed with the simple linear ISA lapse, not McCoy's
  exponential variant `CalculateTemperatureAtAltitudeMcCoy` (kept for
  reference only, `source/calc.hpp`).
