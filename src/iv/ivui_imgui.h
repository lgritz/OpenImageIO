// Copyright Contributors to the OpenImageIO project.
// SPDX-License-Identifier: Apache-2.0
// https://github.com/AcademySoftwareFoundation/OpenImageIO

// Dear ImGui + GLFW implementation of the Iv* GUI types. Each class here
// provides only the members iv uses, with the names and behavior of the Qt
// class it stands in for. Include ivui.h, not this file.

#pragma once

#include <functional>
#include <map>
#include <string>
#include <type_traits>
#include <vector>

#include <glad/gl.h>

struct GLFWwindow;
class IvActionGroup;
class IvGLWidget;
class IvMenu;

#define Q_OBJECT
#define slots



namespace Iv {
enum MouseButton {
    NoButton     = 0,
    LeftButton   = 1,
    RightButton  = 2,
    MiddleButton = 4
};
enum KeyboardModifier {
    ShiftModifier   = 1,
    ControlModifier = 2,
    AltModifier     = 4
};
enum Key {
    Key_Minus      = '-',
    Key_Underscore = '_',
    Key_Plus       = '+',
    Key_Equal      = '=',
    Key_Escape     = 0x1000,
    Key_Left,
    Key_Right,
    Key_Up,
    Key_Down,
    Key_PageUp,
    Key_PageDown
};
enum CursorShape {
    ArrowCursor,
    CrossCursor,
    WaitCursor,
    BusyCursor,
    SplitHCursor,
    SplitVCursor
};
}  // namespace Iv



// A signal with at most one connected slot.
template<class... A> struct IvSignal {
    std::function<void(A...)> fn;
    void operator()(A... a) const
    {
        if (fn)
            fn(a...);
    }
};

// connect(sender, &Sender::signal, receiver, slot): slot may be a member
// function of the receiver or a callable, taking the signal's arguments or
// none.
template<class S, class C, class R, class F, class... A>
void
connect(S* sender, IvSignal<A...> C::* signal, R* receiver, F slot)
{
    auto& fn = (sender->*signal).fn;
    if constexpr (std::is_invocable_v<F, R*, A...>)
        fn = [=](A... a) { std::invoke(slot, receiver, a...); };
    else if constexpr (std::is_invocable_v<F, R*>)
        fn = [=](A...) { std::invoke(slot, receiver); };
    else if constexpr (std::is_invocable_v<F, A...>)
        fn = [=](A... a) { slot(a...); };
    else
        fn = [=](A...) { slot(); };
}



// connect(sender, &Sender::signal, callable)
template<class S, class C, class F, class... A>
void
connect(S* sender, IvSignal<A...> C::* signal, F slot)
{
    (sender->*signal).fn = slot;
}



struct IvPoint {
    int m_x = 0, m_y = 0;
    IvPoint() {}
    IvPoint(int x, int y) : m_x(x), m_y(y) {}
    int x() const { return m_x; }
    int y() const { return m_y; }
    IvPoint operator/(int d) const { return IvPoint(m_x / d, m_y / d); }
};

struct IvColor {
    int r, g, b, a;
    IvColor(int r = 0, int g = 0, int b = 0, int a = 255)
        : r(r), g(g), b(b), a(a)
    {
    }
    void setAlpha(int alpha) { a = alpha; }
};

struct IvStringList : std::vector<std::string> {
    void push_front(const char* s) { insert(begin(), s); }
};

inline const std::string&
iv_str(const std::string& s)
{
    return s;
}



struct IvEvent {
    void accept() {}
};
using IvCloseEvent  = IvEvent;
using IvFocusEvent  = IvEvent;
using IvResizeEvent = IvEvent;

struct IvMouseEvent : IvEvent {
    IvPoint m_pos;
    Iv::MouseButton m_button = Iv::NoButton;
    int m_modifiers          = 0;
    IvPoint pos() const { return m_pos; }
    Iv::MouseButton button() const { return m_button; }
    int modifiers() const { return m_modifiers; }
};

struct IvWheelEvent : IvEvent {
    IvPoint m_delta;
    IvPoint angleDelta() const { return m_delta; }
};

struct IvKeyEvent : IvEvent {
    int m_key = 0, m_modifiers = 0;
    int key() const { return m_key; }
    int modifiers() const { return m_modifiers; }
};



class IvAction {
public:
    IvAction(const char* text, void* parent = nullptr);
    explicit IvAction(void* parent) : IvAction("", parent) {}
    void setShortcut(const char* keys);
    void setCheckable(bool c) { m_checkable = c; }
    void setChecked(bool c);
    bool isChecked() const { return m_checked; }
    void setEnabled(bool e) { m_enabled = e; }
    void setVisible(bool v) { m_visible = v; }
    void setText(const char* text);
    const std::string& text() const { return m_text; }
    // Do what a click on the menu item does.
    void trigger();

    IvSignal<bool> triggered, toggled;

