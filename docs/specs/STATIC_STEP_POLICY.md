# Static Step Policy

**Decision (human, recorded 2026-10-01): 288 inches for publication; 576
recorded as measured-clean headroom.** Coordinator-adopted from the
`CLOSURE_INTEGRATION_PLAN.md` Task 1 recommendation as written — reversible
by one-word override (swap the cap in the rule below; the reasoning stands
either way).

The pre-registered E3 rule said 576, and 576 passed it — but the 1.21x
governor margin plus unprobed mainstream paths (profile joints, Boatright,
bands/custom, tails, inverse) make 576 a measurement, not yet a default.
288 carries 4.9x measured margin, ~49x effective (thresholds sit at 10% of
reporting granularity, so margin against full granularity runs 10x the
measured figure). Promotion to 576 follows automatically on
second-platform + wider-envelope confirmation (see below) — no new
proposal needed.

## Rule

1. Within the measured envelope: step **288 in**.
2. Outside the measured envelope: the **36-in default**.
3. An explicit `Builder::StepSize` **always wins** — finer for demanding
   applications, coarser at the caller's own validation risk.

## Measured envelope

Smooth forward solves at ranges at or below 3000 ft, excluding: coarse
steps across multi-point wind-node joints (C5 ran uniform wind only); full
Boatright spin geometry (Litz only); BC bands and custom tables
(single-BC only); incline, Coriolis, transonic/subsonic tails, and the
lapse-scaled inverse tail; inverse solves generally.

## Evidence

- Cap 576 rule-certified with a 1.21x governor margin: C5-uniform
  elevation @3000 ft, 0.00828 MOA vs the 0.01 threshold
  (`test/validation/baselines/step_error_map.json`, `cap_governor`;
  `.superpowers/sdd/exp-e3-report.md`, cap derivation). The governor is
  C5-uniform elevation @3000 ft at every rung.
- Adopted 288 with 4.9x measured margin: same governor cell-range,
  0.00204 MOA vs 0.01 (`step_error_map.json`, C5-uniform 288
  `elevation_moa` @3000; E3 report cap table).
- Coarsening veto CLEAR at 72 in: 102.7x elevation, 587x deflection,
  779.3x time-of-flight (`step_error_map.json`, `veto_72` — 779.3x
  recomputed from raw TOF; E3 report veto verdict).
- No breakage in the measured set: 44/48 series strictly increasing
  (the 4 exceptions are C1 deflection all-zeros, calm case at floor),
  observed order p~2 throughout (E3 report breakage notes).

## Why static instead of adaptive

Both tier-calibration attempts came back degenerate: the two width
constructions spanned 36x/54x in input uncertainty yet everything pinned
at the cap (attempt 1: 8/8 at cap; attempt 2: 7/8 at cap plus one member
1.3x below cap — still pinned at one end). Even lab-grade u = 0.01 MOA
implies step ~204, 22x above the 9-in floor; a floor-threatening
uncertainty would be ~2000x finer than any honest tier, physically
absurd. The solver error curve sits 1-3 orders of magnitude below every
realistic tier's needs, so the step knob has no accuracy leverage and
tiers carry no step information (`docs/specs/STEP_CALIBRATION_EXPERIMENTS.md`,
E3 verdict; `.superpowers/sdd/exp-e3-report.md`, tier table). The honest
outcome is a static cap; the explicit-`StepSize` escape hatch keeps
demanding applications unharmed.

## Second platform: pending

The 576 headroom leans on single-platform (x86_64-linux) evidence.
Two-platform confirmation per `docs/specs/NUMERICAL_VALIDATION_SPEC.md`
§17 baseline-promotion item (b) (plans shorthand §17.3b) is pending via
the CI matrix legs (`CLOSURE_INTEGRATION_PLAN.md` Task 4). Automatic
promotion condition: second-platform confirmation plus wider-envelope
coverage upgrades the default to 576 by amendment with a dated note — no
new experiment proposal required. A ceiling trip on a non-linux leg
widens per §17 only if trivially mechanical; systematic divergence stops
for a new investigation proposal, never blind widening.

## Review history

- E3 report: `.superpowers/sdd/exp-e3-report.md` (all numbers above).
- E3 review: `.superpowers/sdd/review-f580ed1..aa3962e.diff`
  (covers the `67ea823` ladder extension + `aa3962e` error map).
- E3 verdict as agreed: `docs/specs/STEP_CALIBRATION_EXPERIMENTS.md`
  (recorded 2026-09-29, review-agreed).
- The 288 decision with its 1.21x-vs-4.9x reasoning:
  `docs/specs/CLOSURE_INTEGRATION_PLAN.md`, Task 1.
