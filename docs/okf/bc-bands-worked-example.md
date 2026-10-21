---
type: Guide
title: BC Bands Worked Example
description: Bands to a trajectory, with a copy-pasteable example.
tags: [ballistic-coefficient, example, drag]
generated: {by: human:joel, at: 2026-10-09T00:00:00Z}
verified: [{by: human:joel, at: 2026-10-09T00:00:00Z}]
status: stable
---

# BC Bands Worked Example

A concrete example of
[BC/Velocity-Band Transformation](bc-velocity-band-transformation.md), using coefficients
that are also used by the test suite so the result is checkable.

## The measurements

A bullet measured at three velocities, each with a G7-referenced
coefficient that rises as velocity increases (from 0.20 at 2000 fps
to 0.40 at 3000 fps).

```text
 2000 fps  ->  BC 0.20
 2500 fps  ->  BC 0.30
 3000 fps  ->  BC 0.40
```

Read the low coefficient at low speed correctly. A coefficient is a
divisor, so the low-speed measurement means *more* drag than the
reference bullet, not less. The bullet is losing more speed early and
saving it late, which is what a low-drag superbullet does.

## The resulting curve

Below the slowest band the curve is the reference divided by the first
coefficient. Between bands it is the smooth interpolation. Above the
fastest band it is the reference divided by the last coefficient.

@image html bc_transformation.svg "Reference drag curve against the curve transformed from velocity bands" width=700px

The visible features are the ones
[the transformation](bc-velocity-band-transformation.md) promises: the curve is smooth
everywhere, with no kinks at the measured Mach numbers, and outside the
measured range it runs parallel to the reference curve, shifted by the
edge coefficient.

## Building the context

```cpp
#include <array>
#include <cassert>
#include <cstdint>

#include "lob/lob.hpp"

const float kFps[] = {2000.0F, 2500.0F, 3000.0F};
const float kBcs[] = {0.20F, 0.30F, 0.40F};

const lob::Context kCtx =
    lob::Builder()
        .BCAtmosphere(lob::AtmosphereReferenceT::kIcao)
        .BCDragFunction(lob::DragFunctionT::kG7)
        .DiameterInch(0.308)
        .MassGrains(168.0)
        .InitialVelocityFps(3100)
        .ZeroDistanceYds(100.0)
        .BCVelocityBands(kFps, kBcs, 3)
        .Build();
assert(kCtx.error == lob::ErrorT::kNone);

std::array<uint32_t, 4> kRanges = {300U, 600U, 900U, 1200U};
std::array<lob::Output, 4> kOutputs = {};
lob::Solve(kCtx, kRanges, kOutputs.data(), kRanges.size());
```

Note that the reference curve here is G7 rather than G1. The reference is
independent of the transformation, and any of the standard curves can
be transformed.

## What changes relative to a single coefficient

The comparison worth making is against using the middle coefficient for
the whole flight. That produces a proportionally scaled curve, which is
the wrong *shape* rather than merely the wrong size.

Because the bands say the bullet is draggier than the reference at low
speed and slipperier at high speed, the trajectory drops less early and
more late than the single-coefficient baseline. At short range the
bands-based solution is the flatter one; at long range it is the steeper
one. A single coefficient that happens to match at one range is
simultaneously optimistic at another, and the bands remove that
trade-off rather than relocating it.

This is the entire reason to supply bands. Not a better constant, a
better shape.

## Related

- [BC/Velocity-Band Transformation](bc-velocity-band-transformation.md): the method and
  its edge cases.
- [Ballistic Coefficient](ballistic-coefficient.md): what the coefficients mean.
- [Drag Model](drag-model.md): the curve being scaled.