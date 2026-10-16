---
type: DesignDecision
title: No Dynamic Allocation
description: No heap, no throws — suit the strictest callers.
tags: [architecture, embedded]
generated: {by: human:joel, at: 2026-10-09T00:00:00Z}
verified: [{by: human:joel, at: 2026-10-09T00:00:00Z}]
status: stable
---

# No Dynamic Allocation

lob never allocates from the heap and never throws. Builders stage
into an inline buffer, tables are borrowed rather than copied, and the
integration state is a small fixed footprint.

The motive is callers the library author never meets: embedded targets
and real-time contexts that forbid heap use, and integrators who want
to vendor the library with no allocator hooks. The consequence is a
caller contract — borrowed tables must outlive the build — stated on
the builder surface rather than hidden.
