// Copyright Contributors to the OpenImageIO project.
// SPDX-License-Identifier: Apache-2.0
// https://github.com/AcademySoftwareFoundation/OpenImageIO

// Dear ImGui + GLFW implementation of the GUI types and helper functions
// declared in ivui.h / ivui_imgui.h.

#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>
#undef GLAD_GL_IMPLEMENTATION

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl2.h>
#include <tinyfiledialogs.h>

#include <algorithm>
#include <mutex>

#ifdef _WIN32
#    include <process.h>
#else
#    include <spawn.h>
extern char** environ;
#endif

#include <OpenImageIO/filesystem.h>
#include <OpenImageIO/strutil.h>
#include <OpenImageIO/sysutil.h>

#include "imageviewer.h"

extern const unsigned char iv_font_sans[], iv_font_mono[];
extern const unsigned int iv_font_sans_size, iv_font_mono_size;

namespace {

IvMainWindow* g_mainwin = nullptr;
std::vector<IvAction*> g_actions;
std::vector<IvTimer*> g_timers;
std::vector<IvPanel*> g_panels;
ImFont* g_font_mono = nullptr;
bool g_dark         = true;
bool g_in_frame     = false;
int g_frames_needed = 3;  // frames still to draw before idling
float g_wheel       = 0;  // accumulated wheel motion, in notches
int g_buttons_down  = 0;  // mouse buttons whose press went to the image
bool g_mouse_inside = false;

// Input events and action triggers found while building a frame. They run
// after the frame ends, because handling them may draw frames itself.
std::vector<std::function<void()>> g_queue;

// Functions posted from other threads.
std::mutex g_posted_mutex;
std::vector<std::function<void()>> g_posted;

const float base_font_size = 14;  // logical pixels
const float mono_font_size = 16;



// ImGui units per logical pixel: 1 where the window system already scales
// window coordinates (macOS, Wayland), the content scale elsewhere.
float
ui_scale()
{
    GLFWwindow* w = g_mainwin->m_window;
    int ww, wh, fw, fh;
    glfwGetWindowSize(w, &ww, &wh);
    glfwGetFramebufferSize(w, &fw, &fh);
    float fbscale = ww > 0 ? float(fw) / float(ww) : 1.0f;
    return g_mainwin->devicePixelRatio() / fbscale;
}



std::string
strip_mnemonic(const char* text)
{
    return Strutil::replace(text, "&", "", true);
}



void
set_glfw_cursor(Iv::CursorShape shape)
{
    static GLFWcursor* cross = glfwCreateStandardCursor(GLFW_CROSSHAIR_CURSOR);
    ImGuiIO& io              = ImGui::GetIO();
    io.ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;
    switch (shape) {
    case Iv::CrossCursor:
        // ImGui has no crosshair, so take over from its backend.
        io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
        glfwSetCursor(g_mainwin->m_window, cross);
        break;
    case Iv::WaitCursor: ImGui::SetMouseCursor(ImGuiMouseCursor_Wait); break;
    case Iv::BusyCursor:
        ImGui::SetMouseCursor(ImGuiMouseCursor_Progress);
        break;
    case Iv::SplitHCursor:
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        break;
    case Iv::SplitVCursor:
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
        break;
    default: break;
    }
}



// Turn this frame's keyboard input into action triggers and key events.
void
handle_keys()
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantTextInput)
        return;
    int mods = io.KeyMods & (ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiMod_Alt);
    std::vector<int> used_keys;
    for (IvAction* a : g_actions) {
        if (!a->m_enabled)
            continue;
        bool hit = false;
        if (a->m_keychord) {
            ImGuiKey key = ImGuiKey(a->m_keychord & ~ImGuiMod_Mask_);
            int want     = a->m_keychord & ImGuiMod_Mask_;
            int have = a->m_shift_optional ? (mods & ~ImGuiMod_Shift) : mods;
            used_keys.push_back(key);
            hit = have == want && ImGui::IsKeyPressed(key, true);
        } else if (a->m_char && !(mods & (ImGuiMod_Ctrl | ImGuiMod_Alt))) {
            for (ImWchar c : io.InputQueueCharacters)
                hit |= (c == a->m_char);
        }
        if (hit)
            g_queue.push_back([a] { a->trigger(); });
    }

    auto send = [&](int key) {
        IvKeyEvent ev;
        ev.m_key       = key;
        ev.m_modifiers = ((mods & ImGuiMod_Shift) ? Iv::ShiftModifier : 0)
                         | ((mods & ImGuiMod_Ctrl) ? Iv::ControlModifier : 0)
                         | ((mods & ImGuiMod_Alt) ? Iv::AltModifier : 0);
        g_queue.push_back([ev]() mutable { g_mainwin->keyPressEvent(&ev); });
    };
    static const std::pair<ImGuiKey, int> special[] = {
        { ImGuiKey_LeftArrow, Iv::Key_Left },
        { ImGuiKey_RightArrow, Iv::Key_Right },
        { ImGuiKey_UpArrow, Iv::Key_Up },
        { ImGuiKey_DownArrow, Iv::Key_Down },
        { ImGuiKey_PageUp, Iv::Key_PageUp },
        { ImGuiKey_PageDown, Iv::Key_PageDown },
        { ImGuiKey_Escape, Iv::Key_Escape },
    };
    // A key that is some action's shortcut is not also sent as a key event.
    for (auto& s : special)
        if (ImGui::IsKeyPressed(s.first, true)
            && std::find(used_keys.begin(), used_keys.end(), int(s.first))
                   == used_keys.end())
            send(s.second);
    if (!(mods & (ImGuiMod_Ctrl | ImGuiMod_Alt)))
        for (ImWchar c : io.InputQueueCharacters)
            if (c == '+' || c == '-' || c == '=' || c == '_')
                send(c);
}



