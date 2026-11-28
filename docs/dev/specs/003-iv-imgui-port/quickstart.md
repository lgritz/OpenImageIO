# Quickstart: build and validate the ImGui iv

How to build both backends and prove parity by hand. Options are defined in
[contracts/build-options.md](contracts/build-options.md).

## Prerequisites

- A normal OIIO development setup.
- OpenGL.
- ImGui backend: GLFW 3.3+ installed, or allow auto-build. On Linux see the
  package list in build-options.md.
- Qt backend (for comparison): Qt 5 or 6 as today.
- Test images: a multi-subimage EXR, a MIP-mapped `.tx`, an image with more
  than 4 channels, a partially written EXR, two same-size images for
  comparison, and any 4K image.

## Build both backends side by side

```bash
# ImGui backend, GLFW auto-built if not installed
cmake -B build-imgui -S . -DOIIO_IV_BACKEND=ImGui \
      -DOpenImageIO_BUILD_MISSING_DEPS=glfw3
cmake --build build-imgui --target iv

# Qt backend (reference)
cmake -B build-qt -S . -DOIIO_IV_BACKEND=Qt
cmake --build build-qt --target iv
```

Expected: configure prints the chosen iv backend. With `ImGui`, no Qt
package is searched for; with `Qt`, no GLFW.

## Checks for the build stories (US1, US3)

| # | Do | Expect |
|---|---|---|
| B1 | Configure `ImGui` on a machine with no Qt | iv builds |
| B2 | Configure `ImGui` with no GLFW and no auto-build | Warning names glfw3; iv skipped; rest builds |
| B3 | Auto-build GLFW on Linux without Wayland dev packages | Status line says X11-only; iv builds and runs |
| B4 | `-DOIIO_IV_EXTERNAL_IMGUI=ON` with a packaged imgui | Vendored copy not compiled |
| B5 | Time a clean `iv` target build with ImGui, minus the same build with `ENABLE_iv=OFF` | Under 60 s |
| B6 | Count lines: `cat src/iv/*.cpp src/iv/*.h src/iv/CMakeLists.txt \| wc -l` | At most 12,027 |
| B7 | `git diff main --stat -- src/iv/ivimage.cpp src/iv/ivutils.h` | Empty |

## Parity checklist (US2)

Run each item in `build-imgui/bin/iv -F` and compare with
`build-qt/bin/iv -F`. Repeat on Linux (X11 and Wayland), macOS and Windows.

**File**
- [ ] Open (multi-select, format filters), Open recent, Reload, Close Image
- [ ] Save As, Save Window As, Save Selection As
- [ ] Move to new window: second iv process opens with that image; image leaves the first
- [ ] Delete from disk asks for confirmation; No leaves the file
- [ ] Preferences opens; Exit writes settings; recent files and preferences survive restart
- [ ] Drag one file, then several files, from the file manager onto the window

**View**
- [ ] PgUp / PgDown / arrows, T toggle
- [ ] Channels c r g b a, `,` `.`; color modes 1 l h; RGBA / RGB from menu
- [ ] Exposure `[ ] { }`, gamma `( )`
- [ ] Subimage `<` `>`; MIP levels on a `.tx`
- [ ] Ctrl++ / Ctrl+- / + - =, Ctrl+0, f, Alt+f, Ctrl+f and Escape
- [ ] Wheel zoom at cursor, drag pan, Alt-drag variants
- [ ] Data and display window borders
- [ ] Menu check marks track state changed by shortcuts

**Compare**
- [ ] W wipe: drag the split, orientation switch, cursor shape
- [ ] Difference: Off, Absolute, FLIP, D cycles, FLIP colormap toggle
- [ ] `--compare wipe|absdiff|flipdiff`; bad value errors out
- [ ] Compare items disabled with fewer than two images

**Tools**
- [ ] Image info (Ctrl+I) shows metadata and follows image changes
- [ ] Pixel closeup (P): tracks mouse, text sharp and correctly placed, follow-mouse preference, closeup size and averaging preferences
- [ ] Area sample (Ctrl+A): select region, probe text
- [ ] Slide show: start, delay preference, loop, stop at end
- [ ] Sort: name, path, image date, file date, reverse
- [ ] Rotate left/right (Ctrl+Shift+L/R), flip horizontal/vertical

**OCIO** (when built with OCIO)
- [ ] Use OCIO toggle enables/disables the two submenus
- [ ] Image color space by family; Display/View
- [ ] `--image-color-space`, `--display`, `--view`

**Help**
- [ ] About; Image Formats (scrollable list)

**Status bar and window**
- [ ] Image info text, view info text, progress during a slow load
- [ ] Title shows the image name; "no image loaded" state
- [ ] UI responsive while a large image loads
- [ ] Idle iv uses about 0% CPU

**Command line** (contracts/cli.md)
- [ ] Each option; directory argument; missing file message

**Display**
- [ ] HiDPI / Retina: image 1:1 is pixel exact, text sharp
- [ ] Move the window between monitors with different scale factors
- [ ] Dark palette preference
- [ ] Typing in a preferences number field does not trigger shortcuts

**Robustness**
- [ ] Partially written EXR with default and `--missing-pixels strict`
- [ ] Linux without zenity/kdialog: File > Open shows a clear message

## Performance check (SC-005)

Open the 4K image in both builds. Pan, wheel-zoom and move the pixel closeup
quickly. The ImGui build must not feel slower. If in doubt, compare frame
times with a GL frame-time overlay or `-v` statistics.

## Done

All B checks pass, every parity item is ticked on all three platforms, and
any intentional difference is one already listed in spec FR-023 to FR-029.
