# Implementation Plan: Port iv from Qt to Dear ImGui

**Branch**: `lg-ivimgui` | **Date**: 2026-11-28 | **Spec**: [spec.md](spec.md)

**Input**: Feature specification from `docs/dev/specs/003-iv-imgui-port/spec.md`

## Summary

Replace Qt in `iv` with Dear ImGui + GLFW, selectable at build time while
both exist. The existing viewer code is kept. A single header, `ivui.h`,
gives backend-neutral `Iv*` names: aliases of Qt classes in the Qt build,
small look-alike classes in the ImGui build. Shared code changes by type
rename only. The ImGui side adds one implementation file that runs the frame
loop, draws menus/status/panels from the look-alike objects, and turns
GLFW/ImGui input into look-alike events for the existing handlers. iv's
OpenGL rendering code is unchanged.

## Technical Context

**Language/Version**: C++17

**Primary Dependencies**: Dear ImGui 1.92.x (vendored: core, GLFW backend,
OpenGL 2 backend), tinyfiledialogs (vendored), glad-generated OpenGL 2.1
loader (vendored), GLFW 3.3+ (found or auto-built at 3.4), OpenGL, optional
OpenColorIO. Qt 5/6 remains for the Qt backend.

**Storage**: Per-user text settings file (`OpenImageIO/iv.ini` in the
platform config directory). See data-model.md.

**Testing**: CI build of both backends on Linux, macOS, Windows. Manual
parity checklist (quickstart.md). No automated UI tests in this feature.

**Target Platform**: Linux (X11 and Wayland), macOS, Windows desktops with a
compatibility-profile OpenGL context (2.1 or newer).

**Project Type**: Desktop application inside a C++ library repository.

**Performance Goals**: Pan, zoom and closeup tracking no slower than the Qt
build. Zero CPU use when idle (event-driven loop).

**Constraints**:
- Non-vendored `src/iv` at most 12,027 lines while both backends exist
  (1.5 x 8018; raised from 1.25x after implementation).
- Shared-file diffs limited to type renames, helper-function calls and
  `#if` blocks around Qt-specific bodies.
- ImGui UI extra build time under 60 seconds.

**Scale/Scope**: About 90 actions, 14 menus/submenus, 5 secondary panels,
8 preferences, 10 command-line options. About 1400 new lines.

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

`.specify/memory/constitution.md` is still the unfilled template, so it
defines no gates. The project rules in `AGENTS.md` are used instead:

| Rule (AGENTS.md) | Status |
|---|---|
| Keep PRs narrow and easy to review | Pass: five-step PR sequence below |
| Avoid new third-party deps without strong reason | Pass: replaces Qt with far smaller deps; user approved each |
| Preserve API/ABI/behavior unless task requires a break | Pass: library untouched; iv differences listed in spec FR-023 to FR-029 |
| CLI changes update tests and help text | Pass: CLI unchanged (contracts/cli.md) |
| Build/dependency changes check CMake, CI, optional features | Pass: R13, contracts/build-options.md |
| clang-format, ASCII, SPDX headers on new files | Pass: vendored code excluded via `.clang-format-ignore` |
| Use `Strutil::format`, no `printf`/streams in new code | Pass |

Post-design re-check: no change. No violations to justify.

## Project Structure

### Documentation (this feature)

```text
docs/dev/specs/003-iv-imgui-port/     (top-level specs/ is a symlink to docs/dev/specs)
├── spec.md
├── plan.md              # this file
├── research.md          # decisions R1 to R16
├── data-model.md        # look-alike types, settings keys, event flow
├── quickstart.md        # build both backends, manual parity checklist
├── contracts/
│   ├── ivui-seam.md     # exact Qt subset each Iv* type must provide
│   ├── build-options.md # CMake options and dependency behavior
│   └── cli.md           # command-line contract (unchanged)
└── tasks.md             # created by /speckit-tasks
```

### Source Code (repository root)

```text
src/iv/
├── CMakeLists.txt        # backend option, per-backend source lists
├── ivui.h                # NEW  the seam: Qt aliases or look-alike include,
│                         #      helper function declarations
├── ivui_qt.cpp           # NEW  Qt helpers (dialogs, post, screen); Qt only
├── ivui_imgui.h          # NEW  look-alike class declarations; ImGui only
├── ivui_imgui.cpp        # NEW  frame loop, menus, shortcuts, events,
│                         #      dialogs, settings, panels; ImGui only
├── imageviewer.h/.cpp    # shared: type renames, helper calls, few #if
├── ivgl.h/.cpp           # shared: type renames, #if for text, GL checks
├── ivgl_ocio.h/.cpp      # shared: constructor signature rename only
├── ivmain.cpp            # shared: type renames only
├── ivimage.cpp, ivutils.h  # untouched
├── ivinfowin.cpp         # Qt only (ImGui panel lives in ivui_imgui.cpp)
├── ivpref.cpp            # Qt only (ImGui panel lives in ivui_imgui.cpp)
└── ext/                  # NEW  vendored, excluded from line budget
    ├── imgui/            #      core + backends/glfw + backends/opengl2
    ├── tinyfiledialogs/
    └── glad/

src/cmake/
├── externalpackages.cmake  # find Qt or glfw3 depending on backend
└── build_glfw3.cmake       # NEW  auto-build recipe

.github/workflows/ci.yml, src/build-scripts/*   # ImGui-backend jobs, deps
INSTALL.md, THIRD-PARTY.md, .clang-format-ignore
```

