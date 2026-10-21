#!/usr/bin/env python3
"""Tests for docs/okf_to_doxygen.py (stdlib unittest, no deps)."""
import unittest

from okf_to_doxygen import parse_frontmatter, page_id_for, render_page
import tempfile
from okf_to_doxygen import rewrite_links, generate
from pathlib import Path
from okf_to_doxygen import check_bundle
from okf_to_doxygen import main


class MissingDirTest(unittest.TestCase):
    def test_check_missing_src_dir(self):
        with tempfile.TemporaryDirectory() as tmp:
            missing = str(Path(tmp) / "nope")
            self.assertEqual(
                check_bundle(Path(missing)), [missing + ": not a directory"])

    def test_generate_missing_src_dir_raises(self):
        with tempfile.TemporaryDirectory() as tmp:
            with self.assertRaises(FileNotFoundError):
                generate(Path(tmp) / "nope", Path(tmp) / "out")

    def test_main_check_missing_src_dir_exits_2(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.assertEqual(
                main(["--check", str(Path(tmp) / "nope")]), 2)

    def test_main_generate_missing_src_dir_exits_2(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.assertEqual(
                main([str(Path(tmp) / "nope"), str(Path(tmp) / "out")]), 2)


class ParseTest(unittest.TestCase):
    def test_minimal_type_only(self):
        fm, body = parse_frontmatter("---\ntype: Concept\n---\nHello\n")
        self.assertEqual(fm, {"type": "Concept"})
        self.assertEqual(body, "Hello\n")

    def test_missing_frontmatter_raises(self):
        with self.assertRaises(ValueError):
            parse_frontmatter("No frontmatter here\n")

    def test_empty_type_raises(self):
        with self.assertRaises(ValueError):
            parse_frontmatter("---\ntype: \n---\nBody\n")


class NestedFrontmatterTest(unittest.TestCase):
    def test_block_style_round_trip(self):
        text = ("---\ntype: Concept\ntitle: Drag\n"
                "generated:\n  by: human:joel\n  at: 2026-10-09T00:00:00Z\n"
                "verified:\n  - by: human:joel\n    at: 2026-10-09T00:00:00Z\n"
                "  - by: agent:opencode\n    at: 2026-10-10T00:00:00Z\n"
                "sources:\n  - path: docs/pages/drag.md\n    note: legacy page\n"
                "tags:\n  - drag\n  - splines\n"
                "---\nBody\n")
        fm, body = parse_frontmatter(text)
        self.assertEqual(fm, {
            "type": "Concept", "title": "Drag",
            "generated": {"by": "human:joel", "at": "2026-10-09T00:00:00Z"},
            "verified": [
                {"by": "human:joel", "at": "2026-10-09T00:00:00Z"},
                {"by": "agent:opencode", "at": "2026-10-10T00:00:00Z"}],
            "sources": [{"path": "docs/pages/drag.md",
                         "note": "legacy page"}],
            "tags": ["drag", "splines"]})
        self.assertEqual(body, "Body\n")

    def test_tab_indentation_raises(self):
        with self.assertRaises(ValueError):
            parse_frontmatter(
                "---\ntype: Concept\ngenerated:\n\tby: x\n---\nB\n")

    def test_flow_list_skill_md(self):
        fm, _ = parse_frontmatter(
            "---\ntype: Concept\nexclude: [SKILL.md]\n---\nB\n")
        self.assertEqual(fm["exclude"], ["SKILL.md"])

    def test_flow_map_inline(self):
        fm, _ = parse_frontmatter(
            "---\ntype: Concept\n"
            "generated: {by: human:joel, at: 2026-10-09T00:00:00Z}\n"
            "---\nB\n")
        self.assertEqual(
            fm["generated"],
            {"by": "human:joel", "at": "2026-10-09T00:00:00Z"})

    def test_top_level_dash_raises(self):
        with self.assertRaises(ValueError):
            parse_frontmatter("---\ntype: Concept\n- by: x\n---\nB\n")


class PageIdTest(unittest.TestCase):
    def test_nested_path(self):
        self.assertEqual(
            page_id_for("ballistic_model/drag.md"),
            "okf_ballistic_model_drag")

    def test_dashes_and_dots(self):
        self.assertEqual(
            page_id_for("my-dir/some.page.md"),
            "okf_my_dir_some_page")


class RenderTest(unittest.TestCase):
    def test_page_header_uses_title(self):
        out = render_page(
            "okf_design_overview", {"type": "DesignDecision",
                                    "title": "Design Decisions"}, "Body\n")
        self.assertTrue(
            out.startswith("@page okf_design_overview Design Decisions\n"))

    def test_title_falls_back_to_page_id(self):
        out = render_page(
            "okf_foo_bar", {"type": "Concept"}, "Body\n")
        self.assertTrue(out.startswith("@page okf_foo_bar okf_foo_bar\n"))


class RewriteTest(unittest.TestCase):
    def test_relative_md_link_becomes_ref(self):
        id_map = {"numerical_methods/splines.md":
                  "okf_numerical_methods_splines"}
        out = rewrite_links(
            "see [splines](../numerical_methods/splines.md) here",
            "ballistic_model/drag.md", id_map)
        self.assertEqual(
            out,
            'see @ref okf_numerical_methods_splines "splines" here')

    def test_api_ref_passes_through(self):
        out = rewrite_links(
            "see @ref api_builder for construction",
            "design/overview.md", {})
        self.assertIn("@ref api_builder", out)

    def test_unknown_md_target_raises(self):
        with self.assertRaises(ValueError):
            rewrite_links("see [x](nope.md)", "a/b.md", {})

    def test_anchored_md_link_becomes_ref(self):
        out = rewrite_links(
            "see [x](a.md#section)", "b.md", {"a.md": "okf_a"})
        self.assertEqual(out, 'see @ref okf_a "x"')

    def test_anchored_unknown_target_raises(self):
        with self.assertRaises(ValueError):
            rewrite_links("see [x](nope.md#section)", "a.md", {})


class GenerateTest(unittest.TestCase):
    def test_end_to_end_two_files(self):
        with tempfile.TemporaryDirectory() as src, \
                tempfile.TemporaryDirectory() as out:
            srcp = Path(src)
            (srcp / "index.md").write_text(
                "# Bundle\n- [Drag](drag.md)\n", encoding="utf-8")
            (srcp / "drag.md").write_text(
                "---\ntype: Concept\ntitle: Drag Model\n---\n"
                "See [idx](index.md).\n", encoding="utf-8")
            ids = generate(srcp, Path(out))
            self.assertEqual(ids, ["okf_drag"])
            rendered = (Path(out) / "okf_drag.md").read_text(
                encoding="utf-8")
            self.assertTrue(rendered.startswith(
                "@page okf_drag Drag Model\n"))

    def test_stale_generated_pages_removed(self):
        with tempfile.TemporaryDirectory() as src, \
                tempfile.TemporaryDirectory() as out:
            srcp = Path(src)
            (srcp / "index.md").write_text(
                "# Bundle\n- [A](a.md)\n- [B](b.md)\n", encoding="utf-8")
            (srcp / "a.md").write_text(
                "---\ntype: Concept\ntitle: A\n---\nA\n", encoding="utf-8")
            (srcp / "b.md").write_text(
                "---\ntype: Concept\ntitle: B\n---\nB\n", encoding="utf-8")
            generate(srcp, Path(out))
            (Path(out) / "keep.txt").write_text("keep", encoding="utf-8")
            (srcp / "b.md").unlink()
            (srcp / "index.md").write_text(
                "# Bundle\n- [A](a.md)\n", encoding="utf-8")
            generate(srcp, Path(out))
            self.assertEqual(
                sorted(p.name for p in Path(out).iterdir()),
                ["keep.txt", "okf_a.md", "okf_mainpage.md"])

    def test_root_index_frontmatter_stripped_from_mainpage(self):
        with tempfile.TemporaryDirectory() as src, \
                tempfile.TemporaryDirectory() as out:
            srcp = Path(src)
            (srcp / "index.md").write_text(
                "---\nokf_version: \"0.2\"\n---\n"
                "# Bundle\n- [Drag](drag.md)\n", encoding="utf-8")
            (srcp / "drag.md").write_text(
                "---\ntype: Concept\ntitle: Drag Model\n---\nBody\n",
                encoding="utf-8")
            generate(srcp, Path(out))
            mainpage = (Path(out) / "okf_mainpage.md").read_text(
                encoding="utf-8")
            self.assertNotIn("---", mainpage)
            self.assertNotIn("okf_version", mainpage)
            self.assertIn("# Bundle", mainpage)


class CheckTest(unittest.TestCase):
    def _bundle(self, tmp, files):
        base = Path(tmp)
        for rel, text in files.items():
            p = base / rel
            p.parent.mkdir(parents=True, exist_ok=True)
            p.write_text(text, encoding="utf-8")
        return base

    def test_clean_bundle(self):
        with tempfile.TemporaryDirectory() as tmp:
            base = self._bundle(tmp, {
                "index.md": "# B\n- [A](a.md)\n",
                "a.md": "---\ntype: Concept\ntitle: A\n---\nHi\n",
            })
            self.assertEqual(check_bundle(base), [])

    def test_missing_type_and_dangling_link_reported(self):
        with tempfile.TemporaryDirectory() as tmp:
            base = self._bundle(tmp, {
                "index.md": "# B\n",
                "bad.md": "---\ntype: \n---\nBody\n",
                "lonely.md": "---\ntype: Concept\n---\n"
                             "See [z](gone.md).\n",
                "orphan.md": "---\ntype: Concept\n---\nNo index links me.\n",
            })
            errs = check_bundle(base)
            # single-pass: bad.md frontmatter reports once (rewrite
            # reuses the parse); reachability is independent:
            # bad.md unparsable x1 + unreachable x1, lonely.md
            # dangling x1 + unreachable x1, orphan.md unreachable x1 = 5.
            self.assertEqual(len(errs), 5)
            self.assertEqual(
                sum(1 for e in errs if e.startswith("bad.md")), 2)
            self.assertEqual(
                sum(1 for e in errs if e.startswith("lonely.md")), 2)
            self.assertEqual(
                sum(1 for e in errs if e.startswith("orphan.md")), 1)

    def test_subdir_index_with_frontmatter_reported(self):
        with tempfile.TemporaryDirectory() as tmp:
            base = self._bundle(tmp, {
                "index.md": "# B\n",
                "sub/index.md": "---\ntype: Concept\n---\nNav\n",
            })
            errs = check_bundle(base)
            self.assertTrue(any("index.md" in e for e in errs))

    def test_stable_without_verified_is_warning_only(self):
        with tempfile.TemporaryDirectory() as tmp:
            base = self._bundle(tmp, {
                "index.md": "# B\n- [A](a.md)\n",
                "a.md": "---\ntype: Concept\nstatus: stable\n---\nHi\n",
            })
            self.assertEqual(check_bundle(base), [])

    def test_skill_md_ignored(self):
        with tempfile.TemporaryDirectory() as tmp, \
                tempfile.TemporaryDirectory() as out:
            base = self._bundle(tmp, {
                "index.md": "---\nexclude: [SKILL.md]\n---\n"
                            "# B\n- [A](a.md)\n",
                "a.md": "---\ntype: Concept\ntitle: A\n---\nHi\n",
                "SKILL.md": "# Skill\nNo frontmatter, unlinked.\n",
                "sub/SKILL.md": "# Nested skill\n",
            })
            self.assertEqual(check_bundle(base), [])
            self.assertEqual(generate(base, Path(out)), ["okf_a"])

    def test_exclude_list_skips_custom_file(self):
        with tempfile.TemporaryDirectory() as tmp, \
                tempfile.TemporaryDirectory() as out:
            base = self._bundle(tmp, {
                "index.md": "---\nexclude: [scratch.md]\n---\n"
                            "# B\n- [A](a.md)\n",
                "a.md": "---\ntype: Concept\ntitle: A\n---\n"
                        "Hi [s](scratch.md).\n",
                "scratch.md": "No frontmatter, unlinked, excluded.\n",
            })
            self.assertEqual(check_bundle(base), [])
            self.assertEqual(generate(base, Path(out)), ["okf_a"])


if __name__ == "__main__":
    unittest.main()
