---

description: "Task list for porting iv from Qt to Dear ImGui"
---

# Tasks: Port iv from Qt to Dear ImGui

**Input**: Design documents from `docs/dev/specs/003-iv-imgui-port/`

**Prerequisites**: plan.md, spec.md, research.md (R1 to R16), data-model.md,
contracts/ (ivui-seam.md, build-options.md, cli.md), quickstart.md

**Tests**: No automated test tasks. The spec asks for CI builds plus the
manual parity checklist in quickstart.md.

**Organization**: Tasks are grouped by user story. Phase 2 maps to PR steps
1 and 2 in plan.md, US1 to steps 3 and 4 (first half), US2 to steps 4
(second half) and 5.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: US1 build without Qt, US2 feature parity, US3 backend
  selection, US4 small maintainable code
- "seam" means contracts/ivui-seam.md. "Rn" means research.md item n.

## Suggested model per phase

- Fable: T001 (spike), T011 to T016 (seam introduction), T027 to T033
  (frame loop, events, GL integration).
- Opus: the rest of the code tasks.
- Sonnet: T017 to T021 (vendoring, GLFW recipe), T057 to T059 (CI, scripts,
  docs), T064, T065.

---

## Phase 1: Setup

**Purpose**: Remove the main technical risk before any real code depends on it.

- [X] T001 Build the throwaway spike described in R15 in a scratch directory outside the repo: GLFW window with default context hints, glad, ImGui 1.92.x with `imgui_impl_glfw` + `imgui_impl_opengl2`, one textured quad drawn with a GLSL 1.20 shader and fixed-function matrices, ImGui menu bar and background-draw-list text on top, correct on a Retina display. Record the outcome (OpenGL 2 backend confirmed, or switch to OpenGL 3 backend with `#version 120`) as a dated note under R2 in docs/dev/specs/003-iv-imgui-port/research.md
- [X] T002 [P] Record the baseline for the size budget: output of `cat src/iv/*.cpp src/iv/*.h src/iv/CMakeLists.txt | wc -l` on main, added as a note under R14 in docs/dev/specs/003-iv-imgui-port/research.md

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Make the Qt code ready for a second backend without changing
behavior. Two PRs: T003 to T010, then T011 to T016.

**CRITICAL**: No user story work starts until this phase is done and the Qt
build passes the quickstart.md parity checklist unchanged.

### PR step 1: Qt-only preparation

- [X] T003 Convert every `connect(..., SIGNAL(...), ..., SLOT(...))` in src/iv/imageviewer.cpp `createActions()` and `createOCIOMenus()` to pointer-to-member form valid in Qt 5 and 6. Use a lambda where the slot has default arguments (`zoomIn`, `zoomOut`, `fitWindowToImage`) so `triggered(bool)` is not passed as the first argument
- [X] T004 Remove `sender()` and `qobject_cast` uses in src/iv/imageviewer.cpp: `openRecentFile()` takes an index captured by its lambda; `ocioColorSpaceAction()` and `ocioDisplayViewAction()` take the names captured by their lambdas; update the declarations in src/iv/imageviewer.h
- [X] T005 [P] Convert the string-form `connect` calls in src/iv/ivpref.cpp and src/iv/ivinfowin.cpp to pointer-to-member form
- [X] T006 Replace `QString::arg` and `QString` concatenation in viewer logic with `Strutil::format` plus `.c_str()` at the setter: status messages in src/iv/imageviewer.cpp (lines near 1123, 1347, 1417, 1523) and `update_area_probe_text()` in src/iv/ivgl.cpp (near 1976)
- [X] T007 Remove Print: `printAct`, `print()`, the menu entry and the dead `QPrinter` code in src/iv/imageviewer.cpp and src/iv/imageviewer.h (R12)
- [X] T008 Remove `FORCE_OPENGL_1` and the code paths it selects from src/iv/CMakeLists.txt, src/iv/ivgl.cpp and any other reference found by `grep -rn FORCE_OPENGL_1` (R12)
- [X] T009 Replace the `((QOpenGLWidget*)(glwin))->update()` and `glwin->QWidget::update()` casts in src/iv/imageviewer.cpp with one spelling that will survive the rename to `IvGLWidget` (for example a `parent_t`-style typedef exposed by src/iv/ivgl.h)
- [X] T010 Build the Qt iv and run the parity checklist in docs/dev/specs/003-iv-imgui-port/quickstart.md on it; fix any behavior change introduced by T003 to T009 in src/iv/imageviewer.cpp

