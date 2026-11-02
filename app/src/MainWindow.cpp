// SPDX-FileCopyrightText: claudemods
// SPDX-License-Identifier: MIT

#include "MainWindow.h"

#include "BackupView.h"
#include "CcuView.h"
#include "GuideView.h"
#include "RootSession.h"
#include "UpdatesView.h"

#include <QCloseEvent>
#include <QKeySequence>
#include <QShortcut>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPushButton>
#include <QScreen>
#include <QStyle>
#include <QTabWidget>
#include <QTimer>
#include <QVBoxLayout>

namespace
{
// The release every Claude Codex tool shows
const char* const kRelease = "Compendium v1.0 01-11-2026";
}  // namespace

MainWindow::MainWindow( QWidget* parent )
    : QMainWindow( parent )
{
    setWindowTitle( tr( "Claude Codex Cloner - %1" ).arg( QString::fromLatin1( kRelease ) ) );
    setWindowIcon( QIcon::fromTheme( QStringLiteral( "claude-codex-cloner" ), QIcon::fromTheme( QStringLiteral( "drive-harddisk" ) ) ) );
    // never bigger than the screen (scaled desktops have less room): pages scroll when it is small
    setMinimumSize( 960, 640 );
    const QRect avail = screen() ? screen()->availableGeometry() : QRect( 0, 0, 1320, 860 );
    resize( qMin( 1320, avail.width() * 94 / 100 ), qMin( 860, avail.height() * 92 / 100 ) );

    auto* central = new QWidget;
    auto* col = new QVBoxLayout( central );
    col->setContentsMargins( 0, 0, 0, 0 );
    col->setSpacing( 0 );
    col->addWidget( makeHeader() );

    m_tabs = new QTabWidget;
    m_tabs->setObjectName( QStringLiteral( "mainTabs" ) );
    m_tabs->setDocumentMode( true );

    m_backup = new BackupView;
    m_tabs->addTab( m_backup, QIcon::fromTheme( QStringLiteral( "media-optical" ) ), tr( "Backup" ) );

    if ( CcuView::available() )
    {
        m_ccu = new CcuView;
        m_tabs->addTab( m_ccu, QIcon::fromTheme( QStringLiteral( "virtualbox" ), QIcon::fromTheme( QStringLiteral( "network-server" ) ) ), tr( "Containers" ) );
        connect( m_ccu,
                 &CcuView::writeUsbRequested,
                 this,
                 [ this ]( const QString& iso )
                 {
                     m_tabs->setCurrentIndex( BackupTab );
                     m_backup->showWriteUsb( iso );
                 } );
    }
    else
    {
        m_tabs->addTab( makeCcuMissing(), QIcon::fromTheme( QStringLiteral( "network-server" ) ), tr( "Containers" ) );
    }

    m_updates = new UpdatesView;
    m_tabs->addTab( m_updates, QIcon::fromTheme( QStringLiteral( "system-software-update" ) ), tr( "Updates" ) );
    m_guide = new GuideView( m_backup );
    m_tabs->addTab( m_guide, QIcon::fromTheme( QStringLiteral( "help-contents" ) ), tr( "Guide" ) );
    col->addWidget( m_tabs, 1 );
    setCentralWidget( central );

    connect( m_backup, &BackupView::updatesRequested, this, [ this ]() { m_tabs->setCurrentIndex( UpdatesTab ); } );
    connect( m_updates,
             &UpdatesView::checked,
             this,
             [ this ]( bool ok, bool newer, const QString& latest )
             {
                 if ( !ok )
                 {
                     setPill( m_updatePill, QStringLiteral( "off" ), tr( "Update check failed" ) );
                 }
                 else if ( newer )
                 {
                     setPill( m_updatePill, QStringLiteral( "warn" ), tr( "Update %1 available" ).arg( latest ) );
                 }
                 else
                 {
                     setPill( m_updatePill, QStringLiteral( "ok" ), tr( "Up to date" ) );
                 }
             } );
    connect( m_tabs,
             &QTabWidget::currentChanged,
             this,
             [ this ]( int index )
             {
                 if ( index == GuideTab )
                 {
                     m_guide->loadDistroNotes();
                 }
             } );

    // F11: full screen and back (it opens maximised)
    auto* fullScreen = new QShortcut( QKeySequence( Qt::Key_F11 ), this );
    connect( fullScreen, &QShortcut::activated, this, [ this ]() { isFullScreen() ? showMaximized() : showFullScreen(); } );

    // the administrator helper: one password for the window
    connect( RootSession::instance(),
             &RootSession::unlockedChanged,
             this,
             [ this ]( bool unlocked )
             {
                 setPill( m_rootPill,
                          unlocked ? QStringLiteral( "ok" ) : QStringLiteral( "warn" ),
                          unlocked ? tr( "Administrator: unlocked" ) : tr( "Administrator: locked" ) );
             } );

    // Ask GitHub in the background once the window is up (never blocks, never updates by itself)
    QTimer::singleShot( 1500, m_updates, &UpdatesView::checkQuietly );
}

