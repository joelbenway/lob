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
