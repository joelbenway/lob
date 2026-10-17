---
type: Concept
title: BC/Velocity-Band Transformation
description: Turning several measured coefficients into one smooth drag curve.
tags: [ballistic-coefficient, splines, pchip, interpolation]
generated: {by: human:joel, at: 2026-10-09T00:00:00Z}
verified: [{by: human:joel, at: 2026-10-09T00:00:00Z}]
status: stable
---

# BC/Velocity-Band Transformation

This is how a bullet's measured ballistic coefficients at several
velocities become a single drag curve the solver can fly. It matters
because the alternative, picking one coefficient, is wrong at the ends
of the speed range precisely where long-range shooting lives.

## The problem with switching

The straightforward way to use several coefficients is to pick the one
whose measured speed is nearest, and switch as the bullet accelerates.
Do not do that. A coefficient that jumps produces a drag curve with a
step in it, and a step in the drag curve is a step in the trajectory
and in its derivative. The integrator would have to cope with a
discontinuous right-hand side, and the solver's smoothness assumptions
would no longer hold exactly where the bullet is moving fastest.

So the measurements are treated as samples of a smooth underlying
function rather than as a lookup table. The bullet flies the smooth
curve, and the measurements are merely where that curve is pinned.

## What the caller supplies

A list of velocity and coefficient pairs, ordered by increasing
velocity. Each velocity becomes a Mach number through the local speed
of sound, so the same measurements work at different altitudes and
temperatures. Each coefficient is normalized to the same atmosphere
convention the library uses internally, so the numbers mean what they
mean everywhere else in lob.

The measurements must be well-formed: strictly increasing velocity,
positive and finite coefficients, and velocities inside the range the
reference drag curve covers. Violations are rejected at build time
rather than producing a curve nobody intended. Tables are not sorted
for you, because silently reordering a caller's data hides a mistake
rather than fixing it.

## Building the curve

Two splines are constructed and then combined.

The **retardation curve** is the inverse of the bullet's coefficient
against Mach, fitted through the measured pairs. Because the operation
lob needs is a division of the reference curve, it is convenient to
interpolate the reciprocal, and doing so keeps the arithmetic local
to the spline.

The **reference drag curve** is the selected standard curve on the same
knots the library always uses, so nothing about the representation
changes when bands are supplied.

The bullet's curve is their product:

```text
Cd(M) = S(M) * Cd_std(M)
```

Rather than evaluating two splines per step, the product is merged
once, at build time, into a single spline. The merge preserves the
derivative as well as the value at every knot, using the product rule,
because the integrator's accuracy depends on the curve being smooth in
its derivative as well as its height. The result is indistinguishable,
to the solver, from any other drag curve it has ever been handed.

## Why this interpolation

The fitted curve is a shape-preserving interpolant rather than a
natural cubic spline. It passes exactly through each measured pair,
which is what "the manufacturer measured this" demands, and between
measurements it does not overshoot the way a natural spline does.
Overshoot would invent drag that nobody measured, in the region between
data points where the bullet is actually flying.

Outside the measured range the curve is held flat, at the value of the
nearest measurement. A bullet leaves the muzzle slower than any
published band and arrives slower than the fastest band, so extrapolation
is not an edge case, it is most of the flight. Holding the edge value
is the assumption that needs the fewest additional facts. A caller who
knows better can supply additional bands near the ends and define the
behavior explicitly.

## The constant case

If every supplied coefficient is the same, the retardation curve is
flat and the whole construction collapses to dividing the reference
curve by one number, which is exactly what the single-coefficient path
does. The tests assert the two paths agree, so a caller who happened to
measure one bullet at several speeds gets the same answer as a caller
who trusted a single number for it.

## Limitations

- Bands and a custom drag table are mutually exclusive. Supplying both
  discards whichever came first; lob does not blend a measured table
  with measured coefficients.
- Extrapolation outside the measured range is flat, by choice, as
  described above.
- The atmosphere convention applies to every band uniformly. There is
  no per-band atmosphere reference, which is not a limitation anyone
  has yet needed.
- Coefficients are only as good as the velocities they were measured
  at. The transformation is exact at the measurements and
  interpolative between them, which is the best that any method can do
  with the data given.

## Related

- [Ballistic Coefficient](ballistic-coefficient.md): what is being interpolated, and
  why one number is a compromise.
- [Drag Model](drag-model.md): the reference curve this
  multiplies, and how the merged result is represented.