// Turn this frame's mouse input over the image area into events for the GL
// widget.
void
handle_mouse(IvGLWidget* gl)
{
    ImGuiIO& io = ImGui::GetIO();
    float s     = gl->m_ui_scale;
    float mx    = (io.MousePos.x - gl->m_x0) / s,
          my    = (io.MousePos.y - gl->m_y0) / s;
    bool inside = !io.WantCaptureMouse && mx >= 0 && my >= 0 && mx < gl->width()
                  && my < gl->height();
    IvMouseEvent ev;
    ev.m_pos       = IvPoint(int(mx), int(my));
    ev.m_modifiers = (io.KeyShift ? Iv::ShiftModifier : 0)
                     | (io.KeyCtrl ? Iv::ControlModifier : 0)
                     | (io.KeyAlt ? Iv::AltModifier : 0);
    static const Iv::MouseButton buttons[] = { Iv::LeftButton, Iv::RightButton,
                                               Iv::MiddleButton };
    for (int b = 0; b < 3; ++b) {
        ev.m_button = buttons[b];
        if (inside && ImGui::IsMouseClicked(b)) {
            g_buttons_down |= (1 << b);
            g_queue.push_back([gl, ev]() mutable { gl->mousePressEvent(&ev); });
        }
        if ((g_buttons_down & (1 << b)) && ImGui::IsMouseReleased(b)) {
            g_buttons_down &= ~(1 << b);
            g_queue.push_back(
                [gl, ev]() mutable { gl->mouseReleaseEvent(&ev); });
        }
    }
    ev.m_button = Iv::NoButton;
    if ((inside || g_buttons_down)
        && (io.MouseDelta.x != 0 || io.MouseDelta.y != 0))
        g_queue.push_back([gl, ev]() mutable { gl->mouseMoveEvent(&ev); });
    if (g_mouse_inside && !inside && !g_buttons_down)
        g_queue.push_back([gl] {
            IvEvent leave;
            gl->leaveEvent(&leave);
        });
    g_mouse_inside = inside;

    // Deliver the wheel one notch at a time, so that the small increments
    // of a trackpad add up instead of each counting as a full step.
    if (inside)
        g_wheel += io.MouseWheel;
    for (; std::abs(g_wheel) >= 1; g_wheel -= (g_wheel > 0 ? 1 : -1)) {
        IvWheelEvent wheel;
        wheel.m_delta = IvPoint(0, g_wheel > 0 ? 120 : -120);
        g_queue.push_back([gl, wheel]() mutable { gl->wheelEvent(&wheel); });
    }

    Iv::CursorShape cursor = !IvApplication::s_cursors.empty()
                                 ? IvApplication::s_cursors.back()
                             : gl->m_cursor != Iv::ArrowCursor
                                 ? gl->m_cursor
                                 : g_mainwin->m_cursor;
    set_glfw_cursor(
        inside || !IvApplication::s_cursors.empty() ? cursor : Iv::ArrowCursor);
}



