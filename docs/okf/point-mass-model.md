---
type: Concept
title: Point-Mass Model
description: The equations lob integrates, and why it steps by distance instead of time.
tags: [physics, equations-of-motion, integration]
generated: {by: human:joel, at: 2026-10-09T00:00:00Z}
verified: [{by: human:joel, at: 2026-10-09T00:00:00Z}]
status: stable
---

# Point-Mass Model

lob models a projectile as a point mass with three degrees of freedom:
where it is, how fast it is moving, and how long it has been flying. No
body, no orientation, no spin axis in the state vector. That is a severe
simplification of a real bullet, and it is the right one. The
point-mass trajectory is what the published reference solutions solve,
so solving the same problem is what lets lob be checked against them.
The things a point mass cannot represent, chiefly lift and the
aerodynamics of a real body, enter later as measured corrections rather
than as something guessed at mid-trajectory.

## The state, and why distance is the independent variable

The state is a position, a velocity, and a time of flight. What makes
the model interesting is the choice of independent variable: lob steps
forward by down-range distance, not by time.

The reason is control. Stepping by distance means a single step
parameter, expressed in yards of down-range progress, directly
governs both accuracy and cost. A caller intercepting a target a few
hundred yards out and a caller solving a multi-mile shot want different
resolutions, and they get them from the same knob because the knob is
already in the units they care about. Integrating in time would put the
resolution choice in a place where the mapping from step size to
trajectory accuracy depends on the shot itself.

Time of flight rides along inside the integration as a third component
of the state, rather than being estimated afterward from the finished
trajectory. It falls out of the stepping for free that way, and it
stays consistent with the path that was actually taken rather than
being reconstructed from it.

## The equations

In the time domain, the model is the familiar one:

```text
dp/dt = v
dv/dt = -k * |v - w| * (v - w)  -  Coriolis(v)  +  g
d(TOF)/dt = 1
```

where `w` is the wind vector, `g` is gravity, and `k` collects the
Mach-dependent drag coefficient with the local air density. Drag acts
on the velocity *relative to the air*, not relative to the ground,
which is the entire reason wind enters the drag term and not as a
separate nudge.

Because lob steps by down-range distance, everything divides through by
the down-range velocity component:

```text
dp/dx = v / vx
dv/dx = dv/dt / vx
```

and the time of flight, which accumulates in the time domain, becomes
the reciprocal of down-range speed per unit distance. One expression
covers all three state components, evaluated at whatever the current
down-range position is.

Gravity and the rotation of the Earth are handled as adjustments to
that frame rather than as separate physics. Gravity is rotated into the
trajectory's own axes by the range inclination, so a shot downhill and
the same shot uphill fall in the direction the shooter is actually
facing. The Coriolis term is the standard coupling of the three axes
for a rotating observer:

```text
dvx/dt -= vy * 2w cos(lat) sin(az) + vz * 2w sin(lat)
dvy/dt += vx * 2w cos(lat) sin(az) + vz * 2w cos(lat) cos(az)
dvz/dt += vx * 2w sin(lat)         - vy * 2w cos(lat) cos(az)
```

Only the latitude and azimuth of the shot are involved, and the
coefficients collapse to three scalars that get computed once when the
context is built rather than on every step.

Where the ballistic coefficient is folded in depends on how the drag
curve was supplied. A standard curve carries it, scaled into the curve
itself, so the density scaling applied each step stays a pure
function of the air. A custom table or a velocity-band curve already
embeds it. Either way the per-step scaling only has to answer "how
thick is the air right here", which keeps the two concerns from
competing for the same coefficient.

## When the projectile stops advancing

A step-by-distance integrator has a degenerate case: if the projectile
is no longer making down-range progress, there is no distance left to
step through and dividing by its down-range velocity is undefined.
That happens when a lofted shot has passed its apex and is falling
nearly straight down.

The model handles it by declaring the trajectory finished rather than
producing nonsense. When down-range velocity is nonpositive and cannot
advance downrange, the derivative contribution is zeroed and the
component is held at zero; distance-based integration stops rather than
continuing the descent. The integration terminates through its ordinary
"cannot reach the target" path, so a shot that will not arrive is
reported as unreachable rather than as an angle that misses.

## Two step variants

The equations above admit two ways to treat the air, and lob ships both
because they trade against each other.

The **fast variant** assumes the air is the air at the firing site, for
the whole flight. A bullet that stays in roughly one atmosphere sees
almost no error from this, and the per-step cost drops to almost
nothing because nothing about the air needs recomputing. This is the
default for forward solves and for finding a zero.

The **lapse variant** recomputes density and speed of sound each step,
projecting the projectile's climb onto the standard lapse rate, so a
long shot that gains thousands of feet of altitude flies through
correctly thinning air. It costs real work per step.

Inverse solving picks between them per range, on the basis of how much
climb that particular shot needs. Shots that stay low take the cheap
path; the long ones pay. The two are not guaranteed to agree when
altitude-dependent drag or speed-of-sound corrections are nonzero, so
the choice is about how hard the shot is and the tradeoff being made,
not a correctness difference in all conditions.

## Assumptions

Stated plainly, because they bound what the solutions mean:

- **Point mass.** No lift. The spin-related effects that a point mass
  cannot represent are applied afterward as corrections: the
  aerodynamic jump enters the launch angle, and spin drift enters the
  lateral deflection.
- **Flat Earth** over the horizon a trajectory actually covers.
  Altitude-dependent air density comes from the lapse variant, not
  from a curved gravity field.
- **Uniform wind.** One vector for the whole flight. Wind shear along
  the trajectory is not part of the point-mass model.

## Related

- [Drag Model](drag-model.md): the curve that supplies the drag term, and
  how the ballistic coefficient is folded into it.
- [Heun Integration](heun-integration.md): the method that advances this
  system along its trajectory.
- [Angle Solver, in Depth](angle-solver.md): fires candidate
  trajectories through this model and reads back the miss.