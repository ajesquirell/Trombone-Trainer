# Note Range and Layout Update Implementation Plan

> **For agentic workers:** Execute the following tasks and verify results before completion.

**Goal:** Let players choose an inclusive pitch range, reverse the position column, correct the instrument orientation, and omit octave numbers on question labels.
**Architecture:** Keep explicit pitch/octave data. Filter the existing verified seven-note pool by session settings. Use the existing PGE3 settings controls and draw primitives.
**Tech Stack:** Existing C++20, PGE3, Lomse, CMake.
**Spec:** User's follow-up request in this conversation.

## Constraints and decisions
- Supported range remains A-flat 3 through D4; no unverified note mappings are added.
- Low/high bounds are inclusive; singleton ranges repeat safely. Replay preserves settings.
- Range pickers include octaves to distinguish bounds; question names omit them.
- Selector is seven equal full-height bands, 7 at top and 1 at bottom.
- Bell and moving slide both face right; fixed parts remain stationary.
- PGE3 controls only. Use short-layout tabs where all settings cannot fit simultaneously.

## Tasks
- [x] Add failing tests, then implement pitch-range filtering, normalized bounds, display names, and reversed input mapping.
- [x] Update range controls, selector labels and bell/slide geometry.
- [x] Build native core/diagnostic and full WebAssembly application, review integration, document exact results and limits.

## Validation
Both full native Linux and WebAssembly applications build. Native CTest passes2/2; core943checks pass both natively and underWebAssembly/Node. Independent core/layout/integration review found no remaining important issues after correcting the range heading width. Visual/touch runtime validation remains blocked by sandbox display/browser restrictions. Details: docs/validation/STATUS.md.
