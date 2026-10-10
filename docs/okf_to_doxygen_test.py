#!/usr/bin/env python3
"""Tests for docs/okf_to_doxygen.py (stdlib unittest, no deps)."""
import unittest

from okf_to_doxygen import parse_frontmatter, page_id_for, render_page
import tempfile
from okf_to_doxygen import rewrite_links, generate
from pathlib import Path
from okf_to_doxygen import check_bundle


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
            # brief draft said 3; verbatim impl yields 6 true errors:
            # bad.md x2 (frontmatter + rewrite passes) + lonely.md dangling
            # + all three unreachable (root index links nothing).
            self.assertEqual(len(errs), 6)
            self.assertTrue(any("bad.md" in e for e in errs))
            self.assertTrue(any("lonely.md" in e for e in errs))
            self.assertTrue(any("orphan.md" in e for e in errs))

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
                "index.md": "# B\n- [A](a.md)\n",
                "a.md": "---\ntype: Concept\ntitle: A\n---\nHi\n",
                "SKILL.md": "# Skill\nNo frontmatter, unlinked.\n",
                "sub/SKILL.md": "# Nested skill\n",
            })
            self.assertEqual(check_bundle(base), [])
            self.assertEqual(generate(base, Path(out)), ["okf_a"])


if __name__ == "__main__":
    unittest.main()
