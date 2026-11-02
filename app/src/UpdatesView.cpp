// SPDX-FileCopyrightText: claudemods
// SPDX-License-Identifier: MIT

#include "UpdatesView.h"

#include "AnsiLog.h"

#include <QCheckBox>
#include <QFileInfo>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMessageBox>
#include <QPointer>
#include <QProgressBar>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QStandardPaths>
#include <QStyle>
#include <QTimer>
#include <QVBoxLayout>

#include <unistd.h>

namespace
{
QFrame*
card()
{
    auto* f = new QFrame;
    f->setObjectName( QStringLiteral( "card" ) );
    return f;
}

QLabel*
label( const QString& text, const char* name )
{
    auto* l = new QLabel( text );
    l->setObjectName( QString::fromLatin1( name ) );
    l->setWordWrap( true );
    return l;
}

QPushButton*
button( const QString& text, const QString& icon, const char* kind = nullptr )
{
    auto* b = new QPushButton( QIcon::fromTheme( icon ), text );
    if ( kind )
    {
        b->setObjectName( QString::fromLatin1( kind ) );
    }
    b->setCursor( Qt::PointingHandCursor );
    return b;
}

void
setState( QLabel* l, const QString& state )
{
    l->setProperty( "state", state );
    l->style()->unpolish( l );
    l->style()->polish( l );
}

// key=value lines of the suite command; repeated keys (tool=) are kept in order
QList< QPair< QString, QString > >
keyValues( const QString& text )
{
    QList< QPair< QString, QString > > out;
    for ( const QString& line : text.split( QLatin1Char( '\n' ), Qt::SkipEmptyParts ) )
    {
        const int eq = line.indexOf( QLatin1Char( '=' ) );
        if ( eq > 0 )
        {
            out.append( { line.left( eq ).trimmed(), line.mid( eq + 1 ).trimmed() } );
        }
    }
    return out;
}
}  // namespace

QString
UpdatesView::suiteCommand()
{
    const QString found = QStandardPaths::findExecutable( QStringLiteral( "claude-codex" ) );
    return found.isEmpty() ? QStringLiteral( "/usr/local/bin/claude-codex" ) : found;
}

