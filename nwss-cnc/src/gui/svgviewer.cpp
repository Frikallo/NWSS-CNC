// svgviewer.cpp
#include "svgviewer.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QSvgRenderer>
#include <QMouseEvent>
#include <QScrollBar>
#include <QFileInfo>
#include <QGraphicsDropShadowEffect>
#include <QTimer>

// SVGView Implementation
SVGView::SVGView(QWidget *parent)
    : QGraphicsView(parent),
      m_renderer(nullptr),
      m_zoomFactor(1.0),
      m_isPanning(false),
      m_useRasterization(true),
      m_rasterizedZoomFactor(0.0)
{
    // Set up the scene
    m_scene = new QGraphicsScene(this);
    setScene(m_scene);
    
    // Configure the view
    setRenderHint(QPainter::Antialiasing, true);
    setRenderHint(QPainter::SmoothPixmapTransform, true);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorUnderMouse);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    
    // Visual style
    setFrameShape(QFrame::NoFrame);
    setBackgroundBrush(QBrush(QColor(240, 240, 240)));
    
    // Override the scene drawing for our custom rendering
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
}

SVGView::~SVGView()
{
    if (m_renderer) {
        delete m_renderer;
    }
}

void SVGView::setSvgFile(const QString &filePath)
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
    
    // Get SVG bounds and set scene rect
    m_svgBounds = QRectF(QPointF(0, 0), m_renderer->defaultSize());
    m_scene->setSceneRect(m_svgBounds);
    
    // Set a reasonable base size for rasterization
    m_baseSize = m_renderer->defaultSize();
    if (m_baseSize.width() > 1000 || m_baseSize.height() > 1000) {
        // Scale down very large SVGs
        double factor = 1000.0 / qMax(m_baseSize.width(), m_baseSize.height());
        m_baseSize = QSize(m_baseSize.width() * factor, m_baseSize.height() * factor);
    }
    
    // Reset zoom and create initial rasterized image
    resetZoom();
}

void SVGView::updateRasterizedImage()
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

void SVGView::paintEvent(QPaintEvent *event)
{
    if (!m_renderer) {
        // Fall back to default rendering if no SVG is loaded
        QGraphicsView::paintEvent(event);
        return;
    }
    
    QPainter painter(viewport());
    
    // Clear background
    painter.fillRect(rect(), backgroundBrush());
    
    // Set the world transformation based on current view
    painter.setWorldTransform(viewportTransform());
    
    if (m_useRasterization && m_zoomFactor >= RASTERIZATION_THRESHOLD) {
        // For high zoom levels, use the rasterized approach
        drawRasterizedImage(&painter);
    } else {
        // For lower zoom levels, render the SVG directly
        drawSvgDirectly(&painter);
    }
    
    painter.end();
}

void SVGView::drawSvgDirectly(QPainter *painter)
{
    // Direct SVG rendering
    m_renderer->render(painter, m_svgBounds);
}

void SVGView::drawRasterizedImage(QPainter *painter)
{
    // Check if we need to update the rasterized image
    if (m_rasterizedImage.isNull() || 
        qAbs(m_rasterizedZoomFactor - m_zoomFactor) > 0.1) {
        // Create or update the rasterized image
        updateRasterizedImage();
    }
    
    // Draw the rasterized image
    painter->drawPixmap(m_svgBounds, m_rasterizedImage, QRectF(m_rasterizedImage.rect()));
}

void SVGView::wheelEvent(QWheelEvent *event)
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

void SVGView::setZoom(int zoomLevel)
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

void SVGView::resetZoom()
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

void SVGView::resizeEvent(QResizeEvent *event)
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


void SVGView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton || event->button() == Qt::LeftButton) {
        m_lastMousePos = event->pos();
        m_isPanning = true;
        setCursor(Qt::ClosedHandCursor);
        event->accept();
    } else {
        QGraphicsView::mousePressEvent(event);
    }
}

void SVGView::mouseMoveEvent(QMouseEvent *event)
{
    if (m_isPanning) {
        QPoint delta = event->pos() - m_lastMousePos;
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
        m_lastMousePos = event->pos();
        event->accept();
    } else {
        QGraphicsView::mouseMoveEvent(event);
    }
}

void SVGView::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton || event->button() == Qt::LeftButton) {
        m_isPanning = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
    } else {
        QGraphicsView::mouseReleaseEvent(event);
    }
}

