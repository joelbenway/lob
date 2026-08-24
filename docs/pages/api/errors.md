@page api_errors Errors

# Errors

All lob errors are values, never exceptions. `Builder::Build()` writes one
`LobErrorT` / `lob::ErrorT` into `Context::error`; every `LobSolve*` checks it
first and returns `0` on any non-`kNone` (`source/lob_solve.cpp`). `Builder`
never throws and never allocates — setters overwrite stored values, `Build()`
stops at the first failing stage and reports that error only.

`kLobErrorNotFormed` is internal (sentinel before any stage runs) and never
exposed from a successful `Build()`; `kLobErrorNumberOfErrors` is the count.

## Catalog

| `LobErrorT` | Trigger (first failing `Build*` stage) |
|---|---|
| `kNone` | success |
| `kBallisticCoefficientRequired` | no drag source and no `BallisticCoefficientPsi` |
| `kBallisticCoefficientOOR` | `BC ≤ 0` |
| `kInitialVelocityRequired` | missing or `≤0` fps (`BuildStability`) |
| `kZeroDataRequired` | neither `ZeroAngleMOA` nor `ZeroDistanceYds` |
| `kZeroDistanceOOR` | `ZeroDistanceYds ≤ 0` |
| `kZeroAngleOOR` | explicit `abs(ZeroAngleMOA) > 45°` |
| `kZeroUnreachable` | `SolveAngle` returns NaN for the zero distance (vacuum-parabola seed, 10 iters, ±45°) |
| `kRangeAngleOOR` | `abs(RangeAngleDeg) ≥ 90°` |
| `kDiameterOOR` / `kLengthOOR` / `kMassOOR` | non-positive geometry needed for `stability_factor` |
| `kMeplatDiameterOOR` / `kBaseDiameterOOR` / `kNoseLengthOOR` / `kTailLengthOOR` / `kOgiveRtROOR` | Boatright geometry out of `[0,∞)` or `ogiveRtR ∉ [0,1]` |
| `kAltitudeOfFiringSiteOOR` / `kAltitudeOfBarometerOOR` / `kAltitudeOfThermometerOOR` | `abs(alt) ≥ 65 617 ft` (`kIsaStratosphereAltitudeFt`) |
| `kAirPressureOOR` | `AirPressureInHg < 0` |
| `kHumidityOOR` | `RelativeHumidityPercent ∉ [0,100]` |
| `kAzimuthOOR` | `abs(azimuth) > 360°` |
| `kLatitudeOOR` | `abs(latitude) > 90°` |
| `kWindHeadingOOR` | internal `wind_heading_rad` outside `±360°` |
| `kMaximumTimeOOR` | `MaximumTime < 0` |
| `kMachDragTableTooShort` | `size < 2` |
| `kMachDragTableNotMonotonic` | Machs not strictly increasing |
| `kMachDragTableInvalid` | `Mach < 0`, `Cd < 0`, non-finite, or cubic extrapolation negative at 0/5 |
| `kBcBandsTooShort` | `size < 2` |
| `kBcBandsInvalid` | `size > 16`, any `fps ≤ 0` / `BC ≤ 0` / non-finite, or `max(fps)/c ≥ 5.0` |
| `kBcBandsNotMonotonic` | `fps` not strictly increasing |
| `kInternalError` | Boatright supersonic integration to `Mach 1.2` exceeded 60 s (temporary guard) |
| `kNotFormed` | internal sentinel — not returned |
| `kNumberOfErrors` | count |

C names are in `include/lob/lob.h`; C++ scoped names are `lob::ErrorT::k*`
(`include/lob/lob.hpp`). `Build()` order is `BuildEnvironment` → `BuildSpline` →
`BuildCoefficients` → `BuildWind` → `BuildOpticHeight` → `BuildStability` →
`BuildCoriolis` → `BuildBoatright` → `BuildLitzAerodynamicJump` → `BuildOptions` →
`BuildZeroAngle` (`source/lob_builder.cpp` `LobBuilderBuild`) — first failure wins.

@section api-errors-handling Handling

```cpp
lob::Context ctx = lob::Builder().BallisticCoefficientPsi(0.5).Build();
if (ctx.error != lob::ErrorT::kNone) {
  // ctx.error is one of the rows above; no throw, no alloc
}
size_t n = lob::Solve(ctx, ranges, &outs); // returns 0 if ctx.error != kNone
```

`LobSolve`/`LobSolveInverse` also return `0` on null pointers, `size==0`,
non-finite `zero_angle`, `velocity==0`, `speed_of_sound≤0`, or non-monotonic
`pranges` — those are solve-time guards, not `LobErrorT`.
