#!/usr/bin/env python3
"""Tests for docs/okf_to_doxygen.py (stdlib unittest, no deps)."""
import unittest

from okf_to_doxygen import parse_frontmatter, page_id_for, render_page
import tempfile
from okf_to_doxygen import rewrite_links, generate
from pathlib import Path


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


if __name__ == "__main__":
    unittest.main()
