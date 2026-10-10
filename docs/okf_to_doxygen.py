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


MD_LINK_RE = re.compile(r"\[([^\]]+)\]\(([^)\"']+?)(?:\s+\"[^\"]*\")?\)")
RESERVED = {"index.md", "log.md", "SKILL.md"}


def _is_reserved(path):
    return Path(path).name in RESERVED


def rewrite_links(body, src_rel, id_map):
    """Rewrite '[t](other.md)' -> '@ref <target-id> "t"'.

    Leaves existing '@ref ...' text, external/absolute links, and links
    to reserved files (index.md, log.md, SKILL.md) untouched. Raises ValueError
    on a relative .md target missing from id_map.
    """
    src_dir = Path(src_rel).parent

    def sub(m):
        text, target = m.group(1), m.group(2)
        if not target.endswith(".md"):
            return m.group(0)
        if target.startswith(("http://", "https://", "#", "/")):
            return m.group(0)
        if _is_reserved(target):
            return m.group(0)
        norm = str((src_dir / target).as_posix())
        # collapse './' and 'a/../' segments without touching the fs
        parts = []
        for seg in norm.split("/"):
            if seg in ("", "."):
                continue
            if seg == "..":
                if parts:
                    parts.pop()
                continue
            parts.append(seg)
        key = "/".join(parts)
        if key not in id_map:
            raise ValueError(
                "dangling link %r in %s" % (target, src_rel))
        return '@ref %s "%s"' % (id_map[key], text)

    return MD_LINK_RE.sub(sub, body)


def collect_concepts(src_dir):
    """Return {bundle_rel: path} for non-reserved .md files, sorted."""
    found = {}
    for path in sorted(src_dir.rglob("*.md")):
        rel = path.relative_to(src_dir).as_posix()
        if _is_reserved(rel):
            continue
        found[rel] = path
    return found


def generate(src_dir, out_dir):
    """Render every concept to '<page_id>.md' in out_dir; return IDs."""
    src_dir, out_dir = Path(src_dir), Path(out_dir)
    concepts = collect_concepts(src_dir)
    id_map = {rel: page_id_for(rel) for rel in concepts}
    out_dir.mkdir(parents=True, exist_ok=True)
    written = []
    for rel in sorted(concepts):
        fm, body = parse_frontmatter(
            concepts[rel].read_text(encoding="utf-8"))
        body = rewrite_links(body, rel, id_map)
        pid = id_map[rel]
        (out_dir / (pid + ".md")).write_text(
            render_page(pid, fm, body), encoding="utf-8")
        written.append(pid)
    # root index.md becomes the @mainpage
    root_index = src_dir / "index.md"
    if root_index.exists():
        text = root_index.read_text(encoding="utf-8")
        m = FRONTMATTER_RE.match(text)
        if m:
            text = text[m.end():]
        text = rewrite_links(text, "index.md", id_map)
        (out_dir / "okf_mainpage.md").write_text(
            "@mainpage Overview\n" + text, encoding="utf-8")
    return written


def _index_targets(src_dir):
    """All .md link targets reachable from any index.md, as bundle rels."""
    targets = set()
    for index in sorted(src_dir.rglob("index.md")):
        text = index.read_text(encoding="utf-8")
        if index != src_dir / "index.md":
            if text.startswith("---"):
                continue  # reported by check; skip link scan
        base = index.parent
        for m in MD_LINK_RE.finditer(text):
            target = m.group(2)
            if not target.endswith(".md"):
                continue
            if target.startswith(("http://", "https://", "#", "/")):
                continue
            try:
                rel_target = (base / target).relative_to(src_dir).as_posix()
            except ValueError:
                continue  # escapes the bundle; rewrite pass reports it
            parts = []
            for seg in rel_target.split("/"):
                if seg in ("", "."):
                    continue
                if seg == "..":
                    if parts:
                        parts.pop()
                    continue
                parts.append(seg)
            targets.add("/".join(parts))
    return targets


def check_bundle(src_dir):
    """Validate bundle; return error strings (warnings go to stderr)."""
    src_dir = Path(src_dir)
    errors = []
    concepts = collect_concepts(src_dir)
    for rel, path in sorted(concepts.items()):
        try:
            fm, _ = parse_frontmatter(
                path.read_text(encoding="utf-8"))
        except ValueError as exc:
            errors.append("%s: %s" % (rel, exc))
            continue
        if fm.get("status") == "stable" and not fm.get("verified"):
            print("warning: %s stable without verified" % rel,
                  file=sys.stderr)
    id_map = {rel: page_id_for(rel) for rel in concepts}
    for rel, path in sorted(concepts.items()):
        try:
            _, body = parse_frontmatter(
                path.read_text(encoding="utf-8"))
            rewrite_links(body, rel, id_map)
        except ValueError as exc:
            errors.append("%s: %s" % (rel, exc))
    for index in sorted(src_dir.rglob("index.md")):
        rel = index.relative_to(src_dir).as_posix()
        if index != src_dir / "index.md" and \
                index.read_text(encoding="utf-8").startswith("---"):
            errors.append("%s: non-root index.md must not have frontmatter"
                          % rel)
    for rel in sorted(concepts):
        if rel not in _index_targets(src_dir):
            errors.append("%s: not reachable from any index.md" % rel)
    return errors


def main(argv):
    if len(argv) == 2 and argv[0] == "--check":
        errors = check_bundle(Path(argv[1]))
        for err in errors:
            print("error: " + err)
        return 1 if errors else 0
    if len(argv) == 2:
        for pid in generate(Path(argv[0]), Path(argv[1])):
            print("wrote " + pid)
        return 0
    print("usage: okf_to_doxygen.py [--check <src>] | [<src> <out>]")
    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
