// SPDX-FileCopyrightText: claudemods
// SPDX-License-Identifier: MIT

#include "Backend.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStandardPaths>

#include <pwd.h>
#include <unistd.h>

namespace
{
const char* const kCancelFlag = "--cancel-on-eof";
}

Backend::Backend( QObject* parent )
    : QObject( parent )
{
}

Backend::~Backend()
{
    if ( m_process && m_process->state() != QProcess::NotRunning )
    {
        // Closing stdin makes a cancellable task stop and clean up its mounts
        m_process->closeWriteChannel();
        m_process->waitForFinished( 5000 );
    }
}

QString
Backend::clonerPath()
{
    const QString env = qEnvironmentVariable( "CLAUDEBACKUP_CLONER" );
    if ( !env.isEmpty() && QFileInfo( env ).isExecutable() )
    {
        return QFileInfo( env ).absoluteFilePath();
    }
    const QString beside = QCoreApplication::applicationDirPath() + QStringLiteral( "/claudebackup-linux" );
    if ( QFileInfo( beside ).isExecutable() )
    {
        return beside;
    }
    return QStringLiteral( "/usr/bin/claudebackup-linux" );
}

QString
Backend::userName()
{
    if ( struct passwd* pw = getpwuid( getuid() ) )
    {
        return QString::fromLocal8Bit( pw->pw_name );
    }
    return qEnvironmentVariable( "USER" );
}

QString
Backend::runUser( const QStringList& args, int* exitCode, const QByteArray& input ) const
{
    QProcess p;
    p.setProcessChannelMode( QProcess::SeparateChannels );
    p.start( clonerPath(), args );
    if ( !p.waitForStarted( 5000 ) )
    {
        if ( exitCode )
        {
            *exitCode = -1;
        }
        return QString();
    }
    if ( !input.isEmpty() )
    {
        p.write( input );
    }
    p.closeWriteChannel();
    p.waitForFinished( 60000 );
    if ( exitCode )
    {
        *exitCode = p.exitStatus() == QProcess::NormalExit ? p.exitCode() : -1;
    }
    // stderr carries the cloner's error lines ([ccb] ...); keep them for messages
    const QString err = QString::fromUtf8( p.readAllStandardError() ).trimmed();
    const QString out = QString::fromUtf8( p.readAllStandardOutput() );
    return err.isEmpty() ? out : out + QStringLiteral( "\n@@stderr " ) + err;
}

ClonerStatus
Backend::status() const
{
    ClonerStatus s;
    if ( !QFileInfo( clonerPath() ).isExecutable() )
    {
        s.error = tr( "The cloner (%1) is not installed. Run the compendium's build.sh first." ).arg( clonerPath() );
        return s;
    }
    int rc = 0;
    const QString out = runUser( { QStringLiteral( "--status" ) }, &rc );
    for ( const QString& line : out.split( QLatin1Char( '\n' ) ) )
    {
        const int eq = line.indexOf( QLatin1Char( '=' ) );
        if ( eq <= 0 || line.startsWith( QLatin1String( "@@" ) ) )
        {
            continue;
        }
        const QString key = line.left( eq ), value = line.mid( eq + 1 );
        const QStringList parts = value.split( QLatin1Char( '|' ) );
        if ( key == QLatin1String( "customFile" ) && parts.size() == 3 )
        {
            s.customFiles.append( { parts[ 0 ], parts[ 1 ], parts[ 2 ] == QLatin1String( "1" ) } );
        }
        else if ( key == QLatin1String( "installerFile" ) && parts.size() == 3 )
        {
            s.installerFiles.append( { parts[ 0 ], parts[ 1 ], parts[ 2 ] == QLatin1String( "1" ) } );
        }
        else
        {
            s.values.insert( key, value );
        }
    }
    s.ok = rc == 0 && s.values.contains( QStringLiteral( "distro" ) );
    if ( !s.ok )
    {
        s.error = tr( "Could not read the status from %1." ).arg( clonerPath() );
    }
    return s;
}

QList< PlanStep >
Backend::plan() const
{
    QList< PlanStep > steps;
    for ( const QString& line : runUser( { QStringLiteral( "--plan" ) } ).split( QLatin1Char( '\n' ) ) )
    {
        if ( !line.startsWith( QLatin1String( "step=" ) ) )
        {
            continue;
        }
        const QString v = line.mid( 5 );
        const int bar = v.indexOf( QLatin1Char( '|' ) );
        steps.append( { bar < 0 ? v : v.left( bar ), bar < 0 ? QString() : v.mid( bar + 1 ) } );
    }
    return steps;
}

QString
Backend::guide() const
{
    QString g = runUser( { QStringLiteral( "--guide" ) } );
    const int err = g.indexOf( QLatin1String( "\n@@stderr " ) );
    return err < 0 ? g : g.left( err );
}

bool
Backend::set( const QStringList& keyValues, QString* error ) const
{
    int rc = 0;
    QStringList args { QStringLiteral( "--set" ) };
    args << keyValues;
    const QString out = runUser( args, &rc );
    if ( rc != 0 && error )
    {
        const int err = out.indexOf( QLatin1String( "@@stderr " ) );
        *error = err < 0 ? tr( "Saving the settings failed." ) : out.mid( err + 9 ).remove( QRegularExpression( "\x1b\\[[0-9;]*m" ) );
    }
    return rc == 0;
}

bool
Backend::isRunning() const
{
    return m_process && m_process->state() != QProcess::NotRunning;
}

