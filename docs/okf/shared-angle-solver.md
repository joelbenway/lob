---
type: DesignDecision
title: Shared Angle Solver
description: One routine serves zero-finding and inverse alike.
tags: [numerics, solver]
generated: {by: human:joel, at: 2026-10-09T00:00:00Z}
verified: [{by: human:joel, at: 2026-10-09T00:00:00Z}]
status: stable
---

# Shared Angle Solver

Zero-finding at build time and adjustment-solving at inverse time are
the same mathematical problem: find the launch angle whose trajectory
meets the target. So lob solves both with one shared routine.

Sharing guarantees the zero a rifle is zeroed with and the adjustments
that assume that zero are computed with identical tolerance, bounds,
and jump handling; two implementations could drift subtly apart. It
also halves maintenance: a fix to convergence behavior applies to both
features at once, with a smaller diff than guarding every caller. See
[Shared Angle Solver](angle-solver.md) for the dedicated treatment, and
[Drag](drag-model.md) for how the curves it flies were
built.
