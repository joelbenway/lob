# Browser solver example

A static, dependency-free front end for the lob WebAssembly package. It is an
example consumer of the package, not part of the package API.

The input rules are documented in
[`docs/okf/solver-input-state-machine.md`](../../docs/okf/solver-input-state-machine.md).
The progressive form hides and disables gates that do not apply to the selected
drag, projectile, atmosphere, wind, and Earth-effect sources.

## Build and run

From the repository root:

```sh
nix develop --command npm run build:wasm
python3 -m http.server -d build/wasm 8000
```

Open <http://localhost:8000/solver/>. The wasm build assembles this directory
into `build/wasm/solver/`.

The deployed copy is at
<https://joelbenway.github.io/lob/docs/solver/>.
