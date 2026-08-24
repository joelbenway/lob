@page design_overview Design Decisions

# Design Decisions

This page records the “why” behind the choices a contributor might question.
Each claim is tied to the code or tests; where rationale is not evident in the
implementation the text says so.

@section design-capi C/C++ API architecture

lob exposes a **C ABI** (`include/lob/lob.h`, `source/lob_builder.cpp` — pure
technical reference, root `BUILDING.md`/`README.md` are intentionally separate)
and a C++ wrapper (`include/lob/lob.hpp`).

- The C layer `LobBuilder*`, `LobContext`, `LobSolve` is `extern "C"` with
  plain aggregates, `LOB_EXPORT`, and opaque `LobBuilder` (a 272-byte buffer).
  It provides a stable ABI for FFI: any language with a C FFI can call lob
  without a C++ toolchain or name-mangling concerns.
- The C++ `lob::Builder` etc. (`include/lob/lob.hpp`) holds a `LobBuilder`
  by value, forwards to the C functions, and adds type-safe overloads
  (`std::array`, scoped enums, `std::isfinite` guards, deleted r-value
  temporaries for tables).  Layout equivalence is enforced via
  `static_assert(sizeof(Context)==sizeof(LobContext))` (`include/lob/lob.hpp`).

Implementation lives once (in `source/lob_builder.cpp`, `source/lob_solve.cpp`);
the wrapper is header-only and `inline`.  hourglass shape keeps the solver
independent of language bindings.

@section design-units Strong unit types

`source/eng_units.hpp` defines `StrongT<E,U,T>`, a `constexpr` wrapper
parameterized by category (Length, Speed, Angle…) and unit (Inches, Feet…).
Cross-category arithmetic is ill-formed; intra-category conversion is an
explicit `operator Other` (e.g. `InchT → FeetT`, `MoaT → RadiansT`).

Purpose: eliminate unit mixups inside the integrator, atmosphere, and
Boatright/Litz code — the class of bugs with no test failure but wrong
trajectory.  The types are zero-cost and have `Value()`, `IsNaN()`, `U32()`
etc. for the ABI boundary.

Where it stops: `LobContext` and `LobOutput` store plain `double`/`uint16_t`
so the C ABI is not templated; callers using the C API directly must get
units right themselves.  The C++ builder names units in the setter
(`DiameterInch`, `MassGrains`) to recover safety at the ergonomic layer.

@section design-noalloc No dynamic allocation

`lob` never calls `new`, `malloc`, or throws.  `LobBuilder` is an inline
buffer (`LOB_BUILDER_BUFFER_SIZE 272`, `include/lob/lob.h`); tables are not
copied (`table_xs/table_ys` are borrowed pointers, `source/lob_builder.cpp`);
the spline is 60 floats; the state is two `CartesianT`s plus `SecT`.

Motivation: suit embedded targets and real-time callers that forbid heap use;
make the library trivially `FetchContent`-friendly with no allocator hooks.
Consequence: tables must outlive `Build()`, which the docs call out explicitly
(`include/lob/lob.h`).

@section design-builder Builder pattern

See @ref api_builder.  Ballistic solves have ~25 optional interacting inputs;
a flat function would have an unusable arity and no place for cross-field
validation (e.g. “zero distance without velocity is meaningless”,
“Army vs ICAO conversion”).  The builder lets callers name only what they
know, defaults the rest to ISA/zero, and validates the whole set before the
solver runs.  The two stages also let high-fidelity branches (Boatright) test
“is this geometry complete?” without penalizing minimal builds.

@section design-integration Numerical integration choices

See @ref num_ode.  lob integrates in down-range distance using **Heun (RK2)**
with a fixed 1-yard default step and a trapezoidal `TOF` update. Integrating
over distance with a configurable step size provides flexibility across diverse
applications — intercepting a drone at 50 yards within a millisecond requires
different spatial resolution than calculating a multi-mile long-range shot.

The method choice follows BRL experience:
"The cumulative experience of the Ballistic Research Laboratory has shown that
the one-step, second-order methods are, in the practical sense, optimum
solutions of the point-mass trajectory problem." (McCoy, *Modern Exterior
Ballistics — The Launch and Flight Dynamics of Symmetric Projectiles*, 2nd ed).
RK4 is ~2× the cost and cannot quite double the step size (`benchmark/ode.cpp`
— each method there is tuned to ~1 ft error vs an RK4 `dt=1e-5` reference, so
the 2× is cost-matched, not `dt`-matched); Euler needs sub-inch steps to meet
the `1 fps` tolerance.  Heun on the 1-yard grid passes the registered reference
trajectories within `@ref validation_overview` tolerances with linear cost.

Step size is caller-configurable via `StepSize` (inches, `0`=default yard);
reducing it monotonically improves accuracy (benchmark `benchmark/ode.cpp`).

@section design-perf Performance considerations

- Drag tables are 87-point data resampled once onto 16 PCHIP knots → 60 floats; the six `kG*Coefs` are `constexpr` (`source/splines.hpp`). Evaluation is 7 FLOPs, branch-light.
- `CurveView` caches the segment index; flight Mach is monotone decreasing so `Seek` is `O(1)`.
- No heap, no exceptions, no virtual calls on the hot path.

No universal speed claim is made. Reproduce locally:

```bash
cmake --preset=ci-benchmark && cmake --build build/benchmark -j && ./build/benchmark/ode && ./build/benchmark/loblerp
```

Sample (4×3400 MHz, 2026-08-22, `benchmark_min_time=0.2s`):

| Benchmark | Time | Error (ft) vs exact ODE |
|-----------|------|------------------------|
| Heun (production, 1-yd) | 33 226 ns | 7.36 |
| IterativeHeun 3× | 67 766 ns | 6.94 |
| LobLerp | 329 128 ns | — |
| CurveView (PCHIP) | 167 774 ns | — |

Heun is ~2× faster than 3×-iterated Heun for 0.4 ft error delta; `CurveView` is ~1.96× faster than linear lerp on the same 87-point table. Step size via `Builder::StepSize` reduces error monotonically with linear cost — measure your trajectory, don’t trust a global number.

@section design-shared Shared angle solver — why it matters

See @ref design_shared_solver for the dedicated treatment.  In short, zero
finding and inverse solving solve the same root `f(θ)=0`; sharing
`SolveAngle`/`FastInverseAngle` (`source/solve_angle.hpp`) guarantees the
zero used at build time and the adjustments returned later are consistent.

