#!/usr/bin/env python3
"""Tests for docs/okf_to_doxygen.py (stdlib unittest, no deps)."""
import unittest

from okf_to_doxygen import parse_frontmatter, page_id_for, render_page


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


if __name__ == "__main__":
    unittest.main()
