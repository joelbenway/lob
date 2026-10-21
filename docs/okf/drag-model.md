---
type: Concept
title: Drag Model
description: How lob describes a bullet's drag, and how the solver uses it.
tags: [drag, splines, performance]
generated: {by: human:joel, at: 2026-10-09T00:00:00Z}
verified: [{by: human:joel, at: 2026-10-09T00:00:00Z}]
status: stable
---

# Drag Model

Drag dominates an exterior ballistics trajectory. Everything else in the
model, gravity, wind, Coriolis, spin corrections, bends the path; drag
sets how fast the bullet bleeds energy and where it stops. So the
question that matters most is not which forces lob models but how it
represents the one that matters most.

lob's answer: a projectile's drag is a curve of drag coefficient
against Mach number, resampled once onto a small set of smooth spline
knots and evaluated as arithmetic. Every path that supplies a drag
curve, whether it is a published standard or your own measurements,
ends up in that same representation, and the solver only ever sees the
spline.

## What a drag curve means

The curve answers: at this speed, how much does the bullet slow down?
Speed is expressed as Mach number rather than raw velocity, because
what resists the bullet is the difference between its speed and the
speed of sound in the surrounding air, and that dividing line moves with
altitude and temperature. A curve is therefore a statement about a
projectile flying through air, independent of the conditions that air
happens to be in.

The published standard curves lob ships, G1 through G8, come from
Ballistic Research Laboratory drag studies. They are broadly similar
bullet shapes, measured and published through JBM Ballistics. The
distinction between them is mostly nose shape and boat tail, and the
practical difference shows up in how much each one loses energy
through the transonic region.

## Four ways to describe a bullet's drag

**A standard curve, scaled by ballistic coefficient.** The common case:
pick G1 or G7, then say how much of it applies to your bullet with a
ballistic coefficient. The curve's shape does the work and the
coefficient scales it, which is why a single number can stand in for a
whole measured table. A coefficient only makes sense relative to an
atmosphere convention, so lob converts it between the Army Standard
Metro and ICAO references internally, so the same number always means
the same thing to the solver.

**Your own Mach versus drag table.** If you have measured data, or a
manufacturer published one, you can supply the curve directly. lob
fits it onto the same spline knots the standard curves use, so a custom
table costs nothing at solve time beyond the fact that it was fitted at
build time. The table must be ordered by increasing Mach and cannot
report negative drag. If it does not span the full Mach range, the
spline extrapolates smoothly past the ends rather than clamping, on the
understanding that you told us the shape of your data; extrapolation
that would imply a negative drag coefficient is rejected instead of
quietly believed. Pad the table with explicit entries at the ends if
you want to define the edge behavior yourself.

**Ballistic coefficients measured at several velocities.** Real bullets
are not G1-shaped at every speed. Manufacturers publish drag
coefficients measured at a handful of velocities, and the true curve is
narrower than any standard curve, which is exactly the regime where
long-range accuracy is won or lost. lob transforms those bands into a
form-factor curve and blends it with the standard curve you selected,
giving a projectile-specific drag function instead of a scaled
standard one. The transformation is worth understanding on its own, and
it is covered in the ballistic coefficient chapter.

**Raw spline coefficients.** An escape hatch for the cases the others
do not fit: round-tripping a context between lob instances, or
importing coefficients computed elsewhere. It sets the curve directly
and skips both table fitting and coefficient scaling, which means the
ballistic coefficient and atmosphere settings stop mattering to
anything. Reach for it when you already hold the answer.

## How the solver uses the curve

At every step of the trajectory integration, the solver evaluates the
curve at the projectile's current Mach number and scales it by the
local air density. Two things follow from how that works.

Mach is recomputed each step from the projectile's speed and the local
speed of sound, so a bullet climbing through changing air is
corrected as it goes rather than being evaluated once against fixed
conditions. Relatedly, there are two variants of the step: one holds
the density and speed of sound at the firing site, and the other
recomputes both per step for shots that climb far enough to care.
Which one the inverse solver picks is a speed-versus-fidelity
tradeoff; they are not guaranteed to agree when altitude-dependent drag
or speed-of-sound corrections are nonzero, and any agreement claim is
limited to conditions where those corrections do not affect the result.

Evaluation happens outside the curve's defined Mach domain by holding
the edge value. The standard curves are defined across a range that
covers everything from subsonic through supersonic flight, and a
trajectory that never leaves that range never hits the clamp.

Drag tables are resampled onto their spline knots once, at build time,
and the standard curves are projected at compile time, so the solver
pays only for evaluating a short polynomial. Evaluating a drag curve is
one of the most frequent operations in the whole solver, which is why
that evaluation is a handful of arithmetic operations rather than a
table search.

## Related

- [Performance Posture](performance-posture.md): why the spline
  representation is chosen for evaluation cost.
- [Angle Solver, in Depth](angle-solver.md): the root-finding
  that fires candidate trajectories against this curve.
- [Heun Integration](heun-integration.md): the integrator that steps
  through it.