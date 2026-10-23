# @joelbenway/lob

Exterior ballistics calculation library, compiled from GPL-3.0-or-later
C++ to WebAssembly. Runs on Node 20+; browser support is not yet tested.

## Usage

```js
import { Builder } from "@joelbenway/lob";

const context = new Builder()
  .ballisticCoefficientPsi(0.243)
  .bcDragFunction("G7")
  .diameterInch(0.308)
  .massGrains(168)
  .initialVelocityFps(2700)
  .zeroDistanceYds(100)
  .build();

const rows = context.solveYds([0, 100, 200, 300]);
for (const row of rows) {
  console.log(row.rangeYds, row.velocityFps, row.elevationInch);
}
```

The wrapper is chainable and camelCased from the C++ API. Units are part
of each method name. `solve` takes ranges in feet; `solveYds` takes yards.
Forward solutions return `elevationInch`/`deflectionInch`; inverse
solutions return `elevationMoa`/`deflectionMoa`. Fall-short trajectories
return fewer rows. `solveInverse` is a forward solve plus the closed-form
geometric correction with no iteration. The `solveELR` and `solveInverseELR`
variants recompute air density and the speed of sound along the flight for
extreme-range shots; use them when altitude changes can matter.

The raw C API is available as `@joelbenway/lob/raw`: import the default
factory, await it, then call the `Lob*` functions (for example
`module._LobVersion()`). Memory for the wrapper is managed automatically;
Builder and Context objects also expose `dispose()`.

## License

GPL-3.0-or-later. See `COPYING.md`.
