# lob OKF Skill

Work on the lob knowledge bundle in `docs/okf/` (Open Knowledge Format
v0.2, spec at
https://github.com/GoogleCloudPlatform/open-knowledge-format/blob/main/SPEC.md).
Use when the user mentions OKF, knowledge bundle, or editing technical
reference content.

## Types (only these four)

- `Guide`: install/use paths.
- `Concept`: physics, numerics, features.
- `DesignDecision`: why lob is shaped this way.
- `Reference`: constants, tables.

Every concept file needs YAML frontmatter with non-empty `type`.
Recommended: `title`, `description`. Optional v0.2: `tags`, `resource`,
`sources`, `generated: {by, at}`, `verified: [{by, at}]`,
`status: draft|stable|deprecated`, `stale_after`.

## Rules

- One concept per file. Path minus `.md` is the concept ID.
- `index.md` files are navigation only, no frontmatter except
  `okf_version: "0.2"` at the bundle root. Link order is reading order.
- Bodies are high-level: no `source/` or `include/` paths. Link APIs via
  `@ref api_*` passthrough, concepts via `[text](relative/other.md)`.
- `log.md` is optional; Git history is the source of truth.
- Non-concept files (e.g. SKILL.md) go in root index.md `exclude`.

## Workflow

```bash
python3 docs/okf_to_doxygen.py --check docs/okf   # validate, exit 0 = clean
python3 docs/okf_to_doxygen.py docs/okf /tmp/okf_pages  # preview output
```

Keep `stable` concepts `verified`; warn (don't fail) when missing.
