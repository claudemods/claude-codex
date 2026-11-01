// SPDX-FileCopyrightText: claudemods
// SPDX-License-Identifier: MIT
//
// Claude Codex Cloner: the window. A header (brand, release, what is installed) over four tabs:
//   Backup      clone this running system into a bootable live ISO, write it to USB, install it
//   Containers  Claude Codex CCU: clone it into an Incus container, install / export / ISO of a container
//   Updates     the version of every Claude Codex tool, check GitHub, update the whole suite
//   Guide       how to use everything, and the cloner's notes for this distro
// Each feature lives on one tab only: USB sticks are written on Backup, updates happen on Updates.

#pragma once

#include <QMainWindow>

class BackupView;
class CcuView;
class GuideView;
class QLabel;
class QTabWidget;
class UpdatesView;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow( QWidget* parent = nullptr );

protected:
    void closeEvent( QCloseEvent* event ) override;

private:
    enum Tab
    {
        BackupTab,
        ContainersTab,
        UpdatesTab,
        GuideTab
    };

    QWidget* makeHeader();
    QWidget* makeCcuMissing();
    void setPill( QLabel* pill, const QString& state, const QString& text );

    QTabWidget* m_tabs = nullptr;
    BackupView* m_backup = nullptr;
    CcuView* m_ccu = nullptr;   // null when Claude Codex CCU is not installed
    UpdatesView* m_updates = nullptr;
    GuideView* m_guide = nullptr;
    QLabel* m_ccuPill = nullptr;
    QLabel* m_updatePill = nullptr;
};
