# Tune Tool Contract — v1

**Status:** contract draft (not code, not a formal implementation spec).
**Scope:** the data + behavior contract between lob validation artifacts and
any step-size tool consumer (standalone page now, overhauled docs later).
Shells are replaceable; this document is not — integration into the new docs
is mechanical conformance to what's written here.

## 1. What the tool is and isn't

- IS: a step-size recommender + contribution table for one trajectory, from
  stated input uncertainties and versioned calibration tables.
- IS NOT: a solver (no trajectory integration, no `lob` linkage required),
  an uncertainty reporter (no `u_c` claims — step-only, per the locked scope),
  a persistence layer, a user account system, or per-shot adaptation (session
  granularity: run on mode change, bake the answer into config).
- Step-only, v1 through vForever. No fidelity toggles exist in this contract
  and none may be added without a new proposal.

## 2. Table schemas (inputs the tool reads)

### 2.1 Convergence map (family × band → error constant)

```json
{
  "contract_version": 1,
  "family": "G7-supersonic",
  "range_band_yd": [0, 1000],
  "drag_source": "G7-single-BC",
  "c_in_per_in2": 5.4e-09,
  "fit_points": [{"step_in": 36, "err_in": 7.1e-05}],
  "provenance": {"lob_version": "0.13.0", "git_sha": "<12-hex>",
                 "artifact": "step_error_map.json"}
}
```

- Error model: `ε ≈ C · Δx²` (Heun order-2, verified p≈2 on all measured
  series). `fit_points` are the measured (step, error) pairs `C` was fit
  from — never fewer than two rungs spanning ≥4× in step, or the fit is
  unreviewable.
- `c_in_per_in2` example value is illustrative of shape only until the
  upward-ladder tables land; the checked-in file carries measured values.

### 2.2 Sensitivity rows (referenced, not redefined)

Per-input coefficients are read from the existing `pareto.json` row shape
(`input`, `raw_deriv`, `canned_response`, `status`, `h_accepted`) at the
reference range band. Only rows with `status: "genuine"` enter the
propagation; `below_floor` rows are excluded with reason (same rule as
Phase 2). No second sensitivity schema exists — one shape, two consumers.

### 2.3 Table pinning (anti-staleness)

Every table file carries `contract_version` + full provenance. The tool
bundles a manifest of expected `{lob_version, git_sha-per-table}` pins.
On mismatch the tool REFUSES with a message naming the stale table —
warn-and-continue is forbidden (stale calibration presented as fresh is the
exact failure this contract exists to prevent). Pin updates ride with lob
releases, reviewed like code.

## 3. Input contract (what the caller provides)

```json
{
  "trajectory": {"bc": 0.232, "drag_function": "G7", "velocity_fps": 2800,
                 "range_band_yd": [0, 1000], "zero_yd": 100},
  "inputs": [{"name": "velocity_fps", "u": 3.0, "units": "fps",
              "provenance": "Garmin Xero C1 spec, rev C + indoor use"},
             {"name": "wind_speed_mph", "u": 2.5, "units": "mph",
              "provenance": "Kestrel 5700 + mirage judgment"}],
  "ratio_alpha": 0.1,
  "latency_ms": null
}
```

- Every `u` is REQUIRED with a `provenance` string. Missing `u` or missing
  provenance → refuse with the field named. No defaults, no presets in v1
  (presets arrive as a named-selected library later, never as silent values).
- `trajectory` selects the family/band row; unknown family or band →
  refuse with the covered-envelope list (fail-closed beats extrapolation).
- `ratio_alpha` defaults to 0.1 when absent (methodology default — ours to
  maintain, documented here, overridable). This is the ONLY default in the
  contract, and it is a statement about our conservatism, not about physics.
- `latency_ms` reserved, currently informational only (no fidelity toggles
  exist to trade against it; recorded for forward compatibility, ignored
  loudly — i.e. a non-null value warns "latency constraints not yet
  satisfiable", never silently ignored).

## 4. Output contract (what the tool returns)

```json
{
  "step_size_in": 288,
  "contributions": [{"input": "velocity_fps", "contribution_in": 0.26,
                     "share_pct": 44.1}],
  "yardstick": {"u_pred_in": 1.01, "epsilon_num_in": 7.1e-05,
                "ratio": 0.1, "u_c": null},
  "assumptions": ["independence across inputs", "linearity at operating point",
                  "single-platform calibration (x86_64-linux)"],
  "warnings": []
}
```

- `step_size_in`: unsigned integer inches, clamped to `[9, 288]` (floor/cap
  from the calibration plan; cap revisable only by the recorded promotion
  procedure, never by editing this file's intent).
- `contributions`: L1 shares (`|c·u|/Σ|c·u|`, percentages summing to 100 —
  display shares; variance combination underneath stays RSS per GUM).
- `yardstick.u_c` is ALWAYS null in v1 (no combined uncertainty is claimed;
  the field exists so the shape survives the day real budgets arrive).
- `assumptions` enumerates every load-bearing simplification, machine-
  readably. `warnings` is empty on the happy path; staleness/out-of-envelope
  are REFUSALS (no output document at all), not warnings — warnings cover
  advisory notes only (e.g. latency ignored).

## 5. Band vocabulary (shared with Phase 6)

`R_sig > 10` distinguishable / `0.1–10` marginal / `< 0.1`
indistinguishable — conventional reporting bands, not a project threshold;
no application decision may cite them without stating its own `k`. The tool
reports bands for the step it recommends (sanity: recommended steps should
land marginal-or-below on every channel — a distinguishable recommendation
means the inputs demand finer than the cap allows, escalate to explicit
`StepSize`, never silently emit).

## 6. Explicit non-goals (v1, binding)

No solver contact. No persistence (no saved profiles/sessions — v1 is
stateless: document in, document out). No per-shot adaptation (session
granularity reaffirmed). No fidelity toggles. No `u_c` claims. No silent
defaults. No second table schema for sensitivities.

## 7. Deferred, named (not forgotten)

Preset profile library (named/selected/versioned — the day typing sigmas
gets old). Python CLI twin (shares the golden vectors below, nothing else).
WASM solver exhibits (blocked on toolchain + API-stability call). Per-shot
adaptation (rejected for v1; needs its own proposal with a use case).
`latency_ms` enforcement (needs fidelity toggles that don't exist).

## 8. Conformance (how shells prove they implement THIS)

Static golden vectors (checked in with the tool, inputs → byte-exact
expected outputs, incl. one refusal case per fail-closed rule:
missing-u, unknown family, stale pin). Any shell — standalone page today,
overhauled docs tomorrow, Python CLI if ever — passes the same vectors.
A shell that can't consume this contract unchanged is non-conformant by
definition, not "a different interpretation."
