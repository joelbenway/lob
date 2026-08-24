@page bc_worked_example BC Bands Worked Example

# BC Bands Worked Example

This page complements @ref bc_transformation with a visual and a copy-pasteable trajectory. All data are from the test suite so the numbers are checkable.

@section bc-we-figure Resulting drag curve

@image html bc_transformation.svg "G1 reference vs BC-band transformed drag curve" width=800px

*Figure — same as in @ref bc_transformation — G1 reference (blue) and the curve transformed with bands `0.20@2000, 0.30@2500, 0.40@3000` fps (red) at ISA `c=1116 fps` (band Machs ≈1.79, 2.24, 2.69, marked). Dashed gray is `G1/0.30`. Static `docs/figures/bc_transformation.svg` from `source/tables.hpp` via the same PCHIP as the library.*

How to read it:

- Low BC (0.20) → high drag (`/0.20 = 5×`); high BC (0.40) → lower drag (`/0.40 = 2.5×`).
- Between marks the curve is smooth PCHIP, not kinked.
- Outside marks scaling is flat — parallels the reference shifted by the edge BC. See @ref bc_transformation for the math and flat-padding rationale.

@section bc-we-worked Worked example — bands to trajectory

```cpp
const float fps[] = {2000.f, 2500.f, 3000.f};
const float bcs[] = {0.20f,  0.30f,  0.40f};

lob::Context ctx = lob::Builder()
    .BCAtmosphere(lob::AtmosphereReferenceT::kIcao)
    .BCDragFunction(lob::DragFunctionT::kG1)   // or G7 for the usual use
    .DiameterInch(0.308)
    .MassGrains(168.0)
    .InitialVelocityFps(3100)
    .ZeroDistanceYds(100.0)
    .BCVelocityBands(fps, bcs)                  // last call wins
    .Build();
assert(ctx.error == lob::ErrorT::kNone);

// Inspect the scaled drag (optional):
lob::spline::CurveView curve(lob::spline::kKnots, ctx.drags);
// curve.Eval(1500.f/ctx.speed_of_sound) ≈ Cd_std/0.20  (below bands)
// curve.Eval(2250.f/ctx.speed_of_sound) between  /0.20 and /0.30

// Trajectory:
std::array<uint32_t,4> ranges{300, 600, 900, 1200}; // feet
std::array<lob::Output,4> outs{};
lob::Solve(ctx, ranges, &outs);          // forward (inches)
lob::SolveInverse(ctx, ranges, &outs);   // or MOA adjustments directly
```

For comparison, a constant `BC=0.30` is `Builder().BallisticCoefficientPsi(0.30)` and yields a proportionally scaled curve — the bands case uses more drag low and less drag high, so the trajectory drops less at short range and more at long range than any single-BC fit.

The three regimes are locked by `test/source/lob_builder_test.cpp` (`BCVelocityBandsNonConstantClampingAndInterpolation`): below the slowest band clamped to first BC, between bands strictly between the bracketing scalings and matching the retardation product to `2e-3`, above the fastest band clamped to last BC.

@section bc-we-limits Limitations in this example

Same limits as the transformation itself — see @ref bc_transformation for the full list (N bounds, monotonicity, Mach <5, last-setter wins, flat extrapolation). The point of the figure is that you can see the flat extrapolation outside the marks.
