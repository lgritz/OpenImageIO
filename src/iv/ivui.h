// Copyright Contributors to the OpenImageIO project.
// SPDX-License-Identifier: Apache-2.0
// https://github.com/AcademySoftwareFoundation/OpenImageIO

// Backend-neutral names for the GUI toolkit types and calls that iv uses.
// This is the only header that knows which GUI backend is being built; the
// rest of iv refers to the Iv* names declared here.

#pragma once

#include <functional>
#include <string>
#include <vector>

#include <OpenImageIO/platform.h>

#ifdef IV_USE_QT

#    ifndef OIIO_QT_MAJOR
#        error "Build problem? OIIO_QT_MAJOR not defined."
#    endif

#    include <QAction>
#    include <QActionGroup>
#    include <QApplication>
#    include <QCheckBox>
#    include <QCloseEvent>
#    include <QComboBox>
#    include <QKeyEvent>
#    include <QLabel>
#    include <QMainWindow>
#    include <QMenu>
#    include <QMenuBar>
#    include <QMouseEvent>
#    include <QOpenGLExtraFunctions>
#    include <QOpenGLWidget>
#    include <QProgressBar>
#    include <QResizeEvent>
#    include <QSettings>
#    include <QSignalBlocker>
#    include <QSpinBox>
#    include <QStatusBar>
#    include <QTimer>
#    include <QWheelEvent>

namespace Iv = Qt;

using IvAction        = QAction;
using IvActionGroup   = QActionGroup;
using IvApplication   = QApplication;
using IvCheckBox      = QCheckBox;
using IvCloseEvent    = QCloseEvent;
using IvColor         = QColor;
using IvComboBox      = QComboBox;
using IvEvent         = QEvent;
using IvFocusEvent    = QFocusEvent;
using IvKeyEvent      = QKeyEvent;
using IvLabel         = QLabel;
using IvMainWindow    = QMainWindow;
using IvMenu          = QMenu;
using IvMouseEvent    = QMouseEvent;
using IvPoint         = QPoint;
using IvProgressBar   = QProgressBar;
using IvResizeEvent   = QResizeEvent;
using IvSettings      = QSettings;
using IvSignalBlocker = QSignalBlocker;
using IvSpinBox       = QSpinBox;
using IvStringList    = QStringList;
using IvTimer         = QTimer;
using IvWheelEvent    = QWheelEvent;
using IvWidget        = QWidget;

// Base class of the widget that draws the image with OpenGL.
struct IvGLWidget : public QOpenGLWidget, protected QOpenGLExtraFunctions {
    explicit IvGLWidget(QWidget* parent) : QOpenGLWidget(parent) {}
};

// Qt 5 has overloads of QSpinBox::valueChanged.
#    define IV_SPIN_VALUE_CHANGED QOverload<int>::of(&QSpinBox::valueChanged)

inline std::string
iv_str(const QString& s)
{
    return s.toStdString();
}

#elif defined(IV_USE_IMGUI)

#    include "ivui_imgui.h"

#else
#    error "Build problem? Neither IV_USE_QT nor IV_USE_IMGUI is defined."
#endif



// Ask the user for one or more existing files to open. `filters` is a Qt
// style filter string ("Name (*.ext *.ext2);;Name2 (*.ext3)"). `dir` is the
// directory to start in (empty for the current directory) and is updated
// to the directory the user ended up in. Returns an empty vector if the
// user cancelled.
std::vector<std::string> iv_open_files(IvMainWindow* parent, const char* title,
                                       std::string& dir, const char* filters);

// Ask the user for a file name to save to. Returns "" if cancelled.
std::string iv_save_file(IvMainWindow* parent, const char* title,
                         const std::string& suggested,
                         const char* filters = nullptr);

// Show a message and wait for the user to dismiss it.
void iv_message(IvMainWindow* parent, const char* title,
                const std::string& text);

// Ask a yes/no question. Returns true for yes.
bool iv_confirm(IvMainWindow* parent, const char* title,
                const std::string& text);

// Show a block of read-only, scrollable text.
void iv_text_dialog(IvMainWindow* parent, const char* title,
                    const std::string& text);

// Run `fn` soon on the GUI thread. Safe to call from any thread. The call
// is dropped if `context` is destroyed first.
void iv_post(IvMainWindow* context, std::function<void()> fn);

// The part of the screen holding window `w` that is available to
// applications (excluding docks, task bars, etc.).
void iv_available_screen(IvMainWindow* w, int& x, int& y, int& width,
                         int& height);
