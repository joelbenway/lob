# Conformance Remediation — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Bring every validation file to clang-tidy-clean (`-p build/dev`, exit 0) under the repo's `.clang-tidy` (`*`, `WarningsAsErrors: '*'`), following existing-code conventions. Format, spell, and cppcheck already pass and must stay passing.

**Architecture:** One task per TU (+ shared headers in Task 1); each task's gate is `clang-tidy -p build/dev <file>` exit 0 plus rebuild + focused tests green. No semantic changes: test logic, constant VALUES, assertions, and artifact schemas stay byte-identical; only style/conformance changes.

**Tech Stack:** C++14, clang-tidy 19.1.7 (repo config), existing test patterns as reference.

## Global Constraints

- C++14 only; no new dependencies; `lob_lob` diff stays empty (test files + test headers only).
- NO semantic changes: no assertion changes, no constant VALUE changes, no test renames beyond the one mandated `Underscore` fix, no schema changes. A task that changes what a test verifies fails review.
- NOLINT is allowed in EXACTLY two shapes (everything else must be a proper fix):
  1. `// NOLINTNEXTLINE(readability-function-cognitive-complexity)` on over-threshold TEST bodies (4 existing precedents) and, as a justified extension established in Task 3, on large driver helpers where splitting would risk semantics (refactor-instead remains preferred where low-risk, as Task 2 demonstrated).
  2. `// NOLINTNEXTLINE(concurrency-mt-unsafe)` on the 5× `std::getenv` gate lines, each with a reason comment (`// single-threaded gtest; env gates select offline drivers`) placed so clang-format cannot split the suppression apart (above-line if the combined line would exceed 100 cols — Task 2 established this placement tidies clean). No repo precedent exists; no conformant alternative exists (the gates are load-bearing, gtest is single-threaded by design). This is a justified exception, not a habit.
- Deterministic tests stay deterministic; no behavior change.

## Fix Playbook (established empirically against this repo — apply, don't reinvent)

- Bare literals in expressions → named `kName` constants (`lob_env_test.cpp` fixture pattern: `const double kTestBC = 0.232;`). Initializing a named constant is exempt; bare arguments/assignments fire. This also clears `bugprone-argument-comment` hits.
- `arr[i]` with runtime index → `arr.at(i)` (`calc_test.cpp` pattern).
- Loop vars `n/c/r/a` → `i`/`j`/`k` or descriptive ≥2-char names (`i` is exempt in existing files).
- Non-const `kName` locals → `lower_case`; add `const` where `misc-const-correctness` asks.
- Free functions in `tests` scope → anonymous namespace (existing `namespace tests { namespace {` pattern) or `static`.
- Structs: initialize every field (`pro-type-member-init`).
- Includes: add what `misc-include-cleaner` demands, remove what it flags unused (verify build still passes — cleaner is occasionally wrong about gtest macros).
- Math: explicit parens (`(2.0 * x) + 1.0`); braces on all single-statement `if`s; `std::min`/`std::max`; `emplace_back`; `ostringstream` over `snprintf`/`printf` (no `printf`/`cout` precedent in tests — `std::cout` for SURVEY lines, the only conformant output facility); raw strings `R"(...)"` for escaped JSON literals.
- `for` loops convertible to range-for → convert (`modernize-loop-convert` only flags convertible ones).
- `UnderscoreInGoogletestName` test → rename without underscore (one instance).
- `use-auto`, `const-correctness`, `string-concatenation`, `inefficient-vector-operation` → apply the suggested fix mechanically.
- `validation_io.hpp` `char buf[32]` + `snprintf` → `std::ostringstream` with precision (file already uses ostringstream elsewhere).

## File Structure

```
test/source/testing.hpp                       Task 1 (C1-helper + math-helper additions)
test/source/validation_io.hpp                 Task 1 (writer header)
test/source/validation_sensitivity.hpp        Task 1 (math header)
test/source/validation_convergence_angle_test.cpp  Task 1 (7 findings; proves the loop)
test/source/validation_budget_test.cpp        Task 2 (~109 findings)
test/source/validation_sensitivity_test.cpp   Task 3 (~113 findings)
test/source/validation_reference_test.cpp     Task 4 (~135 findings)
test/source/validation_convergence_test.cpp   Task 5 (~215 findings)
```

Why headers first: every TU includes them; fixing headers first shrinks all downstream outputs. Task 1 gates on the angle TU (smallest) reaching exit 0 — which transitively proves the headers clean as included there — plus explicit header re-checks in later tasks' outputs.

---

### Task 1: Shared headers + angle test (proves the loop)

**Files:** `test/source/testing.hpp`, `test/source/validation_io.hpp`, `test/source/validation_sensitivity.hpp`, `test/source/validation_convergence_angle_test.cpp`. No CMake changes.

