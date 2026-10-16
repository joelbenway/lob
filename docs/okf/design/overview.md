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

- [C ABI narrow waist](hourglass.md) — one implementation, every language.
- [Strong unit types](units.md) — wrong units fail to compile.
- [No dynamic allocation](no-alloc.md) — suit targets that forbid the heap.
- [Builder pattern](builder.md) — name what you know; the rest is defaulted and validated.
- [Heun integration](heun.md) — second-order steps, per BRL experience.
- [Performance posture](perf.md) — speed from modeling, measured per trajectory.
- [Shared angle solver](shared-solver.md) — one routine serves zero and inverse alike.
