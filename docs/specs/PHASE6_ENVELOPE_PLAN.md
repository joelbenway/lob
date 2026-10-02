# Phase 6 — Signal Analysis, Envelope Claims & Technical-Reference Integration — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Systematize signal-versus-uncertainty comparisons (§13) into a full effect matrix with resolution-relative `R_sig` verdicts, publish the honest all-provisional envelope-claims document, close the three deferred items naming Phase 6 as consumer (spline mapping §8.6, duplicate-vector flag, `P(out-of-envelope)` tracking decision), integrate everything into the Doxygen reference (§23), and re-verify the regression tripwires — the final implementation phase of the framework.

**Architecture:** Paired-difference harness (same case+config, effect on/off, divide by the stated yardstick) in a new LOB-set test file for public-API effects; a static-only HELPER-set probe file for effects with no public knob (tolerance, lapse stepping, tilted-datum characterization, ceiling clamp); an env-gated offline driver running the full §13.1 list into `signal_{effect}_{case}.json`; a curated checked-in `envelope_claims.json`; Doxygen pointer edits plus the `wind_coriolis.md` rewrite. No solver changes, no new dependencies, no new runners.

**Tech Stack:** C++14, GTest/GMock 1.14 (existing), nlohmann/json (existing, test target only), CMake 3.14+, Doxygen markdown pages.

## Global Constraints

- C++14 only; no `std::filesystem`, no structured bindings, no NEW third-party dependency (nlohmann/json reuse only, `lob_test` target only — `lob_lob` stays zero-dependency).
- `lob_lob` library diff is empty: no `source/*.cpp|hpp` production change, no public API change.
- Public-API effects → `LOB_TEST_SOURCES`; internal-header probes (`solve_step.hpp`, `solve_angle.hpp`, `wind.hpp`, `splines.hpp`) → `HELPER_TEST_SOURCES` static-only (the Phase 1 Task 7 split, same rule). No `Impl`/`Pimpl` includes anywhere.
- Deterministic: fixed cases, fixed configs, paired solves share seeds; MC-σ yardstick uses fixed-seed runs only; no wall-clock, no threads, no file writes in CI tests.
- THE central honest-scoping rule: no `u_c` exists anywhere (Phase 4 all-incomplete, no §21-item-7 values). Every `R_sig` denominator is therefore `u_total(R) = sqrt(u_num² + granularity²)` (+ MC σ only where the channel is nonlinear-flagged AND an MC run exists — C1/C5). All verdicts are **resolution-relative** ("clears / is marginal against / is buried under the solver's own noise floor"), NEVER full prediction-uncertainty significance. Every artifact carries the yardstick field `{u_num_source, granularity, mc_sigma_or_absent, u_c: null}` stating exactly this. Claiming otherwise would be fabrication.
- No universal significance threshold (spec §12-decision, §13.2): verdict BANDS are `R_sig > 10` distinguishable / `0.1–10` marginal / `< 0.1` indistinguishable — order-of-magnitude reporting bands from §13.2's ≫/≈/≪ language, fixed in code as `kSigClear = 10.0` / `kSigMarginalLo = 0.1`, cited as conventional-not-prescribed. Verdict strings always carry band + yardstick description, never bare pass/fail. No application decision may cite them without stating its own `k`.
- Terminology gate (Appendix B): "validated" only with cell + case IDs; "error" only as `ε_num`/`δ`/residual; every claim cites artifact IDs.
- CI additions under 30 s; offline driver env-gated (`LOB_FULL_SIGNAL=1`, else `GTEST_SKIP`), consistent with the family.
- Plan location: `docs/specs/PHASE6_ENVELOPE_PLAN.md` (versioned with the other phase plans; commit in the docs pass).

---

## Scope

Phase 6 of `docs/specs/NUMERICAL_VALIDATION_SPEC.md` §20 ONLY: signal methodology (§13), envelope-claims document, §23 documentation integration, §19.8–19.10 acceptance slice. This phase ALSO closes, explicitly: (a) spline `±5e-3 → inches/MOA` mapping (§8.6, deferred in Phase 1b with "real consumer is Phase 6" in the ledger); (b) barometer≡weather-station duplicate flag (Phase 3 final-review affordance note); (c) `P(out-of-envelope)` tracking decision (Phase 5 handoff — resolved below with NO runner change: manifests already carry cell ids, so the matrix joins summaries to cells by existing ids; the runner stays envelope-agnostic by design); (d) `u_c_or_absent` spelling decision (Phase 4 handoff — DECIDED HERE: `null` stays, brief-sanctioned, twice-reviewed; recorded in the ledger, no code).

