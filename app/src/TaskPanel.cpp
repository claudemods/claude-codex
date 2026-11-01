// SPDX-FileCopyrightText: claudemods
// SPDX-License-Identifier: MIT

#include "TaskPanel.h"

#include "AnsiLog.h"
#include "Backend.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QRegularExpression>
#include <QStyle>
#include <QTimer>
#include <QVBoxLayout>

TaskPanel::TaskPanel( Backend* backend, QWidget* parent )
    : QWidget( parent )
    , m_backend( backend )
{
    auto* layout = new QVBoxLayout( this );
    layout->setContentsMargins( 0, 0, 0, 0 );
    layout->setSpacing( 8 );

    auto* top = new QHBoxLayout;
    m_title = new QLabel( tr( "Ready" ) );
    m_title->setObjectName( QStringLiteral( "taskTitle" ) );
    m_elapsed = new QLabel;
    m_elapsed->setObjectName( QStringLiteral( "muted" ) );
    m_cancel = new QPushButton( tr( "Cancel" ) );
    m_cancel->setObjectName( QStringLiteral( "danger" ) );
    m_cancel->setEnabled( false );
    top->addWidget( m_title, 1 );
    top->addWidget( m_elapsed );
    top->addWidget( m_cancel );
    layout->addLayout( top );

    m_progress = new QProgressBar;
    m_progress->setRange( 0, 1000 );
    m_progress->setValue( 0 );
    m_progress->setTextVisible( true );
    m_progress->setFormat( QStringLiteral( "" ) );
    layout->addWidget( m_progress );

    m_state = new QLabel;
    m_state->setObjectName( QStringLiteral( "muted" ) );
    m_state->setWordWrap( true );
    layout->addWidget( m_state );

    m_log = new AnsiLog;
    layout->addWidget( m_log, 1 );

    m_timer = new QTimer( this );
    m_timer->setInterval( 1000 );
    connect( m_timer, &QTimer::timeout, this, &TaskPanel::tick );

    connect( m_cancel,
             &QPushButton::clicked,
             this,
             [ this ]()
             {
                 if ( QMessageBox::question( this,
                                             tr( "Cancel" ),
                                             tr( "Stop \"%1\"? Running programs are stopped and the build's mounts are removed." )
                                                 .arg( m_title->text() ) )
                      == QMessageBox::Yes )
                 {
                     m_cancel->setEnabled( false );
                     m_backend->cancel();
                 }
             } );
}

bool
TaskPanel::start( const QString& title,
                  const QStringList& args,
                  bool cancellable,
                  const QStringList& stepNames,
                  qint64 expectedBytes,
                  const QByteArray& stdinData )
{
    if ( m_backend->isRunning() )
    {
        QMessageBox::information( this, title, tr( "Another task is still running. Wait for it to finish first." ) );
        return false;
    }
    m_log->clearLog();
    m_steps = stepNames;
    m_step = 0;
    m_total = stepNames.size();
    m_expectedBytes = expectedBytes;
    m_bytes = 0;
    m_title->setText( title );
    m_state->setText( tr( "Waiting for the administrator password..." ) );
    m_progress->setValue( 0 );
    m_progress->setFormat( QStringLiteral( "" ) );
    m_progress->setProperty( "state", QString() );
    m_progress->style()->unpolish( m_progress );
    m_progress->style()->polish( m_progress );
    m_cancel->setEnabled( cancellable );

    m_active = true;
    connect( m_backend, &Backend::output, this, &TaskPanel::onOutput );
    connect( m_backend, &Backend::step, this, &TaskPanel::onStep );
    connect( m_backend, &Backend::taskFinished, this, &TaskPanel::onFinished );
    if ( !m_backend->startRoot( title, args, cancellable, stdinData ) )
    {
        if ( m_active )
        {
            // startRoot reports why through taskFinished; make sure we are detached either way
            onFinished( false, false, tr( "The task could not be started." ) );
        }
        return false;
    }
    m_clock.start();
    m_timer->start();
    tick();
    return true;
}

