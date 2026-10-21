---
type: Concept
title: Cubic Hermite Splines
description: The interpolation scheme every drag curve in lob is stored as.
tags: [splines, pchip, interpolation, numerics]
generated: {by: human:joel, at: 2026-10-09T00:00:00Z}
verified: [{by: human:joel, at: 2026-10-09T00:00:00Z}]
status: stable
---

# Cubic Hermite Splines

Every drag curve in lob is stored as a piecewise cubic Hermite spline
fitted with shape-preserving tangents. That one choice is what makes the
solver fast, and the shape-preserving part is what makes it safe.

## Why a spline at all

A drag curve is a function the integrator evaluates hundreds of times per
trajectory. Whatever representation is chosen has to be cheap to
evaluate and cheap to locate a point on. A lookup table means a search
and a branch on every evaluation. The spline inverts that: the curve is
reduced ahead of time to a fixed set of knots and a small block of
coefficients, and evaluating it is arithmetic with no searching at all.

The second requirement is less obvious. The integrator's accuracy
depends on the curve being smooth, not merely continuous. A curve with a
kink in it puts a kink in the trajectory, and an integrator taking fixed
steps cannot resolve a kink exactly however small the step becomes. So
the representation has to preserve smoothness.

## What the representation is

The curve is divided into intervals at the knots. Within each interval
the curve is a cubic, described in local coordinates by the value and
slope at the interval's two endpoints. Specifying value and slope at both
ends of every interval is enough to pin down a cubic, which is why this
is called a Hermite spline: it interpolates the values it is given and
lets the slopes decide the curvature in between.

What makes lob's version special is how those slopes are chosen.

## The tangent rule

The naive choice for the slope at a knot is an average of the data
either side of it. That produces a smooth curve, and it also produces
curves that swing wildly between closely spaced data points: a natural
cubic spline famously overshoots, inventing values higher than anything
in the data. For a drag curve that is not a cosmetic problem. An
invented peak in drag between two measured points is a peak the
projectile actually flies through.

lob fits tangents with the Fritsch-Carlson rules instead, and the
decision tree is the whole design:

- At the ends of the curve, the tangent is the slope of the adjacent
  span.
- Inside, if the data slopes one way on the left and the other way on
  the right, the knot is a local extreme and the tangent is zero. A
  horizontal tangent at a peak or trough cannot overshoot, because the
  curve flattens there instead of turning sharply.
- Everywhere else, the tangent is a weighted average of the two
  neighboring slopes, taken as a harmonic mean and weighted toward the
  closer neighbor.

The result is that a curve passing through monotonic data stays
monotonic, and a curve through a peak does not overshoot it. Every drag
curve in lob, from the compiled-in standard curves through to a custom
table and a velocity-band transformation, is fitted with these same
rules.

## Knots

The knots are a fixed, deliberately non-uniform set spanning the Mach
range the reference curves cover, stored alongside the coefficient
tables so the standard curves need no construction at runtime at all.

They are clustered where the curves are hardest to represent, which is
through the transonic region where drag turns sharply, and spread out
where the curve is smooth and easy to follow. The placement was chosen
by measuring, not by taste: candidate knot sets were scored against the
full reference tables, and the set that shipped is the one that kept
the worst-case deviation across every standard curve within the
accuracy budget the solver is validated to. Since the knots are fixed,
any incoming table can be resampled onto them without re-deciding where
they should go, so a caller's custom data lands on the same footing as
the standard curves.

## Evaluation

Evaluation holds a view over the knot and coefficient arrays along with
the index of the interval it last used. A new Mach number is resolved by
moving that index, forward or backward, until the Mach number falls
inside the interval it points at.

This is worth a second look, because it is not a binary search. The
projectile's position progresses along the curve in one direction, so
the cached index walks the knots the same way the projectile does and
finding the right interval costs a single comparison in the overwhelming
majority of steps. Mach may vary during flight (not necessarily moving
monotonically along the Mach axis in all phases), so movement along the
curve is the relevant invariant for the cached index. The rare search,
such as the transonic hunt during angle finding, is where the index has
to move more.

Once the interval is known, evaluating is a polynomial in local
coordinates using that interval's four coefficients, and the derivative
is another. Nothing is searched, nothing is allocated, and the whole
curve is a small contiguous block of numbers.

## Why not linear interpolation

Linear interpolation between samples is simpler still, and lob keeps
an implementation of it around for comparison. It loses on smoothness.
A linear segment is continuous but its slope jumps at every knot, so
the curve is not differentiable there, and the derivative the solver
uses does not exist at those points. The Hermite form is continuously
differentiable everywhere, which is the property the integrator is
actually relying on.

## What backs it

The spline tests check the tangent rules against their definitions,
that a curve passes through its input values with the intended slopes,
that interval location finds the right segment in both directions, that
the cached index behaves when the search reverses, and that the
derivative matches the curve.

Two of them check the budget rather than the machinery. One confirms
the fixed knots keep every standard curve within its accuracy budget
across the whole Mach range. The other measures what that budget is
worth: it perturbs the drag coefficient across the transonic band and
measures the effect on the resulting trajectory, showing the
trajectory impact of the residual curve error is small enough to sit
below the solver's own noise. That is a claim about impact, not about
curve shape, which is the quantity that actually matters to a caller.

## Related

- [Drag Model](drag-model.md): the curves this representation stores.
- [BC/Velocity-Band Transformation](bc-velocity-band-transformation.md):
  the merging step that has to preserve these same properties.
- [Integration Accuracy and Step Size](integration-accuracy.md): what
  the smoothness buys at the level of a whole trajectory.