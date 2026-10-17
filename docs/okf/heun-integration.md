---
type: DesignDecision
title: Heun Integration
description: Second-order one-step integration, per BRL experience.
tags: [numerics, integration]
generated: {by: human:joel, at: 2026-10-09T00:00:00Z}
verified: [{by: human:joel, at: 2026-10-09T00:00:00Z}]
status: stable
---

# Heun Integration

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
need impractically fine steps to hold accuracy. We did not take McCoy's
word for it: the benchmark tree pits RK4 against Heun at matched
accuracy, and Heun comes out faster. Error falls as the step
shrinks, at linear cost. Callers with unusual regimes measure their
own trajectories rather than trusting a global number. See [Integration Accuracy and Step Size](integration-accuracy.md)
for accuracy notes.
