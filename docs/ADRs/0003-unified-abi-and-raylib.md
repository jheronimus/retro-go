# ADR 0003: Unified ABI and Raylib Frontend (Superseded)

## Status
Superseded (by ADR 0006)

## Context
With the frontend logic stripped from individual cores (ADR 0002), we initially planned to design a single, consistent ABI to interface with them programmatically, alongside a unified testing frontend using Raylib.

## Decision
**This decision has been superseded by the pivot to `retro-go` (ADR 0006).**
1. **No Custom ABI Needed:** Because we are integrating directly into the `retro-go` framework, we no longer need a bespoke `libemu` ABI. The cores simply need to compile directly into the target binary (for G32 and SDL2) and conform to `retro-go`'s internal entry-point lifecycle (`_main()`).
2. **No Raylib Port Needed:** `retro-go` natively includes a highly optimized SDL2 wrapper for testing and debugging on desktop machines (like Mac). The SDL2 target provides all the visual and manual verification capabilities we need without introducing Raylib as a secondary frontend dependency.

## Consequences
- Less abstraction overhead: Cores hook directly into the `retro-go` application lifecycle instead of an intermediate ABI layer.
- Consolidated tooling: Visual testing happens via the native `retro-go` SDL2 build rather than a separate Raylib app.
