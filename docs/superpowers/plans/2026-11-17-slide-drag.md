# Horizontal slide dragging implementation plan

**Goal:** Grab the moving slide with mouse or touch, drag horizontally, and select
positions 1–7 alongside the existing vertical selector.

**Architecture:** Use one dependency-free geometry/drag header shared by the
renderer and game input. Capture either column or slide for the duration of a
gesture. Preserve the press offset and snap with hysteresis.

**Stack:** C++20, existing PGE3 pointer APIs, native and Emscripten CMake builds.
**Spec:** The approved drag interaction in this conversation.

- [x] Add `src/SlideInput.h` and extend `tests/core_tests.cpp` with failing
  examples for scale/centering, padded slide hits, no press jump, all seven
  positions, clamped endpoints, and stable reverse snapping; implement and pass.
- [x] Use `InstrumentGeometry::make(x,y,w,h)` in `src/Trombone.h` to share the
  existing 1000x180 design transform and 50-unit position step. Compare recorded
  primitives before and after at all seven positions to preserve the drawing.
- [x] Replace `Game`'s drag boolean with `SlideDrag`. Column captures read y;
  slide captures read x. Finish on release, cancel on resize or leaving gameplay,
  and preserve PGE3 touch ownership and mouse suppression.
- [x] Verify focus/cancel behavior against the pinned PGE3 header and review
  padded hit regions at phone/desktop dimensions, including position seven.
- [x] Run native build/CTest, Emscripten web-site build, and core tests as Wasm
  under Node. Attempt browser interaction only if Chrome can launch; distinguish
  calculated/CPU evidence from graphical or physical touch testing.
- [x] Document both input methods and refresh the local four-file web ZIP.