UpdatesView::UpdatesView( QWidget* parent )
    : QWidget( parent )
{
    // the page scrolls when the window is small, so nothing is squeezed on top of each other
    auto* outer = new QVBoxLayout( this );
    outer->setContentsMargins( 0, 0, 0, 0 );
    auto* area = new QScrollArea;
    area->setWidgetResizable( true );
    area->setFrameShape( QFrame::NoFrame );
    auto* content = new QWidget;
    area->setWidget( content );
    outer->addWidget( area );
    auto* l = new QVBoxLayout( content );
    l->setContentsMargins( 28, 22, 28, 20 );
    l->setSpacing( 14 );

    auto* title = label( tr( "Updates" ), "pageTitle" );
    auto* sub = label( tr( "One updater for the whole suite: Claude Codex Backup, Install, CCU and this window. "
                           "It downloads the latest release from GitHub and rebuilds everything with build.sh, "
                           "using the choices of your last build." ),
                       "muted" );
    l->addWidget( title );
    l->addWidget( sub );

    // ---- installed ----------------------------------------------------------------------------
    auto* top = new QHBoxLayout;
    top->setSpacing( 14 );
    auto* installed = card();
    auto* il = new QVBoxLayout( installed );
    il->setContentsMargins( 22, 16, 22, 16 );
    il->setSpacing( 6 );
    il->addWidget( label( tr( "Installed" ), "section" ) );
    m_release = label( QString(), "heroTitle" );
    m_details = label( QString(), "muted" );
    il->addWidget( m_release );
    il->addWidget( m_details );
    m_tools = new QGridLayout;
    m_tools->setHorizontalSpacing( 18 );
    m_tools->setVerticalSpacing( 8 );
    il->addLayout( m_tools );
    il->addStretch( 1 );
    top->addWidget( installed, 3 );

    // ---- check / update -----------------------------------------------------------------------
    auto* upd = card();
    auto* ul = new QVBoxLayout( upd );
    ul->setContentsMargins( 22, 16, 22, 16 );
    ul->setSpacing( 8 );
    ul->addWidget( label( tr( "Latest release" ), "section" ) );
    m_state = label( tr( "Not checked yet." ), "muted" );
    ul->addWidget( m_state );
    m_force = new QCheckBox( tr( "Rebuild even when it is up to date (repair)" ) );
    ul->addWidget( m_force );
    auto* buttons = new QHBoxLayout;
    m_check = button( tr( "Check Now" ), QStringLiteral( "view-refresh" ) );
    m_update = button( tr( "Update Everything" ), QStringLiteral( "system-software-update" ), "primary" );
    buttons->addWidget( m_check );
    buttons->addWidget( m_update );
    buttons->addStretch( 1 );
    ul->addLayout( buttons );
    auto* note = label( tr( "From a terminal: claude-codex update  (claude-codex update --check only looks)." ), "muted" );
    ul->addWidget( note );
    ul->addStretch( 1 );
    top->addWidget( upd, 2 );
    l->addLayout( top );

    // ---- progress and log ---------------------------------------------------------------------
    auto* run = card();
    auto* rl = new QVBoxLayout( run );
    rl->setContentsMargins( 14, 14, 14, 14 );
    rl->setSpacing( 8 );
    auto* head = new QHBoxLayout;
    m_phase = label( tr( "Ready" ), "taskTitle" );
    m_elapsed = label( QString(), "muted" );
    m_elapsed->setWordWrap( false );
    m_cancel = button( tr( "Cancel" ), QStringLiteral( "process-stop" ), "danger" );
    m_cancel->setEnabled( false );
    head->addWidget( m_phase, 1 );
    head->addWidget( m_elapsed );
    head->addWidget( m_cancel );
    rl->addLayout( head );
    m_bar = new QProgressBar;
    m_bar->setRange( 0, 1000 );
    m_bar->setValue( 0 );
    m_bar->setFormat( QString() );
    rl->addWidget( m_bar );
    m_log = new AnsiLog;
    m_log->setMinimumHeight( 300 );
    rl->addWidget( m_log, 1 );
    l->addWidget( run, 1 );
    // the cards above keep their natural height; only the log grows
    installed->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Maximum );
    upd->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Maximum );

    m_timer = new QTimer( this );
    m_timer->setInterval( 1000 );
    connect( m_timer, &QTimer::timeout, this, &UpdatesView::tick );
    connect( m_check, &QPushButton::clicked, this, [ this ]() { check( false ); } );
    connect( m_update, &QPushButton::clicked, this, &UpdatesView::startUpdate );
    connect( m_cancel, &QPushButton::clicked, this, &UpdatesView::stopUpdate );

    refreshInstalled();
}