### PR step 2: introduce the seam, still Qt-only

- [X] T011 Create src/iv/ivui.h with the Qt-build half only: one alias per row of the seam Types table (`using IvAction = QAction;` ...), `namespace Iv = Qt;`, the small `IvGLWidget` struct deriving `QOpenGLWidget` and `QOpenGLExtraFunctions` with `viewport_x0()`/`viewport_y0()` returning 0, `IV_SPIN_VALUE_CHANGED`, `iv_str()`, and declarations of the seven `iv_*` helper functions
- [X] T012 Create src/iv/ivui_qt.cpp implementing the seven `iv_*` helpers by moving the existing Qt code out of src/iv/imageviewer.cpp (`QFileDialog`, `QMessageBox`, the formats `QDialog`, `QMetaObject::invokeMethod`, `availableGeometry` with its Qt 5 branch)
- [X] T013 Rename Qt types to their `Iv*` names and `Qt::` to `Iv::` throughout src/iv/imageviewer.h and src/iv/imageviewer.cpp, call the `iv_*` helpers at the sites emptied by T012, make `ImageViewer` derive from `IvMainWindow`, and replace Qt includes with `#include "ivui.h"`
- [X] T014 Rename Qt types to their `Iv*` names and `Qt::` to `Iv::` in src/iv/ivgl.h, src/iv/ivgl.cpp, src/iv/ivgl_ocio.h and src/iv/ivgl_ocio.cpp; make `IvGL` derive from `IvGLWidget`; route the two viewport setups in src/iv/ivgl.cpp (`resizeGL` and near line 1713) through `viewport_x0()`/`viewport_y0()` (R10)
- [X] T015 [P] Rename `QApplication` and `QTimer` uses in src/iv/ivmain.cpp to `IvApplication` and `IvTimer`
- [X] T016 Add a shared `ImageViewer::add_dropped_files(const std::vector<std::string>&)` in src/iv/imageviewer.cpp and make the Qt `dropEvent` call it (data-model.md, Events, file drop)

**Checkpoint**: Qt iv builds from the renamed sources and still passes the
parity checklist. Shared files contain no Qt class names outside `#if`
blocks and Qt-only files.

---

## Phase 3: User Story 1 - Build iv without Qt (Priority: P1) MVP

**Goal**: With no Qt installed, OIIO configures, builds an ImGui iv, and it
opens a window showing an image that can be panned and zoomed.

**Independent Test**: quickstart.md checks B1 to B5.

### Dependencies and build system (PR step 3)

- [X] T017 [P] [US1] Vendor Dear ImGui (files listed in R5, version from T001) into src/iv/ext/imgui/ with `LICENSE.txt` and a `README.md` stating the upstream tag and the update procedure
- [X] T018 [P] [US1] Vendor tinyfiledialogs (`tinyfiledialogs.c`, `tinyfiledialogs.h`, license text, `README.md` with version) into src/iv/ext/tinyfiledialogs/
- [X] T019 [P] [US1] Generate and vendor the glad 2 header for OpenGL 2.1 compatibility plus the extensions `check_gl_extensions()` tests for, into src/iv/ext/glad/ with a `README.md` giving the generator settings (R3)
- [X] T020 [P] [US1] Add `src/iv/ext/*` to .clang-format-ignore and add Dear ImGui, tinyfiledialogs and glad license entries to THIRD-PARTY.md
- [X] T021 [P] [US1] Create src/cmake/build_glfw3.cmake following src/cmake/build_Robinmap.cmake: version 3.4 pinned to a commit, static, examples/tests/docs off, and `GLFW_BUILD_WAYLAND=OFF` with a status message when `wayland-scanner`, `wayland-client` or `xkbcommon` is missing (R4, FR-004a)
- [X] T022 [US1] Add `OIIO_IV_BACKEND` (`Qt` default, `ImGui`; other values are an error) and `OIIO_IV_EXTERNAL_IMGUI` in src/cmake/externalpackages.cmake: search Qt only for `Qt`, `glfw3` 3.3+ only for `ImGui`, per contracts/build-options.md
- [X] T023 [US1] Rework src/iv/CMakeLists.txt: explicit `SRC` list per backend, `CMAKE_AUTOMOC` only for Qt, `IV_USE_QT` or `IV_USE_IMGUI` definition, vendored sources and include paths (or `imgui::imgui` when external), link `glfw` and `OpenGL`, configure message `iv backend: ...`, and the skip-with-warning outcomes in contracts/build-options.md
- [X] T024 [US1] Add a CMake step in src/iv/CMakeLists.txt that turns src/fonts/Droid_Sans/DroidSans.ttf and the Droid Sans Mono regular font into a generated `.cpp` in the build directory exposing two byte arrays (R9)