- [ ] **Step 1: Run the gate to record the baseline** — `clang-tidy -p build/dev test/source/validation_convergence_angle_test.cpp` → save error count (expect ~7 + header-attributed lines from including TUs later; headers surface via their includers).
- [ ] **Step 2: Apply the playbook** — named constants for the six C1 fixture literals; drop unused `<cstdint>` if cleaner demands (verify build); fix every header-attributed finding in the three headers (member-init, naming, includes, loops, parens, raw strings, ostringstream, linkage).
- [ ] **Step 3: Verify** — `clang-tidy -p build/dev test/source/validation_convergence_angle_test.cpp` exit 0 AND `cmake --build --preset=dev --target lob_test` clean AND `./build/dev/test/lob_test --gtest_filter='ValidationAngleConvergence.*:ValidationMath.*'` PASS. Then confirm zero header-attributed lines remain in the tidy output (grep the output for `testing.hpp|validation_io.hpp|validation_sensitivity.hpp` → no `error:` lines).
- [ ] **Step 4: Commit** — `git add` the four files; `style: conform shared validation headers and angle test to clang-tidy`.

### Task 2: Budget test (~109 findings)

**Files:** `test/source/validation_budget_test.cpp`. Same 4 steps with gate `clang-tidy -p build/dev test/source/validation_budget_test.cpp` exit 0 + `Budget*` tests green. Commit `style: conform budget validation test to clang-tidy`.

### Task 3: Sensitivity test (~113 findings)

**Files:** `test/source/validation_sensitivity_test.cpp` (+ `validation_sensitivity.hpp` leftovers if any surface). Gate: tidy exit 0 + `Sensitivity*` green. Includes the `Underscore` rename, printf-free SURVEY lines (already `std::cout`? verify — the survey prints via printf per the vararg hits → convert), getenv NOLINTNEXTLINE (1 site). Commit `style: conform sensitivity validation test to clang-tidy`.

### Task 4: Reference test (~135 findings)

**Files:** `test/source/validation_reference_test.cpp`. Gate: tidy exit 0 + `Reference*` green. Includes braces-around-statements (24), use-std-min-max (22), avoid-c-arrays (4 — check what they are; likely local `char[]` or C arrays → `std::array`), cognitive-complexity NOLINTNEXTLINE (precedent shape), getenv NOLINTNEXTLINE (1 site). Commit `style: conform reference validation test to clang-tidy`.

### Task 5: Convergence test (~215 findings)

**Files:** `test/source/validation_convergence_test.cpp`. Gate: tidy exit 0 + `Validation*` green. The bulk is `pro-bounds-constant-array-index` (113 → `.at()`) + magic numbers (39 → named constants) + vararg printf→cout (25, SURVEY + ladder prints) + min-max (23) + getenv NOLINTNEXTLINE (2 sites) + complexity NOLINTNEXTLINE (precedent shape). Commit `style: conform convergence validation test to clang-tidy`.

### Task 6: Full gate + process fix

**Files:** none (verification only).
- [ ] **Step 1: All TUs tidy-clean** — run `clang-tidy -p build/dev` over all five validation TUs → exit 0 each.
- [ ] **Step 2: format-fix + spell + cppcheck** — `cmake -D FORMAT_COMMAND=clang-format -DFIX=YES -P cmake/lint.cmake` (check the FIX flag name in `cmake/lint-targets.cmake` first — it passes `-D FIX=YES`), `cmake -P cmake/spell.cmake`, cppcheck with the CI flags on the touched files. Any NEW findings from remediation edits get fixed in place (`style:` amend or follow-up).
- [ ] **Step 3: Full ctest** — `ctest --preset=dev` 100% green (no assertion may have changed — suite counts identical to pre-remediation: record before/after test counts).
- [ ] **Step 4: Process fix (no commit — untracked plans)** — append to the coordinator ledger: future phase plans' Definition of Done includes `clang-tidy -p build/dev <touched files>` exit 0 before commit. (Phase 5/6 plans will carry it from birth.)

---

## Self-Review

**1. Coverage:** every finding category in the tidy outputs maps to a playbook rule; every file mapped to a task; headers owned by Task 1 with downstream confirmation; format/spell/cppcheck re-verified in Task 6 (they pass now — remediation must not regress them).
**2. Placeholder scan:** none — all steps are concrete commands with expected outputs.
**3. Risk:** the only judgment calls (getenv suppression, complexity suppression) are pre-decided with precedent/reasoning recorded in Global Constraints — implementers don't improvise, reviewers don't re-litigate.

---

*End of plan. Phase 5 execution follows remediation merge, with the tidy gate in its Definition of Done.*
