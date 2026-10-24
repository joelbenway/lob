---
type: Guide
title: Solver Input State Machine
description: Which solver questions to ask, when to skip them, and what each answer unlocks.
tags: [guide, inputs, ux, solver]
generated: {by: agent:opencode, at: 2026-10-22T00:00:00Z}
status: draft
---

# Solver Input State Machine

An interactive solver has two jobs: collect enough information to build
a context, and avoid asking questions whose answers cannot change the
result. The builder is deliberately permissive, so a flat form over its
whole surface becomes a wall of irrelevant boxes. This page defines the
questions, visibility rules, defaults, and skip behavior shared by the
web solver example and lobber.

The machine is a set of gates, not a fixed list. Each gate has a trigger
(the user chooses the capability), required inputs, per-field visibility
rules, and a skip behavior. Skipping a gate means omitting its builder
calls entirely, not filling them with `NaN` or zero: the builder's
documented defaults apply. Changing an upstream answer clears answers
that depended on it.

The machine describes questions, not presentation. A wizard can ask one
gate at a time; a progressive form can reveal the same gates in place.
The order below is the recommended reading order.

The web solver example exposes every gate in this document, including
the expert native-coefficient source. lobber can render the same gates
as prompts and does not need to show every field at once.

## Core path

Every build needs a drag source, a muzzle velocity, and a zero. The
application also needs ranges to solve.

| Step | Question | Required | Default |
| --- | --- | --- | --- |
| 1 | Which drag source do you have? | Yes | Standard BC |
| 2 | Muzzle velocity | Yes | None |
| 3 | How is the rifle zeroed? | Yes | Zero distance, 100 yd (UI default) |
| 4 | Which ranges? | Yes | 0 to 1000 yd by 100 yd |

The builder itself has no default for velocity or zero. Those defaults
belong to the application. The drag source default (standard BC with G1)
comes from the builder.

## Drag source gate

This gate is mutually exclusive. Ask exactly one source, then only the
fields that source uses. The last drag-setting call wins inside the
builder, so the implementation must not leave a previous source's call
in the chain.

| Source | Ask | Do not ask | Notes |
| --- | --- | --- | --- |
| Single BC | Ballistic coefficient; drag function; BC atmosphere | Bands, table, coefficients | Drag function defaults to G1; BC atmosphere defaults to Army Standard Metro |
| BC velocity bands | Two to sixteen velocity/BC rows; drag function; BC atmosphere | Single BC | Velocities must be positive and strictly increasing; BCs positive; highest velocity below Mach 5 |
| Custom Mach/Cd table | Two or more Mach/Cd rows | Single BC, drag function, BC atmosphere | Mach non-negative and strictly increasing; Cd non-negative; the table replaces the standard curve |
| Native coefficients | The 60 drag-curve coefficients | Single BC, drag function, BC atmosphere | Expert gate; coefficients must all be finite |

The drag function selects the base curve for the standard and band
sources. The BC atmosphere reference converts a coefficient measured
against Army Standard Metro into the ICAO reference. Neither applies to
a custom table or native coefficients, which replace the curve outright.

## Projectile detail gate

Ask how much the user knows about the bullet, not for every geometry
field at once. Each tier includes the previous one.

| Tier | Ask | Unlocks |
| --- | --- | --- |
| BC only | Nothing | Drag, gravity, wind, atmosphere; energy is not meaningful |
| Mass | Mass in grains | Muzzle energy and the minimum-energy stop condition |
| Stability | Mass, diameter, length, twist | Miller stability factor and the Litz aerodynamic jump when a crosswind exists |
| Full geometry | Stability fields plus nose length, meplat diameter, base diameter, tail length, and ogive Rt/R | Boatright spin drift and the higher-fidelity crosswind aerodynamic jump |

Full geometry is only meaningful after the stability tier. If the full
set is incomplete, the builder still produces a context; it falls back
to the simpler aerodynamic jump and leaves the higher-fidelity spin
drift unavailable. The UI should say which model is active rather than
silently pretending the missing data was used.

The minimum-energy stop condition is visible only when mass is present;
energy cannot be derived from a velocity alone.

## Zero gate

A zero is either a distance or an angle, never both.

| Question | Visible when | Default |
| --- | --- | --- |
| Zero source: distance or angle | Always | Distance |
| Zero distance | Source is distance | 100 yd (UI default) |
| Zero impact height | Source is distance | 0 in |
| Zero angle | Source is angle | None |
| Sight height | Always | 1.5 in |

Sight height still affects the reported elevation when a zero angle is
supplied; it is not part of the zero solution. Switching zero sources
must clear the other source's fields so a stale value cannot be sent.

## Atmosphere gate

Ask where the weather numbers came from. The builder can derive missing
pressure and temperature from altitude, or accept measurements taken at
a different place.

| Source | Ask | Visible when | Default |
| --- | --- | --- | --- |
| Standard atmosphere | Nothing | Always an option | Sea-level ISA |
| Altitude | Firing-site altitude; humidity | Source is altitude | Altitude required for this source; humidity optional |
| Measured | Pressure, temperature, humidity, firing-site altitude | Source is measured | Each field optional; missing values fall back to ISA |
| Measurements away from the firing site | Barometer altitude; thermometer altitude | Source is measured, the corresponding measurement was supplied, and the user says it was taken away from the site | Required when away; hidden otherwise |