### Minimal ImGui backend (PR step 4, first half)

- [X] T025 [US1] Add the ImGui-build half of src/iv/ivui.h: include `ivui_imgui.h`, define `Q_OBJECT` and `slots` empty, and provide `iv_str()` and `IV_SPIN_VALUE_CHANGED` for look-alikes
- [X] T026 [US1] Create src/iv/ivui_imgui.h declaring every look-alike type in the seam Types table and `namespace Iv` enums, with exactly the listed members and the state fields from data-model.md (UI objects table). Value types and trivial state setters/getters are inline
- [X] T027 [US1] In src/iv/ivui_imgui.cpp implement `IvApplication` and `IvMainWindow` startup and shutdown: GLFW init, window with default context hints, `gladLoadGL`, ImGui context, GLFW and OpenGL 2 backends, fonts from the T024 arrays scaled by device pixel ratio (R2, R3, R9)
- [X] T028 [US1] In src/iv/ivui_imgui.cpp implement the frame loop per plan.md "ImGui frame" and R11: event-driven wait, redraw-needed flag, two settle frames, action queue run after the frame, `processEvents()` that draws without running queued actions, `iv_post` queue with `glfwPostEmptyEvent`
- [X] T029 [US1] In src/iv/ivui_imgui.cpp implement `IvGLWidget`: logical `width()`/`height()` of the image area, `viewport_x0()`/`viewport_y0()`, `devicePixelRatio()` from framebuffer ratio or content scale, `update()`, no-op `makeCurrent()`/`doneCurrent()`, and the calls to `initializeGL`, `resizeGL`, `paintGL` in the frame (R10)
- [X] T030 [US1] In src/iv/ivui_imgui.cpp translate mouse buttons, motion and wheel from `ImGuiIO` into `IvMouseEvent`/`IvWheelEvent` with image-area-relative logical positions and deliver them to the `IvGL` handlers when ImGui does not want the mouse (data-model.md, Events)
- [X] T031 [US1] Add the `#if IV_USE_QT` / `#else` branches in src/iv/ivgl.cpp for the `IvGL` constructor and `check_gl_extensions()` (glad version and extension flags), and make the frame code unbind shader program, PBO and texture unit before ImGui draws (R2, R3)
- [X] T032 [US1] Add the ImGui branches for `shadowed_text()` (background draw list `AddText` with the mono font) and the `draw_corners` lambda (`AddLine`) in src/iv/ivgl.cpp (R9)
- [X] T033 [US1] Make src/iv/imageviewer.cpp and src/iv/ivmain.cpp compile and link for ImGui: `#if` the palette and drag-drop setup in the `ImageViewer` constructor, `dropEvent`/`dragEnterEvent`, and info/preferences window creation, using only the `#if` sites allowed by the seam contract
- [X] T034 [US1] Run quickstart.md checks B1, B2, B3 and B5 on the development machine; fix failures in src/iv/CMakeLists.txt or src/cmake/build_glfw3.cmake Done on macOS: B1 (ImGui compile lines carry no Qt paths), B2 (warning now names GLFW), GLFW auto-build recipe (static, links and runs), B5 (iv objects build in about 19 CPU-seconds). B3 needs Linux; covered by T066.

**Checkpoint**: `-DOIIO_IV_BACKEND=ImGui` builds without Qt; `iv -F image.exr`
shows the image; wheel zoom and drag pan work; menus may still be empty.

---

## Phase 4: User Story 2 - Feature-parity viewing (Priority: P1)

**Goal**: Every feature of the Qt iv works in the ImGui iv (FR-010 to
FR-022), with the intentional differences in FR-023 to FR-029.

**Independent Test**: The parity checklist in quickstart.md, compared
against the Qt build.

### Actions, menus, shortcuts

