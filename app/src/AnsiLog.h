// SPDX-FileCopyrightText: claudemods
// SPDX-License-Identifier: MIT
//
// AnsiLog: a read-only console that shows the cloner's coloured output the way a terminal does
// (SGR colours, \r redraws of progress bars; cursor and clear codes are ignored).

#pragma once

#include <QPlainTextEdit>
#include <QTextCharFormat>

class AnsiLog : public QPlainTextEdit
{
    Q_OBJECT
public:
    explicit AnsiLog( QWidget* parent = nullptr );

    void appendAnsi( const QString& text );
    void clearLog();

    // The percentage on the last line (mksquashfs " 45%", dd's byte count is handled by the caller)
    int lastPercent() const { return m_lastPercent; }
    QString lastLine() const { return m_lastLine; }

private:
    void applySgr( const QString& params );
    void insertText( const QString& text );

    QTextCharFormat m_format;
    QTextCharFormat m_default;
    QString m_pending;          // an escape sequence split between two chunks
    bool m_overwrite = false;   // a \r was seen: the next text replaces the current line
    int m_lastPercent = -1;
    QString m_lastLine;
};
