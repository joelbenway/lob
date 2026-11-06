# JavaScript Library Design

Date: 2026-11-06
Status: Accepted

## Goal

Ship `@joelbenway/lob`: a JavaScript package backed by the same C++
solver as the native library, compiled to WebAssembly. The C API in
`include/lob/lob.h` is the single source of truth. The package targets Node 20+
and modern standard browsers (native ES modules), has no runtime dependencies,
and is usable without a bundler.

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
automatically. Type declarations are provided via `@joelbenway/lob/raw`.

### Wrapper

`index.mjs` instantiates the raw module once at import time (top-level
await) and provides a GC-managed API modeled on `lob.hpp`:

- `Builder`: chainable, camel-cased setters for every builder option.
  Enum setters also accept strings (`bcDragFunction("G7")`,
  `bcAtmosphere("ICAO")`) in addition to the exported
  `DragFunction`/`Atmosphere` values.
- `Context`: unit-bearing solver methods:
  - `solveFt(rangesFt)`: forward solution taking ranges in feet.
  - `solveYds(rangesYd)`: forward solution taking ranges in yards (rounded to integer feet).
  - `solveELRFt(rangesFt)`: extended long range forward solution with continuous atmospheric recalculation.
  - `solveELRYds(rangesYd)`: extended long range forward solution in yards.
  - `solveInverseFt(rangesFt)`: closed-form geometric inverse solution taking ranges in feet.
  - `solveInverseYds(rangesYd)`: closed-form geometric inverse solution taking ranges in yards.
  - `solveInverseELRFt(rangesFt)`: extended long range geometric inverse solution taking ranges in feet.
  - `solveInverseELRYds(rangesYd)`: extended long range geometric inverse solution taking ranges in yards.
  - `dispose()`: frees context wasm memory; idempotent.
- Rows are plain objects:
  - Forward rows expose `rangeFt`, `rangeYds`, `velocityFps`, `energyFtLbs`,
    `elevationInch`, `deflectionInch`, and `timeOfFlightSec`.
  - Inverse rows expose `rangeFt`, `rangeYds`, `velocityFps`, `energyFtLbs`,
    `elevationMoa`, `deflectionMoa`, and `timeOfFlightSec`.
  - The elevation and deflection units are distinct across forward and inverse types.
- `LobError extends Error`: thrown by `build()` with a numeric `.code`
  and a named message for ballistics domain violations.
- `version()` and every unit conversion in `lob.h`.

`Builder.build()` may be called multiple times to produce independent contexts.
If the configuration is rejected by the C solver, `build()` throws `LobError`
and frees the temporary context.

`Builder.reset()` resets configuration to default and frees retained array buffers.
`Builder.dispose()` and `Context.dispose()` are idempotent. Calling any operational
method on a disposed instance throws an `Error`.

Array setters (`machVsDragTable`, `bcVelocityBands`,
`splineCoefficients`, `windProfile`) copy values into wasm memory and
retain those buffers until replaced, `reset()`, or `dispose()`; the C builder
references the pointers until `Build()`. Heap views are re-read after
every allocation because memory growth detaches them. A
`FinalizationRegistry` frees builders, contexts, and retained buffers
if callers forget to call `dispose()`.

### Validation Policy

Validation responsibilities are divided cleanly:

1. **JavaScript Layer**:
   - Primitive type checking (`TypeError` for strings, booleans, null, undefined, boxed primitives, non-arrays, non-objects).
   - Numerical representation contract (`RangeError` for `NaN`, `+Infinity`, `-Infinity`).
   - Integer and range representation (`RangeError` for non-integers, out-of-range integer widths like `uint16`, or clock angle not in 1..12).
   - Array shapes and lengths (`TypeError` for non-array/view; `RangeError` for mismatched lengths or coefs length != 60).
   - Range arrays: finite, non-negative, strictly ascending, within uint32 bounds (`RangeError`). Empty array returns `[]`.
2. **C Library Layer**:
   - Physical and ballistics-domain bounds checking (e.g., transonic thresholds, minimum projectile mass, stability).
   - Throws `LobError` upon `build()`.
   - Fall-short trajectories terminate gracefully and return reached rows without throwing.

## Web solver example

`example/loblab/` is a static, dependency-free consumer of the wrapper, not
package API. It is assembled into `build/wasm/loblab/` and `build/wasm/solver/`
by the wasm build.

## Build and Packaging

The `wasm` CMake preset uses `cmake/emscripten.cmake`.

```sh
cmake --preset=wasm
cmake --build --preset=wasm
```

Artifacts land in `build/wasm/`:

- `lob.mjs`, `lob.wasm`: raw layer.
- `index.mjs`: wrapper.
- `index.d.ts`, `lob.d.ts`: handwritten TypeScript declaration files.
- `package.json`: generated from `wasm/package.json.in`, version from
  `project(VERSION)`.
- `README.md`, `COPYING.md`.

The package is ESM-only. `package.json` maps `.` to `index.mjs` with `index.d.ts`,
and `./raw` to `lob.mjs` with `lob.d.ts`.

## Tooling

- `npm run build:wasm`: configure and build the preset.
- `npm test`: Node test runner over `wasm/test/*.test.mjs` and `example/loblab/test/*.test.mjs`.
- `npm run test:types`: TypeScript type checking over consumer fixtures.
- `npm run test:browser`: Browser smoke test across headless Chromium and Firefox.
- `npm run lint`: ESLint flat config.
- `npm run format` / `npm run format:check`: Prettier over JS tooling, `wasm/`, and `example/loblab/`.

## Deferred

- Publishing to npm from CI with provenance.
- Turning the exhibits harness into a shipped consumer.