// SVGViewer Implementation
SVGViewer::SVGViewer(QWidget *parent)
    : QWidget(parent)
{
    // Create main layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    // Set up the toolbar
    setupToolbar();
    
    // Create the SVG view
    m_svgView = new SVGView(this);
    mainLayout->addWidget(m_svgView);
    
    // Connect the zoom changed signal from SVGView
    connect(m_svgView, &SVGView::zoomChanged, this, &SVGViewer::onSvgViewZoomChanged);
    
    // Set up the status bar
    setupStatusBar();
    
    // Disable the convert button initially
    m_convertButton->setEnabled(false);
    m_fitButton->setEnabled(false);
}

void SVGViewer::onSvgViewZoomChanged(double zoomFactor)
{
    // Update slider without triggering signals
    m_zoomSlider->blockSignals(true);
    m_zoomSlider->setValue(qRound(zoomFactor));
    m_zoomSlider->blockSignals(false);
    
    // Update zoom label
    updateZoomLabel(qRound(zoomFactor));
}

void SVGViewer::setupToolbar()
{
    QToolBar *toolbar = new QToolBar(this);
    toolbar->setMovable(false);
    toolbar->setFloatable(false);
    
    // Open SVG button
    m_openButton = new QPushButton(tr("Open SVG"));
    toolbar->addWidget(m_openButton);
    
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
    
    m_convertButton = new QPushButton(tr("Convert to GCode"));
    m_convertButton->setStyleSheet("background-color: #4CAF50; color: white;");
    toolbar->addWidget(m_convertButton);
    
    // Add the toolbar to the layout
    static_cast<QVBoxLayout*>(layout())->addWidget(toolbar);
    
    // Connect signals
    connect(m_openButton, &QPushButton::clicked, this, &SVGViewer::onOpenSvgClicked);
    connect(m_zoomSlider, &QSlider::valueChanged, this, &SVGViewer::onZoomChanged);
    connect(m_fitButton, &QPushButton::clicked, this, &SVGViewer::onFitToViewClicked);
    connect(m_convertButton, &QPushButton::clicked, this, &SVGViewer::onConvertClicked);
}

void SVGViewer::setupStatusBar()
{
    QHBoxLayout *statusLayout = new QHBoxLayout();
    statusLayout->setContentsMargins(5, 2, 5, 2);
    
    // File name display
    m_fileNameLabel = new QLabel(tr("No file loaded"));
    statusLayout->addWidget(m_fileNameLabel);
    
    // Add spacer to push items to left and right edges
    statusLayout->addStretch();
    
    // Add the status bar to the layout
    static_cast<QVBoxLayout*>(layout())->addLayout(statusLayout);
}

void SVGViewer::loadSvgFile(const QString &filePath)
{
    m_svgView->setSvgFile(filePath);
    
    if (m_svgView->isSvgLoaded()) {
        // Update UI
        QFileInfo fileInfo(filePath);
        m_fileNameLabel->setText(fileInfo.fileName());
        m_convertButton->setEnabled(true);
        m_fitButton->setEnabled(true); // Enable the fit button
        
        // Reset zoom slider to 100%
        m_zoomSlider->setValue(100);
        
        // Emit signal that SVG was loaded
        emit svgLoaded(filePath);
    }
}

bool SVGViewer::isSvgLoaded() const
{
    return m_svgView->isSvgLoaded();
}

QString SVGViewer::currentSvgFile() const
{
    return m_svgView->svgFilePath();
}

void SVGViewer::onZoomChanged(int value)
{
    m_svgView->setZoom(value);
    updateZoomLabel(value);
}

void SVGViewer::updateZoomLabel(int value)
{
    m_zoomLabel->setText(QString("%1%").arg(value));
}

void SVGViewer::onOpenSvgClicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("Open SVG File"),
                                                   QString(),
                                                   tr("SVG Files (*.svg)"));
    if (!filePath.isEmpty()) {
        loadSvgFile(filePath);
    }
}

void SVGViewer::onConvertClicked()
{
    if (!isSvgLoaded()) {
        return;
    }
    
    // Emit signal to request conversion
    emit convertToGCode(currentSvgFile());
}

void SVGViewer::onFitToViewClicked()
{
    // Only proceed if an SVG is loaded
    if (!m_svgView->isSvgLoaded()) {
        return;
    }
    
    m_svgView->resetZoom();
    
    // Update zoom slider to match the actual zoom level
    double zoomPercent = m_svgView->transform().m11() * 100;
    
    // Block signals to prevent feedback loop
    m_zoomSlider->blockSignals(true);
    m_zoomSlider->setValue(qRound(zoomPercent));
    m_zoomSlider->blockSignals(false);
    
    updateZoomLabel(qRound(zoomPercent));
}