# Trombone Slide Position Trainer

A small C++20 trainer using **olcPixelGameEngine3** for every control and drawing,
and **Lomse** for genuine engraved notation. CMake builds the same C++ application
for desktop and Emscripten. No audio, browser UI overlay, or game framework.

**Current validation status:** both full native Linux and Emscripten applications
compile and link successfully, including packaged fonts. The core suite passes
1,381 checks natively and as WebAssembly under Node; native CTest also verifies
PGE3 font preservation. Graphical runtime remains unverified here; this sandbox
cannot access the X11 display or launch Chrome.
Genuine Lomse CPU engraving has been checked for every supported pitch in both
clefs (34 distinct, nonempty bitmaps), including B-flat 2 ledger lines.
The required browser rendering gate is **not yet passed**. See
[the validation record](docs/validation/STATUS.md) for exact commands and evidence.
There is no substitute notation renderer.

## Build

Prerequisites: CMake 3.24+, a C++20 compiler, Git, and network access for the first
dependency fetch. Ninja is optional. Both dependencies are fetched at fixed commits:

| Dependency | Revision |
| --- | --- |
| [PGE3](https://github.com/OneLoneCoder/olcPixelGameEngine3) | `ee9210acf196c22a9919105a57457f19dc3d4b5e` |
| [Lomse 0.30.0](https://github.com/lenmus/lomse/releases/tag/0.30.0) | `9f774d7a3a3904c89b7ebcc8fbada085a9f3d687` |

On Debian/Ubuntu, install `build-essential cmake git libfreetype6-dev libpng-dev
libx11-dev libxi-dev libgl1-mesa-dev`. Native builds use FreeType from the system;
PNG is needed by PGE3. Lomse fontconfig, threading, PNG import, and compressed-score
import are disabled because this application uses generated LDP scores only.

```sh
cmake -S . -B build-native -DCMAKE_BUILD_TYPE=Release
cmake --build build-native -j2
ctest --test-dir build-native --output-on-failure
./build-native/notation-check
./build-native/trombone-trainer
```

Run `notation-check` first. It must display a clef, staff, and three quarter notes.
Use its two canvas buttons to switch clefs and cycle the first pitch, including
sharp, flat, and ledger-line examples. Then run the trainer.

Windows requires a C++20 toolchain and native FreeType development installation
available to CMake (for example through an existing package-manager toolchain).
macOS requires Xcode command-line tools and FreeType; CMake selects Objective-C++
and the frameworks used by the upstream PGE3 examples. These platforms are
configured but have not been tested in this Linux environment. Native fonts are
staged in the build tree and addressed by their absolute build path; this first
version is run from that tree, not an installable relocatable desktop package.

## WebAssembly

Activate a recent [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html)
first. Emscripten **6.0.10** was available for validation here. Its ports need
network access on first build, and its cache must be writable.

```sh
emcmake cmake -S . -B build-web -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
cmake --build build-web -j2
python3 -m http.server 8000 --directory build-web
```

Open `http://localhost:8000/notation-check.html` to verify the integration, then
`http://localhost:8000/trombone-trainer.html`. Do not open the HTML with `file://`.
Each application produces `.html`, `.js`, `.wasm`, and a `.data` asset package.
Serve all of them from the same directory. A phone on the same network can use
the computer's LAN address in place of localhost.

The web link uses WebGL 2, Asyncify, a 4 MiB stack, growing memory, C++ exception
handling, and Emscripten's FreeType, PNG, and zlib ports. Native libraries are not
reused for WebAssembly. Lomse is static. The pinned Lomse repository includes
Bravura and Liberation fonts; CMake stages those fonts and their licenses and
preloads them into `/assets/fonts/`. The renderer checks required files before
initialization and sets Lomse's font path explicitly, including the trailing
slash. Missing fonts or rendering errors produce an error, never fake symbols.

The custom shell supplies only the canvas, loading status, and gesture handling.
All game controls and logic are C++. Canvas pointer capture retains drags outside
its bounds, touch gestures do not scroll the page, and the canvas receives focus.

Two small PGE3 workarounds are generated in a private build header without
modifying the dependency checkout: return unsupported for relative mouse mode on
hosts that omit the override, and resize the web canvas to its entire parent.
The app then resizes its logical screen and recomputes all geometry. Lomse's
embedded build-version paths are corrected automatically in a build-local copy;
its bitmap fit is calculated explicitly to avoid the release's width-for-height
zoom-fit bug. A further build-local patch derives FreeType's outline tag pointer
type with `decltype`, supporting both older `char*` and newer `unsigned char*`
headers. See the comments in `cmake/` and `src/Notation.cpp`.
Native builds also export the public FreeType dependency and supply a missing
standard `<limits>` include in the build-local Lomse copy for modern GCC.
The generated PGE3 header is copied without template expansion: its embedded
font data contains literal `@...@` sequences that must remain unchanged.

## Live version on GitHub Pages

The source repository and playable browser version can live together on GitHub.
The included [.github/workflows/pages.yml](.github/workflows/pages.yml) tests the
quiz, builds with pinned Emscripten 6.0.10, and deploys the static game whenever
`main` changes. Action versions are pinned to full commits.

To enable publishing:

1. Open the repository's **Settings → Pages**. Under **Build and deployment**,
   select **GitHub Actions** as the Source.
2. Commit and push the workflow and CMake staging changes to `main`.
3. Follow **Actions → Build and deploy GitHub Pages**. It can also be started
   manually with **Run workflow** on `main`.

After the first successful deployment, the expected live URL for this repository
is [ajesquirell.github.io/Trombone-Trainer](https://ajesquirell.github.io/Trombone-Trainer/).
Add that URL to the repository's **About → Website** field so visitors can find
both the code and game. This setup follows GitHub's
[custom Pages workflow instructions](https://docs.github.com/en/pages/getting-started-with-github-pages/using-custom-workflows-with-github-pages).

To create the same static site locally:

```sh
cmake --build build-web --target web-site -j2
python3 -m http.server 8000 --directory build-web/site
```

Open `http://localhost:8000/`. The site directory contains only `index.html` and
the game's `.js`, `.wasm`, and `.data` siblings; these relative URLs also work
under a project subdirectory. The generated files need not be committed. GitHub
Actions builds and uploads them. CI supplies `CMAKE_POLICY_VERSION_MINIMUM=3.5`
to accommodate Lomse 0.30.0's older minimum declaration on CMake 4 runners.

Local build/staging and YAML checks passed; a remote GitHub Actions deployment
has not been executed from this workspace.

## Play

Choose bass or treble clef (both concert pitch), then 5, 10, 20, or a custom
1–100 questions using minus/plus. Set the **Low** and **High** notes using their
minus/plus controls; both endpoints are included. The range pickers retain octave
numbers and show compact accidentals (for example, `Bb3` or `C#4`), while question
headings show only `B-flat` or `C-sharp`. Where space allows, Lomse previews the
two range limits on a staff. On short, narrow screens, switch between the
**Quiz** and **Range** tabs before starting; the **Notes** tab opens the exact-pitch picker.

The default range is **B-flat 2–B-flat 3**. Open **Notes** to enable or disable
exact pitches individually, including their octaves. Teal buttons are included;
gray pitches lie outside your current range. **All** enables the entire pool,
and **None** clears it; the range still limits which enabled pitches can appear.
Use Prev/Next on smaller screens to reach every pitch. Start is disabled until at
least one pitch in the range is enabled. Replay preserves your range and choices.

Start practice. Read the note, tap or drag the full-height seven-band column
(**7 at the top, 1 at the bottom**), and press Submit. A correct reference **or ordinary
alternate position** earns a point. Feedback stays visible until Next. Results
show score and accuracy; Play Again keeps settings and resets the exercise.
Clef buttons can also change the writing of the current pitch during practice.

The slide moves evenly between seven positions; fixed bell/tubing remain still,
and the bell faces right in the same direction as the slide. The illustration has
a curved, hollow bell, rear tuning bow, straight return neck, silver inner rails and rigid
outer slide; it is drawn in one batch of PGE3 primitives, with uniform scaling.
The slide is drawn in front of the bell where the two overlap.
The pool covers **17 chromatic pitches, B-flat 2 through D4**.
See [note and alternate-position sources](docs/validation/NOTE-SOURCES.md).
Consecutive questions do not repeat a pitch unless the range contains only one
note. Play Again preserves the selected range. Position resets to 1 each question.
Landscape is preferred; portrait retains controls and asks for a wider view.

## Tests and platform diagnostics

Core tests require no downloaded libraries or graphical display:

```sh
cmake -S . -B build-tests -DTRAINER_BUILD_APP=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build-tests
ctest --test-dir build-tests --output-on-failure
```

They check pitches/octaves, all seven reference/alternate positions, wrong
answers, single submission, feedback/results transitions, clamped question
counts, inclusive pitch bounds, exact-pitch toggles, empty selections, singleton
ranges, replay, octave-free display
names, clef invariance, random sequence constraints, and reversed selector edge
coordinates at different sizes. Checks stay active in Release builds.
Native builds that include PGE3 also run `pge3_font_payload`, which checks that
header generation preserves the complete upstream font data.

The instrument can also be inspected without a graphical display. The optional
preview target records the **same drawing code** used by PGE3; the Python tool
requires Pillow only for producing PNGs, not for building or running the game:

```sh
cmake --build build-native --target trombone-preview
python3 tools/preview_trombone.py build-native/trombone-preview
```

This creates PNG and SVG previews in `build-art-preview/` and checks their bounds.
See [the illustration preview](docs/validation/trombone-preview.png). These are
geometry previews, not browser screenshots.

The optional `pge3-bootstrap` is a **PGE3-only platform diagnostic**, not a Lomse
compatibility result. Build it by adding `-DTRAINER_BUILD_APP=OFF
-DTRAINER_BUILD_BOOTSTRAP=ON -DBUILD_TESTING=OFF` to native or web configuration.
It deliberately contains no notation.

For offline source checkouts at the pinned revisions, supply
`-DFETCHCONTENT_SOURCE_DIR_PGE3_SOURCE=/path/to/olcPixelGameEngine3` and
`-DFETCHCONTENT_SOURCE_DIR_LOMSE=/path/to/lomse`. The latter must include `fonts/`.
An offline web build also needs the FreeType/PNG/zlib Emscripten ports cached.

Optional browser automation uses Playwright installed separately:

```sh
npm install --no-save playwright
npx playwright install chromium
# With build-web served on port 8000 in another terminal:
node tests/browser_smoke.cjs http://localhost:8000
```

This checks startup, real render logging, visible pitch/clef updates, resize,
console errors, and captures screenshots in `build-browser-artifacts/`. It does
not replace visual inspection of clef/notehead/stem/ledger placement or a full
manual game/touch test. Set `PLAYWRIGHT_PATH` and/or `CHROME_PATH` to use existing
installations. No browser smoke test or screenshot succeeded in this sandbox.

## Files

- `src/main.cpp`, `Game.h`, `Game.cpp`: PGE3 UI, input, settings and flow.
- `src/Trombone.h`: batched instrument drawing shared by the game and preview tool.
- `src/Notes.h`, `Quiz.h`: explicit concert-pitch data and dependency-free logic.
- `src/Notation.h`, `Notation.cpp`: Lomse RGBA rendering, bitmap ownership and PGE3 upload.
- `src/compatibility.cpp`: separate clef/pitch integration check.
- `src/bootstrap.cpp`: PGE3-only diagnostic.
- `cmake/`: pinned dependencies, portability fixes and browser shell.
- `tests/`: lightweight core executable and optional browser smoke script.
- `tools/`: optional same-code instrument preview recorder and PNG/SVG generator.

Dependencies retain their upstream licenses: PGE3's OLC-3 license and Lomse's
MIT license. Font license files are copied alongside the fonts from Lomse's
repository. No third-party source or font binaries are vendored in this tree.
