# Contract: the `ivui.h` seam

`ivui.h` is the only header shared code includes for UI types. This file
lists what each `Iv*` name must provide in the ImGui build. In the Qt build
each name is an alias of the Qt class in the second column, so the Qt
behavior is the reference.

Rule: the ImGui look-alike implements exactly the members listed here,
because these are the ones iv calls. If shared code needs a new member,
add it here first.

## Types

| Name | Qt build | Members iv uses |
|---|---|---|
| `IvAction` | `QAction` | ctor `(text, parent)` and `(parent)`; `setShortcut`, `setCheckable`, `setChecked`, `isChecked`, `setEnabled`, `setVisible`, `setText`, `text`; signals `triggered`, `toggled(bool)` |
| `IvActionGroup` | `QActionGroup` | ctor `(parent)`; `addAction`, `setExclusive`, `checkedAction` |
| `IvMenu` | `QMenu` | ctor `(title, parent = nullptr)`; `addAction`, `addMenu`, `addSeparator`, `setEnabled` |
| `IvMenuBar` | `QMenuBar` | `addMenu`, `show`, `hide`, `height` |
| `IvStatusBar` | `QStatusBar` | `addWidget`, `show`, `hide`, `height` |
| `IvLabel` | `QLabel` | ctor `()` and `(text)`; `setText`, `show`, `hide`, `isHidden`, `setToolTip` |
| `IvProgressBar` | `QProgressBar` | `setRange`, `setValue`, `reset`, `show`, `hide` |
| `IvComboBox` | `QComboBox` | `addItem`, `currentIndex`, `hide` |
| `IvCheckBox` | `QCheckBox` | ctor `(text)`; `setChecked`, `isChecked` |
| `IvSpinBox` | `QSpinBox` | `setRange`, `setValue`, `value`, `setSingleStep`, `setSuffix`, `setToolTip`, `setAccelerated`; signal `valueChanged(int)` |
| `IvTimer` | `QTimer` | `start(ms)`, `stop`, `isActive`; signal `timeout`; static `singleShot(ms, context, fn)` |
| `IvSignalBlocker` | `QSignalBlocker` | RAII ctor `(IvAction*)` |
| `IvSettings` | `QSettings` | ctor `(org, app)`; `value(key, default = {})` with `.toBool()`, `.toInt()`, `.toStringList()`; `setValue(key, v)` for bool, int, string list |
| `IvStringList` | `QStringList` | iteration, `push_front` |
| `IvPoint` | `QPoint` | ctor `()` and `(x, y)`; `x()`, `y()`; `operator/` by int (wheel delta) |
| `IvColor` | `QColor` | ctor `(r, g, b, a = 255)`; `setAlpha`; component access for the text helper |
| `IvMouseEvent` | `QMouseEvent` | `pos()`, `button()`, `modifiers()` |
| `IvWheelEvent` | `QWheelEvent` | `angleDelta()`, `accept()` |
| `IvKeyEvent` | `QKeyEvent` | `key()`, `modifiers()` |
| `IvResizeEvent`, `IvCloseEvent`, `IvFocusEvent`, `IvEvent` | `QResizeEvent`, `QCloseEvent`, `QFocusEvent`, `QEvent` | passed through to the base-class handler only; no members used |
| `IvWidget` | `QWidget` | pointer type of the `IvGL` constructor's parent argument only |
| `IvApplication` | `QApplication` | ctor `(argc, argv)`; `exec()`; static `processEvents()`, `setOverrideCursor`, `restoreOverrideCursor`, `overrideCursor` |
| `IvMainWindow` | `QMainWindow` | `menuBar()`, `statusBar()`, `setCentralWidget`, `setWindowTitle`, `resize`, `move`, `width`, `height`, `show`, `close`, `raise`, `activateWindow`, `showFullScreen`, `showNormal`, `setCursor`, `unsetCursor`, `devicePixelRatio`, `x`, `y`; static `tr()`; `connect()`, `disconnect(timer, 0, 0, 0)`; virtual `keyPressEvent`, `resizeEvent`, `closeEvent` |
| `IvGLWidget` | small struct deriving `QOpenGLWidget` and `QOpenGLExtraFunctions` | `width`, `height`, `devicePixelRatio`, `update`, `makeCurrent`, `doneCurrent`, `setCursor`, `unsetCursor`, `resize`, `initializeOpenGLFunctions`; ImGui build only, called from the `#else` branches of `shadowed_text` and `draw_lines`: `draw_overlay_text`, `draw_overlay_line`; virtual `initializeGL`, `resizeGL`, `paintGL` and the mouse/wheel/focus/leave handlers |

