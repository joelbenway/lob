#!/usr/bin/env python3
# Copyright (c) 2026  Joel Benway
# SPDX-License-Identifier: GPL-3.0-or-later
# Please see end of file for extended copyright information

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
            raise ValueError("unparsable value: %r" % value)
        inner = s[1:-1].strip()
        if not inner:
            return []
        return [_strip_quotes(p) for p in _split_top_level(inner, ",")
                if p.strip()]
    if s.startswith("{"):
        if not s.endswith("}"):
            raise ValueError("unparsable value: %r" % value)
        inner = s[1:-1].strip()
        out = {}
        if inner:
            for part in _split_top_level(inner, ","):
                if ":" not in part:
                    raise ValueError("unparsable value: %r" % value)
                k, _, v = part.partition(":")
                if not k.strip():
                    raise ValueError("unparsable value: %r" % value)
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
                raise ValueError("unparsable list entry: %r" % raw)
            if ":" in rest and rest[0] not in ("'", '"'):
                sk, _, sv = rest.partition(":")
                if not sk.strip():
                    raise ValueError("unparsable list entry: %r" % raw)
                cur = {sk.strip(): _parse_scalar(sv)}
                items.append(cur)
            else:
                cur = None
                items.append(_strip_quotes(rest))
        elif indent == 4 and cur is not None \
                and ":" in content and not content.startswith("-"):
            ck, _, cv = content.partition(":")
            if not ck.strip():
                raise ValueError("unparsable list entry: %r" % raw)
            cur[ck.strip()] = _parse_scalar(cv)
        else:
            raise ValueError("unparsable frontmatter line: %r" % raw)
    return items


def _parse_nested_map(kids):
    """'  k: v' lines under 'key:' (one level; deeper is ValueError)."""
    sub = {}
    for indent, content, raw in kids:
        if indent != 2 or content.startswith("-") or ":" not in content:
            raise ValueError("unparsable frontmatter line: %r" % raw)
        sk, _, sv = content.partition(":")
        if not sk.strip() or not sv.strip():
            raise ValueError("unparsable frontmatter line: %r" % raw)
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
            raise ValueError("unparsable frontmatter line: %r" % line)
        key, _, value = content.partition(":")
        if not key.strip():
            raise ValueError("unparsable frontmatter line: %r" % line)
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


def render_page(page_id, frontmatter, body, graph_dot=""):
    """Emit '@page <id> <title>' header plus banner, exhibit, and body."""
    title = frontmatter.get("title") or page_id
    banner = ""
    if graph_dot:
        banner = (
            "\\htmlonly\n"
            '<div class="okf-neighborhood-graph">\n'
            '  <div class="okf-graph-caption">'
            "Topic Map &bull; Prerequisites &amp; Related Concepts</div>\n"
            "\\endhtmlonly\n"
            "%s\n"
            "\\htmlonly\n"
            "</div>\n"
            "\\endhtmlonly\n"
            % graph_dot.strip()
        )
    exhibit = frontmatter.get("exhibit")
    if exhibit:
        if isinstance(exhibit, str):
            ex = exhibit.strip()
        else:
            ex = str(exhibit).strip()
        if ex:
            exhibit_html = (
                "\\htmlonly\n"
                '<div data-lob-exhibit="%s"></div>\n'
                '<script type="module" src="exhibits/%s.js"></script>\n'
                "\\endhtmlonly\n"
                % (ex, ex)
            )
            return "@page %s %s\n%s%s%s" % (
                page_id, title, banner, exhibit_html, body)
    return "@page %s %s\n%s%s" % (page_id, title, banner, body)


MD_LINK_RE = re.compile(r"\[([^\]]+)\]\(([^)\"']+?)(?:\s+\"[^\"]*\")?\)")
RESERVED = {"index.md", "log.md"}


def _is_reserved(path):
    return Path(path).name in RESERVED


def _exclude_list(src_dir):
    """Root index.md frontmatter 'exclude' list; [] if absent/unparsable."""
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
        path_part = target.split("#", 1)[0]
        if not path_part.endswith(".md"):
            return m.group(0)
        if target.startswith(("http://", "https://", "#", "/")):
            return m.group(0)
        if _is_reserved(path_part):
            return m.group(0)
        norm = str((src_dir / path_part).as_posix())
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


def extract_concept_links(body, src_rel, concept_rels, exclude=()):
    """Return set of normalized concept bundle_rels referenced by links in body."""
    src_dir = Path(src_rel).parent
    out = set()
    for m in MD_LINK_RE.finditer(body):
        target = m.group(2)
        path_part = target.split("#", 1)[0]
        if not path_part.endswith(".md"):
            continue
        if target.startswith(("http://", "https://", "#", "/")):
            continue
        if _is_reserved(path_part):
            continue
        norm = (src_dir / path_part).as_posix()
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
            continue
        if key in concept_rels and key != src_rel:
            out.add(key)
    return out


