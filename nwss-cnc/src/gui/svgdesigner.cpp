#include "svgdesigner.h"
#include "mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QSvgRenderer>
#include <QMouseEvent>
#include <QScrollBar>
#include <QFileInfo>
#include <QGraphicsDropShadowEffect>
#include <QTimer>
#include <QInputDialog>
#include <QSvgGenerator>
#include <QtMath>
#include <QToolButton>
#include <QAction>
#include <QMenu>
#include <QColorDialog>
#include <QFontDialog>

// Define the static constant
const double DesignerView::RASTERIZATION_THRESHOLD = 2.0;

// DesignerView Implementation
DesignerView::DesignerView(QWidget *parent)
    : QGraphicsView(parent),
      m_renderer(nullptr),
      m_zoomFactor(1.0),
      m_isPanning(false),
      m_useRasterization(true),
      m_rasterizedZoomFactor(0.0),
      m_materialWidth(200.0),
      m_materialHeight(200.0),
      m_bedWidth(300.0),
      m_bedHeight(300.0),
      m_gridVisible(true),
      m_gridSpacing(10.0),
      m_rulersVisible(true),
      m_rulerWidth(25),
      m_currentTool(Select),
      m_snapMode(Grid),
      m_snapGridSize(5.0),
      m_isMeasuring(false),
      m_isMovingSelection(false)
{
    // Set up the scene
    m_scene = new QGraphicsScene(this);
    setScene(m_scene);
    
    // Configure the view
    setRenderHint(QPainter::Antialiasing, true);
    setRenderHint(QPainter::SmoothPixmapTransform, true);
    setDragMode(QGraphicsView::NoDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorUnderMouse);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    
    // Visual style
    setFrameShape(QFrame::NoFrame);
    setBackgroundBrush(QBrush(QColor(240, 240, 240)));
    
    // Set the alignment to top-left for better ruler positioning
    setAlignment(Qt::AlignLeft | Qt::AlignTop);
    
    // Set scene rect to include material and bed boundaries
    updateBoundaries();
    
    // Set viewport update mode for smoother updates
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    
    // Initialize the cursor
    updateCursor();
}

DesignerView::~DesignerView()
{
    if (m_renderer) {
        delete m_renderer;
    }
}

void DesignerView::setSvgFile(const QString &filePath)
{
    // Clear existing scene
    m_scene->clear();
    
    // Delete old renderer if it exists
    if (m_renderer) {
        delete m_renderer;
        m_renderer = nullptr;
    }
    
    m_filePath = filePath;
    
    // Try to load the SVG file
    m_renderer = new QSvgRenderer(filePath);
    if (!m_renderer->isValid()) {
        delete m_renderer;
        m_renderer = nullptr;
        QMessageBox::warning(this, tr("SVG Load Error"), 
                             tr("Failed to load SVG file: %1").arg(filePath));
        return;
    }
    
    // Create SVG items from the renderer elements
    QGraphicsSvgItem *svgItem = new QGraphicsSvgItem();
    svgItem->setSharedRenderer(m_renderer);
    svgItem->setFlags(QGraphicsItem::ItemIsSelectable | 
                     QGraphicsItem::ItemIsMovable | 
                     QGraphicsItem::ItemIsFocusable);
    m_scene->addItem(svgItem);
    
    // Get SVG bounds and set scene rect
    m_svgBounds = QRectF(QPointF(0, 0), m_renderer->defaultSize());
    
    // Update scene rect to include the SVG
    updateBoundaries();
    
    // Set a reasonable base size for rasterization
    m_baseSize = m_renderer->defaultSize();
    if (m_baseSize.width() > 1000 || m_baseSize.height() > 1000) {
        // Scale down very large SVGs
        double factor = 1000.0 / qMax(m_baseSize.width(), m_baseSize.height());
        m_baseSize = QSize(m_baseSize.width() * factor, m_baseSize.height() * factor);
    }
    
    // Reset zoom and fit the view
    resetZoom();
}

void DesignerView::updateRasterizedImage()
{
    if (!m_renderer) return;
    
    // Calculate image size based on zoom level
    QSize imageSize = m_baseSize * std::max(1.0, (m_zoomFactor / 10));
    
    // Create a new pixmap and render the SVG into it
    m_rasterizedImage = QPixmap(imageSize);
    m_rasterizedImage.fill(Qt::transparent);
    
    QPainter painter(&m_rasterizedImage);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    
    m_renderer->render(&painter);
    painter.end();
    
    // Store the zoom level we rendered at
    m_rasterizedZoomFactor = m_zoomFactor;
    
    // Update the viewport
    viewport()->update();
}

void DesignerView::drawSvgDirectly(QPainter *painter)
{
    // Direct rendering is now handled by QGraphicsScene
    // This method is kept for compatibility but is not directly used
}

void DesignerView::drawRasterizedImage(QPainter *painter)
{
    // Rasterized image rendering is now handled differently with QGraphicsSvgItem
    // This method is kept for compatibility but is not directly used
}

