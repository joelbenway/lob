# Wasm Exhibits Design

Date: 2026-10-09
Status: Draft

## Goal

Embed interactive exhibits directly in the technical reference, powered
by lob itself compiled to wasm. Proves the physics/numerics prose with
runnable math. Full demo solver is out of scope (later product-demo
spec); this spec covers docs-grade exhibits only.

## Exhibits (all in spec)

1. **BC bands transformation** (build first) — band inputs → blended
   drag curve, from the BC worked example.
2. **Drag curve explorer** (build second) — Mach vs Cd for G1–G8 +
   custom tables, PCHIP vs linear lerp.
3. **Atmosphere lapse** (deferred) — altitude → density / speed of
   sound, firing-site (`Fast`) vs lapse-scaled (`k_lapse`).
4. **Heun step-size** (deferred) — step → error vs linear cost.
5. **Zero-angle root** (deferred) — launch angle → `f(θ)` convergence
   (`FastSolveAngle`/`SolveAngle` shared solver).

Build order: 1, 2, then 3 → 4 → 5. All share one harness.

## Harness

One pattern per exhibit, no framework — vanilla JS + canvas/SVG:

```html
<div data-lob-exhibit="bc-bands"></div>
<script type="module" src="exhibits/bc-bands.js"></script>
```

Each exhibit calls 1–2 pure wasm functions and renders client-side.
No `Builder`/`Context`/`Solve` bindings in this spec. Exhibits degrade
to a static figure/table when wasm fails to load.

## Wasm build

- **Toolchain:** Emscripten. `ci-wasm` CMake preset:
  `CMAKE_TOOLCHAIN_FILE=$EMSCRIPTEN/cmake/Platform/Emscripten.cmake`,
  `BUILD_SHARED_LIBS=OFF`, `BUILD_TESTING=OFF`, `BUILD_EXAMPLES=OFF`,
  `BUILD_TOOLS=OFF`, C++14, Release, `MODULARIZE=1`, `EXPORT_ES6=1`.
- **Targets:** static `lob_lob` to wasm + `lob_exhibits` bindings
  exposing only the pure-math functions §1 needs (drag eval, ISA
  atmosphere, Heun demo kernel, angle residual).
- **Flake:** add `emscripten` + `nodejs` to `flake.nix` dev shell.
  Wasm CI job is ubuntu+nix-only (matches existing ubuntu-nix path;
  no Windows/Mac wasm).
- **CI:** `test` matrix gains `wasm` entry: `ci-wasm` preset build +
  `node --check` smoke + wasm instantiation. No size/perf gate in v1.
- **Local:** `cmake --preset=ci-wasm && cmake --build build-wasm`.

### As built

- The nix emscripten package keeps the toolchain file at
  `$EMSCRIPTEN/share/emscripten/cmake/Modules/Platform/Emscripten.cmake`,
  so the preset points there; `EMSCRIPTEN` and a writable `EM_CACHE` are
  exported by the flake dev shell (the store copy of emscripten is
  read-only).
- The wasm build is its own CI job (`wasm`), not a `test` matrix entry:
  it is ubuntu+nix-only, so it does not fit the OS matrix.
- `wasm/wasm.cpp` holds the exported entry points (`LobExhibit*`),
  `wasm/smoke.mjs` instantiates the module in Node and calls one
  function, and `wasm/CMakeLists.txt` links it against `lob_lob`.

## JS wrapper

One thin module, `lob-exhibits.mjs` (ES module: `import`/`export`,
loads via `<script type="module">` and `node` unmodified, no bundler).
Per-exhibit functions; wrapper owns heap strings/arrays so callers
pass typed arrays and get numbers back. Portable APIs only (standard
JS + WebAssembly, no `fs`/`process`/`require`, zero npm deps) so it
runs identically in browser and Node. Full `Builder`/`Solve`
ergonomics deferred to the full-demo spec.

// ponytail: no framework, no bundler, no TS types until JS devs ask

## Docs embedding

- OKF concept declares `exhibit: bc-bands` in frontmatter; the OKF
  converter emits the exhibit div + script tag into the generated
  Doxygen page. Concepts without `exhibit` are unaffected.
- Wasm/JS artifacts build to `build/generated/exhibits/` and copy into
  the Doxygen HTML output dir; gh-pages serves them as static files
  alongside the API reference.

## npm (deferred)

CI builds the wasm + `lob-exhibits.mjs` artifact but does not publish.
`npm publish` from CI (provenance, version from `project(VERSION)`)
is a named follow-up, not in this spec.
