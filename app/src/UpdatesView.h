// SPDX-FileCopyrightText: claudemods
// SPDX-License-Identifier: MIT
//
// Claude Codex Cloner - the Updates tab: one updater for the whole suite. It shows what is installed
// (`claude-codex status --plain`), asks GitHub for the latest release (`claude-codex update --check`)
// and updates everything with `claude-codex update --yes` through pkexec: the new source is downloaded
// and the compendium's build.sh rebuilds and reinstalls every tool with the choices of the last build.

#pragma once

#include <QElapsedTimer>
#include <QProcess>
#include <QWidget>

class AnsiLog;
class QCheckBox;
class QGridLayout;
class QLabel;
class QProgressBar;
class QPushButton;
class QTimer;

class UpdatesView : public QWidget
{
    Q_OBJECT
public:
    explicit UpdatesView( QWidget* parent = nullptr );

    static QString suiteCommand();   // /usr/local/bin/claude-codex (or on PATH)
    bool confirmClose();

public Q_SLOTS:
    void checkQuietly();             // background check at start: only the header and this page change

Q_SIGNALS:
    void checked( bool ok, bool newer, const QString& latest );

private:
    void refreshInstalled();
    void check( bool quiet );
    void startUpdate();
    void stopUpdate();
    void readOutput();
    void updateFinished( int code, QProcess::ExitStatus status );
    void setBusy( bool busy );
    void tick();

    QLabel* m_release = nullptr;
    QLabel* m_details = nullptr;
    QGridLayout* m_tools = nullptr;
    QLabel* m_state = nullptr;
    QPushButton* m_check = nullptr;
    QPushButton* m_update = nullptr;
    QCheckBox* m_force = nullptr;
    QLabel* m_phase = nullptr;
    QLabel* m_elapsed = nullptr;
    QProgressBar* m_bar = nullptr;
    QPushButton* m_cancel = nullptr;
    AnsiLog* m_log = nullptr;
    QTimer* m_timer = nullptr;
    QElapsedTimer m_clock;

    QProcess* m_process = nullptr;
    QProcess* m_checker = nullptr;
    QByteArray m_buffer;
    bool m_cancelled = false;
};
