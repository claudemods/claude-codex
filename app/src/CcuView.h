// SPDX-FileCopyrightText: claudemods
// SPDX-License-Identifier: MIT
//
// Claude Codex Cloner - the Containers tab: Claude Codex CCU (the Container Creator Utility).
// It does not duplicate the tool: it collects the settings, runs the `cccu` command line tool with them
// (unattended) and shows its progress and log live. The window runs as your user; each task asks for the
// administrator password once, for the whole window (RootSession; passwords for the new users go through stdin,
// never the command line). Writing an ISO to USB is done on the Backup tab (Write USB), so it is not repeated here.

#pragma once

#include "RootSession.h"

#include <QByteArray>
#include <QList>
#include <QPointer>
#include <QProcess>
#include <QWidget>

#include <array>
#include <functional>
#include <vector>

class QCheckBox;
class QComboBox;
class QGridLayout;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QProgressBar;
class QPushButton;
class QSpinBox;
class QTabWidget;
class QTableWidget;

class CcuView : public QWidget {
    Q_OBJECT
public:
    explicit CcuView(QWidget* parent = nullptr);

    static QString cccuPath();     // $CLAUDE_CODEX_CCU, cccu on PATH, or /usr/local/bin/cccu
    static bool available();       // cccu is installed
    bool isBusy() const { return running_; }
    bool confirmClose();           // the window is closing: false keeps it open

Q_SIGNALS:
    void writeUsbRequested(const QString& iso);   // open Backup > Write USB (with this ISO, when known)

private:
    enum Mode { Install = 0, Running = 1, Export = 2, Iso = 3, Containers = 4 };
    static constexpr int kModes = 5;

    struct Field {
        QLabel* badge;
        bool required;
        std::function<bool()> isSet;
    };
    struct PageData {
        QGridLayout* grid = nullptr;
        int cell = 0;                  // next free cell of the two-column grid
        std::vector<Field> fields;
    };
    // Settings that Install and Running have in common.
    struct Common {
        QLineEdit* user = nullptr;
        QLineEdit* pass = nullptr;
        QLineEdit* containerUser = nullptr;
        QLineEdit* name = nullptr;
        QSpinBox* size = nullptr;
        QComboBox* compress = nullptr;
        QCheckBox* immutable = nullptr;
        QComboBox* display = nullptr;
    };

    QWidget* buildPage(Mode mode);
    QWidget* buildIsoPage();
    QWidget* buildContainersPage();
    QWidget* card(const QString& title, const QString& text, Mode mode);
    void buildCommon(Mode mode);
    QWidget* addRow(Mode mode, const QString& label, QWidget* shown, const QList<QWidget*>& watched, bool required,
                    std::function<bool()> isSet, bool wide = false);
    QWidget* fileRow(QLineEdit* edit, bool save, const QString& filter, bool folder = false);
    void refreshDrives(QComboBox* box);
    void refreshPartitions(QComboBox* box);
    void refreshContainers(bool askPassword);
    void listContainers(bool elevated, bool askPassword);
    void fillContainers(const QString& plainOutput);
    int missing(Mode mode) const;
    void refreshStatus();
    QStringList buildArgs(Mode mode) const;
    QString selectedContainer() const;

    void start();
    void runCli(const QStringList& args, Mode mode, const QString& title);
    void cancel();
    void stopTool();
    void readOutput(const QByteArray& data);
    void handleLine(const QString& line);
    void finished(int code, QProcess::ExitStatus status);
    void setRunning(bool running);
    void appendLog(const QString& text, const QString& color, bool bold = false);
    Mode currentMode() const;

    QTabWidget* tabs_ = nullptr;
    std::array<PageData, kModes> data_;
    std::array<Common, 2> common_;

    // Install
    QComboBox* targetMode_ = nullptr;   // wipe a whole drive / use two existing partitions
    QComboBox* drive_ = nullptr;
    QComboBox* bootPart_ = nullptr;
    QComboBox* rootPart_ = nullptr;
    QCheckBox* keepBoot_ = nullptr;
    QLineEdit* installImage_ = nullptr;
    QComboBox* hostSource_ = nullptr;
    QLineEdit* hostImage_ = nullptr;
    QComboBox* hostPreset_ = nullptr;
    QLineEdit* hostname_ = nullptr;
    QLineEdit* rootPass_ = nullptr;
    QComboBox* fs_ = nullptr;
    // Set up on this system
    QComboBox* runSource_ = nullptr;
    QLineEdit* runImage_ = nullptr;
    // Export
    QLineEdit* exportPath_ = nullptr;
    // ISO
    QComboBox* isoContainer_ = nullptr;
    QLineEdit* isoOut_ = nullptr;
    QLineEdit* isoName_ = nullptr;
    QLineEdit* isoTag_ = nullptr;
    QComboBox* isoFormat_ = nullptr;
    QSpinBox* isoLevel_ = nullptr;
    QComboBox* isoInstaller_ = nullptr;
    QCheckBox* isoRam_ = nullptr;
    QCheckBox* isoCache_ = nullptr;
    // Containers
    QTableWidget* table_ = nullptr;
    QLineEdit* copyName_ = nullptr;
    QCheckBox* copySwitch_ = nullptr;
    QLabel* containerHint_ = nullptr;
    QProcess* lister_ = nullptr;        // the container list being read as you (async)
    QPointer<RootProcess> rootLister_;  // ... or through the administrator helper
    QByteArray listOut_;

    QLabel* summary_ = nullptr;
    QLabel* stepLabel_ = nullptr;
    QLabel* result_ = nullptr;
    QProgressBar* bar_ = nullptr;
    QPlainTextEdit* log_ = nullptr;
    QCheckBox* dryRun_ = nullptr;
    QPushButton* startButton_ = nullptr;
    QPushButton* cancelButton_ = nullptr;

    QPointer<RootProcess> process_;   // the running task, in the window's administrator helper (RootSession)
    QString startError_;              // why it could not start (not authorised ...)
    QByteArray pending_;     // output not yet ended by a newline
    bool running_ = false;
    int currentStep_ = 1;
    QString stepBase_;       // "Step 3 of 12  -  Title": the detail (bytes, speed, ETA) is appended to it
    Mode runningMode_ = Install;
    QString savedIso_;       // "ISO saved: <path>" seen in the output
    bool consoleRun_ = false;
    bool rebootRun_ = false;
};
