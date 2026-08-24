@page bc_overview Ballistic Coefficient Overview

# Ballistic Coefficient

@section bc-overview-what What a BC is

A ballistic coefficient (BC) is a sectional-density / form-factor ratio that
scales a standard drag curve to a particular bullet:

```
Cd_bullet(M) = Cd_std(M) / BC_effective(M)
```

In lob `Cd_std` is one of the G* tables (@ref model_drag); division by BC
is performed in the spline coefficients so the solver sees a single
`Cd(M)` lookup.  `BC_effective` may be constant (single-BC mode) or a
Mach-dependent curve built from multiple BC/velocity pairs
(@ref bc_transformation).

BCs are expressed as pounds-mass per square inch (Pmsi), i.e. the customary
American unit.  Internally `source/calc.hpp` computes `ρ·π / (8·BC·144)` with
`144 = in²/ft²`; the solver stores `drag_coeff = ρ·π / 8` and divides `BC`
into the spline `drags[60]` (see @ref api_context and @ref model_point_mass).

@section bc-overview-std Standard curves

The six curves G1, G2, G5, G6, G7, G8 in `source/tables.hpp` are 87-point
Mach 0–5 tables from BRL/JBM.  Lob projects each onto 16 PCHIP knots
(@ref num_splines) at compile time (`source/splines.hpp`).  G1 is the
flat-base spitzer and the library default (most common among casual users
relying on default settings); G7 is the boattail match preferred for
long-range rifle bullets.  The builder parameter `BCDragFunction` selects
the curve; the atmosphere reference (Army vs ICAO) adds the
`kArmyToIcaoBcConversionFactor` (≈0.982) before the division.

@section bc-overview-single Single BC

With `BallisticCoefficientPsi(BC)` the context stores

```
drags[i] = coefs_std[i] / (BC · conversion)   (source/lob_builder.cpp)
```

A constant BC across all velocities reproduces the single-BC result exactly;
the BC-band test `BCVelocityBandsConstantBcScalesG1Curve`
(`test/source/lob_builder_test.cpp`) verifies this to `1e-4`.

@section bc-overview-why-bands Why bands

A single BC cannot follow a bullet whose form factor changes with Mach (most
modern bullets).  Manufacturers increasingly publish BCs at several velocities.
Fitting one `Cd_std/BC` curve through all of them by discretely switching BC
would create a kink in `Cd(M)` and a discontinuity in the numerical derivative.
Instead lob builds a smooth `1/BC(M)` curve and multiplies it with `Cd_std(M)`.

See @ref bc_transformation for the full case study.
