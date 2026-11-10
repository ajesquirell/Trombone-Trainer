# Validation record

## Update: GitHub Pages preparation

Added a pinned GitHub Actions workflow to run core tests, build with Emscripten
6.0.10, and upload/deploy the four-file browser site on pushes to `main` or manual
dispatch. Added the CMake `web-site` target and `cmake/StageWeb.cmake` for staging
the HTML as `index.html` and preserving JS/Wasm/data filenames.

- Local `cmake --build build-web --target web-site -j2`: **PASS**.
- Staged directory contents, HTML script reference, and byte-for-byte comparisons
  with all four generated build files: **PASS**.
- Workflow YAML parsing, trigger, permissions, full action pins, and deploy/build
  dependency checks: **PASS**; independent static workflow review found no blockers.
- Clean core-only configure/build/CTest using the workflow commands: **PASS**.

No commit, push, repository setting change, or remote deployment was performed.
The repository owner must select Pages Source **GitHub Actions** and push the new
files. The expected URL after a successful deployment is
`https://ajesquirell.github.io/Trombone-Trainer/`. The remote Actions run and live
site remain unverified.

## Update: straight neck and foreground slide

Replaced the curved return neck with a horizontal tube connecting directly to
the lower slide rail. Raised the complete slide assembly and mouthpiece together;
the slide is drawn after the bell so it overlaps in front. Reduced the uniformly
scaled design box to 1000x180 to fit the flatter silhouette.

- Native and Emscripten rebuilds: **PASS**.
- Native CTest: **2/2 PASS**.
- Regenerated and inspected full-size and compact same-code previews.
- Recorded all seven positions: **PASS** bounds checks; 1,118 fixed primitives
  remain identical, while 270 moving primitives translate rigidly and evenly.
- Updated `trombone-preview.png`. It remains an offline geometry preview;
  interactive browser rendering was not executed.

## Update: B-flat 2, exact note picker, and instrument redraw

The pool now contains 17 chromatic concert pitches, B-flat 2 through D4. Session
defaults are B-flat 2 through B-flat 3 inclusive (13 pitches). The Notes screen
toggles exact pitches and octaves, with All/None controls, pagination, disabled
out-of-range entries, and a disabled Start when no eligible pitches are selected.
Selection and range persist on replay. Mapping sources and all accepted positions
are listed in NOTE-SOURCES.md.

The instrument is now a uniformly scaled brass side view: curved hollow bell,
rear tuning bow/gooseneck, silver inner rails and a rigid moving outer slide.
Game and offline preview share `src/Trombone.h`; all drawing uses one PGE3 batch.
`trombone-preview.png` is a same-code geometry preview, not a browser screenshot.

Executed validation:

- Full native Linux Release build and full Emscripten build: **PASS**.
- Native CTest: **2/2 PASS**; core executable: **1,381 checks, 0 failures**.
- Same core compiled with Emscripten and executed by Node: **1,381 checks, 0 failures**.
- Agent-executed strict GCC and UBSan core runs: **PASS**.
- Exact-picker source/layout review at 1120x720, 740x360, 640x360, 375x740,
  375x500, and 375x300: no important issues. Grid labels toggle their matching
  pitch-mask bits; Back, page controls, and Start remain separate targets.
- Offline instrument recorder: all seven positions remain within bounds;
  fixed primitives remain identical, moving primitives translate evenly with
  constant length, and sleeves retain at least 85 design units of overlap.
  Positions 1, 4, 7 and a compact preview were rendered and visually inspected.
- Built the native `trombone-preview` CMake target and ran
  `python3 tools/preview_trombone.py build-native/trombone-preview`: **PASS**.
- Genuine Lomse CPU engraving probe, run without DISPLAY/WAYLAND_DISPLAY:
  **all 17 pitches in both clefs produced 34 distinct, nonempty, edge-safe RGBA
  bitmaps**, with no parse or font errors. Visually inspected the B-flat 2 and
  B-flat 3 endpoint PNGs in both clefs, including treble ledger lines. Probe:
  `/tmp/trombone-lower-notation-probe.cpp`, linked to the actual native Lomse,
  FreeType and PNG libraries and using the production sizing/scale/crop settings.

CPU notation and recorded illustration previews do not verify GPU upload,
browser display, or physical touch interaction. Those runtime checks remain
unavailable under the display/browser restrictions recorded below.

## Update: note range and layout

Implemented inclusive Low/High note settings for the existing A-flat 3–D4 pool,
with a genuine Lomse endpoint preview where space permits. Short, narrow settings
use Exercise/Note range tabs; all settings buttons remain at least 44px tall.
Bounds cannot cross through the controls, a single-note range repeats safely,
and replay preserves the range. Question names omit octaves, while notation and
pitch data retain them. The selector labels and coordinate mapping now run from
7 at the top to 1 at the bottom. The fixed bell opens right above the moving
slide; the slide tubes and bow move together and retain overlap at every position.

