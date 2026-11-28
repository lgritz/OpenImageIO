# Feature Specification: Port iv from Qt to Dear ImGui

**Feature Branch**: `lg-ivimgui`

**Created**: 2026-11-28

**Status**: Draft

**Input**: User description: "Switch the `iv` image viewer from the very
heavyweight Qt dependency to Dear ImGui. Lighter dependency that is easy to
auto-build or vendor; keep all capabilities and cross-platform support; not
significantly more code than today; minimal, obvious diffs confined to the
Qt-touching parts, ideally with a build-time Qt/ImGui selection during the
transition; users should see a version-to-version change, not an overhaul."

## Context

This feature is a dependency migration, so naming the replacement technologies
is unavoidable. The spec still describes outcomes. Mechanism belongs in the
plan.

Baseline today: `src/iv` is 8018 lines in 12 files (including CMakeLists.txt).
Qt use is concentrated in `imageviewer.cpp`/`.h` (menus, actions, dialogs,
status bar, settings), `ivgl.cpp`/`.h` (GL widget, mouse/wheel events, text
overlay painting), `ivinfowin.cpp`, `ivpref.cpp`, and `ivmain.cpp`.
`ivimage.cpp`, `ivgl_ocio.cpp` (mostly) and `ivutils.h` are Qt-free.

Two failure modes to avoid:
- A ground-up rewrite. A previous ImGui port came out at more than 5x the
  current code size and was rejected as unmaintainable.
- Merging UI and viewer logic in a way that makes the two backends diverge.

## Clarifications

### Session 2026-11-28

- Q: How should "File > Move to new window" work in the ImGui build? → A:
  Launch a new `iv` process with that image and remove it from the current
  window; no shared state.
- Q: On macOS, system menu bar or in-window menus? → A: In-window ImGui menu
  bar on every platform, including macOS.
- Q: On Linux, X11, native Wayland, or both? → A: Both, selected at runtime;
  auto-build drops Wayland if its dev packages are missing (XWayland covers
  Wayland sessions then).
- Q: How is the action/menu/preferences code shared between the Qt and ImGui
  builds? → A: Look-alike types. Small `Iv*` classes with the same method
  names as the Qt subset iv uses; in the Qt build they are aliases of the Qt
  classes, so the existing code compiles for both with type renames only.
- Planning finding: iv's GL code needs a legacy (compatibility) OpenGL
  context, which on macOS means OpenGL 2.1. The vendored ImGui renderer
  backend is therefore the OpenGL 2 one, and a generated OpenGL function
  loader (glad) is vendored for Windows. See research.md R2, R3.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Build iv without Qt (Priority: P1)

A developer or packager builds OIIO, including `iv`, on Linux, macOS, or
Windows without Qt installed. The dependencies `iv` needs beyond OIIO's
existing ones are either vendored or fetched and built automatically by the
usual OIIO dependency auto-build mechanism, in well under a minute of extra
build time.

**Why this priority**: Removing the heavy Qt dependency is the reason for the
feature. Without it nothing else matters.

**Independent Test**: On a clean machine with no Qt, configure and build OIIO
with dependency auto-build enabled. `iv` is built and launches with an image.

**Acceptance Scenarios**:

1. **Given** a system with OpenGL and no Qt, **When** OIIO is configured with
   the ImGui backend selected, **Then** `iv` is built, and configure output
   reports which backend was chosen.
2. **Given** the windowing library is not installed, **When** configuring with
   dependency auto-build enabled, **Then** it is fetched and built
   automatically and `iv` links against it.
3. **Given** a packager who wants system copies of the third-party UI code,
   **When** they set the documented option, **Then** the build uses the
   external copy instead of the vendored one.
4. **Given** the ImGui backend's deps are unavailable and auto-build is off,
   **When** configuring, **Then** configure prints a clear warning that `iv`
   will not be built and the rest of OIIO builds normally, as Qt does today.

---

### User Story 2 - Feature-parity viewing (Priority: P1)

A user who knows today's `iv` opens images and uses every existing feature
(menus, keyboard shortcuts, mouse modes, pixel closeup, info window,
preferences, OCIO, comparison modes, slide show, sorting, recent files) and
finds that it all works and looks broadly familiar.

**Why this priority**: Parity is a hard requirement. A lighter build that
loses features is not acceptable.

**Independent Test**: Walk the parity checklist (FR-010 through FR-022) against
both backends with the same images and compare results.

**Acceptance Scenarios**:

