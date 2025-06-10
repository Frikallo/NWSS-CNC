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
#include <QGroupBox>

// Define the static constant
const double DesignerView::RASTERIZATION_THRESHOLD = 2.0;
const double DesignerView::HANDLE_SIZE = 8.0;
const double DesignerView::MIN_DESIGN_SIZE = 1.0;

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
      m_currentTool(Select),
      m_snapMode(Grid),
      m_snapGridSize(5.0),
      m_isMeasuring(false),
      m_isMovingSelection(false),
      m_designBoundingBox(nullptr),
      m_widthLabel(nullptr),
      m_heightLabel(nullptr),
      m_isResizing(false),
      m_activeResizeHandle(-1),
      m_designScale(1.0),
      m_isMovingBoundingBox(false)
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
    
    // Set the alignment to top-left
    setAlignment(Qt::AlignLeft | Qt::AlignTop);
    
    // Set scene rect to include material and bed boundaries
    updateBoundaries();
    
    // Set viewport update mode for smoother updates
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    
    // Initialize the cursor
    updateCursor();
    
    // Create the design bounding box (initially hidden)
    createDesignBoundingBox();
    updateDesignBoundingBox();
    
    // Initially sync SVG with the design bounds
    updateSvgItemsTransform();
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
    
    // Reset bounding box pointers since scene->clear() deleted them
    m_designBoundingBox = nullptr;
    m_widthLabel = nullptr;
    m_heightLabel = nullptr;
    m_resizeHandles.clear();
    
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
    
    // Store reference to SVG items
    m_svgItems.clear();
    m_svgItems.append(svgItem);
    
    // Get SVG bounds and set scene rect
    m_svgBounds = QRectF(QPointF(0, 0), m_renderer->defaultSize());
    m_originalSvgBounds = m_svgBounds;  // Store original bounds for scaling reference
    
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
    
    // Initialize design bounds to match SVG bounds
    m_currentDesignBounds = m_svgBounds;
    
    // Recreate the design bounding box
    createDesignBoundingBox();
    updateDesignBoundingBox();
    
    // Initially sync SVG with the design bounds
    updateSvgItemsTransform();
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
            // Check if we clicked on a resize handle first
            if (m_designBoundingBox && m_designBoundingBox->isVisible()) {
                QPointF scenePos = mapToScene(event->pos());
                int handleIndex = getResizeHandleAt(scenePos);
                if (handleIndex >= 0) {
                    m_isResizing = true;
                    m_activeResizeHandle = handleIndex;
                    m_resizeStartPos = scenePos;
                    setCursor(Qt::SizeFDiagCursor);
                    break;
                }
                
                // Check if we clicked on the bounding box itself for moving
                if (m_designBoundingBox->contains(m_designBoundingBox->mapFromScene(scenePos))) {
                    m_isMovingBoundingBox = true;
                    m_boundingBoxMoveStartPos = scenePos;
                    setCursor(Qt::SizeAllCursor);
                    break;
                }
            }
            
            // Let the scene handle selection
            QGraphicsView::mousePressEvent(event);
            
            // Check if we clicked on a selected item
            if (event->button() == Qt::LeftButton) {
                QList<QGraphicsItem*> items = scene()->selectedItems();
                if (!items.isEmpty()) {
                    // If bounding box is visible, don't allow direct SVG manipulation
                    if (m_designBoundingBox && m_designBoundingBox->isVisible()) {
                        // Deselect SVG items to prevent direct manipulation
                        for (QGraphicsItem *item : items) {
                            if (m_svgItems.contains(item)) {
                                item->setSelected(false);
                            }
                        }
                    } else {
                        m_isMovingSelection = true;
                        m_selectionStartPos = mapToScene(event->pos());
                    }
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
            if (m_isResizing && (event->buttons() & Qt::LeftButton)) {
                // Handle bounding box resizing
                performResize(scenePos);
            } else if (m_isMovingBoundingBox && (event->buttons() & Qt::LeftButton)) {
                // Handle bounding box movement
                QPointF delta = scenePos - m_boundingBoxMoveStartPos;
                m_currentDesignBounds.translate(delta);
                updateDesignBoundingBox();
                updateSvgItemsTransform();
                m_boundingBoxMoveStartPos = scenePos;
            } else if (m_isMovingSelection && (event->buttons() & Qt::LeftButton)) {
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
                // Check for hover over resize handles
                if (m_designBoundingBox && m_designBoundingBox->isVisible()) {
                    int handleIndex = getResizeHandleAt(scenePos);
                    if (handleIndex >= 0) {
                        setCursor(Qt::SizeFDiagCursor);
                    } else if (m_designBoundingBox->contains(m_designBoundingBox->mapFromScene(scenePos))) {
                        setCursor(Qt::SizeAllCursor);
                    } else {
                        updateCursor();
                    }
                }
                
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
            if (m_isResizing) {
                m_isResizing = false;
                m_activeResizeHandle = -1;
                updateCursor();
                emit designBoundsChanged(m_currentDesignBounds, m_designScale);
            } else if (m_isMovingBoundingBox) {
                m_isMovingBoundingBox = false;
                updateCursor();
                emit designBoundsChanged(m_currentDesignBounds, m_designScale);
            } else if (m_isMovingSelection) {
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

// Design bounding box methods
void DesignerView::createDesignBoundingBox()
{
    // Don't create if already exists
    if (m_designBoundingBox) return;
    
    // Create the main bounding box rectangle
    m_designBoundingBox = new QGraphicsRectItem();
    m_designBoundingBox->setPen(QPen(QColor(0, 120, 215), 2, Qt::DashLine));
    m_designBoundingBox->setBrush(Qt::NoBrush);
    m_designBoundingBox->setVisible(false);
    m_scene->addItem(m_designBoundingBox);
    
    // Create resize handles
    createResizeHandles();
    
    // Create size labels
    m_widthLabel = new QGraphicsTextItem();
    m_widthLabel->setDefaultTextColor(QColor(0, 120, 215));
    m_widthLabel->setFont(QFont("Arial", 10, QFont::Bold));
    m_widthLabel->setVisible(false);
    m_scene->addItem(m_widthLabel);
    
    m_heightLabel = new QGraphicsTextItem();
    m_heightLabel->setDefaultTextColor(QColor(0, 120, 215));
    m_heightLabel->setFont(QFont("Arial", 10, QFont::Bold));
    m_heightLabel->setVisible(false);
    m_scene->addItem(m_heightLabel);
}

void DesignerView::createResizeHandles()
{
    // Don't create if already exist
    if (!m_resizeHandles.isEmpty()) return;
    
    // Create 8 resize handles (corners and midpoints)
    for (int i = 0; i < 8; ++i) {
        QGraphicsRectItem *handle = new QGraphicsRectItem();
        handle->setRect(-HANDLE_SIZE/2, -HANDLE_SIZE/2, HANDLE_SIZE, HANDLE_SIZE);
        handle->setPen(QPen(QColor(0, 120, 215), 1));
        handle->setBrush(QBrush(Qt::white));
        handle->setVisible(false);
        m_resizeHandles.append(handle);
        m_scene->addItem(handle);
    }
}

void DesignerView::updateDesignBoundingBox()
{
    if (!m_designBoundingBox) return;
    
    // Update the bounding box rectangle
    m_designBoundingBox->setRect(m_currentDesignBounds);
    
    // Update resize handles
    updateResizeHandles();
    
    // Update size labels
    updateSizeLabels();
}

void DesignerView::updateResizeHandles()
{
    if (m_resizeHandles.isEmpty()) return;
    
    QRectF rect = m_currentDesignBounds;
    
    // Position handles: 0=TL, 1=T, 2=TR, 3=R, 4=BR, 5=B, 6=BL, 7=L
    QPointF positions[8] = {
        rect.topLeft(),                                    // 0: Top-left
        QPointF(rect.center().x(), rect.top()),          // 1: Top
        rect.topRight(),                                   // 2: Top-right
        QPointF(rect.right(), rect.center().y()),         // 3: Right
        rect.bottomRight(),                                // 4: Bottom-right
        QPointF(rect.center().x(), rect.bottom()),        // 5: Bottom
        rect.bottomLeft(),                                 // 6: Bottom-left
        QPointF(rect.left(), rect.center().y())           // 7: Left
    };
    
    for (int i = 0; i < 8 && i < m_resizeHandles.size(); ++i) {
        m_resizeHandles[i]->setPos(positions[i]);
    }
}

void DesignerView::updateSizeLabels()
{
    if (!m_widthLabel || !m_heightLabel) return;
    
    QRectF rect = m_currentDesignBounds;
    
    // Update width label
    QString widthText = QString("%1 mm").arg(rect.width(), 0, 'f', 1);
    m_widthLabel->setPlainText(widthText);
    QPointF widthPos(rect.center().x() - m_widthLabel->boundingRect().width()/2, 
                     rect.bottom() + 5);
    m_widthLabel->setPos(widthPos);
    
    // Update height label
    QString heightText = QString("%1 mm").arg(rect.height(), 0, 'f', 1);
    m_heightLabel->setPlainText(heightText);
    
    // Rotate height label 90 degrees
    m_heightLabel->setRotation(-90);
    QPointF heightPos(rect.left() - 25, 
                      rect.center().y() + m_heightLabel->boundingRect().width()/2);
    m_heightLabel->setPos(heightPos);
}

int DesignerView::getResizeHandleAt(const QPointF &pos)
{
    for (int i = 0; i < m_resizeHandles.size(); ++i) {
        QRectF handleRect = m_resizeHandles[i]->sceneBoundingRect();
        if (handleRect.contains(pos)) {
            return i;
        }
    }
    return -1;
}

void DesignerView::performResize(const QPointF &currentPos)
{
    if (m_activeResizeHandle < 0) return;
    
    QRectF newRect = m_currentDesignBounds;
    QPointF delta = currentPos - m_resizeStartPos;
    
    // Apply resize based on which handle is being dragged
    switch (m_activeResizeHandle) {
        case 0: // Top-left
            newRect.setTopLeft(newRect.topLeft() + delta);
            break;
        case 1: // Top
            newRect.setTop(newRect.top() + delta.y());
            break;
        case 2: // Top-right
            newRect.setTopRight(newRect.topRight() + delta);
            break;
        case 3: // Right
            newRect.setRight(newRect.right() + delta.x());
            break;
        case 4: // Bottom-right
            newRect.setBottomRight(newRect.bottomRight() + delta);
            break;
        case 5: // Bottom
            newRect.setBottom(newRect.bottom() + delta.y());
            break;
        case 6: // Bottom-left
            newRect.setBottomLeft(newRect.bottomLeft() + delta);
            break;
        case 7: // Left
            newRect.setLeft(newRect.left() + delta.x());
            break;
    }
    
    // Enforce minimum size
    if (newRect.width() < MIN_DESIGN_SIZE) {
        if (m_activeResizeHandle == 0 || m_activeResizeHandle == 6 || m_activeResizeHandle == 7) {
            newRect.setLeft(newRect.right() - MIN_DESIGN_SIZE);
        } else {
            newRect.setRight(newRect.left() + MIN_DESIGN_SIZE);
        }
    }
    
    if (newRect.height() < MIN_DESIGN_SIZE) {
        if (m_activeResizeHandle == 0 || m_activeResizeHandle == 1 || m_activeResizeHandle == 2) {
            newRect.setTop(newRect.bottom() - MIN_DESIGN_SIZE);
        } else {
            newRect.setBottom(newRect.top() + MIN_DESIGN_SIZE);
        }
    }
    
    // Update the design bounds
    m_currentDesignBounds = newRect;
    
    // Calculate new scale based on original SVG size
    if (!m_svgBounds.isEmpty()) {
        double scaleX = newRect.width() / m_svgBounds.width();
        double scaleY = newRect.height() / m_svgBounds.height();
        m_designScale = qMax(scaleX, scaleY); // Use uniform scaling
    }
    
    // Update the visual representation
    updateDesignBoundingBox();
    
    // Update SVG items to match the new bounds
    updateSvgItemsTransform();
    
    // Update the resize start position for next move
    m_resizeStartPos = currentPos;
}

void DesignerView::showDesignBoundingBox(bool show)
{
    if (!m_designBoundingBox) return;
    
    m_designBoundingBox->setVisible(show);
    
    if (m_widthLabel) {
        m_widthLabel->setVisible(show);
    }
    if (m_heightLabel) {
        m_heightLabel->setVisible(show);
    }
    
    for (QGraphicsRectItem *handle : m_resizeHandles) {
        if (handle) {
            handle->setVisible(show);
        }
    }
    
    // Control SVG item interactivity based on bounding box visibility
    for (QGraphicsItem *item : m_svgItems) {
        if (item) {
            if (show) {
                // Disable direct manipulation when bounding box is active
                item->setFlags(QGraphicsItem::ItemIsSelectable);
                item->setSelected(false);  // Deselect any selected items
            } else {
                // Re-enable normal interaction when bounding box is hidden
                item->setFlags(QGraphicsItem::ItemIsSelectable | 
                              QGraphicsItem::ItemIsMovable | 
                              QGraphicsItem::ItemIsFocusable);
            }
        }
    }
}

void DesignerView::setDesignSize(double width, double height)
{
    if (width < MIN_DESIGN_SIZE || height < MIN_DESIGN_SIZE) return;
    
    // Keep the center position the same
    QPointF center = m_currentDesignBounds.center();
    m_currentDesignBounds.setSize(QSizeF(width, height));
    m_currentDesignBounds.moveCenter(center);
    
    // Calculate new scale
    if (!m_svgBounds.isEmpty()) {
        double scaleX = width / m_svgBounds.width();
        double scaleY = height / m_svgBounds.height();
        m_designScale = qMax(scaleX, scaleY);
    }
    
    updateDesignBoundingBox();
    
    // Update SVG items to match the new size
    updateSvgItemsTransform();
    
    emit designBoundsChanged(m_currentDesignBounds, m_designScale);
}

QList<QGraphicsItem*> DesignerView::getSvgItems() const
{
    return m_svgItems;
}

void DesignerView::updateSvgItemsTransform()
{
    if (m_svgItems.isEmpty() || m_originalSvgBounds.isEmpty()) return;
    
    // Calculate the scale and position needed to fit SVG into design bounds
    double scaleX = m_currentDesignBounds.width() / m_originalSvgBounds.width();
    double scaleY = m_currentDesignBounds.height() / m_originalSvgBounds.height();
    
    // Use uniform scaling to maintain aspect ratio
    double scale = qMin(scaleX, scaleY);
    
    // Calculate the position to center the scaled SVG in the design bounds
    double scaledWidth = m_originalSvgBounds.width() * scale;
    double scaledHeight = m_originalSvgBounds.height() * scale;
    
    QPointF targetPos = m_currentDesignBounds.center() - QPointF(scaledWidth/2, scaledHeight/2);
    
    // Apply transform to all SVG items
    for (QGraphicsItem *item : m_svgItems) {
        if (item) {
            // Reset transform first
            item->setTransform(QTransform());
            
            // Apply scaling
            item->setScale(scale);
            
            // Apply position
            item->setPos(targetPos);
        }
    }
    
    // Update the design scale
    m_designScale = scale;
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
    
    // Connect measure updated signal
    connect(m_designerView, &DesignerView::measureUpdated, this, &SVGDesigner::onMeasureUpdated);
    
    // Connect design bounds changed signal
    connect(m_designerView, &DesignerView::designBoundsChanged, this, &SVGDesigner::onDesignBoundsChanged);
    
    // Set up the status bar
    setupStatusBar();
    
    // Initialize undo stack
    m_undoStack = new QUndoStack(this);
    
    m_zoomSlider->setEnabled(false);
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
    
    // Add the toolbar to the layout
    static_cast<QVBoxLayout*>(layout())->addWidget(toolbar);
    
    // Connect signals
    connect(m_openButton, &QPushButton::clicked, this, &SVGDesigner::onOpenSvgClicked);
    connect(m_zoomSlider, &QSlider::valueChanged, this, &SVGDesigner::onZoomChanged);
    connect(m_fitButton, &QPushButton::clicked, this, &SVGDesigner::onFitToViewClicked);

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
        m_zoomSlider->setEnabled(true);
        m_fitButton->setEnabled(true);
        
        // Reset zoom slider to 100%
        m_zoomSlider->setValue(100);
        
        // Emit signal that SVG was loaded
        emit svgLoaded(filePath);
        m_designerView->showDesignBoundingBox(true);
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
    
    // Get design transformation information
    QRectF designBounds = m_designerView->getDesignBounds();
    double designScale = m_designerView->getDesignScale();
    
    // Calculate offset from material origin (bottom-left)
    QPointF designOffset = designBounds.topLeft();  // Top-left in screen coordinates
    
    // Check if design fits within material bounds
    double materialWidth = m_designerView->getMaterialWidth();
    double materialHeight = m_designerView->getMaterialHeight();
    
    bool fitsInMaterial = true;
    QStringList warnings;
    
    // Check if design extends beyond material boundaries
    if (designBounds.left() < 0) {
        warnings << tr("Design extends %1 mm beyond the left edge of the material").arg(-designBounds.left(), 0, 'f', 2);
        fitsInMaterial = false;
    }
    if (designBounds.top() < 0) {
        warnings << tr("Design extends %1 mm beyond the top edge of the material").arg(-designBounds.top(), 0, 'f', 2);
        fitsInMaterial = false;
    }
    if (designBounds.right() > materialWidth) {
        warnings << tr("Design extends %1 mm beyond the right edge of the material").arg(designBounds.right() - materialWidth, 0, 'f', 2);
        fitsInMaterial = false;
    }
    if (designBounds.bottom() > materialHeight) {
        warnings << tr("Design extends %1 mm beyond the bottom edge of the material").arg(designBounds.bottom() - materialHeight, 0, 'f', 2);
        fitsInMaterial = false;
    }
    
    // Show warning dialog if design doesn't fit
    if (!fitsInMaterial) {
        QMessageBox warningBox(this);
        warningBox.setIcon(QMessageBox::Warning);
        warningBox.setWindowTitle(tr("Design Size Warning"));
        warningBox.setText(tr("The design does not fit within the material boundaries."));
        
        QString detailText = tr("Material size: %1 x %2 mm\n")
                                .arg(materialWidth, 0, 'f', 1)
                                .arg(materialHeight, 0, 'f', 1);
        detailText += tr("Design size: %1 x %2 mm\n")
                         .arg(designBounds.width(), 0, 'f', 1)
                         .arg(designBounds.height(), 0, 'f', 1);
        detailText += tr("Design position: (%1, %2) mm\n\n")
                         .arg(designBounds.left(), 0, 'f', 1)
                         .arg(designBounds.top(), 0, 'f', 1);
        detailText += tr("Issues found:\n• %1").arg(warnings.join("\n• "));
        
        warningBox.setDetailedText(detailText);
        warningBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
        warningBox.setDefaultButton(QMessageBox::Cancel);
        warningBox.button(QMessageBox::Yes)->setText(tr("Continue Anyway"));
        warningBox.button(QMessageBox::Cancel)->setText(tr("Cancel"));
        
        int result = warningBox.exec();
        if (result != QMessageBox::Yes) {
            return;  // User cancelled
        }
    }
    
    // Emit signal to request conversion with design information
    // Note: G-code generation is now handled by the unified G-code options panel system
    // Users should use the "Generate G-Code" button in the G-code options panel
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

void SVGDesigner::onDesignBoundsChanged(QRectF bounds, double scale)
{
    // This information can be used by the G-Code generator
    // The bounds and scale provide the final dimensions and scaling factor
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

QRectF SVGDesigner::getDesignBounds() const
{
    if (m_designerView) {
        return m_designerView->getDesignBounds();
    }
    return QRectF();
}

double SVGDesigner::getDesignScale() const
{
    if (m_designerView) {
        return m_designerView->getDesignScale();
    }
    return 1.0;
}

QPointF SVGDesigner::getDesignOffset() const
{
    if (m_designerView) {
        QRectF bounds = m_designerView->getDesignBounds();
        return bounds.topLeft();
    }
    return QPointF();
}