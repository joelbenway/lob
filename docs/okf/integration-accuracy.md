---
type: Concept
title: Integration Accuracy and Step Size
description: What step size buys you, what it costs, and where the library's recommendations stop.
tags: [numerics, accuracy, step-size, validation]
generated: {by: human:joel, at: 2026-10-09T00:00:00Z}
verified: [{by: human:joel, at: 2026-10-09T00:00:00Z}]
status: stable
---

# Integration Accuracy and Step Size

lob takes a fixed step through the trajectory and lets the caller
choose how big it is. This page is about what that knob actually buys,
what it costs, and where the library's guidance stops being true.

## The knob

Accuracy is not a guarantee lob makes; it is a parameter the caller
controls. The step is expressed in down-range inches, and the default is
a deliberately coarse grid that is fast and, over the range lob is
validated against, good enough. A caller who needs more can ask for a
finer step, and lob will spend the time.

The step is not a suggestion the library second-guesses. It is never
widened automatically, in either direction. If a caller asks for a
coarser step, that is their call, and it is a call they can get wrong.

Two details make the stepping behave. The final step is shortened to
land exactly on the requested range instead of overshooting it and
reporting a position past the target. A shot that stops making down-range
progress before reaching the requested range can leave the solver in a
repeating state rather than terminating cleanly.

## What a smaller step buys

For smooth trajectories, Heun's global integration error is typically
proportional to the square of the step size, while work is roughly
inversely proportional to step size. Halving the step typically reduces
truncation error by about four times and doubles work, until an error
floor or knot effects dominate.

Returns diminish. Past some point the error is no longer dominated by
the integration and shrinking the step buys nothing but time, and where
that point falls depends on the trajectory rather than on the solver.

## Why fixed rather than adaptive

An adaptive integrator estimates its own error and grows or shrinks the
step to hit a target. lob does not, and the reason is that adaptivity
has a cost that is not obvious from the outside.

Every adaptive step carries an error estimate, which means extra work
per step, and every step carries a decision, which means a branch that
is taken differently each time. Both are exactly the kind of cost that
the [drag spline](cubic-hermite-splines.md) representation was chosen to
avoid. It also makes the cost of a solve less predictable, since the
number of steps becomes a property of the trajectory rather than of the
caller's setting.

What is given up is honest and worth naming: lob cannot tighten the step
automatically where a trajectory happens to be hard and loosen it where
the trajectory is easy. It spends the same effort on a flat subsonic
descent as on a transonic climb. A caller who wants that behavior has
to ask for it by choosing the step.

## Where the guidance stops

The default step is validated over a range of trajectories, and the
library documents which. It also documents a coarser step that a caller
may opt into, and the range of trajectories over which that coarser
step is defensible. Past that envelope the coarser step stops being a
recommendation and becomes a guess, and the library says so rather than
quietly continuing to offer it.

The current values are in the API reference rather than here. That is
deliberate: a documented default that can drift out of date in prose is
worse than a pointer to the constant that defines it.

## What backs it

The accuracy claims are measured against registered reference
trajectories rather than asserted. Those checks confirm that the
default step reproduces the reference trajectories within reporting
granularity for the six registered cases and their listed ranges. The
convergence checks require successive step-size differences to decrease
or reach their numerical floor for the tested cases and channels; they
do not measure error against the reference trajectories.

The step-size question is settled by paired experiments rather than
argument: the same trajectory solved at two step sizes, compared, so
that the measured sensitivity is a property of the trajectory rather
than an assumption about it. Where the comparison shows no meaningful
change, that is recorded as a limit of what the step size can be held
responsible for, which is a more useful statement than a single number.

The method choice is benchmarked separately, at matched accuracy: each
candidate method is run at the step size that reaches a common error
target, so the comparison is between methods rather than between
arbitrary settings. That is what establishes the choice of a
second-order step over a higher-order one, which
[Heun Integration](heun-integration.md) argues from the literature.

## Limitations

- Fixed step, no embedded error estimate, no adaptive stepping.
- The per-step air-density variant is gated by the inverse solver
  rather than always on. Most shots do not climb far enough for it to
  matter and it is not free, so the cheap path is the default and the
  accurate path is paid for per range where it is needed. See
  [Point-Mass Model](point-mass-model.md).

## Related

- [Heun Integration](heun-integration.md): why this order of accuracy
  and not another.
- [Point-Mass Model](point-mass-model.md): the equations being stepped
  and why the step is measured in distance.
- [Cubic Hermite Splines](cubic-hermite-splines.md): the smoothness the
  integrator is relying on.