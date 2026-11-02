// SPDX-FileCopyrightText: claudemods
// SPDX-License-Identifier: MIT
//
// LogWindow: a task's log in its own full-screen window, following it live (colours, progress bars
// redrawn in place). Esc or Close shuts it; the task keeps running.

#pragma once

#include <QPointer>
#include <QWidget>

class QPlainTextEdit;
class QTimer;

class LogWindow : public QWidget
{
    Q_OBJECT
public:
    // Opens a full-screen window that mirrors `source`
    static void open( QPlainTextEdit* source, const QString& title );

private:
    LogWindow( QPlainTextEdit* source, const QString& title );
    void sync();

    QPointer< QPlainTextEdit > m_source;
    QPlainTextEdit* m_view = nullptr;
    QTimer* m_timer = nullptr;
    bool m_dirty = true;
};