void DesignerView::wheelEvent(QWheelEvent *event)
{
    // Only zoom if an SVG is loaded
    if (!m_renderer) {
        QGraphicsView::wheelEvent(event);
        return;
    }
    
    // Calculate the zoom factor based on the scroll amount
    double delta = event->angleDelta().y() / 120.0;
    double factor = std::pow(1.2, delta);
    
    // Apply zoom limits
    double newZoomFactor = m_zoomFactor * factor;
    if (newZoomFactor > 50.0) {
        factor = 50.0 / m_zoomFactor;
        newZoomFactor = 50.0;
    } else if (newZoomFactor < 0.05) {
        factor = 0.05 / m_zoomFactor;
        newZoomFactor = 0.05;
    }
    
    // Store old zoom center
    QPointF oldPos = mapToScene(event->position().toPoint());
    
    // Apply the zoom
    m_zoomFactor = newZoomFactor;
    scale(factor, factor);
    
    // Center on cursor position
    QPointF newPos = mapToScene(event->position().toPoint());
    QPointF delta2 = newPos - oldPos;
    translate(delta2.x(), delta2.y());
    
    // Emit signal about zoom change
    emit zoomChanged(m_zoomFactor * 100);
    
    // If zoom crossed the rasterization threshold, update
    bool wasAboveThreshold = m_rasterizedZoomFactor >= RASTERIZATION_THRESHOLD;
    bool isAboveThreshold = m_zoomFactor >= RASTERIZATION_THRESHOLD;
    
    if (wasAboveThreshold != isAboveThreshold) {
        // Just crossed the threshold, force update
        updateRasterizedImage();
    }
    
    event->accept();
}

void DesignerView::updateCursor()
{
    switch (m_currentTool) {
        case Select:
            setCursor(Qt::ArrowCursor);
            break;
        case Measure:
            setCursor(Qt::CrossCursor);
            break;
        case Pan:
            setCursor(Qt::OpenHandCursor);
            break;
        default:
            setCursor(Qt::ArrowCursor);
            break;
    }
}

void DesignerView::mousePressEvent(QMouseEvent *event)
{
    if (!m_renderer) {
        QGraphicsView::mousePressEvent(event);
        return;
    }
    
    // Store the mouse position
    m_lastMousePos = event->pos();
    
    // Handle based on current tool
    switch (m_currentTool) {
        case Select:
            // Let the scene handle selection
            QGraphicsView::mousePressEvent(event);
            
            // Check if we clicked on a selected item
            if (event->button() == Qt::LeftButton) {
                QList<QGraphicsItem*> items = scene()->selectedItems();
                if (!items.isEmpty()) {
                    m_isMovingSelection = true;
                    m_selectionStartPos = mapToScene(event->pos());
                }
            }
            break;
            
        case Measure:
            if (event->button() == Qt::LeftButton) {
                // Start measuring
                m_isMeasuring = true;
                m_measureStart = mapToScene(event->pos());
                m_measureEnd = m_measureStart;
                
                // Snap to grid if enabled
                if (m_snapMode == Grid) {
                    m_measureStart = snapToGrid(m_measureStart);
                }
                
                emit measureUpdated(0, 0);
                viewport()->update();
            }
            break;
            
        case Pan:
            if (event->button() == Qt::LeftButton) {
                m_isPanning = true;
                setCursor(Qt::ClosedHandCursor);
            }
            break;
            
        default:
            QGraphicsView::mousePressEvent(event);
            break;
    }
}

QPointF DesignerView::snapToGrid(const QPointF &pos)
{
    if (m_snapMode == NoSnap) {
        return pos;
    }
    
    if (m_snapMode == Grid) {
        // Snap to the nearest grid intersection
        double x = round(pos.x() / m_snapGridSize) * m_snapGridSize;
        double y = round(pos.y() / m_snapGridSize) * m_snapGridSize;
        return QPointF(x, y);
    }
    
    // Object snap would be implemented here
    return pos;
}

void DesignerView::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_renderer) {
        QGraphicsView::mouseMoveEvent(event);
        return;
    }
    
    QPointF scenePos = mapToScene(event->pos());
    
    switch (m_currentTool) {
        case Select:
            if (m_isMovingSelection && (event->buttons() & Qt::LeftButton)) {
                // Move the selected items
                QPointF delta = scenePos - m_selectionStartPos;
                
                // Apply the movement
                foreach (QGraphicsItem *item, scene()->selectedItems()) {
                    item->moveBy(delta.x(), delta.y());
                }
                
                // Update start position for next move
                m_selectionStartPos = scenePos;
                
                // Signal that selection changed (position)
                emit selectionChanged();
            } else {
                // Let scene handle hover effects
                QGraphicsView::mouseMoveEvent(event);
            }
            break;
            
        case Measure:
            if (m_isMeasuring) {
                m_measureEnd = scenePos;
                
                // Snap to grid if enabled
                if (m_snapMode == Grid) {
                    m_measureEnd = snapToGrid(m_measureEnd);
                }
                
                // Calculate distance and angle
                double distance = QLineF(m_measureStart, m_measureEnd).length();
                double angle = QLineF(m_measureStart, m_measureEnd).angle();
                
                // Emit measurement update
                emit measureUpdated(distance, angle);
                
                // Update view to show the measurement line
                viewport()->update();
            }
            break;
            
        case Pan:
            if (m_isPanning && (event->buttons() & Qt::LeftButton)) {
                QPoint delta = event->pos() - m_lastMousePos;
                horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
                verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
                m_lastMousePos = event->pos();
            }
            break;
            
        default:
            QGraphicsView::mouseMoveEvent(event);
            break;
    }
}

void DesignerView::mouseReleaseEvent(QMouseEvent *event)
{
    if (!m_renderer) {
        QGraphicsView::mouseReleaseEvent(event);
        return;
    }
    
    switch (m_currentTool) {
        case Select:
            if (m_isMovingSelection) {
                m_isMovingSelection = false;
                emit selectionChanged();
            }
            QGraphicsView::mouseReleaseEvent(event);
            break;
            
        case Measure:
            if (m_isMeasuring && event->button() == Qt::LeftButton) {
                m_isMeasuring = false;
                viewport()->update();
            }
            break;
            
        case Pan:
            if (m_isPanning && event->button() == Qt::LeftButton) {
                m_isPanning = false;
                updateCursor(); // Reset to appropriate cursor
            }
            break;
            
        default:
            QGraphicsView::mouseReleaseEvent(event);
            break;
    }
}

