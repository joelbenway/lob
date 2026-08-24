@page bc_transformation BC/Velocity-Band Transformation

# BC/Velocity-Band Transformation

This page is the worked case study for a recent numerical feature: allowing
multiple `BC + velocity` pairs to tailor the standard drag curve.  It follows
the implementation in `source/splines.hpp`, `source/lob_builder.cpp`
and is validated by `test/source/lob_builder_test.cpp`,
`test/source/splines_test.cpp`.

@section bc-t-motivation Motivation

A single ballistic coefficient assumes the bullets form factor relative to
the standard projectile is constant with Mach.  Real bullets violate this —
the shock structure and base flow change with Mach — so a single BC is a
compromise.  A manufacturer might quote:

- G7 BC 0.20 at 3000 fps (≈ Mach 2.69)
- G7 BC 0.25 at 2500 fps (≈ Mach 2.24)
- G7 BC 0.30 at 2000 fps (≈ Mach 1.79)

Using any one of those numbers for the whole flight either over- or under-
predicts drag outside its band.  The transformation lets the caller supply
all of them and have lob produce a single smooth `Cd(M)` that is
`Cd_std(M)/BC(M)` at the band Machs and PCHIP-interpolated between them.

@section bc-t-reference Reference drag curve

The reference is a standard curve, e.g. G7, sampled at 87 Mach points and
projected onto 16 PCHIP knots (`source/splines.hpp`, `source/tables.hpp`).
Call `Cd_std(M)` the spline evaluation of that curve (@ref num_splines).  For
the figure below the reference is **G1** (the most familiar shape) with
`BC=1` so `Cd_ref = Cd_std`; the same machinery applies to any G* curve.

@section bc-t-pairs BC/velocity pairs

The caller supplies `N` pairs `(fps_i, BC_i)` via
@ref LobBuilderBCVelocityBands / `Builder::BCVelocityBands`
(`include/lob/lob.h`):

```
fps  = [2000, 2500, 3000]   strictly increasing,  >0,  highest < Mach 5
BC   = [0.20, 0.30, 0.40]   each >0 finite
2 ≤ N ≤ 16
```

`fps_i` is converted to Mach `M_i = fps_i / c` where `c = ctx.speed_of_sound`
(~1116 fps at ISA sea level).  `BC_i` is multiplied by the atmosphere factor:

```
BC_icao = BC · k   with k = kArmyToIcaoBcConversionFactor if Army, else 1
```

so the curve is always ICAO-normalized (`source/lob_builder.cpp`).

The bands example used in tests and in the figure satisfies all constraints;
the maximum band Mach `3000/1116 ≈ 2.69 < 5.0` and `N=3`.

@section bc-t-transform Transformation

Two splines are built:

1. **Scaling (retardation) spline** `S(M) = 1/BC(M)` via
   `MakeRetardationCoefs` (`source/splines.hpp`):

   ```
   inputs:  M_i = fps_i/c,   Y_i = 1/BC_icao,i
   padded:  (0, 1/BC_0) and (5, 1/BC_{N-1})   // flat extrapolation
   S = PCHIP(inputs ∪ padding) resampled onto the 16 knots
   ```

   Flat padding implements “constant extrapolation outside the user Mach
   range” — below the slowest band and above the fastest band `BC` is held
   constant at the edge value.

2. **Drag spline** `Cd_std(M)` — the selected G* curve resampled onto the same
   knots.

They are **merged** via `Merge` (`source/splines.hpp`):

```
Cd(M) = S(M) · Cd_std(M)
dCd/dM = S'·Cd_std + S·Cd_std'
```

The merged coefficients are the Hermite data `(Cd, dCd/dM)` at each knot, so
the solver's `CurveView::Eval` returns the product directly with correct
derivatives.

For a constant `BC` the inputs are collinear, `S(M)=1/BC` is flat, and
`Merge` degenerates to `Cd_std/BC` — bit-identical to the single-BC path
(modulo floating-point order).  The constant-BC tests verify `1e-4` agreement.

Math summary:

```
Given M_i, BC_i, build S(M) = PCHIP(1/BC | M) with flat ends on [0,5]
       Cd(M) = S(M) · Cd_std(M)
```

@section bc-t-interp Interpolation

`S(M)` is a true **PCHIP** (Fritsch–Carlson) interpolant, not linear:

- Endpoints interpolate exactly: `S(M_i)=1/BC_i`.
- Interior intervals use harmonic-mean tangents when the adjacent secants agree
  in sign, otherwise `0` — this preserves monotonicity and prevents the
  overshoot that a natural cubic spline would produce.
- Outside the band Machs `S` is constant (the padding), so drag does not
  diverge.

Between bands the scaled drag varies smoothly; the test
`BCVelocityBandsNonConstantClampingAndInterpolation`
(`test/source/lob_builder_test.cpp`) checks the three regimes:

- `M < smallest band` → clamped to first BC (`Eval(1500/c)` matches `/0.20`);
- `M between bands` → strictly between the two bracketing scaled drags and
  matches the explicit retardation-curve product to `2e-3`;
- `M > largest band` → clamped to last BC (`Eval(3500/c)` matches `/0.40`).

For a visual and a copy-pasteable trajectory see @ref bc_worked_example. That page reuses the same `0.20@2000, 0.30@2500, 0.40@3000` fps bands and the same `c=1116 fps` figure (static `docs/figures/bc_transformation.svg` from `source/tables.hpp` via the same PCHIP as the library).

@image html bc_transformation.svg "G1 reference vs BC-band transformed (thumbnail — see BC Bands Worked Example for full size)" width=400px

@section bc-t-limits Limitations and edge cases

- `N < 2` → `kLobErrorBcBandsTooShort`; `N > 16` → `kLobErrorBcBandsInvalid`.
- `fps` not strictly increasing → `kLobErrorBcBandsNotMonotonic`.
- Any `fps ≤ 0` or any `BC ≤ 0` or non-finite → `kLobErrorBcBandsInvalid`.
- Fastest `fps/c ≥ 5.0` → `kLobErrorBcBandsInvalid` (would require Mach ≥5 evaluation where the reference tables end).
- Tables are not reordered; the caller must sort by velocity.
- Only the last drag-table setter wins: a `MachVsDragTable` followed by `BCVelocityBands` (or vice versa) discards the earlier one (`include/lob/lob.h`).
- Custom `MachVsDragTable` and `BCVelocityBands` are mutually exclusive for the same build.
- Flat extrapolation outside bands is deliberate; if a different edge behavior is desired, add explicit bands near 0 and 5 Mach.
- No per-band atmosphere conversion — the single `BCAtmosphere` applies to all bands uniformly.