    std::string m_text, m_shortcut;
    int m_keychord        = 0;      // ImGuiKeyChord, or 0
    unsigned int m_char   = 0;      // typed character that triggers it, or 0
    bool m_shift_optional = false;  // chord matches with or without Shift
    bool m_checkable = false, m_checked = false;
    bool m_enabled = true, m_visible = true, m_blocked = false;
    IvActionGroup* m_group = nullptr;
};

class IvActionGroup {
public:
    explicit IvActionGroup(void* /*parent*/) {}
    void addAction(IvAction* a)
    {
        m_actions.push_back(a);
        a->m_group = this;
    }
    void setExclusive(bool) {}
    IvAction* checkedAction() const;
    std::vector<IvAction*> m_actions;
};

struct IvSignalBlocker {
    explicit IvSignalBlocker(IvAction* a) : m_action(a), m_was(a->m_blocked)
    {
        a->m_blocked = true;
    }
    ~IvSignalBlocker() { m_action->m_blocked = m_was; }
    IvAction* m_action;
    bool m_was;
};

class IvMenu {
public:
    IvMenu(const char* title, void* parent = nullptr);
    void addAction(IvAction* a) { m_items.push_back({ a, nullptr }); }
    void addMenu(IvMenu* m) { m_items.push_back({ nullptr, m }); }
    void addSeparator() { m_items.push_back({ nullptr, nullptr }); }
    void setEnabled(bool e) { m_enabled = e; }
    void draw();

    struct Item {
        IvAction* action;
        IvMenu* menu;
    };
    std::string m_title;
    std::vector<Item> m_items;
    bool m_enabled = true;
};



// Anything with show()/hide(). Subclasses that draw themselves in the status
// bar or a panel override draw().
class IvWidget {
public:
    virtual ~IvWidget() {}
    void show() { m_visible = true; }
    void hide() { m_visible = false; }
    bool isHidden() const { return !m_visible; }
    void setToolTip(const char* tip) { m_tooltip = tip; }
    virtual void draw() {}
    bool m_visible = true;
    std::string m_tooltip;
};

class IvLabel : public IvWidget {
public:
    IvLabel(const char* text = "") : m_text(text) {}
    void setText(const char* text) { m_text = text; }
    void draw() override;
    std::string m_text;
};

class IvProgressBar : public IvWidget {
public:
    void setRange(int lo, int hi) { m_min = lo, m_max = hi; }
    void setValue(int v) { m_value = v; }
    void reset() { m_value = m_min; }
    void draw() override;
    int m_min = 0, m_max = 100, m_value = 0;
};

class IvComboBox : public IvWidget {
public:
    void addItem(const char* item) { m_items.push_back(item); }
    int currentIndex() const { return m_current; }
    std::vector<std::string> m_items;
    int m_current = 0;
};

class IvCheckBox : public IvWidget {
public:
    IvCheckBox(const char* text) : m_text(text) {}
    void setChecked(bool c) { m_checked = c; }
    bool isChecked() const { return m_checked; }
    void draw() override;
    std::string m_text;
    bool m_checked = false;
};

class IvSpinBox : public IvWidget {
public:
    void setRange(int lo, int hi) { m_min = lo, m_max = hi; }
    void setValue(int v);
    int value() const { return m_value; }
    void setSingleStep(int s) { m_step = s; }
    void setSuffix(const char* s) { m_suffix = s; }
    void setAccelerated(bool) {}
    void draw() override;
    IvSignal<int> valueChanged;
    int m_min = 0, m_max = 99, m_value = 0, m_step = 1;
    std::string m_suffix;
};
#define IV_SPIN_VALUE_CHANGED &IvSpinBox::valueChanged

class IvMenuBar : public IvWidget {
public:
    void addMenu(IvMenu* m) { m_menus.push_back(m); }
    int height() const { return m_visible ? m_height : 0; }
    std::vector<IvMenu*> m_menus;
    int m_height = 0;  // logical pixels, measured when drawn
};

class IvStatusBar : public IvWidget {
public:
    void addWidget(IvWidget* w) { m_widgets.push_back(w); }
    int height() const { return m_visible ? m_height : 0; }
    std::vector<IvWidget*> m_widgets;
    int m_height = 0;  // logical pixels, measured when drawn
};



class IvTimer {
public:
    IvTimer();
    ~IvTimer();
    void start(int msec);
    void stop() { m_active = false; }
    bool isActive() const { return m_active; }
    static void singleShot(int msec, void* context, std::function<void()> fn);
    IvSignal<> timeout;
    double m_deadline = 0, m_interval = 0;  // seconds
    bool m_active = false, m_single = false;
};

inline void
disconnect(IvTimer* t, int, int, int)
{
    t->timeout.fn = nullptr;
}



// A value read from or written to IvSettings.
struct IvVariant {
    IvVariant() {}
    IvVariant(bool b) : m_values { b ? "true" : "false" } {}
    IvVariant(int i) : m_values { std::to_string(i) } {}
    IvVariant(const IvStringList& l) : m_values(l) {}
    bool toBool() const { return !m_values.empty() && m_values[0] == "true"; }
    int toInt() const
    {
        return m_values.empty() ? 0 : atoi(m_values[0].c_str());
    }
    IvStringList toStringList() const
    {
        IvStringList l;
        l.assign(m_values.begin(), m_values.end());
        return l;
    }
    std::vector<std::string> m_values;
};

