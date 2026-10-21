---
type: Concept
title: Solver Context
description: What a context promises about lifetime, reuse, threading, and failure.
tags: [api, context, lifetime, errors, threading]
generated: {by: human:joel, at: 2026-10-09T00:00:00Z}
verified: [{by: human:joel, at: 2026-10-09T00:00:00Z}]
status: stable
---

# Solver Context

Everything lob computes is a function of two things: a gun and its
ammunition, and the conditions it is shooting in. The context is those
two things, resolved once into the form the solver consumes. This page is
about what that object promises, because the promises are unusually
strong and callers are entitled to rely on them.

## One build, then read forever

The context has exactly one phase. Building it resolves inputs,
validates them, derives the quantities that would otherwise be recomputed
per shot, and produces the context. After that it is read-only.

There are no mutators on a context and no way back to the builder. That
single fact is what makes the rest of its behavior predictable: a
context does not change underneath you, so a result computed from it
today describes the same configuration as a result computed from it
next week.

It also means building is where you pay. A context that took real work to
derive is worth keeping, and the intended pattern is one context per
gun-and-load-and-conditions, rebuilt when any of those change and
reused for every range in between.

## It is self-contained after build

This is the part that catches people, so it is worth being exact about.

While you are configuring a builder, lob may hold references to data you
supplied, such as a custom drag table. Those references are borrowed for
the duration of the build only. Once the build completes, the derived
quantities are copied in, and the context no longer points at anything
you own.

The practical consequence: your builder and its tables may be
discarded immediately after building. They do not need to outlive the
context, and they cannot be mutated underneath it, because it is not
looking at them any more. A caller who misunderstands this and frees a
table too early, or keeps one they think is still feeding solves, has a
use-after-free that the library cannot catch.

A context is also a plain value. It can be copied trivially, snapshotted,
and stored in a container, which matters when you want a per-thread or
per-slot copy rather than sharing one.

## Safe to share, because it never changes

Solves take the context by const reference and mutate nothing in it.
Nothing is allocated during a solve, and no shared state is written.

The consequence is that one built context can back any number of solves,
from any number of threads, at the same time, with no locking. Two
threads solving against the same context cannot interfere, because
neither of them writes. This is not a best-effort thread safety claim,
it falls out of the immutability.

The corollary is that a context is only as stable as the inputs that
produced it. If the weather changes, the context is now a description of
conditions that no longer hold, and no amount of thread safety makes its
answers current. Rebuilt on changed conditions, not patched.

## Failures arrive two different ways

lob reports failure through two separate channels, and telling them
apart is most of what a caller needs to know about errors.

**Build-time failure** is reported on the context itself, as a single
error code. Building is staged, and it stops at the first stage that
fails, so the code names one problem: the earliest one found. Fixing it
may reveal another. The design is deliberate, since a builder that
returns a list of everything wrong is a builder that has to allocate.

**Solve-time failure** is reported by the count of results produced. A
solve that produces nothing returns zero.

Zero from a solve does not necessarily mean the context is bad. Beyond
a context error, a solve also refuses on its own guards: missing or
empty arguments, values that are not finite, and a range table that is
not in increasing order. Those refuse the request without setting an
error code, because they are faults in the call rather than in the
configuration. So the diagnostic order is: check the context's error
first, and if it is clean, a zero return can indicate either invalid
arguments or an unreachable first range.

The full catalog of what each error code means, and which stage raises
it, is generated alongside the API reference. What is worth knowing here
is the shape of the model: errors are values rather than exceptions, one
error at a time, and configuration faults are separated from call faults.

## What is worth knowing about the contents

A context holds more than inputs. Along with the projectile and the
conditions, it carries the results of work done once so it need not be
redone per shot: the drag curve as spline coefficients, the atmospheric
quantities the integrator divides by every step, the wind resolved into
the shooting frame, gravity rotated into that frame, and the corrections
for spin drift and aerodynamic jump.

Two fields behave differently from the rest in ways that show up in
results. The zero angle is a number found once, and everything downstream
is measured against it. The spin drift factor is absent when the inputs
were too thin to compute it, which is how the solver knows to fall back
to the lower-fidelity model rather than report a zero.

## Related

- [Builder Pattern](builder-pattern.md): the mutating half of the
  lifecycle.
- [Angle Solver Mechanics](angle-solver.md): what produced the zero
  angle stored on the context.
- [Inverse Solving](inverse-solving.md): the solve-time contract,
  including the deliberate asymmetry between its two entry points.