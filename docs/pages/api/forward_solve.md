@page api_forward Forward Solve

# Forward Solve

The forward solve returns the trajectory's height and windage at the requested
down-range distances.

@section api-forward-signatures Signatures

```cpp
size_t lob::Solve(const Context& ctx,
                  const uint32_t* pranges, Output* pouts, size_t size);
size_t lob::Solve(const Context& ctx,
                  const std::array<uint32_t,N>& ranges,
                  std::array<Output,N>* pouts);
size_t lob::Solve(const Context& ctx, uint32_t range, Output* pout);

size_t LobSolve(const LobContext* pctx,
                const uint32_t* pranges, LobOutput* pouts, size_t size);
```

`pranges` are **feet**, strictly increasing, `> 0` except an entry may be `0`
for the muzzle.  `pouts[i].range` echoes the achieved range in feet (truncated
from the integrated position).  Returns the number of outputs written; on
invalid arguments returns `0` (`source/lob_solve.cpp`).

@section api-forward-output Output

`LobOutput` (`include/lob/lob.h`):

- `range` — feet reached for this entry
- `velocity` — fps at that range, `uint16_t`
- `energy` — foot-pounds, `uint32_t`
- `elevation` — **inches** of vertical displacement relative to the sight line
  (`P.Y - optic_height`).  Negative is below the line of sight.
- `deflection` — **inches** of lateral displacement (wind + spin drift).
- `time_of_flight` — seconds

For a forward solve, `elevation`/`deflection` are linear distances.  For
inverse solves they are angles in MOA (@ref api_inverse).

@section api-forward-stops Termination

Integration proceeds in down-range steps (@ref num_ode) until one of:

- all requested ranges are reached — outputs are exact (last step clamped to
  the target);
- `max_time` exceeded — the last output is linearly interpolated to `max_time`;
- `minimum_speed` or `minimum_energy` (converted to a speed via
  `CalculateVelocityFromKineticEnergy`; the effective `minimum_speed` is
  `max(explicit, from_energy)`, `source/lob_builder.cpp`) reached — linearly
  interpolated to that speed;
- projectile tumbles (`|v_y| > 3 v_x` — treated as falling straight down,
  `source/lob_solve.cpp`). Output records the achieved state at that step.

If the projectile cannot reach a requested range, `LobSolve` still produces an
output at the fall-short distance (`max_time` and `minimum_speed` are linearly
interpolated, while tumble records the step state); `LobSolveInverse` calls that
same `LobSolve` and then stops at the first `R` where `SolveAngle` returns NaN,
returning only the reachable prefix (@ref api_inverse).

@section api-forward-errors Preconditions

`LobSolve` returns `0` without touching `pouts` if:

- any pointer is null, `size == 0`, `ctx.error != kLobErrorNone`,
  `ctx.velocity == 0`, `ctx.speed_of_sound <= 0`, or `ctx.zero_angle` is NaN;
- `pranges` is not strictly increasing.

No allocation or exceptions occur inside the solve.
