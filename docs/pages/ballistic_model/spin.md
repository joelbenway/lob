@page model_spin Spin Drift and Aerodynamic Jump

# Spin Drift and Aerodynamic Jump

Both are spin-induced corrections that lob models with two fidelity levels — **Litz** (empirical, needs only Miller stability) and **Boatright** (needs full geometry + flight history to Mach 1.2). Selection is automatic: try Boatright, fall back to Litz, otherwise zero. Both are built once at `Build()`.

@section model-spin-drift Spin drift

Lateral deflection from gyroscopic precession (yaw of repose), applied post-solve to `deflection`.

**Boatright** (`source/lob_builder.cpp`) when diameter, meplat, base, length, nose, tail, `ogiveRtR`, velocity, `c`, mass, twist, `stability_factor`, BC and `wind.z` are known:
1. Computes ogive geometry `RT`, `LFN`, aspect ratio, Mach, `Q`, `S`, `CL`, `CDa`, `ρ`, `Iy/Ix`, `P`, `R`, `N`, `F1+F2`, `F2`, `Tn`.
2. Short-integrates to `v = Mach 1.2·c` (60 s timeout → `kLobErrorInternalError`,
   temporary guard — revisit when boatright path is optimized) for supersonic
   time.
3. Forms `KV=log(1.2c/v0)`, `Kω`, `QTS`, `β(R,Ω)`, boattail-adjusted `CL`, `CL(T)` → `spindrift_factor = 0.388132·QTS·β·CL(T)/mass` stored in `ctx.spindrift_factor`. At solve time (`source/lob_solve.cpp`): `deflection += spindrift_factor · |elevation|`.

If incomplete, `spindrift_factor = NaN` and drift falls through to Litz.

**Litz** (`source/litz.hpp`): `drift = 1.25·sign(sg)·(|sg|+1.2)·TOF^1.83` inches, applied when `spindrift_factor` is NaN and `|sg|>0` (`source/lob_solve.cpp`). `sg` is Miller stability (@ref model_atmosphere). Tests in `test/source/lob_spin_drift_test.cpp`.

**Selection:**
```
if (finite spindrift_factor) use Boatright (factor·|drop|)
else if (|sg|>0)             use Litz (TOF^1.83)
else                         no drift
```

**Limitations:** Litz is empirical, not 6-DOF; Boatright's many intermediates are from Boatright & Ruiz papers, not claimed beyond “tests pass”.

@section model-aerodynamic-jump Aerodynamic jump

Vertical pitch at the muzzle from crosswind on a spinning projectile, modeled as an angle added to launch:

```
launch = zero_angle + aerodynamic_jump   (source/lob_solve.cpp, source/solve_angle.hpp)
```

Computed once and reused for every solve (including zero-finding and inverse).

**Boatright** when same full inputs present (`source/lob_builder.cpp`):
```
γ   = w_z / v0
R   = epicyclic ratio from sg
N   = nutation cycles
CD  = CD0 + γ²·R²/(R−1)²·CDa
pitch = γ·(R²−1)/(2πNR)·(1 − cos(2πN/(R−1)))
Jv  = sign(twist)·N·Tn·Q·S·(CL+CD)·sin(pitch)
jump = −Jv / MOM   (radians → MOA)
```
Uses same supersonic `Tn` etc. as drift; stored in `ctx.aerodynamic_jump`.

**Litz** if Boatright didn't produce a jump (`source/lob_builder.cpp`):
- `w_z==0` → 0
- else if `sg`, `diameter`, `length` available: `jump = sign(sg)·(0.01·|sg| − 0.0024·L/D + 0.032)·w_z_mph` MOA (`source/litz.hpp`, p.422)
- else 0

Both forward (`LobSolve`) and angle-finding (`SolveAngle`) add `jump` before `v = v0·(cos launch, sin launch, 0)`, so zero and inverse share it (@ref design_shared_solver). Validated in `test/source/lob_cwaj_test.cpp` and `lob_inverse_test.cpp` with non-zero jump.

**Limitations:** Single scalar added to elevation; no rolling-moment history; zero when inputs missing (deliberate graceful degradation).

