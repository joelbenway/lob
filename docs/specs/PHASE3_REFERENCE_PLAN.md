# Phase 3 — Reference Validation Infrastructure — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Transcribe the six borrowed reference trajectories into versioned `reference_*.json` case files with provenance and envelope tags, wire the repo's existing nlohmann/json dependency into the test target plus a residual-decomposition harness (`r = ε_num + δ + η_ref`), and gate CI on loader round-trip + C1-ICAO decomposition while the full coverage matrix runs offline.

**Architecture:** Data files (hand-transcribed, reviewer-diffed number-by-number) parsed with the repo's existing nlohmann/json dependency (v3.12.0, same FetchContent pattern as `example/lobber` — tests only, never the library) + explicit per-case Builder factories (no stringly-typed generic interpreter) + decomposition asserts reusing Phase 1 floor constants and the `OutputNearMatcher` MOA-space convention. Existing `lob_env_test.cpp` vectors stay untouched (no dedup refactor — explicitly deferred).

**Tech Stack:** C++14, GTest/GMock 1.14 (existing), nlohmann/json v3.12.0 (existing repo dependency via `example/lobber` — tests only), CMake 3.14+.

## Global Constraints

- C++14 only; no `std::filesystem`, no structured bindings, no NEW third-party dependency. nlohmann/json v3.12.0 is explicitly allowed: it is already a repo dependency (`example/lobber/source/CMakeLists.txt` FetchContent fallback, also in the nix dev shell), so reusing it adds zero new supply-chain surface. HARD BOUNDARY: it links to the `lob_test` target only — `lob_lob` stays zero-dependency (no `#include <nlohmann/...>` outside `test/`), preserving the embedded core. The Phase 1 hand-rolled *writer* stays as-is (no reason to churn it).
- `lob_lob` library diff is empty: no `source/*.cpp|hpp` production change, no public API change.
- All new test code uses the PUBLIC API only → `LOB_TEST_SOURCES` (shared+static CI). No internal headers, no `Impl`/`Pimpl`.
- Deterministic: fixed factories, fixed ranges, default 36-in step, `0.01` MOA tolerance, no RNG, no wall-clock, no threads, no file writes in CI tests (read-only file access to checked-in case files is allowed — they are inputs, like headers).
- Case files live at `test/validation/cases/reference_*.json` (checked in, reviewed numbers). Test binary reads them via a `LOB_VALIDATION_CASES_DIR` compile definition (same pattern as Phase 1 `LOB_VALIDATION_DIR`).
- Reporting granularities reused verbatim: velocity `±1 fps`, energy `±5 ft·lbf`, `0.1 MOA`, TOF `±0.01 s`.
- `u_ref` is `"unknown"` on every expected field in every case (spec §10.6) → every cell caps at **provisional**; the decomposition harness treats unknown reference uncertainty as a visible unknown row, never zero.
- Plan location note: `docs/superpowers/plans/` is gitignored in this repo, so this plan lives versioned at `docs/specs/PHASE3_REFERENCE_PLAN.md`.

---

## Scope

Phase 3 of `docs/specs/NUMERICAL_VALIDATION_SPEC.md` §20 ONLY: reference methodology (§10), its CI smoke (§14.3, §17), case records (§10.4), model-form spot checks as scenarios (§10.5), and the §19.4 acceptance slice. Phases 4–6 (budgets, MC, envelope claims page) get their own plans after Phase 3 cases merge, per spec Appendix A.

