# ADR 0006: Forking retro-go and ESP-IDF Integration

## Status
Accepted

## Context
While our goal is to build highly portable, strictly clean C99 emulator cores for low-end hardware (like ESP32 and Cortex A7), writing the target embedded OS frontend, display drivers, audio drivers, and SD card handlers for ESP32 from scratch is redundant. The `ducalex/retro-go` repository already provides a highly mature, ESP-IDF-based launcher and driver framework optimized for exactly these constrained environments. 

## Decision
1. **Repo Transition:** This repository will formally transition into a fork of `ducalex/retro-go`. We will ingest the `retro-go` framework as our primary embedded shell.
2. **Core Replacement:** We will aggressively delete *all* existing emulator cores bundled within `retro-go` (e.g., their versions of `gnuboy`, `nofrendo`, `pce-go`, etc.). 
3. **Core Injection:** We will replace the deleted cores exclusively with our four newly refactored, heavily verified `libemu` cores (`libxnes`, `smsplus-gx`, `temper`, and `walnut_cgb`).
4. **ESP-IDF Testing Harness:** On top of our existing `oracle` testing suite, we will establish an ESP-IDF testing harness. This pipeline will use Espressif's native tools (like `idf.py size`) to continuously verify that our cores successfully compile against the `retro-go` framework and remain strictly within the extreme memory constraints of the ESP32 platform.

## Consequences
- We inherit a proven embedded launcher (`retro-go`) instead of writing our own from scratch.
- We maintain our strict quality control by substituting their unverified cores with our `oracle`-verified C99 cores.
- We guarantee extreme portability by testing memory bounds through `ESP-IDF` directly alongside accuracy testing.
