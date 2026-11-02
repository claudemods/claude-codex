// SPDX-FileCopyrightText: claudemods
// SPDX-License-Identifier: MIT
//
// RootSession: one administrator password for the whole window. The first time root is needed (the
// window asks right after it opens) the app starts itself once more as a small helper through pkexec
// ("claude-codex-cloner --root-helper", see RootHelper.cpp). Every root task of Backup, Containers and
// Updates then runs through that helper: no further password prompts while the window is open. If the
// prompt is cancelled, the next task that needs root asks again.
//
// The helper only starts the suite's own tools (an allowlist: the cloner, cccu, claude-codex, the
// installer launcher) and stops when the window closes.

#pragma once

#include <QHash>
#include <QObject>
#include <QPointer>
#include <QStringList>

class QProcess;
class RootSession;

// One root task running in the helper; used like a QProcess
class RootProcess : public QObject
{
    Q_OBJECT
public:
    QString program() const { return m_program; }
    bool isRunning() const { return m_running; }

    void write( const QByteArray& data );
    void closeWriteChannel();
    void terminate();   // SIGTERM to the task and everything it started (its process group)
    void kill();        // SIGKILL, the same way

Q_SIGNALS:
    void output( const QByteArray& data );         // stdout and stderr, as it comes
    void finished( int exitCode, bool crashed );   // always emitted once, also when it never started
    void failedToStart( const QString& why );      // before finished(): not authorised, not allowed ...

private:
    friend class RootSession;
    RootProcess( RootSession* session, qint64 id, const QString& program );

    RootSession* m_session;
    qint64 m_id;
    QString m_program;
    bool m_running = true;
};

class RootSession : public QObject
{
    Q_OBJECT
public:
    static RootSession* instance();

    bool isUnlocked() const { return m_ready; }   // the helper runs with administrator rights
    void unlock();                                 // start the helper (the password prompt) if it is not running

    // Starts `program` as root. The display variables of this session are passed on (for the installer).
    RootProcess* start( const QString& program, const QStringList& args );

Q_SIGNALS:
    void unlockedChanged( bool unlocked );

private:
    friend class RootProcess;
    explicit RootSession( QObject* parent );
    void launch();
    void send( const QByteArray& line );
    void sendOp( const char* op, qint64 id, const QString& key = QString(), const QString& value = QString() );
    void onData();
    void onHelperGone( int exitCode );

    QProcess* m_helper = nullptr;
    bool m_ready = false;
    QByteArray m_buffer;
    QList< QByteArray > m_queue;   // requests sent while the helper is not ready yet
    QHash< qint64, QPointer< RootProcess > > m_procs;
    qint64 m_next = 1;
};
