# Closure & Integration — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to execute the code-bearing tasks (Task 3 only). Docs/decision tasks are coordinator-executed with human gates — see Review Gradation below.

**Goal:** Close out the validation-framework + information-aware-stepping program: record the static-cap decision, sweep remaining review debt, commit the full docs record, push the branch through CI for the standing second-platform confirmation, and formally close or explicitly defer every §21 item. After this plan there is no tracked open work except human-owned inputs (sensor values, field data).

**Architecture:** No new machinery. One decision record, one minor-sweep code task, one docs commit, one push/CI watch, one spec-stamping task. The only judgment-heavy step is Task 1 (the cap call), which is human-made with a recorded recommendation — subagents execute, humans decide, same division as the whole program.

**Tech Stack:** C++14, existing gated drivers, git/gh CLI, Doxygen markdown.

## Global Constraints

- `lob_lob` library diff stays empty (the only code touched is `tools/lob_mc/main.cpp` warts in Task 3, plus test-only EXPECT/ASSERT promotions — no solver, no headers, no API).
- C++14, established style, `clang-tidy -p build/dev` exit 0 on any touched test/tool file before commit. NOLINT only in the two sanctioned shapes (none expected — NEEDS_CONTEXT if one is believed needed).
- Deterministic, no behavior changes except the intended EXPECT→ASSERT failure-mode tightening (fails earlier, same verdicts).
- Plan location: `docs/specs/CLOSURE_INTEGRATION_PLAN.md` (versioned with the other plans; joins the Task 4 docs pass).

## Review Gradation (binding — proportional assurance, not ceremony)

- **Subagent + reviewer loop:** Task 3 only (the one touching compiled code).
- **Coordinator diff-read:** Tasks 1, 2, 4 (docs/data/md + one-word-scale config). The coordinator reads every hunk before commit, same practice as all prior plan-file edits.
- **Human gates (cannot be delegated):** the 576/288 call in Task 1; the push/PR call in Task 5 (standing choice, still unmade); §21-item-7/11 ownership acknowledgment in Task 6.

---

## Scope

Closure of everything tracked open: the static-cap decision + mini-spec, carried review minors, the docs/specs/ commit pass, the branch push with CI matrix watch, §21 closeout, final acceptance. Explicitly NOT in scope: new measurements (second-platform confirmation arrives via CI, not new campaigns), the 288-vs-576 insurance question beyond the Task 1 call (re-measurement would be a new experiment proposal, not closure), field-data acquisition, sensor-value elicitation (human track, parallel).

---

### Task 1: Cap call + static-step mini-spec (human decision + decision record)

**Files:**
- Create: `docs/specs/STATIC_STEP_POLICY.md`
- Modify: `docs/pages/numerical_methods/ode.md` (one-paragraph user guidance) — fallback `docs/pages/validation/overview.md` if no natural anchor; implementer states which and why.

**Interfaces:** Consumes E3 numbers (cap 576 @1.21×, 288 @4.9×, veto margins, envelope bounds, unprobed-path list). Produces the rule the program converges on.

