# Expanded Notes and Instrument Implementation Plan

**Goal:** Support Bb2 and a default Bb2–Bb3 range, exact pitch selection, and a recognizable trombone illustration.
**Architecture:** Extend the explicit pitch table to 17 chromatic pitches Bb2–D4. Intersect the range with a per-pitch bit mask. Keep UI in PGE3, with a paginated toggle grid. Render the instrument with a single batch of PGE3 primitives, using the same geometry for offline preview inspection.
**Constraints:** Concert pitch; no trigger/pedal/seventh-partial answers; preserve octave-free question headings and 7-to-1 selector. Include octaves on exact-pitch buttons. Reject empty question pools; preserve selection on replay.

- [x] Verify lower-note positions against primary chart; add failing core tests, then implement data/default/filter/empty-pool behavior.
- [x] Add exact pitch picker, All/None controls, pagination, and disabled Start for empty selection.
- [x] Redraw and inspect trombone at positions 1/4/7 and compact size; verify constant outer-slide length, fixed geometry and overlap.
- [x] Run native and web builds/tests, review integration and document results.

Validation: full native/web builds pass; native CTest 2/2, core 1,381 checks natively and WebAssembly/Node. Genuine CPU notation probe passed all 34 pitch/clef combinations with no parse/font errors. Instrument preview and exact-picker layout reviewed; interactive browser/touch verification remains sandbox-blocked.