`namespace Iv` (alias of `Qt` in the Qt build) provides: `NoButton`,
`LeftButton`, `RightButton`, `MiddleButton`; `AltModifier`, `ShiftModifier`,
`ControlModifier`; `Key_Left`, `Key_Right`, `Key_Up`, `Key_Down`,
`Key_PageUp`, `Key_PageDown`, `Key_Escape`, `Key_Minus`, `Key_Underscore`,
`Key_Plus`, `Key_Equal`; `CrossCursor`, `WaitCursor`, `BusyCursor`,
`SplitHCursor`, `SplitVCursor`.

## `connect`

Shared code uses only these forms (after PR step 1), valid for Qt 5 and 6:

```cpp
connect(action, &IvAction::triggered, this, &ImageViewer::slot);   // slot()
connect(action, &IvAction::triggered, this, [this] { ... });
connect(action, &IvAction::toggled,   this, &ImageViewer::slot);   // slot(bool)
connect(timer,  &IvTimer::timeout,    this, &ImageViewer::slot);
connect(spin,   IV_SPIN_VALUE_CHANGED, this, [this](int v) { ... });
```

`IV_SPIN_VALUE_CHANGED` hides the `QOverload<int>` spelling Qt 5 needs.

The three-argument form `connect(sender, signal, callable)` is also
accepted. In the ImGui build a signal holds one slot; a second `connect`
replaces the first.

## Behavior notes for the ImGui build

- `IvMainWindow::resize(w, h)` sets the size of the area below the menu
  bar, matching what Qt reports for a `QMainWindow` with a native menu bar.
- `IvApplication::processEvents()` handles input and draws a frame, but
  does not run queued actions; those run after the frame that queued them.
- `IvGLWidget` sizes and mouse positions are logical pixels relative to the
  image area. `resizeGL` gets the logical size; the widget sets the GL
  viewport and scissor in framebuffer pixels before `paintGL`.
- `IvInfoWindow` and `IvPreferenceWindow` derive from `IvPanel`, a floating
  panel inside the main window with `show`, `hide`, `setWindowTitle` and a
  `draw_contents()` override.

## Strings

- `tr("text")` returns `QString` in Qt and `const char*` in ImGui.
- Shared code passes text to `Iv*` setters as `const char*` or
  `std::string::c_str()`. It builds formatted text with `Strutil::format`,
  never with `QString::arg`.
- Where shared code receives text from an `Iv*` object it converts with
  `iv_str(x)`, which returns `std::string` from either `QString` or
  `std::string`.

## Helper functions

Declared in `ivui.h`, implemented once per backend.

```cpp
std::vector<std::string> iv_open_files(IvMainWindow* parent, const char* title,
                                       std::string& dir, const char* filters);
std::string iv_save_file(IvMainWindow* parent, const char* title,
                         const std::string& suggested,
                         const char* filters = nullptr);
void iv_message(IvMainWindow* parent, const char* title, const std::string& text);
bool iv_confirm(IvMainWindow* parent, const char* title, const std::string& text);
void iv_text_dialog(IvMainWindow* parent, const char* title, const std::string& text);
void iv_post(IvMainWindow* context, std::function<void()> fn);  // dropped if context dies
void iv_available_screen(IvMainWindow* w, int& x, int& y, int& width, int& height);

// ImGui build only, called from #else branches in imageviewer.cpp
void iv_set_dark_style(bool dark);
bool iv_launch(const std::vector<std::string>& args);  // start another iv process
```

In the ImGui build the dialog helpers use tinyfiledialogs. If it has no
graphical helper to run (Linux without zenity, kdialog or similar), they
show a panel that says so; the file helpers then return empty,
`iv_confirm` returns false, and `iv_message` shows its text in the panel.

Empty return from the file helpers means cancelled. `iv_open_files` updates
`dir` to the directory last used.

## Where `#if IV_USE_QT` is allowed in shared files

- `ImageViewer` constructor: palette and drag-drop setup; `setPalette`
  calls where the info and preference windows are created.
- `ImageViewer::dropEvent` / `dragEnterEvent` (Qt only; both call the shared
  `add_dropped_files`).
- `ImageViewer::moveToNewWindow` (in-process window vs new process).
- `ImageViewer::about` (HTML vs plain text).
- `IvGL` constructor (focus policy, mouse tracking), the capability
  detection part of `check_gl_extensions`, `shadowed_text`, and
  `draw_lines` (the closeup corner markers are collected as line segments
  in shared code and drawn by this one backend-specific function).
- `imageviewer.h`: the palette member and accessor, the drag-and-drop
  overrides, and the `IvInfoWindow` / `IvPreferenceWindow` declarations.
- Include blocks for Qt-only headers.
- Creation of `IvInfoWindow` / `IvPreferenceWindow` if their constructors
  differ.

Anything else needing an `#if` is a sign the seam is missing a member or a
helper; fix it there.