Deliberate deferrals (recorded so reviewers don't re-litigate):
- **No full-uncertainty `R_sig`.** Blocked on §21-item-7 human values. The `u_c: null` slot + yardstick field is the mechanical upgrade path.
- **No field/radar acquisition** (§21 item 11). Long-range cells stay unclaimed; the claims file renders the gap as data. If the human answers item 11 later, the claims doc gains rows — no code changes needed.
- **No per-effect application thresholds.** Bands reported, `k` left to the application (hunting-vs-ELR example stays in docs).
- **No `lob_env_test.cpp` dedup** (deferred since Phase 3 — stays deferred; one line in the claims limitations).
- **No canonical `inputs_hash`** (Phase 5 handoff). Signal artifacts record factory-name + solver-config + git SHA (the Phase 4 precedent: sufficient provenance without canonical hashing). Full canonical-hash work remains its own proposal.

---

## File Structure

```
test/
  CMakeLists.txt                      MODIFY: append validation_signal_test.cpp
                                      to LOB_TEST_SOURCES; append
                                      validation_signal_probes_test.cpp to
                                      HELPER_TEST_SOURCES (2 lines; json +
                                      cases-dir defines already on target)
  source/
    testing.hpp                       UNTOUCHED (diff helpers reused)
    validation_signal_test.cpp        CREATE: R_sig math + harness +
                                      CI smoke (lapse-convergence + jump-live
                                      + bit-identity oracle) + offline effect
                                      matrix driver (env-gated)
    validation_signal_probes_test.cpp CREATE (static-only): tolerance effect,
                                      lapse Fast-vs-Solve stepping, GetWind
                                      height-response characterization,
                                      ceiling-clamp verification
  validation/
    baselines/
      floors.json                     UNTOUCHED (u_num source)
      pareto.json                     MODIFY (data-only): add top-level
                                      `_reading_note` stating cutoff_90 is
                                      share-ranked regardless of genuine
                                      status (Phase 2 handoff; additive
                                      metadata only, all bytes otherwise
                                      identical — reviewer diffs this)
      envelope_claims.json            CREATE: curated cell verdicts +
                                      artifact links + limitations +
                                      human_signoff: pending (checked in,
                                      all-provisional)
docs/
  pages/validation/overview.md        MODIFY: envelope-claims section +
                                      coverage-matrix pointer
  pages/numerical_methods/ode.md      MODIFY: ladder + observed-order pointer
                                      (McCoy paragraph byte-intact)
  pages/numerical_methods/splines.md  MODIFY: 5e-3→inches mapping pointer
  pages/ballistic_model/drag.md       MODIFY: same mapping pointer (one line)
  pages/numerical_methods/zero_angle.md MODIFY: tolerance-tightening pointer
  pages/numerical_methods/inverse.md  MODIFY: Fast-vs-lapse branch pointer
  pages/api/*                         MODIFY only where a branch table exists
                                      to point at (else skip — no filler)
  pages/ballistic_model/wind_coriolis.md  MODIFY: rewrite vector section
                                      (wind_nodes[], direction+magnitude,
                                      1-ft/tilted-datum, cite don't copy
                                      WIND_INTERFACE_SPEC.md)
  pages/overview.md                   MODIFY: map link to validation spec
  specs/PHASE6_ENVELOPE_PLAN.md       THIS FILE
build/validation/                     GENERATED (gitignored):
                                      signal_{effect}_{case}.json
```

Why this shape: one LOB test file (harness + smoke + driver share the paired-difference core, Phase 3 colocation pattern); one static probe file (the Phase 1 Task 7 split rule — tolerance/lapse-tilted/ceiling have no public knob); claims curated-checked-in like floors/pareto; docs pointers except the wind page, whose content is stale (documents removed `ctx.wind` storage) and must be rewritten, not pointed at.

---

## Effect list (exact — spec §13.1 minimum set; isolation level stated per effect)

`Δ_effect(R) = y_with − y_without` at fixed case+config; yardstick per Global Constraints; band per range/channel; artifact `signal_{effect}_{case}.json` with the honesty header. Isolation levels: ISOLATED (only the effect differs) / JOINT (named confounds move together — reported as joint, never mislabeled) / BOUND (no toggle exists — upper-bound characterization, labeled as such):

1. `step-36-to-9` (C1, ISOLATED): expect ≪1 everywhere (Phase 1: 7e-05 in). The control — proves the harness sees "nothing" correctly.
2. `angle-tol` (C1, ISOLATED, STATIC probe via `SolveAngle`/`FastSolveAngle` tolerance params 0.01→0.001): expect ≪1 (Phase 1 tightening result, now in R_sig form).
3. `lapse-fast-vs-solve` (C9 ballistics, ISOLATED, STATIC probe: same initial state stepped with `FastSolveStep` vs `SolveStep` per `solve_step.hpp`, compare trajectories): expect small-but-nonzero (the path gate exists because it matters at range). CI smoke covers the public path-consequence (see Task 2).
4. `coriolis-on-off` (C7-style lat 45° × N/S/E/W, ISOLATED via omitting lat/az): expect ≫1 on deflection, ≈1-or-below on elevation.
5. `spin-litz-vs-off` + `spin-boatright-vs-litz` (C8 geometry, JOINT drift+jump by construction — the inputs that enable jump also enable drift; report both rows, never claim isolation): Litz-vs-off expect ≫1 on deflection (twist share genuine); Boatright-vs-Litz NO expectation (first gap measurement — report, don't predict).
6. `jump-on-off` (C8 crosswind vs wind-zeroed, JOINT with drift for the same reason; plus the analytic fact that zero crosswind ⇒ zero jump by `BuildLitzAerodynamicJump` early-out): expect ≫1 on elevation (jump −0.48 MOA measured). CI smoke covers the pair.
7. `single-bc-vs-bands` (C1 vs C10 band values, ISOLATED drag-source swap): expect ≫1 somewhere in 0–3000 ft (band spread 0.20–0.40 large by construction). (No floor-cell blocker: the yardstick is the C1 u_num, same solver path — recorded reasoning from planning.)
8. `g7-vs-g1` (C1 builder, ISOLATED): expect ≫≫1 — re-derived through the harness as the screaming-signal control (Phase 3: 401 in), not a new measurement.
9. `spline-pm-5e-3` (C1, ISOLATED via custom-table ±5e-3 Cd shift on the transonic band — public `MachVsDragTable` route, NOT direct `drags[]` edits which would break the builder contract): expect ≪1 or ≈1. CLOSES the §8.6 deferral — either outcome is a finding (≪1: table resolution comfortably below granularity; ≈1: earns a budget row). Commit message records the closure.
10. `profile-vs-uniform` (C6 stations at α=0, ISOLATED): expect ≪1 elevation, small-but-genuine deflection (5→10 mph station spread).
11. `shear-off-on` (C6, α 0 vs 0.25 same heights, ISOLATED): expect ≫1 deflection (Phase 2: shear genuine on deflection only), ≪1 elsewhere.
12. `measured-vs-nan-heights` (C6 α=0.25, ISOLATED): quantify, no direction predicted.
13. `tilted-vs-flat-datum` (C6b incline, BOUND): no flat-h solver path exists. Resolution: STATIC probe characterizing `GetWind` height-response (same downrange, several Y values → scaling curve) at incline vs flat + the gravity-corrected flat-fire comparison (CrosswindBlindToInclineAtSolve pattern); report as an UPPER BOUND on datum sensitivity, labeled bound-not-effect.
14. `ceiling-clamp` (C6c high-arc, BOUND): completion + sign oracle only (Phase 1 C6c pattern); no R_sig (a guardrail has no quantity). STATIC probe: `GetWind` at above-300-ft-equivalent states → verify clamped (wind stops growing).
15. `single-point-vs-uniform` (C5 one-station profile vs uniform setters, ISOLATED): expect BIT-IDENTITY — `EXPECT_DOUBLE_EQ` on all ranges, not R_sig (strongest oracle in the wind suite; 1 ulp fails loudly).

---

### Task 1: R_sig math + harness (no CI registration yet)

**Files:**
- Create: `test/source/validation_signal_test.cpp` (harness + math unit tests; registration in Task 2)

**Interfaces:**
- Consumes: stdlib + testing.hpp diff helpers + floors.json values (hardcoded with cross-links).
- Produces (used by Tasks 2, 3): `tests::EffectDelta` (paired vectors → per-range signed Δ), `tests::UTotal` (sqrt(u_num² + granularity²), optional mc_sigma param defaulting NaN→ignored), `tests::RSig` (|Δ|/u_total; u_total ≤ 0 → NaN verdict, never inf), `tests::SigBand` (`kDistinguishable/kMarginal/kIndistinguishable` at `kSigClear = 10.0` / `kSigMarginalLo = 0.1`, fixed literals CITED as conventional-not-prescribed).

- [ ] **Step 1: Create the test file with math unit tests on synthetic pairs (fails: harness undeclared — deferred-registration pattern, `g++ -fsyntax-only` RED).**
```cpp
TEST(SignalMath, DeltaOfKnownVectorsIsExact) { /* {..., -89.73} vs {..., -89.70} → Δ values */ }
TEST(SignalMath, UTotalFollowsHypotenuse) { /* sqrt(3²+4²) = 5.0 — the Phase 4 sqrt(52) tradition */ }
TEST(SignalMath, RSigGuardsZeroYardstick) { /* u_total 0 → NaN verdict, never inf */ }
TEST(SignalMath, BandsAtTenAndTenth) { /* 11.0→clear, 1.0→marginal, 0.01→indistinguishable */ }
```
(Full code at implementation time; values above are the contract.)
- [ ] **Step 2: RED proof** via syntax check (undeclared harness names).
- [ ] **Step 3: Implement harness above the tests** (~80 lines; ordered comparisons; quiet_NaN).
- [ ] **Step 4: GREEN proof** — syntax-check + standalone gtest link-run (established pattern). Full-suite proof waits for Task 2.
- [ ] **Step 5: Commit** — `git add test/source/validation_signal_test.cpp; git commit -m "test: add signal-versus-uncertainty harness with unit tests"`.

---

### Task 2: CI smoke — lapse-consequence + jump-live + bit-identity (first build integration)

**Files:**
- Modify: `test/source/validation_signal_test.cpp` (append smoke), `test/CMakeLists.txt` (1 line `LOB_TEST_SOURCES`)

**Interfaces:**
- Consumes: Task 1 harness, C9-tail floor context (quote in comments), C8-Litz survey builder values (mirror exactly, cite survey test).
- Produces: the CI gate for signal analysis.

Smoke (`SignalSmoke.*`, hermetic, in-memory, 36-in default step) — RECORD-don't-tripwire rule for physics outcomes (brief-mandated, reviewer-confirmed pattern): mechanism breakage FAILS, physics outcomes are RECORDED:
- `LapsePathConverged`: C9-tail-style context with forward drop asserted < −1200 (fail loud otherwise — path-engagement precondition, Phase 1 Task 5 pattern); forward-`Solve` vs `SolveInverse`-derived elevation compared; Δ + band RECORDED via SCOPED_TRACE-equivalent logging (mechanism: both solves complete; outcome: whatever R_sig results).
- `JumpIsLive`: C8 crosswind vs wind-zeroed; assert `aerodynamic_jump` nonzero vs zero (fail loud if the path goes quiet — the tripwire); Δ + band RECORDED.
- `SinglePointEqualsUniform`: one-station profile vs uniform setters → `EXPECT_DOUBLE_EQ` all ranges (THIS one trips — exact-identity oracle, effect 15).
- [ ] **Step 1: CMake line + smoke tests.**
- [ ] **Step 2: Build + run** — focused filter → PASS; first full-suite count recorded. Mechanism failure (path not engaged, jump unexpectedly zero) → NEEDS_CONTEXT with values.
- [ ] **Step 3: Commit** — `git add test/source/validation_signal_test.cpp test/CMakeLists.txt; git commit -m "test: add lapse/jump signal smoke with bit-identity oracle"`.

---

### Task 3: Static probes + offline effect matrix + spline closure + duplicate flag + claims doc

**Files:**
- Create: `test/source/validation_signal_probes_test.cpp` (static-only) + `test/validation/claims/envelope_claims.json`
- Modify: `test/source/validation_signal_test.cpp` (append env-gated driver), `test/CMakeLists.txt` (1 line `HELPER_TEST_SOURCES`)
- Generated (gitignored): `build/validation/signal_{effect}_{case}.json`

**Interfaces:**
- Consumes: Tasks 1–2, all 15 effects, floors (u_num), granularity, C1/C5 MC σ where nonlinear (read mc files if present, `mc_sigma_absent` otherwise — never fail for missing MC).
- Produces: per-effect JSONs (Δ/u_total/R_sig/band/yardstick-header per range/channel) + curated claims doc.

Probes (static-only, characterization with verdicts where applicable):
- Tolerance: `SolveAngle`/`FastSolveAngle` 0.01→0.001 on C1 @900 ft → R_sig (effect 2).
- Lapse stepping: `FastSolveStep` vs `SolveStep` trajectories from identical C9 initial state → per-range Δ (effect 3).
- GetWind height-response: same downrange × Y sweep at flat + incline 15° → scaling curves (effect 13 bound input).
- Ceiling: above-300-ft-equivalent states → clamped proof (effect 14).
Driver (`LOB_FULL_SIGNAL=1`, else `GTEST_SKIP`; existing `LOB_VALIDATION_DIR` — no CMake change beyond the HELPER line): all 15 effects × cases; spline effect 9 via custom-table ±5e-3 (CLOSES §8.6 — commit message says so); duplicate check byte-compares barometer vs weather-station expected[] (flag in claims limitations, vectors untouched); claims doc transcribed from the run (all `provisional`, `human_signoff: pending`, unclaimed list from Phase 3 matrix, limitations incl. duplicate + u_c-absent + single-platform + no-dedup notes).
- [ ] **Step 1: Implement probes + driver + run gated**, hand-inspect one screaming-signal + one floor-level JSON.
- [ ] **Step 2: Transcribe `envelope_claims.json`** (reviewed numbers, zero placeholders) + skip-by-default proof + `ctest -R Validation` green both ways. Static file MUST also run in the shared/static matrix sense — HELPER set is static-only by design (documented, like Phase 1 Task 7).
- [ ] **Step 3: Commit** — `git add test/source/validation_signal_probes_test.cpp test/source/validation_signal_test.cpp test/validation/claims/envelope_claims.json test/CMakeLists.txt; git commit -m "test: add offline signal matrix with envelope claims (closes 8.6 spline mapping)"`.

---

### Task 4: Doxygen integration + pareto reading note

**Files:** all under `docs/pages/` (8 files) + `test/validation/baselines/pareto.json` (one additive `_reading_note` key — reviewer diffs bytes-otherwise-identical).

- [ ] **Step 1: Pointer edits** per File Structure (McCoy paragraph byte-intact; citations with test + artifact IDs; evidence-before-synthesis voice; no unsupported claims; api/* only where tables exist).
- [ ] **Step 2: wind_coriolis.md rewrite** (vector section → wind_nodes[] + direction+magnitude + 1-ft/tilted-datum; cite WIND_INTERFACE_SPEC.md, don't copy; heading convention + limits stay).
- [ ] **Step 3: pareto `_reading_note`** — `"cutoff_90 is share-ranked regardless of genuine status; join with per-driver status before treating a cutoff driver as informative."` Byte-diff the file to prove nothing else changed.
- [ ] **Step 4: Doxygen build check** if toolchain present (else CI docs job covers it — state which in report). Commit — `git add docs/pages/ test/validation/baselines/pareto.json; git commit -m "docs: integrate validation methodology into technical reference"`.

---

### Task 5: Full gate + lint + acceptance snapshot

**Files:** none (verification only, unless the gate finds regressions).

- [ ] **Step 1: Full local gate** — `cmake --build --preset=dev && ctest --preset=dev --output-on-failure -j 4` → 100% PASS (signal driver SKIPPED); `LOB_FULL_SIGNAL=1` focused run → PASS + artifacts.
- [ ] **Step 2: Linters** — format + spell per HACKING.md; cppcheck on touched files; tidy exit 0 on both new files (Definition of Done). Fix in place, `style:` commit if needed.
- [ ] **Step 3: Regression tripwire re-verification** (§19.8): the Phase 1 ceiling tests still trip on synthetic ε-inflation — prove by temporarily forcing a coarse step in a scratch (uncommitted) run? NO — scratch-modifying tracked files risks accidental commit. Resolution: assert by code reading that the ceiling asserts remain in place + cite the last green run; the tripwire is structural (ceilings + monotone asserts), not a separate test. Record this reasoning — do NOT hack the tree to "test the test".
- [ ] **Step 4: Acceptance snapshot** (§19.8–19.10 + framework roll-up): regression (tripwires structurally in place, suite green), docs (all §23 pages + no unsupported claims + envelope page with cells/verdicts/links), CI/perf (total added gate time summed across phases < 3 min per-phase-budget reading — measure and report; no core dep; purity grep re-run clean; benchmark shape unchanged). Commit nothing (or the Step-2 style commit).

---

## Self-Review

**1. Spec coverage (§13 / §19.8–19.10 / §20 Phase 6 row / §23):**
- §13.1 all 15 effects with isolation levels → Tasks 2–3 (13 paired, 14 oracle, 15 identity; joint/bound labeled, never mislabeled).
- §13.2 bands without global k → Task 1 edges (10.0/0.1, conventional-not-prescribed, yardstick-carrying verdicts).
- §13.3 engineering output (Δ/R_sig/verdict/artifacts + keep/drop/envelope-restrict reading guidance) → Task 3 JSONs + claims (verdicts read as resolution-relative; drop/keep language appears in docs as application guidance with the k-caveat, never as project verdicts — CHECK: §13.3 says verdict (keep/drop/envelope-restrict). Resolution: per-effect rows carry band + a `reading` string suggesting the engineering question ("below resolution — complexity unjustified ON THIS ENVELOPE"), stopping one step short of a project verdict. Reviewer confirms the line holds.
- §19.8–19.10 → Task 5 (tripwire reasoning + docs + CI/perf measurement).
- §20 Phase 6 row → files (Doxygen + claims + smoke)/deps (human sign-off field)/tests/artifacts/CI (smoke only)/cost (small) covered.
- §23 docs list → Task 4 (all pages + wind rewrite + map link-not-include).
- Deferred closures → Task 3 (spline §8.6 with commit-message record; duplicate flag in claims limitations).
- Phase 2/4/5 handoffs → Task 4 pareto note; u_c spelling DECIDED (null stays, ledger record, no code); P(out-of-envelope)/inputs_hash explicitly NOT in Phase 6 (runner-agnostic matrix join by existing cell ids needs no new code — the "tracking" is the claims doc's per-cell run-manifest links; reviewer confirms this reading satisfies the handoff).
- §21: item 7 stays open (claims `human_signoff: pending` is the formal handoff); item 11 stays open (long-range unclaimed); item 12 upheld (no global k); item 6 cells decided long ago.
- GAP CHECK: Task 2's record-don't-tripwire vs "CI gates should trip" — resolved: bit-identity + mechanism asserts trip; physics outcomes record. The line is drawn in-task; reviewer re-confirms.
- GAP CHECK 2: static probes file runs ONLY in static CI (HELPER set) — the tolerance/lapse-tilted/ceiling evidence therefore has no shared-build coverage. Accepted (same as Phase 1 Task 7, documented in-task); the LOB-set smoke covers the public consequences (lapse-consequence, jump-live) on all legs.

**2. Placeholder scan:** no TBD/TODO/later in deliverables; `u_c: null` + `human_signoff: pending` are schema states with named consumers; claims numbers from the real run.

**3. Type consistency:** `EffectDelta/UTotal/RSig/SigBand` fixed in Task 1 tests; `LOB_FULL_SIGNAL` joins the env-gate family (`LADDER/PARE/TO/MATRIX/BUDGET/SIGNAL` — six gates, consistent naming); `signal_{effect}_{case}.json` matches spec §20 exactly; claims under `test/validation/claims/` (new dir rationale recorded: curated verdicts ≠ transcribed cases ≠ measured baselines ≠ human manifests).

---

*End of plan. This is the final implementation phase. Remaining work after Phase 6: human decisions (§21 items 7, 11), merge strategy for the branch, and the docs/specs/ commit pass.*
