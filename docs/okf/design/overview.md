---
type: DesignDecision
title: Design Decisions
description: Why lob is built the way it is.
tags: [architecture, api, numerics, performance]
generated: {by: human:joel, at: 2026-10-09T00:00:00Z}
verified: [{by: human:joel, at: 2026-10-09T00:00:00Z}]
status: stable
---

# Design Decisions

This page records the "why" behind the choices a contributor might
question. Claims here are deliberately qualitative: exact sizes,
tolerances, and measured figures live in the code and tests, where the
compiler and CI keep them honest.

## C ABI narrow waist

lob exposes a C ABI with plain aggregates and an opaque builder, with a
header-only C++ wrapper above it that restores type-safe overloads and
scoped enums. The implementation lives once, in C++; the wrapper
forwards to it.

The hourglass shape keeps the solver independent of language bindings:
any language with a C FFI can call lob without a C++ toolchain or
name-mangling concerns, while C++ callers get ergonomics back through
the wrapper. See @ref api_builder for the builder surface.

## Strong unit types

Inside the solver, quantities carry their engineering units as types:
lengths, speeds, and angles are distinct, and mixing categories fails
to compile while same-category conversion is explicit. The motive is a
class of bug with no test failure but a wrong trajectory — a unit
mixup deep in the integrator, atmosphere, or spin corrections.

The types cost nothing at runtime and stop at the ABI boundary, where
plain numeric values cross. Safety is recovered at the ergonomic layer
by naming units in the builder setters, so a caller states what each
number means. Direct C callers must get units right themselves.

## No dynamic allocation

lob never allocates from the heap and never throws. Builders stage
into an inline buffer, tables are borrowed rather than copied, and the
integration state is a small fixed footprint.

The motive is callers the library author never meets: embedded targets
and real-time contexts that forbid heap use, and integrators who want
to vendor the library with no allocator hooks. The consequence is a
caller contract — borrowed tables must outlive the build — stated on
the builder surface rather than hidden.

## Builder pattern

A ballistic solution takes dozens of optional, interacting inputs. A
flat solve function would have unusable arity and no place for
cross-field validation, such as a zero distance without a velocity
being meaningless. The builder lets callers name only what they know,
defaults the rest, and validates the whole set before the solver runs.

The two stages also let high-fidelity branches ask "is this geometry
complete?" without penalizing minimal builds: a sparse caller gets a
well-formed solution, a fully described projectile unlocks the demanding
models.

## Heun integration

lob integrates the trajectory with Heun's method, a one-step
second-order predictor-corrector, stepping in down-range distance with
a caller-configurable step around a coarse default. Time of flight
rides along as an integrated component, not a post-step estimate.

The method choice follows Ballistic Research Laboratory experience, via
McCoy (*Modern Exterior Ballistics*): "The cumulative experience of the
Ballistic Research Laboratory has shown that the one-step,
second-order methods are, in the practical sense, optimum solutions of
the point-mass trajectory problem." Higher-order methods cost roughly
twice as much without earning back the step size; first-order methods
need impractically fine steps to hold accuracy. Error falls as the step
shrinks, at linear cost — callers with unusual regimes measure their
own trajectories rather than trusting a global number. See @ref num_ode
for accuracy notes.

## Performance posture

Speed comes from modeling choices, not heroic optimization. Standard
drag tables are resampled once onto a small set of smooth spline knots,
so each evaluation is a handful of branch-light arithmetic ops rather
than a table walk. The curve view caches its segment index, which pays
because Mach falls monotonically through a flight. The hot path has no
heap, no exceptions, and no virtual calls.

No universal speed claim is made here: the benchmark tree reproduces
the comparisons locally, and per-trajectory measurement beats any
published figure.

## Shared angle solver

Zero-finding at build time and adjustment-solving at inverse time are
the same mathematical problem — find the launch angle whose trajectory
meets the target — so lob solves both with one shared routine.

Sharing guarantees the zero a rifle is zeroed with and the adjustments
that assume that zero are computed with identical tolerance, bounds,
and jump handling; two implementations could drift subtly apart. It
also halves maintenance: a fix to convergence behavior applies to both
features at once, with a smaller diff than guarding every caller. See
@ref design_shared_solver for the dedicated treatment, and
[Drag](../ballistic_model/drag.md) for how the curves it flies were
built.
