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
  assert.ok(
    flat.includes(
      '<nav class="hero-links" aria-label="Solver resources"> <a href="https://joelbenway.github.io/lob/docs/">Documentation</a> <a href="https://joelbenway.github.io/lob/bench">Benchmarks</a> </nav>',
    ),
  );
  assert.ok(/<h1>\s*Ballistics Explorer\s*<\/h1>/.test(flat));
  assert.ok(css.includes(".hero-links"));
  assert.ok(css.includes(".hero a"));
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