bool
Backend::startRoot( const QString& title, const QStringList& args, bool cancellable, const QByteArray& stdinData )
{
    if ( isRunning() )
    {
        return false;
    }
    if ( m_process )
    {
        m_process->deleteLater();
    }
    m_process = new QProcess( this );
    m_process->setProcessChannelMode( QProcess::MergedChannels );
    m_buffer.clear();
    m_result.clear();
    m_cancelRequested = false;

    QStringList clonerArgs { QStringLiteral( "--ccb-user=" ) + userName() };
    clonerArgs << args;
    if ( cancellable )
    {
        clonerArgs << QString::fromLatin1( kCancelFlag );
    }

    QString program;
    QStringList programArgs;
    if ( geteuid() == 0 )
    {
        program = clonerPath();
        programArgs = clonerArgs;
    }
    else
    {
        // pkexec needs the absolute path the polkit rule names
        program = QStandardPaths::findExecutable( QStringLiteral( "pkexec" ) );
        if ( program.isEmpty() )
        {
            Q_EMIT taskFinished( false, false, tr( "pkexec (polkit) is not installed - it asks for the administrator password." ) );
            return false;
        }
        programArgs = QStringList { clonerPath() } + clonerArgs;
    }

    connect( m_process, &QProcess::readyRead, this, &Backend::readOutput );
    connect( m_process, &QProcess::finished, this, &Backend::onFinished );
    connect( m_process,
             &QProcess::errorOccurred,
             this,
             [ this ]( QProcess::ProcessError e )
             {
                 if ( e == QProcess::FailedToStart )
                 {
                     Q_EMIT taskFinished( false, false, tr( "Could not start %1." ).arg( m_process->program() ) );
                 }
             } );

    Q_EMIT taskStarted( title );
    Q_EMIT output( QStringLiteral( "\x1b[36m$ %1 %2\x1b[0m\n" ).arg( program, programArgs.join( QLatin1Char( ' ' ) ) ) );
    m_process->start( program, programArgs );
    if ( !cancellable )
    {
        if ( !stdinData.isEmpty() )
        {
            m_process->write( stdinData );
        }
        m_process->closeWriteChannel();
    }
    return true;
}

void
Backend::cancel()
{
    if ( !isRunning() )
    {
        return;
    }
    m_cancelRequested = true;
    Q_EMIT output( QStringLiteral( "\n\x1b[33mCancelling - stopping the running step and removing the mounts...\x1b[0m\n" ) );
    m_process->closeWriteChannel();
}

void
Backend::readOutput()
{
    m_buffer += m_process->readAll();
    // Whole lines: markers are taken out, everything else goes to the log
    int nl;
    while ( ( nl = m_buffer.indexOf( '\n' ) ) >= 0 )
    {
        const QByteArray line = m_buffer.left( nl );
        m_buffer.remove( 0, nl + 1 );
        if ( line.startsWith( "@@" ) )
        {
            handleMarker( QString::fromUtf8( line.mid( 2 ) ).trimmed() );
        }
        else
        {
            Q_EMIT output( QString::fromUtf8( line ) + QLatin1Char( '\n' ) );
        }
    }
    // Progress bars (mksquashfs, dd, mkfs.erofs) redraw one line with \r and no \n: pass them on now.
    // A partial line that may still become a marker waits for the rest.
    if ( !m_buffer.isEmpty() && !m_buffer.startsWith( "@" ) )
    {
        const int cr = m_buffer.lastIndexOf( '\r' );
        if ( cr >= 0 )
        {
            Q_EMIT output( QString::fromUtf8( m_buffer.left( cr + 1 ) ) );
            m_buffer.remove( 0, cr + 1 );
        }
        else if ( m_buffer.size() > 4096 )
        {
            Q_EMIT output( QString::fromUtf8( m_buffer ) );
            m_buffer.clear();
        }
    }
}

void
Backend::handleMarker( const QString& marker )
{
    const QStringList parts = marker.split( QLatin1Char( ' ' ), Qt::SkipEmptyParts );
    if ( parts.size() >= 4 && parts[ 0 ] == QLatin1String( "step" ) )
    {
        Q_EMIT step( parts[ 1 ].toInt(), parts[ 2 ].toInt(), parts[ 3 ] );
    }
    else if ( parts.size() >= 2 && parts[ 0 ] == QLatin1String( "result" ) )
    {
        m_result = parts[ 1 ];
    }
}

void
Backend::onFinished( int exitCode, QProcess::ExitStatus status )
{
    if ( !m_buffer.isEmpty() )
    {
        Q_EMIT output( QString::fromUtf8( m_buffer ) );
        m_buffer.clear();
    }
    const bool cancelled = m_cancelRequested || m_result == QLatin1String( "cancelled" );
    const bool ok = !cancelled && status == QProcess::NormalExit && exitCode == 0;
    QString message;
    if ( cancelled )
    {
        message = tr( "Cancelled." );
    }
    else if ( ok )
    {
        message = tr( "Finished successfully." );
    }
    else if ( exitCode == 126 || exitCode == 127 )
    {
        // pkexec: the password dialog was dismissed, or the user is not allowed
        message = tr( "Not authorised - the administrator password was not given." );
    }
    else
    {
        message = tr( "Failed (exit code %1). The log is in /var/log/claudebackup-linux.log." ).arg( exitCode );
    }
    Q_EMIT taskFinished( ok, cancelled, message );
}
