// SPDX-FileCopyrightText: claudemods
// SPDX-License-Identifier: MIT

#include "LogWindow.h"

#include <QHBoxLayout>
#include <QIcon>
#include <QKeySequence>
#include <QLabel>
#include <QPlainTextDocumentLayout>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QShortcut>
#include <QTextDocument>
#include <QTimer>
#include <QVBoxLayout>

void
LogWindow::open( QPlainTextEdit* source, const QString& title )
{
    if ( !source )
    {
        return;
    }
    auto* w = new LogWindow( source, title );
    w->setAttribute( Qt::WA_DeleteOnClose );
    w->showFullScreen();
}

LogWindow::LogWindow( QPlainTextEdit* source, const QString& title )
    : QWidget( nullptr, Qt::Window )
    , m_source( source )
{
    setWindowTitle( tr( "%1 - log" ).arg( title ) );
    setWindowIcon( QIcon::fromTheme( QStringLiteral( "claude-codex-cloner" ) ) );
    setObjectName( QStringLiteral( "logWindow" ) );

    auto* col = new QVBoxLayout( this );
    col->setContentsMargins( 16, 12, 16, 16 );
    col->setSpacing( 10 );
    auto* top = new QHBoxLayout;
    auto* heading = new QLabel( title );
    heading->setObjectName( QStringLiteral( "taskTitle" ) );
    auto* hint = new QLabel( tr( "Live - Esc closes this window, the task keeps running" ) );
    hint->setObjectName( QStringLiteral( "muted" ) );
    auto* close = new QPushButton( QIcon::fromTheme( QStringLiteral( "window-close" ) ), tr( "Close" ) );
    top->addWidget( heading );
    top->addSpacing( 16 );
    top->addWidget( hint, 1 );
    top->addWidget( close );
    col->addLayout( top );

    m_view = new QPlainTextEdit;
    m_view->setObjectName( QStringLiteral( "console" ) );
    m_view->setReadOnly( true );
    m_view->setLineWrapMode( source->lineWrapMode() );
    m_view->setFont( source->font() );
    col->addWidget( m_view, 1 );

    connect( close, &QPushButton::clicked, this, &QWidget::close );
    auto* esc = new QShortcut( QKeySequence( Qt::Key_Escape ), this );
    connect( esc, &QShortcut::activated, this, &QWidget::close );

    // the source's document is copied (with its colours) whenever it changed, a few times a second
    connect( source->document(), &QTextDocument::contentsChanged, this, [ this ]() { m_dirty = true; } );
    connect( source, &QObject::destroyed, this, &QWidget::close );
    m_timer = new QTimer( this );
    m_timer->setInterval( 300 );
    connect( m_timer, &QTimer::timeout, this, &LogWindow::sync );
    m_timer->start();
    sync();
}

void
LogWindow::sync()
{
    if ( !m_dirty || !m_source )
    {
        return;
    }
    m_dirty = false;
    QScrollBar* bar = m_view->verticalScrollBar();
    const bool follow = bar->value() >= bar->maximum() - 4;
    const int keep = bar->value();

    QTextDocument* copy = m_source->document()->clone( m_view );
    copy->setDocumentLayout( new QPlainTextDocumentLayout( copy ) );
    copy->setDefaultFont( m_source->font() );
    QTextDocument* old = m_view->document();
    m_view->setDocument( copy );
    if ( old && old->parent() == m_view )
    {
        old->deleteLater();
    }
    bar->setValue( follow ? bar->maximum() : keep );
}