Pressure and temperature are independent: the user may supply one and
let the other derive from altitude. The firing-site altitude also drives
the dynamic density lapse along the trajectory, so it matters even when
pressure and temperature are supplied. Barometer and thermometer
altitudes only adjust their corresponding measurement.

## Wind gate

This gate is mutually exclusive: none, uniform, or profile. Do not send
uniform fields after a profile or the uniform mode wins.

| Source | Ask | Notes |
| --- | --- | --- |
| None | Nothing | Windage and crosswind effects are zero |
| Uniform | Wind speed; heading | Ask heading only when speed is nonzero; clock angle 1 through 12 or degrees |
| Profile | Range, heading, speed, and optional height for each point; shear exponent | First range must be 0; ranges strictly increasing; at most eight points |

Uniform wind heading is a clock angle by convention in lobber, but the
builder also accepts degrees. Pick one presentation and do not mix them.

The shear exponent is only visible for a profile that supplies heights.
Zero disables height scaling exactly, which is the builder default. A
muzzle crosswind is what enables aerodynamic jump; a pure headwind or
tailwind does not.

## Shot geometry and Earth gate

These are independent questions.

| Question | Visible when | Default |
| --- | --- | --- |
| Range angle (up/down slope) | The user opts into shot geometry | 0 degrees |
| Include Coriolis? | Always an option | No |
| Latitude | Coriolis is on | None |
| Azimuth | Coriolis is on | None |

Coriolis requires both latitude and azimuth. Supplying only one leaves
the effect disabled; the UI should ask for the pair, not one field at a
time. Range angle changes gravity and the wind frame; it is not part of
Coriolis.

## Solver controls gate

These controls change how the solve runs, not what is being solved.
Keep them behind an advanced gate and default them off.

| Question | Visible when | Default |
| --- | --- | --- |
| Maximum time | Advanced controls are on | No limit |
| Minimum speed | Advanced controls are on | No limit |
| Minimum energy | Advanced controls are on and mass is present | No limit |
| Step size | Advanced controls are on | 1 yd (builder zero) |

When both minimum speed and minimum energy are set, the solver uses the
larger implied speed. A stop condition can make the result shorter than
the requested range list; report that explicitly rather than presenting
a partial table as complete.

## Capability ladder

| Capability | Requires | What it changes |
| --- | --- | --- |
| Basic trajectory | Drag source, velocity, zero | Drag, gravity, and standard-atmosphere defaults |
| Muzzle energy | Mass | Reported energy and the minimum-energy stop |
| Stability | Mass, diameter, length, twist | Stability factor and Litz aerodynamic jump |
| Full spin model | Stability plus full geometry | Boatright spin drift and the higher-fidelity crosswind jump |
| Dynamic density | Firing-site altitude | Density and speed-of-sound change along the trajectory |
| Measured atmosphere | Pressure, temperature, or humidity | Replaces the ISA defaults; station altitudes adjust measurements to the firing site |
| Windage | Uniform or profile wind | Lateral deflection and crosswind aerodynamic jump |
| Coriolis | Latitude and azimuth | Earth-rotation correction |
| Stop conditions | Maximum time, minimum speed, or minimum energy | Earlier termination and fewer output rows |
| Fine integration | Step size | Accuracy and cost trade-off |

## Defaults and omissions

| Input | Omitted means |
| --- | --- |
| Drag function | G1 |
| BC atmosphere reference | Army Standard Metro |
| Sight height | 1.5 in |
| Zero impact height | 0 in |
| Weather | Sea-level ISA; humidity at the ISA default |
| Wind | Calm, uniform mode |
| Wind shear | 0 (disabled exactly) |
| Range angle | 0 degrees |
| Coriolis | Off |
| Maximum time | No limit |
| Minimum speed | 0 (no limit) |
| Minimum energy | No limit |
| Step size | 1 yd |
| Mass | Energy is not meaningful |
| Projectile geometry | No stability or spin model |

## Presentation rules

- Ask gates in the order above. A gate that is off hides all of its
  fields; a field whose prerequisite is missing is hidden, not disabled.
- Do not send a builder call for a hidden field. Omission is the
  mechanism that selects the default.
- When an upstream answer changes, clear the answers that depended on
  it before showing the new path. This is most important for drag source,
  zero source, and wind source.
- Show a review before solving: supplied values versus defaults, and
  which model tiers are active. This is where the user discovers that a
  blank geometry field means no spin drift, not zero spin drift.
- Validate only what the builder cannot express well, such as required
  fields and mutually exclusive choices. Let the builder report its own
  error codes for ranges and cross-field rules. It reports one error at
  a time.
- A wizard should preserve answers when moving Back, but still apply the
  clearing rule when a choice actually changes.

## Related

- [Builder Pattern](builder-pattern.md): the mutating half of the
  configuration lifecycle.
- [Ballistic Coefficient](ballistic-coefficient.md): single and
  velocity-dependent coefficients.
- [BC/Velocity-Band Transformation](bc-velocity-band-transformation.md):
  how a band table becomes one curve.
- [Atmospheric Model](atmospheric-model.md): what pressure, temperature,
  humidity, and altitude do.
- [Wind and Coriolis](wind-and-coriolis.md): the corrections these gates
  control.
- [Integration Accuracy and Step Size](integration-accuracy.md): the
  cost of the step-size control.
- [Inverse Solving](inverse-solving.md): the output mode that can share
  the same context.