// Build and draw one frame of the whole window.
void
draw_frame()
{
    if (g_in_frame || !g_mainwin || g_mainwin->m_closed)
        return;
    g_in_frame       = true;
    IvMainWindow* mw = g_mainwin;
    IvGLWidget* gl   = mw->m_central;
    float scale      = ui_scale();

    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGuiIO& io = ImGui::GetIO();

    float top = 0, bottom = io.DisplaySize.y;
    if (mw->m_menubar.m_visible && ImGui::BeginMainMenuBar()) {
        for (IvMenu* m : mw->m_menubar.m_menus) {
            if (ImGui::BeginMenu(m->m_title.c_str(), m->m_enabled)) {
                m->draw();
                ImGui::EndMenu();
            }
        }
        top                    = ImGui::GetWindowSize().y;
        mw->m_menubar.m_height = int(top / scale);
        ImGui::EndMainMenuBar();
    }
    if (mw->m_statusbar.m_visible) {
        float h = ImGui::GetFrameHeight();
        bottom -= h;
        mw->m_statusbar.m_height = int(h / scale);
        ImGui::SetNextWindowPos(ImVec2(0, bottom));
        ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, h));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,
                            ImVec2(ImGui::GetStyle().FramePadding.x, 0));
        ImGui::Begin("##statusbar", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove
                         | ImGuiWindowFlags_NoSavedSettings
                         | ImGuiWindowFlags_NoBringToFrontOnFocus
                         | ImGuiWindowFlags_NoFocusOnAppearing);
        ImGui::AlignTextToFramePadding();
        for (IvWidget* w : mw->m_statusbar.m_widgets) {
            if (w->m_visible) {
                w->draw();
                ImGui::SameLine(0, 2 * ImGui::GetStyle().ItemSpacing.x);
            }
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }
    for (size_t i = 0; i < g_panels.size(); ++i)
        g_panels[i]->draw();

    handle_keys();

    int fbw, fbh;
    glfwGetFramebufferSize(mw->m_window, &fbw, &fbh);
    glViewport(0, 0, fbw, fbh);
    glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    if (gl && bottom > top && fbw > 0) {
        float fbscale  = float(fbw) / io.DisplaySize.x;
        int w          = std::max(1, int(io.DisplaySize.x / scale));
        int h          = std::max(1, int((bottom - top) / scale));
        gl->m_x0       = 0;
        gl->m_y0       = top;
        gl->m_ui_scale = scale;
        gl->m_dpr      = mw->devicePixelRatio();
        if (!gl->m_initialized) {
            gl->m_initialized = true;
            gl->initializeGL();
        }
        handle_mouse(gl);
        if (w != gl->m_w || h != gl->m_h) {
            gl->m_w = w;
            gl->m_h = h;
            gl->resizeGL(w, h);
            g_queue.push_back([mw] {
                IvResizeEvent resize;
                mw->resizeEvent(&resize);
            });
        }
        // The image area, in framebuffer pixels with y up.
        int vx = 0, vy = int((io.DisplaySize.y - bottom) * fbscale);
        int vw = fbw, vh = int((bottom - top) * fbscale);
        glViewport(vx, vy, vw, vh);
        glScissor(vx, vy, vw, vh);
        glEnable(GL_SCISSOR_TEST);
        gl->paintGL();
        gl->m_dirty = false;
        // ImGui's fixed-function renderer needs these unbound.
        glDisable(GL_SCISSOR_TEST);
        glUseProgram(0);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
        glActiveTexture(GL_TEXTURE0);
    }

    ImGui::Render();
    glViewport(0, 0, fbw, fbh);
    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(mw->m_window);
    g_in_frame = false;
}



// Run everything that was queued while drawing: input events, action
// triggers, expired timers, and functions posted from other threads.
void
run_queued()
{
    std::vector<std::function<void()>> todo;
    todo.swap(g_queue);
    {
        std::lock_guard<std::mutex> lock(g_posted_mutex);
        todo.insert(todo.end(), g_posted.begin(), g_posted.end());
        g_posted.clear();
    }
    double now = glfwGetTime();
    for (IvTimer* t : std::vector<IvTimer*>(g_timers)) {
        if (!t->m_active || t->m_deadline > now)
            continue;
        t->m_active   = !t->m_single;
        t->m_deadline = now + t->m_interval;
        todo.push_back([t] {
            t->timeout();
            if (t->m_single)
                delete t;
        });
    }
    for (auto& fn : todo)
        fn();
    if (!todo.empty())
        g_frames_needed = 3;
}

}  // namespace



IvAction::IvAction(const char* text, void* /*parent*/)
    : m_text(strip_mnemonic(text))
{
    g_actions.push_back(this);
}



void
IvAction::setText(const char* text)
{
    m_text = strip_mnemonic(text);
}



