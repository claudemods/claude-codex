// SPDX-FileCopyrightText: claudemods
// SPDX-License-Identifier: MIT

#include "MainWindow.h"

#include "AnsiLog.h"
#include "TaskPanel.h"

#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QLocale>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QSaveFile>
#include <QScrollArea>
#include <QSpinBox>
#include <QSplitter>
#include <QStackedWidget>
#include <QStandardItemModel>
#include <QStandardPaths>
#include <QStorageInfo>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

#include <algorithm>
#include <functional>

namespace
{
QWidget*
pageHeader( const QString& title, const QString& subtitle )
{
    auto* w = new QWidget;
    auto* l = new QVBoxLayout( w );
    l->setContentsMargins( 0, 0, 0, 6 );
    l->setSpacing( 2 );
    auto* t = new QLabel( title );
    t->setObjectName( QStringLiteral( "pageTitle" ) );
    auto* s = new QLabel( subtitle );
    s->setObjectName( QStringLiteral( "muted" ) );
    s->setWordWrap( true );
    l->addWidget( t );
    l->addWidget( s );
    return w;
}

QFrame*
card()
{
    auto* f = new QFrame;
    f->setObjectName( QStringLiteral( "card" ) );
    return f;
}

QLabel*
sectionLabel( const QString& text )
{
    auto* l = new QLabel( text );
    l->setObjectName( QStringLiteral( "section" ) );
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

// A page's content with the standard margins, scrollable when the window is small
QWidget*
pageFrame( QWidget* content, bool scroll = false )
{
    content->layout()->setContentsMargins( 28, 24, 28, 22 );
    if ( !scroll )
    {
        return content;
    }
    auto* area = new QScrollArea;
    area->setWidgetResizable( true );
    area->setFrameShape( QFrame::NoFrame );
    area->setWidget( content );
    return area;
}

void
clearLayout( QLayout* layout )
{
    while ( QLayoutItem* item = layout->takeAt( 0 ) )
    {
        if ( QWidget* w = item->widget() )
        {
            w->deleteLater();
        }
        else if ( QLayout* l = item->layout() )
        {
            clearLayout( l );
            l->deleteLater();
        }
        delete item;
    }
}

QString
runCapture( const QString& program, const QStringList& args )
{
    QProcess p;
    p.start( program, args );
    if ( !p.waitForStarted( 3000 ) )
    {
        return QString();
    }
    p.waitForFinished( 10000 );
    return QString::fromUtf8( p.readAllStandardOutput() );
}

// Whole disks holding the running system (/, /boot, /home ...), through LUKS / LVM as well
QStringList
systemDisks()
{
    QStringList disks;
    for ( const char* mp : { "/", "/boot", "/boot/efi", "/home", "/usr" } )
    {
        QString source = runCapture( QStringLiteral( "findmnt" ), { QStringLiteral( "-no" ), QStringLiteral( "SOURCE" ), QString::fromLatin1( mp ) } ).trimmed();
        source = source.section( QLatin1Char( '[' ), 0, 0 );   // btrfs: /dev/sda2[/@]
        if ( !source.startsWith( QLatin1String( "/dev/" ) ) )
        {
            continue;
        }
        const QString tree = runCapture( QStringLiteral( "lsblk" ), { QStringLiteral( "-nspo" ), QStringLiteral( "NAME,TYPE" ), source } );
        for ( const QString& line : tree.split( QLatin1Char( '\n' ), Qt::SkipEmptyParts ) )
        {
            const QStringList f = line.simplified().split( QLatin1Char( ' ' ) );
            if ( f.size() == 2 && f[ 1 ] == QLatin1String( "disk" ) )
            {
                // -s prints the tree with branch characters in front of the name
                QString name = f[ 0 ];
                name.remove( QRegularExpression( QStringLiteral( "^[^/]*" ) ) );
                if ( !disks.contains( name ) )
                {
                    disks << name;
                }
            }
        }
    }
    return disks;
}

QString
jsonText( const QJsonValue& v )
{
    return v.isBool() ? ( v.toBool() ? QStringLiteral( "1" ) : QStringLiteral( "0" ) ) : v.toVariant().toString();
}
}  // namespace

MainWindow::MainWindow( QWidget* parent )
    : QMainWindow( parent )
    , m_backend( new Backend( this ) )
{
    setWindowTitle( tr( "claudebackup linux" ) );
    setWindowIcon( QIcon::fromTheme( QStringLiteral( "drive-harddisk" ) ) );
    setMinimumSize( 980, 640 );
    resize( 1200, 780 );

    auto* central = new QWidget;
    auto* row = new QHBoxLayout( central );
    row->setContentsMargins( 0, 0, 0, 0 );
    row->setSpacing( 0 );

    // ---- sidebar --------------------------------------------------------------------------
    auto* side = new QFrame;
    side->setObjectName( QStringLiteral( "sidebar" ) );
    side->setFixedWidth( 232 );
    auto* sl = new QVBoxLayout( side );
    sl->setContentsMargins( 14, 20, 14, 14 );
    sl->setSpacing( 10 );
    auto* brand = new QLabel( QStringLiteral( "claudebackup" ) );
    brand->setObjectName( QStringLiteral( "brand" ) );
    auto* brandSub = new QLabel( tr( "linux system cloner" ) );
    brandSub->setObjectName( QStringLiteral( "brandSub" ) );
    sl->addWidget( brand );
    sl->addWidget( brandSub );
    sl->addSpacing( 10 );

    m_nav = new QListWidget;
    m_nav->setObjectName( QStringLiteral( "nav" ) );
    m_nav->setIconSize( QSize( 20, 20 ) );
    m_nav->setFocusPolicy( Qt::NoFocus );
    const QList< QPair< QString, QString > > navItems = {
        { tr( "Overview" ), QStringLiteral( "computer" ) },
        { tr( "Build ISO" ), QStringLiteral( "media-optical" ) },
        { tr( "Settings" ), QStringLiteral( "preferences-system" ) },
        { tr( "Edit Configs" ), QStringLiteral( "document-edit" ) },
        { tr( "Write USB" ), QStringLiteral( "drive-removable-media" ) },
        { tr( "Install" ), QStringLiteral( "system-software-install" ) },
        { tr( "Guide" ), QStringLiteral( "help-contents" ) },
        { tr( "Maintenance" ), QStringLiteral( "applications-utilities" ) },
    };
    for ( const auto& n : navItems )
    {
        m_nav->addItem( new QListWidgetItem( QIcon::fromTheme( n.second ), n.first ) );
    }
    sl->addWidget( m_nav, 1 );
    m_statusBadge = new QLabel;
    m_statusBadge->setObjectName( QStringLiteral( "badge" ) );
    m_statusBadge->setWordWrap( true );
    sl->addWidget( m_statusBadge );

    // ---- pages (same order as Page) -------------------------------------------------------
    m_pages = new QStackedWidget;
    m_pages->setObjectName( QStringLiteral( "pages" ) );
    m_pages->addWidget( makeOverviewPage() );
    m_pages->addWidget( makeBuildPage() );
    m_pages->addWidget( makeSettingsPage() );
    m_pages->addWidget( makeConfigsPage() );
    m_pages->addWidget( makeUsbPage() );
    m_pages->addWidget( makeInstallPage() );
    m_pages->addWidget( makeGuidePage() );
    m_pages->addWidget( makeMaintenancePage() );

    row->addWidget( side );
    row->addWidget( m_pages, 1 );
    setCentralWidget( central );

    connect( m_nav,
             &QListWidget::currentRowChanged,
             this,
             [ this ]( int r )
             {
                 m_pages->setCurrentIndex( r );
                 if ( r == Usb )
                 {
                     refreshIsos();
                     refreshDrives( m_driveCombo, true );
                 }
             } );
    m_nav->setCurrentRow( Overview );

    QTimer::singleShot( 0, this, &MainWindow::refreshStatus );
}

void
MainWindow::goTo( Page page )
{
    m_nav->setCurrentRow( page );
}

QString
MainWindow::isoOutputDir() const
{
    return m_status.value( QStringLiteral( "isoOutputDir" ) );
}

QString
MainWindow::humanSize( qint64 bytes ) const
{
    return QLocale().formattedDataSize( bytes, 1, QLocale::DataSizeTraditionalFormat );
}

// ============================================================================================
// Status
// ============================================================================================
void
MainWindow::refreshStatus()
{
    QApplication::setOverrideCursor( Qt::WaitCursor );
    m_status = m_backend->status();
    m_plan = m_status.ok ? m_backend->plan() : QList< PlanStep >();
    QApplication::restoreOverrideCursor();

    if ( !m_status.ok )
    {
        m_statusBadge->setText( QStringLiteral( "<span style='color:#ff5c6c'>●</span> %1" ).arg( tr( "Cloner not available" ) ) );
        m_distroTitle->setText( tr( "The cloner could not be started" ) );
        m_distroSub->setText( m_status.error );
        clearLayout( m_checks );
        return;
    }
    const bool ready = m_status.flag( QStringLiteral( "supported" ) ) && m_status.value( QStringLiteral( "missingTools" ) ).isEmpty()
        && m_status.flag( QStringLiteral( "bootFiles" ) );
    m_statusBadge->setText( QStringLiteral( "<span style='color:%1'>●</span> %2<br><span style='color:#8a909c'>%3 · %4</span>" )
                                .arg( ready ? QStringLiteral( "#5fbf7a" ) : QStringLiteral( "#e5c07b" ),
                                      ready ? tr( "Ready to build" ) : tr( "Setup needed" ),
                                      m_status.value( QStringLiteral( "familyLabel" ) ),
                                      m_status.value( QStringLiteral( "version" ) ) ) );
    fillOverview();
    fillBuild();
    fillSettings();
    fillConfigs();
    fillInstall();
    if ( m_guide->toPlainText().isEmpty() )
    {
        m_guide->setPlainText( m_backend->guide() );
    }
    m_versionLabel->setText( tr( "%1 %2 on %3" )
                                 .arg( m_status.value( QStringLiteral( "app" ) ),
                                       m_status.value( QStringLiteral( "version" ) ),
                                       m_status.value( QStringLiteral( "distro" ) ) ) );
    m_mkinitcpio->setVisible( isArch() );
}

// ============================================================================================
// Overview
// ============================================================================================
QWidget*
MainWindow::makeOverviewPage()
{
    auto* page = new QWidget;
    auto* l = new QVBoxLayout( page );
    l->setSpacing( 16 );
    l->addWidget( pageHeader( tr( "Overview" ),
                              tr( "Clone this running system into a bootable live ISO - a full clone: users, passwords, "
                                  "settings and /home exactly as they are. ClaudeInstall puts it back on a disk." ) ) );

    auto* hero = card();
    hero->setObjectName( QStringLiteral( "hero" ) );
    auto* hl = new QHBoxLayout( hero );
    hl->setContentsMargins( 22, 18, 22, 18 );
    auto* texts = new QVBoxLayout;
    m_distroTitle = new QLabel( tr( "Reading the system..." ) );
    m_distroTitle->setObjectName( QStringLiteral( "heroTitle" ) );
    m_distroSub = new QLabel;
    m_distroSub->setObjectName( QStringLiteral( "muted" ) );
    m_distroSub->setWordWrap( true );
    texts->addWidget( m_distroTitle );
    texts->addWidget( m_distroSub );
    hl->addLayout( texts, 1 );
    auto* build = button( tr( "Build ISO" ), QStringLiteral( "media-record" ), "primary" );
    auto* usb = button( tr( "Write USB" ), QStringLiteral( "drive-removable-media" ) );
    auto* refresh = button( tr( "Refresh" ), QStringLiteral( "view-refresh" ) );
    hl->addWidget( build );
    hl->addWidget( usb );
    hl->addWidget( refresh );
    connect( build, &QPushButton::clicked, this, &MainWindow::startBuild );
    connect( usb, &QPushButton::clicked, this, [ this ]() { goTo( Usb ); } );
    connect( refresh, &QPushButton::clicked, this, &MainWindow::refreshStatus );
    l->addWidget( hero );

    auto* checksCard = card();
    auto* cl = new QVBoxLayout( checksCard );
    cl->setContentsMargins( 20, 16, 20, 16 );
    cl->addWidget( sectionLabel( tr( "System readiness" ) ) );
    m_checks = new QVBoxLayout;
    m_checks->setSpacing( 2 );
    cl->addLayout( m_checks );
    l->addWidget( checksCard );
    l->addStretch( 1 );
    return pageFrame( page, true );
}

void
MainWindow::fillOverview()
{
    const ClonerStatus& s = m_status;
    m_distroTitle->setText( s.value( QStringLiteral( "distro" ) ) );
    m_distroSub->setText( tr( "%1 family · %2 · next ISO: %3" )
                              .arg( s.value( QStringLiteral( "familyLabel" ) ),
                                    s.value( QStringLiteral( "arch" ) ),
                                    s.value( QStringLiteral( "isoPath" ) ) ) );

    clearLayout( m_checks );
    enum State
    {
        Ok,
        Warn,
        Fail,
        Info
    };
    auto add = [ this ]( State state, const QString& title, const QString& detail, const QString& action = QString(), std::function< void() > run = nullptr )
    {
        auto* rowW = new QFrame;
        rowW->setObjectName( QStringLiteral( "checkRow" ) );
        auto* r = new QHBoxLayout( rowW );
        r->setContentsMargins( 6, 8, 6, 8 );
        static const char* const marks[] = { "✓", "!", "✕", "•" };
        static const char* const colors[] = { "#5fbf7a", "#e5c07b", "#ff5c6c", "#61afef" };
        auto* mark = new QLabel( QStringLiteral( "<span style='font-size:15pt;color:%1'>%2</span>" )
                                     .arg( QString::fromLatin1( colors[ state ] ), QString::fromUtf8( marks[ state ] ) ) );
        mark->setFixedWidth( 30 );
        mark->setAlignment( Qt::AlignCenter );
        auto* text = new QLabel( QStringLiteral( "<b>%1</b><br><span style='color:#8a909c'>%2</span>" ).arg( title.toHtmlEscaped(), detail.toHtmlEscaped() ) );
        text->setWordWrap( true );
        r->addWidget( mark );
        r->addWidget( text, 1 );
        if ( !action.isEmpty() && run )
        {
            auto* b = button( action, QString() );
            connect( b, &QPushButton::clicked, this, run );
            r->addWidget( b );
        }
        m_checks->addWidget( rowW );
    };

    const bool supported = s.flag( QStringLiteral( "supported" ) );
    add( supported ? Ok : Fail,
         tr( "Distribution" ),
         supported ? tr( "%1 - supported (%2 family)" ).arg( s.value( QStringLiteral( "distro" ) ), s.value( QStringLiteral( "familyLabel" ) ) )
                   : tr( "%1 is not mapped to a supported family (Arch, Debian, Fedora, openSUSE, Alpine, Gentoo)." ).arg( s.value( QStringLiteral( "distro" ) ) ) );

    const QString missing = s.value( QStringLiteral( "missingTools" ) );
    add( missing.isEmpty() ? Ok : Fail,
         tr( "Build tools" ),
         missing.isEmpty() ? tr( "mksquashfs, xorriso, mtools and dosfstools are installed." )
                           : tr( "Missing: %1 - run build.sh in the compendium folder." ).arg( missing ) );

    add( s.flag( QStringLiteral( "bootFiles" ) ) ? Ok : Fail,
         tr( "Boot files" ),
         isArch() ? tr( "Embedded claudemods ISO tree (GRUB BIOS/UEFI, theme)." )
                  : s.flag( QStringLiteral( "bootFiles" ) ) ? tr( "BIOS/UEFI bootloaders are installed." )
                                                            : tr( "The bootloaders are not installed - run build.sh in the compendium folder." ) );

    if ( isArch() )
    {
        const bool mk = s.flag( QStringLiteral( "mkinitcpio" ) );
        add( mk ? Ok : Warn,
             tr( "mkinitcpio config" ),
             mk ? tr( "11-dm-initramfs.rules is in /usr/lib/initcpio/udev." ) : tr( "11-dm-initramfs.rules is missing from /usr/lib/initcpio/udev." ),
             mk ? QString() : tr( "Fix" ),
             [ this ]()
             {
                 goTo( Maintenance );
                 m_maintTask->start( tr( "mkinitcpio config" ), { QStringLiteral( "--mkinitcpio-config" ) }, false );
             } );
    }

    const QString installer = s.value( QStringLiteral( "installer" ) );
    const bool installerReady = s.flag( QStringLiteral( "installerReady" ) );
    if ( installer == QLatin1String( "alpine" ) )
    {
        add( installerReady ? Ok : Warn,
             tr( "Alpine installer tools" ),
             installerReady ? tr( "The live ISO can install the clone to a disk (Install page)." )
                            : tr( "sfdisk / GRUB / mkinitfs are missing: the ISO could not install itself. Run build.sh and answer yes." ) );
    }
    else if ( installer != QLatin1String( "none" ) )
    {
        add( installerReady ? Ok : Warn,
             tr( "ClaudeInstall" ),
             installerReady ? tr( "Installed and configured to put this clone on a disk from the live ISO." )
                            : tr( "Not set up: the live ISO would have no installer. Run build.sh in the compendium folder and answer yes to ClaudeInstall." ) );
    }

    const bool erofs = s.value( QStringLiteral( "imageFormat" ) ) == QLatin1String( "erofs" );
    if ( erofs )
    {
        const bool erofsOk = s.flag( QStringLiteral( "mkfsErofs" ) ) && s.flag( QStringLiteral( "erofsKernel" ) );
        add( erofsOk ? Ok : Fail,
             tr( "Image: erofs" ),
             erofsOk ? tr( "mkfs.erofs and kernel support found." ) : tr( "erofs needs mkfs.erofs and kernel support (CONFIG_EROFS_FS) - pick squashfs in Settings." ) );
    }
    else
    {
        add( Ok,
             tr( "Image: squashfs" ),
             s.value( QStringLiteral( "compression" ) ) == QLatin1String( "gzip" )
                 ? tr( "gzip compression." )
                 : tr( "zstd compression, level %1." ).arg( s.value( QStringLiteral( "level" ) ) ) );
    }

    // Free space where the work directory is (or would be created)
    QString dir = s.value( QStringLiteral( "effectiveWorkDir" ) );
    while ( !dir.isEmpty() && !QFileInfo::exists( dir ) )
    {
        dir = QFileInfo( dir ).path();
        if ( dir == QLatin1String( "/" ) )
        {
            break;
        }
    }
    QStorageInfo storage( dir.isEmpty() ? QStringLiteral( "/" ) : dir );
    const qint64 freeBytes = storage.bytesAvailable();
    add( freeBytes > 20LL * 1024 * 1024 * 1024 ? Ok : Warn,
         tr( "Free space" ),
         tr( "%1 free on %2 - a build needs roughly twice the size of the finished ISO." ).arg( humanSize( freeBytes ), storage.rootPath() ) );

    int edited = 0;
    for ( const auto& f : s.customFiles )
    {
        edited += f.edited ? 1 : 0;
    }
    add( Info,
         tr( "Edited configs" ),
         edited ? tr( "%n edited boot/initramfs file(s) will be used by the next build.", nullptr, edited )
                : tr( "None - the build generates GRUB, boot text and initramfs configs itself." ),
         tr( "Edit" ),
         [ this ]() { goTo( Configs ); } );

    // CCCU, the compendium's container creator (cccu/): a separate program with its own window
    const QString cccu = QStandardPaths::findExecutable( QStringLiteral( "cccu-gui" ) );
    add( cccu.isEmpty() ? Info : Ok,
         tr( "CCCU containers" ),
         cccu.isEmpty() ? tr( "Not installed - build.sh builds it on Arch, Debian, Fedora and openSUSE (needs GCC 14+)." )
                        : tr( "Clone this system into an Incus container, or build a live ISO of a container." ),
         cccu.isEmpty() ? QString() : tr( "Open" ),
         [ this, cccu ]()
         {
             if ( !QProcess::startDetached( cccu, {} ) )
             {
                 QMessageBox::warning( this, tr( "CCCU" ), tr( "%1 could not be started." ).arg( cccu ) );
             }
         } );
    m_checks->addStretch( 1 );
}

// ============================================================================================
// Build ISO
// ============================================================================================
QWidget*
MainWindow::makeBuildPage()
{
    auto* page = new QWidget;
    auto* l = new QVBoxLayout( page );
    l->setSpacing( 14 );
    l->addWidget( pageHeader( tr( "Build ISO" ),
                              tr( "Everything is unmounted at the end, also when the build fails or is cancelled." ) ) );

    m_buildSummary = new QLabel;
    m_buildSummary->setObjectName( QStringLiteral( "summary" ) );
    m_buildSummary->setWordWrap( true );
    m_buildSummary->setTextInteractionFlags( Qt::TextSelectableByMouse );
    l->addWidget( m_buildSummary );

    auto* split = new QSplitter( Qt::Horizontal );
    auto* stepsCard = card();
    auto* sc = new QVBoxLayout( stepsCard );
    sc->setContentsMargins( 14, 14, 14, 14 );
    sc->addWidget( sectionLabel( tr( "Steps" ) ) );
    m_stepList = new QListWidget;
    m_stepList->setObjectName( QStringLiteral( "steps" ) );
    m_stepList->setSelectionMode( QAbstractItemView::NoSelection );
    m_stepList->setFocusPolicy( Qt::NoFocus );
    sc->addWidget( m_stepList, 1 );
    stepsCard->setMinimumWidth( 250 );

    auto* taskCard = card();
    auto* tc = new QVBoxLayout( taskCard );
    tc->setContentsMargins( 14, 14, 14, 14 );
    m_buildTask = new TaskPanel( m_backend );
    tc->addWidget( m_buildTask );
    split->addWidget( stepsCard );
    split->addWidget( taskCard );
    split->setStretchFactor( 0, 0 );
    split->setStretchFactor( 1, 1 );
    split->setSizes( { 290, 800 } );
    l->addWidget( split, 1 );

    auto* buttons = new QHBoxLayout;
    m_buildButton = button( tr( "Build ISO" ), QStringLiteral( "media-record" ), "primary" );
    m_openOutput = button( tr( "Open Output Folder" ), QStringLiteral( "folder-open" ) );
    m_toUsb = button( tr( "Write to USB..." ), QStringLiteral( "drive-removable-media" ) );
    buttons->addWidget( m_buildButton );
    buttons->addStretch( 1 );
    buttons->addWidget( m_openOutput );
    buttons->addWidget( m_toUsb );
    l->addLayout( buttons );

    connect( m_buildButton, &QPushButton::clicked, this, &MainWindow::startBuild );
    connect( m_openOutput, &QPushButton::clicked, this, [ this ]() { QDesktopServices::openUrl( QUrl::fromLocalFile( isoOutputDir() ) ); } );
    connect( m_toUsb, &QPushButton::clicked, this, [ this ]() { goTo( Usb ); } );

    auto mark = [ this ]( int row, const QString& symbol, const QString& color )
    {
        if ( QListWidgetItem* it = m_stepList->item( row ) )
        {
            it->setText( symbol + QStringLiteral( "  " ) + it->data( Qt::UserRole ).toString() );
            it->setForeground( QColor( color ) );
        }
    };
    connect( m_buildTask,
             &TaskPanel::stepStarted,
             this,
             [ this, mark ]( int index, const QString& )
             {
                 for ( int i = 0; i < m_stepList->count(); ++i )
                 {
                     if ( i < index )
                     {
                         mark( i, QStringLiteral( "✓" ), QStringLiteral( "#5fbf7a" ) );
                     }
                     else if ( i == index )
                     {
                         mark( i, QStringLiteral( "▶" ), QStringLiteral( "#ffffff" ) );
                         m_stepList->scrollToItem( m_stepList->item( i ) );
                     }
                 }
             } );
    connect( m_buildTask,
             &TaskPanel::finished,
             this,
             [ this, mark ]( bool ok, bool )
             {
                 for ( int i = 0; i < m_stepList->count(); ++i )
                 {
                     if ( m_stepList->item( i )->text().startsWith( QStringLiteral( "▶" ) ) )
                     {
                         mark( i, ok ? QStringLiteral( "✓" ) : QStringLiteral( "✕" ), ok ? QStringLiteral( "#5fbf7a" ) : QStringLiteral( "#ff5c6c" ) );
                     }
                     else if ( ok )
                     {
                         mark( i, QStringLiteral( "✓" ), QStringLiteral( "#5fbf7a" ) );
                     }
                 }
                 m_buildButton->setEnabled( true );
                 refreshStatus();
                 if ( ok )
                 {
                     QMessageBox::information( this,
                                               tr( "ISO ready" ),
                                               tr( "The ISO was built in %1.\n\nTest it in a virtual machine (UEFI) before installing it anywhere." )
                                                   .arg( isoOutputDir() ) );
                 }
             } );
    return pageFrame( page );
}

void
MainWindow::fillBuild()
{
    const ClonerStatus& s = m_status;
    const bool erofs = s.value( QStringLiteral( "imageFormat" ) ) == QLatin1String( "erofs" );
    const QString image = erofs ? tr( "erofs" )
        : s.value( QStringLiteral( "compression" ) ) == QLatin1String( "gzip" )
        ? tr( "squashfs, gzip" )
        : tr( "squashfs, zstd level %1" ).arg( s.value( QStringLiteral( "level" ) ) );
    m_buildSummary->setText( tr( "<b>ISO:</b> %1<br><b>Image:</b> %2 &nbsp; <b>RAM mode entry:</b> %3 &nbsp; <b>Package caches:</b> %4"
                                 "<br><b>Clone directory:</b> %5 &nbsp; <b>Work directory:</b> %6" )
                                 .arg( s.value( QStringLiteral( "isoPath" ) ).toHtmlEscaped(),
                                       image,
                                       s.flag( QStringLiteral( "ramMode" ) ) ? tr( "yes" ) : tr( "no" ),
                                       s.flag( QStringLiteral( "cleanCache" ) ) ? tr( "left out" ) : tr( "included" ),
                                       s.value( QStringLiteral( "effectiveCloneDir" ) ).toHtmlEscaped(),
                                       s.value( QStringLiteral( "effectiveWorkDir" ) ).toHtmlEscaped() ) );
    // Keep the marks of the running or last build while the plan is the same (startBuild clears them)
    bool same = m_stepList->count() == m_plan.size();
    for ( int i = 0; same && i < m_plan.size(); ++i )
    {
        same = m_stepList->item( i )->data( Qt::UserRole ).toString() == m_plan[ i ].name;
    }
    if ( same || m_buildTask->isActive() )
    {
        return;
    }
    m_stepList->clear();
    for ( const PlanStep& p : m_plan )
    {
        auto* it = new QListWidgetItem( QStringLiteral( "○  " ) + p.name );
        it->setData( Qt::UserRole, p.name );
        it->setToolTip( p.description );
        it->setForeground( QColor( "#8a909c" ) );
        m_stepList->addItem( it );
    }
}

void
MainWindow::startBuild()
{
    if ( !m_status.ok || !m_status.flag( QStringLiteral( "supported" ) ) )
    {
        QMessageBox::warning( this, tr( "Build ISO" ), tr( "This system is not supported, or the cloner is not installed." ) );
        return;
    }
    if ( !m_status.value( QStringLiteral( "missingTools" ) ).isEmpty() )
    {
        QMessageBox::warning( this, tr( "Build ISO" ), tr( "Missing tools: %1\n\nRun build.sh in the compendium folder first." ).arg( m_status.value( QStringLiteral( "missingTools" ) ) ) );
        return;
    }
    if ( m_status.value( QStringLiteral( "installer" ) ) == QLatin1String( "claudeinstall" ) && !m_status.flag( QStringLiteral( "installerReady" ) )
         && QMessageBox::question( this,
                                   tr( "Build ISO" ),
                                   tr( "ClaudeInstall is not set up, so the live ISO will not be able to install itself to a disk.\n\nBuild anyway?" ) )
             != QMessageBox::Yes )
    {
        return;
    }
    if ( m_settingsDirty
         && QMessageBox::question( this, tr( "Build ISO" ), tr( "The Settings page has unsaved changes. Save them before building?" ) ) == QMessageBox::Yes )
    {
        saveSettings();
    }
    if ( QMessageBox::question( this,
                                tr( "Build ISO" ),
                                tr( "Clone the whole running system - users, passwords, settings and /home - into\n%1 ?\n\n"
                                    "Anyone with the ISO can read those files." )
                                    .arg( m_status.value( QStringLiteral( "isoPath" ) ) ) )
         != QMessageBox::Yes )
    {
        return;
    }
    goTo( Build );
    m_stepList->clear();   // fresh marks for this build
    fillBuild();
    QStringList names;
    for ( const PlanStep& p : m_plan )
    {
        names << p.name;
    }
    if ( m_buildTask->start( tr( "Building the ISO" ), { QStringLiteral( "--build-iso" ) }, true, names ) )
    {
        m_buildButton->setEnabled( false );
    }
}

// ============================================================================================
// Settings
// ============================================================================================
QWidget*
MainWindow::makeSettingsPage()
{
    auto* page = new QWidget;
    auto* l = new QVBoxLayout( page );
    l->setSpacing( 14 );
    l->addWidget( pageHeader( tr( "Settings" ), tr( "Saved in ~/.config/claudebackup-linux/configuration.txt - the terminal menu uses the same file." ) ) );

    auto* c = card();
    auto* cl = new QVBoxLayout( c );
    cl->setContentsMargins( 22, 18, 22, 18 );
    m_form = new QFormLayout;
    m_form->setLabelAlignment( Qt::AlignRight | Qt::AlignVCenter );
    m_form->setHorizontalSpacing( 18 );
    m_form->setVerticalSpacing( 12 );

    auto dirty = [ this ]() { m_settingsDirty = true; };
    auto withBrowse = [ this, dirty ]( QLineEdit* edit, bool cloneParent ) -> QWidget*
    {
        auto* w = new QWidget;
        auto* h = new QHBoxLayout( w );
        h->setContentsMargins( 0, 0, 0, 0 );
        h->addWidget( edit, 1 );
        auto* b = button( tr( "Browse..." ), QStringLiteral( "folder-open" ) );
        h->addWidget( b );
        connect( edit, &QLineEdit::textEdited, this, dirty );
        connect( b,
                 &QPushButton::clicked,
                 this,
                 [ this, edit, cloneParent, dirty ]()
                 {
                     const QString start = edit->text().isEmpty() ? QDir::homePath() : edit->text();
                     QString d = QFileDialog::getExistingDirectory( this, tr( "Choose a folder" ), start );
                     if ( d.isEmpty() )
                     {
                         return;
                     }
                     // the system is bind-mounted into <parent>/clone_system_temp, as in the terminal menu
                     if ( cloneParent && !d.endsWith( QLatin1String( "/clone_system_temp" ) ) )
                     {
                         d += QStringLiteral( "/clone_system_temp" );
                     }
                     edit->setText( d );
                     dirty();
                 } );
        return w;
    };

    m_isoName = new QLineEdit;
    m_isoName->setPlaceholderText( tr( "automatic: claudebackup-<distro>-<date>.iso" ) );
    connect( m_isoName, &QLineEdit::textEdited, this, dirty );
    m_form->addRow( tr( "ISO name" ), m_isoName );

    m_workDir = new QLineEdit;
    m_form->addRow( tr( "Work directory" ), withBrowse( m_workDir, false ) );
    m_outputDir = new QLineEdit;
    m_outputDir->setPlaceholderText( tr( "the work directory" ) );
    m_form->addRow( tr( "Output directory" ), withBrowse( m_outputDir, false ) );
    m_cloneDir = new QLineEdit;
    m_form->addRow( tr( "Clone directory" ), withBrowse( m_cloneDir, true ) );

    m_isoTag = new QLineEdit;
    m_isoTag->setPlaceholderText( QStringLiteral( "2026" ) );
    m_isoTag->setValidator( new QRegularExpressionValidator( QRegularExpression( QStringLiteral( "[^\\s\"']*" ) ), m_isoTag ) );
    connect( m_isoTag, &QLineEdit::textEdited, this, dirty );
    m_form->addRow( tr( "ISO tag (volume label)" ), m_isoTag );

    m_imageFormat = new QComboBox;
    m_imageFormat->addItem( tr( "squashfs (default)" ), QStringLiteral( "squashfs" ) );
    m_imageFormat->addItem( tr( "erofs" ), QStringLiteral( "erofs" ) );
    m_form->addRow( tr( "Image type" ), m_imageFormat );

    m_level = new QSpinBox;
    m_level->setRange( 1, 22 );
    m_level->setToolTip( tr( "Higher = smaller ISO, slower build. Default 3." ) );
    m_form->addRow( tr( "zstd compression level" ), m_level );

    m_ramMode = new QCheckBox( tr( "Add a \"RAM mode\" boot entry (copies the system to RAM at boot)" ) );
    m_form->addRow( QString(), m_ramMode );
    m_cleanCache = new QCheckBox( tr( "Leave package caches, /var/tmp and ~/.cache out of the image" ) );
    m_form->addRow( QString(), m_cleanCache );

    connect( m_imageFormat,
             &QComboBox::currentIndexChanged,
             this,
             [ this, dirty ]( int )
             {
                 m_level->setEnabled( m_imageFormat->currentData().toString() == QLatin1String( "squashfs" ) );
                 dirty();
             } );
    connect( m_level, &QSpinBox::valueChanged, this, dirty );
    connect( m_ramMode, &QCheckBox::toggled, this, dirty );
    connect( m_cleanCache, &QCheckBox::toggled, this, dirty );

    cl->addLayout( m_form );
    m_settingsNote = new QLabel;
    m_settingsNote->setObjectName( QStringLiteral( "muted" ) );
    m_settingsNote->setWordWrap( true );
    cl->addWidget( m_settingsNote );
    l->addWidget( c );

    auto* buttons = new QHBoxLayout;
    auto* save = button( tr( "Save" ), QStringLiteral( "document-save" ), "primary" );
    auto* revert = button( tr( "Revert" ), QStringLiteral( "edit-undo" ) );
    buttons->addStretch( 1 );
    buttons->addWidget( revert );
    buttons->addWidget( save );
    l->addLayout( buttons );
    l->addStretch( 1 );
    connect( save, &QPushButton::clicked, this, &MainWindow::saveSettings );
    connect( revert,
             &QPushButton::clicked,
             this,
             [ this ]()
             {
                 m_settingsDirty = false;
                 fillSettings();
             } );
    return pageFrame( page, true );
}

void
MainWindow::fillSettings()
{
    const ClonerStatus& s = m_status;
    const QString family = s.family();
    auto showRow = [ this ]( QWidget* field, bool visible )
    {
        field->setVisible( visible );
        if ( QWidget* label = m_form->labelForField( field ) )
        {
            label->setVisible( visible );
        }
    };
    showRow( m_isoTag, isArch() );
    showRow( m_level, family != QLatin1String( "gentoo" ) );

    // erofs only where the family supports it, and only selectable when mkfs.erofs + the kernel can do it
    auto* model = qobject_cast< QStandardItemModel* >( m_imageFormat->model() );
    const bool erofsPossible = s.flag( QStringLiteral( "erofsSupported" ) ) && s.flag( QStringLiteral( "mkfsErofs" ) ) && s.flag( QStringLiteral( "erofsKernel" ) );
    if ( model && model->item( 1 ) )
    {
        model->item( 1 )->setEnabled( erofsPossible );
        model->item( 1 )->setToolTip( erofsPossible ? QString()
                                      : !s.flag( QStringLiteral( "erofsSupported" ) )
                                      ? tr( "%1 only builds squashfs images." ).arg( s.value( QStringLiteral( "distro" ) ) )
                                      : tr( "Needs mkfs.erofs (erofs-utils) and a kernel with erofs support." ) );
    }

    if ( m_settingsDirty )
    {
        return;   // do not overwrite what the user is editing
    }
    const QSignalBlocker b1( m_imageFormat ), b2( m_level ), b3( m_ramMode ), b4( m_cleanCache );
    m_isoName->setText( s.value( QStringLiteral( "isoName" ) ) );
    m_workDir->setText( s.value( QStringLiteral( "workDir" ) ) );
    m_workDir->setPlaceholderText( s.value( QStringLiteral( "defaultWorkDir" ) ) );
    m_outputDir->setText( s.value( QStringLiteral( "outputDir" ) ) );
    m_cloneDir->setText( s.value( QStringLiteral( "cloneDir" ) ) );
    m_cloneDir->setPlaceholderText( s.value( QStringLiteral( "defaultCloneDir" ) ) );
    m_isoTag->setText( s.value( QStringLiteral( "isoTag" ) ) );
    m_imageFormat->setCurrentIndex( s.value( QStringLiteral( "imageFormat" ) ) == QLatin1String( "erofs" ) ? 1 : 0 );
    m_level->setValue( qBound( 1, s.value( QStringLiteral( "level" ) ).toInt(), 22 ) );
    m_level->setEnabled( m_imageFormat->currentIndex() == 0 );
    m_ramMode->setChecked( s.flag( QStringLiteral( "ramMode" ) ) );
    m_cleanCache->setChecked( s.flag( QStringLiteral( "cleanCache" ) ) );
    m_settingsNote->setText( tr( "Blank fields use the defaults shown. Boot parameters for this distro: %1" ).arg( s.value( QStringLiteral( "bootParams" ) ) ) );
}

void
MainWindow::saveSettings()
{
    QStringList kv;
    kv << QStringLiteral( "isoName=" ) + m_isoName->text().trimmed();
    kv << QStringLiteral( "workDir=" ) + m_workDir->text().trimmed();
    kv << QStringLiteral( "outputDir=" ) + m_outputDir->text().trimmed();
    kv << QStringLiteral( "cloneDir=" ) + m_cloneDir->text().trimmed();
    if ( isArch() )
    {
        kv << QStringLiteral( "isoTag=" ) + m_isoTag->text().trimmed();
    }
    kv << QStringLiteral( "imageFormat=" ) + m_imageFormat->currentData().toString();
    if ( m_status.family() != QLatin1String( "gentoo" ) )
    {
        kv << QStringLiteral( "level=%1" ).arg( m_level->value() );
    }
    kv << QStringLiteral( "ramMode=" ) + ( m_ramMode->isChecked() ? QStringLiteral( "1" ) : QStringLiteral( "0" ) );
    kv << QStringLiteral( "cleanCache=" ) + ( m_cleanCache->isChecked() ? QStringLiteral( "1" ) : QStringLiteral( "0" ) );

    QString error;
    if ( !m_backend->set( kv, &error ) )
    {
        QMessageBox::warning( this, tr( "Settings" ), error );
        return;
    }
    m_settingsDirty = false;
    refreshStatus();
}

// ============================================================================================
// Edit Configs
// ============================================================================================
QWidget*
MainWindow::makeConfigsPage()
{
    auto* page = new QWidget;
    auto* l = new QVBoxLayout( page );
    l->setSpacing( 14 );
    l->addWidget( pageHeader( tr( "Edit Configs" ),
                              tr( "Boot and initramfs files are your own copies, used by every build until reset. "
                                  "ClaudeInstall's files are the system's own: they go into the clone and decide how it installs." ) ) );

    auto* split = new QSplitter( Qt::Horizontal );
    auto* listCard = card();
    auto* lc = new QVBoxLayout( listCard );
    lc->setContentsMargins( 12, 12, 12, 12 );
    m_configList = new QListWidget;
    m_configList->setObjectName( QStringLiteral( "configList" ) );
    lc->addWidget( m_configList );
    listCard->setMinimumWidth( 270 );

    auto* editCard = card();
    auto* ec = new QVBoxLayout( editCard );
    ec->setContentsMargins( 14, 12, 14, 12 );
    m_configPath = new QLabel( tr( "Choose a file on the left." ) );
    m_configPath->setObjectName( QStringLiteral( "muted" ) );
    m_configPath->setTextInteractionFlags( Qt::TextSelectableByMouse );
    ec->addWidget( m_configPath );
    m_editor = new QPlainTextEdit;
    m_editor->setObjectName( QStringLiteral( "editor" ) );
    m_editor->setFont( QFontDatabase::systemFont( QFontDatabase::FixedFont ) );
    m_editor->setLineWrapMode( QPlainTextEdit::NoWrap );
    m_editor->setEnabled( false );
    ec->addWidget( m_editor, 1 );
    auto* buttons = new QHBoxLayout;
    m_configCreate = button( tr( "Create My Copy" ), QStringLiteral( "document-new" ), "primary" );
    m_configReset = button( tr( "Reset To Generated" ), QStringLiteral( "edit-delete" ), "danger" );
    m_configReload = button( tr( "Reload" ), QStringLiteral( "view-refresh" ) );
    m_configSave = button( tr( "Save" ), QStringLiteral( "document-save" ), "primary" );
    buttons->addWidget( m_configCreate );
    buttons->addWidget( m_configReset );
    buttons->addStretch( 1 );
    buttons->addWidget( m_configReload );
    buttons->addWidget( m_configSave );
    ec->addLayout( buttons );
    m_configTask = new TaskPanel( m_backend );
    m_configTask->setMaximumHeight( 170 );
    m_configTask->setVisible( false );
    ec->addWidget( m_configTask );
    for ( QPushButton* b : { m_configCreate, m_configReset, m_configReload, m_configSave } )
    {
        b->setVisible( false );
    }

    split->addWidget( listCard );
    split->addWidget( editCard );
    split->setStretchFactor( 1, 1 );
    split->setSizes( { 300, 800 } );
    l->addWidget( split, 1 );

    connect( m_editor, &QPlainTextEdit::textChanged, this, [ this ]() { m_editorDirty = m_editor->isEnabled(); } );
    connect( m_configList,
             &QListWidget::itemClicked,
             this,
             [ this ]( QListWidgetItem* it )
             {
                 if ( it != m_configCurrent )
                 {
                     loadConfigItem( it );
                 }
             } );
    connect( m_configReload,
             &QPushButton::clicked,
             this,
             [ this ]()
             {
                 if ( m_configCurrent && confirmDiscardEdits() )
                 {
                     m_editorDirty = false;
                     QListWidgetItem* it = m_configCurrent;
                     m_configCurrent = nullptr;
                     loadConfigItem( it );
                 }
             } );
    connect( m_configSave, &QPushButton::clicked, this, &MainWindow::saveConfigItem );
    connect( m_configCreate,
             &QPushButton::clicked,
             this,
             [ this ]()
             {
                 if ( !m_configCurrent )
                 {
                     return;
                 }
                 const QString name = m_configCurrent->data( Qt::UserRole + 1 ).toString();
                 m_configTask->setVisible( true );
                 // finished arrives later, so connecting after a successful start never misses it
                 if ( !m_configTask->start( tr( "Creating your copy of %1" ).arg( name ), { QStringLiteral( "--seed-custom" ), name }, false ) )
                 {
                     return;
                 }
                 connect(
                     m_configTask,
                     &TaskPanel::finished,
                     this,
                     [ this ]( bool ok, bool )
                     {
                         if ( ok )
                         {
                             refreshStatus();
                             QListWidgetItem* it = m_configCurrent;
                             m_configCurrent = nullptr;
                             m_editorDirty = false;
                             loadConfigItem( it );
                         }
                     },
                     Qt::SingleShotConnection );
             } );
    connect( m_configReset,
             &QPushButton::clicked,
             this,
             [ this ]()
             {
                 if ( !m_configCurrent )
                 {
                     return;
                 }
                 const QString name = m_configCurrent->data( Qt::UserRole + 1 ).toString();
                 if ( QMessageBox::question( this, tr( "Reset" ), tr( "Delete your copy of %1? The next build generates it again." ).arg( name ) )
                      != QMessageBox::Yes )
                 {
                     return;
                 }
                 QFile::remove( m_status.value( QStringLiteral( "customDir" ) ) + QLatin1Char( '/' ) + name );
                 m_editorDirty = false;
                 refreshStatus();
                 QListWidgetItem* it = m_configCurrent;
                 m_configCurrent = nullptr;
                 loadConfigItem( it );
             } );
    return pageFrame( page );
}

void
MainWindow::fillConfigs()
{
    auto header = [ this ]( const QString& text )
    {
        auto* it = new QListWidgetItem( text );
        it->setFlags( Qt::NoItemFlags );
        it->setData( Qt::UserRole, QStringLiteral( "header" ) );
        QFont f = it->font();
        f.setBold( true );
        it->setFont( f );
        it->setForeground( QColor( "#d7263d" ) );
        m_configList->addItem( it );
    };
    auto customText = []( const ClonerStatus::CustomFile& f ) { return f.label + ( f.edited ? QStringLiteral( "   ✎" ) : QString() ); };

    if ( m_configList->count() > 0 )
    {
        // already listed: only the "edited" marks change
        for ( int i = 0; i < m_configList->count(); ++i )
        {
            QListWidgetItem* it = m_configList->item( i );
            if ( it->data( Qt::UserRole ).toString() != QLatin1String( "custom" ) )
            {
                continue;
            }
            for ( const auto& f : m_status.customFiles )
            {
                if ( f.name == it->data( Qt::UserRole + 1 ).toString() )
                {
                    it->setText( customText( f ) );
                }
            }
        }
        return;
    }
    header( tr( "Boot & initramfs (next build)" ) );
    for ( const auto& f : m_status.customFiles )
    {
        auto* it = new QListWidgetItem( customText( f ) );
        it->setData( Qt::UserRole, QStringLiteral( "custom" ) );
        it->setData( Qt::UserRole + 1, f.name );
        m_configList->addItem( it );
    }
    if ( !m_status.installerFiles.isEmpty() )
    {
        header( tr( "ClaudeInstall (installs the clone)" ) );
        for ( const auto& f : m_status.installerFiles )
        {
            auto* it = new QListWidgetItem( f.label );
            it->setData( Qt::UserRole, QStringLiteral( "installer" ) );
            it->setData( Qt::UserRole + 1, f.path );
            it->setToolTip( f.path );
            m_configList->addItem( it );
        }
    }
}

bool
MainWindow::confirmDiscardEdits()
{
    if ( !m_editorDirty )
    {
        return true;
    }
    return QMessageBox::question( this, tr( "Unsaved changes" ), tr( "Discard the changes to this file?" ) ) == QMessageBox::Yes;
}

void
MainWindow::loadConfigItem( QListWidgetItem* item )
{
    if ( !item || item->data( Qt::UserRole ).toString() == QLatin1String( "header" ) || !confirmDiscardEdits() )
    {
        if ( m_configCurrent )
        {
            m_configList->setCurrentItem( m_configCurrent );
        }
        return;
    }
    m_configCurrent = item;
    m_configList->setCurrentItem( item );
    const bool custom = item->data( Qt::UserRole ).toString() == QLatin1String( "custom" );
    const QString key = item->data( Qt::UserRole + 1 ).toString();
    const QString path = custom ? m_status.value( QStringLiteral( "customDir" ) ) + QLatin1Char( '/' ) + key : key;
    const bool exists = QFileInfo( path ).isFile();

    m_configPath->setText( path + ( custom ? tr( "   (your copy, used by the next build)" ) : tr( "   (saved as administrator, keeps a .bak)" ) ) );
    m_editor->setEnabled( false );
    if ( exists )
    {
        QFile f( path );
        m_editor->setPlainText( f.open( QIODevice::ReadOnly ) ? QString::fromUtf8( f.readAll() ) : QString() );
        m_editor->setEnabled( true );
    }
    else
    {
        m_editor->setPlainText( custom ? tr( "No copy yet.\n\n\"Create My Copy\" makes one from what the build generates for %1." ).arg( m_status.value( QStringLiteral( "distro" ) ) )
                                       : tr( "Not installed. Run build.sh in the compendium folder and answer yes to ClaudeInstall." ) );
    }
    m_editorDirty = false;
    m_configCreate->setVisible( custom && !exists );
    m_configReset->setVisible( custom && exists );
    m_configReload->setVisible( exists );
    m_configSave->setVisible( exists );
}

void
MainWindow::saveConfigItem()
{
    if ( !m_configCurrent )
    {
        return;
    }
    const bool custom = m_configCurrent->data( Qt::UserRole ).toString() == QLatin1String( "custom" );
    const QString key = m_configCurrent->data( Qt::UserRole + 1 ).toString();
    const QByteArray content = m_editor->toPlainText().toUtf8();
    if ( custom )
    {
        QSaveFile f( m_status.value( QStringLiteral( "customDir" ) ) + QLatin1Char( '/' ) + key );
        if ( !f.open( QIODevice::WriteOnly ) || f.write( content ) != content.size() || !f.commit() )
        {
            QMessageBox::warning( this, tr( "Save" ), tr( "Could not write %1: %2" ).arg( f.fileName(), f.errorString() ) );
            return;
        }
        m_editorDirty = false;
        refreshStatus();
        return;
    }
    m_configTask->setVisible( true );
    if ( !m_configTask->start( tr( "Saving %1" ).arg( key ), { QStringLiteral( "--write-system-file" ), key }, false, {}, 0, content ) )
    {
        return;
    }
    connect(
        m_configTask,
        &TaskPanel::finished,
        this,
        [ this ]( bool ok, bool )
        {
            if ( ok )
            {
                m_editorDirty = false;
            }
        },
        Qt::SingleShotConnection );
}

// ============================================================================================
// Write USB
// ============================================================================================
QWidget*
MainWindow::makeUsbPage()
{
    auto* page = new QWidget;
    auto* l = new QVBoxLayout( page );
    l->setSpacing( 14 );
    l->addWidget( pageHeader( tr( "Write USB" ), tr( "Writes the ISO to a whole drive. Everything on that drive is erased." ) ) );

    auto* c = card();
    auto* form = new QFormLayout( c );
    form->setContentsMargins( 22, 18, 22, 18 );
    form->setHorizontalSpacing( 18 );
    form->setVerticalSpacing( 12 );

    auto* isoRow = new QHBoxLayout;
    m_isoCombo = new QComboBox;
    m_isoCombo->setMinimumWidth( 420 );
    auto* browse = button( tr( "Browse..." ), QStringLiteral( "document-open" ) );
    isoRow->addWidget( m_isoCombo, 1 );
    isoRow->addWidget( browse );
    form->addRow( tr( "ISO" ), isoRow );

    auto* driveRow = new QHBoxLayout;
    m_driveCombo = new QComboBox;
    auto* refresh = button( tr( "Refresh" ), QStringLiteral( "view-refresh" ) );
    driveRow->addWidget( m_driveCombo, 1 );
    driveRow->addWidget( refresh );
    form->addRow( tr( "Drive" ), driveRow );

    auto* write = button( tr( "Write ISO To Drive" ), QStringLiteral( "media-flash" ), "danger" );
    form->addRow( QString(), write );
    l->addWidget( c );

    auto* tc = card();
    auto* tl = new QVBoxLayout( tc );
    tl->setContentsMargins( 14, 14, 14, 14 );
    m_usbTask = new TaskPanel( m_backend );
    tl->addWidget( m_usbTask );
    l->addWidget( tc, 1 );

    connect( browse,
             &QPushButton::clicked,
             this,
             [ this ]()
             {
                 const QString f = QFileDialog::getOpenFileName( this, tr( "Choose an ISO" ), isoOutputDir(), tr( "ISO images (*.iso)" ) );
                 if ( !f.isEmpty() )
                 {
                     m_isoCombo->insertItem( 0, QFileInfo( f ).fileName() + QStringLiteral( "  (%1)" ).arg( humanSize( QFileInfo( f ).size() ) ), f );
                     m_isoCombo->setCurrentIndex( 0 );
                 }
             } );
    connect( refresh, &QPushButton::clicked, this, [ this ]() { refreshDrives( m_driveCombo, true ); } );
    connect( write, &QPushButton::clicked, this, &MainWindow::writeUsb );
    return pageFrame( page );
}

void
MainWindow::refreshIsos()
{
    const QString keep = m_isoCombo->currentData().toString();
    m_isoCombo->clear();
    const QFileInfoList isos = QDir( isoOutputDir() ).entryInfoList( { QStringLiteral( "*.iso" ) }, QDir::Files, QDir::Time );
    for ( const QFileInfo& fi : isos )
    {
        m_isoCombo->addItem( QIcon::fromTheme( QStringLiteral( "media-optical" ) ),
                             fi.fileName() + QStringLiteral( "  (%1, %2)" ).arg( humanSize( fi.size() ), QLocale().toString( fi.lastModified(), QLocale::ShortFormat ) ),
                             fi.absoluteFilePath() );
    }
    if ( isos.isEmpty() )
    {
        m_isoCombo->addItem( tr( "No ISO in %1 yet - build one, or Browse..." ).arg( isoOutputDir() ), QString() );
    }
    const int i = m_isoCombo->findData( keep );
    if ( i >= 0 )
    {
        m_isoCombo->setCurrentIndex( i );
    }
}

void
MainWindow::refreshDrives( QComboBox* combo, bool removableFirst )
{
    const QString keep = combo->currentData().toString();
    combo->clear();
    const QJsonDocument doc = QJsonDocument::fromJson(
        runCapture( QStringLiteral( "lsblk" ), { QStringLiteral( "-J" ), QStringLiteral( "-b" ), QStringLiteral( "-d" ), QStringLiteral( "-o" ), QStringLiteral( "NAME,PATH,SIZE,MODEL,TRAN,RM,HOTPLUG,TYPE" ) } )
            .toUtf8() );
    const QStringList system = systemDisks();
    struct Drive
    {
        QString path, text;
        bool removable, isSystem;
    };
    QList< Drive > drives;
    for ( const QJsonValue& v : doc.object().value( QStringLiteral( "blockdevices" ) ).toArray() )
    {
        const QJsonObject o = v.toObject();
        if ( jsonText( o.value( QStringLiteral( "type" ) ) ) != QLatin1String( "disk" ) )
        {
            continue;
        }
        QString path = jsonText( o.value( QStringLiteral( "path" ) ) );
        if ( path.isEmpty() )
        {
            path = QStringLiteral( "/dev/" ) + jsonText( o.value( QStringLiteral( "name" ) ) );
        }
        if ( path.startsWith( QLatin1String( "/dev/zram" ) ) || path.startsWith( QLatin1String( "/dev/loop" ) ) )
        {
            continue;
        }
        const bool removable = jsonText( o.value( QStringLiteral( "rm" ) ) ) == QLatin1String( "1" ) || jsonText( o.value( QStringLiteral( "hotplug" ) ) ) == QLatin1String( "1" )
            || jsonText( o.value( QStringLiteral( "tran" ) ) ) == QLatin1String( "usb" );
        const QString model = jsonText( o.value( QStringLiteral( "model" ) ) ).trimmed();
        const bool isSystem = system.contains( path );
        // lsblk -b: a number in newer util-linux, a string in older ones
        const QJsonValue sizeValue = o.value( QStringLiteral( "size" ) );
        const qint64 size = sizeValue.isDouble() ? qint64( sizeValue.toDouble() ) : sizeValue.toString().toLongLong();
        QString text = QStringLiteral( "%1   %2   %3" ).arg( path, humanSize( size ), model.isEmpty() ? tr( "(no model name)" ) : model );
        if ( removable )
        {
            text += tr( "   [removable]" );
        }
        if ( isSystem )
        {
            text += tr( "   [running system - not selectable]" );
        }
        drives.append( { path, text, removable, isSystem } );
    }
    if ( removableFirst )
    {
        std::stable_sort( drives.begin(), drives.end(), []( const Drive& a, const Drive& b ) { return a.removable && !b.removable; } );
    }
    auto* model = qobject_cast< QStandardItemModel* >( combo->model() );
    for ( const Drive& d : drives )
    {
        combo->addItem( QIcon::fromTheme( d.removable ? QStringLiteral( "drive-removable-media-usb" ) : QStringLiteral( "drive-harddisk" ) ), d.text, d.path );
        if ( d.isSystem && model )
        {
            model->item( combo->count() - 1 )->setEnabled( false );
        }
    }
    if ( drives.isEmpty() )
    {
        combo->addItem( tr( "No drives found" ), QString() );
    }
    int i = combo->findData( keep );
    if ( i < 0 )
    {
        // first selectable drive
        for ( int j = 0; j < drives.size(); ++j )
        {
            if ( !drives[ j ].isSystem )
            {
                i = j;
                break;
            }
        }
    }
    combo->setCurrentIndex( qMax( 0, i ) );
}

void
MainWindow::writeUsb()
{
    const QString iso = m_isoCombo->currentData().toString();
    const QString drive = m_driveCombo->currentData().toString();
    if ( iso.isEmpty() || drive.isEmpty() )
    {
        QMessageBox::information( this, tr( "Write USB" ), tr( "Choose an ISO and a drive first." ) );
        return;
    }
    QMessageBox box( QMessageBox::Warning,
                     tr( "Erase %1?" ).arg( drive ),
                     tr( "Everything on\n\n%1\n\nwill be erased and replaced by %2." ).arg( m_driveCombo->currentText(), QFileInfo( iso ).fileName() ),
                     QMessageBox::Cancel,
                     this );
    QPushButton* erase = box.addButton( tr( "Erase And Write" ), QMessageBox::DestructiveRole );
    box.setDefaultButton( QMessageBox::Cancel );
    box.exec();
    if ( box.clickedButton() != erase )
    {
        return;
    }
    m_usbTask->start( tr( "Writing %1 to %2" ).arg( QFileInfo( iso ).fileName(), drive ), { QStringLiteral( "--write-usb" ), iso, drive }, true, {}, QFileInfo( iso ).size() );
}

// ============================================================================================
// Install
// ============================================================================================
QWidget*
MainWindow::makeInstallPage()
{
    auto* page = new QWidget;
    auto* l = new QVBoxLayout( page );
    l->setSpacing( 14 );
    l->addWidget( pageHeader( tr( "Install" ),
                              tr( "In the live ISO: put the cloned system on a disk - your users, settings and /home, not a fresh install. "
                                  "The Desktop shortcut \"Install ...\" does the same." ) ) );

    m_installStack = new QStackedWidget;

    // 0: ClaudeInstall
    auto* ci = card();
    auto* cil = new QVBoxLayout( ci );
    cil->setContentsMargins( 22, 18, 22, 18 );
    cil->setSpacing( 10 );
    cil->addWidget( sectionLabel( tr( "ClaudeInstall" ) ) );
    auto* about = new QLabel( tr( "ClaudeInstall partitions the disk (ext4 or btrfs, erase or manual), unpacks the ISO's image, writes a new fstab and "
                                  "machine-id, builds the initramfs and installs GRUB for UEFI or BIOS. No users are created: the clone's "
                                  "accounts carry over." ) );
    about->setWordWrap( true );
    cil->addWidget( about );
    m_installerState = new QLabel;
    m_installerState->setWordWrap( true );
    cil->addWidget( m_installerState );
    m_launchInstaller = button( tr( "Launch ClaudeInstall" ), QStringLiteral( "system-software-install" ), "primary" );
    auto* launchRow = new QHBoxLayout;
    launchRow->addWidget( m_launchInstaller );
    launchRow->addStretch( 1 );
    cil->addLayout( launchRow );
    cil->addStretch( 1 );
    connect( m_launchInstaller, &QPushButton::clicked, this, &MainWindow::launchInstaller );
    m_installStack->addWidget( ci );

    // 1: Alpine's own installer
    auto* al = new QWidget;
    auto* all = new QVBoxLayout( al );
    all->setContentsMargins( 0, 0, 0, 0 );
    all->setSpacing( 14 );
    auto* ac = card();
    auto* af = new QFormLayout( ac );
    af->setContentsMargins( 22, 18, 22, 18 );
    af->setVerticalSpacing( 12 );
    m_alpineDisk = new QComboBox;
    auto* diskRow = new QHBoxLayout;
    auto* refresh = button( tr( "Refresh" ), QStringLiteral( "view-refresh" ) );
    diskRow->addWidget( m_alpineDisk, 1 );
    diskRow->addWidget( refresh );
    af->addRow( tr( "Target disk" ), diskRow );
    m_alpineFs = new QComboBox;
    m_alpineFs->addItem( tr( "ext4" ), QStringLiteral( "ext4|0" ) );
    m_alpineFs->addItem( tr( "btrfs with zstd:22 compression" ), QStringLiteral( "btrfs|1" ) );
    m_alpineFs->addItem( tr( "btrfs without compression" ), QStringLiteral( "btrfs|0" ) );
    af->addRow( tr( "Filesystem" ), m_alpineFs );
    auto* install = button( tr( "Install Cloned System" ), QStringLiteral( "system-software-install" ), "danger" );
    af->addRow( QString(), install );
    all->addWidget( ac );
    auto* atc = card();
    auto* atl = new QVBoxLayout( atc );
    atl->setContentsMargins( 14, 14, 14, 14 );
    m_installTask = new TaskPanel( m_backend );
    atl->addWidget( m_installTask );
    all->addWidget( atc, 1 );
    connect( refresh, &QPushButton::clicked, this, [ this ]() { refreshDrives( m_alpineDisk, false ); } );
    connect( install, &QPushButton::clicked, this, &MainWindow::installAlpine );
    m_installStack->addWidget( al );

    l->addWidget( m_installStack, 1 );
    return pageFrame( page );
}

void
MainWindow::fillInstall()
{
    m_installStack->setCurrentIndex( isAlpine() ? 1 : 0 );
    if ( isAlpine() )
    {
        if ( m_alpineDisk->count() == 0 )
        {
            refreshDrives( m_alpineDisk, false );
        }
        return;
    }
    const bool ready = m_status.flag( QStringLiteral( "installerReady" ) );
    const bool available = m_status.value( QStringLiteral( "installer" ) ) == QLatin1String( "claudeinstall" );
    m_installerState->setText( !available ? tr( "<span style='color:#ff5c6c'>This distro is not supported.</span>" )
                               : ready    ? tr( "<span style='color:#5fbf7a'>✓ ClaudeInstall is installed and configured for this clone.</span>" )
                                          : tr( "<span style='color:#e5c07b'>! ClaudeInstall is not set up. Run build.sh in the compendium folder and answer yes to ClaudeInstall, "
                                                "then build the ISO again.</span>" ) );
    m_launchInstaller->setEnabled( available && ready );
}

void
MainWindow::launchInstaller()
{
    const QString launcher = m_status.value( QStringLiteral( "installerLauncher" ) );
    bool started = false;
    if ( QFileInfo( launcher ).isExecutable() )
    {
        started = QProcess::startDetached( launcher, {} );
    }
    else
    {
        started = QProcess::startDetached( QStringLiteral( "pkexec" ), { QStringLiteral( "claudeinstall" ) } );
    }
    if ( !started )
    {
        QMessageBox::warning( this, tr( "Install" ), tr( "ClaudeInstall could not be started (%1)." ).arg( launcher ) );
    }
}

void
MainWindow::installAlpine()
{
    const QString disk = m_alpineDisk->currentData().toString();
    if ( disk.isEmpty() )
    {
        QMessageBox::information( this, tr( "Install" ), tr( "Choose a target disk first." ) );
        return;
    }
    bool ok = false;
    const QString typed = QInputDialog::getText( this,
                                                 tr( "Erase %1?" ).arg( disk ),
                                                 tr( "EVERYTHING on %1 will be erased and the cloned system installed on it.\n\nType YES to continue:" ).arg( disk ),
                                                 QLineEdit::Normal,
                                                 QString(),
                                                 &ok );
    if ( !ok || typed != QLatin1String( "YES" ) )
    {
        return;
    }
    const QStringList fs = m_alpineFs->currentData().toString().split( QLatin1Char( '|' ) );
    m_installTask->start( tr( "Installing the cloned system on %1" ).arg( disk ), { QStringLiteral( "--install-alpine" ), disk, fs.value( 0 ), fs.value( 1 ) }, false );
}

// ============================================================================================
// Guide
// ============================================================================================
QWidget*
MainWindow::makeGuidePage()
{
    auto* page = new QWidget;
    auto* l = new QVBoxLayout( page );
    l->setSpacing( 14 );
    l->addWidget( pageHeader( tr( "Guide" ), tr( "How a clone is made on this distro, step by step." ) ) );
    m_guide = new QPlainTextEdit;
    m_guide->setObjectName( QStringLiteral( "guide" ) );
    m_guide->setReadOnly( true );
    m_guide->setFont( QFontDatabase::systemFont( QFontDatabase::FixedFont ) );
    l->addWidget( m_guide, 1 );
    return pageFrame( page );
}

// ============================================================================================
// Maintenance
// ============================================================================================
QWidget*
MainWindow::makeMaintenancePage()
{
    auto* page = new QWidget;
    auto* l = new QVBoxLayout( page );
    l->setSpacing( 14 );
    l->addWidget( pageHeader( tr( "Maintenance" ), tr( "Updates, the Arch mkinitcpio config and the log." ) ) );

    auto* c = card();
    auto* cl = new QVBoxLayout( c );
    cl->setContentsMargins( 22, 18, 22, 18 );
    m_versionLabel = new QLabel;
    m_versionLabel->setObjectName( QStringLiteral( "heroTitle" ) );
    m_updateLabel = new QLabel( tr( "Not checked for updates yet." ) );
    m_updateLabel->setObjectName( QStringLiteral( "muted" ) );
    cl->addWidget( m_versionLabel );
    cl->addWidget( m_updateLabel );
    auto* row = new QHBoxLayout;
    auto* check = button( tr( "Check For Updates" ), QStringLiteral( "view-refresh" ) );
    auto* update = button( tr( "Update Cloner" ), QStringLiteral( "system-software-update" ), "primary" );
    m_mkinitcpio = button( tr( "mkinitcpio Config" ), QStringLiteral( "run-build" ) );
    auto* log = button( tr( "Open Log" ), QStringLiteral( "text-x-log" ) );
    auto* cfg = button( tr( "Open Settings Folder" ), QStringLiteral( "folder-open" ) );
    row->addWidget( check );
    row->addWidget( update );
    row->addWidget( m_mkinitcpio );
    row->addStretch( 1 );
    row->addWidget( log );
    row->addWidget( cfg );
    cl->addLayout( row );
    l->addWidget( c );

    auto* tc = card();
    auto* tl = new QVBoxLayout( tc );
    tl->setContentsMargins( 14, 14, 14, 14 );
    m_maintTask = new TaskPanel( m_backend );
    tl->addWidget( m_maintTask );
    l->addWidget( tc, 1 );

    connect( check, &QPushButton::clicked, this, &MainWindow::checkForUpdates );
    connect( update,
             &QPushButton::clicked,
             this,
             [ this ]()
             {
                 if ( QMessageBox::question( this, tr( "Update" ), tr( "Download, build and install the latest cloner from GitHub?" ) ) == QMessageBox::Yes )
                 {
                     m_maintTask->start( tr( "Updating the cloner" ), { QStringLiteral( "--update" ) }, false );
                 }
             } );
    connect( m_mkinitcpio,
             &QPushButton::clicked,
             this,
             [ this ]() { m_maintTask->start( tr( "mkinitcpio config" ), { QStringLiteral( "--mkinitcpio-config" ) }, false ); } );
    connect( log,
             &QPushButton::clicked,
             this,
             [ this ]() { QDesktopServices::openUrl( QUrl::fromLocalFile( m_status.value( QStringLiteral( "logFile" ) ) ) ); } );
    connect( cfg,
             &QPushButton::clicked,
             this,
             [ this ]() { QDesktopServices::openUrl( QUrl::fromLocalFile( QFileInfo( m_status.value( QStringLiteral( "configFile" ) ) ).path() ) ); } );
    connect( m_maintTask, &TaskPanel::finished, this, [ this ]( bool, bool ) { refreshStatus(); } );
    return pageFrame( page );
}

void
MainWindow::checkForUpdates()
{
    m_updateLabel->setText( tr( "Checking GitHub..." ) );
    auto* p = new QProcess( this );
    connect( p,
             &QProcess::finished,
             this,
             [ this, p ]( int, QProcess::ExitStatus )
             {
                 QString latest;
                 bool newer = false;
                 for ( const QString& line : QString::fromUtf8( p->readAllStandardOutput() ).split( QLatin1Char( '\n' ) ) )
                 {
                     if ( line.startsWith( QLatin1String( "latest=" ) ) )
                     {
                         latest = line.mid( 7 ).trimmed();
                     }
                     else if ( line == QLatin1String( "newer=1" ) )
                     {
                         newer = true;
                     }
                 }
                 m_updateLabel->setText( latest.isEmpty() ? tr( "The update check failed (offline, or GitHub unreachable)." )
                                         : newer          ? tr( "Version %1 is available - press Update Cloner." ).arg( latest )
                                                          : tr( "Up to date (%1)." ).arg( latest ) );
                 p->deleteLater();
             } );
    p->start( Backend::clonerPath(), { QStringLiteral( "--check-update" ) } );
}

void
MainWindow::closeEvent( QCloseEvent* event )
{
    if ( m_backend->isRunning() )
    {
        if ( QMessageBox::question( this, tr( "Quit" ), tr( "A task is still running. Cancel it and quit?" ) ) != QMessageBox::Yes )
        {
            event->ignore();
            return;
        }
        m_backend->cancel();
    }
    if ( m_settingsDirty
         && QMessageBox::question( this, tr( "Quit" ), tr( "Save the changed settings before quitting?" ) ) == QMessageBox::Yes )
    {
        saveSettings();
    }
    event->accept();
}
