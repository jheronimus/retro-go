# ADR 0003: Unified ABI and Raylib Frontend

## Status
Accepted

## Context
With the frontend logic stripped from individual cores (ADR 0002), we need a single, consistent way to interface with them programmatically. We also need a modern, simple frontend to test the games visually and manually verify hardware fidelity beyond automated testing.

## Decision
1. **Clean ABI (`libemu` API):** We will design a clean, unified Application Binary Interface (ABI). Every core will implement exactly the same function pointer interface (e.g., `emu_init()`, `emu_load_rom()`, `emu_run_frame()`, `emu_set_input()`). Cores will emit audio and video blindly to memory buffers provided by the host. This simplifies integrating our cores into 3rd-party projects.
2. **Raylib Frontend:** We will implement a single, unified testing frontend using the **Raylib** framework. Raylib is extremely lightweight, fast to iterate on, and compiles natively everywhere (including WebAssembly if needed).
3. **Core Loading:** The Raylib frontend will load the specific core libraries dynamically or link them statically, rendering the exposed video buffer and playing the exposed audio buffer via Raylib's native API.

## Consequences
- Integrators and UI developers only need to understand one API contract to run *any* supported console.
- Significant reduction in duplicate boilerplate code across the project.
