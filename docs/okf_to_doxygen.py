#!/usr/bin/env python3
# Copyright (c) 2026  Joel Benway
# SPDX-License-Identifier: GPL-3.0-or-later
"""OKF bundle -> Doxygen pages. Stdlib only (pathlib, re, sys)."""
import re
import sys
from pathlib import Path

FRONTMATTER_RE = re.compile(r"\A---\n(.*?)\n---\n?", re.DOTALL)


def _strip_quotes(s):
    s = s.strip()
    if len(s) >= 2 and s[0] == s[-1] and s[0] in ("'", '"'):
        return s[1:-1]
    return s


def _split_top_level(s, delim):
    """Split s on delim, ignoring delim inside []/{}/quotes."""
    parts, depth, quote, cur = [], 0, None, []
    for ch in s:
        if quote:
            cur.append(ch)
            if ch == quote:
                quote = None
        elif ch in ("'", '"'):
            quote = ch
            cur.append(ch)
        elif ch in "[{":
            depth += 1
            cur.append(ch)
        elif ch in "]}":
            depth -= 1
            cur.append(ch)
        elif ch == delim and depth == 0:
            parts.append("".join(cur))
            cur = []
        else:
            cur.append(ch)
    parts.append("".join(cur))
    return parts


def _parse_scalar(value):
    s = value.strip()
    if s.startswith("["):
        if not s.endswith("]"):
            raise ValueError("unparseable value: %r" % value)
        inner = s[1:-1].strip()
        if not inner:
            return []
        return [_strip_quotes(p) for p in _split_top_level(inner, ",")
                if p.strip()]
    if s.startswith("{"):
        if not s.endswith("}"):
            raise ValueError("unparseable value: %r" % value)
        inner = s[1:-1].strip()
        out = {}
        if inner:
            for part in _split_top_level(inner, ","):
                if ":" not in part:
                    raise ValueError("unparseable value: %r" % value)
                k, _, v = part.partition(":")
                if not k.strip():
                    raise ValueError("unparseable value: %r" % value)
                out[k.strip()] = _strip_quotes(v)
        return out
    return _strip_quotes(s)


def _parse_sequence(kids):
    """'  - item' strings / '  - k: v' + '    k2: v2' dicts (depth max 2)."""
    items, cur = [], None
    for indent, content, raw in kids:
        if indent == 2 and content.startswith("-"):
            rest = content[1:].strip()
            if not rest:
                raise ValueError("unparseable list entry: %r" % raw)
            if ":" in rest and rest[0] not in ("'", '"'):
                sk, _, sv = rest.partition(":")
                if not sk.strip():
                    raise ValueError("unparseable list entry: %r" % raw)
                cur = {sk.strip(): _parse_scalar(sv)}
                items.append(cur)
            else:
                cur = None
                items.append(_strip_quotes(rest))
        elif indent == 4 and cur is not None \
                and ":" in content and not content.startswith("-"):
            ck, _, cv = content.partition(":")
            if not ck.strip():
                raise ValueError("unparseable list entry: %r" % raw)
            cur[ck.strip()] = _parse_scalar(cv)
        else:
            raise ValueError("unparseable frontmatter line: %r" % raw)
    return items


def _parse_nested_map(kids):
    """'  k: v' lines under 'key:' (one level; deeper is ValueError)."""
    sub = {}
    for indent, content, raw in kids:
        if indent != 2 or content.startswith("-") or ":" not in content:
            raise ValueError("unparseable frontmatter line: %r" % raw)
        sk, _, sv = content.partition(":")
        if not sk.strip() or not sv.strip():
            raise ValueError("unparseable frontmatter line: %r" % raw)
        sub[sk.strip()] = _parse_scalar(sv)
    return sub


def _parse_block(block):
    """Flat mapping plus one-level 2-space nesting; no 'type' check."""
    fm, lines = {}, block.splitlines()
    i, n = 0, len(lines)
    while i < n:
        line = lines[i]
        if not line.strip() or line.strip().startswith("#"):
            i += 1
            continue
        lead = line[:len(line) - len(line.lstrip())]
        if "\t" in lead:
            raise ValueError("tab indentation: %r" % line)
        if len(lead):
            raise ValueError("unexpected indented line: %r" % line)
        content = line.strip()
        if content.startswith("-") or ":" not in content:
            raise ValueError("unparseable frontmatter line: %r" % line)
        key, _, value = content.partition(":")
        if not key.strip():
            raise ValueError("unparseable frontmatter line: %r" % line)
        key = key.strip()
        if value.strip():
            fm[key] = _parse_scalar(value)
            i += 1
            continue
        kids, k = [], i + 1
        while k < n:
            nxt = lines[k]
            if not nxt.strip() or nxt.strip().startswith("#"):
                k += 1
                continue
            lead2 = nxt[:len(nxt) - len(nxt.lstrip())]
            if "\t" in lead2:
                raise ValueError("tab indentation: %r" % nxt)
            if not len(lead2):
                break
            kids.append((len(lead2), nxt.strip(), nxt))
            k += 1
        if not kids:
            fm[key] = ""
            i += 1
            continue
        if kids[0][1].startswith("-"):
            fm[key] = _parse_sequence(kids)
        else:
            fm[key] = _parse_nested_map(kids)
        i = k
    return fm


