// SPDX-FileCopyrightText: claudemods
// SPDX-License-Identifier: MIT

#include "RootHelper.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QSocketNotifier>

#include <cerrno>
#include <csignal>
#include <utility>

#include <sys/types.h>
#include <unistd.h>

namespace
{
bool g_stdoutOpen = true;

void
send( const QJsonObject& o )
{
    if ( !g_stdoutOpen )
    {
        return;
    }
    const QByteArray line = QJsonDocument( o ).toJson( QJsonDocument::Compact ) + '\n';
    const char* p = line.constData();
    qsizetype left = line.size();
    while ( left > 0 )
    {
        const ssize_t n = ::write( STDOUT_FILENO, p, size_t( left ) );
        if ( n < 0 )
        {
            if ( errno == EINTR )
            {
                continue;
            }
            g_stdoutOpen = false;   // the window is gone
            return;
        }
        p += n;
        left -= n;
    }
}

// Only the suite's own tools run as root through the helper
QStringList
allowedPrograms()
{
    QStringList out;
    for ( const char* path : { "/usr/bin/claudebackup-linux",
                               "/usr/local/bin/claudebackup-linux",
                               "/usr/local/bin/cccu",
                               "/usr/bin/cccu",
                               "/usr/local/bin/claude-codex",
                               "/usr/local/bin/claudebackup-install",
                               "/usr/bin/claudeinstall",
                               "/usr/bin/claudeinstall-tui" } )
    {
        const QFileInfo fi( QString::fromLatin1( path ) );
        if ( fi.exists() )
        {
            out << fi.canonicalFilePath();
        }
    }
    return out;
}

// What a request may set in the task's environment (the window's display, for the installer)
const char* const kPassEnv[]
    = { "DISPLAY", "WAYLAND_DISPLAY", "XAUTHORITY", "XDG_RUNTIME_DIR", "XDG_SESSION_TYPE", "XDG_CURRENT_DESKTOP" };
}  // namespace

