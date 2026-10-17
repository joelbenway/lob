---
type: Concept
title: Ballistic Coefficient
description: How one number stands in for a bullet's entire drag curve.
tags: [ballistic-coefficient, drag, form-factor]
generated: {by: human:joel, at: 2026-10-09T00:00:00Z}
verified: [{by: human:joel, at: 2026-10-09T00:00:00Z}]
status: stable
---

# Ballistic Coefficient

A ballistic coefficient is the single number that stands in for a
projectile's entire drag curve. It says how much drag a bullet suffers
compared to a standard reference projectile, and dividing a standard
curve by it produces that bullet's curve:

```text
Cd_bullet(M) = Cd_std(M) / BC(M)
```

A larger coefficient means a slipperier bullet and less drag. That one
number carries a surprising amount of information, which is why
ballistic coefficient is the primary unit that the long-range
literature, the manufacturers, and shooters all exchange.

It works as a summary because the dominant influence on drag is the
bullet's shape relative to its frontal area, and for bullets of similar
shape that ratio varies only modestly with speed. lob expresses
coefficients in pounds-mass per square inch, the customary American
unit, and internally performs the division by folding it into the drag
curve's own representation. The solver is therefore never aware that a
ballistic coefficient exists: it evaluates one curve and that is all.

## Standard curves

The coefficient only means something relative to a reference projectile,
and lob ships the standard references that the literature uses: the G
series, a set of reference projectiles spanning blunt and streamlined
shapes, tabulated by the Ballistic Research Laboratory and published
through JBM Ballistics. G1 is a flat-base projectile with a short
ogival nose, and it is the default because it is the most widely assumed
when nothing else is specified. G7 is a long boat-tail reference with a
long ogive, and it is what long-range rifle shooters usually want,
because a modern match bullet resembles it far more closely than it
resembles G1. The others cover projectiles that sit between those two.

Choosing the reference is not the same as describing your bullet. The
reference sets the *shape* of the curve; the coefficient sets the scale.
A shooter who knows only a coefficient is implicitly saying "my bullet
has this shape and is this slippery relative to it."

## One coefficient, or several

Using a single coefficient for the whole flight assumes the bullet's
shape relative to the reference is the same at every speed. For many
bullets that assumption holds well enough, and a single coefficient is
the whole interface: one number, one standard curve, done.

Real bullets are less obliging. The shock structure around the nose and
the flow off the base both change as the bullet accelerates, so the
ratio to the reference is not constant, and a single coefficient is a
compromise that is wrong at both ends even when it is right somewhere
in the middle. The mismatch is worst for the modern high-performance
bullets that long-range shooting cares most about.

This is why manufacturers increasingly publish a coefficient at
several velocities rather than one. It is also why lob accepts them
directly. Given coefficients at a handful of speeds, it builds the
smooth curve those measurements describe, rather than forcing the
caller to pick one and live with the error.

The obvious way to use several coefficients would be to switch between
them discretely as the bullet passes each measured speed. lob does not,
because a discontinuity in the drag curve is a discontinuity in the
trajectory and in its derivative, and a solver that never sees a kink
is a solver that does not have to special-case one. Instead the
measurements are interpolated into a smooth curve first, and that curve
is what the bullet actually flies. The method is the subject of
[BC/Velocity-Band Transformation](bc-velocity-band-transformation.md).

## Related

- [Drag Model](drag-model.md): the curve that a
  coefficient scales.
- [BC/Velocity-Band Transformation](bc-velocity-band-transformation.md): how several
  measured coefficients become one smooth curve.