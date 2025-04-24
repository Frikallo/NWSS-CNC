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
#include <QTimer>
#include <QPainter>

class SVGView : public QGraphicsView
{
    Q_OBJECT
    
public:
    explicit SVGView(QWidget *parent = nullptr);
    ~SVGView();
    
    void setSvgFile(const QString &filePath);
    QSvgRenderer* renderer() const { return m_renderer; }
    void zoomIn();
    void zoomOut();
    void resetZoom();
    void setZoom(int zoomLevel);
    
    bool isSvgLoaded() const { return m_renderer != nullptr; }
    QString svgFilePath() const { return m_filePath; }
    double currentZoomFactor() const { return m_zoomFactor; }
    
signals:
    void zoomChanged(double zoomFactor);
    
protected:
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    
private:
    // New methods for rasterization
    void updateRasterizedImage();
    void drawSvgDirectly(QPainter *painter);
    void drawRasterizedImage(QPainter *painter);
    
    QGraphicsScene *m_scene;
    QSvgRenderer *m_renderer;
    QString m_filePath;
    QPoint m_lastMousePos;
    double m_zoomFactor;
    bool m_isPanning;
    
    // For rasterization approach
    bool m_useRasterization;
    QPixmap m_rasterizedImage;
    double m_rasterizedZoomFactor;
    QRectF m_svgBounds;
    QSize m_baseSize;
    
    // Constants
    static inline const double RASTERIZATION_THRESHOLD = 2.0; 
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
    void onSvgViewZoomChanged(double zoomFactor);
    
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
    void setupOptimizedRendering();
};

#endif // SVGVIEWER_H