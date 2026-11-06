# @joelbenway/lob

Exterior ballistics calculation library, compiled from GPL-3.0-or-later C++ to WebAssembly.
Provides fluent JavaScript and TypeScript APIs for Node 20+ and modern standard browsers.

## Installation

```sh
npm install @joelbenway/lob
```

## Quick Start

```js
import { Builder, DragFunction } from "@joelbenway/lob";

// Configure a projectile and zero
const context = new Builder()
  .ballisticCoefficientPsi(0.243)
  .bcDragFunction(DragFunction.G7) // or string "G7"
  .diameterInch(0.308)
  .massGrains(168)
  .initialVelocityFps(2700)
  .zeroDistanceYds(100)
  .build();

// Forward trajectory in yards (returns drop and deflection in inches)
const forwardRows = context.solveYds([0, 100, 200, 300]);
for (const row of forwardRows) {
  console.log(
    `${row.rangeYds} yd: drop ${row.elevationInch} in, vel ${row.velocityFps} fps`,
  );
}

// Inverse sight adjustments in yards (returns elevation and windage in MOA)
const inverseRows = context.solveInverseYds([100, 200, 300]);
for (const row of inverseRows) {
  console.log(`${row.rangeYds} yd: dial ${row.elevationMoa} MOA up`);
}

// Release WebAssembly memory when finished
context.dispose();
```

## API Highlights

- **Unit-Bearing Names**: Quantities carry explicit unit suffixes (`diameterInch`, `initialVelocityFps`, `zeroDistanceYds`, `solveFt`, `solveYds`).
- **Forward vs Inverse Solutions**:
  - `solveFt(ranges)` / `solveYds(ranges)` return drop and deflection in **inches**.
  - `solveInverseFt(ranges)` / `solveInverseYds(ranges)` return elevation and windage corrections in **MOA**.
  - `solveInverse*` uses a closed-form geometric correction on top of the forward solution (no iteration).
- **Extended Long Range (ELR)**:
  - `solveELRFt`, `solveELRYds`, `solveInverseELRFt`, `solveInverseELRYds` continuously recalculate ambient air density, air pressure, and speed of sound at each integration step as projectile altitude changes.
- **Fall-Short Trajectories**: If a projectile impacts ground or drops below configured thresholds, solvers return only reached rows without throwing.
- **Resource Management**: Instances manage WASM memory automatically via `FinalizationRegistry`, but explicit `dispose()` is recommended for predictable resource reclamation.

## Low-Level Raw API

For direct access to the underlying Emscripten C ABI functions, import the module factory from `@joelbenway/lob/raw`:

```js
import createLobModule from "@joelbenway/lob/raw";

const wasm = await createLobModule();
console.log(`lob version: ${wasm.UTF8ToString(wasm._LobVersion())}`);
```

## Documentation

Full class, method, and OKF conceptual documentation is available in the project's generated Doxygen Technical Reference (`build/docs/html/index.html` or online documentation):

- [Builder Reference](classBuilder.html)
- [Context Reference](classContext.html)
- [LobError Reference](classLobError.html)

## License

GPL-3.0-or-later. See `COPYING.md`.
