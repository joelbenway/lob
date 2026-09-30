# Phase 4 — Uncertainty Budgets — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build the GUM uncertainty-budget combiner (`u_c²(y) = Σc_i²u²(x_i) + u²(ε) + u²(δ)`, covariances explicit) with a human-authored input-uncertainty manifest schema, fail-closed TBD handling, nonlinear rerouting to MC, and an offline assembler that joins Phase 1 floors + Phase 2 sensitivities + Phase 3 residuals into per-cell budget documents.

**Architecture:** Pure-math combiner (no solver contact, unit-tested on hand-computed fixtures) + manifest loader (nlohmann/json, `at()`-only, TBD markers fail closed) + env-gated offline assembler that reads the three checked-in artifact families and emits budget documents. No `u(x)` values are invented anywhere: the template ships TBD, the combiner refuses to emit a combined number while any row is TBD, and the incomplete budget listing missing rows IS the Phase 4 deliverable alongside proven math.

**Tech Stack:** C++14, GTest/GMock 1.14 (existing), nlohmann/json v3.12.0 (existing, test target only — same HARD BOUNDARY as Phase 3), CMake 3.14+.

## Global Constraints

- C++14 only; no `std::filesystem`, no structured bindings, no NEW third-party dependency (nlohmann/json reuse only, `lob_test` target only — `lob_lob` stays zero-dependency).
- `lob_lob` library diff is empty: no `source/*.cpp|hpp` production change, no public API change.
- All new test code uses the PUBLIC API only → `LOB_TEST_SOURCES` (shared+static CI). No internal headers, no `Impl`/`Pimpl`.
- Deterministic: fixed fixtures, no RNG, no wall-clock, no threads, no file writes in CI tests (checked-in manifest template is read-only input, same justification as Phase 3 case files).
- GUM vocabulary exact (spec §3.1): standard uncertainty `u(x)`, sensitivity coefficient `c_i`, combined standard uncertainty `u_c`, coverage factor `k` (default `k=1`, no `k=2` without recorded normality justification). Never "confidence interval", never "error bars".
- No invented `u(x)`: every manifest row needs `u` + `provenance`, or the literal `"TBD — human input required"` which fails closed. Example placeholders are banned from combined numbers (spec §11.3).
- CI additions under 30 s; offline assembler env-gated (`LOB_FULL_BUDGET=1`, else `GTEST_SKIP`).
- Plan location note: `docs/superpowers/plans/` is gitignored in this repo, so this plan lives versioned at `docs/specs/PHASE4_BUDGET_PLAN.md`.

---

## Scope

Phase 4 of `docs/specs/NUMERICAL_VALIDATION_SPEC.md` §20 ONLY: uncertainty-budget methodology (§11), its CI smoke (§14.3, §17), manifest/elicitation template (§11.3, §21 item 7), nonlinear rerouting (§11.2 → §12), and the §19.5 acceptance slice.