**Structure Decision**: Everything stays in `src/iv`. Backend choice is
confined to `ivui.h`, two backend implementation files, and `#if` blocks in
shared files where a function body is inherently Qt (QPainter text, GL
capability query, drag-and-drop event type).

## Design

Details and rationale are in research.md; type-by-type requirements are in
contracts/ivui-seam.md.

### The seam

- `ivui.h` in the Qt build: `using IvAction = QAction;` and similar for
  every Qt class named in shared code, plus `namespace Iv = Qt;`.
- `ivui.h` in the ImGui build: includes `ivui_imgui.h`, which defines
  classes with those names and the method subset iv calls.
- `ImageViewer` derives from `IvMainWindow`, `IvGL` from `IvGLWidget`.
  `Q_OBJECT` and `slots` are defined empty in the ImGui build.
- Procedural Qt calls go through seven `iv_*` helper functions (R6).

### ImGui frame (ivui_imgui.cpp)

1. Wait for events (or a timer deadline, or a posted function).
2. `NewFrame`. Draw the menu bar from the `IvMenu` tree, the status bar
   from its child widgets, and visible panels (info, preferences, formats,
   message). Clicked menu items and matched shortcuts queue their actions.
3. Translate mouse, wheel, key and drop input into look-alike events and
   call the `IvGL` / `ImageViewer` handlers, unless ImGui wants the input.
4. Clear, set the viewport to the image area, call `IvGL::paintGL()`.
   Overlay text goes to ImGui's background draw list.
5. Unbind shader/PBO, `Render`, draw ImGui data, swap.
6. After the frame: run queued actions, fired timers, posted functions.

### PR sequence

Each step builds and runs; steps 1 and 2 change no behavior.

1. **Qt-only preparation.** Convert `SIGNAL()/SLOT()` string connects to
   pointer-to-member or lambda form (works in Qt 5 and 6). Replace
   `sender()`/`qobject_cast` uses with captured values. Replace
   `QString::arg` formatting in viewer logic with `Strutil::format`. Remove
   Print and `FORCE_OPENGL_1`.
2. **Introduce the seam, still Qt-only.** Add `ivui.h` with the aliases and
   `ivui_qt.cpp` with the helpers. Rename types in shared code. Add
   `OIIO_IV_BACKEND` with only `Qt` implemented.
3. **Dependencies.** Vendor ImGui, tinyfiledialogs, glad under
   `src/iv/ext/`. Add `build_glfw3.cmake`, find logic, license entries,
   format-ignore entries. No iv code.
4. **ImGui backend.** `ivui_imgui.h/.cpp`, the `#if` branches in `ivgl.cpp`
   and `imageviewer.cpp`, CI jobs, `INSTALL.md`.
5. **Parity pass.** Work through quickstart.md on three platforms, fix gaps.

Later, separate from this feature: switch the default to ImGui, then remove
Qt and tidy up.

A throwaway spike (R15) comes before step 3 to confirm the OpenGL 2 backend,
glad and Retina handling on macOS.

## Risks

| Risk | Mitigation |
|---|---|
| OpenGL 2 ImGui backend conflicts with iv's shader/PBO state | Spike first (R15); unbind before ImGui draw; fallback is the OpenGL 3 backend with `#version 120` |
| Look-alike layer grows past its estimate and breaks the 1.5x budget | contracts/ivui-seam.md lists the exact subset; anything not listed is not implemented; count lines in each PR |
| "No larger than baseline after Qt removal" (SC-002) not reached by removal alone | Needs the tidy-up described in R14; flagged now so it is planned, not discovered |
| Punctuation shortcuts on non-US keyboards | Match typed characters, not key codes (R8) |
| HiDPI differences across macOS, Wayland, X11, Windows | One place (`IvGLWidget`) converts to Qt-style logical coordinates (R10); checklist covers mixed-DPI monitors |
| Step 1 changes Qt behavior by accident (slots with default arguments such as `zoomIn(bool smooth = true)`) | Use lambdas for those slots; run the checklist on the Qt build after step 1 |
| tinyfiledialogs has no helper program on a minimal Linux desktop | Detect failure and show which tools to install (spec edge case) |
| Nested ImGui frames from progress callbacks | Actions run between frames; `processEvents()` draws a frame without running actions (R11) |

## Complexity Tracking

No constitution violations. Nothing to justify.
