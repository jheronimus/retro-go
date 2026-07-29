# ADR 0002: Removal of Custom Scaffolding and Frontends

## Status
Accepted

## Context
The raw imported cores contain extensive scaffolding meant for standalone execution on various operating systems or old, specific hardware platforms (MIPS, PSP, GP2X). They contain their own Makefiles, SDL1 hooks, GUI logic, platform-specific `#ifdef`s, and ad-hoc input parsing.

## Decision
1. **Strip Frontend Logic:** We will aggressively delete all rendering logic, audio output hooks (e.g., direct calls to DirectSound or ALSA), GUI drawing routines, and input polling libraries (e.g., SDL keyboard states) directly from the emulator core source code.
2. **Remove Legacy Scaffolding:** All platform-specific `#ifdef` blocks (e.g., `_WIN32`, `LINUX`, `PSP_BUILD`) and old standalone `Makefile`s will be excised.
3. **Build System:** We will establish a single unified build system (e.g. CMake) at the root of `libemu` to orchestrate building the cores as headless libraries or unified executables.

## Consequences
- The cores will no longer compile or run on their own as standalone applications out-of-the-box in their current form.
- The cores will be reduced to pure logic circuits that accept buffers (video, audio, input state) and step through cycles.