def _escape_dot(s):
    return str(s).replace("\\", "\\\\").replace('"', '\\"')


DOT_FONT = "ui-sans-serif, system-ui, sans-serif"


def build_neighborhood_dot(pid, title, ptype, incoming_nodes, outgoing_nodes):
    """Construct a 1-hop Graphviz digraph for the concept page."""
    if not incoming_nodes and not outgoing_nodes:
        return ""

    lines = [
        "\\dot",
        'digraph "neighborhood" {' ,
        "  rankdir=LR;",
        '  bgcolor="transparent";',
        '  node [fontname="%s", fontsize=10, '
        'height=0.34, shape=box, style="filled,rounded"];' % DOT_FONT,
        '  edge [color="#859399", penwidth=1.2, arrowsize=0.65];',
        "",
    ]

    for nid, info in sorted(incoming_nodes.items()):
        ntitle = _escape_dot(info.get("title", nid))
        shape_attr = (
            "peripheries=2"
            if info.get("type") == "DesignDecision"
            else 'shape=box, style="filled,rounded"'
        )
        lines.append(
            '  %s [label="%s", tooltip="%s", URL="@ref %s", '
            'fillcolor="#fffdf8", fontcolor="#17202a", color="#d8d1c2", %s];'
            % (nid, ntitle, ntitle, nid, shape_attr)
        )

    ctitle = _escape_dot(title)
    cshape = (
        "peripheries=2"
        if ptype == "DesignDecision"
        else 'shape=box, style="filled,rounded"'
    )
    lines.append(
        '  %s [label="%s", tooltip="Current Page", fillcolor="#a44122", '
        'fontcolor="#ffffff", color="#7b2e17", penwidth=2.0, %s];'
        % (pid, ctitle, cshape)
    )

    for nid, info in sorted(outgoing_nodes.items()):
        ntitle = _escape_dot(info.get("title", nid))
        shape_attr = (
            "peripheries=2"
            if info.get("type") == "DesignDecision"
            else 'shape=box, style="filled,rounded"'
        )
        lines.append(
            '  %s [label="%s", tooltip="%s", URL="@ref %s", '
            'fillcolor="#fffdf8", fontcolor="#17202a", color="#d8d1c2", %s];'
            % (nid, ntitle, ntitle, nid, shape_attr)
        )

    lines.append("")
    for inc_id in sorted(incoming_nodes.keys()):
        lines.append("  %s -> %s;" % (inc_id, pid))
    for out_id in sorted(outgoing_nodes.keys()):
        lines.append("  %s -> %s;" % (pid, out_id))

    lines.extend(["}", "\\enddot", ""])
    return "\n".join(lines)


def reduce_edges(outgoing, id_map):
    """Transitive reduction of outgoing concept links, preserving cycles."""
    graph = {
        id_map[u]: {
            id_map[v]
            for v in targets
            if v in id_map and id_map[v] != id_map[u]
        }
        for u, targets in outgoing.items()
    }

    def reachable_nodes(start, avoid_edge=None):
        visited = set()
        queue = [start]
        while queue:
            curr = queue.pop()
            if curr not in visited:
                visited.add(curr)
                for neighbor in graph.get(curr, ()):
                    if avoid_edge and curr == avoid_edge[0] and neighbor == avoid_edge[1]:
                        continue
                    queue.append(neighbor)
        return visited

    reduced = {u: set(targets) for u, targets in graph.items()}
    for u in sorted(graph):
        for v in sorted(graph[u]):
            v_reaches = reachable_nodes(v)
            if u in v_reaches:
                continue
            u_reaches_without = reachable_nodes(u, avoid_edge=(u, v))
            if v in u_reaches_without:
                reduced[u].remove(v)
    return reduced


