---
type: Concept
title: Inverse Solving
description: Turning a range into the sight adjustment that hits it.
tags: [inverse, adjustments, solver, workflow]
generated: {by: human:joel, at: 2026-10-09T00:00:00Z}
verified: [{by: human:joel, at: 2026-10-09T00:00:00Z}]
status: stable
---

# Inverse Solving

A forward solve answers "where does this shot land". An inverse solve
answers the question a shooter actually has: given a range, what do I
hold or wind by to hit it. The two are different questions, and the
second is the more common one.

The answer comes back as a pair of adjustments in angle units,
measured from the zero the rifle is set to, so what the caller gets is
dope rather than a raw elevation. Windage is likewise reported as a
hold-off at the range asked for.

## Why it takes work

A forward solve integrates a trajectory and reads off where the
projectile lands. That is easy, because nothing has to be found. The
inverse asks for an angle, which means solving for one: find the launch
angle whose trajectory meets the target, then report the difference
between that angle and the zero.

That is a root-finding problem nested inside a numerical integration,
and doing it naively means integrating repeatedly from scratch for
every range in a table.

## Two ways to ask

lob offers two entry points, backed by the same angle solver, and they
differ in how much they trust the geometry.

**The iterative solve** does the honest thing. It integrates, finds the
launch angle that hits the target by iterating, and reports the
adjustment. This is the authoritative answer, and the one to use for
anything published or compared against.

**The fast solve** skips the iteration entirely. It takes the forward
solution you already have and warps it with the closed-form geometric
correction, the same correction the angle solver uses to take one step.
No integration happens. It is substantially cheaper, and it is only
valid where the forward solve actually reached the range you are asking
about, because a shot that fell short has no residual to correct.

## How the work is avoided

The pipeline is what makes the iterative solve affordable across a
whole table of ranges.

First, one forward pass establishes two things at once: how much the
shot misses by at every range, and whether it can reach those ranges at
all. Then, for each range, that miss is fed into the closed-form
correction as a starting guess. Because the correction is derived from
the geometry, the guess lands very close to the true answer, and the
iteration normally converges in a step or two rather than searching
from nothing.

So the integration is not repeated per range from scratch. It happens
once, and the per-range work is a short iteration seeded from data that
was already computed. That is the whole reason asking for a table of
dope is not dramatically more expensive than asking where one shot
lands.

## An asymmetry that is deliberate

The two entry points do not agree on what to do when a range cannot be
reached, and the difference is intentional.

The iterative solve detects it: the angle search fails, so it stops and
reports only the ranges it could solve, returning a shorter result. The
caller sees exactly how far the solution got.

The fast solve cannot detect it, because it never evaluates whether the
shot reaches the range. It would happily produce a number. So it does not
stop, and the burden of not misusing it is on the caller, which is
exactly what the API documentation warns about.

The design bet is that a short array is a recoverable result and a
plausible wrong number is not.

## Edges the API handles for you

A range of zero is not an error. It is the shooter's own position, and
it is reported as zero adjustment rather than being dropped or
producing a division by zero.

Results that came back non-finite from the forward pass are skipped
rather than converted, because there is nothing to convert.

Ordering matters for the table, and a table that is not in increasing
order of range is rejected rather than silently reordered. Sorting a
caller's data would hide a mistake instead of surfacing it, which is the
same reasoning as not reordering velocity bands.

## Which to call

Use the iterative solve for published tables, for anything compared
against another solution, and anywhere correctness matters more than
time.

Use the fast solve when you already have the forward outputs, you know
they reached their ranges, and you want to avoid a second integration
sweep. Spot-check a few ranges against the iterative solve to confirm
that its error is acceptable for the trajectories you care about. That
check is cheap and it is the whole cost of using the fast path.

## What backs it

The inverse tests cover the sign conventions against the forward solve,
confirm that adjustments grow monotonically with range, exercise the
zero-range and unreachable cases, and check both the raw-pointer and
array forms of the API agree.

The one worth calling out is the cross-check between the two paths. They
are run against each other with a nonzero aerodynamic jump, and they are
required to agree. That is not merely a numeric consistency check: it is
a test that both paths applied the muzzle jump the same way, which is
the property that makes the adjustment meaningful relative to a rifle
that was zeroed through a different code path.

## Related

- [Angle Solver Mechanics](angle-solver.md): the root-finding this
  page sits on top of.
- [Shared Angle Solver](shared-angle-solver.md): why the same routine
  serves the zero and the adjustment.
- [Point-Mass Model](point-mass-model.md): the trajectory being
  inverted.