// What `claude-codex status --plain` reports: the release and every tool with its path
void
UpdatesView::refreshInstalled()
{
    while ( QLayoutItem* item = m_tools->takeAt( 0 ) )
    {
        delete item->widget();
        delete item;
    }
    if ( !QFileInfo( suiteCommand() ).isExecutable() )
    {
        m_release->setText( tr( "claude-codex is not installed" ) );
        m_details->setText( tr( "The suite command comes with build.sh. Run ./build.sh from the compendium folder." ) );
        m_update->setEnabled( false );
        m_check->setEnabled( false );
        return;
    }
    QProcess p;
    p.start( suiteCommand(), { QStringLiteral( "status" ), QStringLiteral( "--plain" ) } );
    p.waitForFinished( 8000 );
    QString release, version, built, options, source;
    int row = 0;
    for ( const auto& kv : keyValues( QString::fromUtf8( p.readAllStandardOutput() ) ) )
    {
        if ( kv.first == QLatin1String( "release" ) )
        {
            release = kv.second;
        }
        else if ( kv.first == QLatin1String( "version" ) )
        {
            version = kv.second;
        }
        else if ( kv.first == QLatin1String( "built" ) )
        {
            built = kv.second;
        }
        else if ( kv.first == QLatin1String( "options" ) )
        {
            options = kv.second;
        }
        else if ( kv.first == QLatin1String( "source" ) )
        {
            source = kv.second;
        }
        else if ( kv.first == QLatin1String( "tool" ) )
        {
            // tool=Name|command|/full/path (empty path: not installed)
            const QStringList f = kv.second.split( QLatin1Char( '|' ) );
            const bool there = !f.value( 2 ).isEmpty();
            // one line each (word wrap in a grid squeezes the rows together)
            auto* name = label( f.value( 0 ), "fieldLabel" );
            name->setWordWrap( false );
            auto* cmd = label( f.value( 1 ), "muted" );
            cmd->setWordWrap( false );
            auto* pill = new QLabel( there ? tr( "installed" ) : tr( "not installed" ) );
            pill->setObjectName( QStringLiteral( "pill" ) );
            pill->setAlignment( Qt::AlignCenter );
            pill->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
            setState( pill, there ? QStringLiteral( "ok" ) : QStringLiteral( "off" ) );
            pill->setToolTip( f.value( 2 ) );
            m_tools->addWidget( name, row, 0 );
            m_tools->addWidget( cmd, row, 1 );
            m_tools->addWidget( pill, row, 2, Qt::AlignLeft );
            ++row;
        }
    }
    m_tools->setColumnStretch( 1, 1 );
    m_release->setText( release.isEmpty() ? tr( "Claude Codex" ) : tr( "Claude Codex %1" ).arg( release ) );
    QStringList lines;
    if ( !version.isEmpty() )
    {
        lines << tr( "Version %1" ).arg( version ) + ( built.isEmpty() ? QString() : tr( ", built %1" ).arg( built ) );
    }
    if ( !options.isEmpty() )
    {
        lines << tr( "Build choices: %1" ).arg( options );
    }
    if ( !source.isEmpty() )
    {
        lines << tr( "Source: %1" ).arg( source );
    }
    m_details->setText( lines.join( QLatin1Char( '\n' ) ) );
}

void
UpdatesView::checkQuietly()
{
    check( true );
}

void
UpdatesView::check( bool quiet )
{
    if ( m_checker || !QFileInfo( suiteCommand() ).isExecutable() )
    {
        return;
    }
    if ( !quiet )
    {
        m_state->setText( tr( "Asking GitHub..." ) );
    }
    m_checker = new QProcess( this );
    connect( m_checker,
             &QProcess::finished,
             this,
             [ this ]( int, QProcess::ExitStatus )
             {
                 QString installed, latest;
                 bool newer = false;
                 for ( const auto& kv : keyValues( QString::fromUtf8( m_checker->readAllStandardOutput() ) ) )
                 {
                     if ( kv.first == QLatin1String( "installed" ) )
                     {
                         installed = kv.second;
                     }
                     else if ( kv.first == QLatin1String( "latest" ) )
                     {
                         latest = kv.second;
                     }
                     else if ( kv.first == QLatin1String( "newer" ) )
                     {
                         newer = kv.second == QLatin1String( "1" );
                     }
                 }
                 m_checker->deleteLater();
                 m_checker = nullptr;
                 if ( latest.isEmpty() )
                 {
                     m_state->setText( tr( "The check failed - offline, or GitHub could not be reached." ) );
                 }
                 else if ( newer )
                 {
                     m_state->setText( tr( "Version %1 is available (installed: %2). Press Update Everything." ).arg( latest, installed ) );
                 }
                 else
                 {
                     m_state->setText( tr( "Up to date: version %1 is installed, the latest is %2." ).arg( installed, latest ) );
                 }
                 Q_EMIT checked( !latest.isEmpty(), newer, latest );
             } );
    m_checker->start( suiteCommand(), { QStringLiteral( "update" ), QStringLiteral( "--check" ) } );
}