1. **Given** `iv` launched with several images, **When** the user presses
   PgUp/PgDown, T, r/g/b/a/c/l/h/1, `[` `]` `{` `}` `(` `)`, `<` `>`, `,` `.`,
   f, Alt+f, Ctrl+0, Ctrl++/-, Ctrl+f, W, D, P, Ctrl+I, **Then** each does
   what it does in the Qt build.
2. **Given** an image is displayed, **When** the user zooms with the wheel,
   pans by dragging, selects a region, or uses wipe compare, **Then** the
   behavior matches the Qt build.
3. **Given** the pixel closeup view is enabled, **When** the mouse moves over
   the image, **Then** the closeup and its antialiased pixel-value text
   overlay are drawn and follow the mouse (or stay put, per preference).
4. **Given** a multi-subimage, multi-MIP, or many-channel file, **When** the
   user steps through subimages, MIP levels and channels, **Then** the status
   bar and display update as in the Qt build.
5. **Given** OCIO is available, **When** the user picks image color space,
   display and view from the menus or passes `--image-color-space`,
   `--display`, `--view`, **Then** the result matches the Qt build.

---

### User Story 3 - Build-time backend selection during transition (Priority: P2)

A maintainer can build `iv` with either Qt or ImGui by setting one CMake
option, so the two can be compared side by side and Qt stays available as a
fallback until the ImGui backend has proven itself.

**Why this priority**: It lowers risk and makes review easy. It is not
user-visible.

**Independent Test**: Build twice, once per backend, from the same source tree.
Both binaries run and pass the parity checklist.

**Acceptance Scenarios**:

1. **Given** the backend option set to Qt, **When** building, **Then** no ImGui
   or windowing-library code is compiled and the behavior is today's.
2. **Given** the backend option set to ImGui, **When** building, **Then** no Qt
   headers or libraries are needed.
3. **Given** a reviewer reading the diff, **When** looking at shared
   viewer-logic files, **Then** changes there are limited to `#if` seams or to
   moving Qt-specific code into backend-specific files.

---

### User Story 4 - Small, reviewable, maintainable code (Priority: P2)

A maintainer reviewing or later changing `iv` finds the ImGui UI code about the
same size and shape as the Qt UI code it replaces. Adding a menu item or a
preference is a change in roughly as many places as today.

**Why this priority**: This is the main reason the previous port was rejected.

**Independent Test**: Count lines in `src/iv` excluding vendored third-party
files. Adding one new menu action touches no more call sites than in the Qt
build.

**Acceptance Scenarios**:

1. **Given** the dual-backend tree, **When** counting non-vendored lines under
   `src/iv`, **Then** the total is at most 1.5x the baseline (about 12,027
   lines).
2. **Given** the Qt backend is later removed, **When** counting again, **Then**
   `src/iv` is no larger than the baseline.

---

### Edge Cases

- Image opened on the command line before the window and GL context exist.
- Very large images and slow loads: the progress indicator still updates, and
  the UI stays responsive while loading where it does today.
- Files dropped onto the window from the OS file manager (single or multiple).
- HiDPI / Retina displays and windows moved between monitors with different
  scale factors: image pixels, closeup and text stay correctly sized and sharp.
- Window resize, full screen toggle and return, fit window to image when the
  image is larger than the screen.
- Keyboard shortcuts while a text field (spin box) has focus must not trigger
  viewer actions.
- "Delete from disk" confirmation and other confirmations and errors: modal
  prompts still require explicit confirmation.
- No usable native file dialog helper on Linux (e.g. zenity/kdialog missing):
  the user gets a clear message rather than a silent no-op.
- OCIO absent at build time: OCIO menus are hidden, as today.
- macOS: Cmd versus Ctrl shortcut mapping (Cmd+Q quits, Cmd+, opens
  Preferences) works even though menus are in-window (FR-028).
- Partially written or missing-pixel EXR files keep today's tolerant behavior.

## Requirements *(mandatory)*

### Functional Requirements

**Build and dependencies**

- **FR-001**: The build MUST offer a single option selecting the `iv` UI
  backend (Qt or ImGui) during the transition period.
- **FR-002**: The ImGui backend MUST NOT require Qt at configure, build, or run
  time.
