# libemu Subagents

This project defines two specialized subagents to assist with coding and review workflows. They are configured for specific models to balance speed and reasoning capabilities.

## 1. Implementer (`implementer`)
**Role:** Writes and implements C99 emulator features, fixes, and unit tests.
**Model:** Flash 3.6 (call using `flash` model argument)
**Description:** Focuses on writing fast, accurate C99 code for target emulators, ensuring compatibility with `oracle` auto regression testing tool. Mindful of constraints for ESP32 and Cortex A7 targets.

## 2. Reviewer (`reviewer`)
**Role:** Reviews the code implemented by the implementer or the user.
**Model:** Gemini 3.1 Pro (call using `pro` model argument)
**Description:** Acts as a strict C99 expert. Reviews code against the `docs/STYLE.md` guidelines, memory safety practices, single responsibility rules, and integration checks.

## Usage
These subagents are active in the environment and can be invoked dynamically during development using the agent toolkit.

For example, to implement a new NES CPU op-code and then review it:
1. Invoke the `implementer` agent with `flash` model.
2. Invoke the `reviewer` agent with `pro` model to verify the resulting changes.
