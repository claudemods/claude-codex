// SPDX-FileCopyrightText: claudemods
// SPDX-License-Identifier: MIT
//
// Claude Codex Cloner - the Backup tab: clone this running system into a bootable live ISO.
// A sidebar of pages:
//   Overview      what is ready on this system, with fixes, and quick actions
//   Build ISO     the build steps, progress and live log (or build it from init on the next boot)
//   Settings      ISO name, directories, image type, compression, boot options
//   Edit Configs  GRUB / boot text / initramfs files and the Claude Codex Install configuration
//   Write USB     put an ISO on a drive (also the ISOs made on the Containers tab)
//   Install       in the live ISO: Claude Codex Install (or the Alpine installer) puts the clone on a disk
//   Maintenance   mkinitcpio config, log, settings folder
// The distro guide is on the window's Guide tab, updates on its Updates tab.

#pragma once

#include "Backend.h"

#include <QWidget>

class QCheckBox;
class QComboBox;
class QFormLayout;
class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QPlainTextEdit;
class QPushButton;
class QSpinBox;
class QStackedWidget;
class QVBoxLayout;
class TaskPanel;

class BackupView : public QWidget
{
    Q_OBJECT
public:
    explicit BackupView( QWidget* parent = nullptr );

    bool confirmClose();                        // the window is closing: false keeps it open
    bool isBusy() const { return m_backend->isRunning(); }
    void showWriteUsb( const QString& iso );    // Write USB, with this ISO selected
    QString guideText() const;                  // the cloner's guide for this distro

Q_SIGNALS:
    void updatesRequested();                    // "Open Updates" on the Maintenance page

private:
    enum Page
    {
        Overview,
        Build,
        Settings,
        Configs,
        Usb,
        Install,
        Maintenance
    };

    QWidget* makeOverviewPage();
    QWidget* makeBuildPage();
    QWidget* makeSettingsPage();
    QWidget* makeConfigsPage();
    QWidget* makeUsbPage();
    QWidget* makeInstallPage();
    QWidget* makeMaintenancePage();

    void refreshStatus();
    void fillOverview();
    void fillBuild();
    void fillSettings();
    void fillConfigs();
    void fillInstall();
    void refreshIsos();
    void refreshDrives( QComboBox* combo, bool removableFirst );

    void goTo( Page page );
    void startBuild();
    void toggleBootBuild();
    void saveSettings();
    void loadConfigItem( QListWidgetItem* item );
    bool confirmDiscardEdits();
    void saveConfigItem();
    void writeUsb();
    void launchInstaller();
    void installAlpine();

    QString isoOutputDir() const;
    QString humanSize( qint64 bytes ) const;
    bool isAlpine() const { return m_status.family() == QLatin1String( "alpine" ); }
    bool isArch() const { return m_status.family() == QLatin1String( "archlinux" ); }

    Backend* m_backend;
    ClonerStatus m_status;
    QList< PlanStep > m_plan;

    QListWidget* m_nav = nullptr;
    QStackedWidget* m_pages = nullptr;
    QLabel* m_statusBadge = nullptr;

    // Overview
    QLabel* m_distroTitle = nullptr;
    QLabel* m_distroSub = nullptr;
    QVBoxLayout* m_checks = nullptr;

    // Build
    QLabel* m_buildSummary = nullptr;
    QListWidget* m_stepList = nullptr;
    TaskPanel* m_buildTask = nullptr;
    QPushButton* m_buildButton = nullptr;
    QPushButton* m_bootBuild = nullptr;     // build on the next boot, from init (or cancel it)
    bool m_buildIsIso = false;              // the build task is an ISO build (not setting up the boot build)
    QPushButton* m_openOutput = nullptr;
    QPushButton* m_toUsb = nullptr;

    // Settings
    QFormLayout* m_form = nullptr;
    QLineEdit* m_isoName = nullptr;
    QLineEdit* m_workDir = nullptr;
    QLineEdit* m_outputDir = nullptr;
    QLineEdit* m_cloneDir = nullptr;
    QLineEdit* m_isoTag = nullptr;
    QComboBox* m_imageFormat = nullptr;
    QSpinBox* m_level = nullptr;
    QCheckBox* m_ramMode = nullptr;
    QCheckBox* m_cleanCache = nullptr;
    QLabel* m_settingsNote = nullptr;
    bool m_settingsDirty = false;

    // Edit Configs
    QListWidget* m_configList = nullptr;
    QPlainTextEdit* m_editor = nullptr;
    QLabel* m_configPath = nullptr;
    QPushButton* m_configSave = nullptr;
    QPushButton* m_configReload = nullptr;
    QPushButton* m_configReset = nullptr;
    QPushButton* m_configCreate = nullptr;
    TaskPanel* m_configTask = nullptr;
    QListWidgetItem* m_configCurrent = nullptr;
    bool m_editorDirty = false;
    QStringList m_seedFailed;   // boot/initramfs files that could not be generated (shows "Try Again")
    QStringList m_previewed;    // files whose generated version was fetched this session

    // Write USB
    QComboBox* m_isoCombo = nullptr;
    QComboBox* m_driveCombo = nullptr;
    TaskPanel* m_usbTask = nullptr;

    // Install
    QStackedWidget* m_installStack = nullptr;
    QLabel* m_installerState = nullptr;
    QPushButton* m_launchInstaller = nullptr;
    QComboBox* m_alpineDisk = nullptr;
    QComboBox* m_alpineFs = nullptr;
    TaskPanel* m_installTask = nullptr;

    // Maintenance
    QLabel* m_versionLabel = nullptr;
    QPushButton* m_mkinitcpio = nullptr;
    TaskPanel* m_maintTask = nullptr;
};
