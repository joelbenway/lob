# Phase 5 — Monte Carlo Propagation — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build the desktop-only Monte Carlo runner (`tools/lob_mc/`) that samples input distributions through the unchanged deterministic solver and emits output distributions with convergence-checked summaries — plus a CI smoke test proving the seeded-sampling method — while the embedded core gains zero lines, zero dependencies, zero threads.

**Architecture:** New `tools/lob_mc/` client program (own `add_subdirectory`, developer-mode-gated like `example/` and `benchmark/`) links installed `lob::lob` and reads a run-manifest JSON (dimensions from `pareto.json` genuine + nonlinear flags, distribution families from `budget_template.json` rows). Sampling core: `std::mt19937_64` + Splitmix64-derived worker subseeds, `<random>` distribution types only, embarrassingly parallel across samples. CI smoke is self-contained inline sampling in the test file (no test→tools coupling — rationale below). Offline runs write summaries + branch counters + Spearman cross-checks vs the Phase 2 Pareto.

**Tech Stack:** C++14, `std::thread` + `<random>` (tool only), nlohmann/json (tool's manifest I/O — same existing dependency), GTest (smoke only), CMake 3.14+.

## Global Constraints

- C++14 only; no `std::filesystem`, no structured bindings, no NEW third-party dependency (nlohmann/json + GTest reuse only).
- `lob_lob` library diff is empty AND stays thread/RNG/heap-free: no `<random>`, no `<thread>`, no I/O, no exceptions, no allocation in `source/` or `include/` — the tool is a CLIENT of the installed library, never a modification of it. The final review verifies this with a grep-level source guard.
- The tool builds ONLY in developer mode (`option(BUILD_TOOLS ... "${LOB_DEVELOPER_MODE}")` + `add_subdirectory(tools)`, mirroring `BUILD_EXAMPLES`/`BUILD_BENCHMARKS` in the top-level `CMakeLists.txt`). Consumers (FetchContent, installs) never compile it, never link it, never see its threads.
- Canonical MC defaults (spec §21 item 4, locked by this plan — amend by human only): pilot `N=1024`, CI smoke `N=256` fixed seed, `E_target = 0.1σ` for mean half-width, `std::mt19937_64` + splitmix64 subseeds (`subseed_w = Splitmix64(seed ^ (w + 1))`), distributions via `<random>` types only (`normal_distribution`, `uniform_real_distribution`, triangular via two-uniform difference — no hand-rolled RNG math).
- Sampling happens in BUILDER INPUT SPACE (spec §12.1, §21 item 14 as decided): speed magnitudes + heading degrees (wrapped normal reduced mod 360, never Cartesian x/z), heights jittered only when `α ≠ 0`, shear exponent sampled on `[0,1]` truncated, BC lognormal-or-normal per future supplier evidence (template default normal, human-confirmed). `WindSpeedMph` magnitude-only + heading-pair rule from Phase 2 applies per sample.
- Deterministic components (solver config, drag tables, constants, envelope tags) fixed per run and hashed into the run ID; stochastic components are ONLY the declared draws. One sample = one `Build + Solve(+Inverse)` through the public C++ API. Build failures (OOR draws) counted + reported, never silently dropped (resample cap 100, then fail the sample loudly per §12.1).
- No `rand()`, no time seeds, no thread-local unsynchronized engines, no reseed-per-sample. Worker count must not change the sequence (pre-derived subseeds).
- CI additions under 30 s; offline runs minutes–hours, threaded, desktop-only.
- Plan location note: `docs/superpowers/plans/` is gitignored in this repo, so this plan lives versioned at `docs/specs/PHASE5_MONTECARLO_PLAN.md`.

---

## Scope

Phase 5 of `docs/specs/NUMERICAL_VALIDATION_SPEC.md` §20 ONLY: MC methodology (§12), its CI smoke (§14.3, §17), runner placement (§12.4, §18.2), and the §19.6 acceptance slice.

Deliberate deferrals (recorded so reviewers don't re-litigate):
- **No real-u(x) production runs.** Manifests ship with the template's TBD-derived illustrative distributions CLEARLY LABELED synthetic (see Task 2) — the runner's first honest job is demonstrating scale, convergence behavior, and the Spearman-vs-Pareto cross-check on the C1 cell, not publishing uncertainties. Real sensor distributions await the §21-item-7 human values (same gate as Phase 4).
- **No adaptive/importance sampling.** Plain Monte Carlo with pilot-then-scale sizing only. Variance-reduction techniques are a follow-up proposal with their own bias analysis, not this phase.
- **No Python plotting helper** (spec §15 mentions an optional ≤200-line helper). CSV+JSON outputs are plottable by hand; the helper arrives only if a human asks.
- **No in-library API.** No `LobSample`, no batch-solve entry point, no RNG hooks in the core — the runner loops over the existing public API. Throughput comes from threads outside the library, not new surface inside it.

---

## File Structure

```
CMakeLists.txt                        MODIFY: BUILD_TOOLS option +
                                      add_subdirectory(tools) (mirrors
                                      examples/benchmarks blocks)
tools/
  CMakeLists.txt                      CREATE: lob_mc target, C++14, links
                                      lob::lob + nlohmann_json, threads
                                      (Threads::Threads), developer-mode only
  lob_mc/
    main.cpp                          CREATE: arg parse (--manifest, --seed,
                                      --samples, --workers, --out-dir),
                                      run orchestration, exit codes
    sampler.hpp                       CREATE: splitmix64, subseed derivation,
                                      distribution adapters (normal/uniform/
                                      triangular/fixed/categorical-scenario),
                                      truncation + resample-cap logic
    case_adapter.hpp                  CREATE: manifest dimension → Builder
                                      mutations per sample (scalar setters,
                                      integer-snapped velocity, profile
                                      station heading/speed pairs, shear,
                                      heights-iff-sheared) + Solve/SolveInverse
                                      invocation + branch counters
                                      (fall-short/tumble/cap-hit/path-switch)
    summary.hpp                       CREATE: running mean/σ + batch-means SE,
                                      P2.5/P5/P50/P95/P97.5 with binomial CI
                                      widths, Spearman rank correlations,
                                      multimodality/branch-split summaries,
                                      JSON+CSV writers (reuse §15.2 header)
test/
  CMakeLists.txt                      MODIFY: append validation_mc_smoke_test.cpp
                                      to LOB_TEST_SOURCES (1 line; no new
                                      defines, no new deps)
  source/
    validation_mc_smoke_test.cpp      CREATE: self-contained N=256 fixed-seed
                                      inline sampler (no tools/ coupling) +
                                      integrity + schema asserts
docs/
  pages/validation/overview.md        MODIFY: append 3-line MC pointer
                                      (no numbers)
  specs/PHASE5_MONTECARLO_PLAN.md     THIS FILE
build/validation/                     GENERATED (gitignored): mc_run_*.json +
                                      samples.csv(.gz at N≥10⁴)
```

Why this shape: `sampler`/`case_adapter`/`summary` split by responsibility (RNG math / solver contact / statistics — each independently unit-testable in principle, though Phase 5 tests them through the smoke + gated runs rather than a new unit suite, per YAGNI: the statistics are textbook formulas verified against hand computations in the smoke test). The CI smoke duplicates ~50 lines of sampling glue inline INSTEAD of including `tools/` headers — deliberate: test→tools coupling would drag the threaded runner's build graph into every CI configuration including shared/static matrix legs; the smoke proves the *method* (seeded draws → public API → summaries), the tool proves *scale*; both pin the same RNG + subseed constants, and the Task 3 review diffs those constants across the two files.

---

### Task 1: Runner skeleton + sampler (no solver contact yet)

**Files:**
- Create: `tools/CMakeLists.txt`, `tools/lob_mc/main.cpp`, `tools/lob_mc/sampler.hpp`
- Modify: top-level `CMakeLists.txt` (BUILD_TOOLS block mirroring examples/benchmarks)

**Interfaces:**
- Consumes: `<random>`, nlohmann/json (manifest stub: seed + samples + workers only at this stage).
- Produces (used by Tasks 2, 3): `mc::Splitmix64(uint64_t)`, `mc::Subseed(seed, worker)`, `mc::SampleNormal/Uniform/Triangular/Fixed` adapters with truncation `{lo, hi, max_retries=100}` semantics, CLI skeleton with `--help` + exit codes (0 ok, 2 usage, 3 build-fail-overflow — mirroring `lob_bench.cpp`'s exit-3 convention for broken chains).

Sampler semantics (exact, spec §12.2):
- `Splitmix64`: the standard Steele et al. sequence (x += 0x9E3779B97F4A7C15; z = (x ^ (x>>30)) * 0xBF58476D1CE4E5B9; z = (z ^ (z>>27)) * 0x94D049BB133111EB; return z ^ (z>>31)).
- `Subseed(seed, w) = Splitmix64(seed ^ (w + 1))` (w+1 so worker 0 still mixes).
- Truncation: draw-then-check against `[lo,hi]`; resample up to 100; then return `{ok=false}` and the CALLER counts the failed sample (never infinite-loop, never clip-silently — clip-and-record is a manifest-level opt-in, default resample).
- Triangular(a,c,b): `u1+u2` difference method via two `uniform_real_distribution(0,1)` scaled — no custom PDF math.
- Categorical scenarios (G-curve weights etc.): weighted index draw via `discrete_distribution` — scenario selection only, never continuous perturbation.

- [ ] **Step 1: Top-level CMake edit + tools skeleton + sampler header** (no test file yet — this task is tool-side; verification is build + `--help` + a `--selfcheck` mode).
- [ ] **Step 2: `--selfcheck` deterministic proof** — `main` supports `--selfcheck` which draws 1024 normals at fixed seed through 1 vs 4 workers and asserts identical sequences (worker-count independence) + prints Splitmix64(0) first output for the record. Run: `./build/dev/tools/lob_mc/lob_mc --selfcheck` → PASS. This is the reproducibility contract made executable.
- [ ] **Step 3: Commit**

```bash
git add CMakeLists.txt tools/
git commit -m "tool: add lob_mc skeleton with seeded sampler and selfcheck"
```

---

### Task 2: Case adapter + run manifests (solver contact, synthetic distributions)

**Files:**
- Modify: `tools/lob_mc/` (+ `case_adapter.hpp`, extend `main.cpp` run path)
- Create: `tools/lob_mc/manifests/c1_smoke.json` (N=256, seed fixed, SYNTHETIC distributions labeled `"synthetic_illustrative_only": true`), `tools/lob_mc/manifests/c1_pilot.json` (N=1024 + C5/C8 dimension sets)

**Interfaces:**
- Consumes: Task 1 sampler, public `lob::Builder`/`Solve`/`SolveInverse`, pareto.json driver lists (dimension selection), budget_template rows (distribution families).
- Produces (used by Task 3): per-sample `mc::TrajectorySample { outputs, branch_flags, draws[] }` + run loop with worker pool + per-sample OOR accounting.

Adapter rules (spec §12.1 + Phase 2 patterns, exact):
- Scalar setters: clone base builder per sample (cheap — no heap), apply drawn value, `Build()`; OOR `ctx.error` → count `build_failed`, continue (never abort the run).
- Velocity: `llround` + `uint16_t` cast (Phase 2 integer rule).
- Wind at zero baseline: draw speed magnitude + heading jointly — heading drawn from wrapped normal around baseline, reduced mod 360 into `WindHeadingDeg`; NEVER negative speed.
- Profile stations (C6-style): per-station speed/heading draws with STATED cross-station correlation flag (default independent with the flag recorded; correlated sampling needs a covariance input that doesn't exist — fail the manifest loudly if `correlation: "correlated"` appears without a matrix, per fail-closed).
- Heights drawn ONLY when manifest shear `alpha != 0` (inert-at-α0 pruning, Phase 2 rule); shear sampled on `[0,1]` truncated.
- BC: normal around band value, truncated positive (lognormal upgrade deferred to human supplier evidence).
- Categorical: G-curve choice as weighted scenario per manifest weights (default weight 1.0 on the case's own curve — i.e. degenerate unless a study says otherwise).
- Branch counters per sample: `reached_all_ranges` (else fall-short index), `tumble_hit`, `angle_cap_hit` (SolveAngle NaN → inverse prefix-short), `density_path` taken. Nonzero branch counts split downstream summaries (spec §12.3 — pooled stats across branches forbidden).

Manifest schema: `{run_id, cell, seed, samples, workers, solver_config{...}, dimensions: [{input, family, params, truncation, baseline}], scenarios: [...], synthetic_illustrative_only: bool}`. The smoke manifest's flag is `true` with a comment string naming the §21-item-7 gate for real values.

- [ ] **Step 1: Implement adapter + manifests + run path** (worker pool via `std::thread`, pre-derived subseeds, deterministic join order — results assembled by sample index, never completion order).
- [ ] **Step 2: Determinism proof** — same manifest + seed at 1 vs 4 workers → byte-identical `samples.csv` (diff, not eyeball). Record the command pair + `cmp` result in the report.
- [ ] **Step 3: Commit**

```bash
git add tools/
git commit -m "tool: add case adapter with synthetic run manifests"
```

---

### Task 3: Summaries + CI smoke (method proof)

**Files:**
- Create: `tools/lob_mc/summary.hpp` (statistics + writers)
- Create: `test/source/validation_mc_smoke_test.cpp` + CMake 1-line registration
- Modify: `tools/lob_mc/main.cpp` (emit summaries + CSV)

**Interfaces:**
- Consumes: Task 2 samples; Phase 2 Pareto shares (Spearman cross-check target); §15.2 provenance header pattern.
- Produces: `mc_run_{id}.json` + `samples.csv` (+ `.gz` suggestion at N≥10⁴ — shell gzip, not a library), CI gate.

Summary semantics (exact, spec §12.3):
- Running mean/σ (Welford, double precision) + batch-means SE with B=32 batches (fewer batches than samples → B=min(32, N/8), documented).
- Percentiles P2.5/P5/P50/P95/P97.5 via order statistics (nth_element, deterministic) + binomial CI half-widths `1.96·sqrt(p(1−p)/N)` reported alongside (tails beyond P1/P99 reported "unresolved" unless N≥10⁵ — a string verdict, not a number).
- Mean-converged verdict per primary output: `SE < 0.1·σ` (E_target default).
- Spearman rank correlation per (sampled input × primary output) — cross-checked against Phase 2 Pareto ordering in the Task 4 report (same top driver = method agreement; disagreement is a finding, not a failure — different methods see different projections).
- Branch-split: any nonzero branch counter → summaries computed per branch + pooled explicitly labeled `pooled_across_branches: true` with a DO-NOT-USE warning string (forbidden for claims, present for diagnostics).

CI smoke (`MonteCarloSmoke.FixedSeedIntegrity`, N=256, seed `0xC10CA1` — hex pun on the C1 cell, documented):
- Self-contained inline sampler (~50 lines: mt19937_64 + normal/uniform draws, NO tools/ headers) driving C1-ICAO velocity (±10 normal) + wind-speed (half-normal-ish via abs(normal), heading fixed 90°) through public `Solve` at {900, 1800}.
- Asserts: solve count == 256 (zero build failures on this benign box); mean elevation within 3·SE_pilot of the deterministic baseline (integrity, not accuracy — proves sampling didn't break the solver path); summary JSON shape keys present (mean/sd/n/seed/P5/P50/P95); Spearman(velocity, elevation) > 0.9 in magnitude with consistent sign (velocity dominates — Phase 2 agreement); deflection mean within 3·SE of 0 baseline calm... precisely: wind_speed drawn positive-only at heading 90° gives POSITIVE deflection mean — assert mean > 0 with SE width (directional integrity proves the wind path is live, not zeroed).
- Hermetic, in-memory (no file I/O, no env reads), < 30 s (256 solves ≈ milliseconds).

- [ ] **Step 1: Implement summary.hpp + wire into main + write smoke test + CMake line.**
- [ ] **Step 2: Build + run** — focused filter → PASS; first full-suite count recorded. On Spearman/sign failure: STOP, NEEDS_CONTEXT (sampling broke the physics path — structural news).
- [ ] **Step 3: Commit**

```bash
git add tools/ test/source/validation_mc_smoke_test.cpp test/CMakeLists.txt
git commit -m "test: add MC summaries with fixed-seed integrity smoke"
```

---

### Task 4: Pilot-then-scale demonstration + cost model (offline)

**Files:**
- Modify: `tools/lob_mc/main.cpp` (pilot-then-scale sizing: pilot N=1024 → estimate σ → scale to `N ≥ (2·σ/E_target)²`... precisely: `(z·σ/E)²` with z=2 default approximating 95%, E=E_target=0.1σ default → N≈400 minimum for means; percentiles need N≥10⁴ for stable P5/P95 — the tool prints the recommended N per output and runs it on `--auto-scale`).
- Generated (gitignored): `build/validation/mc_run_c1.json`, `mc_run_c5.json`, `samples.csv`

**Interfaces:**
- Consumes: Tasks 1–3, C1/C5 manifests.
- Produces: the §19.6 evidence (pilot→scale selection, batch-means SE, percentiles with widths, branch-split summaries, Spearman-vs-Pareto agreement) + measured `cost_single` (µs/solve, from `std::chrono::steady_clock` around the solve loop, reported per run).

Run plan (offline, threaded): C1 pilot 1024 → auto-scale → full run (expect N≈10⁴ for percentile stability, minutes single-threaded / seconds threaded); C5 same; record wall times + `cost_single`; verify Spearman top-driver == Pareto top-driver per cell; verify mean within E_target of the deterministic baseline solve (sanity: symmetric inputs → mean ≈ deterministic); record all of it in the report. If wall time exceeds 30 min single-threaded, cap at N=10⁴ with the cap recorded (percentile widths widen honestly — never silently).

- [ ] **Step 1: Implement auto-scale + run C1/C5 pilots and full runs.**
- [ ] **Step 2: Determinism re-proof at scale** (1-vs-N-workers byte-identical CSV on the pilot).
- [ ] **Step 3: Commit** (tool changes only; `build/` untracked)

```bash
git add tools/
git commit -m "tool: add pilot-then-scale sizing with cost reporting"
```

---

### Task 5: Docs pointer + full gate + lint

**Files:**
- Modify: `docs/pages/validation/overview.md` (append 3-line MC pointer under the budget paragraph — method pointer, no numbers)

Pointer text (verbatim):

```markdown
Monte Carlo propagation samples input distributions through the unchanged
deterministic solver (`docs/specs/NUMERICAL_VALIDATION_SPEC.md` §12); the
desktop-only runner lives in `tools/lob_mc/` (never the embedded core) with
a fixed-seed integrity smoke in `test/source/validation_mc_smoke_test.cpp`.
Full runs are offline, threaded, and reproducible by seed; distributions are
synthetic until human-supplied sensor evidence arrives.
```

- [ ] **Step 1: Append pointer.**
- [ ] **Step 2: Full local gate** — `cmake --build --preset=dev && ctest --preset=dev --output-on-failure -j 4` → 100% PASS (all env-gated drivers SKIP by default); `LOB_FULL_BUDGET=1` + `LOB_FULL_MATRIX=1` + tool selfcheck still green (no regressions in prior phases).
- [ ] **Step 3: Linters** — format + spell per HACKING.md; fix in place, `style:` commit if needed. PLUS the grep-level core-purity guard (new in this phase, then permanent): `grep -rn "random\|<thread\|mutex\|fopen\|ifstream\|ofstream\|new \|delete\|throw" source/ include/ | grep -v "^.*test"` must print nothing beyond pre-existing reviewed lines — record the output (or its emptiness) in the report. If the guard finds a violation, that is a BLOCKING finding (core contamination), not a nit.
- [ ] **Step 4: Acceptance snapshot** (§19.6): fixed-seed N=256 smoke bit-identical on one platform; pilot→scale selection demonstrated; batch-means SE + percentiles with widths; branch-split summaries; Spearman-vs-Pareto agreement shown; `lob_lob` diff empty AND core-purity grep clean. Commit docs.

```bash
git add docs/pages/validation/overview.md
git commit -m "docs: point validation page at Monte Carlo methodology"
```

---

## Self-Review

**1. Spec coverage (§12 / §14 / §15 / §17 / §19.6 / §20 Phase 5 row):**
- §12.1 distributions + determinism contract (families, truncation+resample-cap, input-space sampling, fixed-config run ID, OOR counting) → Tasks 1–2 (categorical-scenario weights included; correlated-profiles fail loudly without a matrix).
- §12.2 sampling/RNG/seeding/reproducibility (mt19937_64, subseeds, worker-count independence, no time seeds) → Task 1 selfcheck makes it executable; Task 2/4 re-prove at scale.
- §12.3 MC convergence + summaries (batch-means SE, E_target, percentiles + binomial widths, tail-unresolved rule, branch-split, Spearman) → Task 3 (summary.hpp) + Task 4 (demonstration).
- §12.4 cost/parallel/placement (linear cost, threaded desktop-only, tools/ not core, measured cost_single) → Tasks 2/4 + BUILD_TOOLS gating + Task 5 purity grep.
- §14 test arch (desktop tool + CI smoke separation, no nondeterministic CI) → smoke is fixed-seed integrity-only with 3·SE bounds (spec §14.3 rule honored).
- §15 artifacts (run manifests versioned, summaries with provenance, samples.csv) → manifests checked in under tools/, outputs gitignored with §15.2 headers.
- §17 CI (smoke only, fixed seed, minutes-scale offline) → Task 3 hermetic + Task 4 offline.
- §19.6 acceptance → Task 5 snapshot (all seven clauses: bit-identical smoke, pilot→scale, SE, percentiles, branch-split, Spearman-vs-Pareto, empty core diff — plus the purity grep as extra).
- §20 Phase 5 row → files (tools/lob_mc/, smoke test, templates)/deps (none new)/tests/artifacts/CI/cost all covered.
- §21 items 3+4 (runner home + MC defaults) → locked by this plan's Global Constraints; human approves by approving the plan. Item 9 (categorical weights) → degenerate-default rule (weight 1.0 on own curve) means no weights need approval until a study uses them. Item 14 (wind sampling) → input-space rule already decided, implemented in Task 2.
- GAP CHECK: §12.3 "P(out-of-envelope)" and "P(build-fail)" in summaries — Task 3 list omits them (has branch counters but not the probability readouts). FIX: summary.hpp emits `p_build_fail = fails/N` and `p_branch` per branch counter as probabilities (they're just normalized counters — no new machinery). Implementer adds both fields; reviewer checks presence. (Recording here so it rides with Task 3.)
- GAP CHECK 2: §12.2 "seed + distribution JSON + code version + solver config fully determine the run" — run manifest needs a `lob_version` + `git_sha` capture at runtime (tool reads `lob::Version()` + bakes `LOB_GIT_SHA`-style define like the test target has). FIX: tools/CMakeLists wires the same git-sha execute_process block as test/CMakeLists (copy the pattern); manifest echo includes both. Rides with Task 1.

**2. Placeholder scan:** no TBD/TODO/later in deliverables; `synthetic_illustrative_only: true` + comment is a labeled-synthetic flag with a named unblocking gate (§21 item 7), not a placeholder; manifests' TBD-derived distributions inherit the template's fail-closed vocabulary by reference (the runner never reads budget_template.json directly — dimensions come from pareto + manifest params, stated to avoid a phantom dependency).

**3. Type consistency:** `Subseed(seed, w)` signature fixed in Task 1 and reused in Tasks 2/4 (reviewers check the call sites match); exit codes 0/2/3 consistent with `lob_bench.cpp` precedent; `LOB_FULL_BUDGET`/`LOB_FULL_MATRIX`/`LOB_FULL_PARETO`/`LOB_FULL_LADDER` env-gate family consistent; `LOB_GIT_SHA` define pattern copied (not reinvented) for the tool target; smoke-test RNG constants (`0xC10CA1` seed, N=256) fixed in Task 3 and never redefined elsewhere.

---

*End of plan. Phase 6 (technical-reference integration + envelope reporting) plan is written after Phase 5 runs merge, per spec Appendix A.*