QWidget*
MainWindow::makeHeader()
{
    auto* header = new QFrame;
    header->setObjectName( QStringLiteral( "header" ) );
    auto* h = new QHBoxLayout( header );
    h->setContentsMargins( 22, 12, 22, 12 );
    h->setSpacing( 10 );

    auto* icon = new QLabel;
    icon->setPixmap( windowIcon().pixmap( 40, 40 ) );
    h->addWidget( icon );

    auto* brandCol = new QVBoxLayout;
    brandCol->setSpacing( 0 );
    auto* brandRow = new QHBoxLayout;
    brandRow->setSpacing( 6 );
    auto* claude = new QLabel( QStringLiteral( "Claude" ) );
    claude->setObjectName( QStringLiteral( "brand" ) );
    auto* codex = new QLabel( QStringLiteral( "Codex" ) );
    codex->setObjectName( QStringLiteral( "brandRed" ) );
    auto* cloner = new QLabel( QStringLiteral( "Cloner" ) );
    cloner->setObjectName( QStringLiteral( "brand" ) );
    brandRow->addWidget( claude );
    brandRow->addWidget( codex );
    brandRow->addWidget( cloner );
    brandRow->addStretch( 1 );
    auto* release = new QLabel( tr( "%1  ·  Backup · Install · CCU" ).arg( QString::fromLatin1( kRelease ) ) );
    release->setObjectName( QStringLiteral( "release" ) );
    auto* tagline = new QLabel( tr( "Sailing the 7 seas like Penguin's Eggs, Remastersys, Refracta, Systemback and father Knoppix!" ) );
    tagline->setObjectName( QStringLiteral( "tagline" ) );
    brandCol->addLayout( brandRow );
    brandCol->addWidget( release );
    brandCol->addWidget( tagline );
    h->addLayout( brandCol, 1 );

    m_ccuPill = new QLabel;
    m_ccuPill->setObjectName( QStringLiteral( "pill" ) );
    setPill( m_ccuPill,
             CcuView::available() ? QStringLiteral( "ok" ) : QStringLiteral( "off" ),
             CcuView::available() ? tr( "CCU installed" ) : tr( "CCU not installed" ) );
    m_updatePill = new QLabel;
    m_updatePill->setObjectName( QStringLiteral( "pill" ) );
    setPill( m_updatePill, QStringLiteral( "off" ), tr( "Checking for updates..." ) );
    m_rootPill = new QLabel;
    m_rootPill->setObjectName( QStringLiteral( "pill" ) );
    m_rootPill->setToolTip( tr( "Root tasks run through one administrator helper: the password is asked once per window" ) );
    setPill( m_rootPill, QStringLiteral( "off" ), tr( "Administrator: asking..." ) );
    h->addWidget( m_rootPill );
    h->addWidget( m_ccuPill );
    h->addWidget( m_updatePill );
    return header;
}

// Containers tab when cccu is not installed: what it is and how to get it
QWidget*
MainWindow::makeCcuMissing()
{
    auto* page = new QWidget;
    auto* l = new QVBoxLayout( page );
    l->setContentsMargins( 28, 24, 28, 22 );
    auto* c = new QFrame;
    c->setObjectName( QStringLiteral( "card" ) );
    auto* cl = new QVBoxLayout( c );
    cl->setContentsMargins( 22, 18, 22, 18 );
    cl->setSpacing( 8 );
    auto* title = new QLabel( tr( "Claude Codex CCU is not installed" ) );
    title->setObjectName( QStringLiteral( "heroTitle" ) );
    auto* text = new QLabel( tr( "CCU clones this system into an Incus container (with the GPU through cage, or on a "
                                 "server without one), installs that setup to a drive, exports images and builds live "
                                 "ISOs of containers.\n\nIt runs on the Arch, Debian/Ubuntu, Fedora and openSUSE families "
                                 "and needs GCC 14+ or Clang 17+. Run ./build.sh again and answer yes to "
                                 "\"Build CCCU\" - or use Updates > Update Everything after enabling it." ) );
    text->setObjectName( QStringLiteral( "muted" ) );
    text->setWordWrap( true );
    cl->addWidget( title );
    cl->addWidget( text );
    l->addWidget( c );
    l->addStretch( 1 );
    return page;
}

void
MainWindow::setPill( QLabel* pill, const QString& state, const QString& text )
{
    pill->setText( text );
    pill->setProperty( "state", state );
    pill->style()->unpolish( pill );
    pill->style()->polish( pill );
}

void
MainWindow::closeEvent( QCloseEvent* event )
{
    if ( !m_backup->confirmClose() || ( m_ccu && !m_ccu->confirmClose() ) || !m_updates->confirmClose() )
    {
        event->ignore();
        return;
    }
    event->accept();
}