- [X] T035 [US2] Implement `IvAction`, `IvActionGroup` and `IvSignalBlocker` behavior in src/iv/ivui_imgui.cpp per data-model.md "State rules carried over from Qt": `trigger()`, checkable flip, `toggled` emission, blocked signals, exclusive groups, `&` stripped from labels, and the `connect` forms listed in the seam contract
- [X] T036 [US2] Implement `IvMenu` and `IvMenuBar` drawing in src/iv/ivui_imgui.cpp: walk the menu tree each frame with `BeginMenu`/`MenuItem`, show shortcut text and check marks, honor enabled and visible flags, queue the action when clicked (FR-028)
- [X] T037 [US2] Implement shortcut parsing and dispatch in src/iv/ivui_imgui.cpp per R8: Qt-style strings to key chords for letters, digits and named keys; typed-character matching for punctuation; no dispatch while `io.WantTextInput`; Ctrl meaning Cmd on macOS
- [X] T038 [US2] Deliver `IvKeyEvent` to `ImageViewer::keyPressEvent` from src/iv/ivui_imgui.cpp for the keys in `namespace Iv` (arrows, PageUp/PageDown, Escape, `+ - = _`)

### Window, status bar, timers

- [X] T039 [US2] Implement the remaining `IvMainWindow` members in src/iv/ivui_imgui.cpp: `setWindowTitle`, `resize`, `move`, `raise`, `activateWindow`, `close` (calls `closeEvent`), `showFullScreen`/`showNormal` via `glfwSetWindowMonitor`, `resizeEvent` delivery, and `iv_available_screen` via `glfwGetMonitorWorkarea`
- [X] T040 [P] [US2] Implement `IvStatusBar` drawing with `IvLabel`, `IvProgressBar` and `IvComboBox` children in src/iv/ivui_imgui.cpp, with `height()` reported for `fitWindowToImage()` (FR-019)
- [X] T041 [P] [US2] Implement `IvTimer` (`start`, `stop`, `isActive`, `timeout`, `singleShot`) in src/iv/ivui_imgui.cpp, checked by the frame loop and used as its wait deadline (R11)
- [X] T042 [US2] Implement cursor shapes in src/iv/ivui_imgui.cpp: `setCursor`/`unsetCursor` on `IvGLWidget` and `IvMainWindow`, `IvApplication` override cursor, mapped to ImGui mouse cursors each frame (R12)
- [X] T043 [US2] Wire the GLFW drop callback in src/iv/ivui_imgui.cpp to `ImageViewer::add_dropped_files()` (FR-021)

### Dialogs, settings, panels

- [X] T044 [US2] Implement `iv_open_files`, `iv_save_file`, `iv_message` and `iv_confirm` with tinyfiledialogs in src/iv/ivui_imgui.cpp, including splitting the Qt filter string into pattern arrays and the fallback message panel when no Linux dialog helper exists (R6) Done; no-helper path is untested (needs a Linux box without zenity/kdialog).
- [X] T045 [US2] Implement `IvSettings` in src/iv/ivui_imgui.cpp: file location and format per R7; keys, types and defaults per data-model.md "Settings file" (`pixelviewFollowsMouse`, `linearInterpolation`, `darkPalette`, `autoMipmap`, `maxMemoryIC`, `slideShowDuration`, `closeupPixels`, `RecentFiles`); ImGui layout file `iv_layout.ini` beside it (FR-024)
- [X] T046 [P] [US2] Implement the preferences panel in src/iv/ivui_imgui.cpp as an ImGui window drawing the viewer's `IvCheckBox` and `IvSpinBox` objects in the order src/iv/ivpref.cpp lays them out, with `valueChanged` emission, tooltips and suffixes (FR-020, FR-025)
- [X] T047 [P] [US2] Implement the image info panel in src/iv/ivui_imgui.cpp as an ImGui window with scrolling text from `IvImage::longinfo()`, matching the `IvInfoWindow` interface (`update(img)`, `show`, `hide`) used by src/iv/imageviewer.cpp (FR-013, FR-025)
- [X] T048 [P] [US2] Implement `iv_text_dialog` (Image Formats) as a scrolling ImGui panel in src/iv/ivui_imgui.cpp, and add the plain-text `#else` branch of `about()` in src/iv/imageviewer.cpp (FR-015)
- [X] T049 [US2] Apply light or dark ImGui style from the `darkPalette` preference at startup in src/iv/ivui_imgui.cpp (FR-026; data-model.md Settings file `darkPalette`)

