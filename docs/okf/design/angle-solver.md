---
type: DesignDecision
title: Shared Angle Solver
description: Zero-finding and inverse-solving are one problem, solved once.
tags: [numerics, solver]
generated: {by: human:joel, at: 2026-10-09T00:00:00Z}
verified: [{by: human:joel, at: 2026-10-09T00:00:00Z}]
status: stable
---

# Shared Angle Solver

lob has exactly one routine that answers "what launch angle makes this
shot go there?". It serves both features that need that answer, and it
is the same routine for both, down to the tolerance, the bounds, and
the iteration policy. This page explains the problem it solves, how it
works, why it is shared rather than duplicated, and what backs it up.

## The problem

Two features need a launch angle solved for:

- **Zero.** The shooter picks a distance and an impact height. lob
  returns the mechanical launch angle that puts the bullet through the
  optic axis at that distance. This is computed once, when the solver
  context is built, and stored as the context's zero angle.
- **Inverse.** The shooter asks for the adjustment that moves impact
  onto a target at some range. lob returns that adjustment *relative to
  the zero angle*, because that is what a shooter actually needs: the
  dope for the gun they just zeroed.

Both reduce to the same residual function. Fire a trajectory from a
candidate angle, integrate it out to the requested range, and measure
the miss:

```text
f(θ) = height_at_range(θ) - optic_height - desired_impact_height
```

The answer is the root of `f`. A zero is the root that lands on the
optic axis; an inverse adjustment is the root that lands on the
target, minus the zero.

Historically a ballistics library grows two implementations here, one
per call site, and the two drift subtly apart. That drift is the whole
reason this is one routine: a rifle zeroed by one solver and adjusted by
the other wears the difference between them as a bias.

## How it works

The solver alternates between two cheap steps until it settles.

**Fire and measure.** Integrate a full trajectory from the candidate
angle out to the range, and return the residual `f`. The
[Heun](heun.md) integrator supplies the step, against the
[drag curve](../ballistic_model/drag.md) the context already carries.

**One geometric step.** Correct the angle in closed form. If the shot
at range `R` landed `f` too low, raise elevation by approximately
`atan(f / R)`:

```text
θ_next = atan( tan(θ) - f / R )
```

This is the flat-fire correction, and it is why the iteration
converges quickly: it already accounts for most of the error, and each
pass only has to trim the remainder.

**Settle or give up.** Repeat until the angle stops moving by more than
the tolerance, then return. The search is bounded to a physically
sensible range of launch angles, and the iteration count is capped, so
the routine always terminates.

**Honest failure.** A shot that cannot reach the requested range at
all (it stalls out, or runs past its time or speed limits) has no
residual to speak of, and neither does a root outside the bounds. In
both cases the solver returns "no answer" rather than a plausible-looking
garbage angle. At build time that becomes an explicit unreachable-zero
error; in an inverse solve it truncates the batch at the range that
failed, so a caller never receives a fabricated adjustment for the
ranges past it.

Two entry points exist, differing only in which density the integration
assumes as the projectile climbs: one holds the firing-site density,
the other recomputes density per step. Inverse solving picks between
them based on how far the shot has to climb. Everything else about the
search, including the shared tolerance and bounds, is identical.

## Seeds carry intent

Iteration needs a first guess, and each call site knows something
better than "zero" about where the answer lies.

- **Zero seeds from the vacuum parabola.** With no drag at all, the
  launch angle that reaches a range is exactly `0.5 * g * R / v²`.
  That is the closed-form answer to the simplest possible version of
  the problem, so it is a principled place to start. It is clamped to
  the search bounds before use.
- **Inverse seeds from the forward solve.** A forward solve already
  reports how far the shot lands off at that range. Applying the same
  one geometric step to the zero angle with that error corrects the
  zero into a near-miss guess for the target, which the iteration then
  refines.

One subtlety is deliberately inside the trajectory rather than at each
call site: aerodynamic jump is added to the launch angle when the
initial velocity is built. So the angle the solver searches over is the
*mechanical* angle the shooter thinks in, and both the zero and the
inverse adjustments inherit jump identically. Neither caller has to
remember to apply it, which is one more way the two paths cannot drift.

## Why sharing matters

**Consistency.** The zero a rifle is zeroed with and the adjustments
that assume that zero come out of identical tolerance, bounds, iteration
policy, and jump handling. Split the routine and each of those becomes a
place the two can disagree.

**Maintenance.** A fix to convergence behavior, a change to the bounds,
a new failure mode: it applies to both features at once, and one guard
inside the shared routine is a smaller and safer diff than the same
guard repeated at every call site.

## What backs it up

The solver tests pin the properties the design depends on:

- **Round-trip.** Firing at a solved angle reproduces the height that
  angle was solved for, so the residual means what the formula says it
  means.
- **Builder and solver agree.** The zero the builder computes is
  identical to calling the shared routine directly, so the builder
  adds no drift of its own on the way in.
- **Iteration earns its keep.** The refined answer agrees with the
  single geometric step, including when aerodynamic jump is nonzero.
  If the iteration ever started drifting away from the closed-form
  answer it would be inventing results the one-step physics does not
  support.
- **The two density variants agree** where both are valid, so choosing
  between them per range is a speed choice rather than an accuracy
  choice.

## Related

- [Heun Integration](heun.md): the integrator that fires each candidate
  trajectory.
- [Drag Model](../ballistic_model/drag.md): the curve the integration
  flies against.