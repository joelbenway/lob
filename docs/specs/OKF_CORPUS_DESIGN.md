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

```text
docs/okf/
  index.md                  # root, @mainpage source, okf_version frontmatter
  getting_started.md
  ballistic_model/index.md  # nav-only
  ballistic_model/point_mass.md
  ballistic_model/drag.md
  ballistic_model/atmosphere.md
  ballistic_model/wind_coriolis.md
  ballistic_model/spin.md
  numerical_methods/index.md
  numerical_methods/ode.md
  numerical_methods/splines.md
  numerical_methods/zero_angle.md
  numerical_methods/inverse.md
  bc/index.md
  bc/overview.md
  bc/transformation.md
  bc/worked_example.md
  design/index.md
  design/overview.md
  design/shared_solver.md
  validation/index.md
  validation/overview.md
  reference/index.md
  reference/constants.md
  log.md                    # optional; Git history is source of truth
```

Directory = chapter. File path minus `.md` = concept ID. Doxygen `@page`
IDs derive from concept IDs via the converter spec (`okf_` prefix).

## Types (4 only)

- `Guide` — getting started, install/use paths.
- `Concept` — drag, atmosphere, point-mass, splines, ODE integration.
- `DesignDecision` — hourglass C ABI, strong units, no-alloc, builder, Heun.
- `Reference` — constants, tables.

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
3. Bodies use bundle-relative links: `[splines](../numerical_methods/splines.md)`.
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