void DesignerView::setZoom(int zoomLevel)
{
    if (!m_renderer) return;
    
    // Calculate the new zoom factor (zoomLevel is a percentage)
    double newZoomFactor = zoomLevel / 100.0;
    
    // Apply zoom limits
    if (newZoomFactor > 50.0) newZoomFactor = 50.0;
    if (newZoomFactor < 0.05) newZoomFactor = 0.05;
    
    // Store center point to maintain focus
    QPointF centerPoint = mapToScene(viewport()->rect().center());
    
    // Reset the transformation
    resetTransform();
    
    // Apply the new zoom
    scale(newZoomFactor, newZoomFactor);
    m_zoomFactor = newZoomFactor;
    
    // Center on previous center point
    centerOn(centerPoint);
    
    // Check if we crossed the rasterization threshold
    bool needsRasterUpdate = m_useRasterization && 
                            (m_zoomFactor >= RASTERIZATION_THRESHOLD) && 
                            (qAbs(m_rasterizedZoomFactor - m_zoomFactor) > 0.1);
    
    if (needsRasterUpdate) {
        updateRasterizedImage();
    }
    
    // Emit signal about zoom change
    emit zoomChanged(zoomLevel);
    
    // Update the view
    viewport()->update();
}

void DesignerView::resetZoom()
{
    if (!m_renderer) return;
    if (m_svgBounds.isNull()) return;
    
    // Reset the transformation
    resetTransform();
    
    // Fit the view to the SVG
    fitInView(m_svgBounds, Qt::KeepAspectRatio);
    
    // Calculate the actual zoom factor
    QTransform t = transform();
    m_zoomFactor = t.m11();  // m11 is the X scale factor in the transform matrix
    
    // Create initial rasterized image
    updateRasterizedImage();
    
    // Emit signal about zoom change
    emit zoomChanged(m_zoomFactor * 100);
}

void DesignerView::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);
    
    // Update view when widget is resized
    if (m_renderer && m_scene) {
        // Preserve zoom level during resize
        QTransform t;
        t.scale(m_zoomFactor, m_zoomFactor);
        setTransform(t);
        
        // Update rasterized image if needed
        if (m_useRasterization && m_zoomFactor >= RASTERIZATION_THRESHOLD) {
            updateRasterizedImage();
        }
    }
}

void DesignerView::drawBackground(QPainter *painter, const QRectF &rect)
{
    // Call base implementation to fill with background color
    QGraphicsView::drawBackground(painter, rect);
    
    // Draw grid
    if (m_gridVisible) {
        drawGrid(painter, rect);
    }
    
    // Draw bed boundary
    drawBedBoundary(painter);
    
    // Draw material boundary
    drawMaterialBoundary(painter);
}

void DesignerView::drawForeground(QPainter *painter, const QRectF &rect)
{
    // Call base implementation
    QGraphicsView::drawForeground(painter, rect);
    
    // Draw rulers
    if (m_rulersVisible) {
        drawRulers(painter, rect);
    }
    
    // Draw measurement line
    if (m_isMeasuring) {
        drawMeasureLine(painter);
    }
}

void DesignerView::drawGrid(QPainter *painter, const QRectF &rect)
{
    // Set up the painter with a semi-transparent light gray color
    QPen gridPen(QColor(180, 180, 180, 120), 0.5);
    painter->setPen(gridPen);
    
    // Calculate grid lines based on spacing and view rect
    double left = floor(rect.left() / m_gridSpacing) * m_gridSpacing;
    double top = floor(rect.top() / m_gridSpacing) * m_gridSpacing;
    double right = ceil(rect.right() / m_gridSpacing) * m_gridSpacing;
    double bottom = ceil(rect.bottom() / m_gridSpacing) * m_gridSpacing;
    
    // Draw vertical grid lines
    for (double x = left; x <= right; x += m_gridSpacing) {
        painter->drawLine(QPointF(x, rect.top()), QPointF(x, rect.bottom()));
    }
    
    // Draw horizontal grid lines
    for (double y = top; y <= bottom; y += m_gridSpacing) {
        painter->drawLine(QPointF(rect.left(), y), QPointF(rect.right(), y));
    }
    
    // Draw major grid lines (every 5 minor lines) with darker color
    gridPen.setColor(QColor(120, 120, 120, 150));
    gridPen.setWidth(1);
    painter->setPen(gridPen);
    
    int majorInterval = 5;
    
    // Vertical major lines
    for (double x = left; x <= right; x += m_gridSpacing * majorInterval) {
        painter->drawLine(QPointF(x, rect.top()), QPointF(x, rect.bottom()));
    }
    
    // Horizontal major lines
    for (double y = top; y <= bottom; y += m_gridSpacing * majorInterval) {
        painter->drawLine(QPointF(rect.left(), y), QPointF(rect.right(), y));
    }
}

