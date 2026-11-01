// SPDX-FileCopyrightText: claudemods
// SPDX-License-Identifier: MIT

#include "GuideView.h"

#include "BackupView.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QListWidget>
#include <QTextBrowser>
#include <QVBoxLayout>

namespace
{
struct Section
{
    const char* title;
    const char* icon;
    const char* html;
};

// The same guide as the README's "Guide" section, kept short enough to read in the window
const Section kSections[] = {
    { "Start here",
      "go-home",
      R"(<h2>Claude Codex in one minute</h2>
<p>Claude Codex is four tools that work together:</p>
<table cellpadding="6">
<tr><td><b>Claude Codex Backup</b></td><td>clones the running system into a bootable live ISO - users, passwords, settings and /home as they are.</td></tr>
<tr><td><b>Claude Codex Install</b></td><td>the installer on that ISO: puts the clone on a disk (GUI or terminal).</td></tr>
<tr><td><b>Claude Codex CCU</b></td><td>the Container Creator Utility: clones the system into an Incus container, installs that setup to a drive, exports images, builds ISOs of containers.</td></tr>
<tr><td><b>Claude Codex Cloner</b></td><td>this window: Backup and CCU in tabs, plus Updates and this Guide.</td></tr>
</table>
<h3>The usual way</h3>
<ol>
<li><b>Backup &gt; Overview</b> - fix anything marked in red (missing tools, boot files).</li>
<li><b>Backup &gt; Build ISO</b> - build the live ISO now, or on the next boot from init.</li>
<li><b>Backup &gt; Write USB</b> - put the ISO on a stick.</li>
<li>Boot the stick, then <b>Install &lt;distro&gt;</b> on the desktop opens Claude Codex Install.</li>
</ol>
<p>Every task asks for the administrator password when it needs it. Nothing runs before you press its button.</p>)" },
    { "Build a live ISO",
      "media-optical",
      R"(<h2>Backup: build a live ISO</h2>
<ol>
<li><b>Overview</b> shows what is ready. A red line has a <i>Fix</i> button (installs a missing tool, downloads the boot files).</li>
<li><b>Settings</b>: the ISO name, the work and output folders, squashfs or erofs, the zstd level, a copy-to-RAM boot entry, package caches.</li>
<li><b>Edit Configs</b> (optional): the GRUB menu, the boot text and the initramfs config of the ISO, and the installer's settings. The files open directly; Save writes them, Reset goes back to the generated version.</li>
<li><b>Build ISO</b>: <i>Build ISO</i> runs now and shows every step with its progress. <i>Build On Next Boot</i> sets a one-time service: after the reboot the ISO is built on the console (VT1) before the login screen - with nothing else running, so nothing changes while it is copied. <i>Cancel Boot Build</i> takes it back.</li>
<li>The ISO lands in the output folder (default <code>~/Downloads</code>). <b>Write USB</b> or <i>Open Folder</i> from there.</li>
</ol>
<p>Terminal: <code>claude-codex backup</code> opens the menu; <code>claudebackup-linux --help-batch</code> lists the batch commands.</p>)" },
    { "Write a USB stick",
      "drive-removable-media",
      R"(<h2>Backup: write a USB stick</h2>
<ol>
<li><b>Backup &gt; Write USB</b>. The newest ISOs of the output folder are listed; <i>Browse...</i> picks any other one (also ISOs made on the Containers tab).</li>
<li>Pick the drive. Removable drives come first; the drives of the running system are marked and refused.</li>
<li><i>Write ISO To Drive</i> asks you to type the device name, then erases the whole drive and writes the ISO with progress.</li>
</ol>
<p>Test a new ISO in a virtual machine (UEFI) before you rely on it.</p>)" },
    { "Install the clone",
      "system-software-install",
      R"(<h2>Install: put the clone on a disk</h2>
<p>Boot the live ISO. Its desktop has <b>Install &lt;distro&gt;</b> (also <code>claudebackup-install</code> in a terminal), which opens <b>Claude Codex Install</b> as root:</p>
<ol>
<li>Language and keyboard.</li>
<li>Partitions: erase a disk, or choose your own (ext4, btrfs with zstd:22, xfs; optional LUKS). The default file system is the one chosen in build.sh.</li>
<li>Summary, then install: the image is unpacked, fstab, initramfs and GRUB are written for the new disk.</li>
<li>Reboot. The users and passwords are the ones of the cloned system - there is no users page.</li>
</ol>
<p>Built as <b>terminal only</b> (build.sh question 2), the shortcut opens <code>claudeinstall-tui</code> in a terminal. Alpine has the cloner's own <b>Install Cloned System To Disk</b> instead (Backup &gt; Install).</p>
<p>Other installer modes (build.sh question 1): <b>new</b> is a normal distro installer with users and packages; <b>distro</b> installs the claudemods distributions online (Arch).</p>)" },
    { "Containers (CCU)",
      "network-server",
      R"(<h2>Containers: Claude Codex CCU</h2>
<p>CCU runs your system inside an Incus container. The <b>Display</b> setting decides how it reaches the screen:</p>
<ul>
<li><b>Desktop session</b> (GPU): the host shows a <i>Container Plasma</i> login; it runs the container's desktop through the cage compositor with the host's GPU.</li>
<li><b>Native</b> (GPU): the container's own login screen on the console.</li>
<li><b>Server</b> (no GPU): Plasma Wayland in software, plus remote access - <b>KRDP</b> (RDP, port 3389) and <b>SSH</b> (the host on 22, the container on 2222). The steps are printed when it finishes.</li>
</ul>
<h3>The tabs</h3>
<ul>
<li><b>Install to a drive</b> - wipes a drive (or uses two existing partitions): EFI partition, a small tty host with Incus, and your desktop container, created on the first boot with live progress.</li>
<li><b>Set up on this system</b> - installs Incus, SDDM, cage and the rest with your package manager, then creates the container. Reboot afterwards.</li>
<li><b>Export</b> - writes this system as an image (.tar.zst ...) to clone elsewhere.</li>
<li><b>Create ISO</b> - a live ISO of a container's system, built inside the container on the next boot, with Claude Codex Install on it. Then write it on Backup &gt; Write USB.</li>
<li><b>Manage containers</b> - start, stop, switch the one the host session shows, duplicate.</li>
</ul>
<p>Every field shows <b>SET</b> or <b>NOT SET</b>; Start is enabled when nothing required is missing. <i>Dry run</i> prints what would happen. Logs: <code>/var/log/cccu/</code>. Terminal: <code>claude-codex ccu</code>.</p>)" },
    { "Updates",
      "system-software-update",
      R"(<h2>Updates</h2>
<p>The <b>Updates</b> tab checks GitHub when the window opens and shows the result in the header. <i>Update Everything</i> downloads the latest release and runs build.sh with the answers of your last build, so every tool is rebuilt and reinstalled together - a tool that does not compile keeps its old version.</p>
<p>Terminal:</p>
<pre>claude-codex update --check     only look
claude-codex update             update everything
claude-codex update --force     rebuild the same version (repair)</pre>
<p>Your answers are kept in <code>/etc/claude-codex/build.conf</code>. To change them, run <code>./build.sh</code> again.</p>)" },
    { "Commands",
      "utilities-terminal",
      R"(<h2>Commands</h2>
<pre>claude-codex                    menu of everything
claude-codex cloner             this window
claude-codex backup [args]      the cloner (menu, or batch commands)
claude-codex ccu [args]         Claude Codex CCU (as root)
claude-codex install [--tui]    Claude Codex Install (as root)
claude-codex status             what is installed
claude-codex update [--check]   update everything
claude-codex help</pre>
<p>The single tools are there too: <code>claudebackup-linux</code>, <code>claudeinstall</code>, <code>claudeinstall-tui</code>, <code>cccu</code>, and the <code>claude-codex-*</code> links.</p>)" },
    { "Notes for this distro", "dialog-information", nullptr },
};
constexpr int kSectionCount = int( sizeof( kSections ) / sizeof( kSections[ 0 ] ) );
}  // namespace

