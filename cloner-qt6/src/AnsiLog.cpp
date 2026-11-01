// SPDX-FileCopyrightText: claudemods
// SPDX-License-Identifier: MIT

#include "AnsiLog.h"

#include <QFontDatabase>
#include <QRegularExpression>
#include <QScrollBar>
#include <QTextBlock>
#include <QTextCursor>

namespace
{
// Terminal colours tuned for the app's dark console
QColor
sgrColor( int code )
{
    switch ( code )
    {
    case 30: return QColor( "#7f8590" );
    case 31: return QColor( "#ff5c6c" );
    case 32: return QColor( "#5fbf7a" );
    case 33: return QColor( "#e5c07b" );
    case 34: return QColor( "#61afef" );
    case 35: return QColor( "#c678dd" );
    case 36: return QColor( "#56b6c2" );
    case 37: return QColor( "#dcdfe4" );
    case 90: return QColor( "#9aa0aa" );
    case 91: return QColor( "#ff7b88" );
    case 92: return QColor( "#98e07f" );
    case 93: return QColor( "#f2d58a" );
    case 94: return QColor( "#7cc4ff" );
    case 95: return QColor( "#d7a2ee" );
    case 96: return QColor( "#7fdbe6" );
    case 97: return QColor( "#ffffff" );
    default: return QColor();
    }
}
}  // namespace

AnsiLog::AnsiLog( QWidget* parent )
    : QPlainTextEdit( parent )
{
    setObjectName( QStringLiteral( "console" ) );
    setReadOnly( true );
    setUndoRedoEnabled( false );
    setLineWrapMode( QPlainTextEdit::WidgetWidth );
    setMaximumBlockCount( 20000 );
    QFont mono = QFontDatabase::systemFont( QFontDatabase::FixedFont );
    mono.setPointSizeF( qMax( 9.0, mono.pointSizeF() ) );
    setFont( mono );
    m_default.setForeground( QColor( "#c8ccd4" ) );
    m_format = m_default;
}

void
AnsiLog::clearLog()
{
    clear();
    m_format = m_default;
    m_pending.clear();
    m_overwrite = false;
    m_lastPercent = -1;
    m_lastLine.clear();
}

void
AnsiLog::applySgr( const QString& params )
{
    const QStringList codes = params.isEmpty() ? QStringList { QStringLiteral( "0" ) } : params.split( QLatin1Char( ';' ) );
    for ( const QString& p : codes )
    {
        const int code = p.toInt();
        if ( code == 0 )
        {
            m_format = m_default;
        }
        else if ( code == 1 )
        {
            m_format.setFontWeight( QFont::Bold );
        }
        else if ( code == 22 )
        {
            m_format.setFontWeight( QFont::Normal );
        }
        else if ( code == 39 )
        {
            m_format.setForeground( m_default.foreground() );
        }
        else if ( sgrColor( code ).isValid() )
        {
            m_format.setForeground( sgrColor( code ) );
        }
    }
}

void
AnsiLog::insertText( const QString& text )
{
    if ( text.isEmpty() )
    {
        return;
    }
    QTextCursor cur( document() );
    cur.movePosition( QTextCursor::End );
    if ( m_overwrite )
    {
        m_overwrite = false;
        // "\r" then "\n" in the next chunk is a plain line end, not a redraw
        if ( !text.startsWith( QLatin1Char( '\n' ) ) )
        {
            cur.movePosition( QTextCursor::StartOfBlock, QTextCursor::KeepAnchor );
            cur.removeSelectedText();
        }
    }
    cur.insertText( text, m_format );

    QTextBlock last = document()->lastBlock();
    if ( last.text().isEmpty() && last.previous().isValid() )
    {
        last = last.previous();
    }
    m_lastLine = last.text();
    static const QRegularExpression percent( QStringLiteral( "(\\d{1,3})%" ) );
    QRegularExpressionMatchIterator it = percent.globalMatch( m_lastLine );
    int value = -1;
    while ( it.hasNext() )
    {
        value = it.next().captured( 1 ).toInt();
    }
    if ( value >= 0 && value <= 100 )
    {
        m_lastPercent = value;
    }
}

void
AnsiLog::appendAnsi( const QString& chunk )
{
    QScrollBar* bar = verticalScrollBar();
    const bool follow = bar->value() >= bar->maximum() - 4;

    const QString text = m_pending + chunk;
    m_pending.clear();
    QString plain;
    auto flush = [ & ]()
    {
        insertText( plain );
        plain.clear();
    };

    for ( int i = 0; i < text.size(); ++i )
    {
        const QChar c = text.at( i );
        if ( c == QChar( 0x1b ) )
        {
            if ( i + 1 >= text.size() )
            {
                m_pending = text.mid( i );
                break;
            }
            if ( text.at( i + 1 ) != QLatin1Char( '[' ) )
            {
                ++i;   // two-character escape: skip it
                continue;
            }
            int j = i + 2;
            while ( j < text.size() && !( text.at( j ) >= QLatin1Char( '@' ) && text.at( j ) <= QLatin1Char( '~' ) ) )
            {
                ++j;
            }
            if ( j >= text.size() )
            {
                m_pending = text.mid( i );
                break;
            }
            flush();
            if ( text.at( j ) == QLatin1Char( 'm' ) )
            {
                applySgr( text.mid( i + 2, j - i - 2 ) );
            }
            i = j;   // cursor moves, clears, hide/show cursor: nothing to draw
            continue;
        }
        if ( c == QLatin1Char( '\r' ) )
        {
            if ( i + 1 < text.size() && text.at( i + 1 ) == QLatin1Char( '\n' ) )
            {
                continue;
            }
            flush();
            m_overwrite = true;
            continue;
        }
        if ( c == QLatin1Char( '\b' ) || c == QLatin1Char( '\a' ) )
        {
            continue;
        }
        plain += c;
    }
    flush();

    if ( follow )
    {
        bar->setValue( bar->maximum() );
    }
}
