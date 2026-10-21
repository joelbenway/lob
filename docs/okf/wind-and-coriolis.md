---
type: Concept
title: Wind and Coriolis
description: The two corrections that act on velocity rather than on geometry.
tags: [wind, coriolis, crosswind, shear]
generated: {by: human:joel, at: 2026-10-09T00:00:00Z}
verified: [{by: human:joel, at: 2026-10-09T00:00:00Z}]
status: stable
---

# Wind and Coriolis

Wind and the Earth's rotation are the two corrections lob applies that
act on the projectile's velocity rather than on the geometry of its
path. Neither one changes the drag curve or the atmosphere. Both are
resolved once when the solver context is built and then applied at every
integration step, which is why they are cheap and why they are grouped
together here.

## Wind

Wind enters the equations in only two places, and understanding both of
them explains most of what wind does to a solution.

**Drag acts on airspeed, not groundspeed.** The trajectory computes its
drag from the projectile's velocity relative to the moving air. That
single fact accounts for the entire effect of a headwind or tailwind: a
headwind increases the relative speed the bullet sees and therefore the
drag it suffers, and a tailwind decreases both. Neither is a force
pushed on the bullet; both fall out of computing drag against the air
rather than the ground.

**Crosswind affects drag through relative airspeed, and spin adds jump.**
Lateral wind affects the trajectory through relative-air drag even
without spin corrections (drag acts on velocity relative to the moving
air). Spin effects add aerodynamic jump pitched at the muzzle, which is
covered in [Spin Drift and Aerodynamic Jump](spin-drift-and-aerodynamic-jump.md).
That correction reads only the lateral component of the wind at the muzzle,
and deliberately so: the lateral direction is perpendicular to the plane of
fire, so tilting the shot for an inclined range does not disturb it.

The heading convention is the one shooters already use on a clock face.
Twelve o'clock is a tailwind, six o'clock a headwind, three o'clock a
pure crosswind from the right.

## Wind profiles

A single wind vector is the common case and is the default, but wind
does not hold still over a long trajectory, and lob can represent that.

A **profile** is a list of wind points along the trajectory, each with a
magnitude, a heading, and the height at which it was measured. A
weather reading taken at ground level does not describe the air a
long-range shot crosses at altitude, so each point's height matters.

Heights are normalized once, when the context is built, against a fixed
reference height using the standard power-law relation for wind speed
versus height over open terrain. Building the profile up front means the
per-step query stays a simple interpolation rather than a computation.

At solve time the profile does two things. It interpolates between the
points by down-range position, so a wind that changes along the shot
changes where the bullet is. And it scales with altitude according to a
shear exponent, which models wind increasing with height. The exponent
defaults to no shear, so a caller who supplies a profile and nothing
else gets a constant wind that varies by range only; shear is something
you opt into.

Inclined fire has one subtlety. Height is measured against the plane
containing the trajectory, not against flat ground, so a downhill shot
is not quietly reading a different part of the wind profile than an
uphill one.

A profile with a single point is exactly equivalent to supplying a
uniform wind, down to the last bit, so the simple case is a genuine
special case of the general one and not a separate code path with its
own rounding.

## Coriolis

The Earth's rotation deflects a moving body. Over the length of a
trajectory the deflection is tiny, but it is systematic: a long shot
aimed north drifts consistently in one direction, and lob accounts for
it.

The correction is standard, derived from working in a rotating
reference frame. What lob does with it is unremarkable and
deliberately so. The shooter's latitude and azimuth determine three
scalars that couple the three velocity components to one another. Those
scalars are computed once when the context is built, and each step
adds the corresponding acceleration to the velocity, after drag and
before gravity.

The correction requires both a latitude and an azimuth. Supplying only
one is not an error, it simply produces no correction, because the
effect is a function of the pair. A shooter in Wisconsin who gives no
azimuth gets no Coriolis, which is the correct answer to the question
they did not ask.

lob does not model the related effect of a plumb line tilting away from
true vertical with altitude. Where that would matter, its effect is
folded into the reported deflection and elevation rather than being a
separate term in the trajectory.

## Limitations

- There is no vertical wind input. Updrafts and downdrafts are not
  represented; wind is horizontal.
- Altitude and range variation in wind are opt-in. Supplying a profile
  gets you variation by range, and supplying a shear exponent gets you
  variation by altitude, but neither is inferred.
- Crosswind reaches the trajectory through the spin corrections only.
  A caller who omits the geometry those models need gets no jump from a
  crosswind, even though the crosswind is present.

## Related

- [Point-Mass Model](point-mass-model.md): the trajectory these accelerations
  are applied to.
- [Spin Drift and Aerodynamic Jump](spin-drift-and-aerodynamic-jump.md): where the lateral wind
  component is consumed.
- [Atmospheric Model](atmospheric-model.md): the wind profile's altitude
  scaling is a separate matter from the air's density.