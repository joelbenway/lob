---
type: Concept
title: Validation
description: How lob's correctness is established, and what is still unproven.
tags: [validation, accuracy, testing, evidence]
generated: {by: human:joel, at: 2026-10-09T00:00:00Z}
verified: [{by: human:joel, at: 2026-10-09T00:00:00Z}]
status: stable
---

# Validation

lob is numerical software, and numerical software cannot be correct by
inspection. What it can do is be honest about what has been measured,
what has not, and where the difference matters. This page is that
inventory.

## The rule everything else follows from

No accuracy claim is made that is not backed by a test or a
measurement, and the wording of a claim mirrors the tolerance it was
measured against rather than rounding up to something more impressive.

That rule has consequences that run through everything below. There is
no single accuracy figure for lob, because there is no single answer to
give. There is a set of claims, each scoped to the case it was measured
on.

## Reference data

Accuracy is only meaningful against something external, and lob uses
three sources.

The drag tables come from Ballistic Research Laboratory work, published
through JBM. The spline representation is checked against the full
tables rather than against a sample of them, with a measured worst-case
budget across the whole Mach range and every standard curve.

Reference trajectories cover the atmosphere rather than one nominal
case. A standard-atmosphere baseline and an altitude case are joined by
three that each break the model in a different way: a hot, low-pressure
day with thin air, a barometer reading taken somewhere other than the
shooter's altitude, and a case supplying pressure, temperature and
humidity together. A weather-station case exercises the corrections a
real reading needs. Each carries its own expected values, transcribed out
of the environment tests along with a record of where they came from.
Each case bounds velocity, energy, angle and time of flight over a
range out to a thousand yards.

The spin corrections are checked against the published papers they were
taken from, which is the only check available for them.

## Convergence and error floors

A solver that converges to the wrong answer is worse than one that does
not converge, so refinement is tested as a property rather than assumed.

The core test is that halving the step must reduce error, monotonically,
on the same trajectory. That is a real claim about the code and it is
enforced in continuous integration. What it does not give you is an
absolute accuracy, which is why the ceilings beside it are curated per
case rather than derived.

Those per-case values are called error floors, and treating them per
envelope cell rather than as one library-wide number is the important
design decision. A trajectory through a hot, high, fast regime and a
trajectory through a cool, sea-level, slow one are different problems,
and a single accuracy figure covering both would be a claim about
neither. The floor is per cell and per output, and a caller working in
one regime can read the number that applies to them.

## Sensitivity

Sensitivity analysis answers a different question: not how accurate is
the answer, but which inputs the answer is most exposed to. Perturbing
one input at a time and ranking the effect tells a caller where their
own measurement effort should go, and it is measured rather than assumed.

Some of this feeds a further step. A budget combines the sensitivities,
the numerical floors, and the residuals against reference data into a
single uncertainty per cell. The machinery for that combination exists
and is tested.

What the machinery cannot yet do is produce a number anyone should quote,
because an uncertainty budget is only as good as the uncertainties going
into it, and the input uncertainties, muzzle velocity scatter, sensor
error, the input tables themselves, have not been supplied with evidence
attached. The schema reserves the fields and the combiner is proven, but
the honest state is that the budget is not yet a budget. No combined
uncertainty is claimed anywhere.

## Monte Carlo

Sampling input distributions through the unchanged deterministic solver
propagates input uncertainty to output uncertainty, and runs offline and
reproducibly from a fixed seed.

Two things about its scope. It lives in a desktop tool rather than in the
library, because sampling is not something an embedded caller or a real
time caller asked for and should not pay for. And its distributions are
synthetic for the same reason the budget has no numbers: the shape is
known, the data behind the shape is not.

## Signal versus noise

The hardest question in any of this is whether a change in the answer is
real or is the solver's own resolution. Comparing two results is only
meaningful relative to how precisely the solver can produce either one.

So observed changes are divided by the solver's own measured resolution
and reported in bands: clearly distinguishable, marginal, or
indistinguishable. The verdict is deliberately resolution-relative and is
not a claim about statistical significance of prediction error. That
distinction matters, because a solver can be entirely reproducible and
still be unable to resolve an effect, and saying so is more useful than
a confident number.

## Coverage and gating

Continuous integration runs linting, sanitizers, coverage, and the test
suite across several operating systems and both shared and static
builds, plus a benchmark that tracks instruction counts so a change to
the hot path shows up as a regression rather than as a surprise later.

The test suite mirrors the source layout, so an area that gains a
production file is expected to gain matching tests.

## What is still unproven

This is the section that matters most, and it is not exhaustive because
the list is long and it grows honestly.

**There is no field data.** Accuracy is anchored on published tables and
on reference trajectories, some of them borrowed. Nobody has fired lob's
outputs against measured impacts from an actual range session.

**Long range is anchored on standard conditions.** Accuracy at extreme
range currently rests on the atmosphere the builder assumed, not on
conditions observed at that range.

**Input uncertainties are absent.** Every number in the uncertainty
budget is waiting on a human to supply the evidence behind an input.

**Every claim is marked provisional.** The curated claim rows carry a
pending sign-off, and the long-range and field cells are explicitly
unclaimed rather than provisionally claimed.

**Performance is tracked, not gated.** Instruction counts are monitored
for regression, but there is no performance budget that fails a build.

That list is the accurate summary of what lob knows about itself. A
caller deciding whether to trust it for a given job should read it, and
decide against their own tolerance.