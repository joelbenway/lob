# Step-Calibration Experiments — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to execute these experiments task-by-task. Each experiment is one task unless noted.

**Goal:** Produce the four measurements that gate the information-aware step-size design (spec(Point 4–5 of the design review): R-flatness, upward ladder, cost split, storage headroom), each with **pre-registered veto conditions** — any red result stops the line before the next experiment spends time.

**Architecture:** No library changes in any experiment. E0 is a compile-time probe (reverted immediately). E1 is offline analysis over existing artifacts, re-running existing gated drivers only if per-range data is missing. E2 is profiling via existing benchmark/cachegrind infrastructure plus a scratch timing driver (uncommitted, `/tmp`). E3 extends the existing ladder driver upward. Results land in the ledger, experiment reports, and (E3 only) one new checked-in data file.

**Tech Stack:** C++14, existing gated test drivers, python3 for analysis, cachegrind/perf as available, CMake 3.14+.

## Global Constraints

- `lob_lob` library diff stays empty (experiment-only changes: throwaway probes reverted, driver extensions test-only, one new data file in E3).
- No new dependencies. No ABI changes. No behavior changes to shipped code paths.
- Every experiment records: command(s) run, raw numbers, verdict vs the pre-registered criterion below, commit SHA(s) if any. A result without a recorded verdict is unfinished.
- The veto rule: a red criterion STOPS the line. Do not proceed to the next experiment "to see anyway" without explicit human override — the ordering exists so cheap kills happen before expensive campaigns.
- Plan location: `docs/specs/STEP_CALIBRATION_EXPERIMENTS.md` (versioned with the other plans; commit in the next docs pass).

---

## Dependency chain

```
E0 (sizeof probe, minutes)
 │  gates: in-Builder vs sidecar policy storage
 ▼
E1 (R-flatness, hours, zero new solves) ──┐
 │  gates: coarse-band vs exact-range vs Solve-time architecture
 ▼                                          │ E0+E1+E2 mutually independent —
E2 (cost split, ~day) ─────────────────────┘ run in any order / parallel.
 │  gates: the economics (speed story lives or dies here)
 ▼
E3 (upward ladder + tier calibration, days)
    gates: coarsening safety, max-step cap, tier widths.
    Needs E2's verdict for interpretation + E1's architecture for tier mapping.
```

---

## Scope

These four experiments ONLY, plus the explicit dead list. Out of scope: fidelity cost attribution (killed with step-only-forever), Build-time live-solve timing (no live solves in v1), standalone tier-guess validation (covered by E3 acceptance), any implementation of the step policy itself (that work starts only after all four are green).

---

### E0: `sizeof(Impl)` headroom probe

**Gating question:** does an ~8–16-double uncertainty override list fit in `LobBuilder`'s 320-byte opaque buffer, or must the policy be a sidecar struct?

- [ ] **Step 1: Probe.** Temporarily append `static_assert(sizeof(Impl) <= 0, "probe");` to `source/lob_builder.cpp`, run `cmake --build --preset=dev --target lob_test`, read the actual byte count from the compiler error. Revert immediately (verify with `git diff --stat` showing clean).
- [ ] **Step 2: Verdict.** Headroom = 320 − sizeof(Impl). Policy need ≈ 64–128 bytes (8–16 doubles + mode/flags). Headroom ≥ 128 → in-Builder possible; < 64 → sidecar mandatory; in between → sidecar recommended (no margin for future Builder growth).
- [ ] **Step 3: Record** verdict + numbers in the ledger. No commit (probe reverted; nothing to commit).

**Veto condition:** none — this experiment cannot kill the feature (sidecar always works). It routes storage design only. Effort: minutes.

---

### E1: R-flatness from existing artifacts (zero new solves)

**Gating question:** is the error-to-uncertainty ratio approximately range-flat, deciding Build-time (coarse band) vs exact-range vs Solve-time architecture?

