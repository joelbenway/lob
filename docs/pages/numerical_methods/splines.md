@page num_splines Cubic Hermite Splines

# Cubic Hermite Splines

@section num-splines-why Why splines

Drag is tabulated as `Cd(Mach)`.  The solver evaluates it hundreds of times
per trajectory, so access must be branch-light and monotonicity-preserving.
lob stores drag as cubic Hermite splines with **PCHIP** (Piecewise Cubic
Hermite Interpolating Polynomial, Fritsch–Carlson) tangents.

@section num-splines-math Construction

For each interval `[x0,x1]` with values `y0,y1` and tangents `m0,m1`
(`source/splines.hpp`):

```
h = x1−x0, d = (y1−y0)/h
c0 = y0
c1 = m0
c2 = (3d − 2m0 − m1)/h
c3 = (m0+m1 − 2d)/h²
p(t) = c0 + t·(c1 + t·(c2 + t·c3)),  t = x − x0
```

Tangents (`source/splines.hpp`):

- `n==2` or at an endpoint → secant of the first/last span.
- Interior and `sec_prev·sec_next ≤ 0` → `0` (prevents overshoot).
- Otherwise the weighted harmonic mean

```
w1 = 2·h_next + h_prev,  w2 = h_next + 2·h_prev
m  = (w1+w2) / (w1/d_prev + w2/d_next)
```

which preserves monotonicity for monotonic data.

@section num-splines-knots Knots

`kKnotCount = 16`, `kSegmentCount = 15`, `kCoefsSize = 60`
(`source/splines.hpp`).  Knots span Mach 0 to 5.0:

```
0.00 0.555 0.598 0.760 0.885 0.940 1.003 1.018
1.076 1.213 1.373 1.688 2.126 2.809 3.927 5.00
```

Chosen to keep the worst-case G1/G7 error below `5e-3` across a 5000-point
truth grid interpolated from `dragtable` (`test/source/splines_test.cpp`,
`SplineOptimization::BaselineAccuracyBudget`).  Knot placement is static and
`constexpr`; the G* coefficient tables `kG1Coefs…kG8Coefs` are built at compile
time (`source/splines.hpp`).

`Build` (`source/splines.hpp`) projects an arbitrary input table onto the
knots by evaluating the PCHIP interpolant of that table at each knot, then
fitting a Hermite segment per knot interval via `Segment`.  Hence any input
grid can be resampled without re-optimizing knot locations.

@section num-splines-eval Evaluation

`CurveView` (`source/splines.hpp`) / `Cursor` holds pointers to knots and
coefficients and a cached segment index `idx_`:

- `Clamp(m)` to `[knots[0], knots[N−1]]`.
- `Seek(m)` walks `idx_` forward/backward linearly — `O(1)` for monotonic
  Mach (projectiles only decelerate, so the solver walks downward except for
  rare transonic searches).
- `Eval(m) = PolyVal(c + 4·idx_, m − knots[idx_])`
- `Deriv(m) = PolyDeriv(c + 4·idx_, m − knots[idx_])`

Both are `constexpr`-friendly and allocate nothing.  The 60 floats are 240 bytes
of contiguous storage.

Custom tables (`source/lob_builder.cpp`) produce coefficients via the same
`Build`; BC bands use `MakeRetardationCoefs` + `Merge` (see @ref bc_transformation).

Validation: `test/source/splines_test.cpp` checks secants/tangents, Hermite
endpoint/tangent preservation, binary-search `FindInterval`, monotonic seeks,
derivative correctness and the `5e-3` baseline budget.

@section num-splines-alternatives Why not linear

Linear interpolation (`dragtable::LobLerp` in `source/tables.hpp`) is kept
for reference but is slower to validate for `Cd` integrals; the Hermite form
evaluates with 7 FLOPs and preserves `C¹` continuity, which the benchmark
`benchmark/loblerp.cpp` shows matters for tight loops.