void
IvAction::setShortcut(const char* keys)
{
    // Qt style: optional "Ctrl+", "Shift+", "Alt+" prefixes, then a key
    // name or a single character.
    string_view k(keys);
    int mods = 0;
    for (bool found = true; found;) {
        found = false;
        if (Strutil::parse_prefix(k, "Ctrl+"))
            mods |= ImGuiMod_Ctrl, found = true;
        if (Strutil::parse_prefix(k, "Shift+"))
            mods |= ImGuiMod_Shift, found = true;
        if (Strutil::parse_prefix(k, "Alt+"))
            mods |= ImGuiMod_Alt, found = true;
    }
    m_shortcut = keys;
#ifdef __APPLE__
    m_shortcut = Strutil::replace(m_shortcut, "Ctrl+", "Cmd+");
#endif
    m_keychord = 0;
    m_char     = 0;
    int c      = k.size() == 1 ? k[0] : 0;
    if (k == "PgUp")
        m_keychord = mods | ImGuiKey_PageUp;
    else if (k == "PgDown")
        m_keychord = mods | ImGuiKey_PageDown;
    else if (k == "Delete")
        m_keychord = mods | ImGuiKey_Delete;
    else if (isalpha(c))
        m_keychord = mods | (ImGuiKey_A + (toupper(c) - 'A'));
    else if (!mods)
        m_char = c;  // punctuation and digits: match what was typed
    else if (isdigit(c))
        m_keychord = mods | (ImGuiKey_0 + (c - '0'));
    else if (c == ',')
        m_keychord = mods | ImGuiKey_Comma;
    else if (c == '-')
        m_keychord = mods | ImGuiKey_Minus;
    else if (c == '+')
        m_keychord = mods | ImGuiKey_Equal, m_shift_optional = true;
}



void
IvAction::setChecked(bool c)
{
    if (c == m_checked)
        return;
    m_checked = c;
    if (c && m_group)
        for (IvAction* other : m_group->m_actions)
            if (other != this)
                other->setChecked(false);
    if (!m_blocked)
        toggled(c);
}



void
IvAction::trigger()
{
    if (!m_enabled)
        return;
    // The checked member of an exclusive group stays checked.
    if (m_checkable && !(m_group && m_checked))
        setChecked(!m_checked);
    triggered(m_checked);
}



IvAction*
IvActionGroup::checkedAction() const
{
    for (IvAction* a : m_actions)
        if (a->m_checked)
            return a;
    return nullptr;
}



IvMenu::IvMenu(const char* title, void* /*parent*/)
    : m_title(strip_mnemonic(title))
{
}



void
IvMenu::draw()
{
    for (auto& item : m_items) {
        if (IvAction* a = item.action) {
            if (!a->m_visible)
                continue;
            ImGui::PushID(a);
            if (ImGui::MenuItem(a->m_text.c_str(),
                                a->m_shortcut.empty() ? nullptr
                                                      : a->m_shortcut.c_str(),
                                a->m_checkable && a->m_checked, a->m_enabled))
                g_queue.push_back([a] { a->trigger(); });
            ImGui::PopID();
        } else if (item.menu) {
            if (ImGui::BeginMenu(item.menu->m_title.c_str(),
                                 item.menu->m_enabled)) {
                item.menu->draw();
                ImGui::EndMenu();
            }
        } else {
            ImGui::Separator();
        }
    }
}



