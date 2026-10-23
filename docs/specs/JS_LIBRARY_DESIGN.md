# JavaScript Library Design

Date: 2026-10-22
Status: Draft

## Goal

Ship `@joelbenway/lob`: a JavaScript package backed by the same C++
solver as the native library, compiled to WebAssembly. The C API in
`include/lob/lob.h` is the single source of truth. The package targets Node 20+
(browser support is not yet tested), has no runtime dependencies, and is
usable without a bundler.

The technical-reference exhibits are a future consumer of this
library, not its defining use case. There are no exhibit-specific
entry points.

## Layers

The package exposes two layers.

### Raw C API

`lob.mjs` (the Emscripten ES module factory) plus `lob.wasm` exposes
every `LOB_EXPORT` function in `lob.h`, `_malloc`, `_free`, and a few
ABI shims:

- `LobWasmBuilderSize`, `LobWasmContextSize`, `LobWasmWindPointSize`,
  `LobWasmOutputSize`: struct sizes. The wrapper never hardcodes wasm
  struct layouts.
- `LobWasmContextError`: reads `LobContext.error`.
- `LobWasmOutputsToDoubles`: flattens six `LobOutput` fields per row
  (range, velocity, energy, elevation, deflection, time of flight) so
  JavaScript does not depend on C padding.

Consumers import the factory from `@joelbenway/lob/raw`, await it, and
manage wasm memory themselves. The export list is generated from
`lob.h` at CMake configure time; adding a C function exports it
automatically.

### Wrapper

`index.mjs` instantiates the raw module once at import time (top-level
await) and provides a GC-managed API modeled on `lob.hpp`:

- `Builder`: chainable, camel-cased setters for every builder option.
  Enum setters also accept strings (`bcDragFunction("G7")`,
  `bcAtmosphere("ICAO")`) in addition to the exported
  `DragFunction`/`Atmosphere` values.
- `Context`: `solve(rangesFt)`, `solveYds(rangesYd)`,
  `solveInverse(rangesFt)`, `solveInverseYds(rangesYd)`,
  `fastInverse(rangesFt)`, and `dispose()`. `fastInverse` is a one-step
  approximation and skips zero ranges.
- Rows are plain objects: `rangeFt`, `rangeYds`, `velocityFps`,
  `energyFtLbs`, `timeOfFlightSec`, and
  `elevationInch`/`deflectionInch` for forward solutions or
  `elevationMoa`/`deflectionMoa` for inverse solutions.
- `LobError extends Error`: thrown by `build()` with a numeric `.code`
  and a named message. Solve functions do not throw for fall-short
  trajectories; they return fewer rows.
- `version()` and every unit conversion in `lob.h`.

`Builder.build()` throws instead of returning a context with an error
flag because JavaScript callers have no other reliable way to notice a
rejected configuration. The context is freed before the throw.

Array setters (`machVsDragTable`, `bcVelocityBands`,
`splineCoefficients`, `windProfile`) copy values into wasm memory and
retain those buffers until `reset()` or `dispose()`; the C builder
references the pointers until `Build()`. Heap views are re-read after
every allocation because memory growth detaches them. A
`FinalizationRegistry` frees builders, contexts, and retained buffers
if callers forget; `dispose()` is idempotent.

Ranges are validated in JavaScript before crossing the boundary:
finite, non-negative, no greater than 2^32 - 1, and strictly ascending.
Yard ranges are converted to whole feet. Invalid input throws
`RangeError`; wrong container types throw `TypeError`. Scalar
configuration values are left to the C builder so validation stays in
one place.

## Web solver example

`example/solver/` is a static, dependency-free consumer of the wrapper, not
package API. It follows
`docs/okf/solver-input-state-machine.md` and is assembled into
`build/wasm/solver/` by the wasm build. The docs deploy publishes it at
`/docs/solver/`.

## Build

The `wasm` CMake preset uses `cmake/emscripten.cmake`, which locates an
installed Emscripten through `$EMSDK`, `$EMSCRIPTEN`, or `emcc` on
`PATH` and never downloads an SDK. With Nix, `nix develop` provides
Emscripten 3.1.73 and Node. Without Nix, install and activate emsdk
3.1.73 and source `emsdk_env.sh`.

```sh
cmake --preset=wasm
cmake --build --preset=wasm
```

Artifacts land in `build/wasm/`:

- `lob.mjs`, `lob.wasm`: raw layer.
- `index.mjs`: wrapper, copied from `wasm/index.mjs` at configure time.
- `package.json`: generated from `wasm/package.json.in`, version from
  `project(VERSION)`.
- `README.md`, `COPYING.md`.

The package is ESM-only. `package.json` maps `.` to `index.mjs` and
`./raw` to `lob.mjs`. There are no TypeScript declarations in v1.

## Tooling

- `npm run build:wasm`: configure and build the preset.
- `npm test`: `node --test wasm/test/*.test.mjs example/solver/test/*.test.mjs`
  (build first).
- `npm run lint`: ESLint flat config.
- `npm run format` / `npm run format:check`: Prettier over the JS
  tooling and `wasm/` tree. Other repository files are not reformatted.

The test suite covers the canonical 168 gr .308 G7 trajectory,
forward/inverse fast solutions, fall-short row counts, all 91 raw
exports, heap growth, repeated builds after replacing retained arrays,
disposal, error codes, and input validation.

## Nix and CI

`flake.nix` provides `packages.lob-wasm`. Its `checkPhase` builds with
the preset, runs the smoke test, runs the Node suites, and checks the
solver bundle; it installs the package files and `solver/`.
`nix build .#lob-wasm` is the supported package check.

CI has one dedicated `wasm` job on Ubuntu with Nix that runs
`nix build .#lob-wasm`, and the lint job runs `npm ci`,
`npm run lint`, and `npm run format:check`. There is no cross-OS JS
test matrix in v1.

## Exhibits consumer (future)

Exhibits can import the wrapper or, for minimal payloads, the raw
layer. Docs embedding (frontmatter-driven script tags, artifact copies
into the Doxygen output) remains a separate effort. Adding exhibits
must not add exhibit-specific functions to this package.

## Deferred

- Publishing to npm from CI with provenance.
- TypeScript declarations.
- Automated browser smoke test.
- Cross-OS JS CI.
- Turning the exhibits harness into a shipped consumer.
