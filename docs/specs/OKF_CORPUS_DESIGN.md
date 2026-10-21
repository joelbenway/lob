# OKF Corpus Design

Date: 2026-10-09
Status: Draft

## Goal

Replace the code-coupled pages under `docs/pages/` (~24 files referencing
`source/*`, `include/lob/*`) with a high-level OKF v0.2 bundle at
`docs/okf/` that explains why lob is built the way it is, what the
features are, and what decisions went into it. Easier to maintain because
it does not track refactors.

## Conformance

OKF v0.2 full. Every concept file: UTF-8 markdown with YAML frontmatter,
only required key `type`. Bundle root `index.md` may carry
`okf_version: "0.2"`; subdir `index.md` files carry no frontmatter
(nav-only). Consumers tolerate unknown keys/types; broken links are
stubs, but CI fails on them (see converter spec).

Spec source: https://github.com/GoogleCloudPlatform/open-knowledge-format/blob/main/SPEC.md
(referenced by URL, not vendored).

## Layout

The bundle is flat. No subdirectories.

```text
docs/okf/
  index.md                  # root, @mainpage source, okf_version frontmatter
  SKILL.md                  # agent skill, listed in root index `exclude`
  <concept>.md              # one file per concept, filename = slug of its title
  log.md                    # optional; Git history is source of truth
```

Two rules make a flat bundle work, and both follow from OKF itself:
the spec assigns no meaning to directory structure, so a file has to
identify its subject unaided. Hence the second rule.

**A filename is the kebab-case slug of the concept's `title`.** With no
directory to supply context, `overview.md`, `transformation.md`, and
`worked_example.md` are unmoored: each said nothing about which subject
it belonged to, and the same three names would have collided again in
the next topic. `bc-velocity-band-transformation.md` and
`point-mass-model.md` cannot collide and cannot be misfiled.

**The root `index.md` is the only index.** It carries the whole reading
order, grouped into sections by topic (OKF §8 index format). Per-chapter
`index.md` files are unnecessary in a flat bundle and would collide on
name. Link order inside each section is the reading order for that
topic.

Concept ID = file path minus `.md`. Doxygen `@page` IDs derive from
concept IDs via the converter spec (`okf_` prefix). Renaming a file
therefore renames its page ID; since generated pages are not published
until a release, mid-migration renames are cheap.

## One file per Wikipedia page

An OKF concept is a topic, not a section. Split a page only when each
half would stand on its own as an encyclopedia article; do not split by
heading, by paragraph, or by "someone might link to that bit". Sections
inside a page may freely reference each other and assume the reader has
the whole page in hand.

Worked example: the angle solver was first split into four files
(problem, iteration, seeds, evidence). None of the four would be a
Wikipedia article, and each was unreadable alone. Folding them back into
one page was correct. `heun.md`, `builder.md`, and `no-alloc.md` each
would be, so they stay separate.

## Types

- `Guide` — getting started, install/use paths.
- `Concept` — drag, atmosphere, point-mass, splines, ODE integration.
- `DesignDecision` — hourglass C ABI, strong units, no-alloc, builder, Heun.

Frontmatter per concept:

```yaml
---
type: Concept               # REQUIRED, one of the 4 above
title: Drag Model           # recommended
description: How Cd(Mach) curves become splines  # one sentence
tags: [drag, splines]       # optional
resource:                   # optional URI; absent for abstract ideas
sources:                    # optional v0.2 provenance
generated: {by: human:joel, at: 2026-10-09T00:00:00Z}
verified: [{by: human:joel, at: 2026-10-09T00:00:00Z}]
status: stable              # draft | stable | deprecated, absent = stable
stale_after:                # optional absolute instant
---
```

## Map (index.md is the map)

OKF stores a graph (markdown links); Doxygen shows a tree (@page/@ref).
The map is:

1. `index.md` link order in each dir = human reading order. Root
   `index.md` becomes `@mainpage` (replaces `docs/pages/overview.md`).
2. No separate `nav.yaml`. If agent disclosure order and human reading
   order ever diverge, add one then.
3. Bodies use bundle-relative links: `[Cubic Hermite Splines](cubic-hermite-splines.md)`.
   Converter rewrites to `@ref` (see converter spec).
4. No code paths (`source/...`, `include/...`) in OKF bodies. Link to API
   concepts via reserved `api_*` IDs only.

## Authoring rules

- One concept per file. Prefer structural markdown (headings, lists,
  tables, fenced blocks) over prose.
- `index.md` files are navigation, not content. No concept body in them.
- Bodies stay high-level: physics, numerics, rationale, evidence pointers.
  Test/spec citations go in `sources` frontmatter, not inline file paths.

## Migration

Manual rewrite, not auto-convert — auto-convert would carry over the
code refs this exists to kill. `docs/pages/` frozen during migration;
flip Doxygen INPUT once `docs/okf/` covers all chapters, then delete
`docs/pages/`.
