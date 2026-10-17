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

Every concept file needs YAML frontmatter with non-empty `type`.
Recommended: `title`, `description`. Optional v0.2: `tags`, `resource`,
`sources`, `generated: {by, at}`, `verified: [{by, at}]`,
`status: draft|stable|deprecated`, `stale_after`.

## Rules

- One file per Wikipedia page. A concept is a topic, not a section: do
  not split by heading, and do not split unless each half stands alone
  as an encyclopedia article.
- One concept per file, and the bundle is flat: no subdirectories. OKF
  assigns no meaning to directory structure, so a filename must carry
  its own subject.
- A filename is the kebab-case slug of the concept's `title`, lowercased
  with `-` for spaces. Path minus `.md` is the concept ID.
- The root `index.md` is the only index. It holds the whole reading
  order, grouped into sections by topic; link order within a section is
  the reading order. No frontmatter except `okf_version: "0.2"`.
- Bodies are high-level: no `source/` or `include/` paths, no test
  paths. Link concepts with `[text](relative/other.md)`; `@ref` is
  reserved for the generated API reference and no longer needed here.
- `log.md` is optional; Git history is the source of truth.
- Non-concept files (e.g. SKILL.md) go in root index.md `exclude`.

## Workflow

```bash
python3 docs/okf_to_doxygen.py --check docs/okf   # validate, exit 0 = clean
python3 docs/okf_to_doxygen.py docs/okf /tmp/okf_pages  # preview output
```

Keep `stable` concepts `verified`; warn (don't fail) when missing.