void
UpdatesView::startUpdate()
{
    if ( m_process )
    {
        return;
    }
    if ( QMessageBox::question( this,
                                tr( "Update Everything" ),
                                tr( "Download the latest Claude Codex from GitHub, then rebuild and reinstall every tool "
                                    "with the choices of your last build?\n\nThis compiles Claude Codex Install (about 1000 "
                                    "files) and can take a while. The window stays usable." ) )
         != QMessageBox::Yes )
    {
        return;
    }
    QStringList args { suiteCommand(), QStringLiteral( "update" ), QStringLiteral( "--yes" ) };
    if ( m_force->isChecked() )
    {
        args << QStringLiteral( "--force" );
    }
    QString program;
    if ( ::geteuid() == 0 )
    {
        program = args.takeFirst();
    }
    else
    {
        program = QStandardPaths::findExecutable( QStringLiteral( "pkexec" ) );
        if ( program.isEmpty() )
        {
            QMessageBox::warning( this, tr( "Update" ), tr( "pkexec (polkit) is not installed - it asks for the administrator password." ) );
            return;
        }
    }
    m_log->clearLog();
    m_buffer.clear();
    m_cancelled = false;
    m_bar->setRange( 0, 1000 );
    m_bar->setValue( 0 );
    m_bar->setFormat( QString() );
    m_bar->setProperty( "state", QString() );
    m_bar->style()->unpolish( m_bar );
    m_bar->style()->polish( m_bar );
    m_phase->setText( tr( "Waiting for the administrator password..." ) );

    m_process = new QProcess( this );
    m_process->setProcessChannelMode( QProcess::MergedChannels );
    connect( m_process, &QProcess::readyRead, this, &UpdatesView::readOutput );
    connect( m_process, &QProcess::finished, this, &UpdatesView::updateFinished );
    connect( m_process, &QProcess::started, this, [ this ]() { m_process->closeWriteChannel(); } );
    connect( m_process,
             &QProcess::errorOccurred,
             this,
             [ this ]( QProcess::ProcessError e )
             {
                 if ( e == QProcess::FailedToStart )
                 {
                     m_log->appendAnsi( tr( "\x1b[91mCould not start %1\x1b[0m\n" ).arg( m_process->program() ) );
                     m_process->deleteLater();
                     m_process = nullptr;
                     setBusy( false );
                 }
             } );
    m_log->appendAnsi( QStringLiteral( "\x1b[36m$ %1 %2\x1b[0m\n" ).arg( program, args.join( QLatin1Char( ' ' ) ) ) );
    setBusy( true );
    m_clock.start();
    m_timer->start();
    tick();
    m_process->start( program, args );
}

// The update runs as root: your user cannot signal it, so kill asks for the password once more
void
UpdatesView::stopUpdate()
{
    if ( !m_process
         || QMessageBox::question( this, tr( "Cancel" ), tr( "Stop the update? Tools that were not reinstalled yet keep their old version." ) )
             != QMessageBox::Yes )
    {
        return;
    }
    m_cancelled = true;
    const qint64 pid = m_process->processId();
    if ( ::geteuid() == 0 || pid <= 0 )
    {
        m_process->terminate();
    }
    else
    {
        const QString pkexec = QStandardPaths::findExecutable( QStringLiteral( "pkexec" ) );
        if ( !pkexec.isEmpty() )
        {
            // the whole process group: build.sh and the compiler it runs
            QProcess::startDetached( pkexec, { QStringLiteral( "/bin/kill" ), QStringLiteral( "-TERM" ), QStringLiteral( "--" ), QStringLiteral( "-%1" ).arg( pid ) } );
        }
    }
}