int
runRootHelper( int argc, char** argv )
{
    ::signal( SIGPIPE, SIG_IGN );
    QCoreApplication app( argc, argv );
    if ( ::geteuid() != 0 )
    {
        return 126;
    }
    const QStringList allowed = allowedPrograms();
    QHash< qint64, QProcess* > procs;
    QByteArray input;
    bool stdinOpen = true;

    auto maybeQuit = [ & ]()
    {
        if ( !stdinOpen && procs.isEmpty() )
        {
            app.quit();
        }
    };

    auto handle = [ & ]( const QJsonObject& o )
    {
        const QString op = o.value( QStringLiteral( "op" ) ).toString();
        const qint64 id = qint64( o.value( QStringLiteral( "id" ) ).toDouble() );
        QProcess* p = procs.value( id );

        if ( op == QLatin1String( "start" ) )
        {
            const QString program = o.value( QStringLiteral( "program" ) ).toString();
            const QString canonical = QFileInfo( program ).canonicalFilePath();
            if ( canonical.isEmpty() || !allowed.contains( canonical ) )
            {
                send( QJsonObject { { QStringLiteral( "id" ), double( id ) },
                                    { QStringLiteral( "error" ), QStringLiteral( "%1 is not one of the Claude Codex tools" ).arg( program ) } } );
                return;
            }
            p = new QProcess( &app );
            p->setProcessChannelMode( QProcess::MergedChannels );
            QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
            const QJsonObject requested = o.value( QStringLiteral( "env" ) ).toObject();
            for ( const char* name : kPassEnv )
            {
                const QString key = QString::fromLatin1( name );
                if ( requested.contains( key ) )
                {
                    env.insert( key, requested.value( key ).toString() );
                }
            }
            env.insert( QStringLiteral( "CLAUDE_CODEX_SETSID" ), QStringLiteral( "1" ) );   // claude-codex update: no setsid of its own
            p->setProcessEnvironment( env );
            // its own process group: Cancel stops the task and everything it started
            p->setChildProcessModifier( []() { ::setpgid( 0, 0 ); } );
            QObject::connect( p,
                              &QProcess::readyRead,
                              &app,
                              [ p, id ]()
                              {
                                  const QByteArray data = p->readAll();
                                  if ( !data.isEmpty() )
                                  {
                                      send( QJsonObject { { QStringLiteral( "id" ), double( id ) },
                                                          { QStringLiteral( "out" ), QString::fromLatin1( data.toBase64() ) } } );
                                  }
                              } );
            QObject::connect( p,
                              &QProcess::finished,
                              &app,
                              [ &, p, id ]( int code, QProcess::ExitStatus status )
                              {
                                  const QByteArray data = p->readAll();
                                  if ( !data.isEmpty() )
                                  {
                                      send( QJsonObject { { QStringLiteral( "id" ), double( id ) },
                                                          { QStringLiteral( "out" ), QString::fromLatin1( data.toBase64() ) } } );
                                  }
                                  send( QJsonObject { { QStringLiteral( "id" ), double( id ) },
                                                      { QStringLiteral( "exit" ), code },
                                                      { QStringLiteral( "crashed" ), status == QProcess::CrashExit } } );
                                  procs.remove( id );
                                  p->deleteLater();
                                  maybeQuit();
                              } );
            QObject::connect( p,
                              &QProcess::errorOccurred,
                              &app,
                              [ &, p, id ]( QProcess::ProcessError error )
                              {
                                  if ( error != QProcess::FailedToStart )
                                  {
                                      return;
                                  }
                                  send( QJsonObject { { QStringLiteral( "id" ), double( id ) },
                                                      { QStringLiteral( "error" ), QStringLiteral( "could not start %1" ).arg( p->program() ) } } );
                                  procs.remove( id );
                                  p->deleteLater();
                                  maybeQuit();
                              } );
            procs.insert( id, p );
            QStringList args;
            for ( const QJsonValue& v : o.value( QStringLiteral( "args" ) ).toArray() )
            {
                args << v.toString();
            }
            p->start( canonical, args );
        }
        else if ( !p )
        {
            return;
        }
        else if ( op == QLatin1String( "write" ) )
        {
            p->write( QByteArray::fromBase64( o.value( QStringLiteral( "data" ) ).toString().toLatin1() ) );
        }
        else if ( op == QLatin1String( "close" ) )
        {
            p->closeWriteChannel();
        }
        else if ( op == QLatin1String( "signal" ) )
        {
            const int sig = o.value( QStringLiteral( "sig" ) ).toInt( SIGTERM );
            const qint64 pid = p->processId();
            if ( pid > 0 && ( sig == SIGTERM || sig == SIGKILL || sig == SIGINT ) )
            {
                if ( ::kill( -pid_t( pid ), sig ) != 0 )
                {
                    ::kill( pid_t( pid ), sig );
                }
            }
        }
    };

    QSocketNotifier notifier( STDIN_FILENO, QSocketNotifier::Read );
    QObject::connect( &notifier,
                      &QSocketNotifier::activated,
                      &app,
                      [ & ]()
                      {
                          char buf[ 65536 ];
                          const ssize_t n = ::read( STDIN_FILENO, buf, sizeof buf );
                          if ( n < 0 && errno == EINTR )
                          {
                              return;
                          }
                          if ( n <= 0 )
                          {
                              // the window closed: cancellable tasks stop (their stdin ends), the rest finish
                              notifier.setEnabled( false );
                              stdinOpen = false;
                              g_stdoutOpen = false;
                              for ( QProcess* p : std::as_const( procs ) )
                              {
                                  p->closeWriteChannel();
                              }
                              maybeQuit();
                              return;
                          }
                          input.append( buf, n );
                          int nl;
                          while ( ( nl = input.indexOf( '\n' ) ) >= 0 )
                          {
                              const QJsonObject o = QJsonDocument::fromJson( input.left( nl ) ).object();
                              input.remove( 0, nl + 1 );
                              if ( !o.isEmpty() )
                              {
                                  handle( o );
                              }
                          }
                      } );

    send( QJsonObject { { QStringLiteral( "ready" ), 1 } } );
    return app.exec();
}
