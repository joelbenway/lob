@mainpage Technical Reference Overview

# lob Technical Reference

lob computes the point-mass trajectory of a symmetric projectile: drag,
gravity, wind, atmosphere, Coriolis, spin drift and aerodynamic jump. This
site documents how it works — the physics, the numerics, the design choices
and the evidence behind them. For a quick start, see @ref getting_started; for
installation and packaging, see the repository `README.md` (intentionally
separate from this reference).

@section overview-map How this documentation is organized

- **Getting Started** — @ref getting_started — build, install and minimal
  example.  Start here if you have never built lob.
- **API Reference**
  - @ref api_builder — constructing a `LobContext` / `lob::Context`
  - @ref api_forward — `LobSolve()` / `lob::Solve()`
  - @ref api_inverse — `LobSolveInverse()` / `LobFastInverse()`
  - @ref api_context — `LobContext`, `LobOutput`, lifetime
  - @ref api_errors — full `LobErrorT` catalog
  - @ref api_options — `StepSize` / `MaximumTime` / `MinimumSpeed`/`MinimumEnergy`
  - @ref api_units — unit helpers and conversions
  - @ref ref_constants — physical constants (ISA, McCoy, Huang)
- **Ballistic Model**
  - @ref model_point_mass — equations of motion
  - @ref model_drag — standard curves, custom tables, splines
  - @ref model_atmosphere — ISA atmosphere, density, speed of sound
  - @ref model_wind_coriolis — wind and Coriolis
  - @ref model_spin — spin drift and aerodynamic jump (Litz and Boatright)
- **Numerical Methods**
  - @ref num_ode — Heun integration in down-range
  - @ref num_splines — cubic Hermite PCHIP splines
  - @ref num_zero_angle — zero-angle root finding
  - @ref num_inverse — inverse (adjustment) solving and its shared solver
- **Ballistic Coefficient**
  - @ref bc_overview — concepts
  - @ref bc_transformation — BC/velocity-band transformation (math)
  - @ref bc_worked_example — figure and trajectory (worked example)
- **Design Decisions**
  - @ref design_overview — why the library is shaped this way
  - @ref design_shared_solver — the shared angle solver
- **Validation**
  - @ref validation_overview — how correctness is established

@section overview-reading Where to start

- **New user** → @ref getting_started then @ref api_builder.
- **Auditing the physics** → @ref model_point_mass → @ref model_drag → @ref model_atmosphere.
- **Auditing the numerics** → @ref num_ode → @ref num_splines → @ref num_zero_angle.
- **Understanding BC bands** → @ref bc_worked_example (the most complete worked example).

@section overview-code Where to find the code

Implementation lives under `source/`; public headers under `include/lob/`:

- Builder and environment: `LobBuilder`, `source/lob_builder.cpp`
- Forward/inverse solves: `LobSolve()`, `LobSolveInverse()`, `source/lob_solve.cpp`
- Single step: `source/solve_step.cpp`, `source/solve_step.hpp`
- Angle solver: `source/solve_angle.hpp`
- Splines: `source/splines.hpp`
- Drag tables: `source/tables.hpp`
- Atmosphere/physics helpers: `source/calc.hpp`, `source/constants.hpp`
- Strong units: `source/eng_units.hpp`

Tests under `test/source/` mirror each area and are cited from the technical
pages as evidence rather than claims.