void DesignerView::drawRulers(QPainter *painter, const QRectF &rect)
{
    // Save current painter state
    painter->save();
    
    // Get viewport rect and transform to scene coordinates
    QRect viewRect = viewport()->rect();
    QPointF topLeft = mapToScene(viewRect.topLeft());
    
    // Set up ruler background and font
    QBrush rulerBrush(QColor(240, 240, 240));
    QPen rulerPen(QColor(100, 100, 100));
    QPen tickPen(QColor(80, 80, 80));
    QFont rulerFont = painter->font();
    rulerFont.setPointSize(8);
    painter->setFont(rulerFont);
    
    // Horizontal ruler (top)
    QRectF hRulerRect(rect.left(), rect.top(), rect.width(), m_rulerWidth);
    painter->fillRect(hRulerRect, rulerBrush);
    painter->setPen(rulerPen);
    painter->drawLine(hRulerRect.bottomLeft(), hRulerRect.bottomRight());
    
    // Vertical ruler (left)
    QRectF vRulerRect(rect.left(), rect.top() + m_rulerWidth, m_rulerWidth, rect.height() - m_rulerWidth);
    painter->fillRect(vRulerRect, rulerBrush);
    painter->setPen(rulerPen);
    painter->drawLine(vRulerRect.topRight(), vRulerRect.bottomRight());
    
    // Draw corner square
    QRectF cornerRect(rect.left(), rect.top(), m_rulerWidth, m_rulerWidth);
    painter->fillRect(cornerRect, rulerBrush);
    painter->setPen(rulerPen);
    painter->drawRect(cornerRect);
    
    // Determine tick spacing based on zoom
    double minorTickSpacing = 5.0;  // in scene units
    double majorTickSpacing = 50.0; // in scene units
    
    if (m_zoomFactor > 2.0) {
        minorTickSpacing = 1.0;
        majorTickSpacing = 10.0;
    } else if (m_zoomFactor > 0.5) {
        minorTickSpacing = 5.0;
        majorTickSpacing = 25.0;
    } else if (m_zoomFactor < 0.2) {
        minorTickSpacing = 10.0;
        majorTickSpacing = 100.0;
    }
    
    // Draw horizontal ruler ticks
    painter->setPen(tickPen);
    double startX = floor(rect.left() / minorTickSpacing) * minorTickSpacing;
    double endX = ceil(rect.right() / minorTickSpacing) * minorTickSpacing;
    
    for (double x = startX; x <= endX; x += minorTickSpacing) {
        bool isMajorTick = fabs(fmod(x, majorTickSpacing)) < 0.1;
        double tickHeight = isMajorTick ? 10.0 : 5.0;
        
        QPointF tickTop(x, rect.top() + m_rulerWidth - tickHeight);
        QPointF tickBottom(x, rect.top() + m_rulerWidth);
        painter->drawLine(tickTop, tickBottom);
        
        if (isMajorTick) {
            QString label = QString::number(int(x));
            QRectF textRect(x - 15, rect.top() + 2, 30, m_rulerWidth - 4);
            painter->drawText(textRect, Qt::AlignCenter, label);
        }
    }
    
    // Draw vertical ruler ticks
    double startY = floor(rect.top() / minorTickSpacing) * minorTickSpacing;
    double endY = ceil(rect.bottom() / minorTickSpacing) * minorTickSpacing;
    
    for (double y = startY + m_rulerWidth; y <= endY; y += minorTickSpacing) {
        bool isMajorTick = fabs(fmod(y - m_rulerWidth, majorTickSpacing)) < 0.1;
        double tickWidth = isMajorTick ? 10.0 : 5.0;
        
        QPointF tickLeft(rect.left() + m_rulerWidth - tickWidth, y);
        QPointF tickRight(rect.left() + m_rulerWidth, y);
        painter->drawLine(tickLeft, tickRight);
        
        if (isMajorTick) {
            QString label = QString::number(int(y - m_rulerWidth));
            QRectF textRect(rect.left() + 2, y - 15, m_rulerWidth - 4, 30);
            painter->save();
            painter->translate(rect.left() + m_rulerWidth / 2, y);
            painter->rotate(-90);
            painter->drawText(QRectF(-15, -m_rulerWidth/2, 30, m_rulerWidth), Qt::AlignCenter, label);
            painter->restore();
        }
    }
    
    // Draw current mouse position indicator
    QPoint mousePos = viewport()->mapFromGlobal(QCursor::pos());
    if (viewport()->rect().contains(mousePos)) {
        QPointF scenePos = mapToScene(mousePos);
        
        // Highlight current position on rulers
        painter->setPen(Qt::red);
        
        // Horizontal indicator
        painter->drawLine(QPointF(scenePos.x(), rect.top()), 
                         QPointF(scenePos.x(), rect.top() + m_rulerWidth));
        
        // Vertical indicator
        painter->drawLine(QPointF(rect.left(), scenePos.y()), 
                         QPointF(rect.left() + m_rulerWidth, scenePos.y()));
        
        // Draw position text in corner
        QString posText = QString("X:%1 Y:%2")
                           .arg(int(scenePos.x()))
                           .arg(int(scenePos.y()));
        painter->drawText(cornerRect, Qt::AlignCenter, posText);
    }
    
    // Restore painter state
    painter->restore();
}

void DesignerView::drawMaterialBoundary(QPainter *painter)
{
    // Draw the material boundary
    painter->save();
    
    // Material is shown as a light blue rectangle
    QBrush materialBrush(QColor(200, 230, 255, 30));
    QPen materialPen(QColor(0, 100, 200), 1.5);
    materialPen.setStyle(Qt::SolidLine);
    
    painter->setPen(materialPen);
    painter->setBrush(materialBrush);
    
    QRectF materialRect(0, 0, m_materialWidth, m_materialHeight);
    painter->drawRect(materialRect);
    
    // Label the material
    painter->setPen(QColor(0, 100, 200));
    QFont font = painter->font();
    font.setBold(true);
    painter->setFont(font);
    QString label = tr("Material (%1 × %2)")
                    .arg(m_materialWidth)
                    .arg(m_materialHeight);
    
    painter->drawText(materialRect.adjusted(5, 5, -5, -5), Qt::AlignTop | Qt::AlignLeft, label);
    
    painter->restore();
}