### Remaining behavior

- [X] T050 [US2] Add the ImGui branch of `moveToNewWindow()` in src/iv/imageviewer.cpp: start `Sysutil::this_program_path()` with `-F`, the image path and the OCIO options, then close the image locally (FR-027, contracts/cli.md)
- [ ] T051 [US2] Verify OCIO menus (family submenus, exclusive groups, enable/disable with Use OCIO) work through `IvMenu`/`IvActionGroup`; fix gaps in src/iv/ivui_imgui.cpp (FR-014)
- [ ] T052 [US2] Verify load progress and smooth zoom repaint through `IvApplication::processEvents()` and that the FLIP result arrives through `iv_post`; fix gaps in src/iv/ivui_imgui.cpp (R11, FR-012, FR-019)
- [ ] T053 [US2] Verify pixel closeup, area probe and window-guide overlays line up with the image on standard and HiDPI displays; fix offsets in the ImGui branches of src/iv/ivgl.cpp or in `IvGLWidget` in src/iv/ivui_imgui.cpp (FR-018, R10)
- [ ] T054 [US2] Run the full parity checklist in docs/dev/specs/003-iv-imgui-port/quickstart.md on the development platform against the Qt build; fix each failing item in src/iv/ivui_imgui.cpp

**Checkpoint**: ImGui iv passes the parity checklist on one platform.

---

## Phase 5: User Story 3 - Build-time backend selection (Priority: P2)

**Goal**: One option switches backends; each build is free of the other's
code; CI covers both.

**Independent Test**: Build both backends from one tree (quickstart.md
"Build both backends side by side"); both run.

- [X] T055 [US3] Confirm isolation and fix leaks in src/iv/CMakeLists.txt and src/cmake/externalpackages.cmake: the Qt build compiles nothing under src/iv/ext and does not search for glfw3; the ImGui build includes no Qt header and does not search for Qt (inspect configure output and `compile_commands.json`)
- [X] T056 [US3] Implement and test `OIIO_IV_EXTERNAL_IMGUI=ON` in src/iv/CMakeLists.txt against a packaged imgui (quickstart.md check B4) Tested against a stand-in package built from the vendored sources; a package with no version makes checked_find_package print a harmless "version was empty" line.
- [X] T057 [P] [US3] Add ImGui-backend jobs on Linux, macOS and Windows in .github/workflows/ci.yml, at least one with GLFW auto-built, keeping the existing Qt jobs (FR-005)
- [X] T058 [P] [US3] Add the X11 and Wayland development packages and optional GLFW install to src/build-scripts/gh-installdeps.bash and src/build-scripts/install_homebrew_deps.bash, controlled the same way `QT_VERSION`/`INSTALL_QT` are today Not needed: CI passes the Linux packages through `EXTRA_DEP_PACKAGES` and GLFW is auto-built (`OpenImageIO_BUILD_MISSING_DEPS=all` in ci-startup.bash).
- [X] T059 [P] [US3] Document `OIIO_IV_BACKEND`, `OIIO_IV_EXTERNAL_IMGUI`, the GLFW dependency, platform packages and the Linux dialog-helper runtime note in INSTALL.md, from contracts/build-options.md

**Checkpoint**: CI is green for both backends on three platforms.

---

## Phase 6: User Story 4 - Small, reviewable, maintainable code (Priority: P2)

**Goal**: The size budget and the "obvious diff" property hold and are
checked, not assumed.

**Independent Test**: quickstart.md checks B6 and B7.

- [X] T060 [US4] Count non-vendored lines under src/iv (quickstart.md check B6). If over 12,027 (1.5x; was 10,020), trim src/iv/ivui_imgui.cpp and src/iv/ivui_imgui.h first; record the final count under R14 in docs/dev/specs/003-iv-imgui-port/research.md
- [X] T061 [US4] Audit `#if IV_USE_QT` / `IV_USE_IMGUI` occurrences in src/iv/imageviewer.cpp, src/iv/imageviewer.h, src/iv/ivgl.cpp, src/iv/ivgl.h and src/iv/ivmain.cpp against the allowed list in the seam contract; move anything else behind a look-alike member or helper
- [X] T062 [US4] Audit src/iv/ivui_imgui.h against the seam Types table: remove members shared code does not call, and add to docs/dev/specs/003-iv-imgui-port/contracts/ivui-seam.md any member that had to be added
- [X] T063 [P] [US4] Confirm `git diff main --stat -- src/iv/ivimage.cpp src/iv/ivutils.h` is empty (quickstart.md check B7) and that `git diff main -- src/iv/ivgl_ocio.cpp` shows only renames; revert anything else in those files