- [ ] **Step 1: Inspect for per-range data.** Check `test/validation/baselines/floors.json` + `build/dev/validation/convergence_*.json` for per-range 18→9 deltas per channel (C1/C5), and `pareto.json` for per-range canned responses. If per-range deltas exist → proceed to analysis. If only worsts → re-run the existing ladder driver with per-range logging (minor test-only extension behind the existing env gate; same patterns as Phase 1b), then proceed.
- [ ] **Step 2: Compute.** For C1 and C5, elevation channel, ranges {300, 900, 1800, 3000}: `R(range) = floor_18_9(range) / canned_response(range)` using matched (cell, range, channel) pairs. Report the four ratios + max/min spread per cell.
- [ ] **Step 3: Verdict (pre-registered).** **FLAT** iff max/min ≤ 3× on BOTH cells (3× keeps the step ratio within the α margin: √3 ≈ 1.7× step error → ratio degrades 0.1 → 0.3, still an order below unity). **NOT FLAT** otherwise.
  - FLAT → descriptor carries a coarse range band; Build-time mapping; phone rangefinder-reuse safe.
  - NOT FLAT → descriptor MUST carry an exact reference range (required field, fail closed to today's fixed default when missing); step governed by the set max; phone must rebuild or bound by max intended range.

**Veto condition:** none (both outcomes route, neither kills). Effort: hours. This experiment has the highest information-per-cost on the list — run it first alongside E0.

---

**Verdict (recorded 2026-09-29, review-agreed): FLAT.** C1 spread 2.1050, C5 spread 2.2719 — both ≤3× bar with 42%/32% headroom (independently recomputed in review). Routing: coarse range band, Build-time mapping, phone rangefinder-reuse safe. Caveat carried: C1@1800 floor rests on a provenance-verified build artifact — re-run `LOB_FULL_LADDER=1` before E3/policy work leans on its absolute value. Full tables in `.superpowers/sdd/exp-e1-report.md`.

### E2: Cost breakdown per solve (the economics trial)

**Gating question:** does stepping dominate solve cost enough for step-size savings to matter — or does angle iteration dominate, killing the speed story?

- [ ] **Step 1: Instrument.** Scratch timing driver (uncommitted, `/tmp` or equivalent — NOT in the repo tree): Build time, forward-`Solve` wall time, `SolveInverse` wall time, on C1/C5/C8 at {300, 1800, 3000} ft, steps {36, 9} to separate per-step cost. Prefer existing cachegrind harness (`cg_bench_verify` pattern) where it fits; wall-clock chrono where it doesn't. Reuse test fixtures verbatim; no new test files.
- [ ] **Step 2: Decompose.** Report per-solve ms split into: (a) stepping loop, (b) angle-solver iterations (count iterations + time — expected dominant on inverse), (c) Build (incl. zero-angle). Report stepping share of forward-solve AND inverse-solve separately (phone apps mostly need forward; adjustment tables need inverse).
- [ ] **Step 3: Verdict (pre-registered).** Stepping share of the RELEVANT solve type (forward, unless the app needs inverse tables): **≥ 50% → economics healthy**, proceed. **< 25% → speed story DEAD**: coarsening cannot move total cost; the feature (if anything survives) reframes as precision-on-demand only — and if stepping is truly negligible, the honest outcome may be "always use a fine static step," killing even adaptivity. 25–50% → human judgment call with the numbers on the table.

**Veto condition:** < 25% kills the speed motivation (the feature's stated purpose). Effort: ~day including harness wrangling.

**Verdict (recorded 2026-09-29, review-agreed): HEALTHY, proceed to E3.** Forward stepping ≈100% (margin enormous); inverse at the ~50% line on 17/18 conditions with one condition (C5@3000, both steps — same ballistics, step-invariant iters=2) at ~34% → judgment-call band, judged proceed (use-case split: phones need forward; tables price inverse at a ~2–3× multiplier; coarsening 9→36 cuts totals ~4× for both types). QUALIFIER (binding on all later work): "Build ≈ 0%" holds ONLY for explicit-angle Builds — zero-by-distance Builds (up to 10 full trajectory integrations) were never timed; nothing downstream may inherit "Build is free" without that scope. Full numbers in `.superpowers/sdd/exp-e2-report.md`.

---

### E3: Upward step ladder + tier calibration (the safety trial)

**Gating question:** where does coarsening become unsafe, and what tier widths land derived steps in the useful band?

- [ ] **Step 1: Extend the ladder upward.** Same driver family, rungs {36, 72, 144, 288} (extend to 576 ONLY if 288 is still clean — one rung at a time, same gate), cases C1/C5/C8, ranges {300, 900, 1800, 3000}, all primary channels. Compare against the finest available reference (existing 1-in data where present, else finest rung + Richardson note). Record per-range per-channel deltas + where monotonicity breaks (Mach-knot skipping, wind-node joints, terminal-guard shifts — qualitative notes required, not just numbers).
- [ ] **Step 2: Set the cap (pre-registered rule).** Cap = largest step with ALL primary channels ≤ 10% of reporting granularity (0.1 MOA / 1 fps / 5 ft·lbf / 0.01 s). Rationale recorded: granularity proxies prediction uncertainty conservatively (true u ≥ granularity roughly; smaller u → finer required step, so the cap errs safe, never aggressive).
- [ ] **Step 3: Calibrate tiers.** Tier table (input form → width + one-line rationale; widths proposed from order-of-magnitude literature anchors WITH provenance strings, never vendor-claimed as truth): single-BC / BC-bands / custom-table; uniform / profile / profile+shear+heights wind; bare velocity default (coarse) + override path. Derive implied steps on the measured error curve; acceptance: all tier-implied steps land within [floor, cap] with floor = 9 in (finest allowed; downward data already shows 9-in error ~1e-5 in — refine needs no finer), and the set SPANS the band (not all pinned at one end — a degenerate span means the tiers carry no information; rework widths once, then escalate).
- [ ] **Step 4: Veto check (pre-registered).** If 72-in ALREADY exceeds 10% granularity on any primary channel of C1/C5/C8 → coarsening unsafe below useful savings → VETO the coarsening direction (refine-only + docs table survive; speed story dead — consistent with E2's possible verdict).
- [ ] **Step 5: Commit data.** New checked-in `test/validation/baselines/step_error_map.json` (coarse rungs, same provenance-header pattern as floors.json) + tier table as a docs section (in the eventual spec, not a separate file). Commit message records cap + tier span.

**Verdict (recorded 2026-09-29, review-agreed): VETO CLEAR, CAP 576, TIERS DEGENERATE-REAL → collapse to static cap.** 72-in worst margin 102.7× (governor C5 elev@3000, recomputed); 576 rule-certified (governor margin 1.21× — thin, binding envelope caveats apply: smooth forward solves ≤3000 ft, listed exclusions, second-platform run per §17.3b before policy leans on it). Tier attempts spanned 36×/54× in u yet everything pinned at cap; lab-grade u=0.01 MOA still yields step ~204 (22× above floor); floor-threatening u is ~2000× finer than any honest tier — H2 excluded, degeneracy is physics not artifact. Endorsed: (a) collapse adaptive-step to static cap; explicit StepSize escape hatch keeps demanding apps unharmed. Breakage: none (44/48 strictly increasing, p≈2 throughout). Full numbers in `.superpowers/sdd/exp-e3-report.md`.

**Veto condition:** 72-in breach kills coarsening (see Step 4). Effort: days (the campaign of the four).

---

## Explicitly dead — do not run, do not revive without a new proposal

- **Fidelity cost attribution** (per-step lapse/Coriolis/profile costs). Killed with step-only-forever. The knob discussion is closed.
- **Build-time live-solve timing.** v1 has no live solves; 9 ms by arithmetic stands unchallenged until attach-later revives (it hasn't).
- **Standalone tier-guess validation.** Tiers validate THROUGH E3 acceptance (useful-band span) + margin reasoning. No separate methodology trial.

---

## Results recording (binding on whoever runs these)

Each experiment ends with: commands run, raw numbers, verdict vs the pre-registered criterion above (FLAT/NOT FLAT, shares + verdict, cap + tier span), commit SHA(s) if any, and the routing consequence (which architecture question it closed). Record in `.superpowers/sdd/progress.md` + a dated experiment report next to the task reports. An experiment without a recorded verdict is unfinished — same rule as implementation tasks.

---

## Self-Review

**1. Coverage:** all four surviving experiments from the findings discussion are present with procedures concrete enough to execute (exact files, ranges, rungs, commands families, metric definitions); both dead experiments named with kill reasons; dependency order genuine (E3 truly needs E1's architecture + E2's verdict for tier interpretation; E0/E1/E2 mutually independent).
**2. No invented thresholds:** every veto number derives in-text (3× from margin math, 25/50% from savings-need math, 10% from the α ratio with granularity-as-conservative-u, floor 9 from measured downward data, 100/1000-yd bands pre-decided).
**3. Honest conditionals:** E1's per-range-data check and E3's 576-extension are bounded branches, not open ends; the E2 25–50% gray zone routes to human judgment explicitly rather than pretending a number decides it.

---

*End of plan. Implementation of the step policy itself starts only after all four verdicts are green (or explicitly overridden by human decision, recorded with reasoning).*