void DesignerView::drawBedBoundary(QPainter *painter)
{
    // Draw the bed boundary
    painter->save();
    
    // Bed is shown as a light gray rectangle
    QBrush bedBrush(QColor(220, 220, 220, 20));
    QPen bedPen(QColor(100, 100, 100), 1.0);
    bedPen.setStyle(Qt::DashLine);
    
    painter->setPen(bedPen);
    painter->setBrush(bedBrush);
    
    QRectF bedRect(0, 0, m_bedWidth, m_bedHeight);
    painter->drawRect(bedRect);
    
    // Label the bed
    painter->setPen(QColor(80, 80, 80));
    QString label = tr("Bed (%1 × %2)")
                    .arg(m_bedWidth)
                    .arg(m_bedHeight);
    
    painter->drawText(bedRect.adjusted(5, 25, -5, -5), Qt::AlignTop | Qt::AlignLeft, label);
    
    painter->restore();
}

void DesignerView::drawMeasureLine(QPainter *painter)
{
    // Draw the measurement line
    painter->save();
    
    // Calculate distance and angle
    QLineF line(m_measureStart, m_measureEnd);
    double distance = line.length();
    double angle = line.angle();
    
    // Draw the line
    QPen measurePen(QColor(255, 50, 50), 1.5);
    painter->setPen(measurePen);
    painter->drawLine(line);
    
    // Draw endpoints
    painter->setBrush(QColor(255, 50, 50));
    painter->drawEllipse(m_measureStart, 3, 3);
    painter->drawEllipse(m_measureEnd, 3, 3);
    
    // Draw distance label
    QPointF midPoint = (m_measureStart + m_measureEnd) / 2;
    QString distText = QString("%1").arg(distance, 0, 'f', 1);
    
    // Create a background for the text
    QFontMetricsF fm(painter->font());
    QRectF textRect = fm.boundingRect(distText);
    textRect.moveTo(midPoint);
    textRect.adjust(-5, -3, 5, 3);
    
    // Draw text background
    painter->setBrush(QColor(255, 255, 255, 200));
    painter->setPen(Qt::NoPen);
    painter->drawRoundedRect(textRect, 3, 3);
    
    // Draw text
    painter->setPen(QColor(200, 0, 0));
    painter->drawText(textRect, Qt::AlignCenter, distText);
    
    painter->restore();
}

void DesignerView::updateBoundaries()
{
    // Calculate the scene rect to include material, bed, and some padding
    double maxWidth = std::max(m_bedWidth, m_materialWidth);
    double maxHeight = std::max(m_bedHeight, m_materialHeight);
    
    // Add padding
    double padding = 50.0;
    QRectF sceneRect(-padding, -padding, maxWidth + 2*padding, maxHeight + 2*padding);
    
    // Set the scene rect
    m_scene->setSceneRect(sceneRect);
    
    // Update the view
    viewport()->update();
}

double DesignerView::getMeasureDistance() const
{
    return QLineF(m_measureStart, m_measureEnd).length();
}

QRectF DesignerView::getSelectionBounds() const
{
    QRectF bounds;
    QList<QGraphicsItem*> items = scene()->selectedItems();
    
    if (items.isEmpty()) {
        return bounds;
    }
    
    bounds = items.first()->sceneBoundingRect();
    
    for (int i = 1; i < items.size(); ++i) {
        bounds = bounds.united(items[i]->sceneBoundingRect());
    }
    
    return bounds;
}

void DesignerView::selectAll()
{
    // Select all items in the scene
    foreach (QGraphicsItem *item, m_scene->items()) {
        item->setSelected(true);
    }
    
    emit selectionChanged();
}

void DesignerView::deselectAll()
{
    // Deselect all items in the scene
    foreach (QGraphicsItem *item, m_scene->selectedItems()) {
        item->setSelected(false);
    }
    
    emit selectionChanged();
}

void DesignerView::deleteSelected()
{
    // Delete all selected items
    QList<QGraphicsItem*> items = scene()->selectedItems();
    
    foreach (QGraphicsItem *item, items) {
        scene()->removeItem(item);
        delete item;
    }
    
    emit selectionChanged();
}

void DesignerView::centerSelected()
{
    QList<QGraphicsItem*> items = scene()->selectedItems();
    if (items.isEmpty()) return;
    
    // Get the bounding rectangle of all selected items
    QRectF selectionBounds = getSelectionBounds();
    
    // Calculate center of material
    QPointF materialCenter(m_materialWidth / 2, m_materialHeight / 2);
    
    // Calculate the offset to center the selection
    QPointF selectionCenter = selectionBounds.center();
    QPointF offset = materialCenter - selectionCenter;
    
    // Move all selected items
    foreach (QGraphicsItem *item, items) {
        item->moveBy(offset.x(), offset.y());
    }
    
    emit selectionChanged();
}

void DesignerView::moveSelectedBy(double dx, double dy)
{
    QList<QGraphicsItem*> items = scene()->selectedItems();
    if (items.isEmpty()) return;
    
    // Move all selected items
    foreach (QGraphicsItem *item, items) {
        item->moveBy(dx, dy);
    }
    
    emit selectionChanged();
}

void DesignerView::scaleSelected(double scaleX, double scaleY)
{
    QList<QGraphicsItem*> items = scene()->selectedItems();
    if (items.isEmpty()) return;
    
    // Get the bounding rectangle of all selected items
    QRectF selectionBounds = getSelectionBounds();
    QPointF center = selectionBounds.center();
    
    // Apply scaling to each item
    foreach (QGraphicsItem *item, items) {
        // Calculate item's position relative to selection center
        QPointF relPos = item->scenePos() - center;
        
        // Scale the position
        relPos.setX(relPos.x() * scaleX);
        relPos.setY(relPos.y() * scaleY);
        
        // Set new position
        item->setPos(center + relPos);
        
        // Scale the item's transform
        QTransform transform = item->transform();
        transform.scale(scaleX, scaleY);
        item->setTransform(transform);
    }
    
    emit selectionChanged();
}

