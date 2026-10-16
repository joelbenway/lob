---
type: DesignDecision
title: Strong Unit Types
description: Unit mixups fail to compile instead of flying wrong.
tags: [architecture, safety]
generated: {by: human:joel, at: 2026-10-09T00:00:00Z}
verified: [{by: human:joel, at: 2026-10-09T00:00:00Z}]
status: stable
---

# Strong Unit Types

Inside the solver, quantities carry their engineering units as types,
and each type does at least three jobs. It prevents mixing up
arguments: two parameters that would both be plain numbers become
distinct types, so passing them in the wrong order fails to compile.
It converts implicitly within its category, so a call site states
intent once in whatever unit reads naturally and the factor follows.
And it otherwise behaves like its underlying value, at no runtime cost.
Mixing categories is ill-formed: lengths, speeds, and angles stay
distinct.

The motive is a class of bug with no test failure but a wrong
trajectory: a unit mixup deep in the integrator, atmosphere, or spin
corrections.

The types cost nothing at runtime and stop at the ABI boundary, where
plain numeric values cross. Safety is recovered at the ergonomic layer
by naming units in the builder setters, so a caller states what each
number means. Direct C callers must get units right themselves.
