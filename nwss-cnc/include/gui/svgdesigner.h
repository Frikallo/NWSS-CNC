#ifndef SVGDESIGNER_H
#define SVGDESIGNER_H

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
#include <QDockWidget>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QGraphicsItem>
#include <QUndoStack>
#include <QUndoCommand>
#include <QAction>

class DesignerView : public QGraphicsView
{
    Q_OBJECT
    
public:
    enum Tool {
        Select,
        Measure,
        Pan
    };
    
    enum Snap {
        NoSnap,
        Grid,
        Object
    };
    
    explicit DesignerView(QWidget *parent = nullptr);
    ~DesignerView();
    
    void setSvgFile(const QString &filePath);
    QSvgRenderer* renderer() const { return m_renderer; }
    void zoomIn();
    void zoomOut();
    void resetZoom();
    void setZoom(int zoomLevel);
    
    bool isSvgLoaded() const { return m_renderer != nullptr; }
    QString svgFilePath() const { return m_filePath; }
    double currentZoomFactor() const { return m_zoomFactor; }
    
    // Material and bed settings
    void setMaterialSize(double width, double height) { 
        m_materialWidth = width; 
        m_materialHeight = height; 
        updateBoundaries(); 
    }
    
    void setBedSize(double width, double height) { 
        m_bedWidth = width; 
        m_bedHeight = height; 
        updateBoundaries(); 
    }
    
    // Grid settings
    void setGridSpacing(double spacing) { 
        m_gridSpacing = spacing; 
        viewport()->update(); 
    }
    
    void setGridVisible(bool visible) { 
        m_gridVisible = visible; 
        viewport()->update(); 
    }
    
    // Selection and transformation
    void selectAll();
    void deselectAll();
    void deleteSelected();
    void centerSelected();
    void moveSelectedBy(double dx, double dy);
    void scaleSelected(double scaleX, double scaleY);
    void rotateSelected(double angle);
    
    // Get information about selected items
    QRectF getSelectionBounds() const;
    
    // Tool selection
    void setCurrentTool(Tool tool) { 
        m_currentTool = tool; 
        updateCursor(); 
    }
    
    // Snap settings
    void setSnapMode(Snap mode) { m_snapMode = mode; }
    void setSnapGridSize(double size) { m_snapGridSize = size; }
    
    // Measurement
    QPointF getMeasureStart() const { return m_measureStart; }
    QPointF getMeasureEnd() const { return m_measureEnd; }
    double getMeasureDistance() const;
    
    // Export the current design
    void exportAsSvg(const QString &filePath);
    
signals:
    void zoomChanged(double zoomFactor);
    void selectionChanged();
    void measureUpdated(double distance, double angle);
    
protected:
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void drawBackground(QPainter *painter, const QRectF &rect) override;
    void drawForeground(QPainter *painter, const QRectF &rect) override;
    
private:
    // Helper methods
    void updateRasterizedImage();
    void drawSvgDirectly(QPainter *painter);
    void drawRasterizedImage(QPainter *painter);
    void drawRulers(QPainter *painter, const QRectF &rect);
    void drawGrid(QPainter *painter, const QRectF &rect);
    void drawMaterialBoundary(QPainter *painter);
    void drawBedBoundary(QPainter *painter);
    void drawMeasureLine(QPainter *painter);
    void updateBoundaries();
    void updateCursor();
    QPointF snapToGrid(const QPointF &pos);
    
    // Main components
    QGraphicsScene *m_scene;
    QSvgRenderer *m_renderer;
    QString m_filePath;
    QPoint m_lastMousePos;
    double m_zoomFactor;
    bool m_isPanning;
    
    // Rasterization for improved rendering performance
    bool m_useRasterization;
    QPixmap m_rasterizedImage;
    double m_rasterizedZoomFactor;
    QRectF m_svgBounds;
    QSize m_baseSize;
    
    // Material and bed dimensions
    double m_materialWidth;
    double m_materialHeight;
    double m_bedWidth;
    double m_bedHeight;
    
    // Grid properties
    bool m_gridVisible;
    double m_gridSpacing;
    
    // Rulers
    bool m_rulersVisible;
    int m_rulerWidth;
    
    // Tools and interaction
    Tool m_currentTool;
    Snap m_snapMode;
    double m_snapGridSize;
    
    // Measurement
    QPointF m_measureStart;
    QPointF m_measureEnd;
    bool m_isMeasuring;
    
    // Selection
    QList<QGraphicsItem*> m_selectedItems;
    QPointF m_selectionStartPos;
    bool m_isMovingSelection;
    
    // Constants
    static const double RASTERIZATION_THRESHOLD;
};

class TransformCommand : public QUndoCommand
{
public:
    TransformCommand(DesignerView *view, const QList<QGraphicsItem*> &items, 
                    const QList<QTransform> &oldTransforms,
                    const QList<QTransform> &newTransforms,
                    const QString &text);
    
    void undo() override;
    void redo() override;
    
private:
    DesignerView *m_view;
    QList<QGraphicsItem*> m_items;
    QList<QTransform> m_oldTransforms;
    QList<QTransform> m_newTransforms;
};

class SVGDesigner : public QWidget
{
    Q_OBJECT

public:
    explicit SVGDesigner(QWidget *parent = nullptr);
    
    void loadSvgFile(const QString &filePath);
    bool isSvgLoaded() const;
    QString currentSvgFile() const;
    
    // Material and machine settings
    void setMaterialSize(double width, double height);
    void setBedSize(double width, double height);
    
signals:
    void svgLoaded(const QString &filePath);
    void convertToGCode(const QString &svgFile);
    void selectionChanged();
    
public slots:
    void onZoomChanged(int value);
    void updateMaterialFromSettings();
    
private slots:
    void onOpenSvgClicked();
    void onConvertClicked();
    void onFitToViewClicked();
    void onSvgViewZoomChanged(double zoomFactor);
    void onSelectionChanged();
    void onMeasureUpdated(double distance, double angle);
    void onSelectToolClicked();
    void onMeasureToolClicked();
    void onPanToolClicked();
    void onGridVisibilityChanged(bool visible);
    void onGridSpacingChanged(double spacing);
    void onSnapModeChanged(int index);
    void onApplyTransformClicked();
    
private:
    // UI Components
    DesignerView *m_designerView;
    QSlider *m_zoomSlider;
    QLabel *m_fileNameLabel;
    QLabel *m_zoomLabel;
    QLabel *m_measureLabel;
    QPushButton *m_openButton;
    QPushButton *m_convertButton;
    QPushButton *m_fitButton;
    QComboBox *m_conversionModeComboBox;
    
    // Toolbar buttons for tools
    QPushButton *m_selectToolButton;
    QPushButton *m_measureToolButton;
    QPushButton *m_panToolButton;
    
    // Transform fields
    QDoubleSpinBox *m_posXSpinBox;
    QDoubleSpinBox *m_posYSpinBox;
    QDoubleSpinBox *m_scaleXSpinBox;
    QDoubleSpinBox *m_scaleYSpinBox;
    QDoubleSpinBox *m_rotationSpinBox;
    QCheckBox *m_lockAspectRatioCheckBox;
    
    // Grid and snap controls
    QCheckBox *m_gridVisibleCheckBox;
    QDoubleSpinBox *m_gridSpacingSpinBox;
    QComboBox *m_snapModeComboBox;
    
    // Undo stack for operations
    QUndoStack *m_undoStack;
    
    void setupDesignerToolbar();
    void setupStatusBar();
    void updateZoomLabel(int value);
    void updateTransformFields();
};

#endif // SVGDESIGNER_H