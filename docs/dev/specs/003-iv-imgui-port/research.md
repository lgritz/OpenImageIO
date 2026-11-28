# Research: Port iv from Qt to Dear ImGui

Phase 0 output. Each item records a decision, why, and what was rejected.
Line numbers refer to `src/iv` at the time of writing.

## R1. How the two backends share code (the seam)

**Decision**: Look-alike types (chosen by the user).

- A new header `ivui.h` is the only place that knows which backend is built.
- In the Qt build it contains aliases: `using IvAction = QAction;`,
  `using IvMenu = QMenu;`, `namespace Iv = Qt;`, and so on.
- In the ImGui build it includes `ivui_imgui.h`, which declares small classes
  with the same names and the same method names as the Qt subset iv uses.
- Shared code (`imageviewer.cpp`, `ivgl.cpp`, `ivmain.cpp`) is edited only to
  rename types (`QAction` to `IvAction`, `Qt::LeftButton` to
  `Iv::LeftButton`) and to route a few procedural Qt calls through helper
  functions (R6).
- The ImGui side translates GLFW/ImGui input into look-alike event objects
  and calls the existing handlers (`mousePressEvent`, `keyPressEvent`, ...).

**Rationale**: iv stores UI state inside Qt objects. Pixel view on/off is
`showPixelviewWindowAct->isChecked()`, preferences live in `QCheckBox` and
`QSpinBox`, and about 60 call sites read or write that state. About 500 lines
build 90 actions and the menu tree. Look-alikes leave all of that code as it
is, which gives the smallest and most mechanical diff.

**Alternatives considered**:
- Shared action table with state moved into plain members. Less code in the
  end, but rewrites about 600 shared lines and touches every state call site.
- Separate file per backend. Needs the same state move, and every menu item
  is written twice until Qt is removed.

**Consequence**: After Qt removal the code still reads Qt-style
(`connect(...)`, `setChecked`). A later tidy-up can collapse it (R14).

## R2. OpenGL context and ImGui renderer backend

**Finding**: `ivgl.cpp` and `ivgl_ocio.cpp` use fixed-function OpenGL
(`glMatrixMode`, `glTranslatef`, `glBegin`/`glVertex3f`, `glPushAttrib`)
together with GLSL 1.20 shaders. OCIO shaders are requested as
`GPU_LANGUAGE_GLSL_1_2`. This needs a compatibility context. Qt's default
surface format provides one today. On macOS a compatibility context is
OpenGL 2.1 only.

**Decision**:
- Create the GLFW window with default context hints (no core profile
  request). That yields 2.1 on macOS and the driver's compatibility profile
  on Linux and Windows, the same as Qt gives iv today.
- Use ImGui's `imgui_impl_opengl2` renderer backend. It is written for
  fixed-function contexts and needs no GL loader of its own.
- iv must leave no shader program, PBO, or non-default texture unit bound
  when ImGui draws. The ImGui frame code unbinds them before
  `ImGui_ImplOpenGL2_RenderDrawData`.

**Rationale**: No change to iv's rendering code. The OpenGL 3 backend's
bundled loader and vertex-array use are not reliable on a 2.1 context.

**Alternatives considered**:
- `imgui_impl_opengl3` with `#version 120`. Its docs list GL 2.1, but its
  loader and VAO paths on a macOS 2.1 context are unverified. Kept as the
  fallback if the spike (R15) shows a problem with the OpenGL 2 backend.
- Port iv's GL code to core profile. Correct long-term direction, far out of
  scope here, and it would break the "minimal diff" goal.

**Spike result (2026-11-28, macOS, Apple M4 Max, GLFW 3.5.1, ImGui
v1.92.9b)**: confirmed. Default-hint GLFW window gives "2.1 Metal", GLSL
1.20. An iv-style quad (GLSL 1.20 program, fixed-function matrices, texture
uploaded from a PBO on texture unit 1) renders correctly under ImGui's
OpenGL 2 backend with menu bar and background-draw-list text on top, at 2x
Retina scale, no GL errors. Leaving a PBO bound and unit 1 active when ImGui
draws breaks it (blank menu bar, no text, `GL_INVALID_OPERATION`), so the
unbind rule above is mandatory. All of ImGui compiles in about 4 seconds.
glad was not exercised (not needed on macOS); it is first tested on Windows.

## R3. OpenGL function access without Qt

**Finding**: `IvGL` inherits `QOpenGLExtraFunctions`, so calls like
`glCreateShader` resolve to Qt member functions. Without Qt, Windows headers
expose only OpenGL 1.1.