- [ ] **Step 1 (human, coordinator-led): adopt the cap.** Recommendation on record: **288 inches for publication, 576 recorded as measured-clean headroom.** Reasoning (state it in the doc, don't soften it): the pre-registered rule said 576 and 576 passed it — but the 1.21× governor margin plus unprobed mainstream paths (profile joints, Boatright, bands/custom, tails, inverse) make 576 a measurement, not yet a default. 288 carries 4.9× measured margin ≈ 49× effective, promotion to 576 follows automatically on second-platform + wider-envelope confirmation (§17.3 logic, no new proposal needed). If the human instead adopts 576, the doc records the rule-literal reasoning and the thin margin as accepted risk — either way the *reasoning* is the deliverable.
- [ ] **Step 2: Write STATIC_STEP_POLICY.md** — rule (within envelope → cap step; outside envelope → 36-in default; explicit `StepSize` always wins), envelope bounds verbatim from E3 scope (smooth forward solves ≤3000 ft + exclusion list), tier analysis as the rationale for static-over-adaptive (degeneracy proof summary: 36–54× u-span, everything pinned, floor-threatening u physically absurd), escape hatches, second-platform pending note, review-history links (E3 report, review, this decision).
- [ ] **Step 3: Doxygen pointer** — one paragraph user guidance ("what step to use"), method-pointer voice, no new numbers beyond the adopted cap.
- [ ] **Step 4: Commit with Task 4's docs pass** (not separately — one docs commit for the whole pass; see Task 4).

---

### Task 2: Carried-minor sweep (mechanical, code-bearing)

**Files:** `test/source/validation_convergence_test.cpp` + `test/source/validation_budget_test.cpp` (EXPECT_GT→ASSERT_GT ×2 — wait, inventory says two sites: Phase 1 Task 5 inverse precondition + Phase 1b C9 drop check; confirm both by grep, fix both or report why one differs); `tools/lob_mc/main.cpp` (wall-estimate ×workers factor; cap no-op branch → `std::min`; pilot_n records run_n on non-autoscale runs).

**Interfaces:** Consumes the ledger's carried-minor list (reproduced above — verify against `.superpowers/sdd/progress.md` at execution; anything already fixed stays fixed).

- [ ] **Step 1: Apply the five micro-fixes** (each one-line-scale; EXPECT→ASSERT changes failure timing only — same verdicts, earlier abort).
- [ ] **Step 2: Verify** — focused tests for both touched suites + `lob_mc --selfcheck` + `clang-tidy -p build/dev` exit 0 on touched files + full `ctest --preset=dev` green. Any red → fix-or-escalate, never loosen.
- [ ] **Step 3: Commit** — `git add test/source/validation_convergence_test.cpp test/source/validation_budget_test.cpp tools/lob_mc/main.cpp; git commit -m "style: sweep carried review minors (assert timing, cost-model warts)"`.

---

### Task 3: Docs commit pass (no code)

**Files:** `git add docs/specs/` (8 files: spec + 6 phase plans + remediation + experiments + mini-spec from Task 1) + `git checkout -- README.md` (revert the stray 2-line blurb — verify first it is still the same wind lines, not user content added since).

- [ ] **Step 1: Verify the README hunk** is still only the pre-existing wind blurb (`git diff README.md` — if new user content appeared, STOP and ask; do not revert чужое).
- [ ] **Step 2: Stage + review the docs list** (`git status --short docs/specs/` must show exactly the 8 expected files, nothing else).
- [ ] **Step 3: Commit** — `git commit -m "docs: add validation spec, phase plans, experiments, and step policy"`.
- [ ] **Step 4: Tree-clean proof** — `git status --short` shows nothing but intended state (branch commits + clean tree).

---

### Task 4: Push + PR + CI matrix watch (the standing gate)

**Files:** none (branch operation + observation).

- [ ] **Step 1: Push** (`git push -u origin feat/uncertainty` — first-ever push of this branch; confirm remote name `origin` at execution).
- [ ] **Step 2: Open PR** (`gh pr create` with the framework summary, or manual if gh is unavailable — record which).
- [ ] **Step 3: Watch the matrix**, specifically macos/windows legs = the §17.3b second-platform confirmation open since Phase 1b. Outcomes:
  - Green everywhere → record confirmation, close the carry. Done.
  - Ceiling trip on non-linux → widen per §17.3 (record provenance, margin recompute) as a SMALL follow-up commit if trivially mechanical (single-constant bump, same procedure as Phase 1 Task 4 Step 3); if non-trivial (systematic divergence, libm behavior differences needing analysis) → STOP, new investigation proposal, do NOT widen blindly.
- [ ] **Step 4: Record** CI outcome + platform confirmation status in the ledger (and in STATIC_STEP_POLICY.md if it upgrades the 576 evidence — amend only, with dated note).

---

### Task 5: §21 closeout + final acceptance (docs edits + snapshot)

**Files:** `docs/specs/NUMERICAL_VALIDATION_SPEC.md` (§21 item statuses only — no methodology changes).

- [ ] **Step 1: Stamp all 15 items** from this verified-starting table (implementer re-verifies each against the tree; correct where wrong, never assume):
  1 location — CLOSED (committed). 2 layout — CLOSED (as built). 3 MC home — CLOSED (`tools/lob_mc/`). 4 MC defaults — CLOSED (locked in Phase 5 plan). 5 ladder — CLOSED (36→1 + coarse extension built). 6 envelope cells — CLOSED (as built). 7 sensor values — OPEN, owner: human, vehicle: elicitation template (unchanged). 8 transcription — CLOSED (reviewed 432/432). 9 categoricals — CLOSED as degenerate-default (document the decision explicitly — currently implied; make it a sentence). 10 quantization — CLOSED (LSB floors). 11 field data — OPEN, owner: human, unclaimed cells stand. 12 no-global-k — CLOSED (upheld by design + Phase 6 bands). 13 wind cells — CLOSED (five-way split built). 14 wind MC sampling — CLOSED (implemented per §12.1). 15 wind u(x) — OPEN, same bucket as 7.
- [ ] **Step 2: Commit** the stamps — `git add docs/specs/NUMERICAL_VALIDATION_SPEC.md; git commit -m "docs: close out section 21 review items with dispositions"`.
- [ ] **Step 3: Final acceptance snapshot** — gate green (record count), docs complete (all §23 pages + claims + policy), branch pushed/PR'd, `lob_lob` diff empty (re-verify — closure tasks touched tools/tests only), framework roll-up sentence: all six phases + remediation + experiments + closure implemented, reviewed, green.
- [ ] **Step 4 (if PR merged later):** post-merge ledger note. Out of this plan's scope to execute — recorded as the terminal bookkeeping step.

---

## Self-Review

**1. Coverage:** every tracked-open item from the session maps to exactly one task — cap call (T1), minors (T2), docs+README (T3), push/CI/second-platform (T4), §21 + acceptance (T5), review gradation for each. Nothing from the findings thread is orphaned: 288/576 → T1; tier table → already in E3 report, referenced by T1 mini-spec (no separate task needed); E2 Build qualifier → already stamped in experiments doc; malformed build artifact → verified gitignored-local, no action (stated here so it isn't re-raised).
**2. No invented thresholds:** the only number this plan mints is none — cap comes from T1 human call between two measured options; minors change failure timing, not values; §21 stamps describe existing states.
**3. Honest conditionals:** T4's ceiling-trip branch (trivial → ride along; systematic → new proposal, never blind widening); T1's either-way cap recording; T5's verify-then-stamp table. No open ends disguised as closed ones; the two human-owned opens (sensor values, field data) stay open by design, with owners and vehicles named.

---

*End of plan. After this plan: the program is closed except human-owned inputs and merge execution. No Phase 7 exists and none is proposed.*
