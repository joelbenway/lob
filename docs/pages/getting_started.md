@page getting_started Getting Started

# Getting Started

lob requires C++14 and floating-point support.

@section getting_started-build Building

lob is CMake FetchContent-friendly with no dependencies.

```bash
cmake -B build -DLOB_DEVELOPER_MODE=ON
cmake --build build
ctest --test-dir build
```

For documentation (requires developer mode for the `docs` target):

```bash
cmake -B build -DLOB_DEVELOPER_MODE=ON -DBUILD_DOCS=ON
cmake --build build --target docs
# html in build/docs/html/index.html
```

See `BUILDING.md` for install/packaging options.

@section getting_started-minimal Minimal forward solve

At the heart of lob is the builder → context → solve pipeline
(@ref api_builder, @ref design_overview).

```cpp
#include "lob.hpp"

const lob::Context ctx = lob::Builder()
    .BallisticCoefficientPsi(0.425)
    .InitialVelocityFps(2700)
    .ZeroDistanceYds(100.0)
    .Build();

std::array<uint32_t, 3> ranges{300, 600, 900}; // feet, strictly increasing
std::array<lob::Output, 3> outs{};
const size_t n = lob::Solve(ctx, ranges, &outs);
for (size_t i = 0; i < n; ++i)
    // outs[i].elevation is inches of drop at outs[i].range (feet)
    ;
```

`Builder` validates and returns an error in `Context::error` if required inputs
are missing (BC, velocity, zero).  Check `ctx.error == lob::ErrorT::kNone`
before solving.

@section getting_started-richer Richer example

The same builders scale: add atmosphere, wind, Coriolis, twist and geometry
as needed — see `README.md` for a copy-pasteable richer builder (G7, 155 gr,
Milwaukee example). Any subset may be supplied; lob fills the rest with
defaults or derives higher-fidelity corrections when geometry is complete
(@ref model_spin). Missing data degrades fidelity rather than failing.

@section getting_started-inverse Inverse solve

```cpp
// Forward: inches of drop
lob::Solve(ctx, ranges, &outs);

// Fast one-step conversion of an existing forward result:
lob::FastInverse(ctx, &outs); // elevation/deflection become MOA

// Full iterative solve for MOA adjustments:
lob::SolveInverse(ctx, ranges, &outs); // elevation/deflection are MOA
```

See @ref api_inverse for the difference between the two and when to prefer
each.

@section getting_started-units Units

lob stores everything internally in feet, seconds, pounds and radians.
The public API accepts customary units (inches, yards, fps, grains, inHg,
degrees F, MOA/MIL/degrees/Iphy) and the headers expose free conversion
helpers (`LobMoaToMil`, `lob::MoaToMil`, etc.). See @ref api_units and
@ref design_overview.

From here, the natural reading order is: @ref api_builder for the construction
pipeline, @ref num_ode for the integration method, and @ref validation_overview
for how correctness is established.