**Decision**: Vendor a glad 2 generated header for OpenGL 2.1 compatibility
plus the few extensions iv tests for (half-float, float and sRGB textures).
Load it once with `gladLoadGL(glfwGetProcAddress)`. Use it on all platforms
so there is one include path.

**Alternatives considered**: GLEW (a new dependency to find or build);
`GL_GLEXT_PROTOTYPES` on Linux plus native headers on macOS plus a loader on
Windows only (three code paths).

`check_gl_extensions()` (ivgl.cpp:2328) uses `QOpenGLContext`. It gets an
`#if` branch that uses the glad version and extension flags.

## R4. Windowing: GLFW

**Decision**: GLFW, minimum 3.3, auto-build version 3.4. Found through its
CMake config package `glfw3` (target `glfw`). New `src/cmake/build_glfw3.cmake`
follows the existing `build_dependency_with_cmake` pattern (see
`build_Robinmap.cmake`), static library, examples/tests/docs off.

Linux (FR-004a): GLFW 3.4 builds X11 and Wayland by default and picks at
runtime. The build script checks for `wayland-scanner`, `wayland-client` and
`xkbcommon`; if any is missing it passes `-DGLFW_BUILD_WAYLAND=OFF` and
prints a status message. X11 development headers are required (the configure
warning names the packages).

GLFW features used: window, GL context, content scale, monitor work area
(`glfwGetMonitorWorkarea`, for fit-window-to-image), full screen
(`glfwSetWindowMonitor`), file drop callback, standard cursors,
`glfwWaitEventsTimeout` and `glfwPostEmptyEvent`.

## R5. Dear ImGui version and vendoring

**Decision**: Vendor the latest 1.92.x master-branch release at
implementation time (1.92.9 was current when this was written). Files:
`imgui.cpp`, `imgui_draw.cpp`, `imgui_tables.cpp`, `imgui_widgets.cpp`,
`imgui.h`, `imgui_internal.h`, `imconfig.h`, the three `imstb_*.h`,
`backends/imgui_impl_glfw.{h,cpp}`, `backends/imgui_impl_opengl2.{h,cpp}`,
`LICENSE.txt`. No demo, no other backends.

- Location: `src/iv/ext/imgui/`, with a short `README.md` giving the upstream
  tag and the update procedure (copy files, no local patches).
- 1.92 or newer is required: it has dynamic font sizing (R9), the Cmd/Ctrl
  swap on macOS (R8), and wait/progress mouse cursors.
- Add `src/iv/ext/*` to `.clang-format-ignore` and list the licenses in
  `THIRD-PARTY.md`.
- Option `OIIO_IV_EXTERNAL_IMGUI=ON` uses `find_package(imgui CONFIG)`
  (target `imgui::imgui`, as shipped by vcpkg and Conan) instead. It is best
  effort: the external package must include the GLFW and OpenGL 2 backends.

**Multi-viewport later (FR-009)**: secondary panels are ordinary ImGui
windows. Moving them to separate OS windows means vendoring the docking
branch instead of master and adding the platform-window calls to the frame
loop (about 10 lines). Panel code does not change.

## R6. Procedural Qt calls: helper functions

Some Qt uses are calls, not objects, and have no sensible look-alike. They
go behind small free functions declared in `ivui.h`, implemented in
`ivui_qt.cpp` (existing code moved) and `ivui_imgui.cpp`.

| Helper | Replaces | ImGui implementation |
|---|---|---|
| `iv_open_files(title, dir, filters)` | `QFileDialog` in `open()` | `tinyfd_openFileDialog`, multi-select |
| `iv_save_file(title, name, filters)` | `QFileDialog::getSaveFileName` (3 sites) | `tinyfd_saveFileDialog` |
| `iv_message(title, text)` | `QMessageBox::information`, `about` | `tinyfd_messageBox` |
| `iv_confirm(title, text)` | `QMessageBox::question` | `tinyfd_messageBox` yes/no |
| `iv_text_dialog(title, text)` | `QDialog` + `QTextEdit` in `showImageFormats()` | ImGui panel with scrolling text |
| `iv_post(fn)` | `QMetaObject::invokeMethod(..., Qt::QueuedConnection)` (FLIP thread result) | mutex-protected queue + `glfwPostEmptyEvent` |
| `iv_available_screen(x, y, w, h)` | `screen()->availableGeometry()` and its Qt5 variant | `glfwGetMonitorWorkarea` |

tinyfiledialogs dialogs are native and block the caller, as the Qt ones do,
so the control flow in `open()`, `saveAs()` and `deleteCurrentImage()` does
not change. tinyfiledialogs is vendored (`tinyfiledialogs.c/.h`, zlib
license) in `src/iv/ext/tinyfiledialogs/`. On Linux it runs zenity, kdialog
or similar; if none exists it returns failure and iv shows an ImGui message
panel saying which tools to install.

