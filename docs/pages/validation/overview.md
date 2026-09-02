@page validation_overview Validation

# Validation

lob is numerical software: correctness is established by evidence, not by
assertion. Each claim below cites a test or measurement; no universal
accuracy promise is made.

@section validation-strategy Strategy

- **No invented accuracy claims.**  Each assertion below cites a test or a
  measurement; the phrasing mirrors the tests' tolerances rather than “lob is
  accurate to X everywhere.”
- **Evidence before synthesis**: tests are the primary contract; technical
  pages describe what the code does and the tests bound how well.

@section validation-reference Reference data

- **BRL drag tables**: 87-point Mach 0–5 tables for G1…G8 from JBM
  (`source/tables.hpp`).  The spline reproduces them within `5e-3` max
  absolute error over a 5000-point truth grid
  (`test/source/splines_test.cpp`, `SplineOptimization::BaselineAccuracyBudget`).
- **Atmosphere & ballistics**: reference trajectories in
  `test/source/lob_env_test.cpp` etc. for ICAO, altitude (+4500 ft),
  hot/low-pressure, barometer-offset and weather-station cases, each with
  expected `velocity ±1 fps`, `energy ±5 ft·lbf`, `0.1 MOA` equivalent and
  `±0.01 s` out to 1000 yd — matching McCoy's ISA treatment and Huang's
  saturation formula.
- **Spin/jump**: Boatright and Litz against their papers; the builder test
  forces the Boatright supersonic integration to timeout
  (`kLobErrorInternalError`, `test/source/lob_builder_test.cpp`) to prove
  the guard fires.

@section validation-accuracy Numerical accuracy

- Default 1-yard Heun step meets the reference tolerances above.  Halving the
  step reduces error monotonically (`Builder::StepSize`); the benchmark
  `benchmark/ode.cpp` shows linear cost.
- The PCHIP splines preserve monotonicity and never overshoot by construction
  (`source/splines.hpp`, `test/source/splines_test.cpp`).
- Angle solver tolerance is `0.01 MOA` with at most 10 iterations
  (`source/solve_angle.hpp`); tightening to `0.001 MOA` is tested for
  convergence to `±0.01 in` (`test/source/solve_angle_test.cpp`).
- Inverse vs forward consistency: iterative `SolveInverse` matches
  `FastInverse` within `0.1 MOA` on the cross-check trajectories
  (`test/source/lob_inverse_test.cpp`).  Forward vs inverse sign flip is
  asserted (`test/source/lob_inverse_test.cpp`).

@section validation-invariants Invariants

- `CurveView` clamps below `0` and above `5` Mach to the edge drag
  (`test/source/splines_test.cpp`); seeks from start to end and back are
  `O(1)` (`test/source/splines_test.cpp`).  See also @ref api_errors for the
  full `Build()`-vs-solve error split and @ref ref_constants for the ISA values
  behind the tolerances.
- `FindInterval` is a binary search; its edge cases are unit-tested
  (`test/source/splines_test.cpp`).
- Custom-table extrapolation that would yield negative Cd at 0 or 5 is rejected
  (`test/source/lob_builder_test.cpp`, `source/lob_builder.cpp`).
- BC bands are clamped flat outside the user Machs
  (`test/source/lob_builder_test.cpp`).
- Zero/inverse angle is bounded to `±45°` and returns NaN if exceeded;
  unreachable targets return NaN/`kLobErrorZeroUnreachable`
  (`test/source/solve_angle_test.cpp`).
- Inputs are strictly increasing where required (ranges, Machs, fps); non-
  monotonic returns `0` or the corresponding `*NotMonotonic` error.

@section validation-coverage Coverage and checks

[![codecov](https://codecov.io/gh/joelbenway/lob/graph/badge.svg)](https://codecov.io/gh/joelbenway/lob) — coverage from `cmake --preset=ci-coverage && cmake --build build/coverage -j && ctest --test-dir build/coverage && cmake --build build/coverage -t coverage` (see `.github/workflows/ci.yml` coverage job).

- CI runs `lint`, `sanitize` (ASan+UBSan), `coverage` and cross-OS `test` (`macos`, `ubuntu`, `windows`, shared/static).
- Unit tests live under `test/source/` and mirror each production file: `calc`, `cartesian`, `eng_units`, `helpers`, `ode`, `splines`, `solve_angle`, `solve_step`, `boatright`, `litz`, plus integration tests `lob_env`, `lob_coriolis`, `lob_wind`, `lob_spin_drift`, `lob_cwaj`, `lob_builder`, `lob_inverse`, `lob_api`.

@section validation-future Gaps and follow-ups

- Per-step lapse is gated (`Fast*` firing-site vs `DsDx`/`SolveStep`/`SolveAngle`
  when forward `drop>100ft`), not universal — see @ref model_atmosphere and
  `source/lob_solve.cpp` `kDynamicDropThresholdIn`. Long-range `>100ft` inverse
  accuracy vs field data would be a stronger anchor than builder-time ISA alone.
- No field-data validation beyond the BRL tables and borrowed reference
  trajectories; a “field vs radar” data set would be a stronger anchor.
- Performance is benchmarked (`ci-benchmark` `cachegrind`) but no regression
  gate is enforced in CI beyond `CG_MAX_REGRESSION_BP`.
- The spline knot optimization (`5e-3` budget) is validated but its derivation
  is not documented beyond the exhaustive test's `5000`-point grid — worth a
  short note if the knot set ever changes.
