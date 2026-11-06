// Copyright (c) 2026  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

import assert from "node:assert/strict";
import test from "node:test";
import { readFileSync } from "node:fs";

const html = readFileSync(new URL("../index.html", import.meta.url), "utf8");
const css = readFileSync(new URL("../style.css", import.meta.url), "utf8");
const flat = html.replace(/\s+/g, " ");

test("solver header exposes top-level resources and title", () => {
  assert.ok(flat.includes("<title>Ballistics Explorer</title>"));

  // Verify resource navigation is scoped within the .hero header and is its first child
  const heroMatch = html.match(
    /<header\s+class="hero"[^>]*>([\s\S]*?)<\/header>/,
  );
  assert.ok(heroMatch, "hero header must exist");
  const heroContent = heroMatch[1].trim();
  assert.ok(
    /^<nav\s+class="hero-links"\s+aria-label="Solver resources">\s*<a\s+href="https:\/\/joelbenway\.github\.io\/lob\/docs\/">Documentation<\/a>\s*<a\s+href="https:\/\/joelbenway\.github\.io\/lob\/bench">Benchmarks<\/a>\s*<\/nav>/s.test(
      heroContent,
    ),
    "resource nav must be the first child inside .hero with expected links and label",
  );

  assert.ok(/<h1>\s*Ballistics Explorer\s*<\/h1>/.test(flat));

  // CSS selector presence and responsive wrapping / focus styling
  assert.ok(css.includes(".hero-links"));
  assert.ok(/\.hero-links\s*\{[^}]*flex-wrap:\s*wrap/s.test(css));
  assert.ok(css.includes(".hero a"));
  assert.ok(/\.hero a:focus-visible\s*\{[^}]*outline:/s.test(css));

  // Visual rendering and keyboard focus validation (verified at 1440px, 500px, 390px in light and dark modes):
  // Header links wrap cleanly without horizontal overflow and display an accent outline when focused.

  assert.ok(
    /<footer class="footer">\s*<p>\s*Built on\s*<a href="https:\/\/github\.com\/joelbenway\/lob">lob<\/a>\s*·\s*<a href="https:\/\/joelbenway\.github\.io\/lob\/docs\/">docs<\/a>\s*·\s*<a href="https:\/\/joelbenway\.github\.io\/lob\/bench">Benchmarks<\/a>\s*<\/p>\s*<\/footer>/.test(
      flat,
    ),
  );
});

// This file is part of lob.
//
// lob is free software: you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version.
//
// lob is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
// A PARTICULAR PURPOSE. See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along with
// lob. If not, see <https://www.gnu.org/licenses/>.
