// SPDX-FileCopyrightText: claudemods
// SPDX-License-Identifier: MIT

#include "RootSession.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QStandardPaths>

#include <csignal>
#include <utility>

#include <unistd.h>

namespace
{
QByteArray
jsonLine( const QJsonObject& o )
{
    return QJsonDocument( o ).toJson( QJsonDocument::Compact ) + '\n';
}
}  // namespace

// ---- RootProcess ---------------------------------------------------------------------------------

RootProcess::RootProcess( RootSession* session, qint64 id, const QString& program )
    : QObject( session )
    , m_session( session )
    , m_id( id )
    , m_program( program )
{
}

void
RootProcess::write( const QByteArray& data )
{
    if ( m_running && !data.isEmpty() )
    {
        m_session->sendOp( "write", m_id, QStringLiteral( "data" ), QString::fromLatin1( data.toBase64() ) );
    }
}

void
RootProcess::closeWriteChannel()
{
    if ( m_running )
    {
        m_session->sendOp( "close", m_id );
    }
}

void
RootProcess::terminate()
{
    if ( m_running )
    {
        m_session->sendOp( "signal", m_id, QStringLiteral( "sig" ), QString::number( SIGTERM ) );
    }
}

void
RootProcess::kill()
{
    if ( m_running )
    {
        m_session->sendOp( "signal", m_id, QStringLiteral( "sig" ), QString::number( SIGKILL ) );
    }
}

// ---- RootSession ---------------------------------------------------------------------------------

RootSession*
RootSession::instance()
{
    static RootSession* session = new RootSession( QCoreApplication::instance() );
    return session;
}

RootSession::RootSession( QObject* parent )
    : QObject( parent )
{
}

void
RootSession::unlock()
{
    if ( !m_helper )
    {
        launch();
    }
}

void
RootSession::launch()
{
    m_ready = false;
    m_buffer.clear();
    m_helper = new QProcess( this );
    m_helper->setProcessChannelMode( QProcess::ForwardedErrorChannel );
    connect( m_helper, &QProcess::readyReadStandardOutput, this, &RootSession::onData );
    connect( m_helper, &QProcess::finished, this, [ this ]( int code, QProcess::ExitStatus ) { onHelperGone( code ); } );
    connect( m_helper,
             &QProcess::errorOccurred,
             this,
             [ this ]( QProcess::ProcessError e )
             {
                 if ( e == QProcess::FailedToStart )
                 {
                     onHelperGone( 127 );
                 }
             } );

    const QString self = QCoreApplication::applicationFilePath();
    if ( ::geteuid() == 0 )
    {
        m_helper->start( self, { QStringLiteral( "--root-helper" ) } );
        return;
    }
    // pkexec needs the absolute path its polkit rule names (org.claudemods.claude-codex.cloner)
    const QString pkexec = QStandardPaths::findExecutable( QStringLiteral( "pkexec" ) );
    if ( pkexec.isEmpty() )
    {
        m_helper->start( QStringLiteral( "/nonexistent/pkexec" ), {} );   // reports FailedToStart
        return;
    }
    m_helper->start( pkexec, { self, QStringLiteral( "--root-helper" ) } );
}

void
RootSession::send( const QByteArray& line )
{
    if ( !m_helper )
    {
        launch();
    }
    if ( m_ready )
    {
        m_helper->write( line );
    }
    else
    {
        m_queue.append( line );
    }
}

void
RootSession::sendOp( const char* op, qint64 id, const QString& key, const QString& value )
{
    QJsonObject o { { QStringLiteral( "op" ), QString::fromLatin1( op ) }, { QStringLiteral( "id" ), double( id ) } };
    if ( !key.isEmpty() )
    {
        if ( key == QLatin1String( "sig" ) )
        {
            o.insert( key, value.toInt() );
        }
        else
        {
            o.insert( key, value );
        }
    }
    send( jsonLine( o ) );
}

RootProcess*
RootSession::start( const QString& program, const QStringList& args )
{
    const qint64 id = m_next++;
    auto* p = new RootProcess( this, id, program );
    m_procs.insert( id, p );

    QJsonObject env;
    for ( const char* name : { "DISPLAY", "WAYLAND_DISPLAY", "XAUTHORITY", "XDG_RUNTIME_DIR", "XDG_SESSION_TYPE", "XDG_CURRENT_DESKTOP" } )
    {
        if ( qEnvironmentVariableIsSet( name ) )
        {
            env.insert( QString::fromLatin1( name ), qEnvironmentVariable( name ) );
        }
    }
    QJsonObject o { { QStringLiteral( "op" ), QStringLiteral( "start" ) },
                    { QStringLiteral( "id" ), double( id ) },
                    { QStringLiteral( "program" ), program },
                    { QStringLiteral( "args" ), QJsonArray::fromStringList( args ) },
                    { QStringLiteral( "env" ), env } };
    send( jsonLine( o ) );
    return p;
}

void
RootSession::onData()
{
    m_buffer += m_helper->readAllStandardOutput();
    int nl;
    while ( ( nl = m_buffer.indexOf( '\n' ) ) >= 0 )
    {
        const QJsonObject o = QJsonDocument::fromJson( m_buffer.left( nl ) ).object();
        m_buffer.remove( 0, nl + 1 );
        if ( o.contains( QStringLiteral( "ready" ) ) )
        {
            m_ready = true;
            for ( const QByteArray& line : std::as_const( m_queue ) )
            {
                m_helper->write( line );
            }
            m_queue.clear();
            Q_EMIT unlockedChanged( true );
            continue;
        }
        const qint64 id = qint64( o.value( QStringLiteral( "id" ) ).toDouble() );
        QPointer< RootProcess > p = m_procs.value( id );
        if ( !p )
        {
            continue;
        }
        if ( o.contains( QStringLiteral( "out" ) ) )
        {
            Q_EMIT p->output( QByteArray::fromBase64( o.value( QStringLiteral( "out" ) ).toString().toLatin1() ) );
        }
        else if ( o.contains( QStringLiteral( "error" ) ) )
        {
            m_procs.remove( id );
            p->m_running = false;
            Q_EMIT p->failedToStart( o.value( QStringLiteral( "error" ) ).toString() );
            Q_EMIT p->finished( 127, true );
        }
        else if ( o.contains( QStringLiteral( "exit" ) ) )
        {
            m_procs.remove( id );
            p->m_running = false;
            Q_EMIT p->finished( o.value( QStringLiteral( "exit" ) ).toInt(), o.value( QStringLiteral( "crashed" ) ).toBool() );
        }
    }
}

// The helper is gone: the password prompt was cancelled (126 / 127), or it stopped. Every task that was
// waiting for it or running in it ends; the next task starts a new helper (and asks again).
void
RootSession::onHelperGone( int exitCode )
{
    if ( !m_helper )
    {
        return;
    }
    const bool wasReady = m_ready;
    m_helper->deleteLater();
    m_helper = nullptr;
    m_ready = false;
    m_queue.clear();
    m_buffer.clear();
    const QString why = wasReady ? tr( "The administrator helper stopped." )
                                 : tr( "Not authorised - the administrator password was not given (or pkexec / polkit is missing)." );
    const auto procs = m_procs;
    m_procs.clear();
    for ( const QPointer< RootProcess >& p : procs )
    {
        if ( p )
        {
            p->m_running = false;
            Q_EMIT p->failedToStart( why );
            Q_EMIT p->finished( wasReady ? -1 : 126, true );
        }
    }
    Q_EMIT unlockedChanged( false );
    Q_UNUSED( exitCode )
}