Deliberate deferrals (recorded so reviewers don't re-litigate):
- **No dedup of `lob_env_test.cpp`.** After transcription two copies of the vectors exist (C++ literals + JSON). Removing the C++ copy would touch six passing tests for zero behavioral gain and risk destabilizing the shared/static suites. The JSON is the reporting source of truth from here on; the C++ vectors remain the regression tripwire. Dedup is a cleanup proposal, not this phase.
- **No GUM budget math** (Phase 4). The decomposition reports `r`, `ε_floor`, and `δ = r − ε_floor` per channel — the budget's input rows, not the budget.
- **No new reference data.** The six borrowed trajectories are the entire corpus; long-range/field cells stay unclaimed by policy (§10.6). Acquisition is a human decision (§21 item 11), not a task.
- **No hand-rolled JSON parser.** nlohmann/json v3.12.0 is already a repo dependency (lobber) — writing our own parser next to it would be NIH. The thin contract is: `parse` in try/catch, `at()` for access (never `operator[]`), `is_*()` checks before `get<>()`. Duplicate-key protection comes from the Task 2 scripted transcription diff, not the parser (recorded limitation).

---

## File Structure

```
test/
  CMakeLists.txt                      MODIFY: append validation_reference_test.cpp
                                      to LOB_TEST_SOURCES; add
                                      LOB_VALIDATION_CASES_DIR compile
                                      definition (same block as Phase 1 defs);
                                      add nlohmann_json dependency block
                                      (find_package-or-FetchContent v3.12.0,
                                      same shape as example/lobber) linked to
                                      lob_test ONLY
  source/
    testing.hpp                       UNTOUCHED (OutputNearMatcher,
                                      VerifySolutions, MakeC1IcaoBuilder
                                      reused as-is)
    validation_reference_test.cpp     CREATE: schema unit tests + per-case
                                      factories + CI smoke (loader round-trip
                                      + C1 decomposition) + offline matrix
                                      driver (env-gated)
  validation/
    cases/
      reference_icao.json             CREATE: transcribed ICAO vectors +
                                      provenance + envelope tags
      reference_altitude4500.json     CREATE: same for +4500 ft case
      reference_hot_lowp.json         CREATE: same for 100 °F / 25 inHg case
      reference_barometer.json        CREATE: same for barometer-offset case
      reference_humidity.json         CREATE: same for 29 inHg / 75 °F / 80% case
      reference_weather_station.json  CREATE: same for weather-station case
    baselines/
      floors.json                     UNTOUCHED (C1 floors consumed as ε_num)
docs/
  pages/validation/overview.md        MODIFY: append 3-line reference pointer
                                      (no numbers)
  specs/PHASE3_REFERENCE_PLAN.md      THIS FILE
build/validation/                     GENERATED (gitignored):
                                      envelope_report_*.json from offline driver
```

Why this shape: nlohmann/json is shared infrastructure (lobber already links it; Phases 4–5 reuse the same dependency for budgets and MC manifests) so no test-local parser exists to maintain; one test file holds schema tests + factories + smoke + driver because all four are small and share the case files; case files are data, each reviewed independently.

---

## Reference schema (exact — reader and files implement this, nothing more)

```json
{
  "artifact_schema": 1,
  "id": "ref-icao",
  "provenance": {
    "source": "McCoy, Modern Exterior Ballistics (ISA treatment, pp. 166-168); Huang saturation-pressure formula (J. Appl. Meteor. Climatol.); lob test corpus test/source/lob_env_test.cpp",
    "lob_version": "0.13.0",
    "transcribed_utc": "2026-..-..",
    "transcribed_from": "test/source/lob_env_test.cpp:83-95 (SolveAtICAOAtmosphere kExpected)"
  },
  "builder": {
    "note": "verbatim setter list with units; mirror the fixture",
    "ballistic_coefficient_psi": 0.232,
    "bc_drag_function": "G7",
    "bc_atmosphere": "ICAO",
    "diameter_in": 0.308,
    "mass_grains": 155.0,
    "initial_velocity_fps": 2800,
    "zero_angle_moa": 3.66,
    "optic_height_in": 1.5
  },
  "solver_config": {"step_in": 36, "angle_tol_moa": 0.01, "density_path": "fast"},
  "ranges_ft": [0, 150, 300, 600, 900, 1200, 1500, 1800, 2100, 2400, 2700, 3000],
  "reporting_granularity": {"velocity_fps": 1.0, "energy_ft_lbf": 5.0, "elevation_moa": 0.1, "time_of_flight_s": 0.01},
  "expected": [
    {"range_ft": 0, "velocity_fps": 2800, "energy_ft_lbf": 2696, "elevation_in": -1.5, "deflection_in": 0.0, "time_of_flight_s": 0.0, "u_ref": "unknown"},
    {"range_ft": 150, "velocity_fps": 2699, "...": "..."}
  ],
  "envelope_tags": ["range-0-300yd", "range-300-1000yd", "regime-supersonic", "regime-transonic-tail", "atm-isa", "wind-calm", "spin-off", "density-fast", "drag-g7-single-bc"],
  "status": "provisional",
  "supersedes": null
}
```

Rules the implementer must respect:
- `expected[]` numbers are transcribed digit-for-digit from the cited `kExpected` lines (see Task 2 table). Rounding, re-typing from memory, or "cleaning up" trailing zeros is forbidden — the task review diffs every number.
- `builder` carries ONLY the setters the fixture calls (ICAO case: the 8 above — no invented atmosphere/wind/spin keys). Atmosphere-variant cases add exactly the fixture's extra setters (`altitude_of_firing_site_ft`, `air_pressure_inhg`, `temperature_degf`, `altitude_of_barometer_ft`, `altitude_of_thermometer_ft`, `relative_humidity_percent`) with the fixture's literal values.
- `status` is `"provisional"` on all six (single case per cell + `u_ref` unknown — spec §10.3/§10.6). No `"anchor"` anywhere in Phase 3.
- `envelope_tags` vocabulary is closed: the tag strings in the per-case table (Task 2). New tags require a plan amendment.

---

### Task 1: nlohmann/json wiring + schema unit tests

> **Amendment 2026-09-28 (user catch):** an earlier draft mandated a hand-rolled
> JSON parser. Killed — nlohmann/json v3.12.0 is already a repo dependency
> (`example/lobber`, FetchContent fallback), so reusing it in tests adds zero
> new supply-chain surface. HARD BOUNDARY stands: test target only, `lob_lob`
> stays zero-dependency.

**Files:**
- Test: `test/source/validation_reference_test.cpp` (created here as scaffolding with schema tests; CMake registration lands in Task 3 with the smoke test)
- Modify: `test/CMakeLists.txt` in Task 3 (json dependency block — NOT this task; this task stays syntax-check-only like Phase 2 Task 1)

**Interfaces:**
- Consumes: nlohmann/json v3.12.0 (`<nlohmann/json.hpp>`, include path resolved in Task 3 via the lobber FetchContent pattern).
- Produces (used by Tasks 3, 4): the proven parse idiom — `nlohmann::json::parse` inside try/catch (`json::parse_error` → fail with `e.what()`, which carries the byte offset), strict schema access via `at()` (throws `out_of_range` on missing keys — caught and failed, never `operator[]` which would silently insert), type checks via `is_number()`/`is_string()`/`is_array()` before `get<>()`.

Semantics (exact — this is the whole "reader contract", and it fits in a test file because the library does the parsing):
- Malformed documents throw `parse_error` → tests assert throw/no-throw (no custom error plumbing).
- Missing keys / wrong types throw `out_of_range` / `type_error` → loader (Task 3) catches per-file and fails with the filename + `what()`.
- `u_ref: "unknown"` and `status: "provisional"` are asserted as literal strings (missing-evidence rules as data-shape pins).
- Duplicate keys: nlohmann keeps the last — transcription-typo protection comes from the Task 2 scripted diff, not the parser (recorded limitation, accepted: the diff is the tripwire).

- [ ] **Step 1: Create the test scaffolding with schema unit tests (fails: no json include path yet)**

Create `test/source/validation_reference_test.cpp`:

```cpp
// Copyright (c) 2026  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <string>

namespace tests {

TEST(ReferenceJson, ParsesFlatCaseShape) {
  const nlohmann::json kRoot = nlohmann::json::parse(
      "{\"id\":\"ref-icao\",\"ranges_ft\":[0,150],\"expected\":[{\"range_ft\":0,"
      "\"velocity_fps\":2800,\"u_ref\":\"unknown\"}],\"status\":\"provisional\","
      "\"supersedes\":null}");
  EXPECT_EQ(kRoot.at("id").get<std::string>(), "ref-icao");
  EXPECT_DOUBLE_EQ(kRoot.at("ranges_ft").at(1).get<double>(), 150.0);
  EXPECT_DOUBLE_EQ(
      kRoot.at("expected").at(0).at("velocity_fps").get<double>(), 2800.0);
  EXPECT_EQ(kRoot.at("expected").at(0).at("u_ref").get<std::string>(),
            "unknown");
  EXPECT_TRUE(kRoot.at("supersedes").is_null());
  EXPECT_EQ(kRoot.at("status").get<std::string>(), "provisional");
}

TEST(ReferenceJson, RejectsMalformedDocuments) {
  for (const char* kBad : {"{\"a\":1,}", "{\"a\":", "{\"a\" 1}", "[1,2",
                           "{\"a\":01}", ""}) {
    EXPECT_THROW(nlohmann::json::parse(kBad), nlohmann::json::parse_error)
        << "accepted: " << kBad;
  }
}

TEST(ReferenceJson, MissingKeysThrowInsteadOfInserting) {
  const nlohmann::json kRoot = nlohmann::json::parse("{\"a\":1}");
  EXPECT_THROW(kRoot.at("zzz"), nlohmann::json::out_of_range);
  EXPECT_THROW(kRoot.at("a").get<std::string>(), nlohmann::json::type_error);
}

}  // namespace tests
```

(`{"a":01}` must throw: leading-zero numbers are invalid JSON and a classic transcription typo shape. `at()` — never `operator[]` — is the load-bearing habit: `operator[]` on a const object throws too, but on a non-const object it silently inserts; `at()` always throws. Reviewer checks no `operator[]` appears in test code.)

- [ ] **Step 2: Run to verify it fails (no include path without the Task 3 CMake wiring)**

Run: `g++ -std=c++14 -fsyntax-only -I test/source test/source/validation_reference_test.cpp`
Expected: FAIL — `nlohmann/json.hpp: No such file or directory` (include path arrives with Task 3's FetchContent wiring; same honest-RED trick as Phase 2 Task 1).

- [ ] **Step 3: GREEN proof deferred to Task 3 by design** — no standalone header to create (the library IS the implementation). The implementer verifies the test logic by inspection against nlohmann v3.12.0 documented API (`parse` throws `parse_error`; `at` throws `out_of_range`; `get<T>` throws `type_error`; all stable since v3.0) and reports that; compilation + execution proof lands with Task 3 registration. If the implementer has a system nlohmann/json available, a bonus standalone run is welcome evidence but not required.

- [ ] **Step 4: Commit**

```bash
git add test/source/validation_reference_test.cpp
git commit -m "test: add reference-schema unit tests on nlohmann/json"
```

---

### Task 2: Transcribe the six reference cases (data task)

**Files:**
- Create: `test/validation/cases/reference_*.json` (6 files)

**Interfaces:**
- Consumes: `test/source/lob_env_test.cpp` vectors (read-only source), the schema above.
- Produces: the corpus every later task and phase consumes. No code.

Transcription table (exact source lines — reviewer diffs each number against these):

| file | fixture setters (builder) | kExpected lines | envelope atmosphere tag(s) |
|---|---|---|---|
| `reference_icao.json` | base 8 (schema example) | `lob_env_test.cpp:83-95` | `atm-isa` |
| `reference_altitude4500.json` | base 8 + `altitude_of_firing_site_ft: 4500`, `temperature_degf: 59` | `:118-130` | `atm-altitude-4500ft` |
| `reference_hot_lowp.json` | base 8 + `temperature_degf: 100`, `air_pressure_inhg: 25` | `:153-165` | `atm-hot-lowp` |
| `reference_barometer.json` | base 8 + `altitude_of_firing_site_ft: 5280`, `air_pressure_inhg: 30`, `altitude_of_barometer_ft: 0`, `temperature_degf: 59` | `:192-204` | `atm-barometer-offset` |
| `reference_humidity.json` | base 8 + `air_pressure_inhg: 29`, `temperature_degf: 75`, `relative_humidity_percent: 80` | `:230-242` | `atm-humid` |
| `reference_weather_station.json` | base 8 + `altitude_of_firing_site_ft: 5280`, `air_pressure_inhg: 30`, `altitude_of_barometer_ft: 0`, `temperature_degf: 65`, `altitude_of_thermometer_ft: 3598` | `:272-284` | `atm-weather-station` |

(All line numbers against the file as of the Phase 3 branch point — re-verify with grep before transcribing; if they drifted, cite the new lines in the commit message.)

Shared tags on all six: `range-0-300yd`, `range-300-1000yd` (3000 ft = 1000 yd), `regime-supersonic`, `regime-transonic-tail` (3000-ft velocity ≈ 1150–1440 fps ≈ Mach 1.0–1.3 across cases), `wind-calm`, `spin-off`, `density-fast`, `drag-g7-single-bc`. Provenance `source` string identical on all six (schema example); `transcribed_from` cites the per-case test name + line range.

- [ ] **Step 1: Write the six files by transcription (no code, no tests yet)**
- [ ] **Step 2: Self-verify with a scripted diff (evidence for the report)**

Run: a `python3` one-liner per file extracting `expected[].elevation_in` (or all numeric fields) and diffing against the `kExpected` literals scraped from `lob_env_test.cpp`. Any mismatch → fix the JSON, never the test file. Paste the (empty) diff output into the report.
- [ ] **Step 3: Validate all six parse with the Task 1 reader**

Run: a tiny driver or `python3 -c json.load` for well-formedness PLUS a parse through the Task 3 loader path — at minimum `python3 -c "import json,glob; [json.load(open(f)) for f in glob.glob('test/validation/cases/*.json')]"` for syntax and a schema-key checklist (all 11 top-level keys present, 12 expected rows, ranges length 12).
- [ ] **Step 4: Commit (data only)**

```bash
git add test/validation/cases/
git commit -m "test: add six transcribed reference cases with provenance"
```

---

### Task 3: Loader + C1-ICAO decomposition smoke (CI gate)

**Files:**
- Modify: `test/source/validation_reference_test.cpp` (append factories + smoke)
- Modify: `test/CMakeLists.txt` (append file to `LOB_TEST_SOURCES`; add `LOB_VALIDATION_CASES_DIR` definition in the Phase 1 block)

**Interfaces:**
- Consumes: Task 1 reader, Task 2 case files, Phase 1 `floors.json` C1 cell (noise constants, cross-linked), `testing.hpp` (`MakeC1IcaoBuilder`, `SolveN`, diff helpers).
- Produces: the CI gate for reference validation; loader pattern reused by Task 4.

Loader design (explicit factories, no generic interpreter): `tests::LoadReferenceCase(const std::string& dir, const std::string& id)` reads `<dir>/reference_<suffix>.json` via `std::ifstream` into a string, parses with `nlohmann::json::parse` inside try/catch (`parse_error` → FAIL with filename + `e.what()`, which carries the byte offset), validates schema keys via `at()` (missing/wrong-type → `out_of_range`/`type_error`, caught and failed with key path), and returns the `json` value (caller extracts rows). Per-case Builder factories are 5-line functions (`BuildIcaoCase()` = `MakeC1IcaoBuilder()` verbatim — the C1 fixture IS the ICAO case; the other five wrap it with their extra setters). File-not-found and schema violations FAIL loudly with the path in the message (never fall back to embedded literals — a missing case file must break CI, not silently pass).

Decomposition smoke (`ReferenceDecomposition.C1Icao`, hermetic read-only, default 36-in step):
- Load `reference_icao.json` from `LOB_VALIDATION_CASES_DIR`; assert 12 rows, ranges match `{0..3000}`.
- Solve the ICAO factory at the case ranges; per range assert `VerifySolutions`-equivalence by reusing `OutputNear` with the case's `reporting_granularity` (velocity 1, energy 5, MOA 0.1, TOF 0.01) — this is the loader round-trip: JSON numbers reproduce the C++-literal test outcome.
- Decomposition asserts per range on elevation: `r = |y_lob − y_ref|` (inches, direct — the reference IS the anchor here, no finer rung exists); `ε_floor` = C1 `floors_18_9.elevation_in` (4.15814e-05, cross-linked comment); assert `r ≤ granularity (0.1-MOA-equivalent in inches at that range… precisely: convert via LobInchToMoa like OutputNear does, assert MOA residual ≤ 0.1)` AND record `δ = r − ε_floor` (assert `δ ≥ 0` sanity — a negative δ means the floor exceeds the residual, i.e. "consistent within numerical resolution" per §10.2, which PASSES with that verdict logged via `SCOPED_TRACE`).
- Verdict logic (exact): `|r| ≤ ε_floor` → consistent-within-resolution; `ε_floor < |r| ≤ granularity` → residual-is-model-or-reference (`δ` reported, passes); `|r| > granularity` → FAIL (regression vs the borrowed anchor).

- [ ] **Step 1: Register in CMake + add `LOB_VALIDATION_CASES_DIR` define, write loader + smoke (RED: file-not-found path first — point the define at a wrong dir temporarily? No: honest RED is the smoke failing before factories exist. Write factories + smoke together, build, run.)**
- [ ] **Step 2: Build + run**

Run: `cmake --build --preset=dev --target lob_test && ./build/dev/test/lob_test --gtest_filter='ReferenceJson.*:ReferenceDecomposition.*'`
Expected: PASS (3 reader + 1 smoke). On residual FAIL: do NOT retune numbers — a residual above granularity means the transcription is wrong (re-diff Task 2) or the solver regressed (bisect). Escalate NEEDS_CONTEXT with the range + channel + values.
- [ ] **Step 3: Commit**

```bash
git add test/source/validation_reference_test.cpp test/CMakeLists.txt
git commit -m "test: add reference loader with C1-ICAO decomposition smoke"
```

---

### Task 4: Offline full-matrix driver + coverage matrix (env-gated)

**Files:**
- Modify: `test/source/validation_reference_test.cpp` (append driver)

**Interfaces:**
- Consumes: Tasks 1–3, all six case files.
- Produces: `build/validation/envelope_report.json` (+ per-case CSV rows), the matrix Phase 6 renders.

Driver (`LOB_FULL_MATRIX=1`, else `GTEST_SKIP`; no file writes unless gated; `WriteFiles(LOB_VALIDATION_DIR, ...)` reuse — no CMake change):
- For each of the six cases: load, build via its factory, solve at case ranges, decompose every channel (elevation_in + elevation_moa dual view, deflection_moa, velocity, energy, tof) with the C1 floor family as `ε_num` (comment: cross-case floor reuse is conservative — C1 floors are the only measured set; per-case floors arrive with envelope expansion, ledger note).
- Model-form spot checks (§10.5, same driver run): G7-vs-G1 mismatch on the ICAO builder (scenario swap, `δ_structural`), lapse on/off is N/A offline here (Fast-only corpus — record as `not_applicable` with reason, not skipped-silently), Coriolis/spin/jump all off in this corpus (`not_applicable`, same treatment).
- Coverage matrix output: per envelope tag → case IDs covering it → worst residual per channel → verdict (`provisional` everywhere in Phase 3; `unclaimed` tags listed explicitly: `range-1000yd-plus`, `wind-profile*`, `spin-*`, `density-lapse-tail`, `drag-bands`, `drag-custom`, `field-radar`).
- `envelope_report.json` schema: `{provenance, cells: [{tags, cases, worst_residual, verdict}], unclaimed: [...], structural: [...]}`. Numbers from the actual run; zero placeholders.

- [ ] **Step 1: Implement + run gated** (`LOB_FULL_MATRIX=1 ./build/dev/test/lob_test --gtest_filter='ReferenceMatrix.*'`), check runtime (target: seconds — six solves + checks; if minutes, say so in the report, no reduction needed at this scale).
- [ ] **Step 2: Prove skip-by-default** (unset → SKIPPED, `ctest -R Validation` green both ways).
- [ ] **Step 3: Commit** (test file only; `build/` artifacts untracked)

```bash
git add test/source/validation_reference_test.cpp
git commit -m "test: add offline reference-matrix driver with coverage report"
```

---

### Task 5: Docs pointer + full gate + lint

**Files:**
- Modify: `docs/pages/validation/overview.md` (append 3-line reference pointer under the sensitivity paragraph — method pointer, no numbers)
- No code changes unless the gate finds regressions.

- [ ] **Step 1: Append pointer text**

```markdown
Reference trajectories live in `test/validation/cases/reference_*.json`
(transcribed from `test/source/lob_env_test.cpp` with provenance and
envelope tags, per `docs/specs/NUMERICAL_VALIDATION_SPEC.md` §10). CI
round-trips the loader and decomposes C1-ICAO residuals against measured
numerical floors; the full six-case matrix with coverage runs offline
behind `LOB_FULL_MATRIX=1`. All cells are provisional: reference
uncertainties are unstated and no field data exists yet.
```

- [ ] **Step 2: Full local gate** — `cmake --build --preset=dev && ctest --preset=dev --output-on-failure -j 4` → 100% PASS (matrix SKIPPED); `LOB_FULL_MATRIX=1` focused run → PASS + `envelope_report.json` with provenance.
- [ ] **Step 3: Linters** — `cmake -D FORMAT_COMMAND=clang-format -P cmake/lint.cmake`, `cmake -P cmake/spell.cmake` (via nix per HACKING.md if local); fix in place, `style:` commit if needed.
- [ ] **Step 4: Acceptance snapshot** (§19.4): six cases with provenance; per-cell total residual + floor-subtracted δ; honest provisional/unclaimed matrix; empty `lob_lob` diff proof (`git status --short | grep -v '^??' | grep -E 'source/|include/'` prints nothing). Commit docs.

```bash
git add docs/pages/validation/overview.md
git commit -m "docs: point validation page at reference methodology"
```

---

## Self-Review

**1. Spec coverage (§10 / §14 / §15 / §17 / §19.4 / §20 Phase 3 row):**
- §10.1 reference priority (BRL → borrowed trajectories → papers → external placeholder) → Task 2 transcribes tier 2 with tier-1/3 cited in provenance; tier 4 stays an explicit unclaimed tag.
- §10.2 comparison procedure (frozen config, same metric family, `r = ε + δ + η` decomposition, "consistent within resolution" verdict) → Task 3 smoke implements it verbatim; `η_ref = unknown` carried visibly via `u_ref: "unknown"`.
- §10.3 envelope discipline (tags, per-cell worst, provisional/unclaimed, forbidden extrapolations) → schema tags + Task 4 matrix with explicit unclaimed list.
- §10.4 case record format (id/provenance/builder/solver_config/ranges/expected+u_ref/tags/status/supersedes) → schema has all 11 keys; Task 2 fills them; `supersedes: null` on all six.
- §10.5 model-form spot checks → Task 4 structural runs with `not_applicable` honesty where the corpus can't exercise them (lapse tail, spin) instead of fake coverage.
- §10.6 missing-evidence rules (unknown η, no field data, lapse-gate visibility) → `u_ref: "unknown"` everywhere, all-provisional statuses, docs pointer states both gaps.
- §14 (extend test/source/, LOB-only, no Impl) → one CMake line + one define; reader/test public-only.
- §15 (JSON+CSV, provenance header, raw-vs-curated) → case files are curated inputs (checked in); `envelope_report.json` carries the §15.2 provenance header; per-case CSV rows for plotting.
- §17 (fast gate, offline matrix, promotion) → Tasks 3 (hermetic read-only CI) + 4 (gated offline); transcription review rule (diff-every-number) is the Task 2 promotion procedure.
- §19.4 acceptance → Task 5 snapshot. §20 Phase 3 row → files/tests/artifacts/CI/cost covered (offline cost: seconds — six solves).
- GAP CHECK: §10.3 "velocity regime" tags use Mach approximations in the plan table (1150–1440 fps ≈ Mach 1.0–1.3). Speed of sound varies per case (1116 ISA nominal); the tags say supersonic-with-transonic-tail, which holds across all six cases' 3000-ft velocities. Fine. §15.2 `inputs_hash` — case files hash the builder JSON canonically? The plan's provenance carries version+SHA but not a canonical-inputs hash. FIX: `envelope_report.json` records the factory name + solver_config per case (already in schema via per-case solver_config echo) — sufficient for Phase 3; canonical hashing arrives with the Phase 4 budget schema. Stated, not hidden.

**2. Placeholder scan:** no TBD/TODO/later in deliverables; `transcribed_utc` and provenance filled in-task; `"unknown"` u_ref is spec-mandated vocabulary, not a placeholder; `envelope_report.json` from the real run.

**3. Type consistency:** `nlohmann::json::at()` throws (never inserts — matches fail-loud test policy); `LOB_VALIDATION_CASES_DIR` quoted define consumed bare (Phase 1 pattern); test-target json linkage mirrors `example/lobber` (`find_package` + FetchContent v3.12.0 fallback, `target_link_libraries(lob_test PRIVATE nlohmann_json::nlohmann_json)`); `OutputNear`/`VerifySolutions` reuse needs `SolutionTolerances` — already in `testing.hpp`; `std::ifstream` read-only in CI test is input access, not artifact writing (hermetic holds — nothing is created/modified/deleted).

---

*End of plan. Phase 4 (uncertainty budgets) plan is written after Phase 3 cases merge, per spec Appendix A.*
