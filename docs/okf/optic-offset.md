---
type: Concept
title: Optic Lateral Offset and Sight Convergence
description: Sight plane convergence, lateral sensor offset, and horizontal parallax geometry.
tags: [physics, sight-geometry, offset, convergence]
generated: {by: agent:antigravity, at: 2026-11-12T00:00:00Z}
status: draft
---

# Optic Lateral Offset and Sight Convergence

Automated weapon platforms, counter-UAS turrets, remote weapon stations (RWS), and specialized firearms frequently mount their sighting sensor (daylight camera, thermal/FLIR imager, or optical sight) offset from the bore centerline due to mechanical constraints such as ammunition chutes, recoil buffers, ejection clearance, or sensor pods.

`lob` models 3D sensor placement via vertical height (`optic_height`) and horizontal offset (`optic_offset`). Together, these Cartesian coordinates fully span arbitrary 2D sensor placement across the cross-sectional plane $\mathbb{R}^2$.

## Sight Plane as the Reporting Basis

In exterior ballistics, vertical trajectory drop is reported relative to the sensor's line of sight (LOS) rather than the barrel centerline. `lob` maintains this principle across both vertical and lateral axes:

- **Elevation** is measured relative to the horizontal line-of-sight plane passing through the optical axis ($\text{elevation} = Y_{\text{bullet}} - Y_{\text{sight}}$).
- **Deflection** is measured against the vertical line-of-sight plane passing through the optical axis ($\text{deflection} = Z_{\text{bullet}} - Z_{\text{sight}}$).
- At range zero ($X = 0$), if the sensor is offset $1.5\text{ in}$ to the shooter's right ($Z_{\text{sight}} = -1.5\text{ in}$), the bullet at $Z_{\text{bore}} = 0$ is $+1.5\text{ in}$ to the shooter's left of the sight line.

## Convergence vs. Parallel Zero

Small arms fire control, tank gunnery, and automated turret targeting all employ **convergence** (historically termed "parallax in deflection"):

- The bore is angled laterally by a solved zero azimuth $\phi_0$ so that the bullet trajectory crosses the line of sight precisely at the zero distance $R_0$.
- A "parallel" zero (bore kept strictly parallel to the sight axis) leaves an uncompensated horizontal offset at all ranges and cannot place impacts on the sensor reticle / tracking track.
- In `lob`, `BuildZeroAzimuth` solves for $\phi_0$ at build time, ensuring forward deflection and inverse windage hold at $R_0$ both converge to zero.
- When zeroing in the presence of crosswind, Coriolis, or sheared wind profiles, `BuildZeroAzimuth` absorbs lateral drift into $\phi_0$, matching the physical action of zeroing a sight. When lateral perturbations are absent, a closed-form geometric fast path solves $\phi_0 = \arctan(\text{offset} / R_0)$ with zero ODE integration overhead.

## Direct / Portable Azimuth Zero

When using portable zero angles or calibrated turret offsets from external fire-control computers, `ZeroAzimuthMOA` directly sets $\phi_0$ in MOA, bypassing numerical zero search. If `zero_angle_moa` is set without zero distance and without `zero_azimuth_moa`, `zero_azimuth` defaults to $0.0$.

## Scope and Limitations

- **No parallel zero:** Trajectories always converge to the sight line at zero distance.
- **No lateral impact offset:** Lateral zeroing is always dead-on at $R_0$ (no counterpart to vertical `zero_impact_height`).
