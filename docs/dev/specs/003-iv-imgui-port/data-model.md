# Data Model: Port iv from Qt to Dear ImGui

iv has no database. The "model" here is the set of UI objects that hold
state, the settings file, and how input reaches the viewer. The exact
method list per type is in [contracts/ivui-seam.md](contracts/ivui-seam.md).

## UI objects (look-alike types in the ImGui build)

| Entity | Holds | Relationships |
|---|---|---|
| `IvAction` | label, shortcut, enabled, visible, checkable, checked, signals-blocked flag, `triggered` and `toggled` callbacks | Owned by `ImageViewer`. Listed in zero or more `IvMenu`s. May belong to one `IvActionGroup`. |
| `IvActionGroup` | member actions, exclusive flag | Checking one member unchecks the others. |
| `IvMenu` | title, enabled, ordered items (action, submenu or separator) | Top-level menus are listed in the `IvMenuBar`. |
| `IvMenuBar` | ordered top-level menus, visible flag, height | One per `IvMainWindow`. |
| `IvStatusBar` | ordered child widgets, visible flag, height | One per `IvMainWindow`. |
| `IvLabel` | text, visible | Status bar children; preference labels. |
| `IvProgressBar` | min, max, value, visible | Status bar child. |
| `IvComboBox` | items, current index, visible | Status bar child (mouse mode; hidden today). |
| `IvCheckBox` | label, checked | Preferences. |
| `IvSpinBox` | min, max, step, value, suffix, tooltip, `valueChanged` callback | Preferences. |
| `IvTimer` | interval, active, single-shot, `timeout` callback | Slide show; `--compare` startup delay. |
| `IvSettings` | key to value(s) map backed by the settings file | Read at startup, written at close. |
| `IvMainWindow` | GLFW window, title, size, position, full-screen flag, menu bar, status bar, central `IvGLWidget` | Base class of `ImageViewer`. |
| `IvGLWidget` | logical size, image-area origin, device pixel ratio, cursor shape, redraw-needed flag | Base class of `IvGL`. |
| `IvApplication` | action queue, posted-function queue, timers, override cursor | One per process. Runs the frame loop. |

State rules carried over from Qt:
- `setChecked()` on an action emits `toggled` unless signals are blocked
  (`IvSignalBlocker`). `trigger()` (menu click or shortcut) flips a
  checkable action, then emits `triggered`.
- A disabled action or menu is drawn greyed and ignores its shortcut.
- An invisible action is not drawn (used for unused recent-file slots).

## Events (look-alike, ImGui build)

| Event | Fields used by iv | Delivered to |
|---|---|---|
| `IvMouseEvent` | `pos()` (logical, image-area relative), `button()`, `modifiers()` | `IvGL::mousePressEvent`, `mouseReleaseEvent`, `mouseMoveEvent` |
| `IvWheelEvent` | `angleDelta()`, `accept()` | `IvGL::wheelEvent` |
| `IvKeyEvent` | `key()`, `modifiers()` | `ImageViewer::keyPressEvent` |
| resize | new size | `ImageViewer::resizeEvent`, `IvGL::resizeGL` |
| close | none | `ImageViewer::closeEvent` (writes settings) |
| focus out / leave | none | `IvGL::focusOutEvent`, `leaveEvent` |
| file drop | list of paths | shared `ImageViewer::add_dropped_files()`; the Qt `dropEvent` extracts paths and calls the same function |

Small value types: `IvPoint` (`x()`, `y()`), `IvColor` (r, g, b, a),
`IvStringList`. Enumerations live in namespace `Iv` (alias of `Qt` in the Qt
build): mouse buttons, keyboard modifiers, the handful of `Key_*` values in
`keyPressEvent`, cursor shapes.

## Settings file

Location: see research.md R7. Format: one `key=value` per line; a list key
repeats, one line per entry.

| Key | Type | Default | Used for |
|---|---|---|---|
| `pixelviewFollowsMouse` | bool | false | Preferences |
| `linearInterpolation` | bool | false | Preferences |
| `darkPalette` | bool | false | Preferences; read before the UI is built |
| `autoMipmap` | bool | false | ImageCache `automip` (restart) |
| `maxMemoryIC` | int (MB) | 2048 (512 on 32-bit) | ImageCache `max_memory_MB` (restart) |
| `slideShowDuration` | int (s) | 10 | Slide show |
| `closeupPixels` | int | 13 | Pixel closeup |
| `RecentFiles` | string list | empty | File > Open recent (max 10) |

Keys, types and defaults match the Qt build's `readSettings()` and
`writeSettings()`. A missing or unreadable file means all defaults. Unknown
keys are ignored and not preserved.

## Lifecycle

1. `main`: parse arguments, optionally background the process, construct
   `IvApplication`, construct `ImageViewer` (creates window and GL context,
   reads settings, builds actions and menus), add images.
2. `IvApplication::exec()`: frame loop until the main window closes.
3. Close: `closeEvent` writes settings; loop exits; ImGui and GLFW shut
   down.
