---
type: DesignDecision
title: C ABI Narrow Waist
description: One C implementation with a C++ wrapper above it.
tags: [architecture, api]
generated: {by: human:joel, at: 2026-10-09T00:00:00Z}
verified: [{by: human:joel, at: 2026-10-09T00:00:00Z}]
status: stable
---

# C ABI Narrow Waist

lob exposes a C ABI with plain aggregates and an opaque builder, with a
header-only C++ wrapper above it that restores type-safe overloads and
scoped enums. The implementation lives once, in C++; the wrapper
forwards to it.

The hourglass shape keeps the solver independent of language bindings:
any language with a C FFI can call lob without writing C++ or fighting
name mangling, while C++ callers get ergonomics back through
the wrapper. See @ref api_builder for the builder surface.
