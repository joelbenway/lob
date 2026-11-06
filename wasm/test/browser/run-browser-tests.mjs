// Copyright (c) 2026  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

import http from "node:http";
import fs from "node:fs";
import path from "node:path";
import os from "node:os";
import { spawn, spawnSync } from "node:child_process";
import { fileURLToPath } from "node:url";

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const repoRoot = path.resolve(__dirname, "../../..");
const wasmDir = path.resolve(repoRoot, "wasm");
const buildWasmDir = path.resolve(repoRoot, "build/wasm");

const MIME_TYPES = {
  ".html": "text/html; charset=utf-8",
  ".mjs": "application/javascript; charset=utf-8",
  ".js": "application/javascript; charset=utf-8",
  ".wasm": "application/wasm",
  ".json": "application/json; charset=utf-8",
  ".d.ts": "text/plain; charset=utf-8",
  ".ts": "text/plain; charset=utf-8",
};

function resolveBinary(explicitEnv, binNames, nixPackage) {
  if (explicitEnv && fs.existsSync(explicitEnv)) {
    return { command: explicitEnv, prefixArgs: [] };
  }
  for (const name of binNames) {
    const which = spawnSync("which", [name], { encoding: "utf-8" });
    if (which.status === 0 && which.stdout.trim()) {
      return { command: which.stdout.trim(), prefixArgs: [] };
    }
  }
  const whichNix = spawnSync("which", ["nix"], { encoding: "utf-8" });
  if (whichNix.status === 0 && whichNix.stdout.trim()) {
    return {
      command: "nix",
      prefixArgs: [
        "shell",
        "--substituters",
        "https://cache.nixos.org",
        `nixpkgs#${nixPackage}`,
        "--command",
        binNames[0],
      ],
    };
  }
  return null;
}

function createTestServer() {
  let pendingReportResolver = null;

  const server = http.createServer((req, res) => {
    const parsedUrl = new URL(req.url, `http://${req.headers.host}`);
    const pathname = parsedUrl.pathname;

    if (req.method === "POST" && pathname === "/api/report") {
      let body = "";
      req.on("data", (chunk) => {
        body += chunk;
      });
      req.on("end", () => {
        try {
          const data = JSON.parse(body);
          if (pendingReportResolver) {
            pendingReportResolver(data);
          }
          res.writeHead(200, { "Content-Type": "application/json" });
          res.end(JSON.stringify({ ok: true }));
        } catch (err) {
          res.writeHead(400, { "Content-Type": "application/json" });
          res.end(JSON.stringify({ error: err.message }));
        }
      });
      return;
    }

    if (req.method !== "GET" && req.method !== "HEAD") {
      res.writeHead(405);
      res.end("Method Not Allowed");
      return;
    }

    let filePath = null;
    if (pathname === "/" || pathname === "/index.html") {
      filePath = path.join(__dirname, "index.html");
    } else if (pathname === "/test-runner.mjs") {
      filePath = path.join(__dirname, "test-runner.mjs");
    } else if (pathname.startsWith("/package/")) {
      const rel = pathname.slice("/package/".length);
      const candidates = [
        path.join(wasmDir, rel),
        path.join(buildWasmDir, rel),
      ];
      for (const cand of candidates) {
        if (fs.existsSync(cand) && fs.statSync(cand).isFile()) {
          filePath = cand;
          break;
        }
      }
    }

    if (!filePath || !fs.existsSync(filePath)) {
      res.writeHead(404, { "Content-Type": "text/plain" });
      res.end(`Not Found: ${pathname}`);
      return;
    }

    const ext = path.extname(filePath);
    const mime = MIME_TYPES[ext] || "application/octet-stream";
    const stat = fs.statSync(filePath);
    res.writeHead(200, {
      "Content-Type": mime,
      "Content-Length": stat.size,
      "Cache-Control": "no-store",
    });
    fs.createReadStream(filePath).pipe(res);
  });

  return {
    server,
    waitForReport() {
      return new Promise((resolve) => {
        pendingReportResolver = resolve;
      });
    },
    resetResolver() {
      pendingReportResolver = null;
    },
  };
}