void DesignerView::rotateSelected(double angle)
{
    QList<QGraphicsItem*> items = scene()->selectedItems();
    if (items.isEmpty()) return;
    
    // Get the bounding rectangle of all selected items
    QRectF selectionBounds = getSelectionBounds();
    QPointF center = selectionBounds.center();
    
    // Apply rotation to each item
    foreach (QGraphicsItem *item, items) {
        // Rotate the item around the selection center
        QTransform transform;
        transform.translate(center.x(), center.y());
        transform.rotate(angle);
        transform.translate(-center.x(), -center.y());
        
        item->setTransform(transform, true);
    }
    
    emit selectionChanged();
}

void DesignerView::exportAsSvg(const QString &filePath)
{
    if (!m_renderer) {
        return;
    }
    
    // Create an SVG generator
    QSvgGenerator generator;
    generator.setFileName(filePath);
    generator.setSize(QSize(m_materialWidth, m_materialHeight));
    generator.setViewBox(QRect(0, 0, m_materialWidth, m_materialHeight));
    generator.setTitle("NWSS CNC Design");
    generator.setDescription("Generated with NWSS CNC Designer");
    
    // Create a painter to paint on the SVG generator
    QPainter painter;
    painter.begin(&generator);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Draw all visible elements of the scene
    m_scene->render(&painter, QRectF(), QRectF(0, 0, m_materialWidth, m_materialHeight));
    
    painter.end();
}

// TransformCommand implementation
TransformCommand::TransformCommand(DesignerView *view,
                                 const QList<QGraphicsItem*> &items,
                                 const QList<QTransform> &oldTransforms,
                                 const QList<QTransform> &newTransforms,
                                 const QString &text)
    : QUndoCommand(text),
      m_view(view),
      m_items(items),
      m_oldTransforms(oldTransforms),
      m_newTransforms(newTransforms)
{
}

void TransformCommand::undo()
{
    // Apply old transforms to items
    for (int i = 0; i < m_items.size(); ++i) {
        if (i < m_oldTransforms.size()) {
            m_items[i]->setTransform(m_oldTransforms[i]);
        }
    }
    
    // Notify view of the change
    if (m_view) {
        emit m_view->selectionChanged();
    }
}

void TransformCommand::redo()
{
    // Apply new transforms to items
    for (int i = 0; i < m_items.size(); ++i) {
        if (i < m_newTransforms.size()) {
            m_items[i]->setTransform(m_newTransforms[i]);
        }
    }
    
    // Notify view of the change
    if (m_view) {
        emit m_view->selectionChanged();
    }
}

// SVGDesigner Implementation
SVGDesigner::SVGDesigner(QWidget *parent)
    : QWidget(parent)
{
    // Create main layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    // Set up the toolbar
    setupDesignerToolbar();
    
    // Create the designer view
    m_designerView = new DesignerView(this);
    mainLayout->addWidget(m_designerView);
    
    // Connect the zoom changed signal
    connect(m_designerView, &DesignerView::zoomChanged, this, &SVGDesigner::onSvgViewZoomChanged);
    
    // Connect selection changed signal
    connect(m_designerView, &DesignerView::selectionChanged, this, &SVGDesigner::onSelectionChanged);
    
    // Connect measure updated signal
    connect(m_designerView, &DesignerView::measureUpdated, this, &SVGDesigner::onMeasureUpdated);
    
    // Set up the status bar
    setupStatusBar();
    
    // Initialize undo stack
    m_undoStack = new QUndoStack(this);
    
    // Disable the convert button initially
    m_convertButton->setEnabled(false);
    m_fitButton->setEnabled(false);
}

void SVGDesigner::onSvgViewZoomChanged(double zoomFactor)
{
    // Update slider without triggering signals
    m_zoomSlider->blockSignals(true);
    m_zoomSlider->setValue(qRound(zoomFactor));
    m_zoomSlider->blockSignals(false);
    
    // Update zoom label
    updateZoomLabel(qRound(zoomFactor));
}

