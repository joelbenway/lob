---
type: Guide
title: Getting Started
description: Building lob and getting your first solution.
tags: [guide, getting-started, quickstart]
generated: {by: human:joel, at: 2026-10-09T00:00:00Z}
verified: [{by: human:joel, at: 2026-10-09T00:00:00Z}]
status: draft
---

# Getting Started

This is the shortest path from nothing to a trajectory. Build
instructions for the library itself live in the repository's build
document; this page covers getting a number out of it.

## The shape of every call

Every lob call has the same three moves, whatever you are asking for.

Configure a builder, build a context from it, then solve against that
context. The builder is the only mutable thing and it is mutable only
during configuration. Once you have a context, it is fixed, and you can
solve against it as many times as you like.

```cpp
#include "lob/lob.hpp"

const lob::Context kContext = lob::Builder()
                                  .BallisticCoefficientPsi(0.425)
                                  .InitialVelocityFps(2700)
                                  .ZeroDistanceYds(100.0)
                                  .Build();
```

That is already a complete, if minimal, problem definition: a bullet
with this ballistic coefficient, leaving the muzzle at this velocity,
zeroed at this distance. Everything you leave out takes a documented
default, so this builds successfully.

## Your first forward solve

A forward solve answers "where does this land". You give it ranges, it
gives you back a result for each.

```cpp
#include <array>
#include <cstdint>
#include <iostream>

#include "lob/lob.hpp"

const lob::Context kContext = lob::Builder()
                                  .BallisticCoefficientPsi(0.425)
                                  .InitialVelocityFps(2700)
                                  .ZeroDistanceYds(100.0)
                                  .Build();

int main() {
  if (kContext.error != lob::ErrorT::kNone) {
    return 1;  // configuration was rejected; see the error code
  }

  const std::array<uint32_t, 3> kRanges = {300U, 600U, 900U};
  std::array<lob::Output, 3> kResults = {};

  const size_t solved = lob::Solve(kContext, kRanges, &kResults);
  for (size_t i = 0; i < solved; i++) {
    std::cout << "At " << kResults[i].range << " ft: "
              << kResults[i].elevation << " inches\n";
  }
}
```

The array overloads exist so that the sizes cannot disagree with each
other. Raw pointer forms are available and take an explicit count.

Always check the context's error before solving, and always use the
count that comes back rather than assuming every range was solved.

## Now the inverse

An inverse solve answers the question a shooter actually has: what do I
hold or wind by at this range. The inputs are the same ranges; the
outputs are now adjustments rather than raw drop and drift.

```cpp
const std::array<uint32_t, 3> kRanges = {300U, 600U, 900U};
std::array<lob::Output, 3> kResults = {};

const size_t solved = lob::SolveInverse(kContext, kRanges, &kResults);
```

Same context, same ranges, different question. The elevation that came
back in inches is now an adjustment in minutes of arc measured from the
zero. This is the cheap answer: one forward solve plus the closed-form
geometric correction, no iteration.

For the authoritative answer — a lapse-scaled iterative refinement of
every range, for extreme shots and published tables — call
`lob::SolveInverseELR` instead. It starts from the cheap answer above
and iterates from there.

Ranges must be in increasing order. A table that is not is rejected
rather than quietly sorted, so that a mistake shows up as a mistake.

## Adding real data

The minimal example is deliberately sparse. A fuller description comes
from describing the bullet properly, because the more you say, the more
of lob's models can engage.

Bullet geometry, such as its diameter, length, nose profile, meplat and
base, unlocks the higher-fidelity spin corrections and the velocity-band
drag transformation. A muzzle crosswind is needed before aerodynamic
jump can be computed at all. Weather conditions replace the standard
atmosphere defaults, and every one of them can be specified
independently, with the rest defaulted.

If you have a manufacturer's velocity-dependent coefficients, supply
them as bands rather than picking one. That is the difference between a
proportionally scaled standard curve and a curve shaped like your bullet.

## Units

Builder setters name their unit. There is no setter that takes a bare
double, so the compiler will stop you from passing feet where inches
were meant, and from passing a mass where a length was meant.

Solutions come back in American customary units, the convention lob is
built around. The C layer exposes unit conversions for callers who need
to work in something else.

## Where to go next

For what the numbers mean and how they are computed, the ballistic model
chapter is the reference. For why the library is shaped this way, the
design decisions chapter. For what has and has not been verified, and
the list of things that are still unproven, read the validation page
before relying on a result for anything that matters.