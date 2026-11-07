# Solver Header Links and Title Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Keep the solver’s current theme while moving Documentation and Benchmarks to the header and updating the title.

**Architecture:** Add a small header navigation block and footer/title copy changes in the existing static demo. A focused static-source regression test pins the exact labels, destinations, title, footer text, and required CSS hooks.

**Tech Stack:** Static HTML/CSS, Node test runner, existing `npm` checks/build.

**Spec:** `docs/specs/2026-10-28-solver-header-design.md`

## Global Constraints

- Preserve the current hero styling, palette, typography, dark mode, and sticky output behavior.
- Change only header title/links, header-link styling needed to match the theme, and footer links.
- Use these exact destinations:
  - Documentation: `https://joelbenway.github.io/lob/docs/`
  - Benchmarks: `https://joelbenway.github.io/lob/bench`
  - Footer `lob` link: `https://github.com/joelbenway/lob`
- Use this exact visible title: `Ballistics Explorer`.
- Make the footer text: `Built on lob · docs · Benchmarks.`
- Verify 1440 px, 500 px, and 390 px widths in light and dark modes.

## Review Focus

- Wrong Documentation destination or a moved docs page: Documentation must open the published technical reference.
- Wrong Benchmarks destination or a missing bench page: Benchmarks must open the published benchmark page.
- Header links overflow, clip, or wrap badly at 390 px: both links must remain visible and tappable.
- New header links have insufficient contrast or missing focus indication in either theme: they must match existing accent styling and show focus.
- Footer links: the footer must preserve links to lob, docs, and Benchmarks.

## File Structure

- Modify: `example/solver/index.html:10`, `example/solver/index.html:14-21`, `example/solver/index.html:695-700`
- Modify: `example/solver/style.css` hero rules near lines 37-63
- Create: `example/solver/test/header.test.mjs`
- Read-only verification: built `build/wasm/solver/index.html`

The line numbers are starting points; keep the surrounding theme and structure unchanged.

---

### Task 1: Header title, top links, and footer links

**Files:**
- Create: `example/solver/test/header.test.mjs`
- Modify: `example/solver/index.html`
- Modify: `example/solver/style.css`
- Test: `example/solver/test/header.test.mjs`

**Interfaces:**
- Consumes: existing `.hero`, footer, accent variables, and dark-mode rules.
- Produces:
  - Header nav markup with accessible name `Solver resources`.
  - Hero title markup: `<h1>Ballistics Explorer</h1>`.
  - Footer markup preserving links to lob, docs, and Benchmarks.
  - CSS hooks `.hero-links` and themed `.hero a` styling.

- [ ] **Step 1: Write the failing regression test**

Create `example/solver/test/header.test.mjs`:

```js
import assert from "node:assert/strict";
import test from "node:test";
import { readFileSync } from "node:fs";

const html = readFileSync(new URL("../index.html", import.meta.url), "utf8");
const css = readFileSync(new URL("../style.css", import.meta.url), "utf8");
const flat = html.replace(/\s+/g, " ");

test("solver header exposes top-level resources and title", () => {
  assert.ok(flat.includes("<title>Ballistics Explorer</title>"));
  assert.ok(
    flat.includes(
      '<nav class="hero-links" aria-label="Solver resources"> <a href="https://joelbenway.github.io/lob/docs/">Documentation</a> <a href="https://joelbenway.github.io/lob/bench">Benchmarks</a> </nav>',
    ),
  );
  assert.ok(/<h1>\s*Ballistics Explorer\s*<\/h1>/.test(flat));
  assert.ok(css.includes(".hero-links"));
  assert.ok(css.includes(".hero a"));
  assert.ok(
    /<footer class="footer">\s*<p>\s*Built on\s*<a href="https:\/\/github\.com\/joelbenway\/lob">lob<\/a>\s*·\s*<a href="https:\/\/joelbenway\.github\.io\/lob\/docs\/">docs<\/a>\s*·\s*<a href="https:\/\/joelbenway\.github\.io\/lob\/bench">Benchmarks<\/a>\s*<\/p>\s*<\/footer>/.test(
      flat,
    ),
  );
});
```

- [ ] **Step 2: Run the new test and verify it fails**

Run:

```sh
node --test example/solver/test/header.test.mjs
```

Expected: FAIL because the header nav and CSS hooks are absent.

- [ ] **Step 3: Implement the header, title, footer, and matching styles**

In `example/solver/index.html`:

1. Change document title to:
   `Ballistics Explorer`
2. Insert this nav as the first child of `.hero`:
   ```html
   <nav class="hero-links" aria-label="Solver resources">
     <a href="https://joelbenway.github.io/lob/docs/">Documentation</a>
     <a href="https://joelbenway.github.io/lob/bench">Benchmarks</a>
   </nav>
   ```
3. Change the hero heading to:
   ```html
   <h1>Ballistics Explorer</h1>
   ```
4. Replace the footer paragraph with:
   ```html
   <p>
     Built on <a href="https://github.com/joelbenway/lob">lob</a> ·
     <a href="https://joelbenway.github.io/lob/docs/">docs</a> ·
     <a href="https://joelbenway.github.io/lob/bench">Benchmarks</a>
   </p>
   ```

In `example/solver/style.css`, add only theme-matching rules for `.hero-links` and `.hero a`, including wrapping, right alignment, accent color, and a visible focus outline.

- [ ] **Step 4: Run the new test and verify it passes**

Run:

```sh
node --test example/solver/test/header.test.mjs
```

Expected: PASS.

- [ ] **Step 5: Verify destinations, checks, build, and responsive header**

Run:

```sh
curl -s -o /dev/null -w 'docs %{http_code}\n' https://joelbenway.github.io/lob/docs/
curl -s -o /dev/null -w 'bench %{http_code}\n' https://joelbenway.github.io/lob/bench
npm test
npm run lint
npm run format:check
npm run build:wasm
```

Expected: both URLs return `200`; all checks and the build pass.

Then visually inspect the rebuilt demo at 1440 px, 500 px, and 390 px in light and dark modes. Confirm both header links are visible, the title is "Ballistics Explorer", the current theme is unchanged, and the footer preserves links to lob, docs, and Benchmarks.

- [ ] **Step 6: Commit**

```sh
git add example/solver/index.html example/solver/style.css example/solver/test/header.test.mjs docs/specs/2026-10-28-solver-header-design.md docs/specs/2026-10-28-solver-header-plan.md
git commit -m "demo: move resource links to solver header"
```