def build_global_dot(concepts, titles, types, outgoing_by_rel, id_map):
    """Emit a force-directed, clustered Graphviz graph for the entire OKF corpus."""
    if not concepts:
        return ""
    reduced_edges = reduce_edges(outgoing_by_rel, id_map)
    lines = [
        "\\dot",
        "digraph global_map {",
        '  bgcolor="transparent";',
        "  rankdir=LR;",
        "  nodesep=0.35;",
        "  ranksep=0.6;",
        '  node [shape=box, style="filled,rounded", fontname="%s", fontsize=10];'
        % DOT_FONT,
        '  edge [color="#859399", penwidth=1.2, arrowsize=0.75];',
        "",
    ]
    clusters = [
        ("Guide", "cluster_guides", "Guides & Workflows"),
        ("Concept", "cluster_concepts", "Core Concepts"),
        ("DesignDecision", "cluster_decisions", "Design Decisions"),
    ]
    for ptype, cname, clabel in clusters:
        lines.append("  subgraph %s {" % cname)
        lines.append('    label="%s";' % clabel)
        lines.append('    style="rounded,dashed";')
        lines.append('    color="#d8d1c2";')
        lines.append(
            '    fontname="%s"; fontsize=11; fontcolor="#a44122";' % DOT_FONT
        )
        for rel in sorted(concepts):
            item_type = types.get(rel, "Concept")
            if item_type == ptype or (
                ptype == "Concept"
                and item_type not in ("Guide", "DesignDecision")
            ):
                pid = id_map[rel]
                title = _escape_dot(titles.get(rel) or pid)
                periph = " peripheries=2" if item_type == "DesignDecision" else ""
                lines.append(
                    '    %s [label="%s", tooltip="%s", URL="@ref %s", target="_top", '
                    'fillcolor="#fffdf8", fontcolor="#17202a", color="#d8d1c2"%s];'
                    % (pid, title, title, pid, periph)
                )
        lines.append("  }")

    lines.append("")
    for src_id in sorted(reduced_edges):
        for dst_id in sorted(reduced_edges[src_id]):
            lines.append("  %s -> %s;" % (src_id, dst_id))

    lines.extend(["}", "\\enddot", ""])
    return "\n".join(lines)


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
    for stale in out_dir.glob("okf_*.md"):
        stale.unlink()

    parsed = {}
    outgoing_by_rel = {}
    for rel in concepts:
        fm, b = parse_frontmatter(concepts[rel].read_text(encoding="utf-8"))
        parsed[rel] = (fm, b)
        outgoing_by_rel[rel] = extract_concept_links(
            b, rel, concepts, exclude)

    incoming_by_rel = {rel: set() for rel in concepts}
    for src_rel, targets in outgoing_by_rel.items():
        for tgt_rel in targets:
            incoming_by_rel[tgt_rel].add(src_rel)

    written = []
    for rel in sorted(concepts):
        fm, raw_body = parsed[rel]
        body = rewrite_links(raw_body, rel, id_map, exclude)
        pid = id_map[rel]
        title = fm.get("title") or pid
        ptype = fm.get("type", "Concept")

        inc_nodes = {
            id_map[s]: {
                "title": parsed[s][0].get("title") or id_map[s],
                "type": parsed[s][0].get("type", "Concept"),
            }
            for s in sorted(incoming_by_rel[rel])
            if s != rel
        }
        out_nodes = {
            id_map[t]: {
                "title": parsed[t][0].get("title") or id_map[t],
                "type": parsed[t][0].get("type", "Concept"),
            }
            for t in sorted(outgoing_by_rel[rel])
            if t != rel
        }

        dot = build_neighborhood_dot(
            pid, title, ptype, inc_nodes, out_nodes)
        (out_dir / (pid + ".md")).write_text(
            render_page(pid, fm, body, graph_dot=dot), encoding="utf-8")
        written.append(pid)

    concept_titles = {
        rel: parsed[rel][0].get("title") or id_map[rel] for rel in concepts
    }
    concept_types = {
        rel: parsed[rel][0].get("type", "Concept") for rel in concepts
    }

    guides = []
    core_concepts = []
    decisions = []
    for rel in sorted(concepts):
        ptype = concept_types[rel]
        pid = id_map[rel]
        title = concept_titles[rel]
        if ptype == "Guide":
            guides.append((pid, title))
        elif ptype == "DesignDecision":
            decisions.append((pid, title))
        else:
            core_concepts.append((pid, title))

    # Generate category hubs
    guides_lines = [
        "@page okf_guides Guides & Workflows",
        "",
        "# Guides & Workflows",
        "",
        "End-to-end walkthroughs, step-by-step solver workflows, and worked examples.",
        "",
    ]
    for pid, title in guides:
        guides_lines.append("- @subpage %s \"%s\"" % (pid, title))
    guides_lines.append("")
    (out_dir / "okf_guides.md").write_text(
        "\n".join(guides_lines), encoding="utf-8"
    )

    concepts_lines = [
        "@page okf_concepts Core Concepts",
        "",
        "# Core Concepts",
        "",
        "Fundamental ballistic trajectory models, atmospheric equations, aerodynamic coefficients, "
        "and numerical solvers.",
        "",
    ]
    for pid, title in core_concepts:
        concepts_lines.append("- @subpage %s \"%s\"" % (pid, title))
    concepts_lines.append("")
    (out_dir / "okf_concepts.md").write_text(
        "\n".join(concepts_lines), encoding="utf-8"
    )

    decisions_lines = [
        "@page okf_decisions Design Decisions",
        "",
        "# Design Decisions",
        "",
        "Architectural rationales, memory layout guarantees, C ABI boundaries, and library invariants.",
        "",
    ]
    for pid, title in decisions:
        decisions_lines.append("- @subpage %s \"%s\"" % (pid, title))
    decisions_lines.append("")
    (out_dir / "okf_decisions.md").write_text(
        "\n".join(decisions_lines), encoding="utf-8"
    )

    # root index.md becomes the @mainpage Technical Reference
    root_index = src_dir / "index.md"
    if root_index.exists():
        text = root_index.read_text(encoding="utf-8")
        m = FRONTMATTER_RE.match(text)
        if m:
            text = text[m.end():]
        text = re.sub(
            r'<div style="display:\s*none;?">.*?</div>',
            "",
            text,
            flags=re.DOTALL,
        )
        text = rewrite_links(text, "index.md", id_map, exclude)
        # Title from the index body's first heading, else a generic label.
        heading = re.search(r"^#\s+(.+)$", text, re.MULTILINE)
        title = heading.group(1).strip() if heading else "Technical Reference"

        global_dot = build_global_dot(
            concepts, concept_titles, concept_types, outgoing_by_rel, id_map
        )
        cards_block = (
            "\n\n\\htmlonly\n"
            '<div class="lob-pillar-cards">\n'
            '  <a class="lob-pillar-card" href="okf_guides.html">\n'
            '    <span class="lob-pillar-card-badge">Walkthroughs</span>\n'
            '    <h3>🧭 Guides &amp; Workflows</h3>\n'
            '    <p>Step-by-step solver workflows, basic worked examples, and input state machine rules.</p>\n'
            '    <span class="lob-pillar-card-cta">Explore Guides &rarr;</span>\n'
            '  </a>\n'
            '  <a class="lob-pillar-card" href="okf_concepts.html">\n'
            '    <span class="lob-pillar-card-badge">Physics &amp; Math</span>\n'
            '    <h3>⚛️ Core Concepts</h3>\n'
            '    <p>Supersonic drag tables, 1976 Standard Atmosphere, Heun integration, and aerodynamic jump.</p>\n'
            '    <span class="lob-pillar-card-cta">Explore Concepts &rarr;</span>\n'
            '  </a>\n'
            '  <a class="lob-pillar-card" href="okf_decisions.html">\n'
            '    <span class="lob-pillar-card-badge">Architecture</span>\n'
            '    <h3>⚖️ Design Decisions</h3>\n'
            '    <p>Pure C ABI waist, zero dynamic allocation, cache line alignment, and library invariants.</p>\n'
            '    <span class="lob-pillar-card-cta">Explore Decisions &rarr;</span>\n'
            '  </a>\n'
            '  <a class="lob-pillar-card" href="classes.html">\n'
            '    <span class="lob-pillar-card-badge">C &amp; C++</span>\n'
            '    <h3>⚙️ API Reference</h3>\n'
            '    <p>Full programming interface: C99 narrow-waist ABI, C++ fluent builder, solver context, and types.</p>\n'
            '    <span class="lob-pillar-card-cta">Explore Classes &amp; Headers &rarr;</span>\n'
            '  </a>\n'
            '</div>\n'
            "\\endhtmlonly\n"
        )
        subpages_block = (
            "\n\n## Categories\n"
            "- @subpage okf_guides \"Guides & Workflows\"\n"
            "- @subpage okf_concepts \"Core Concepts\"\n"
            "- @subpage okf_decisions \"Design Decisions\"\n"
        )
        graph_block = ""
        if global_dot:
            graph_block = (
                "\n\n\\htmlonly\n"
                '<div class="okf-neighborhood-graph okf-global-graph">\n'
                '  <div class="okf-graph-caption">'
                "Corpus Topic Map &bull; Complete Knowledge Graph</div>\n"
                "\\endhtmlonly\n"
                "%s\n"
                "\\htmlonly\n"
                "</div>\n"
                "\\endhtmlonly\n"
            ) % global_dot.strip()

        (out_dir / "okf_mainpage.md").write_text(
            "@mainpage %s\n" % title
            + text.strip()
            + subpages_block
            + cards_block
            + graph_block,
            encoding="utf-8",
        )
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
            path_part = target.split("#", 1)[0]
            if not path_part.endswith(".md"):
                continue
            if target.startswith(("http://", "https://", "#", "/")):
                continue
            try:
                rel_target = (base / path_part).relative_to(src_dir).as_posix()
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

# This file is part of lob.
#
# lob is free software: you can redistribute it and/or modify it under the
# terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# lob is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
# A PARTICULAR PURPOSE. See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with
# lob. If not, see <https://www.gnu.org/licenses/>.