Executed after the changes:

- Full native Linux Release compile/link: **PASS**, both `notation-check` and
  `trombone-trainer`. This supersedes the earlier native download-blocked result.
- Native CTest: **2/2 PASS** (core and PGE3 font preservation).
- Core executable: **943 checks, 0 failures**, native and Emscripten/Node.
- Full Emscripten application rebuild: **PASS**, HTML/JS/Wasm/data generated.
- Strict native UI syntax check: **PASS**. Agent-executed core UBSan: **PASS**.
- Read-only integration and calculated layout review: no important issues remain
  at 1120x720, 740x360, 640x360, 375x740, 375x500, and 375x300. A long heading
  found during review was shortened. At 300px height, seven full-height segments
  are necessarily about 43px tall.

Native validation exposed two additional Lomse 0.30.0 integration assumptions:
its public view headers require FreeType includes to propagate to consumers,
and its wedge engraver uses `std::numeric_limits` without including `<limits>`.
Both are corrected through CMake, preserving downloaded sources. The full
native build used the already cached Lomse source through
`FETCHCONTENT_SOURCE_DIR_LOMSE`; no SDK or external checkout was edited.

Browser/native visual appearance and physical touch interaction after these
changes remain unverified due to the display/browser sandbox restrictions
recorded below. The musictheory.net reference was consulted through the web
reader; its interactive exercise requires JavaScript and could not be operated
there, so no exact visual match is claimed.

## Update: corrupted UI font

The user's browser screenshot shows genuine Lomse bass-clef notation with three
quarter notes and ledger lines, but scrambled PGE3 labels. Investigation found
our `file(CONFIGURE ... CONTENT "${pge_header}" @ONLY)` expanded literal `@...@`
sequences inside the packed font string, shortening it from 1024 to 917 bytes.
The font decoder expects 1024 bytes, so this also caused out-of-bounds reads.

Header generation now stages the patched text and uses `configure_file(COPYONLY)`
to preserve it literally without rewriting unchanged final output. New regression
test `tests/pge3_font_test.cmake` failed on the corrupted header and passed after
regeneration. The font payload is now identical to upstream (1024 bytes); the
complete header differs only in the two intentional platform fixes.

Executed after the fix:

- Full Emscripten rebuild of both `notation-check` and `trombone-trainer`: **PASS**.
- Native PGE3 diagnostic compile/link: **PASS**.
- CTest in `build-bootstrap-native`: **2/2 PASS** (core plus font regression).
- Browser-launch retry with crashpad disabled: **BLOCKED**, Chrome reports
  `sandbox_host_linux.cc:41 ... shutdown: Operation not permitted` before loading
  a page. Corrected text has not been visually verified in a browser here.

## Update: FreeType compatibility fix

With Lomse and the Emscripten FreeType port now cached, reproduced the reported
web compile failure in `lomse_font_freetype.cpp`: the SDK's `FT_Outline::tags` is
`unsigned char*`, while Lomse 0.30.0 declared its local pointer as `char*`.
`cmake/Lomse.cmake` now patches that local declaration to
`decltype(outline.tags) tags;` in the build-local source copy. The downloaded
checkout stays unchanged; older native FreeType with `char*` remains supported.

Executed after the fix:

- Full `emcmake cmake -S . -B build-web` and `cmake --build build-web -j2`: **PASS**.
  Both `notation-check` and `trombone-trainer` generated `.html`, `.js`, `.wasm`,
  and `.data` files. This supersedes the earlier download-blocked web result.
- Native GCC C++11 syntax check of the real patched FreeType translation unit,
  using `/usr/include/freetype2`: **PASS**. This is not a full native build.
- `ctest --test-dir build-tests --output-on-failure`: **PASS**, one test executable.

The SDK cache is still read-only to this task, so the web build used
`EM_CACHE=/tmp/trombone-em-cache`, refreshed from the installed SDK cache.
Browser rendering, font use at runtime, and touch interaction remain unverified.

The sections below preserve the earlier validation history; their dependency
download blocker no longer applies to the completed web build.

## Environment blocker (identified before gameplay implementation)
Shell downloads cannot reach GitHub in this restricted environment. `git ls-remote https://github.com/OneLoneCoder/olcPixelGameEngine3.git HEAD` failed with `Could not resolve host: github.com`. Lomse and Bravura are not present in local source/font caches. The web reader can inspect upstream text but cannot populate a CMake dependency checkout or download binary fonts here. FreeType is available natively; the Emscripten port is not cached.

The required browser Lomse compatibility gate is therefore **blocked, not passed**. The application must never fall back to imitation notation. A PGE3-only bootstrap is a separate diagnostic executable, not evidence that Lomse works. Full native/web integration builds will be attempted and their actual results recorded below.

