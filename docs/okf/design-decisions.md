---
type: DesignDecision
title: Design Decisions
description: Why lob is built the way it is.
tags: [architecture]
generated: {by: human:joel, at: 2026-10-09T00:00:00Z}
verified: [{by: human:joel, at: 2026-10-09T00:00:00Z}]
status: stable
---

# Design Decisions

lob's shape is a set of deliberate trade-offs, each recorded on its own
page. Claims here are qualitative: exact sizes, tolerances, and measured
figures live in the code and tests, where the compiler and CI keep them
honest.

- [C ABI narrow waist](c-abi-narrow-waist.md): one implementation, every language.
- [Strong unit types](strong-unit-types.md): wrong units fail to compile.
- [No dynamic allocation](no-dynamic-allocation.md): suit targets that forbid the heap.
- [Builder pattern](builder-pattern.md): name what you know; the rest is defaulted and validated.
- [Heun integration](heun-integration.md): second-order steps, per BRL experience.
- [Performance posture](performance-posture.md): speed from modeling, measured per trajectory.
- [Shared angle solver](shared-angle-solver.md): one routine serves zero and inverse alike.
