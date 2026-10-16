---
type: DesignDecision
title: Performance Posture
description: Speed from modeling choices, measured per trajectory.
tags: [performance]
generated: {by: human:joel, at: 2026-10-09T00:00:00Z}
verified: [{by: human:joel, at: 2026-10-09T00:00:00Z}]
status: stable
---

# Performance Posture

Speed comes from modeling choices, not heroic optimization. Standard
drag tables are resampled once onto a small set of smooth spline knots,
so each evaluation is a handful of branch-light arithmetic ops rather
than a table walk. The curve view caches its segment index, which pays
because Mach falls monotonically through a flight. The hot path has no
heap, no exceptions, and no virtual calls.

No universal speed claim is made here: the benchmark tree reproduces
the comparisons locally, and per-trajectory measurement beats any
published figure.