void SVGDesigner::setupDesignerToolbar()
{
    QToolBar *toolbar = new QToolBar(this);
    toolbar->setMovable(false);
    toolbar->setFloatable(false);
    
    // Open SVG button
    m_openButton = new QPushButton(tr("Open SVG"));
    toolbar->addWidget(m_openButton);
    
    toolbar->addSeparator();
    
    // Tool selection
    m_selectToolButton = new QPushButton(tr("Select"));
    m_selectToolButton->setCheckable(true);
    m_selectToolButton->setChecked(true);
    toolbar->addWidget(m_selectToolButton);
    
    m_measureToolButton = new QPushButton(tr("Measure"));
    m_measureToolButton->setCheckable(true);
    toolbar->addWidget(m_measureToolButton);
    
    m_panToolButton = new QPushButton(tr("Pan"));
    m_panToolButton->setCheckable(true);
    toolbar->addWidget(m_panToolButton);
    
    toolbar->addSeparator();
    
    // Zoom controls
    toolbar->addWidget(new QLabel(tr("Zoom:")));
    m_zoomSlider = new QSlider(Qt::Horizontal);
    m_zoomSlider->setMinimum(10);   // 10%
    m_zoomSlider->setMaximum(500);  // 500%
    m_zoomSlider->setValue(100);    // 100%
    m_zoomSlider->setMaximumWidth(150);
    toolbar->addWidget(m_zoomSlider);
    
    m_zoomLabel = new QLabel("100%");
    m_zoomLabel->setMinimumWidth(50);
    toolbar->addWidget(m_zoomLabel);
    
    m_fitButton = new QPushButton(tr("Fit to View"));
    toolbar->addWidget(m_fitButton);
    
    toolbar->addSeparator();
    
    // Grid controls
    m_gridVisibleCheckBox = new QCheckBox(tr("Grid"));
    m_gridVisibleCheckBox->setChecked(true);
    toolbar->addWidget(m_gridVisibleCheckBox);
    
    toolbar->addWidget(new QLabel(tr("Grid Size:")));
    m_gridSpacingSpinBox = new QDoubleSpinBox();
    m_gridSpacingSpinBox->setRange(1.0, 100.0);
    m_gridSpacingSpinBox->setValue(10.0);
    m_gridSpacingSpinBox->setSingleStep(1.0);
    m_gridSpacingSpinBox->setSuffix(" mm");
    toolbar->addWidget(m_gridSpacingSpinBox);
    
    toolbar->addWidget(new QLabel(tr("Snap:")));
    m_snapModeComboBox = new QComboBox();
    m_snapModeComboBox->addItem(tr("None"), DesignerView::NoSnap);
    m_snapModeComboBox->addItem(tr("Grid"), DesignerView::Grid);
    m_snapModeComboBox->addItem(tr("Object"), DesignerView::Object);
    m_snapModeComboBox->setCurrentIndex(1);  // Grid by default
    toolbar->addWidget(m_snapModeComboBox);
    
    toolbar->addSeparator();
    
    m_convertButton = new QPushButton(tr("Convert to GCode"));
    m_convertButton->setStyleSheet("background-color: #4CAF50; color: white;");
    toolbar->addWidget(m_convertButton);
    
    // Add the toolbar to the layout
    static_cast<QVBoxLayout*>(layout())->addWidget(toolbar);
    
    // Connect signals
    connect(m_openButton, &QPushButton::clicked, this, &SVGDesigner::onOpenSvgClicked);
    connect(m_zoomSlider, &QSlider::valueChanged, this, &SVGDesigner::onZoomChanged);
    connect(m_fitButton, &QPushButton::clicked, this, &SVGDesigner::onFitToViewClicked);
    connect(m_convertButton, &QPushButton::clicked, this, &SVGDesigner::onConvertClicked);
    
    // Connect tool buttons
    connect(m_selectToolButton, &QPushButton::clicked, this, &SVGDesigner::onSelectToolClicked);
    connect(m_measureToolButton, &QPushButton::clicked, this, &SVGDesigner::onMeasureToolClicked);
    connect(m_panToolButton, &QPushButton::clicked, this, &SVGDesigner::onPanToolClicked);
    
    // Connect grid and snap controls
    connect(m_gridVisibleCheckBox, &QCheckBox::toggled, this, &SVGDesigner::onGridVisibilityChanged);
    connect(m_gridSpacingSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
            this, &SVGDesigner::onGridSpacingChanged);
    connect(m_snapModeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SVGDesigner::onSnapModeChanged);
}

void SVGDesigner::setupStatusBar()
{
    QHBoxLayout *statusLayout = new QHBoxLayout();
    statusLayout->setContentsMargins(5, 2, 5, 2);
    
    // File name display
    m_fileNameLabel = new QLabel(tr("No file loaded"));
    statusLayout->addWidget(m_fileNameLabel);
    
    // Add spacer to push items to left and right edges
    statusLayout->addStretch();
    
    // Measurement info
    m_measureLabel = new QLabel("");
    statusLayout->addWidget(m_measureLabel);
    
    // Add the status bar to the layout
    static_cast<QVBoxLayout*>(layout())->addLayout(statusLayout);
}

void SVGDesigner::loadSvgFile(const QString &filePath)
{
    m_designerView->setSvgFile(filePath);
    
    if (m_designerView->isSvgLoaded()) {
        // Update UI
        QFileInfo fileInfo(filePath);
        m_fileNameLabel->setText(fileInfo.fileName());
        m_convertButton->setEnabled(true);
        m_fitButton->setEnabled(true);
        
        // Reset zoom slider to 100%
        m_zoomSlider->setValue(100);
        
        // Emit signal that SVG was loaded
        emit svgLoaded(filePath);
    }
}

bool SVGDesigner::isSvgLoaded() const
{
    return m_designerView->isSvgLoaded();
}

QString SVGDesigner::currentSvgFile() const
{
    return m_designerView->svgFilePath();
}

void SVGDesigner::onZoomChanged(int value)
{
    m_designerView->setZoom(value);
    updateZoomLabel(value);
}

void SVGDesigner::updateZoomLabel(int value)
{
    m_zoomLabel->setText(QString("%1%").arg(value));
}

void SVGDesigner::onOpenSvgClicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("Open SVG File"),
                                                   QString(),
                                                   tr("SVG Files (*.svg)"));
    if (!filePath.isEmpty()) {
        loadSvgFile(filePath);
    }
}

void SVGDesigner::onConvertClicked()
{
    if (!isSvgLoaded()) {
        return;
    }
    
    // Emit signal to request conversion
    emit convertToGCode(currentSvgFile());
}

