---
okf_version: "0.2"
exclude: [SKILL.md]
---

# lob Knowledge

lob computes the point-mass trajectory of a symmetric projectile: drag,
gravity, wind, atmosphere, Coriolis, spin drift and aerodynamic jump.
This bundle documents how it works, why it is shaped the way it is, and
where the design decisions came from. The API reference alongside this
bundle documents what to call.

## Start here

- [Getting Started](getting-started.md): build a context and get your
  first solution.
- [Ballistic Coefficient](ballistic-coefficient.md): the one number
  that stands in for a bullet's drag, and why several of them are better
  than one.
- [Solver Context](solver-context.md): what a context promises about
  lifetime, reuse, threading, and failure.

## The ballistic model

What lob computes.

- [Point-Mass Model](point-mass-model.md): the equations it integrates,
  and why it steps by distance instead of time.
- [Atmospheric Model](atmospheric-model.md): density and speed of sound
  from imperfect weather data.
- [Drag Model](drag-model.md): how drag curves become splines.
- [Spin Drift and Aerodynamic Jump](spin-drift-and-aerodynamic-jump.md):
  two corrections for a spinning bullet in crosswind.
- [Wind and Coriolis](wind-and-coriolis.md): corrections that act on
  velocity rather than on geometry.

## Ballistic coefficients in depth

- [Ballistic Coefficient](ballistic-coefficient.md): how one number
  stands in for a bullet's entire drag curve.
- [BC/Velocity-Band Transformation](bc-velocity-band-transformation.md):
  turning several measured coefficients into one smooth curve.
- [BC Bands Worked Example](bc-bands-worked-example.md): bands to a
  trajectory, with copy-pasteable code.

## Numerical methods

How the trajectories are actually computed.

- [Cubic Hermite Splines](cubic-hermite-splines.md): the interpolation
  scheme every drag curve is stored as.
- [Integration Accuracy and Step Size](integration-accuracy.md): what
  the step size buys, costs, and stops guaranteeing.
- [Inverse Solving](inverse-solving.md): turning a range into sight
  adjustments.

## Design decisions

Why lob is shaped the way it is.

- [Design Decisions](design-decisions.md): survey of the decisions and
  what they cost.
- [C ABI Narrow Waist](c-abi-narrow-waist.md): one implementation,
  every language.
- [Strong Unit Types](strong-unit-types.md): wrong units fail to
  compile.
- [No Dynamic Allocation](no-dynamic-allocation.md): suit targets that
  forbid the heap.
- [Builder Pattern](builder-pattern.md): name what you know.
- [Heun Integration](heun-integration.md): second-order steps, per BRL
  experience.
- [Performance Posture](performance-posture.md): speed from modeling.
- [Shared Angle Solver](shared-angle-solver.md): one routine serves
  zero and inverse.
- [Angle Solver Mechanics](angle-solver.md): the problem, the
  iteration, the seeds, the evidence.

## Evidence

- [Validation](validation.md): how correctness is established, and what
  is still unproven.

For API reference, including every function signature, error code and
constant, see the header documentation generated alongside this bundle.