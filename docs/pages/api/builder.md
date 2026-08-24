@page api_builder Builder

# Builder

`Builder` collects the dozens of optional ballistic inputs, validates them as
a set, and produces an immutable `Context` for the solver.
`include/lob/lob.h` defines `LobBuilder`; `include/lob/lob.hpp`
wraps it as `lob::Builder`.

@section api-builder-why Why a builder

Ballistic solves have many interacting optional inputs (atmosphere, wind,
twist, geometry).  A flat `Solve(bc, mv, ...)` would be error-prone and
un-extensible.  The builder lets callers name only what they know, defaults
the rest, and validates the whole set before any integration runs
(@ref design_overview).  `Build()` never allocates and never throws.

@section api-builder-lifecycle Lifecycle

```cpp
lob::Builder b;
b.BallisticCoefficientPsi(0.425).InitialVelocityFps(2700).ZeroDistanceYds(100);
lob::Context ctx = b.Build();
if (ctx.error != lob::ErrorT::kNone) { /* handle */ }
lob::Solve(ctx, ranges, &outs);

// Reuse or reset
b.Reset();
lob::Builder c = b; // copy is deep, via LobBuilderCopy
```

C API equivalents: `LobBuilderInit`, `LobBuilderDestroy`, `LobBuilderCopy`,
`LobBuilderReset`, `LobBuilderBuild` (`include/lob/lob.h`).

`LobBuilder` is an opaque buffer of `LOB_BUILDER_BUFFER_SIZE` (272 bytes,
`include/lob/lob.h`); C++ `Builder` holds it by value and forwards each setter
to the C function.  C setters are `nullptr`-safe; the C++ wrapper returns
`*this` for chaining.

@section api-builder-required Required vs optional

`Build()` enforces only three required inputs:

- ballistic coefficient (@ref LobBuilderBallisticCoefficientPsi) **or** a
  drag table (@ref LobBuilderSplineFitTable) **or** BC/velocity bands
  (@ref LobBuilderBCVelocityBands) — one source of drag must be present;
- initial velocity (@ref LobBuilderInitialVelocityFps);
- a zero (@ref LobBuilderZeroAngleMOA **or** @ref LobBuilderZeroDistanceYds
  with optional @ref LobBuilderZeroImpactHeightInches).

Everything else defaults:

- optic height 1.5 in, wind 0, range angle 0, humidity 0 %, temperature and
  pressure derived from ISA at the firing-site altitude, atmosphere reference
  Army Standard Metro, drag function G1, step 1 yard.

Geometry for spin-related corrections (`DiameterInch`, `LengthInch`,
`MassGrains`, `TwistInchesPerTurn`, plus Boatright-specific
`MeplatDiameterInch`, `BaseDiameterInch`, `NoseLengthInch`,
`TailLengthInch`, `OgiveRtR`) is optional; when incomplete lob falls back
to lower-fidelity formulas or skips the correction entirely
(@ref model_spin).

@section api-builder-errors Errors

`Context::error` is `kLobErrorNone` on success, otherwise one of the
`LobErrorT` enumerants (`include/lob/lob.h`).  Selected errors:

- `kLobErrorBallisticCoefficientRequired` / `kLobErrorInitialVelocityRequired`
  / `kLobErrorZeroDataRequired`
- `kLobErrorZeroUnreachable` — zero distance cannot be reached within
   `@ref num_zero_angle` bounds (±45°, 10 iterations; above 45° a high/low
   duplicate solution would exist and the solver does not disambiguate)
- Table errors: `kLobErrorMachDragTable*`, `kLobErrorBcBands*`
- Range/atmosphere OOR errors

`Build()` stops at the first error; later setters still overwrite stored
values but the error is reported only at build time.  See
`source/lob_builder.cpp` (`LobBuilderBuild`) for the validation order.

@section api-builder-tables Drag tables

Two table setters override the single-BC path; the **last call between the
two table setters wins** regardless of order (`source/lob_builder.cpp`
`LobBuilderSplineFitTable` / `LobBuilderBCVelocityBands`; `BCDragFunction` is not
a table and does not affect the choice):

- `MachVsDragTable` / `LobBuilderSplineFitTable` — Mach vs Cd.  Must have
  `size >= 2`, Machs strictly increasing, Cd finite and `>= 0`.  If the table
  does not span Mach 0–5 it is cubically extrapolated (PCHIP Hermite);
  extrapolation that would yield negative Cd fails with
  `kLobErrorMachDragTableInvalid` — pad with explicit entries at 0 and/or 5
  to control the edge (`include/lob/lob.h`).
- `BCVelocityBands` / `LobBuilderBCVelocityBands` — velocity (fps) vs BC.
  `2 <= size <= 16`, velocities positive strictly increasing, BCs positive
  finite, highest velocity `< Mach 5` at local speed of sound.  See
  @ref bc_transformation for the transformation.

Both setters copy no data; the caller must keep the pointed-to arrays alive
until `Build()` returns.  The `lob::Builder` overloads taking
`std::array` reject temporaries at compile time.

@section api-builder-thread Thread safety

`Builder` is not thread-safe.  `Context` is immutable after `Build()` and
may be shared across threads; `LobSolve`/`LobSolveInverse` are reentrant
and operate only on their `const LobContext*`.