Deliberate deferrals (recorded so reviewers don't re-litigate):
- **No completed real-world budget.** The first budget with human-supplied sensor values is a human task (§21 item 7), not a subagent task — subagents must not invent chronograph specs. Phase 4 proves the machinery on synthetic fixtures and produces incomplete budgets from real artifacts.
- **No covariance values.** Correlation pairs (pressure/temperature/altitude, band BCs, multi-station profiles) ship as `"correlation": "assumed-independent — see risk"` flags. Estimating real covariances needs measurement campaigns, not code.
- **No `k=2` expanded uncertainties.** Output stays `k=1` with the assumption list; coverage intervals arrive with Phase 5's distributional machinery.
- **No MC runner** (Phase 5). Nonlinear-flagged rows emit `route_to_mc: true` markers the Phase 5 plan consumes.

---

## File Structure

```
test/
  CMakeLists.txt                      MODIFY: append validation_budget_test.cpp
                                      to LOB_TEST_SOURCES (1 line; json +
                                      cases-dir defines already on target)
  source/
    testing.hpp                       UNTOUCHED
    validation_budget_test.cpp        CREATE: combiner + manifest loader +
                                      CI smoke (math fixtures + TBD
                                      fail-closed) + offline assembler
                                      (env-gated)
  validation/
    cases/
      reference_*.json                UNTOUCHED (η rows stay "unknown")
    manifests/
      budget_template.json            CREATE: elicitation template — one row
                                      per Pareto-genuine driver per cell +
                                      ε/δ/η rows, all TBD with provenance
                                      prompts (checked in, human fills later)
    baselines/
      floors.json                     UNTOUCHED (ε_num source)
      pareto.json                     UNTOUCHED (c_i source)
docs/
  pages/validation/overview.md        MODIFY: append 3-line budget pointer
                                      (no numbers)
  specs/PHASE4_BUDGET_PLAN.md         THIS FILE
build/validation/                     GENERATED (gitignored):
                                      budget_{cell}_{channel}.json from
                                      offline assembler
```

Why this shape: the combiner is ~60 lines — a separate header would be abstraction theater; it lives with its tests like Phase 3's loader lives with its smoke. The manifest is data under `validation/manifests/` (new dir: manifests are human-authored inputs, distinct from machine-transcribed `cases/` and measured `baselines/`). One test file because combiner + loader + smoke + assembler share the row schema.

---

## Budget row schema (exact — combiner, template, and assembler implement this)

```json
{
  "input": "velocity_fps",
  "cell": "C1-ICAO",
  "channel": "elevation_in",
  "sensitivity_c": 2.31,
  "sensitivity_source": "pareto.json C1-ICAO elevation_in@1800 raw_deriv",
  "u": "TBD — human input required",
  "u_provenance": "TBD — e.g. chronograph datasheet, model + serial, calibration date",
  "distribution": "normal",
  "correlation": "assumed-independent — see risk",
  "contribution": null,
  "status": "incomplete-missing-u"
}
```

Rules: `contribution = |c|·u` (linear term; combined as root-sum-square); `status ∈ {complete, incomplete-missing-u, nonlinear-route-to-mc, below-floor-excluded}` (spec §9.4/§11.2 vocabulary); `below-floor-excluded` rows carry the floor value, never zero; the assembler emits NO `u_c` field unless every row is `complete` (fail-closed is structural, not a flag).

Manifest template (`budget_template.json`): `{artifact_schema: 1, provenance: {template_version, created_utc, instructions}, cells: [{cell, channel, range_ft, rows: [...]}, ...]}` — one entry per (Pareto cell × forward channel × 1800-ft range) for genuine drivers only (below_floor drivers excluded with reason at template-generation time, listed in an `excluded_with_floor` array so the exclusion is auditable), plus per-cell `epsilon_num` (floors.json value + source), `delta_ref` (envelope_report worst + source, or `"unknown"`), `eta_ref: "unknown"` rows. The `instructions` string tells the human exactly what evidence qualifies per row (datasheet / cert / supplier sheet / labeled judgment) — this string IS the §21-item-7 elicitation template under review.

---

### Task 1: GUM combiner + math fixtures (no solver, no JSON)

**Files:**
- Create: `test/source/validation_budget_test.cpp` (combiner + fixture tests; CMake registration in Task 3 with the smoke test — same deferred-registration pattern as Phases 2–3 Task 1)

**Interfaces:**
- Consumes: stdlib only (`<cmath>`, `<string>`, `<vector>`).
- Produces (used by Tasks 3, 4): `tests::BudgetRow { double c; double u; bool u_known; bool nonlinear; double floor_value; }`, `tests::CombineBudget(const std::vector<BudgetRow>&) -> {bool complete; double u_c; std::string status}` — `complete=false` (and `u_c` NaN, never 0) if any row has `!u_known` or `nonlinear`; otherwise `u_c = sqrt(Σ(c·u)²)`; `status` is the joined row-status vocabulary for the report.

Hand-computed fixture (verify by hand before trusting the test — recompute, don't loosen):
- rows: `{c=2.0, u=3.0}`, `{c=1.0, u=4.0}` → `u_c = sqrt(36+16) = sqrt(52) ≈ 7.211102551`.
- single-row `{c=0.5, u=2.0}` → `1.0`.
- any-row-unknown → `complete=false`, `u_c` NaN (`std::isnan` assert — NaN is the only honest "no number", never 0.0).
- any-row-nonlinear → `complete=false` with `route_to_mc` status (not combined, not dropped).

- [ ] **Step 1: Create the test file with combiner + fixture tests (fails: nothing to fail against yet — honest RED: write tests referencing the combiner, syntax-check fails on undeclared names)**

```cpp
// Copyright (c) 2026  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include <cmath>
#include <limits>
#include <vector>

// Combiner defined below the tests in this task (TDD: tests first).
// ... (full code at implementation time: BudgetRow, CombineBudget)

namespace tests {

TEST(BudgetMath, TwoRowCombinationMatchesHandComputation) {
  const std::vector<BudgetRow> kRows = {{2.0, 3.0, true, false, 0.0},
                                        {1.0, 4.0, true, false, 0.0}};
  const BudgetResult kR = CombineBudget(kRows);
  ASSERT_TRUE(kR.complete);
  EXPECT_NEAR(kR.u_c, 7.211102551, 1e-9);
}

TEST(BudgetMath, UnknownU_FailsClosedWithNaN) {
  const std::vector<BudgetRow> kRows = {{2.0, 3.0, true, false, 0.0},
                                        {1.0, 0.0, false, false, 0.0}};
  const BudgetResult kR = CombineBudget(kRows);
  EXPECT_FALSE(kR.complete);
  EXPECT_TRUE(std::isnan(kR.u_c));
}

TEST(BudgetMath, NonlinearRowRoutesToMc) {
  const std::vector<BudgetRow> kRows = {{2.0, 3.0, true, true, 0.0}};
  const BudgetResult kR = CombineBudget(kRows);
  EXPECT_FALSE(kR.complete);
  EXPECT_EQ(kR.status, "route_to_mc");
}

}  // namespace tests
```

- [ ] **Step 2: RED proof** — `g++ -std=c++14 -fsyntax-only -I test/source test/source/validation_budget_test.cpp` → FAIL (undeclared `BudgetRow`).
- [ ] **Step 3: Implement combiner above the tests** (~60 lines: structs + root-sum-square + fail-closed branches; ordered comparisons; NaN via `std::numeric_limits<double>::quiet_NaN()`).
- [ ] **Step 4: GREEN proof** — syntax-check passes + standalone gtest link-run 3/3 (Phase 2 Task 1 pattern). Full-suite proof waits for Task 3.
- [ ] **Step 5: Commit**

```bash
git add test/source/validation_budget_test.cpp
git commit -m "test: add GUM budget combiner with fixture tests"
```

---

### Task 2: Manifest template + loader with fail-closed semantics

**Files:**
- Create: `test/validation/manifests/budget_template.json`
- Modify: `test/source/validation_budget_test.cpp` (append loader + template tests; still unregistered)

**Interfaces:**
- Consumes: Task 1 combiner, `pareto.json` driver lists (which inputs are genuine per cell — read by the IMPLEMENTER at authoring time, not parsed by the loader; the template is hand-authored, reviewed, frozen), nlohmann/json (parse + `at()` discipline, same as Phase 3).
- Produces (used by Tasks 3, 4): `tests::LoadBudgetManifest(dir) -> {ok, cells, error}` + `tests::AssessManifest(cells) -> {complete_cells, incomplete_cells, missing_rows}` — `AssessManifest` returns the incomplete listing (cell/channel/row/reason) that IS the Phase 4 offline deliverable.

Template construction (mechanical, auditable): for each pareto cell (C1-ICAO, C5-uniform, C8-Litz, C6-shear) × forward channels (elevation_in, deflection_moa, tof_s) × 1800-ft range: one row per `genuine` driver with `sensitivity_c` = the row's `raw_deriv`, `sensitivity_source` citing cell/channel/range, `u` + `u_provenance` TBD strings, `distribution` best-guess default per spec §12.1 mapping (normal for sensors, uniform where only bounds known — LABELED as template defaults the human must confirm, not facts). `below_floor` drivers → `excluded_with_floor` arrays with floor values. Nonlinear-flagged pareto rows → `nonlinear-route-to-mc` status rows. Plus per-cell `epsilon_num` (floors.json number + source string), `delta_ref` (envelope_report worst + source, `"unknown"` where the matrix has no row), `eta_ref: "unknown"` rows. The top-level `instructions` string is the elicitation guide (what evidence qualifies, how to label judgment, who approves — §21 item 7 text).

- [ ] **Step 1: Author the template by hand from pareto.json** (read the JSON, write rows; ~40 rows — the implementer counts genuine drivers per cell/channel and reports the count; reviewer spot-checks 2 cells fully).
- [ ] **Step 2: Append loader + template tests** — `LoadBudgetManifest` parses + validates (schema key presence via `at()`, row shapes); tests assert: template loads `ok`; `AssessManifest` reports ZERO complete cells and N incomplete rows (N from Step 1 — the test asserts the exact count, so silently-added values fail the test); every row's `u` is the TBD literal (a test that greps the template for any numeric `u` outside epsilon/delta rows — the anti-invention tripwire).
- [ ] **Step 3: Syntax-check GREEN** (same command as Task 1).
- [ ] **Step 4: Commit (data + code together — the template is meaningless without its loader test)**

```bash
git add test/validation/manifests/budget_template.json test/source/validation_budget_test.cpp
git commit -m "test: add budget elicitation template with fail-closed loader"
```

---

### Task 3: CI smoke (math + fail-closed on the real template)

**Files:**
- Modify: `test/source/validation_budget_test.cpp` (append smoke), `test/CMakeLists.txt` (1 line `LOB_TEST_SOURCES`; json + cases-dir defines already on target — LOB_VALIDATION_CASES_DIR covers `test/validation/` tree; manifests read via the same define, documented in a comment)

**Interfaces:**
- Consumes: Tasks 1–2.
- Produces: the CI gate for budgets.

Smoke (`BudgetSmoke.TemplateIsIncompleteAndMathHolds`, hermetic read-only, no solver):
1. Load the real `budget_template.json` from the cases-dir define path; assert `ok`.
2. Assert `AssessManifest` finds zero complete cells (template must NEVER accidentally become complete — this is the test that catches a future edit sneaking in a numeric `u` without review... precisely: it catches it at the count level; the per-row TBD-literal test from Task 2 catches it at the value level).
3. Run the Task 1 fixture combination inline (2-row sqrt(52) case) to prove the combiner linked into the suite answers correctly in-suite (not just standalone).
4. Assert the `instructions` string is non-empty and mentions "human" (elicitation template self-identification — cheap tripwire against template replacement).

- [ ] **Step 1: CMake edit + smoke test.**
- [ ] **Step 2: Build + run** — `cmake --build --preset=dev --target lob_test && ./build/dev/test/lob_test --gtest_filter='BudgetMath.*:BudgetSmoke.*'` → PASS (3 math + 1 smoke). First full-suite count recorded in the report.
- [ ] **Step 3: Commit**

```bash
git add test/source/validation_budget_test.cpp test/CMakeLists.txt
git commit -m "test: add budget math and fail-closed template smoke"
```

---

### Task 4: Offline assembler (env-gated) joining all three artifact families

**Files:**
- Modify: `test/source/validation_budget_test.cpp` (append driver)

**Interfaces:**
- Consumes: Tasks 1–3, `floors.json` (ε), `pareto.json` (c_i + flags), `envelope_report.json` (δ worsts), the manifest template.
- Produces: `build/validation/budget_{cell}_{channel}.json` (gitignored) with rows/shares/assumptions + `u_c` ONLY where complete (nowhere in Phase 4 — every document renders incomplete with its missing-row list; the test asserts exactly that: zero emitted `u_c` values across all documents).

Driver (`LOB_FULL_BUDGET=1`, else `GTEST_SKIP`; writes via existing `LOB_VALIDATION_DIR` — no CMake change):
- Per manifest cell/channel: load rows, attach `sensitivity_c` cross-check against live `pareto.json` values (mismatch → FAIL loudly: template drift vs measured sensitivities must break, not slide), attach ε/δ/η rows from floors/envelope, run `CombineBudget`, emit document `{provenance (§15.2 header), rows[], excluded_with_floor[], nonlinear_route_to_mc[], missing_u[], u_c_or_absent, assumptions[]}`.
- `assumptions[]` enumerates every independence/correlation flag, every floor reuse, every unknown — the document's honesty section, machine-readable.
- Assert in-driver: `missing_u[]` non-empty for every cell (if some cell ever completes, the test FAILS with "manifest completed without human review — see §21 item 7" — the tripwire that forces the human gate to be explicit, not accidental).

- [ ] **Step 1: Implement + run gated** (`LOB_FULL_BUDGET=1 ... --gtest_filter='BudgetAssemble.*'`), inspect one document by hand for schema sanity.
- [ ] **Step 2: Prove skip-by-default** + `ctest -R Validation` green both ways.
- [ ] **Step 3: Commit** (test file only; `build/` untracked)

```bash
git add test/source/validation_budget_test.cpp
git commit -m "test: add offline budget assembler with incomplete verdicts"
```

---

### Task 5: Docs pointer + full gate + lint

**Files:**
- Modify: `docs/pages/validation/overview.md` (append 3-line budget pointer under the reference paragraph — method pointer, no numbers)

Pointer text (verbatim):

```markdown
Uncertainty budgets combine sensitivities, numerical floors, and reference
residuals per `docs/specs/NUMERICAL_VALIDATION_SPEC.md` §11; the combiner
and manifest schema live in `test/source/validation_budget_test.cpp` with
the elicitation template at `test/validation/manifests/budget_template.json`.
CI proves the math and the fail-closed template; full assembly runs offline
behind `LOB_FULL_BUDGET=1`. No combined uncertainty is claimed anywhere yet:
every input uncertainty awaits human-supplied evidence.
```

- [ ] **Step 1: Append pointer.**
- [ ] **Step 2: Full local gate** — `cmake --build --preset=dev && ctest --preset=dev --output-on-failure -j 4` → 100% PASS (assembler SKIPPED); gated run → PASS + budget documents with missing-row lists.
- [ ] **Step 3: Linters** — format + spell per HACKING.md; fix in place, `style:` commit if needed.
- [ ] **Step 4: Acceptance snapshot** (§19.5): GUM combiner tested on fixtures; incomplete budgets fail closed; nonlinear rows route to MC visibly; empty `lob_lob` diff proof; manifest template awaiting §21-item-7 human values. Commit docs.

```bash
git add docs/pages/validation/overview.md
git commit -m "docs: point validation page at budget methodology"
```

---

## Self-Review

**1. Spec coverage (§11 / §14 / §15 / §17 / §19.5 / §20 Phase 4 row):**
- §11.1 GUM model (combined `u_c`, covariances, ε/δ/η rows, `k=1` default) → Tasks 1 (combiner) + 2 (schema rows) + 4 (assembly with covariance flags carried, not computed).
- §11.2 linear-failure flags → nonlinear rows from pareto flags emit `route_to_mc`, never combined (Tasks 1/4); quantized-channel + branch + categorical rules cited in template `instructions` as human-review prompts.
- §11.3 known-vs-assumed `u(x)` (no invented values, TBD fail-closed, numerically-known ε/angle rows) → Tasks 2–4 (template TBDs, anti-invention test, ε rows filled from floors, angle-tolerance contribution noted in instructions as derivable — not derived here, honest).
- §14 (extend test/source/, LOB-only, no Impl) → one test file, one CMake line, public API + nlohmann only.
- §15 (provenance headers, raw-vs-curated) → budget documents carry §15.2 headers; template is curated human-input surface (checked in); documents gitignored (generated).
- §17 (fast gate, offline assembler) → Task 3 hermetic CI + Task 4 gated offline.
- §19.5 acceptance → Task 5 snapshot. §20 Phase 4 row → files/tests/artifacts/CI/cost covered (offline cost: seconds — file reads + arithmetic, zero solves).
- §21 item 7 (elicitation template + first sources) → the `instructions` string + TBD rows ARE the template under review; first real values explicitly out of scope (human task). Item 4 (MC defaults) untouched — Phase 5.
- GAP CHECK: §11.1 covariance terms — combiner has NO covariance parameter in the Task 1 struct (only per-row c/u). Pairwise `2·Σc_i·c_j·u(x_i,x_j)` needs a pair list. FIX: add `tests::BudgetCovariance {size_t i, j; double cov;}` + optional vector param defaulting empty (C++14 default arg, no overload explosion); fixture test with one pair: rows from the sqrt(52) case + cov(i,j)=6.0 → u_c² = 52+12 = 64 → u_c = 8.0 exactly. Implementer adds this to Task 1 (struct + param + fourth test); reviewer checks the arithmetic. (Recording here so the fix rides with Task 1, not as a follow-up.)

**2. Placeholder scan:** no TBD/TODO/later in CODE; `"TBD — human input required"` literals are spec-mandated fail-closed vocabulary (§11.3), not placeholders; `u_c` absent (not null, not zero) on incomplete documents — structural honesty.

**3. Type consistency:** `BudgetRow`/`BudgetResult`/`BudgetCovariance` shapes fixed in Task 1 tests; manifest `u` is string-or-number in JSON (loader branches `is_string`→TBD vs `is_number`→value — the anti-invention test pins all-TBD); `LOB_VALIDATION_CASES_DIR` reuse for manifests documented in Task 3 comment; `GTEST_SKIP` 1.14-valid; `std::isnan` for the no-number proof (never `==` on doubles).

---

*End of plan. Phase 5 (Monte Carlo) plan is written after Phase 4 machinery merges, per spec Appendix A.*
