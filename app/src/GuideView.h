// SPDX-FileCopyrightText: claudemods
// SPDX-License-Identifier: MIT
//
// Claude Codex Cloner - the Guide tab: how to use every part of the suite, step by step, and the
// cloner's own notes for this distro (claudebackup-linux --guide).

#pragma once

#include <QWidget>

class BackupView;
class QListWidget;
class QTextBrowser;

class GuideView : public QWidget
{
    Q_OBJECT
public:
    explicit GuideView( BackupView* backup, QWidget* parent = nullptr );

    void loadDistroNotes();   // reads the cloner's guide the first time the tab is opened

private:
    void showSection( int section );

    BackupView* m_backup;
    QListWidget* m_nav = nullptr;
    QTextBrowser* m_text = nullptr;
    QString m_distroNotes;
    bool m_notesLoaded = false;
};