**Checkpoint**: Budget met with the count recorded; every `#if` is at an
allowed site.

---

## Phase 7: Polish and Cross-Cutting

- [X] T064 [P] Run clang-format on the touched and new files under src/iv (not the whole tree, and not src/iv/ext)
- [X] T065 [P] Add standard copyright and SPDX headers to src/iv/ivui.h, src/iv/ivui_qt.cpp, src/iv/ivui_imgui.h, src/iv/ivui_imgui.cpp and src/cmake/build_glfw3.cmake
- [ ] T066 Run the parity checklist in docs/dev/specs/003-iv-imgui-port/quickstart.md on the remaining platforms (Linux X11, Linux Wayland, macOS, Windows, whichever were not covered by T054), including the mixed-DPI monitor item; fix platform issues in src/iv/ivui_imgui.cpp
- [ ] T067 Run the performance check in quickstart.md (SC-005) and the idle-CPU item; fix regressions in the frame loop in src/iv/ivui_imgui.cpp Partly done: idle CPU measured at zero over 20 s on macOS; the timing comparison is still open.
- [ ] T068 Re-run the Qt build and its parity checklist once more after all shared-file edits; fix regressions in src/iv/imageviewer.cpp or src/iv/ivgl.cpp
- [ ] T069 Update the Verification and Assumptions sections of docs/dev/specs/003-iv-imgui-port/spec.md with anything that turned out differently (backend choice from T001, final line count)

---

## Dependencies and Execution Order

### Phase dependencies

- Phase 1: no dependencies. T001 decides the renderer backend used by T017,
  T027 and T031.
- Phase 2: T003 to T010 (PR 1) before T011 to T016 (PR 2). Blocks all
  stories.
- US1 (Phase 3): needs Phase 2. T017 to T024 (PR 3) can proceed in parallel
  with Phase 2 because they touch no iv source; T025 onward needs PR 2
  merged.
- US2 (Phase 4): needs US1 (the look-alike header and frame loop).
- US3 (Phase 5): T055, T056 need US1; T057 to T059 can start once US1
  builds.
- US4 (Phase 6): needs US2 complete, since the count and audits are final
  measurements. Run B6 informally at the end of every PR as well.
- Phase 7: after all stories.

### Within stories

- US1: T022 before T023. T026 before T027 to T030. T027 before T028 before
  T029 before T030. T031, T032 after T029. T033 after T026. T034 last.
- US2: T035 before T036, T037, T051. T039 before T040. T045 before T046 and
  T049. T054 last.

### Parallel opportunities

- T017, T018, T019, T020, T021 (separate directories and files).
- T005 and T015 alongside their neighbors (separate files).
- T040, T041 after T039; T046, T047, T048 after T045 (same file,
  `ivui_imgui.cpp`, but separate self-contained sections; run in parallel
  only with care or split the work by function).
- T057, T058, T059 (CI, scripts, docs).

---

## Parallel Example: User Story 1 dependencies

```text
Task: "Vendor Dear ImGui into src/iv/ext/imgui/"            (T017)
Task: "Vendor tinyfiledialogs into src/iv/ext/tinyfiledialogs/" (T018)
Task: "Generate and vendor glad into src/iv/ext/glad/"      (T019)
Task: "Create src/cmake/build_glfw3.cmake"                  (T021)
```

---

## Implementation Strategy

### MVP (User Story 1)

1. T001 spike.
2. Phase 2 as two Qt-only PRs. Stop and re-validate the Qt build.
3. US1: vendoring PR, then the minimal backend. Stop when an image shows in
   a Qt-free build. This proves the seam and the rendering path.

### Incremental delivery

1. US2 fills in behavior until the checklist passes on one platform.
2. US3 adds CI and docs so both backends stay healthy.
3. US4 measures and trims.
4. Polish covers the other platforms.

### After this feature (not tasks here)

- Switch the default backend to ImGui.
- Remove the Qt backend and do the tidy-up in R14.
- Headless scripted tests (R16).
