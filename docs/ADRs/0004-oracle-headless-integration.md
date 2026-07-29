# ADR 0004: Oracle Headless Integration via Standalone CLI

## Status
Accepted

## Context
The `oracle` automated test suite requires a non-interactive, deterministic execution environment. While Libretro wrappers are supported, `oracle` also supports standalone command-line executables.

## Decision
1. **Standalone CLI Wrapper:** Instead of turning our cores into `libretro` libraries, we will write a generic Standalone CLI wrapper that natively hooks into our unified `libemu` ABI (defined in ADR 0003). 
2. **Memory & I/O Exposure:** The CLI wrapper will map and expose the internal WRAM, SRAM, serial console outputs, and required debugging symbols directly to `oracle` using standard execution signals (e.g., writing `"PASS"` or `"FAIL"` strings directly to memory blocks or stdout/save files per the `oracle` specs).
3. **Headless Execution:** The CLI mode will strip frame-time delays, allowing the emulator to process frames as fast as possible until the test asserts a PASS/FAIL state or times out.

## Consequences
- We bypass the overhead of supporting the massive `libretro.h` API contract.
- Testing is entirely self-contained.
- We have absolute control over the memory layout and serial console reporting required by `oracle`.
