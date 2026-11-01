// SPDX-FileCopyrightText: claudemods
// SPDX-License-Identifier: MIT
//
// claudebackup-linux-qt6: the graphical front end of the claudebackup linux cloner.
// Run it as your normal user; it asks for the administrator password (polkit) when a task needs it.

#include "MainWindow.h"

#include <QApplication>
#include <QIcon>
#include <QPalette>
#include <QStyleFactory>

#include <unistd.h>

namespace
{
// claudemods look: charcoal surfaces, red accent (the cloner's banner colour)
const char* const kStyle = R"QSS(
QWidget { color: #e6e8ec; font-size: 10pt; }
QMainWindow, #pages, QScrollArea, QScrollArea > QWidget > QWidget { background: #15171c; }
#sidebar { background: #0f1115; border-right: 1px solid #23262e; }
#brand { font-size: 17pt; font-weight: 700; color: #ffffff; }
#brandSub { color: #d7263d; font-weight: 600; letter-spacing: 1px; }
#nav { background: transparent; border: none; outline: none; }
#nav::item { padding: 10px 10px; margin: 2px 0; border-radius: 8px; color: #b9bec8; }
#nav::item:hover { background: #1b1e25; color: #ffffff; }
#nav::item:selected { background: #d7263d; color: #ffffff; }
#badge { background: #171a20; border: 1px solid #262a33; border-radius: 8px; padding: 10px; color: #cfd3da; }
#pageTitle { font-size: 20pt; font-weight: 700; color: #ffffff; }
#section { font-size: 11pt; font-weight: 700; color: #ffffff; padding-bottom: 6px; }
#muted { color: #8a909c; }
#summary { background: #1b1e25; border: 1px solid #262a33; border-radius: 10px; padding: 12px 16px; color: #cfd3da; }
#card { background: #1b1e25; border: 1px solid #262a33; border-radius: 12px; }
#hero { background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #2a1218, stop:1 #1b1e25); border: 1px solid #3a1d24; border-radius: 12px; }
#heroTitle { font-size: 15pt; font-weight: 700; color: #ffffff; }
#taskTitle { font-size: 11pt; font-weight: 700; color: #ffffff; }
#checkRow { border-bottom: 1px solid #23262e; }
QPushButton { background: #252932; border: 1px solid #323743; border-radius: 8px; padding: 8px 16px; color: #e6e8ec; }
QPushButton:hover { background: #2d323d; border-color: #3d4351; }
QPushButton:pressed { background: #20232b; }
QPushButton:disabled { color: #5d626d; background: #1d2027; border-color: #262a33; }
QPushButton#primary { background: #d7263d; border-color: #d7263d; color: #ffffff; font-weight: 600; }
QPushButton#primary:hover { background: #e63950; }
QPushButton#primary:disabled { background: #5a2a32; border-color: #5a2a32; color: #b08a90; }
QPushButton#danger { background: transparent; border: 1px solid #b4232f; color: #ff7b88; font-weight: 600; }
QPushButton#danger:hover { background: #2a1519; }
QLineEdit, QComboBox, QSpinBox { background: #12141a; border: 1px solid #2c303a; border-radius: 7px; padding: 6px 9px; selection-background-color: #d7263d; }
QLineEdit:focus, QComboBox:focus, QSpinBox:focus { border-color: #d7263d; }
QComboBox QAbstractItemView { background: #1b1e25; border: 1px solid #2c303a; selection-background-color: #d7263d; }
QCheckBox::indicator { width: 16px; height: 16px; }
QPlainTextEdit#console, QPlainTextEdit#guide { background: #0c0d11; border: 1px solid #23262e; border-radius: 10px; padding: 6px; color: #c8ccd4; }
QPlainTextEdit#editor { background: #101217; border: 1px solid #2c303a; border-radius: 8px; padding: 6px; }
QListWidget#steps, QListWidget#configList { background: transparent; border: none; outline: none; }
QListWidget#steps::item { padding: 5px 4px; }
QListWidget#configList::item { padding: 7px 6px; border-radius: 6px; }
QListWidget#configList::item:selected { background: #2a1519; color: #ffffff; }
QProgressBar { background: #12141a; border: 1px solid #2c303a; border-radius: 7px; height: 18px; text-align: center; color: #ffffff; }
QProgressBar::chunk { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #b4232f, stop:1 #e63950); border-radius: 6px; }
QProgressBar[state="ok"]::chunk { background: #3f9b5c; }
QProgressBar[state="fail"]::chunk { background: #7a2630; }
QSplitter::handle { background: transparent; width: 10px; }
QScrollBar:vertical { background: transparent; width: 10px; margin: 2px; }
QScrollBar::handle:vertical { background: #323743; border-radius: 4px; min-height: 30px; }
QScrollBar::add-line, QScrollBar::sub-line { height: 0; width: 0; }
QToolTip { background: #1b1e25; color: #e6e8ec; border: 1px solid #323743; }
QMessageBox, QInputDialog, QFileDialog { background: #1b1e25; }
)QSS";
}  // namespace

int
main( int argc, char* argv[] )
{
    QApplication app( argc, argv );
    QApplication::setApplicationName( QStringLiteral( "claudebackup-linux-qt6" ) );
    QApplication::setApplicationDisplayName( QStringLiteral( "claudebackup linux" ) );
    QApplication::setOrganizationName( QStringLiteral( "claudemods" ) );
    QApplication::setDesktopFileName( QStringLiteral( "claudebackup-linux-qt6" ) );
    QApplication::setWindowIcon( QIcon::fromTheme( QStringLiteral( "drive-harddisk" ) ) );

    // Fusion + our own palette, so the app looks the same on KDE, GNOME and window managers
    app.setStyle( QStyleFactory::create( QStringLiteral( "Fusion" ) ) );
    QPalette p;
    p.setColor( QPalette::Window, QColor( "#15171c" ) );
    p.setColor( QPalette::WindowText, QColor( "#e6e8ec" ) );
    p.setColor( QPalette::Base, QColor( "#12141a" ) );
    p.setColor( QPalette::AlternateBase, QColor( "#1b1e25" ) );
    p.setColor( QPalette::Text, QColor( "#e6e8ec" ) );
    p.setColor( QPalette::Button, QColor( "#252932" ) );
    p.setColor( QPalette::ButtonText, QColor( "#e6e8ec" ) );
    p.setColor( QPalette::Highlight, QColor( "#d7263d" ) );
    p.setColor( QPalette::HighlightedText, QColor( "#ffffff" ) );
    p.setColor( QPalette::ToolTipBase, QColor( "#1b1e25" ) );
    p.setColor( QPalette::ToolTipText, QColor( "#e6e8ec" ) );
    p.setColor( QPalette::PlaceholderText, QColor( "#5d626d" ) );
    p.setColor( QPalette::Disabled, QPalette::Text, QColor( "#5d626d" ) );
    p.setColor( QPalette::Disabled, QPalette::WindowText, QColor( "#5d626d" ) );
    p.setColor( QPalette::Disabled, QPalette::ButtonText, QColor( "#5d626d" ) );
    app.setPalette( p );
    app.setStyleSheet( QString::fromUtf8( kStyle ) );

    if ( geteuid() == 0 )
    {
        qWarning( "claudebackup-linux-qt6: running as root is not needed - start it as your user, it asks for the password itself." );
    }

    MainWindow w;
    w.show();
    return app.exec();
}