void
IvLabel::draw()
{
    ImGui::TextUnformatted(m_text.c_str());
    if (!m_tooltip.empty() && ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", m_tooltip.c_str());
}



void
IvProgressBar::draw()
{
    float f = float(m_value - m_min) / float(std::max(1, m_max - m_min));
    ImGui::ProgressBar(f, ImVec2(10 * ImGui::GetFontSize(), 0));
}



void
IvCheckBox::draw()
{
    ImGui::Checkbox(m_text.c_str(), &m_checked);
}



void
IvSpinBox::setValue(int v)
{
    v = std::clamp(v, m_min, m_max);
    if (v != m_value) {
        m_value = v;
        valueChanged(v);
    }
}



void
IvSpinBox::draw()
{
    int v = m_value;
    ImGui::PushID(this);
    ImGui::SetNextItemWidth(8 * ImGui::GetFontSize());
    if (ImGui::InputInt("", &v, m_step))
        setValue(v);
    if (!m_tooltip.empty() && ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", m_tooltip.c_str());
    if (!m_suffix.empty()) {
        ImGui::SameLine(0, 0);
        ImGui::TextUnformatted(m_suffix.c_str());
    }
    ImGui::PopID();
}



IvTimer::IvTimer()
{
    g_timers.push_back(this);
}



IvTimer::~IvTimer()
{
    g_timers.erase(std::find(g_timers.begin(), g_timers.end(), this));
}



void
IvTimer::start(int msec)
{
    m_interval = msec / 1000.0;
    m_deadline = glfwGetTime() + m_interval;
    m_active   = true;
    glfwPostEmptyEvent();
}



void
IvTimer::singleShot(int msec, void* /*context*/, std::function<void()> fn)
{
    IvTimer* t    = new IvTimer;  // deleted after it fires
    t->m_single   = true;
    t->timeout.fn = std::move(fn);
    t->start(msec);
}



IvSettings::IvSettings(const char* organization, const char* application)
{
#if defined(_WIN32)
    std::string dir = Sysutil::getenv("APPDATA");
#elif defined(__APPLE__)
    std::string dir = std::string(Sysutil::getenv("HOME"))
                      + "/Library/Application Support";
#else
    std::string dir = Sysutil::getenv("XDG_CONFIG_HOME");
    if (dir.empty())
        dir = std::string(Sysutil::getenv("HOME")) + "/.config";
#endif
    dir += std::string("/") + organization;
    Filesystem::create_directory(dir);
    m_filename = Strutil::format("{}/{}.ini", dir, application);
    std::string text;
    if (Filesystem::read_text_file(m_filename, text)) {
        for (string_view line : Strutil::splitsv(text, "\n")) {
            auto kv = Strutil::splitsv(line, "=", 2);
            if (kv.size() == 2)
                m_values[kv[0]].m_values.emplace_back(kv[1]);
        }
    }
}



IvSettings::~IvSettings()
{
    if (!m_dirty)
        return;
    std::string text;
    for (auto& kv : m_values)
        for (auto& v : kv.second.m_values)
            text += Strutil::format("{}={}\n", kv.first, v);
    Filesystem::write_text_file(m_filename, text);
}



IvVariant
IvSettings::value(const char* key, const IvVariant& def) const
{
    auto found = m_values.find(key);
    return found != m_values.end() ? found->second : def;
}



void
IvSettings::setValue(const char* key, const IvVariant& v)
{
    m_values[key] = v;
    m_dirty       = true;
}



IvApplication::IvApplication(int& /*argc*/, char** /*argv*/)
{
    glfwSetErrorCallback([](int, const char* msg) {
        OIIO::print(stderr, "iv: GLFW error: {}\n", msg);
    });
    if (!glfwInit()) {
        OIIO::print(stderr, "iv: could not initialize the window system\n");
        exit(EXIT_FAILURE);
    }
}



IvApplication::~IvApplication()
{
    if (g_mainwin && g_mainwin->m_window) {
        ImGui_ImplOpenGL2_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        glfwDestroyWindow(g_mainwin->m_window);
        g_mainwin->m_window = nullptr;
    }
    glfwTerminate();
}



int
IvApplication::exec()
{
    IvMainWindow* mw = g_mainwin;
    while (mw && !mw->m_closed) {
        if (g_frames_needed > 0 || (mw->m_central && mw->m_central->m_dirty)) {
            glfwPollEvents();
        } else {
            // Idle: sleep until there is input, a timer is due, or
            // something is posted from another thread.
            double timeout = 3600;
            for (IvTimer* t : g_timers)
                if (t->m_active)
                    timeout = std::min(timeout, t->m_deadline - glfwGetTime());
            glfwWaitEventsTimeout(std::max(timeout, 0.001));
            g_frames_needed = 3;
        }
        if (glfwWindowShouldClose(mw->m_window))
            mw->close();
        if (mw->m_closed)
            break;
        draw_frame();
        --g_frames_needed;
        run_queued();
    }
    if (mw) {
        IvCloseEvent ev;
        mw->closeEvent(&ev);
    }
    return 0;
}



void
IvApplication::processEvents()
{
    if (g_in_frame || !g_mainwin)
        return;
    glfwPollEvents();
    draw_frame();
}



IvMainWindow::IvMainWindow()
{
    g_mainwin = this;
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    // Default context hints: iv's rendering needs a compatibility (legacy)
    // OpenGL context, 2.1 or newer.
    m_window = glfwCreateWindow(640, 480, "iv", nullptr, nullptr);
    if (m_window) {
        glfwMakeContextCurrent(m_window);
        glfwSwapInterval(1);
    }
    if (!m_window || !gladLoadGL(glfwGetProcAddress) || !GLAD_GL_VERSION_2_1) {
        OIIO::print(stderr, "iv: could not create an OpenGL 2.1 window\n");
        exit(EXIT_FAILURE);
    }
    glfwSetDropCallback(m_window, [](GLFWwindow*, int n, const char** paths) {
        std::vector<std::string> files(paths, paths + n);
        g_queue.push_back([files] {
            static_cast<ImageViewer*>(g_mainwin)->add_dropped_files(files);
        });
        g_frames_needed = 3;
    });
    glfwSetWindowFocusCallback(m_window, [](GLFWwindow*, int focused) {
        if (!focused && g_mainwin->m_central)
            g_queue.push_back([] {
                IvFocusEvent ev;
                g_mainwin->m_central->focusOutEvent(&ev);
            });
    });

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    static std::string layout_file;
    {
        IvSettings settings("OpenImageIO", "iv");  // creates the directory
        layout_file = Filesystem::parent_path(settings.filename())
                      + "/iv_layout.ini";
    }
    io.IniFilename = layout_file.c_str();
    ImFontConfig cfg;
    cfg.FontDataOwnedByAtlas = false;
    io.Fonts->AddFontFromMemoryTTF((void*)iv_font_sans, iv_font_sans_size, 0,
                                   &cfg);
    g_font_mono = io.Fonts->AddFontFromMemoryTTF((void*)iv_font_mono,
                                                 iv_font_mono_size, 0, &cfg);
    float scale = ui_scale();
    iv_set_dark_style(g_dark);
    ImGui::GetStyle().ScaleAllSizes(scale);
    ImGui::GetStyle().FontSizeBase = base_font_size * scale;
    m_menubar.m_height = m_statusbar.m_height = int(base_font_size + 6);
    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL2_Init();
    resize(640, 480);
}



void
IvMainWindow::setWindowTitle(const char* title)
{
    glfwSetWindowTitle(m_window, title);
}



float
IvMainWindow::devicePixelRatio() const
{
    float xs = 1, ys = 1;
    glfwGetWindowContentScale(m_window, &xs, &ys);
    return xs;
}



void
IvMainWindow::resize(int w, int h)
{
    float s = ui_scale();
    glfwSetWindowSize(m_window, int(w * s), int((h + m_menubar.height()) * s));
    g_frames_needed = 3;
}



IvPoint
IvMainWindow::pos() const
{
    int x, y;
    glfwGetWindowPos(m_window, &x, &y);
    return IvPoint(int(x / ui_scale()), int(y / ui_scale()));
}



void
IvMainWindow::move(int x, int y)
{
    float s = ui_scale();
    glfwSetWindowPos(m_window, int(x * s), int(y * s));
}



void
IvMainWindow::show()
{
    glfwShowWindow(m_window);
}



bool
IvMainWindow::close()
{
    m_closed = true;
    glfwPostEmptyEvent();
    return true;
}



void
IvMainWindow::raise()
{
    glfwFocusWindow(m_window);
}



void
IvMainWindow::showFullScreen()
{
    glfwGetWindowPos(m_window, &m_saved_geom[0], &m_saved_geom[1]);
    glfwGetWindowSize(m_window, &m_saved_geom[2], &m_saved_geom[3]);
    GLFWmonitor* monitor    = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = monitor ? glfwGetVideoMode(monitor) : nullptr;
    if (mode)
        glfwSetWindowMonitor(m_window, monitor, 0, 0, mode->width, mode->height,
                             mode->refreshRate);
    g_frames_needed = 3;
}



void
IvMainWindow::showNormal()
{
    glfwSetWindowMonitor(m_window, nullptr, m_saved_geom[0], m_saved_geom[1],
                         m_saved_geom[2], m_saved_geom[3], 0);
    g_frames_needed = 3;
}



IvGLWidget::IvGLWidget(IvWidget* parent)
    : m_mainwin(static_cast<IvMainWindow*>(parent))
{
}



void
IvGLWidget::draw_overlay_text(float x, float y, const std::string& s,
                              const IvColor& color)
{
    float size = mono_font_size * m_ui_scale;
    // Start on a whole framebuffer pixel, or the glyphs come out blurred.
    float fb = ImGui::GetIO().DisplayFramebufferScale.y;
    ImVec2 pos(std::floor((m_x0 + x * m_ui_scale) * fb + 0.5f) / fb,
               std::floor((m_y0 + y * m_ui_scale - 0.8f * size) * fb + 0.5f)
                   / fb);
    // Opaque: translucent thin glyphs over an image are hard to read.
    ImGui::GetBackgroundDrawList()->AddText(g_font_mono, size, pos,
                                            IM_COL32(color.r, color.g, color.b,
                                                     255),
                                            s.c_str());
}



void
IvGLWidget::draw_overlay_line(float x1, float y1, float x2, float y2,
                              const IvColor& color)
{
    float s = m_ui_scale;
    ImGui::GetBackgroundDrawList()->AddLine(
        ImVec2(m_x0 + x1 * s, m_y0 + y1 * s),
        ImVec2(m_x0 + x2 * s, m_y0 + y2 * s),
        IM_COL32(color.r, color.g, color.b, color.a), s);
}



IvPanel::IvPanel(const char* title) : m_title(title)
{
    m_visible = false;
    g_panels.push_back(this);
}



IvPanel::~IvPanel()
{
    g_panels.erase(std::find(g_panels.begin(), g_panels.end(), this));
}



void
IvPanel::draw()
{
    if (!m_visible)
        return;
    // "###" keeps the panel's identity when its title changes.
    std::string label = Strutil::format("{}###{}", m_title, (void*)this);
    float em          = ImGui::GetFontSize();
    ImGui::SetNextWindowSize(ImVec2(32 * em, 24 * em), ImGuiCond_FirstUseEver);
    if (ImGui::Begin(label.c_str(), &m_visible))
        draw_contents();
    ImGui::End();
}



IvInfoWindow::IvInfoWindow(ImageViewer& /*viewer*/, bool /*visible*/)
    : IvPanel("Image Info")
{
}



void
IvInfoWindow::update(IvImage* img)
{
    m_info  = img ? img->longinfo() : std::string();
    m_title = img ? Strutil::format("{} - iv Info", img->name()) : "iv Info";
}



void
IvInfoWindow::draw_contents()
{
    if (m_info.empty()) {
        ImGui::TextUnformatted("No image loaded.");
        return;
    }
    // m_info is an HTML table: <tr><td><i>name</i> ...</td><td>value</td></tr>
    if (!ImGui::BeginTable("info", 2))
        return;
    ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch);
    string_view rows(m_info);
    auto skip_past = [&rows](string_view marker) {
        size_t pos         = rows.find(marker);
        string_view before = rows.substr(0, pos);
        rows = pos == string_view::npos ? string_view()
                                        : rows.substr(pos + marker.size());
        return before;
    };
    for (skip_past("<i>"); !rows.empty(); skip_past("<i>")) {
        std::string name(skip_past("</i>"));
        skip_past("</td><td>");
        std::string value(skip_past("</td></tr>"));
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextDisabled("%s", name.c_str());
        ImGui::TableNextColumn();
        ImGui::TextWrapped("%s", value.c_str());
    }
    ImGui::EndTable();
}



IvPreferenceWindow::IvPreferenceWindow(ImageViewer& viewer)
    : IvPanel("iv Preferences"), m_viewer(viewer)
{
}



void
IvPreferenceWindow::draw_contents()
{
    ImageViewer& v = m_viewer;
    auto row       = [](std::initializer_list<IvWidget*> widgets) {
        ImGui::AlignTextToFramePadding();
        for (IvWidget* w : widgets) {
            w->draw();
            ImGui::SameLine();
        }
        ImGui::NewLine();
    };
    v.pixelviewFollowsMouseBox->draw();
    row({ v.closeupPixelsLabel, v.closeupPixelsBox, v.closeupAvgPixelsLabel,
          v.closeupAvgPixelsBox });
    v.linearInterpolationBox->draw();
    v.darkPaletteBox->draw();
    iv_set_dark_style(v.darkPaletteBox->isChecked());
    v.autoMipmap->draw();
    row({ v.maxMemoryICLabel, v.maxMemoryIC });
    row({ v.slideShowDurationLabel, v.slideShowDuration });
}



namespace {

// Read-only scrolling text, for iv_text_dialog().
class TextPanel final : public IvPanel {
public:
    TextPanel() : IvPanel("") {}
    void draw_contents() override
    {
        ImGui::InputTextMultiline("##text", m_text.data(), m_text.size() + 1,
                                  ImVec2(-1, -1), ImGuiInputTextFlags_ReadOnly);
    }
    std::string m_text;
};

}  // namespace



void
iv_set_dark_style(bool dark)
{
    g_dark = dark;
    if (!ImGui::GetCurrentContext())
        return;
    // Switching color sets leaves sizes (already scaled) alone.
    if (dark)
        ImGui::StyleColorsDark();
    else
        ImGui::StyleColorsLight();
}



static std::string g_filter_desc;
static std::vector<std::string> g_filter_strs;
static std::vector<const char*> g_filter_pats;

// Set the tinyfiledialogs filter arguments from the first entry of a Qt
// filter string ("Images (*.a *.b);;Other (*.c)"). Return false, after
// telling the user, if no native dialog can be shown (Linux without zenity,
// kdialog or similar).
static bool
native_dialogs(const char* qt_filters = "")
{
    string_view first(qt_filters);
    g_filter_desc = Strutil::strip(Strutil::parse_until(first, "("));
    g_filter_strs.clear();
    if (Strutil::parse_char(first, '('))
        g_filter_strs = Strutil::splits(Strutil::parse_until(first, ")"), " ");
    g_filter_pats.clear();
    for (auto& p : g_filter_strs)
        g_filter_pats.push_back(p.c_str());
    g_frames_needed = 3;
    if (tinyfd_openFileDialog("tinyfd_query", nullptr, 0, nullptr, nullptr, 0))
        return true;
    iv_text_dialog(nullptr, "iv",
                   "No program to show file dialogs and messages was found.\n"
                   "Install zenity, kdialog or yad.");
    return false;
}



std::vector<std::string>
iv_open_files(IvMainWindow* /*parent*/, const char* title, std::string& dir,
              const char* filters)
{
    std::vector<std::string> names;
    if (!native_dialogs(filters ? filters : ""))
        return names;
    std::string start = dir.empty() ? std::string() : dir + "/";
    const char* r     = tinyfd_openFileDialog(title, start.c_str(),
                                              int(g_filter_pats.size()),
                                              g_filter_pats.data(),
                                              g_filter_desc.c_str(), 1);
    if (r)
        names = Strutil::splits(r, "|");
    if (!names.empty())
        dir = Filesystem::parent_path(names[0]);
    return names;
}



std::string
iv_save_file(IvMainWindow* /*parent*/, const char* title,
             const std::string& suggested, const char* filters)
{
    if (!native_dialogs(filters ? filters : ""))
        return "";
    const char* r = tinyfd_saveFileDialog(title, suggested.c_str(),
                                          int(g_filter_pats.size()),
                                          g_filter_pats.data(),
                                          g_filter_desc.c_str());
    return r ? r : "";
}



void
iv_message(IvMainWindow* /*parent*/, const char* title, const std::string& text)
{
    if (native_dialogs())
        tinyfd_messageBox(*title ? title : "iv", text.c_str(), "ok", "info", 1);
    else
        iv_text_dialog(nullptr, title, text);
}



bool
iv_confirm(IvMainWindow* /*parent*/, const char* title, const std::string& text)
{
    return native_dialogs()
           && tinyfd_messageBox(*title ? title : "iv", text.c_str(), "yesno",
                                "question", 0)
                  == 1;
}



void
iv_text_dialog(IvMainWindow* /*parent*/, const char* title,
               const std::string& text)
{
    static TextPanel* panel = new TextPanel;
    panel->m_title          = title;
    panel->m_text           = text;
    panel->show();
}



void
iv_post(IvMainWindow* /*context*/, std::function<void()> fn)
{
    {
        std::lock_guard<std::mutex> lock(g_posted_mutex);
        g_posted.push_back(std::move(fn));
    }
    glfwPostEmptyEvent();
}



void
iv_available_screen(IvMainWindow* /*w*/, int& x, int& y, int& width,
                    int& height)
{
    x = y = 0, width = 1920, height = 1080;  // if there is no monitor
    if (GLFWmonitor* monitor = glfwGetPrimaryMonitor())
        glfwGetMonitorWorkarea(monitor, &x, &y, &width, &height);
    for (int* v : { &x, &y, &width, &height })
        *v = int(*v / ui_scale());
}



bool
iv_launch(const std::vector<std::string>& args)
{
    std::string program = Sysutil::this_program_path();
    std::vector<char*> argv { const_cast<char*>(program.c_str()) };
    for (auto& a : args)
        argv.push_back(const_cast<char*>(a.c_str()));
    argv.push_back(nullptr);
#ifdef _WIN32
    return _spawnv(_P_NOWAIT, program.c_str(), argv.data()) != -1;
#else
    pid_t pid;
    return posix_spawn(&pid, program.c_str(), nullptr, nullptr, argv.data(),
                       environ)
           == 0;
#endif
}
