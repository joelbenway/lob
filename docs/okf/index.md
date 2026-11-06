---
okf_version: "0.2"
exclude: [SKILL.md]
---

# Technical Reference

Welcome to the technical reference for **lob**, a fast, zero-allocation exterior ballistics library written in modern C with bindings for WebAssembly and modern languages.

This reference documents the complete architecture, physical models, numerical integration routines, and design decisions behind lob:
- **Ballistic Physics:** Point-mass trajectory integration, 1976 Standard Atmosphere, supersonic drag tables, Heun second-order integration, and environmental corrections (wind, Coriolis, spin drift, aerodynamic jump).
- **Numerical Methods:** Cubic Hermite spline interpolation, step size and truncation error bounds, and shared forward/inverse zero solvers.
- **Software Architecture:** Pure C ABI narrow waist, zero dynamic heap allocation, strong unit typing, and builder configurations.

Explore the complete interactive topic map below. Click any concept or design decision node to navigate directly to its documentation, or browse topics using the sidebar navigation tree.

For the full C API reference, including every function signature, error code, and constant, see the Files and Classes documentation in the sidebar.

<div style="display: none;">

- [Basic Worked Example](getting-started.md)
- [Ballistic Coefficient](ballistic-coefficient.md)
- [Solver Context](solver-context.md)
- [Solver Input State Machine](solver-input-state-machine.md)
- [Point-Mass Model](point-mass-model.md)
- [Atmospheric Model](atmospheric-model.md)
- [Drag Model](drag-model.md)
- [Spin Drift and Aerodynamic Jump](spin-drift-and-aerodynamic-jump.md)
- [Wind and Coriolis](wind-and-coriolis.md)
- [BC/Velocity-Band Transformation](bc-velocity-band-transformation.md)
- [BC Bands Worked Example](bc-bands-worked-example.md)
- [Cubic Hermite Splines](cubic-hermite-splines.md)
- [Integration Accuracy and Step Size](integration-accuracy.md)
- [Inverse Solving](inverse-solving.md)
- [Design Decisions](design-decisions.md)
- [C ABI Narrow Waist](c-abi-narrow-waist.md)
- [Strong Unit Types](strong-unit-types.md)
- [No Dynamic Allocation](no-dynamic-allocation.md)
- [Builder Pattern](builder-pattern.md)
- [Heun Integration](heun-integration.md)
- [Performance Posture](performance-posture.md)
- [Shared Angle Solver](shared-angle-solver.md)
- [Angle Solver Mechanics](angle-solver.md)
- [Validation](validation.md)
</div>