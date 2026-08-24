@page model_drag Drag Functions

# Drag Functions

@section model-drag-std Standard curves

lob ships six standard drag curves G1, G2, G5, G6, G7, G8 sampled at
87 Mach points from 0 to 5.0 (`source/tables.hpp`).  The tables are BRL data
as published by JBM Ballistics.  Each curve is stored as `kG*Drags` alongside
the shared Mach grid `kMachs` and is projected once at compile time onto a
compact spline (@ref num_splines).

The ballistic coefficient scales the standard curve.  With atmosphere
reference `kArmyStandardMetro` the BC is converted by
`kArmyToIcaoBcConversionFactor = 0.0751265/0.0764742 ≈ 0.982`
(`source/constants.hpp`) so the spline always represents the ICAO
standard.  The context stores `drags[60] = coefs / (BC·conversion)`
(`source/lob_builder.cpp`).

A `BC ≤ 0` or missing BC is rejected at build time.

@section model-drag-custom Custom tables

`LobBuilderSplineFitTable` / `Builder::MachVsDragTable` accepts a
user-supplied Mach vs Cd table.  Requirements (`source/lob_builder.cpp`):

- `size ≥ 2`, Machs strictly increasing, Cd finite and `≥ 0`.

The table is spline-fit onto the production knots (see @ref num_splines).
If the table does not span Mach 0–5 the spline is cubically extrapolated (PCHIP
Hermite using the outer interval's tangents); extrapolation that would yield
negative Cd at 0 or 5 is rejected with `kLobErrorMachDragTableInvalid` — pad
with explicit entries at 0 and/or 5 to define the edge behavior.  Custom tables
set the effective BC to 1 in the context (`source/lob_builder.cpp`).

@section model-drag-bands BC/velocity bands

`LobBuilderBCVelocityBands` / `Builder::BCVelocityBands` tailors the standard
curve with BCs measured at several velocities (fps).  See @ref bc_transformation
for the transformation.

@section model-drag-curve The drag curve in the solver

At each step `SolveStep` evaluates `Cd = curve.Eval(Mach) · drag_coeff`
(`source/solve_step.cpp`) where `Mach = |v| / speed_of_sound` and `curve`
is a `CurveView` over the context's 60 coefficients.  `CurveView::Eval`
clamps Mach outside 0–5 to the edge value; the integration never evaluates
beyond the checked domain.

The precomputed G* coefficients are `constexpr` (`source/splines.hpp`)
so they cost no runtime construction.  Custom/BC-band curves are built at
`Build()` time via the same spline machinery.
