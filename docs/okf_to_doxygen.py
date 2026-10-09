#!/usr/bin/env python3
# Copyright (c) 2026  Joel Benway
# SPDX-License-Identifier: GPL-3.0-or-later
"""OKF bundle -> Doxygen pages. Stdlib only (pathlib, re, sys)."""
import re
import sys
from pathlib import Path

FRONTMATTER_RE = re.compile(r"\A---\n(.*?)\n---\n?", re.DOTALL)


def parse_frontmatter(text):
    """Split '---\\nYAML\\n---\\nbody' into (dict, body).

    Minimal 'key: value' reader sufficient for our 4 types; raises
    ValueError on missing block or empty/missing 'type'.
    """
    m = FRONTMATTER_RE.match(text)
    if not m:
        raise ValueError("missing YAML frontmatter block")
    fm = {}
    for line in m.group(1).splitlines():
        if not line.strip() or line.strip().startswith("#"):
            continue
        if ":" not in line:
            raise ValueError("unparseable frontmatter line: %r" % line)
        key, _, value = line.partition(":")
        fm[key.strip()] = value.strip().strip("\"'")
    if not fm.get("type"):
        raise ValueError("frontmatter requires non-empty 'type'")
    return fm, text[m.end():]


def page_id_for(relpath):
    """Map 'a/b-c.d.md' -> 'okf_a_b_c_d' (spec verbatim)."""
    stem = str(relpath)
    if stem.endswith(".md"):
        stem = stem[:-3]
    return "okf_" + re.sub(r"[/\\\-.]+", "_", stem)


def render_page(page_id, frontmatter, body):
    """Emit '@page <id> <title>' header plus body verbatim."""
    title = frontmatter.get("title") or page_id
    return "@page %s %s\n%s" % (page_id, title, body)


def main(argv):
    print("generate not yet implemented (Task 2)")
    return 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