// The output is build.sh's: its banners ("BUILD 2/4 ...", "INSTALL", "COMPLETE") and ninja's [n/m]
void
UpdatesView::readOutput()
{
    const QByteArray chunk = m_process->readAll();
    m_log->appendAnsi( QString::fromUtf8( chunk ) );
    if ( m_phase->text() == tr( "Waiting for the administrator password..." ) )
    {
        m_phase->setText( tr( "Updating..." ) );
    }
    m_buffer += chunk;
    if ( m_buffer.size() > 8192 )
    {
        m_buffer = m_buffer.right( 4096 );
    }
    const QString text = QString::fromUtf8( m_buffer ).remove( QRegularExpression( QStringLiteral( "\x1b\\[[0-9;]*m" ) ) );

    static const QRegularExpression phaseRe( QStringLiteral( "===== (BUILD (\\d)/(\\d)[^=\\n]*|INSTALL[^=\\n]*|COMPLETE[^=\\n]*|DOWNLOAD[^=\\n]*) =====" ) );
    static const QRegularExpression ninjaRe( QStringLiteral( "\\[(\\d+)/(\\d+)\\]" ) );
    int value = m_bar->value();
    QRegularExpressionMatchIterator it = phaseRe.globalMatch( text );
    QRegularExpressionMatch last;
    while ( it.hasNext() )
    {
        last = it.next();
    }
    if ( last.hasMatch() )
    {
        const QString name = last.captured( 1 ).trimmed();
        m_phase->setText( name );
        if ( name.startsWith( QLatin1String( "DOWNLOAD" ) ) )
        {
            value = qMax( value, 20 );
        }
        else if ( name.startsWith( QLatin1String( "BUILD" ) ) )
        {
            const int n = last.captured( 2 ).toInt(), total = qMax( 1, last.captured( 3 ).toInt() );
            value = qMax( value, 100 + ( n - 1 ) * 750 / total );
            // inside a build step: ninja's [done/all]
            QRegularExpressionMatchIterator nit = ninjaRe.globalMatch( text.mid( last.capturedEnd() ) );
            QRegularExpressionMatch nl;
            while ( nit.hasNext() )
            {
                nl = nit.next();
            }
            if ( nl.hasMatch() && nl.captured( 2 ).toInt() > 0 )
            {
                value = qMax( value, 100 + ( n - 1 ) * 750 / total + nl.captured( 1 ).toInt() * 750 / total / nl.captured( 2 ).toInt() );
            }
        }
        else if ( name.startsWith( QLatin1String( "INSTALL" ) ) )
        {
            value = qMax( value, 870 );
        }
        else if ( name.startsWith( QLatin1String( "COMPLETE" ) ) )
        {
            value = qMax( value, 990 );
        }
    }
    m_bar->setValue( qMin( value, 1000 ) );
    m_bar->setFormat( QStringLiteral( "%1%" ).arg( m_bar->value() / 10 ) );
}

void
UpdatesView::updateFinished( int code, QProcess::ExitStatus status )
{
    m_log->appendAnsi( QString::fromUtf8( m_process->readAll() ) );
    m_process->deleteLater();
    m_process = nullptr;
    m_timer->stop();
    tick();
    setBusy( false );
    const bool ok = status == QProcess::NormalExit && code == 0 && !m_cancelled;
    QString message;
    if ( ok )
    {
        message = tr( "Everything is updated. Restart Claude Codex Cloner to use the new window." );
    }
    else if ( m_cancelled )
    {
        message = tr( "Cancelled." );
    }
    else if ( code == 126 || code == 127 )
    {
        message = tr( "Not started: the administrator password was not given." );
    }
    else
    {
        message = tr( "The update failed (exit code %1) - see the log above. Nothing that failed to build was installed." ).arg( code );
    }
    m_phase->setText( message );
    m_bar->setRange( 0, 1000 );
    m_bar->setValue( ok ? 1000 : m_bar->value() );
    m_bar->setFormat( ok ? tr( "Done" ) : m_cancelled ? tr( "Cancelled" ) : tr( "Failed" ) );
    m_bar->setProperty( "state", ok ? QStringLiteral( "ok" ) : QStringLiteral( "fail" ) );
    m_bar->style()->unpolish( m_bar );
    m_bar->style()->polish( m_bar );
    m_log->appendAnsi( QStringLiteral( "\n%1%2\x1b[0m\n" ).arg( ok ? QStringLiteral( "\x1b[92m" ) : QStringLiteral( "\x1b[91m" ), message ) );
    refreshInstalled();
    if ( ok )
    {
        check( true );
    }
}

void
UpdatesView::setBusy( bool busy )
{
    m_update->setEnabled( !busy );
    m_check->setEnabled( !busy );
    m_force->setEnabled( !busy );
    m_cancel->setEnabled( busy );
}

void
UpdatesView::tick()
{
    const qint64 s = m_clock.isValid() ? m_clock.elapsed() / 1000 : 0;
    m_elapsed->setText( QStringLiteral( "%1:%2:%3" )
                            .arg( s / 3600, 2, 10, QLatin1Char( '0' ) )
                            .arg( ( s / 60 ) % 60, 2, 10, QLatin1Char( '0' ) )
                            .arg( s % 60, 2, 10, QLatin1Char( '0' ) ) );
}

bool
UpdatesView::confirmClose()
{
    if ( !m_process )
    {
        return true;
    }
    return QMessageBox::question( this,
                                  tr( "Quit" ),
                                  tr( "The update is still running. It keeps running after the window closes. Quit anyway?" ) )
        == QMessageBox::Yes;
}
