---
type: Concept
title: Atmospheric Model
description: How lob turns imperfect weather data into density and speed of sound.
tags: [atmosphere, isa, environment]
generated: {by: human:joel, at: 2026-10-09T00:00:00Z}
verified: [{by: human:joel, at: 2026-10-09T00:00:00Z}]
status: stable
---

# Atmospheric Model

The atmosphere hands the integrator exactly two numbers. One is how
dense the air is, which scales the drag the projectile feels. The other
is how fast sound travels through it, which defines the Mach number
that the drag curve is indexed by. Everything in this chapter exists to
produce those two numbers correctly from whatever a shooter actually
knows about the weather.

## The standard atmosphere as a backbone

The backbone is the International Standard Atmosphere, a reference
model of how the air behaves: temperature falls steadily as altitude
climbs, pressure follows from the temperature, and density follows from
pressure and temperature together. It is a definition rather than a
measurement, which is exactly its value here. It means lob has a
defensible atmosphere even when the shooter supplies no weather data at
all, and it means that any real measurement can be treated as a
correction away from a known starting point.

The model has one structural wrinkle worth knowing. The steady
temperature fall applies only up to the tropopause. Above it the
atmosphere stops cooling with height and becomes isothermal, so pressure
there follows an exponential law instead. Without the branch, a very
high shot would be handed a temperature the atmosphere cannot have.
lob branches on the tropopause rather than extrapolating the linear
lapse past it.

## Making the best of imperfect inputs

Weather data arrives wrong in ways that are specific and fixable, and
each correction handles one of them.

A pressure reading is almost never taken at the shooter's altitude. It
comes from a weather station, a barometer indoors, or a phone app, each
reporting the pressure where *it* is. lob shifts the reading
barometrically to the firing site, so a pressure taken a thousand feet
below the shooter describes the air the bullet actually leaves. If you
supply that barometer altitude, the shift is exact; if you do not, lob
assumes the barometer was at the firing site.

Temperature has the same problem with the same fix. A reading comes
from wherever the thermometer sat, and lob lapses it to the firing site.

Then there is what was not measured at all. No temperature means the
standard sea-level value, lapsed down to the firing site. No pressure
means the standard sea-level value, propagated up to the firing-site
altitude barometrically. No firing-site altitude means sea level. No
humidity means dry air.

The design intent is worth stating plainly, because it is unusual:
lob never refuses to produce a solution for lack of environmental data.
Every missing input has a standard-atmosphere substitute, and every
present-but-wrong input gets corrected into place. The alternative, an
error demanding a fuller weather report, pushes the cost of a partial
answer onto the caller, and the whole design is arranged to avoid
that. The same philosophy that lets a builder take a bare ballistic
coefficient also lets the atmosphere be half-specified.

## Humidity

Humidity does two things to the air, in opposite directions. It makes
the air slightly less dense than dry air at the same temperature and
pressure, and it makes the air slightly faster at carrying sound. Drag
and Mach both shift as a result, so both corrections apply.

Both corrections are driven by the saturation vapor pressure at the
measured temperature, which is what says how much water vapor the air
can be holding. Getting that quantity normally means a lookup table of
measurements. lob uses a published closed-form approximation instead,
chosen specifically so that a library which otherwise carries no data
files does not need to ship one.

The humidity corrections themselves are linear, valid across the range
of humidity where the effect matters at all. Beyond the extremes the
model is not trying to be right; a shooter measuring near-saturation
conditions in absolute humidity is outside what this correction
represents.

## From atmosphere to solver

Density and speed of sound are computed once, when the context is
built, and then carried by the context for the rest of the solve.

Density does not enter the solver as density. It is converted into the
drag coefficient that scales the drag curve, which is why the drag
model never has to know about air pressure, temperature, or humidity.
Speed of sound divides projectile speed to produce the Mach number the
drag curve is evaluated at.

For the per-step variant described in the point-mass model, one further
quantity is precomputed: a lapse coefficient. It lets the solver
project how far the projectile has climbed onto the standard lapse
rate, and derive from that how much thinner and faster the air has
become at the projectile's current altitude, without redoing the
atmospheric model at every step.

## Limitations

- Temperature follows the standard linear lapse rather than a more
  elaborate exponential variant that also appears in the literature.
  The linear form is what the atmosphere model implements; the variant
  is retained in the source for reference, not used.
- The per-step density correction is gated by the inverse solver rather
  than always on, because most shots do not climb far enough for it to
  matter and it is not free. See
  [Point-Mass Model](point-mass-model.md) for how that choice is made.

## Related

- [Point-Mass Model](point-mass-model.md): the integrator that consumes
  density and speed of sound.
- [Drag Model](drag-model.md): what the Mach number produced here indexes.