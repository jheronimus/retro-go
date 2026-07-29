# ADR 0005: Shared Abstracted Helpers

## Status
Accepted

## Context
Many common emulator tasks—such as parsing ZIP archives, rendering filters/scaling, resampling audio, managing input mappings, and serializing memory for savestates/rewind—are traditionally duplicated inside every core's source tree. 

## Decision
We will move all shared functional code into an abstracted `common/` or `libemu_utils/` library.
1. **Data Loading:** ZIP and CHD archive extraction will be managed entirely by the helper library, handing pure raw byte arrays down to the specific console core's `load_rom` function.
2. **Audio/Video:** Features like audio resampling, video scaling algorithms, and color palette generation will be unified and abstracted.
3. **Savestates/Rewind:** The helper library will implement a generic ring-buffer mechanism for rewind and fast-forward, calling the core's `save_state` and `load_state` serialization callbacks asynchronously.

## Consequences
- Individual emulator cores become significantly smaller and focus strictly on hardware logic.
- Adding a feature (like CHD support or Rewind) to the helper library automatically benefits every core simultaneously.
