@page api_inverse Inverse Solve

# Inverse Solve

The inverse solve answers the shooter's question: *what sight adjustment is
needed to hit at this range?*

@section api-inverse-signatures Signatures

```cpp
// Iterative: solves for the angle that makes the trajectory hit
size_t lob::SolveInverse(const Context& ctx,
                         const uint32_t* pranges, Output* pouts, size_t size);

// One-step: converts an existing forward result in place
size_t lob::FastInverse(const Context& ctx, Output* pouts, size_t size);

size_t LobSolveInverse(const LobContext* pctx,
                       const uint32_t* pranges, LobOutput* pouts, size_t size);
size_t LobFastInverse(const LobContext* pctx, LobOutput* pouts, size_t size);
```

All three overloads (raw pointer, `std::array`, single value) share semantics
with the forward solve.

@section api-inverse-semantics What “inverse” means

Forward: at range `R`, with launch angle `θ₀ = zero_angle + aerodynamic_jump`,
the trajectory height is `y(R; θ₀)` (inches).  

Inverse: find `θ*` such that `y(R; θ*) = 0` and report the adjustment
`Δ = θ* − zero_angle` (MOA) plus a windage adjustment
`−deflection_inches → MOA` at the same `R`.

In the returned `Output`, `elevation` and `deflection` are **MOA**, not inches.
`range`, `velocity`, `energy`, `time_of_flight` are unchanged from the
forward trajectory at that `R`.

@section api-inverse-two Two mechanisms

**`LobSolveInverse` — iterative, accurate.**

1. Runs a forward solve for the requested ranges to confirm reachability.
2. For each reachable `R`, computes `f(θ)=y(R;θ)` via the same integrator as
   @ref num_zero_angle.
3. Seeds the iteration with `FastInverseAngle` applied to the forward residual,
   then iterates @ref LobSolveInverse with the shared angle solver
   (@ref num_zero_angle, @ref design_shared_solver).
4. If any `R` is unreachable (`SolveAngle` returns NaN) solving **stops** at
   that index and the unreachable entry is not counted — the caller gets only
   the reachable prefix.  A `range==0` entry is counted with `0` MOA.

Typical convergence: `≤ 10` iterations, `0.01` MOA tolerance, clamped to
`±45°`.

**`LobFastInverse` — one-step approximation.**

Converts already-computed forward outputs **in place**.  For each `out`:

```
Δ = FastInverseAngle(θ₀, elevation_inches, range) − θ₀
deflection_moa = InchToMoa(−deflection_inches, range)
```

Entries with `range==0` or non-finite `elevation`/`deflection` are skipped
and not counted.  **Only convert outputs whose forward solution actually
reached `R`;** fall-short trajectories yield a meaningless residual and a
meaningless one-step angle (the doc comment in `include/lob/lob.h` says
so explicitly).  Unlike `LobSolveInverse`, a short fall does not stop the
batch — it is silently given a (wrong) number, hence the warning.

@section api-inverse-which Which to use

- Default to `SolveInverse` when you need trustworthy MOA numbers.
- Use `FastInverse` when you already have forward outputs, the ranges are
  known-reachable, and you prefer to avoid re-integrating.  Verify by
  comparing against `SolveInverse` on a spot check — the two agree within
  `~0.1` MOA in the tests (`test/source/lob_inverse_test.cpp`).

@section api-inverse-edge Edge cases

- Invalid context / null pointers → `0`.
- Non-monotonic `pranges` → `0` (same guard as forward).
- Zero `range` → `LobSolveInverse` counts it with `0` MOA; `LobFastInverse`
  skips it and does not count it (`source/lob_solve.cpp`).
- Multiple ranges: both functions require strictly increasing order (for
  `LobSolveInverse` the check is the same `LobSolve` guard; `LobFastInverse`
  operates in-place and does not re-check `pranges`).
