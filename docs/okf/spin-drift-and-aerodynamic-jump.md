---
type: Concept
title: Spin Drift and Aerodynamic Jump
description: Two spin-induced corrections, each with an empirical and a geometric model.
tags: [spin, stability, crosswind, corrections]
generated: {by: human:joel, at: 2026-10-09T00:00:00Z}
verified: [{by: human:joel, at: 2026-10-09T00:00:00Z}]
status: stable
---

# Spin Drift and Aerodynamic Jump

A gyroscopically stabilized bullet flying through a crosswind does not
go where it is pointed. The crosswind pushes on the spinning body, the
aerodynamic forces at the nose couple with the spin, and the projectile
leans. Leaning is invisible to a point-mass model, which has no
orientation to lean. So lob models it as two corrections applied around
the point-mass trajectory: spin drift, which pushes the impact sideways,
and aerodynamic jump, which tilts it up or down at the muzzle.

## The two effects

**Spin drift** is the lateral one. A spinning projectile precesses
rather than simply pointing where it was aimed, and the yaw of repose
that results launches it to one side. The effect grows with how hard
the shot is elevated and how fast the bullet is spinning, because both
increase the precession. Drift is therefore mostly a long-range and
elevated-shot phenomenon: it is small enough to ignore up close and
grows to matter exactly where precision shots are contested.

**Aerodynamic jump** is the vertical one, and it happens before the
bullet has flown anywhere. Crosswind on a spinning body pitches it at
the muzzle, which tilts the entire trajectory. The shooter has to hold
off by that angle or hit high or low. Jump is instantaneous from the
shooter's point of view: it is a muzzle event, not something that
accumulates along the flight.

Both effects share a direction of consequence. Crosswind drives both,
stability and spin rate drive both, and both grow more significant as
the shot gets longer. That is why they live together in one model
rather than being scattered across the solver.

## Two fidelity levels

Each effect is available at two levels of fidelity, and the choice
between them is made automatically from what the caller supplied.

**The empirical model** is closed-form arithmetic fitted to observed
behavior, from Brian Litz's *Applied Ballistics for Long Range
Shooting*. It needs very little: a stability factor and, for drift, the
time of flight. It is cheap, it is always available when the basics are
known, and it is the level most callers will get. Being empirical, it is
a description of what bullets were observed to do rather than a
derivation from why they do it.

**The geometric model** derives the effects from the actual mechanics
of a spinning gyroscope in a flow. Precession and nutation are
computed from the projectile's real moments of inertia, which means
the bullet's shape has to be known: its diameter and length, the nose
profile, the boat tail, the meplat and base diameters. On top of that
it needs mass, twist rate, launch velocity, speed of sound, and the
crosswind at the muzzle. Doing this properly means real computation,
including finding the projectile's volume by integrating over its nose
profile and running a short supersonic integration to get how long the
spin phase lasts before the bullet transits the transonic region.

The geometric model is the higher-fidelity answer when the inputs
justify it, and it is the difference between a shooter who described
their bullet and one who only guessed at it.

## Selection is automatic

lob does not ask which model you want. It builds the geometric model
if the geometry is complete enough to support it, falls back to the
empirical model when it is not, and uses no correction at all when
neither can be computed. Both effects are resolved once, when the
solver context is built, and the results are reused by every solve
against that context.

This is deliberate, and it is the same design philosophy the
[Atmospheric Model](atmospheric-model.md) follows with weather data. A caller
who describes their bullet fully gets the geometric answer. A caller
who knows only caliber and twist rate still gets an estimate. A caller
who knows almost nothing gets a trajectory with no spin correction,
rather than an error demanding a full projectile description. The cost
of a partial answer is never pushed back onto the caller.

## Where each one applies

**Drift is a post-solve correction.** It adjusts the lateral deflection
that the trajectory produced, in proportion to the shot's drop below
the bore line, which is how the published model specifies the effect.

**Jump enters the launch angle.** It is added to the angle before the
initial velocity is built, which means the trajectory that gets flown
is the trajectory that accounts for it. The important consequence is
for the [angle solver](angle-solver.md): zero-finding and
inverse solving search over the mechanical launch angle and add jump
themselves, so the zero a rifle is zeroed with and the adjustments
computed against that zero both carry the same jump. If jump were
applied by each caller instead, the two paths could disagree by exactly
the amount jump contributes.

## Limitations

- The empirical model is fitted to data, not derived. It is not a
  six-degree-of-freedom simulation and should not be expected to
  reproduce individual shots.
- The geometric model draws on published work for its intermediate
  quantities, and is backed by the test suite rather than claimed to
  exceed it.
- Jump is a single scalar added to the launch angle. It does not
  model the rolling moment evolving over the flight, only the pitch it
  imparts at the muzzle.
- When inputs are missing the correction degrades to nothing. That is
  intentional, but it means a caller who omits geometry gets no spin
  effects rather than a rough estimate of them.

## Related

- [Point-Mass Model](point-mass-model.md): the trajectory these corrections
  are applied to, and why they cannot be part of it.
- [Angle Solver, in Depth](angle-solver.md): where the jump is
  added, and why it has to be there.