File filters: the Qt filter string (`"JPEG (*.jpg *.jpeg);;..."`) stays the
single source. The ImGui helper splits it into the pattern arrays
tinyfiledialogs wants.

## R7. Settings storage

**Decision**: Look-alike `IvSettings` with `value(key, default)` and
`setValue(key, value)`, so `readSettings()`/`writeSettings()` stay shared.

- File: plain `key=value` lines, list values one `key=value` line per entry.
- Location: `$XDG_CONFIG_HOME` or `~/.config` + `/OpenImageIO/iv.ini` on
  Linux; `~/Library/Application Support/OpenImageIO/iv.ini` on macOS;
  `%APPDATA%\OpenImageIO\iv.ini` on Windows.
- The name differs from Qt's `iv.conf` on Linux on purpose, so the two
  builds never read each other's encoding.
- ImGui's own layout file (panel positions) is written next to it as
  `iv_layout.ini`.
- Keys and defaults are unchanged: see data-model.md.

## R8. Keyboard shortcuts

Qt shortcut strings stay in shared code (`setShortcut(tr("Ctrl+O"))`). The
look-alike `IvAction` parses them once:

- Modifiers `Ctrl`, `Shift`, `Alt`. ImGui (1.90.6 and newer) swaps Cmd and
  Ctrl on macOS at the input level, so `Ctrl` in the table means Cmd there,
  which matches Qt.
- Letters, digits and named keys (`PgUp`, `PgDown`, `Delete`) map to
  `ImGuiKey` chords. Letters ignore case, as in Qt.
- Punctuation without Ctrl/Alt (`[ ] { } ( ) < > , .`) is matched against
  typed characters from ImGui's character queue, so it is correct on
  non-US keyboard layouts.
- Shortcuts and key events are not delivered while an ImGui text field has
  focus (`io.WantTextInput`).
- The extra keys handled in `keyPressEvent` (arrows, Escape, `+ - = _`) are
  delivered as look-alike `IvKeyEvent` objects.

## R9. Text and fonts

- UI font: Droid Sans. Pixel-value overlay font: Droid Sans Mono. Both are
  already in `src/fonts/`.
- They are embedded at build time: a short CMake step turns the two `.ttf`
  files into a generated `.cpp` in the build directory. No runtime font
  lookup, no generated data in the source tree.
- `shadowed_text()` (ivgl.cpp:800) paints through `QPainter` onto a
  `QImage`. Its ImGui branch is one `AddText` call on ImGui's background draw
  list. The `draw_corners` lambda (ivgl.cpp:1391) becomes `AddLine` calls.
  Both are `#if` blocks; the ImGui versions are shorter than the Qt ones.
- Draw order per frame: clear, iv GL drawing (image, closeup, guides), then
  ImGui draw data (overlay text, then menus and panels on top).
- `update_area_probe_text()` formats with `QString::arg`. It is rewritten
  once with `Strutil::format` and shared.

## R10. HiDPI and coordinates

GLFW reports window size in screen coordinates and framebuffer size in
pixels; which platforms scale which one differs. The look-alike
`IvGLWidget` hides that and presents what Qt presents:

- `width()`/`height()`: logical size of the image area (window minus menu
  bar and status bar).
- `devicePixelRatio()`: framebuffer pixels per logical unit, taken from the
  framebuffer/window ratio on macOS and Wayland and from
  `glfwGetWindowContentScale` on Windows and X11.
- Mouse positions in events are logical and relative to the image area.
- Fonts are rasterized at logical size times the ratio.

The image area is not at the window origin, so `glViewport` needs an offset.
`IvGLWidget` supplies it; in the Qt build the offset is zero. This touches
the two places that set the viewport (ivgl.cpp `resizeGL` and line 1713).

## R11. Frame loop and re-entrancy

- The loop is event driven. It blocks in `glfwWaitEventsTimeout` and draws
  only when there was input, `update()` was called, a timer fired, or a
  posted function is waiting. iv uses no CPU when idle. Two extra frames are
  drawn after input so ImGui hover states settle.
- Actions triggered from menus or shortcuts are queued during the ImGui
  frame and run after it ends. ImGui does not allow a frame inside a frame,
  and iv's actions can draw (next point).
- `IvApplication::processEvents()` polls events and draws one frame without
  running queued actions. `image_progress_callback` and the smooth-zoom loops
  (imageviewer.cpp:2554, 2844) call it, as they call
  `QApplication::processEvents()` today.
- Look-alike `IvTimer` (slide show, `singleShot` for `--compare`) is checked
  by the loop, which also uses the next deadline as the wait timeout.

