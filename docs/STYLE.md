# libemu C99 Style Guidelines

This document sets coding standards for `libemu`. All modules must follow these rules uniformly to ensure fast, clean, and practically accurate C99 emulation for NES/FDS, GB/GBC, PCE/PCECD/SGFX, and SMS/GG across hardware from ESP32 to ARM Cortex A7.

## 1. Standard
The codebase strictly targets **C99**. Always use standard integer types (`<stdint.h>`, e.g., `uint8_t`, `uint16_t`, `uint32_t`) for hardware emulation to ensure portability across architectures.

## 2. Single Responsibility
Each module has one clearly stated purpose, reflected in its filename. Use modular `.c` and `.h` pairs; keep public interfaces in headers.

## 3. Module Header
Every file starts with a comment block explaining its purpose and design context.

## 4. Internal Ordering
Within a source file:
1. Public headers, then private headers
2. Preprocessor macros / Constants
3. Public functions
4. Private static helper functions

## 5. File Size Limit
**500 SLOC maximum.** Blank lines, comments, and docstrings do not count. Crossing this requires splitting the file.

## 6. Function Size & Complexity
- **20 SLOC maximum** (excluding blank lines, comments, docstrings).
- **3 indentation levels maximum.** Deeper nesting requires extracting a helper function.
- All function signatures must be fully typed.
- Don't put business logic in `main()`; avoid `goto` except for unified cleanup/resource release.

## 7. Dependencies & Memory Safety
Prefer the standard library. Validate memory usage with AddressSanitizer/Valgrind during host tests. 
Prefer stack memory allocation; use heap allocation (`malloc`/`calloc`) only when strictly necessary, especially given the memory constraints of targets like the ESP32. Pre-allocate emulator state at initialization and avoid dynamic allocation in the hot path.

## 8. Function Comments
Every function declaration in headers must document its behavior, parameters, and return value (Doxygen style `///` or `/** */`):
```c
/// One-line summary.
/// @param arg Description.
/// @return Description.
```

## 9. Performance & Emulation Rules
- Mark short, frequently called functions (e.g., memory read/write) as `static inline`.
- Be mindful of data alignment and struct padding.
- All new implementations and fixes must be verified against the `oracle` auto regression testing tool.
