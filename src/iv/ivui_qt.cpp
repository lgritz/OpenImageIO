// Copyright Contributors to the OpenImageIO project.
// SPDX-License-Identifier: Apache-2.0
// https://github.com/AcademySoftwareFoundation/OpenImageIO

// Qt implementation of the helper functions declared in ivui.h.

#include "ivui.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QScreen>
#include <QTextEdit>
#include <QVBoxLayout>

#if OIIO_QT_MAJOR < 6
#    include <QDesktopWidget>
#endif



std::vector<std::string>
iv_open_files(IvMainWindow* /*parent*/, const char* title, std::string& dir,
              const char* filters)
{
    QString startdir = dir.empty() ? QDir::currentPath()
                                   : QString::fromStdString(dir);
    QFileDialog dialog(nullptr, title, startdir, filters);
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    dialog.setFileMode(QFileDialog::ExistingFiles);
    std::vector<std::string> names;
    if (!dialog.exec())
        return names;
    dir = dialog.directory().path().toStdString();
    for (auto& name : dialog.selectedFiles())
        names.emplace_back(name.toStdString());
    return names;
}



std::string
iv_save_file(IvMainWindow* parent, const char* title,
             const std::string& suggested, const char* filters)
{
    return QFileDialog::getSaveFileName(parent, title,
                                        QString::fromStdString(suggested),
                                        filters)
        .toStdString();
}



void
iv_message(IvMainWindow* parent, const char* title, const std::string& text)
{
    QMessageBox::information(parent, title, QString::fromStdString(text));
}



bool
iv_confirm(IvMainWindow* parent, const char* title, const std::string& text)
{
    return QMessageBox::question(parent, title, QString::fromStdString(text),
                                 QMessageBox::Yes | QMessageBox::No)
           == QMessageBox::Yes;
}



void
iv_text_dialog(IvMainWindow* parent, const char* title, const std::string& text)
{
    QDialog dialog(parent);
    dialog.setWindowTitle(title);
    auto* layout   = new QVBoxLayout(&dialog);
    auto* textedit = new QTextEdit(&dialog);
    textedit->setReadOnly(true);
    textedit->setPlainText(QString::fromStdString(text));
    layout->addWidget(textedit);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog,
                     &QDialog::reject);
    layout->addWidget(buttons);
    dialog.resize(640, 480);
    dialog.exec();
}



void
iv_post(IvMainWindow* context, std::function<void()> fn)
{
    QMetaObject::invokeMethod(context, std::move(fn), Qt::QueuedConnection);
}



void
iv_available_screen(IvMainWindow* w, int& x, int& y, int& width, int& height)
{
#if OIIO_QT_MAJOR >= 6
    QRect g = w->screen()->availableGeometry();
#else
    QRect g = QApplication::desktop()->availableGeometry(w);
#endif
    x      = g.x();
    y      = g.y();
    width  = g.width();
    height = g.height();
}
