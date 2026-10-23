---
type: DesignDecision
title: Angle Solver Mechanics
description: How the shared angle solver finds a launch angle, and what backs it.
tags: [numerics, solver]
generated: {by: human:joel, at: 2026-10-09T00:00:00Z}
verified: [{by: human:joel, at: 2026-10-09T00:00:00Z}]
status: draft
---

# Angle Solver Mechanics

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

## The zero angle is a number, not a procedure

The zero comes out of this solver when the caller does not supply one, and
is stored on the context as a single angle in minutes of arc. If the caller
already knows the zero, the build bounds-checks it and returns, skipping
the search entirely. That is the shape the whole design wants, and it is
worth being explicit about why.

A zero describes a gun, not a shot. Once you know the distance and the
impact height, the mechanical angle that sends the bullet through the
optic axis is a fixed property of the rifle, the ammunition, and the
conditions. Nothing about a later forward solve or inverse solve changes
it. So the search runs once, when the context is built, and what stays
behind is the answer rather than the machinery that found it.

Everything downstream then reads that number. A forward solve launches
from it. An inverse solve reports adjustments relative to it. The angle
solver uses it as the seed for every range in a table. None of them pays
for the search again, which is why a context that has been built is
cheap to keep solving against for as long as the caller likes.

The number is also portable in the useful sense. It is an angle, so it
does not care what units the shooter entered them in, and it does not
depend on the range being asked about. A caller who zeroed in SI units,
who queries in yards, and who hands the context to a colleague solving
in meters is passing around one number that means the same thing to all
of them.

There is a second saving available. A caller who already knows their
zero, from a dope book or a previous session, can state it directly. The
build then bounds-checks it and returns, skipping the seed and the
search altogether. For a shooter working from published data that is the
normal path, and it makes the zero a pure input rather than a
computation.

## How it works

The solver alternates between two cheap steps until it settles.

**Fire and measure.** Integrate a full trajectory from the candidate
angle out to the range, and return the residual `f`. The
[Heun](heun-integration.md) integrator supplies the step, against the
[drag curve](drag-model.md) the context already carries.

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
  small-angle approximation for a level launch is `0.5 * g * R / v²`
  (the exact low-angle form is `½ asin(gR/v²)`). That approximation is
  the closed-form answer to the simplest possible version of the problem,
  so it is a principled place to start. It is clamped to the search bounds
  before use.
- **Inverse seeds from the forward solve.** A forward solve already
  reports how far the shot lands off at that range. Applying the same
  one geometric step to the zero angle with that error corrects the
  zero into a near-miss guess for the target. The cheap
  `SolveInverse` stops there; `SolveInverseELR` hands that guess to
  the lapse-scaled iteration, which then refines it.

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
- **Iteration earns its keep.** The ELR refined answer starts from the
  single geometric step and agrees with it where the lapse correction
  is small, including when aerodynamic jump is nonzero. Where lapse
  scaling matters — long, high shots — the iteration is allowed to
  depart from the closed-form answer, because that departure is the
  whole point of the ELR path.
- **The two density variants agree** where both are valid, so for
  ordinary trajectories the fast and ELR operations stay close. Where
  they do not agree, the caller chose which physics to pay for by
  choosing which operation to call.

## Related

- [Heun Integration](heun-integration.md): the integrator that fires each candidate
  trajectory.
- [Drag Model](drag-model.md): the curve the integration
  flies against.