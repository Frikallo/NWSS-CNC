// svgviewer.cpp
#include "svgviewer.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QSvgRenderer>
#include <QMouseEvent>
#include <QScrollBar>
#include <QFileInfo>
#include <QGraphicsDropShadowEffect>

// SVGView Implementation
SVGView::SVGView(QWidget *parent)
    : QGraphicsView(parent),
      m_svgItem(nullptr),
      m_renderer(nullptr),
      m_zoomFactor(1.0),
      m_isPanning(false)
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
    setInteractive(true);
    
    // Visual style
    setFrameShape(QFrame::NoFrame);
    setBackgroundBrush(QBrush(QColor(240, 240, 240)));
    
    // Default scale
    resetZoom();
}

void SVGView::setSvgFile(const QString &filePath)
{
    // Clear existing items
    m_scene->clear();
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
        m_svgItem = nullptr;
        QMessageBox::warning(this, tr("SVG Load Error"), 
                            tr("Failed to load SVG file: %1").arg(filePath));
        return;
    }
    
    // Create the SVG item
    m_svgItem = new QGraphicsSvgItem();
    m_svgItem->setSharedRenderer(m_renderer);
    m_scene->addItem(m_svgItem);
    
    // Set the scene rect to the SVG bounds
    QRectF bounds = m_svgItem->boundingRect();
    m_scene->setSceneRect(bounds);
    
    // Add a subtle drop shadow
    QGraphicsDropShadowEffect *shadowEffect = new QGraphicsDropShadowEffect();
    shadowEffect->setBlurRadius(20.0);
    shadowEffect->setColor(QColor(0, 0, 0, 50));
    shadowEffect->setOffset(5.0, 5.0);
    m_svgItem->setGraphicsEffect(shadowEffect);
    
    // Reset zoom to show the entire SVG
    resetZoom();
}

void SVGView::zoomIn()
{
    m_zoomFactor *= 1.2;
    setZoom(m_zoomFactor * 100);
}

void SVGView::zoomOut()
{
    m_zoomFactor /= 1.2;
    setZoom(m_zoomFactor * 100);
}

void SVGView::resetZoom()
{
    // Reset the transformation
    m_zoomFactor = 1.0;
    resetTransform();
    
    // Automatically fit the SVG in view if it exists
    if (m_svgItem) {
        QRectF bounds = m_svgItem->boundingRect();
        fitInView(bounds, Qt::KeepAspectRatio);
        
        // Calculate the actual zoom factor after fitting
        QTransform currentTransform = transform();
        m_zoomFactor = currentTransform.m11();
    }
}

void SVGView::setZoom(int zoomLevel)
{
    if (!m_svgItem) return;
    
    // Calculate the new zoom factor (zoomLevel is a percentage)
    double newZoomFactor = zoomLevel / 100.0;
    
    // Reset the transformation
    resetTransform();
    
    // Apply the new zoom
    scale(newZoomFactor, newZoomFactor);
    m_zoomFactor = newZoomFactor;
}

void SVGView::wheelEvent(QWheelEvent *event)
{
    // Only zoom if an SVG is loaded
    if (!m_svgItem) {
        QGraphicsView::wheelEvent(event);
        return;
    }
    
    // Calculate the zoom factor based on the scroll amount
    double delta = event->angleDelta().y() / 120.0;
    double factor = std::pow(1.2, delta);
    
    // Apply the zoom
    m_zoomFactor *= factor;
    scale(factor, factor);
    
    // Update the UI (signal could be added here if needed)
    event->accept();
}

void SVGView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton) {
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
    if (event->button() == Qt::MiddleButton) {
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
    
    // Set up the status bar
    setupStatusBar();
    
    // Disable the convert button initially
    m_convertButton->setEnabled(false);
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
    
    // Conversion controls
    toolbar->addWidget(new QLabel(tr("Conversion Mode:")));
    
    m_conversionModeComboBox = new QComboBox();
    m_conversionModeComboBox->addItems({
        tr("Outline (2D)"),
        tr("Pocket (2D)"),
        tr("Engrave (2D)"),
        tr("V-Carve (3D)")
    });
    toolbar->addWidget(m_conversionModeComboBox);
    
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
    m_svgView->resetZoom();
    
    // Update zoom slider to match the actual zoom level
    double zoomPercent = m_svgView->renderer()->defaultSize().width() > 0 
        ? m_svgView->transform().m11() * 100
        : 100;
    
    // Block signals to prevent feedback loop
    m_zoomSlider->blockSignals(true);
    m_zoomSlider->setValue(qRound(zoomPercent));
    m_zoomSlider->blockSignals(false);
    
    updateZoomLabel(qRound(zoomPercent));
}