---
type: DesignDecision
title: Builder Pattern
description: Name what you know; the rest is defaulted and validated.
tags: [architecture, api]
generated: {by: human:joel, at: 2026-10-09T00:00:00Z}
verified: [{by: human:joel, at: 2026-10-09T00:00:00Z}]
status: stable
---

# Builder Pattern

A ballistic solution takes dozens of optional, interacting inputs. A
flat solve function would have unusable arity and no place for
cross-field validation, such as a zero distance without a velocity
being meaningless. The builder lets callers name only what they know,
defaults the rest, and validates the whole set before the solver runs.

The two stages also let high-fidelity branches ask "is this geometry
complete?" without penalizing minimal builds: a sparse caller gets a
well-formed solution, a fully described projectile unlocks the demanding
models. See [Builder Pattern](builder-pattern.md) for the builder surface.
