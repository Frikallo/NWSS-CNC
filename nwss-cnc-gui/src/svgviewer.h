// svgviewer.h
#ifndef SVGVIEWER_H
#define SVGVIEWER_H

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsSvgItem>
#include <QSvgRenderer>
#include <QWheelEvent>
#include <QToolBar>
#include <QSlider>
#include <QLabel>
#include <QFileInfo>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QComboBox>

class SVGView : public QGraphicsView
{
    Q_OBJECT
    
public:
    explicit SVGView(QWidget *parent = nullptr);
    
    void setSvgFile(const QString &filePath);
    QSvgRenderer* renderer() const { return m_renderer; }
    void zoomIn();
    void zoomOut();
    void resetZoom();
    void setZoom(int zoomLevel);
    
    bool isSvgLoaded() const { return m_svgItem != nullptr; }
    QString svgFilePath() const { return m_filePath; }
    
protected:
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    
private:
    QGraphicsScene *m_scene;
    QGraphicsSvgItem *m_svgItem;
    QSvgRenderer *m_renderer;
    QString m_filePath;
    QPoint m_lastMousePos;
    double m_zoomFactor;
    bool m_isPanning;
};

class SVGViewer : public QWidget
{
    Q_OBJECT

public:
    explicit SVGViewer(QWidget *parent = nullptr);
    
    void loadSvgFile(const QString &filePath);
    bool isSvgLoaded() const;
    QString currentSvgFile() const;
    
signals:
    void svgLoaded(const QString &filePath);
    void convertToGCode(const QString &svgFile);
    
public slots:
    void onZoomChanged(int value);
    
private slots:
    void onOpenSvgClicked();
    void onConvertClicked();
    void onFitToViewClicked();
    
private:
    SVGView *m_svgView;
    QSlider *m_zoomSlider;
    QLabel *m_fileNameLabel;
    QLabel *m_zoomLabel;
    QPushButton *m_openButton;
    QPushButton *m_convertButton;
    QPushButton *m_fitButton;
    QComboBox *m_conversionModeComboBox;
    
    void setupToolbar();
    void setupStatusBar();
    void updateZoomLabel(int value);
};

#endif // SVGVIEWER_H