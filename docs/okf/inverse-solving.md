---
type: Concept
title: Inverse Solving
description: Turning a range into the sight adjustment that hits it.
tags: [inverse, adjustments, solver, workflow]
generated: {by: human:joel, at: 2026-10-09T00:00:00Z}
verified: [{by: human:joel, at: 2026-10-09T00:00:00Z}]
status: draft
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

## Four ways to ask

lob offers four entry points: a cheap and an ELR variant for each of
the forward and inverse questions. They differ in how much they trust
the geometry — and in how much air they recompute.

**The cheap inverse** (`SolveInverse`) does the honest cheap thing. It
runs one forward solve, then applies the closed-form geometric
correction — the same correction the angle solver uses to take one
step — and reports that. No iteration happens. It is substantially
cheaper than iterating, and it is only valid where the forward solve
actually reached the range you are asking about, because a shot that
fell short has no residual to correct.

**The ELR inverse** (`SolveInverseELR`) does the expensive thing. It
starts from the cheap answer, then iterates every nonzero range with
the lapse-scaled angle solver — recomputing air density and the speed
of sound along each candidate trajectory — until it converges. This is
the authoritative answer, and the one to use for anything published,
compared against, or fired past a mile. One qualification: the
iteration re-solves elevation only. Windage stays the geometric
hold-off from the cheap answer; there is no iterative wind solver, so
an ELR table's deflection column is the cheap deflection, not a
lapse-refined one.

The same split applies to the forward question: `Solve` integrates
with firing-site air throughout, while `SolveELR` recomputes the air
along the flight. There is no automatic switching between them. Long
shots do not upgrade themselves to the expensive path; short shots do
not downgrade. The caller picks by calling.

## How the work is avoided

The pipeline is what makes the ELR inverse affordable across a whole
table of ranges.

First, one cheap forward pass establishes two things at once: how much
the shot misses by at every range, and whether it can reach those ranges
at all. Then, for each range, that miss is fed into the closed-form
correction as a starting guess. Because the correction is derived from
the geometry, the guess lands very close to the true answer, and the
ELR iteration normally converges in a step or two rather than searching
from nothing. `SolveInverseELR` always pays for the lapse-scaled
iteration; `SolveInverse` stops at the guess and reports it.

## Truncation, not fabrication

The cheap and ELR operations agree on what to do when a range
cannot be reached, and the agreement is intentional.

Both detect it: a fall-short forward row carries a range below the one
asked for, so both stop and report only the ranges they could solve.
The caller sees exactly how far the solution got. What neither does is
silently produce a plausible wrong number for an unreachable range —
that burden used to sit with callers of the old one-step conversion,
which is gone.

The design bet is that a short array is a recoverable result and a
plausible wrong number is not.

## Edges the API handles for you

A range of zero is not an error. It is the shooter's own position.
Both `LobSolveInverse` and `LobSolveInverseELR` count zero-range entries
and report them as zero adjustment. Results that came back short of the
requested range stop both operations at the first such entry, because
there is nothing to refine. Iterative solving returns early when the
angle is NaN.

Ordering matters for the table. Both inverse operations validate
increasing range order (via the forward pass) and reject tables that are
not in increasing order rather than silently reordering them. Sorting a
caller's data would hide a mistake instead of surfacing it, which is the
same reasoning as not reordering velocity bands.

## Which to call

Use the ELR operations for published tables, for anything compared
against another solution, for extreme range, and anywhere correctness
matters more than time.

Use the cheap operations when you already know the trajectory stays in
roughly firing-site air, when you need an answer dozens of times per
second, or when you want a seed to refine yourself. Spot-check a few
ranges against the ELR answer to confirm that its extra work is
negligible for the trajectories you care about. That check is cheap and
it is the whole cost of using the cheap path.

## What backs it

The inverse tests cover the sign conventions against the forward solve,
confirm that adjustments grow monotonically with range, exercise the
zero-range and unreachable cases, and check all three call shapes of the
API agree.

The one worth calling out is the cross-check between the cheap and ELR
paths. They are run against each other with a nonzero aerodynamic jump,
and they are required to agree where the lapse correction is small. That
is not merely a numeric consistency check: it is a test that both paths
applied the muzzle jump the same way, which is the property that makes
the adjustment meaningful relative to a rifle that was zeroed through
a different code path.

## Related

- [Angle Solver Mechanics](angle-solver.md): the root-finding this
  page sits on top of.
- [Shared Angle Solver](shared-angle-solver.md): why the same routine
  serves the zero and the adjustment.
- [Point-Mass Model](point-mass-model.md): the trajectory being
  inverted.