def parse_frontmatter(text):
    """Split '---\\nYAML\\n---\\nbody' into (dict, body).

    Flat 'key: value' plus one-level 2-space nesting (maps, '- item' /
    '- k: v' sequences) and '[a, b]'/'{k: v}' flow values, stdlib only;
    flow lists/dicts are flat strings-only (nested '[{...}]' stays a
    string, truthiness-only use); raises ValueError on missing block,
    empty/missing 'type', tab indentation, or foreign shapes (loud,
    never junk keys).
    """
    m = FRONTMATTER_RE.match(text)
    if not m:
        raise ValueError("missing YAML frontmatter block")
    fm = _parse_block(m.group(1))
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
RESERVED = {"index.md", "log.md"}


def _is_reserved(path):
    return Path(path).name in RESERVED


def _exclude_list(src_dir):
    """Root index.md frontmatter 'exclude' list; [] if absent/unparseable."""
    root = Path(src_dir) / "index.md"
    if not root.is_file():
        return []
    m = FRONTMATTER_RE.match(root.read_text(encoding="utf-8"))
    if not m:
        return []
    try:
        fm = _parse_block(m.group(1))  # no 'type' on navigation indexes
    except ValueError:
        return []
    exc = fm.get("exclude", [])
    if isinstance(exc, str):
        return [exc]
    if isinstance(exc, list):
        return [str(e) for e in exc]
    return []


def _is_excluded(rel, exclude):
    return rel in exclude or Path(rel).name in exclude


def rewrite_links(body, src_rel, id_map, exclude=()):
    """Rewrite '[t](other.md)' -> '@ref <target-id> "t"'.

    Leaves existing '@ref ...' text, external/absolute links, and links
    to reserved files (index.md, log.md) or excluded files untouched.
    Raises ValueError on a relative .md target missing from id_map.
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
        if _is_excluded(key, exclude):
            return m.group(0)
        if key not in id_map:
            raise ValueError(
                "dangling link %r in %s" % (target, src_rel))
        return '@ref %s "%s"' % (id_map[key], text)

    return MD_LINK_RE.sub(sub, body)


def collect_concepts(src_dir):
    """Return {bundle_rel: path} for non-reserved, non-excluded .md, sorted."""
    exclude = _exclude_list(src_dir)
    found = {}
    for path in sorted(src_dir.rglob("*.md")):
        rel = path.relative_to(src_dir).as_posix()
        if _is_reserved(rel):
            continue
        if _is_excluded(rel, exclude):
            continue
        found[rel] = path
    return found


def generate(src_dir, out_dir):
    """Render every concept to '<page_id>.md' in out_dir; return IDs."""
    src_dir, out_dir = Path(src_dir), Path(out_dir)
    if not src_dir.is_dir():
        raise FileNotFoundError("%s: not a directory" % src_dir)
    exclude = _exclude_list(src_dir)
    concepts = collect_concepts(src_dir)
    id_map = {rel: page_id_for(rel) for rel in concepts}
    out_dir.mkdir(parents=True, exist_ok=True)
    written = []
    for rel in sorted(concepts):
        fm, body = parse_frontmatter(
            concepts[rel].read_text(encoding="utf-8"))
        body = rewrite_links(body, rel, id_map, exclude)
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
        text = rewrite_links(text, "index.md", id_map, exclude)
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
    if not src_dir.is_dir():
        return ["%s: not a directory" % src_dir]
    errors = []
    exclude = _exclude_list(src_dir)
    concepts = collect_concepts(src_dir)
    parsed = {}
    for rel, path in sorted(concepts.items()):
        try:
            fm, body = parse_frontmatter(
                path.read_text(encoding="utf-8"))
        except ValueError as exc:
            errors.append("%s: %s" % (rel, exc))
            continue
        parsed[rel] = body
        if fm.get("status") == "stable" and not fm.get("verified"):
            print("warning: %s stable without verified" % rel,
                  file=sys.stderr)
    id_map = {rel: page_id_for(rel) for rel in concepts}
    for rel in sorted(concepts):
        if rel not in parsed:
            continue  # already reported once; reachability stays independent
        try:
            rewrite_links(parsed[rel], rel, id_map, exclude)
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
        src, check_mode = Path(argv[1]), True
    elif len(argv) == 2:
        src, check_mode = Path(argv[0]), False
    else:
        print("usage: okf_to_doxygen.py [--check <src>] | [<src> <out>]")
        return 2
    if not src.is_dir():
        print("error: %s: not a directory" % src)
        return 2
    if check_mode:
        errors = check_bundle(src)
        for err in errors:
            print("error: " + err)
        return 1 if errors else 0
    for pid in generate(src, Path(argv[1])):
        print("wrote " + pid)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
