# OKF-to-Doxygen Converter Design

Date: 2026-10-09
Status: Draft

## Goal

Translate the `docs/okf/` bundle back into pages Doxygen publishes
alongside the API reference from `include/`. Thin generator, stdlib
only, no new dependencies.

## Tool

`docs/okf_to_doxygen.py` — Python 3 stdlib only (`pathlib`, `re`,
no PyYAML; frontmatter parsed with a minimal `key: value` reader
sufficient for our 4 types).

```text
in:  docs/okf/**/*.md (excluding index.md, log.md)
out: build/generated/okf_pages/*.md
```

Per concept:

1. Parse frontmatter (fail on unparseable block or empty `type`).
2. Page ID = `okf_` + path sanitize (`/`/`-`/`.` → `_`, strip `.md`).
   Example: `ballistic_model/drag.md` → `okf_ballistic_model_drag`.
   Prefix avoids collision with legacy `model_*`/`api_*` during
   migration; drop after `docs/pages/` is deleted.
3. Emit `@page <id> <title>` header (`title` from frontmatter, fallback
   filename), then body verbatim.
4. Rewrite `[text](relative/other.md)` → `@ref <target-id> "text"`.
   `@ref api_*` targets (from `include/`) pass through untouched.
5. Root `docs/okf/index.md` → `@mainpage` page. Subdir `index.md`
   link order → `@subpage` ordering in parent page.

No trust-table rendering in v1 (provenance stays in frontmatter only).

## Validation (`--check`)

Same script, CI-usable, exit non-zero on:

- Non-reserved `.md` without parseable frontmatter or empty `type`.
- Reserved `index.md`/`log.md` violating structure (subdir index with
  frontmatter, bad `YYYY-MM-DD` headings in `log.md`).
- Dangling internal link or concept not reachable from any `index.md`.
- `stable` without `verified` is a warning, not an error (v1).
- `SKILL.md` (exact basename, any dir) is exempt from all checks, like
  reserved files — it is agent tooling, not a concept.

## Docs build integration

- `cmake/docs.cmake` + `cmake/docs-ci.cmake`: run
  `python3 docs/okf_to_doxygen.py docs/okf build/generated/okf_pages`
  before Doxygen.
- `docs/Doxyfile.in`: `INPUT` changes from `include + docs/pages` to
  `include + build/generated/okf_pages`.
- No Doxygen version bump; output still deployed to gh-pages `docs/`
  via existing `docs` CI job.

## Skill (own, opencode-only)

`docs/okf/SKILL.md` — written from scratch under GPL-3.0-or-later
(no vendored Apache-2.0 code; SPEC referenced by URL). Covers: when
to use OKF, our 4 types, `index.md`-is-map rule, link conventions,
converter workflow. Symlinked for opencode only:

```bash
mkdir -p .opencode/skills/okf
ln -s ../../../docs/okf/SKILL.md .opencode/skills/okf/SKILL.md
```

No Claude/Codex/Cursor shims in v1.

## CI

- `lint` job: `python3 docs/okf_to_doxygen.py --check docs/okf`.
- `docs` job: generate → Doxygen → gh-pages (existing flow).

## Skipped

Custom `nav.yaml` (duplicates `index.md`), trust/lifecycle header
tables, `stale_after` enforcement, npm/wasm demo embedding — later
specs. Add when `index.md` order genuinely diverges from agent
disclosure needs or stale docs cause real confusion.

// ponytail: stdlib frontmatter parser, full YAML lib if schema grows
