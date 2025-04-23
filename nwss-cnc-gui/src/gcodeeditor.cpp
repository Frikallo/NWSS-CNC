#include "gcodeeditor.h"
#include <QPainter>
#include <QTextBlock>

// GCode Highlighter implementation
GCodeHighlighter::GCodeHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    // G-commands (G0, G1, G2, etc.)
    gCommandFormat.setForeground(QColor(41, 128, 185)); // Blue
    gCommandFormat.setFontWeight(QFont::Bold);
    QRegularExpression gCommandPattern("\\b[Gg]\\d+(?:\\.\\d+)?\\b");
    highlightingRules.append({gCommandPattern, gCommandFormat});

    // M-commands (M3, M5, etc.)
    mCommandFormat.setForeground(QColor(39, 174, 96)); // Green
    mCommandFormat.setFontWeight(QFont::Bold);
    QRegularExpression mCommandPattern("\\b[Mm]\\d+\\b");
    highlightingRules.append({mCommandPattern, mCommandFormat});

    // Coordinates (X, Y, Z, etc.)
    coordinateFormat.setForeground(QColor(192, 57, 43)); // Red
    QRegularExpression coordinatePattern("\\b[XYZxyz][-+]?\\d*\\.?\\d+\\b");
    highlightingRules.append({coordinatePattern, coordinateFormat});

    // Feed rate and other parameters
    numberFormat.setForeground(QColor(211, 84, 0)); // Orange
    QRegularExpression numberPattern("\\b[FfSs][-+]?\\d*\\.?\\d+\\b");
    highlightingRules.append({numberPattern, numberFormat});

    // Comments
    commentFormat.setForeground(QColor(127, 140, 141)); // Gray
    commentFormat.setFontItalic(true);
    QRegularExpression commentPattern(";.*$|\\(.*\\)");
    highlightingRules.append({commentPattern, commentFormat});
}

void GCodeHighlighter::highlightBlock(const QString &text)
{
    for (const HighlightingRule &rule : std::as_const(highlightingRules)) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
}

// GCode Editor implementation
GCodeEditor::GCodeEditor(QWidget *parent)
    : QPlainTextEdit(parent)
{
    lineNumberArea = new LineNumberArea(this);
    highlighter = new GCodeHighlighter(document());
    
    connect(this, &GCodeEditor::blockCountChanged, this, &GCodeEditor::updateLineNumberAreaWidth);
    connect(this, &GCodeEditor::updateRequest, this, &GCodeEditor::updateLineNumberArea);
    connect(this, &GCodeEditor::cursorPositionChanged, this, &GCodeEditor::highlightCurrentLine);
    
    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
    
    // Set a monospaced font
    QFont font;
    font.setFamily("Courier");
    font.setFixedPitch(true);
    font.setPointSize(10);
    setFont(font);
    
    // Set tab stop width
    QFontMetrics metrics(font);
    setTabStopDistance(4 * metrics.horizontalAdvance(' '));
}

int GCodeEditor::lineNumberAreaWidth()
{
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }
    
    int space = 3 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
    return space;
}

void GCodeEditor::updateLineNumberAreaWidth(int /* newBlockCount */)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void GCodeEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        lineNumberArea->scroll(0, dy);
    else
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());
    
    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void GCodeEditor::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);
    
    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void GCodeEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;
    
    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;
        
        QColor lineColor = QColor(Qt::yellow).lighter(180);
        
        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }
    
    setExtraSelections(extraSelections);
}

void GCodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), QColor(232, 232, 232));
    
    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());
    
    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(QColor(120, 120, 120));
            painter.drawText(0, top, lineNumberArea->width() - 2, fontMetrics().height(),
                             Qt::AlignRight, number);
        }
        
        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}