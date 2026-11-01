// SPDX-FileCopyrightText: claudemods
// SPDX-License-Identifier: MIT
//
// TaskPanel: runs one root task of the Backend and shows it: title, progress (build steps, the
// percentage of mksquashfs / mkfs.erofs, or bytes written by dd), elapsed time, live console, Cancel.

#pragma once

#include <QElapsedTimer>
#include <QWidget>

class AnsiLog;
class Backend;
class QLabel;
class QProgressBar;
class QPushButton;
class QTimer;

class TaskPanel : public QWidget
{
    Q_OBJECT
public:
    TaskPanel( Backend* backend, QWidget* parent = nullptr );

    // stepNames: the build plan, for a progress bar over the steps (empty = no steps).
    // expectedBytes: size of what dd writes, for its progress (0 = none).
    bool start( const QString& title,
                const QStringList& args,
                bool cancellable,
                const QStringList& stepNames = {},
                qint64 expectedBytes = 0,
                const QByteArray& stdinData = {} );
    bool isActive() const { return m_active; }
    AnsiLog* log() const { return m_log; }

Q_SIGNALS:
    void stepStarted( int index, const QString& name );   // 0-based index into stepNames
    void finished( bool ok, bool cancelled );

private:
    void onOutput( const QString& text );
    void onStep( int number, int total, const QString& name );
    void onFinished( bool ok, bool cancelled, const QString& message );
    void updateProgress();
    void tick();

    Backend* m_backend;
    AnsiLog* m_log;
    QLabel* m_title;
    QLabel* m_state;
    QLabel* m_elapsed;
    QProgressBar* m_progress;
    QPushButton* m_cancel;
    QTimer* m_timer;
    QElapsedTimer m_clock;

    bool m_active = false;
    QStringList m_steps;
    int m_step = 0;    // 1-based current step, 0 = none yet
    int m_total = 0;
    qint64 m_expectedBytes = 0;
    qint64 m_bytes = 0;
};