// Persistent settings: a "key=value" text file in the per-user configuration
// directory. Read on construction, written on destruction if changed.
class IvSettings {
public:
    IvSettings(const char* organization, const char* application);
    ~IvSettings();
    IvVariant value(const char* key, const IvVariant& def = IvVariant()) const;
    void setValue(const char* key, const IvVariant& v);
    const std::string& filename() const { return m_filename; }

private:
    std::string m_filename;
    std::map<std::string, IvVariant> m_values;
    bool m_dirty = false;
};



class IvApplication {
public:
    IvApplication(int& argc, char** argv);
    ~IvApplication();
    int exec();
    // Handle pending input and redraw, without running queued actions.
    static void processEvents();
    static void setOverrideCursor(Iv::CursorShape c) { s_cursors.push_back(c); }
    static void restoreOverrideCursor() { s_cursors.pop_back(); }
    static bool overrideCursor() { return !s_cursors.empty(); }
    static inline std::vector<Iv::CursorShape> s_cursors;
};

class IvMainWindow : public IvWidget {
public:
    IvMainWindow();
    static const char* tr(const char* s) { return s; }
    IvMenuBar* menuBar() { return &m_menubar; }
    IvStatusBar* statusBar() { return &m_statusbar; }
    void setCentralWidget(IvGLWidget* w) { m_central = w; }
    void setWindowTitle(const char* title);
    // Size of the window contents below the menu bar, in logical pixels.
    void resize(int w, int h);
    // Top left corner of the window, in logical pixels.
    IvPoint pos() const;
    void move(int x, int y);
    int x() const { return pos().x(); }
    int y() const { return pos().y(); }
    void show();
    bool close();
    void raise();
    void activateWindow() {}
    void showFullScreen();
    void showNormal();
    void setCursor(Iv::CursorShape c) { m_cursor = c; }
    void unsetCursor() { m_cursor = Iv::ArrowCursor; }
    float devicePixelRatio() const;

    virtual void keyPressEvent(IvKeyEvent*) {}
    virtual void resizeEvent(IvResizeEvent*) {}
    virtual void closeEvent(IvCloseEvent*) {}

    GLFWwindow* m_window = nullptr;
    IvMenuBar m_menubar;
    IvStatusBar m_statusbar;
    IvGLWidget* m_central    = nullptr;
    Iv::CursorShape m_cursor = Iv::ArrowCursor;
    int m_saved_geom[4]      = { 0, 0, 0,
                                 0 };  // window pos, size before full screen
    bool m_closed            = false;
};

// Base class of the widget that draws the image with OpenGL. Sizes and
// mouse positions are in logical pixels relative to the image area, as Qt
// reports them.
class IvGLWidget : public IvWidget {
public:
    explicit IvGLWidget(IvWidget* parent);
    int width() const { return m_w; }
    int height() const { return m_h; }
    float devicePixelRatio() const { return m_dpr; }
    void update() { m_dirty = true; }
    void resize(int, int) {}
    void makeCurrent() {}
    void doneCurrent() {}
    void initializeOpenGLFunctions() {}
    void setCursor(Iv::CursorShape c) { m_cursor = c; }
    void unsetCursor() { m_cursor = Iv::ArrowCursor; }
    // Draw over the image, in logical pixels from the top left of the image
    // area. For text, y is the baseline.
    void draw_overlay_text(float x, float y, const std::string& s,
                           const IvColor& color);
    void draw_overlay_line(float x1, float y1, float x2, float y2,
                           const IvColor& color);

    virtual void initializeGL() {}
    virtual void resizeGL(int, int) {}
    virtual void paintGL() {}
    virtual void mousePressEvent(IvMouseEvent*) {}
    virtual void mouseReleaseEvent(IvMouseEvent*) {}
    virtual void mouseMoveEvent(IvMouseEvent*) {}
    virtual void wheelEvent(IvWheelEvent*) {}
    virtual void focusOutEvent(IvFocusEvent*) {}
    virtual void leaveEvent(IvEvent*) {}

    IvMainWindow* m_mainwin;
    int m_w = 1, m_h = 1;          // logical size of the image area
    float m_x0 = 0, m_y0 = 0;      // its top left corner, in ImGui coordinates
    float m_dpr              = 1;  // framebuffer pixels per logical pixel
    float m_ui_scale         = 1;  // ImGui units per logical pixel
    bool m_dirty             = true;
    bool m_initialized       = false;
    Iv::CursorShape m_cursor = Iv::ArrowCursor;
};



// A floating panel drawn inside the main window. Subclasses override
// draw_contents().
class IvPanel : public IvWidget {
public:
    IvPanel(const char* title);
    ~IvPanel() override;
    void setWindowTitle(const char* title) { m_title = title; }
    void draw() override;
    virtual void draw_contents() = 0;
    std::string m_title;
};

// Use the dark or light color theme.
void iv_set_dark_style(bool dark);

// Start another iv process with the given command line arguments.
bool iv_launch(const std::vector<std::string>& args);