void
TaskPanel::onOutput( const QString& text )
{
    m_log->appendAnsi( text );
    if ( m_state->text() == tr( "Waiting for the administrator password..." ) )
    {
        m_state->setText( tr( "Running..." ) );
    }
    if ( m_expectedBytes > 0 )
    {
        // dd status=progress: "123456789 bytes (123 MB, 117 MiB) copied, 5 s, 24.7 MB/s"
        static const QRegularExpression bytes( QStringLiteral( "(\\d+) bytes" ) );
        const QRegularExpressionMatch m = bytes.match( m_log->lastLine() );
        if ( m.hasMatch() )
        {
            m_bytes = m.captured( 1 ).toLongLong();
            m_state->setText( m_log->lastLine().trimmed() );
        }
    }
    updateProgress();
}

void
TaskPanel::onStep( int number, int total, const QString& name )
{
    m_step = number;
    m_total = qMax( total, 1 );
    const int index = m_steps.indexOf( name );
    m_state->setText( tr( "Step %1 of %2: %3" ).arg( number ).arg( m_total ).arg( name ) );
    Q_EMIT stepStarted( index >= 0 ? index : number - 1, name );
    updateProgress();
}

void
TaskPanel::updateProgress()
{
    if ( m_total > 0 && m_step > 0 )
    {
        // within a step only the image step (mksquashfs / mkfs.erofs) reports a percentage
        double within = 0;
        const QString current = m_step - 1 < m_steps.size() ? m_steps.value( m_step - 1 ) : QString();
        if ( current == QLatin1String( "mksquashfs" ) && m_log->lastPercent() >= 0 )
        {
            within = m_log->lastPercent() / 100.0;
        }
        const double v = ( m_step - 1 + within ) / m_total;
        m_progress->setValue( int( v * 1000 ) );
        m_progress->setFormat( QStringLiteral( "%1%" ).arg( int( v * 100 ) ) );
    }
    else if ( m_expectedBytes > 0 && m_bytes > 0 )
    {
        const double v = qMin( 1.0, double( m_bytes ) / m_expectedBytes );
        m_progress->setValue( int( v * 1000 ) );
        m_progress->setFormat( QStringLiteral( "%1%" ).arg( int( v * 100 ) ) );
    }
    else if ( m_total == 0 && m_expectedBytes == 0 )
    {
        m_progress->setRange( 0, 0 );   // busy indicator for tasks without progress
    }
}

void
TaskPanel::tick()
{
    const qint64 s = m_clock.isValid() ? m_clock.elapsed() / 1000 : 0;
    m_elapsed->setText( QStringLiteral( "%1:%2:%3" )
                            .arg( s / 3600, 2, 10, QLatin1Char( '0' ) )
                            .arg( ( s / 60 ) % 60, 2, 10, QLatin1Char( '0' ) )
                            .arg( s % 60, 2, 10, QLatin1Char( '0' ) ) );
}

void
TaskPanel::onFinished( bool ok, bool cancelled, const QString& message )
{
    if ( !m_active )
    {
        return;
    }
    m_active = false;
    disconnect( m_backend, nullptr, this, nullptr );
    m_timer->stop();
    tick();
    m_cancel->setEnabled( false );
    m_progress->setRange( 0, 1000 );
    m_progress->setValue( ok ? 1000 : m_progress->value() );
    m_progress->setFormat( ok ? tr( "Done" ) : cancelled ? tr( "Cancelled" ) : tr( "Failed" ) );
    m_progress->setProperty( "state", ok ? QStringLiteral( "ok" ) : QStringLiteral( "fail" ) );
    m_progress->style()->unpolish( m_progress );
    m_progress->style()->polish( m_progress );
    m_state->setText( message );
    m_log->appendAnsi( QStringLiteral( "\n%1%2\x1b[0m\n" ).arg( ok ? QStringLiteral( "\x1b[92m" ) : QStringLiteral( "\x1b[91m" ), message ) );
    Q_EMIT finished( ok, cancelled );
}
