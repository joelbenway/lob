# Type Consumer Fixture

This fixture tests `@joelbenway/lob` TypeScript declaration consumption.

## Prerequisites

Before running type checking:

1. Build the WebAssembly package:
   ```sh
   npm run build:wasm
   ```
2. Install the local package dependency (automatically performed by `pretest:types`):
   ```sh
   npm --prefix wasm/test/types install
   ```
3. Run type verification:
   ```sh
   npm run test:types
   ```
