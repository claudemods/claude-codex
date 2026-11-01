// SPDX-FileCopyrightText: claudemods
// SPDX-License-Identifier: MIT
//
// Backend: everything the app does goes through the cloner binary (claudebackup-linux), so the GUI
// and the terminal menu share one engine. Info commands run as the user; root commands run through
// pkexec (polkit rule org.claudemods.claudebackup.run) with their output streamed to the GUI.

#pragma once

#include <QHash>
#include <QObject>
#include <QProcess>
#include <QStringList>

struct ClonerStatus
{
    bool ok = false;
    QString error;
    QHash< QString, QString > values;
    struct CustomFile
    {
        QString name, label;
        bool edited = false;
    };
    struct InstallerFile
    {
        QString label, path;
        bool exists = false;
    };
    QList< CustomFile > customFiles;
    QList< InstallerFile > installerFiles;

    QString value( const QString& key, const QString& fallback = QString() ) const { return values.value( key, fallback ); }
    bool flag( const QString& key ) const { return values.value( key ) == QLatin1String( "1" ); }
    QString family() const { return value( QStringLiteral( "family" ) ); }
};

struct PlanStep
{
    QString name, description;
};

class Backend : public QObject
{
    Q_OBJECT
public:
    explicit Backend( QObject* parent = nullptr );
    ~Backend() override;

    // The cloner binary: $CLAUDEBACKUP_CLONER, next to this app, or /usr/bin/claudebackup-linux
    static QString clonerPath();
    static QString userName();

    // Synchronous info commands (run as the user, a few hundred ms)
    ClonerStatus status() const;
    QList< PlanStep > plan() const;
    QString guide() const;
    bool set( const QStringList& keyValues, QString* error ) const;

    // One root task at a time. stdinData is written then stdin closed; with cancellable the
    // cloner instead keeps stdin open and cancels when we close it (--cancel-on-eof).
    bool isRunning() const;
    bool startRoot( const QString& title, const QStringList& args, bool cancellable, const QByteArray& stdinData = {} );
    void cancel();

Q_SIGNALS:
    void taskStarted( const QString& title );
    void output( const QString& text );                         // raw, with ANSI colours and \r
    void step( int number, int total, const QString& name );    // @@step markers of a build
    void taskFinished( bool ok, bool cancelled, const QString& message );

private:
    void readOutput();
    void handleMarker( const QString& marker );
    void onFinished( int exitCode, QProcess::ExitStatus status );
    QString runUser( const QStringList& args, int* exitCode = nullptr, const QByteArray& input = {} ) const;

    QProcess* m_process = nullptr;
    QByteArray m_buffer;
    QString m_result;   // "ok" | "fail" | "cancelled" from @@result
    bool m_cancelRequested = false;
};