GuideView::GuideView( BackupView* backup, QWidget* parent )
    : QWidget( parent )
    , m_backup( backup )
{
    auto* row = new QHBoxLayout( this );
    row->setContentsMargins( 22, 20, 28, 20 );
    row->setSpacing( 16 );

    auto* side = new QVBoxLayout;
    auto* title = new QLabel( tr( "Guide" ) );
    title->setObjectName( QStringLiteral( "pageTitle" ) );
    side->addWidget( title );
    m_nav = new QListWidget;
    m_nav->setObjectName( QStringLiteral( "guideNav" ) );
    m_nav->setFixedWidth( 230 );
    m_nav->setIconSize( QSize( 18, 18 ) );
    m_nav->setFocusPolicy( Qt::NoFocus );
    for ( const Section& s : kSections )
    {
        m_nav->addItem( new QListWidgetItem( QIcon::fromTheme( QString::fromLatin1( s.icon ) ), tr( s.title ) ) );
    }
    side->addWidget( m_nav, 1 );
    row->addLayout( side );

    m_text = new QTextBrowser;
    m_text->setObjectName( QStringLiteral( "guideText" ) );
    m_text->setOpenExternalLinks( true );
    row->addWidget( m_text, 1 );

    connect( m_nav, &QListWidget::currentRowChanged, this, &GuideView::showSection );
    m_nav->setCurrentRow( 0 );
}

void
GuideView::loadDistroNotes()
{
    if ( m_notesLoaded )
    {
        return;
    }
    m_notesLoaded = true;
    QApplication::setOverrideCursor( Qt::WaitCursor );
    m_distroNotes = m_backup->guideText().trimmed();
    QApplication::restoreOverrideCursor();
    if ( m_nav->currentRow() == kSectionCount - 1 )
    {
        showSection( kSectionCount - 1 );
    }
}

void
GuideView::showSection( int section )
{
    if ( section < 0 || section >= kSectionCount )
    {
        return;
    }
    const QString css = QStringLiteral( "<style>h2{color:#ffffff;} h3{color:#ff7b88;} code,pre{color:#e5c07b;} "
                                        "td{vertical-align:top;} li{margin-bottom:4px;}</style>" );
    if ( kSections[ section ].html )
    {
        m_text->setHtml( css + QString::fromUtf8( kSections[ section ].html ) );
        return;
    }
    loadDistroNotes();
    const QString notes = m_distroNotes.isEmpty() ? tr( "The cloner (claudebackup-linux) did not return its guide - is it installed?" )
                                                  : m_distroNotes;
    m_text->setHtml( css + QStringLiteral( "<h2>%1</h2><pre>%2</pre>" ).arg( tr( "Notes for this distro" ), notes.toHtmlEscaped() ) );
}