- **FR-003**: Dear ImGui (core plus the GLFW platform backend and the OpenGL
  renderer backend that matches iv's GL context, without the demo),
  tinyfiledialogs, and a generated OpenGL function loader MUST be vendored at
  pinned upstream releases, with license files and the upstream version
  recorded. An option MUST allow using an external Dear ImGui instead.
- **FR-004**: GLFW MUST be found as an installed package when present and
  otherwise built by OIIO's existing dependency auto-build mechanism.
- **FR-004a**: On Linux, `iv` MUST support both X11 and native Wayland,
  chosen at runtime, when built against GLFW 3.4 or newer (including the
  auto-built copy). With a system GLFW 3.3 the display backend is whichever
  one that GLFW was built for. When auto-building GLFW and the Wayland
  development packages are absent, the build MUST fall back to X11-only
  (with a configure message) rather than fail.
- **FR-005**: CI MUST build the ImGui `iv` on Linux, macOS, and Windows.
- **FR-006**: When `iv` cannot be built, configure MUST say why, and the rest
  of OIIO MUST build normally.

**Code structure**

- **FR-007**: Viewer logic (image management, display state, GL image
  rendering, OCIO, comparison math) MUST stay shared between backends.
  Backend-specific code MUST be limited to windowing, event input, menus and
  shortcuts, dialogs, status bar, text overlay drawing, file dialogs, and
  settings storage.
- **FR-008**: Every user-invokable operation (menu item, keyboard shortcut,
  mouse-mode action) MUST be reachable through a named action that can be
  invoked independently of the input that triggered it, and rendering MUST
  NOT assume a visible window. This keeps future headless or scripted testing
  possible without restructuring.
- **FR-009**: Secondary panels (image info, preferences, pixel closeup, about,
  image formats) MUST be built so that moving them from floating panels inside
  the main window to separate OS windows later is a localized change.

**Feature parity** (ImGui backend MUST provide all of the following)

- **FR-010**: File menu: Open (multi-select, image-format filter), Open recent
  (persistent list), Reload, Close Image, Save As, Save Window As, Save
  Selection As, Move to new window, Delete from disk (with confirmation),
  Preferences, Exit.
- **FR-011**: View menu: previous/next/toggle image; channel selection (Full
  Color, R, G, B, A, prev/next); color modes (RGBA, RGB, single channel,
  luminance, heatmap); exposure +/-1/10 and +/-1/2 stop; gamma +/-0.1;
  prev/next subimage and MIP level; zoom in/out, normal size, fit window to
  image, fit image to window; full screen; data/display window borders.
- **FR-012**: Comparison: wipe comparison, Difference submenu (off, absolute,
  FLIP, cycle, FLIP error colormap), and the `--compare` command-line flag.
- **FR-013**: Tools menu: image info window, pixel closeup view, toggle area
  sample, slide show (start, loop, stop at end), sort (name, path, image date,
  file date, reverse), rotate left/right, flip horizontal/vertical.
- **FR-014**: OCIO menu (when OCIO is available): use-OCIO toggle, image color
  space (grouped by family), display/view.
- **FR-015**: Help menu: About and Image Formats dialogs.
- **FR-016**: Every keyboard shortcut in the current build MUST work with the
  same key, with the platform-conventional Cmd/Ctrl mapping on macOS.
- **FR-017**: Mouse modes Zoom, Pan, Select and Annotate are selectable as
  today. Wheel zoom, drag pan, selection rectangle, and wipe drag behave as
  today.
- **FR-018**: Pixel closeup with configurable size and averaging, plus
  readable antialiased pixel-value text that scales correctly on HiDPI.
- **FR-019**: Status bar showing image info, view info, and load progress.
- **FR-020**: Preferences: pixel view follows mouse, linear interpolation,
  dark palette, auto mipmap, closeup pixels, closeup average pixels, slide
  show delay, image cache max memory. All persist across sessions except
  closeup average pixels, which is not saved today and stays that way.
- **FR-021**: Drag-and-drop of image files from the OS onto the window opens
  them.
- **FR-022**: All current command-line options behave identically:
  `filename...`, `-v`, `-F`, `--no-autopremult`, `--rawcolor`,
  `--missing-pixels`, `--compare`, `--display`, `--image-color-space`,
  `--view`.

**Intentional differences**

- **FR-023**: File > Print is removed from both backends. It is already
  disabled and non-functional in the Qt build.
- **FR-024**: Settings and recent files are stored in a new small file in the
  platform-conventional per-user config location. Existing Qt settings are not
  migrated, so users reset preferences and recent files once.
- **FR-025**: Secondary panels are floating panels inside the main window
  rather than separate OS windows (see FR-009).
- **FR-026**: Visual style uses OIIO's bundled fonts and a theme close to the
  current light and dark palettes. Pixel-exact matching of Qt widgets is not
  required.
- **FR-027**: "Move to new window" launches a separate `iv` process for that
  image and removes the image from the current window. The new process shares
  no state (image cache, settings changes in flight) with the original.
- **FR-028**: Menus are an in-window menu bar on every platform. On macOS
  they are not placed in the system menu bar, which is the usual convention
  for ImGui applications.
- **FR-029**: The fixed-function OpenGL path (`FORCE_OPENGL_1` build option)
  is removed from both backends. Only the existing shader-based rendering
  path is required.
- **FR-029a**: Minor differences accepted in the ImGui backend: menu labels
  have no mnemonic underlines and menus are not opened with Alt+letter; the
  busy cursor falls back to the normal arrow where the platform has none;
  About and confirmation dialogs show plain text without bold or links.

**Transition**

- **FR-030**: Qt remains selectable until the ImGui backend reaches parity.
  ImGui then becomes the default, and the Qt backend is removed in a later
  release. That removal is a separate change, not part of this feature.

### Key Entities

- **UI backend**: the build-time choice (Qt or ImGui) that supplies windowing,
  input, menus, dialogs, text overlay and settings storage around the shared
  viewer core.
- **Action**: a named, user-invokable viewer operation with label, optional
  shortcut, enabled and checked state. Menus and shortcuts bind to actions,
  not the reverse.
- **Settings**: persistent per-user preferences and the recent-files list.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: A from-scratch build of the ImGui `iv` and its auto-built UI
  dependencies adds less than 60 seconds over building OIIO without `iv` on a
  typical CI runner.
- **SC-002**: Non-vendored lines under `src/iv` total at most 1.5x the 8018
  line baseline while both backends exist (raised from 1.25x after
  implementation; the aim is still to bring the count back down), and at most the baseline after Qt
  is removed.
- **SC-003**: 100% of the parity checklist (FR-010 to FR-022) passes on Linux,
  macOS and Windows.
- **SC-004**: Shared viewer-logic files (`ivimage.cpp`, `ivgl_ocio.cpp`,
  `ivutils.h`, and the non-UI portions of `imageviewer.cpp` and `ivgl.cpp`)
  show diffs limited to backend seams. A reviewer can identify every changed
  hunk as Qt-specific.
- **SC-005**: Interactive responsiveness (pan, zoom, closeup tracking) on a
  4K image is no worse than the Qt build on the same machine.
- **SC-006**: Users familiar with the Qt `iv` can perform the checklist tasks
  without documentation changes beyond noting Print removal and the
  settings reset.

## Verification

- CI: build-only for the ImGui backend on all three platforms, which matches
  today's level of `iv` coverage.
- Manual parity checklist (derived from FR-010 to FR-022) run on each platform
  before switching the default backend.
- Future work, out of scope here: headless scripted testing (action
  invocation from the command line, framebuffer snapshot compared with
  `idiff`, Xvfb on Linux CI). FR-008 keeps this possible without
  restructuring.
- Status after implementation: both backends build warning-free on macOS;
  the GLFW auto-build and the external ImGui option were exercised there.
  Linux and Windows builds, and the full parity checklist, are not yet run.

## Assumptions

- OpenGL stays the rendering API. The existing GL image rendering and OCIO
  shader code is reused unchanged apart from context and loader setup.
- GLFW 3.3 or newer is acceptable as the minimum. Dear ImGui is a recent
  master-branch release; the docking/viewports branch is not needed now.
- tinyfiledialogs relies at runtime on zenity, kdialog or similar on Linux.
  Their absence is handled with a message, not a hard dependency.
- The ImGui backend uses the OpenGL 2 renderer of Dear ImGui and a
  compatibility context, because iv's own drawing is fixed-function OpenGL
  plus GLSL 1.20.
- Non-vendored `src/iv` code is 10,020 lines while both backends exist
  (8018 before the change; limit 12,027).
- Dear ImGui Test Engine is not used, because its license is unsuitable for
  many commercial OIIO users.
- No translation/i18n support is needed. The Qt build's `tr()` calls are not
  backed by translations today.
- Python bindings and the libraries are unaffected. Only the `iv` executable
  and its build logic change.
- `INSTALL.md` (dependency list, currently naming Qt) is updated to describe
  the backend option and new dependencies.