## Executed checks

Environment: Linux x86-64, GCC 13.4.0, CMake 3.25, Emscripten 6.0.10,
Node 24.21.0, installed Chrome 156.0.8078.79. No Lomse checkout or music fonts.

| Check | Actual result |
| --- | --- |
| Dependency-free native core, CMake Release + CTest | PASS, one test executable; 645 checks, 0 failures |
| Same core compiled with Emscripten and executed by Node | PASS, 645 checks, 0 failures; this is a Wasm logic test, not browser graphics |
| Core `-DNDEBUG -Wall -Wextra -Wpedantic -Werror` | PASS (agent executed) |
| Core ASan + UBSan | PASS with `ASAN_OPTIONS=detect_leaks=0` (agent executed); LeakSanitizer itself unavailable under this environment's ptrace restrictions |
| Native PGE3-only bootstrap | PASS compilation and link against local pinned PGE3 header |
| Native PGE3 bootstrap runtime | FAIL before initialization: exit 139; `xdpyinfo` independently reports `unable to open display :0`; gdb ptrace denied. No native render success claimed |
| Emscripten PGE3-only bootstrap | PASS after documented upstream compatibility fix and writable Emscripten cache; `.html`, `.js`, `.wasm` generated |
| Native trainer + notation-check full configuration | BLOCKED downloading Lomse: `Could not resolve host: github.com`; no complete native application link |
| Web trainer + notation-check full configuration | BLOCKED downloading Lomse with same error; no complete application `.data` or font package produced |
| UI + compatibility C++ source, native compiler syntax check | PASS against actual PGE3, Quiz, and Notation headers; excludes unavailable Lomse implementation |
| UI + compatibility C++ source, Emscripten syntax check | PASS against patched pinned PGE3 header with `-sUSE_LIBPNG=1 -fexceptions`; excludes unavailable Lomse implementation |
| Lomse CMake embedding patch | Agent executed synthetic build-version fixture test; PASS. This is a CMake-path test, not a Lomse build |
| Chrome/Playwright browser smoke | BLOCKED launching Chrome: `crashpad/util/linux/socket.cc:45 setsockopt: Operation not permitted (1)`; page never loaded; no screenshot |

## Commands used

Local checkout override (the shipped header is unchanged in that checkout):

```sh
cmake -S . -B build-bootstrap-native -G Ninja \
  -DTRAINER_BUILD_APP=OFF -DTRAINER_BUILD_BOOTSTRAP=ON -DBUILD_TESTING=OFF \
  -DFETCHCONTENT_SOURCE_DIR_PGE3_SOURCE=/home/aesquirell/Documents/code/personal/olcPixelGameEngine3
cmake --build build-bootstrap-native
```

Web used the analogous `emcmake cmake ... -B build-bootstrap-web` configuration.
Its first compile exposed an abstract browser host: missing `SetRelativeMouseMode`
override in pinned PGE3. CMake now generates a private header with a default
unsupported result on hosts without that implementation. X11 keeps its override.
A second private fix lets the browser canvas fill its parent in both dimensions.

The SDK cache is read-only here. The first web link failed writing a
`cache/symbol_lists/*.json.lock`. Copying the existing SDK cache into
`/tmp/trombone-em-cache`, then setting `EM_CACHE=/tmp/trombone-em-cache` for build,
resolved that environmental issue. No SDK files were modified.

```sh
cmake -S . -B build-tests -G Ninja -DTRAINER_BUILD_APP=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build-tests
ctest --test-dir build-tests --output-on-failure
./build-tests/core_tests
EM_CACHE=/tmp/trombone-em-cache em++ -std=c++20 -O2 -Isrc tests/core_tests.cpp -o /tmp/trombone-core-web.js
node /tmp/trombone-core-web.js
```

Full native/web configurations were attempted with the PGE3 local source override
and default `TRAINER_BUILD_APP=ON`. Both failed in Lomse FetchContent git clone,
before any Lomse compilation. Compiler syntax checks on Game/main/compatibility
cannot prove that `Notation.cpp` or Lomse links or renders correctly.

## Remaining visual and interaction validation

Not executed: native PGE3 initialization, real Lomse staff/clef/stem/notehead
rendering, font selection in Wasm, pitch/clef glyph placement, browser console
checks after page load, actual mouse/touch dragging, native/browser resize,
visual instrument recognition, and end-to-end gameplay. Core mathematical input
mapping, pitch invariance, scoring, alternates, sequence, and state transitions
were tested. Layout geometry was inspected/calculated at 740x360, 340x740, 375x500,
375x300, and 520x360;
this is not a visual or physical-device test.

The next required gate is to build both full targets in a network-enabled
environment and open `notation-check.html`, confirm real Bravura notation and
console output, then run the trainer and optional `tests/browser_smoke.cjs`.