## R12. Removed or changed features in the ImGui build

- Print: the action is already disabled and its body refers to a
  nonexistent `imageLabel`. Remove it from both builds.
- `FORCE_OPENGL_1`: remove the option and its code paths from both builds.
- Move to new window (FR-027): ImGui build starts
  `Sysutil::this_program_path()` with the image path and the OCIO options,
  then closes the image locally. Qt build keeps its in-process window.
- Busy cursor: ImGui 1.92 has wait/progress cursors; GLFW falls back to the
  arrow where the platform has none.
- Mnemonic underlines (`&Open`): the `&` is stripped from labels.
- Dialog text with HTML (`about()`, delete confirmation): plain text in the
  ImGui build.

## R13. Build and CI

- New cache variable `OIIO_IV_BACKEND` with values `Qt` (default during the
  transition) and `ImGui`. Qt packages are searched only for `Qt`, GLFW only
  for `ImGui`. `CMAKE_AUTOMOC` is set only for `Qt`.
- `src/iv/CMakeLists.txt` lists sources explicitly per backend
  (`fancy_add_executable` accepts `SRC`).
- Compile definition `IV_USE_QT` or `IV_USE_IMGUI`.
- CI: one ImGui-backend job each on Linux, macOS and Windows (FR-005), with
  GLFW auto-built in at least one of them. Existing Qt jobs stay.
  `gh-installdeps.bash` gains the X11/Wayland development packages.
- `INSTALL.md` documents the option and dependencies.

## R14. Code size budget

Baseline 8018 lines. Limit while both backends exist: 12,027 (1.5x; originally 10,020, 1.25x).

| Item | Lines (estimate) |
|---|---|
| `ivui.h` (aliases, helper declarations) | +120 |
| `ivui_imgui.h` (look-alike declarations) | +350 |
| `ivui_imgui.cpp` (loop, menus, shortcuts, events, dialogs, settings) | +650 |
| `ivui_qt.cpp` (helpers, mostly moved code) | +80 |
| ImGui info and preferences panels | +70 |
| `#if` branches in `ivgl.cpp`, `imageviewer.cpp/.h`, `ivmain.cpp` | +140 |
| CMake | +60 |
| Print and `FORCE_OPENGL_1` removal | -80 |
| **Net** | **about +1400 (total about 9400)** |

That leaves roughly 600 lines of margin.

Measured after implementation: 10,020 lines, exactly the original 1.25x
limit, so the estimate above was about 600 lines low. Most of the excess is in
`ivui_imgui.cpp` (1221 lines against 720 estimated: shortcut parsing, HiDPI
handling, the settings file and the two panels took more than planned) and
`ivui_imgui.h` (488 against 350). The limit was then raised to 1.5x to
leave room for fixes, with the aim of bringing the count back down later.

Baseline measured on main at 673efe0a2 with
`cat src/iv/*.cpp src/iv/*.h src/iv/CMakeLists.txt | wc -l`: 8018.

After Qt removal (separate change): the alias blocks, Qt `#if` branches,
`ivui_qt.cpp` and the Qt info/preferences windows go away (about -450),
leaving about +950 over baseline. Reaching "no larger than baseline"
(SC-002) needs the tidy-up that look-alikes make possible afterwards:
a one-line-per-action helper in `createActions()` (about -250), dropping
look-alike methods that only existed for Qt parity, and removing long-dead
commented code. This is achievable but not automatic; it is recorded as a
risk in plan.md.

## R15. Spike before the main work

One throwaway prototype, not merged, to confirm R2, R3 and R10 on macOS (the
riskiest platform): GLFW default-hint window, glad, ImGui with the OpenGL 2
backend, one iv-style textured quad through a GLSL 1.20 shader, ImGui menu
bar and overlay text on top, Retina scaling correct. If the OpenGL 2 backend
misbehaves, switch to the OpenGL 3 backend with `#version 120` before any
real code depends on the choice.

## R16. Testing

No automated UI tests in this feature (spec, Verification). CI builds both
backends. The manual parity checklist is in quickstart.md. FR-008 holds by
construction: every operation is an `IvAction` with a `trigger()` method,
reachable by label, and drawing is requested through `update()`, not tied to
window visibility. A later `--exec` option and framebuffer snapshot can be
added in `ivui_imgui.cpp` without touching shared code.

## Sources

- Dear ImGui releases: https://github.com/ocornut/imgui/releases
- Dear ImGui backends: https://github.com/ocornut/imgui/tree/master/backends
- GLFW: https://www.glfw.org/docs/latest/
- tinyfiledialogs: https://sourceforge.net/projects/tinyfiledialogs/
- glad: https://github.com/Dav1dde/glad
