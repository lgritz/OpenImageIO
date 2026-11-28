# Contract: build options and dependency behavior

## CMake variables

| Variable | Values | Default | Effect |
|---|---|---|---|
| `OIIO_IV_BACKEND` | `Qt`, `ImGui` | `Qt` during the transition | Selects the iv UI backend. Any other value is a configure error. |
| `OIIO_IV_EXTERNAL_IMGUI` | `ON`, `OFF` | `OFF` | `ON`: use `find_package(imgui CONFIG)` (`imgui::imgui`) instead of `src/iv/ext/imgui`. The package must provide the GLFW and OpenGL 2 backends. |
| `USE_QT` | `ON`, `OFF` | `ON` | Unchanged. Only consulted when the backend is `Qt`. |
| `ENABLE_iv` (existing `check_is_enabled`) | `ON`, `OFF` | `ON` | Unchanged. `OFF` skips iv entirely. |
| `OpenImageIO_BUILD_MISSING_DEPS` | existing | existing | Listing `glfw3` (or `all`) allows auto-building GLFW. |
| `glfw3_BUILD_VERSION` | version | `3.4` | Version used by `build_glfw3.cmake`. |

Removed: `FORCE_OPENGL_1`.

## Dependency resolution by backend

| Backend | Searched | Not searched |
|---|---|---|
| `Qt` | OpenGL, Qt6 then Qt5 (as today) | glfw3 |
| `ImGui` | OpenGL, glfw3 >= 3.3 (config package; auto-build allowed) | Qt5, Qt6 |

## Outcomes

| Situation | Result |
|---|---|
| Backend deps found | iv is built. Configure prints `iv backend: Qt` or `iv backend: ImGui`. |
| `ImGui`, glfw3 missing, auto-build allowed | GLFW 3.4 is fetched and built static; iv is built. |
| `ImGui`, auto-building on Linux without Wayland dev packages | GLFW is built X11-only; a status line says so. |
| Backend deps missing, no auto-build | Warning naming the missing package; iv is skipped; everything else builds. |
| OpenGL missing | Same warning and skip, as today. |

## Compile definitions on the `iv` target

- `IV_USE_QT` and `OIIO_QT_MAJOR=5|6`, or `IV_USE_IMGUI`.
- `GL_SILENCE_DEPRECATION` on Apple (as today).

## Platform packages for the ImGui backend (documented in INSTALL.md)

- Linux, system GLFW: `libglfw3-dev` (Debian/Ubuntu), `glfw-devel` (Fedora).
- Linux, auto-built GLFW: X11 headers (`xorg-dev` or `libx11-dev
  libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev`); optional Wayland
  support: `libwayland-dev libxkbcommon-dev wayland-protocols`.
- macOS: `brew install glfw`, or auto-build (needs only Xcode tools).
- Windows: auto-build, or vcpkg `glfw3`.
- Linux runtime, for file dialogs: one of zenity, kdialog, or similar.