async function runBrowser(
  name,
  binSpec,
  browserArgsFactory,
  serverUrl,
  testServer,
) {
  const tmpDir = fs.mkdtempSync(path.join(os.tmpdir(), `lob-${name}-test-`));
  const fullArgs = [
    ...binSpec.prefixArgs,
    ...browserArgsFactory(tmpDir, serverUrl),
  ];

  console.log(`\nLaunching ${name}...`);
  const reportPromise = testServer.waitForReport();

  let child;
  try {
    child = spawn(binSpec.command, fullArgs, {
      stdio: ["ignore", "pipe", "pipe"],
    });
  } catch (err) {
    fs.rmSync(tmpDir, { recursive: true, force: true });
    testServer.resetResolver();
    throw new Error(`Failed to spawn ${name}: ${err.message}`, { cause: err });
  }

  let stdout = "";
  let stderr = "";
  child.stdout.on("data", (d) => {
    stdout += d.toString();
  });
  child.stderr.on("data", (d) => {
    stderr += d.toString();
  });

  const timeoutMs = 30000;
  const timeoutPromise = new Promise((_, reject) => {
    const timer = setTimeout(() => {
      reject(
        new Error(
          `${name} test timed out after ${timeoutMs}ms.\nStdout: ${stdout}\nStderr: ${stderr}`,
        ),
      );
    }, timeoutMs);
    timer.unref();
  });

  try {
    const report = await Promise.race([reportPromise, timeoutPromise]);
    if (report.status !== "pass") {
      throw new Error(
        `${name} test failed: ${report.error}\nStack:\n${report.stack}`,
      );
    }
    console.log(`✔ ${name} passed (${report.checksCount} checks)`);
    console.log(`  User-Agent: ${report.userAgent}`);
    return report;
  } finally {
    testServer.resetResolver();
    child.kill("SIGKILL");
    try {
      fs.rmSync(tmpDir, { recursive: true, force: true });
    } catch {
      // Ignore cleanup error
    }
  }
}

async function main() {
  console.log("=== @joelbenway/lob Browser Smoke Tests ===");

  const { server, waitForReport, resetResolver } = createTestServer();
  await new Promise((resolve) => server.listen(0, "127.0.0.1", resolve));
  const port = server.address().port;
  const serverUrl = `http://127.0.0.1:${port}/index.html`;
  console.log(`Test server running at ${serverUrl}`);

  const browsers = [
    {
      name: "Chromium",
      binSpec: resolveBinary(
        process.env.CHROMIUM_BIN || process.env.CHROME_BIN,
        ["chromium", "google-chrome", "chromium-browser"],
        "chromium",
      ),
      args: (tmpDir, url) => [
        "--headless=new",
        "--no-sandbox",
        "--disable-gpu",
        `--user-data-dir=${tmpDir}`,
        url,
      ],
    },
    {
      name: "Firefox",
      binSpec: resolveBinary(process.env.FIREFOX_BIN, ["firefox"], "firefox"),
      args: (tmpDir, url) => [
        "--headless",
        "--no-remote",
        "--profile",
        tmpDir,
        url,
      ],
    },
  ];

  const results = [];
  try {
    for (const b of browsers) {
      if (!b.binSpec) {
        throw new Error(
          `Could not locate executable for ${b.name}. Install it or set environment variable.`,
        );
      }
      const res = await runBrowser(b.name, b.binSpec, b.args, serverUrl, {
        waitForReport,
        resetResolver,
      });
      results.push({ name: b.name, result: res });
    }
  } finally {
    await new Promise((resolve) => server.close(resolve));
  }

  console.log("\n===========================================");
  console.log("All browser tests passed successfully!");
  for (const r of results) {
    console.log(`- ${r.name}: ${r.result.checksCount} assertions verified.`);
  }
  console.log("===========================================\n");
}

main().catch((err) => {
  console.error("\n❌ Browser test runner error:", err);
  process.exit(1);
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
