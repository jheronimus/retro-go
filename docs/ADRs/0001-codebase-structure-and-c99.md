# ADR 0001: Unified C99 Codebase Structure

## Status
Accepted

## Context
We are importing diverse emulator cores (`Walnut_CGB`, `temper`, `smsplus-gx`, `libxnes`) into a single `libemu` project. Some are written as monolithic single files (e.g., `Walnut_CGB`), while others have complex and messy file structures heavily coupled to old hardware APIs. We need a clean, uniform code structure across all cores to ensure maintainability, extreme portability (from microcontrollers to desktop), and a unified project standard.

## Decision
We will use `libxnes` as our architectural reference standard for all imported cores.
1. **Clean C99:** All emulator cores will strictly conform to the C99 standard (no C++ or platform-specific MSVC/GCC extensions).
2. **Separation of Concerns:** Monolithic files will be eliminated. Each emulator will be refactored into distinct, strictly bounded components (e.g., `cpu.c`, `ppu.c`, `apu.c`, `mapper/`, `cartridge.c`).
3. **No Globals (State Encapsulation):** Where possible, system state will be encapsulated inside a state struct, ensuring the core has no hardcoded static globals that prevent multiple instances of the emulator from running simultaneously.

## Consequences
- Requires breaking up `Walnut_CGB` into separate subsystem files.
- `temper` and `smsplus-gx` codebases will require significant reorganization.
- Increases legibility and makes it significantly easier to run automated unit tests against individual subsystems (e.g., CPU, APU).
