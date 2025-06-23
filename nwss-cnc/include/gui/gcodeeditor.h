#ifndef GCODEEDITOR_H
#define GCODEEDITOR_H

#include <QObject>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>

// Syntax highlighter for GCode
class GCodeHighlighter : public QSyntaxHighlighter {
  Q_OBJECT

 public:
  GCodeHighlighter(QTextDocument *parent = nullptr);

 protected:
  void highlightBlock(const QString &text) override;

 private:
  struct HighlightingRule {
    QRegularExpression pattern;
    QTextCharFormat format;
  };
  QVector<HighlightingRule> highlightingRules;

  QTextCharFormat gCommandFormat;
  QTextCharFormat mCommandFormat;
  QTextCharFormat coordinateFormat;
  QTextCharFormat commentFormat;
  QTextCharFormat numberFormat;
};

// GCode Editor
class GCodeEditor : public QPlainTextEdit {
  Q_OBJECT

 public:
  GCodeEditor(QWidget *parent = nullptr);

  void lineNumberAreaPaintEvent(QPaintEvent *event);
  int lineNumberAreaWidth();
  void setupCustomCursor();

 protected:
  void resizeEvent(QResizeEvent *event) override;

 private slots:
  void updateLineNumberAreaWidth(int newBlockCount);
  void highlightCurrentLine();
  void updateLineNumberArea(const QRect &rect, int dy);

 private:
  QWidget *lineNumberArea;
  GCodeHighlighter *highlighter;
};

// Line number area for the GCode editor
class LineNumberArea : public QWidget {
 public:
  LineNumberArea(GCodeEditor *editor) : QWidget(editor), codeEditor(editor) {}

  QSize sizeHint() const override {
    return QSize(codeEditor->lineNumberAreaWidth(), 0);
  }

 protected:
  void paintEvent(QPaintEvent *event) override {
    codeEditor->lineNumberAreaPaintEvent(event);
  }

 private:
  GCodeEditor *codeEditor;
};

#endif  // GCODEEDITOR_H