void SVGDesigner::onFitToViewClicked()
{
    // Only proceed if an SVG is loaded
    if (!m_designerView->isSvgLoaded()) {
        return;
    }
    
    m_designerView->resetZoom();
    
    // Update zoom slider to match the actual zoom level
    double zoomPercent = m_designerView->currentZoomFactor() * 100;
    
    // Block signals to prevent feedback loop
    m_zoomSlider->blockSignals(true);
    m_zoomSlider->setValue(qRound(zoomPercent));
    m_zoomSlider->blockSignals(false);
    
    updateZoomLabel(qRound(zoomPercent));
}

void SVGDesigner::onSelectionChanged()
{
    // Update properties panel fields
    updateTransformFields();
    
    // Emit signal for other components to react
    emit selectionChanged();
}

void SVGDesigner::updateTransformFields()
{
    QRectF selectionBounds = m_designerView->getSelectionBounds();
    
    if (selectionBounds.isValid()) {
        // Block signals during updates
        m_posXSpinBox->blockSignals(true);
        m_posYSpinBox->blockSignals(true);
        
        // Update position fields
        m_posXSpinBox->setValue(selectionBounds.x());
        m_posYSpinBox->setValue(selectionBounds.y());
        
        // Enable fields
        m_posXSpinBox->setEnabled(true);
        m_posYSpinBox->setEnabled(true);
        m_scaleXSpinBox->setEnabled(true);
        m_scaleYSpinBox->setEnabled(true);
        m_rotationSpinBox->setEnabled(true);
        
        // Unblock signals
        m_posXSpinBox->blockSignals(false);
        m_posYSpinBox->blockSignals(false);
    } else {
        // No selection, disable fields
        m_posXSpinBox->setEnabled(false);
        m_posYSpinBox->setEnabled(false);
        m_scaleXSpinBox->setEnabled(false);
        m_scaleYSpinBox->setEnabled(false);
        m_rotationSpinBox->setEnabled(false);
    }
}

void SVGDesigner::onMeasureUpdated(double distance, double angle)
{
    // Update the measurement label in the status bar
    QString measureText = tr("Distance: %1 mm  Angle: %2°")
                         .arg(distance, 0, 'f', 2)
                         .arg(angle, 0, 'f', 1);
    
    m_measureLabel->setText(measureText);
}

void SVGDesigner::onSelectToolClicked()
{
    // Update tool buttons
    m_selectToolButton->setChecked(true);
    m_measureToolButton->setChecked(false);
    m_panToolButton->setChecked(false);
    
    // Set the current tool
    m_designerView->setCurrentTool(DesignerView::Select);
}

void SVGDesigner::onMeasureToolClicked()
{
    // Update tool buttons
    m_selectToolButton->setChecked(false);
    m_measureToolButton->setChecked(true);
    m_panToolButton->setChecked(false);
    
    // Set the current tool
    m_designerView->setCurrentTool(DesignerView::Measure);
}

void SVGDesigner::onPanToolClicked()
{
    // Update tool buttons
    m_selectToolButton->setChecked(false);
    m_measureToolButton->setChecked(false);
    m_panToolButton->setChecked(true);
    
    // Set the current tool
    m_designerView->setCurrentTool(DesignerView::Pan);
}

void SVGDesigner::onGridVisibilityChanged(bool visible)
{
    // Update the grid visibility
    m_designerView->setGridVisible(visible);
}

void SVGDesigner::onGridSpacingChanged(double spacing)
{
    // Update the grid spacing
    m_designerView->setGridSpacing(spacing);
}

void SVGDesigner::onSnapModeChanged(int index)
{
    // Get the snap mode from the combo box
    DesignerView::Snap snapMode = static_cast<DesignerView::Snap>(
        m_snapModeComboBox->itemData(index).toInt());
    
    // Set the snap mode
    m_designerView->setSnapMode(snapMode);
}

void SVGDesigner::onApplyTransformClicked()
{
    // Apply the transformation settings to the selected items
    double newX = m_posXSpinBox->value();
    double newY = m_posYSpinBox->value();
    double scaleX = m_scaleXSpinBox->value();
    double scaleY = m_scaleYSpinBox->value();
    double rotation = m_rotationSpinBox->value();
    
    // Get the current selection bounds
    QRectF currentBounds = m_designerView->getSelectionBounds();
    
    if (!currentBounds.isValid()) {
        return;
    }
    
    // Calculate position delta
    double deltaX = newX - currentBounds.x();
    double deltaY = newY - currentBounds.y();
    
    // Apply transformations
    if (deltaX != 0 || deltaY != 0) {
        m_designerView->moveSelectedBy(deltaX, deltaY);
    }
    
    if (scaleX != 1.0 || scaleY != 1.0) {
        m_designerView->scaleSelected(scaleX, scaleY);
        
        // Reset scale spinners after applying
        m_scaleXSpinBox->blockSignals(true);
        m_scaleYSpinBox->blockSignals(true);
        m_scaleXSpinBox->setValue(1.0);
        m_scaleYSpinBox->setValue(1.0);
        m_scaleXSpinBox->blockSignals(false);
        m_scaleYSpinBox->blockSignals(false);
    }
    
    if (rotation != 0) {
        m_designerView->rotateSelected(rotation);
        
        // Reset rotation spinner after applying
        m_rotationSpinBox->blockSignals(true);
        m_rotationSpinBox->setValue(0);
        m_rotationSpinBox->blockSignals(false);
    }
    
    // Update fields with new values
    updateTransformFields();
}

void SVGDesigner::setMaterialSize(double width, double height)
{
    m_designerView->setMaterialSize(width, height);
}

void SVGDesigner::setBedSize(double width, double height)
{
    m_designerView->setBedSize(width, height);
}

void SVGDesigner::updateMaterialFromSettings()
{
    // This method would be connected to the GCodeOptionsPanel to update material settings
    // Implementation depends on how the GCodeOptionsPanel is structured
}