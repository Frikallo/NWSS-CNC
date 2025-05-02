#include "gcodeviewer3d.h"
#include <QRegularExpression>
#include <QDebug>
#include <cmath>

GCodeViewer3D::GCodeViewer3D(QWidget *parent)
    : QOpenGLWidget(parent),
      gridVbo(QOpenGLBuffer::VertexBuffer),
      pathVbo(QOpenGLBuffer::VertexBuffer),
      cameraPosition(0.0f, 0.0f, 0.0f),
      cameraTarget(0.0f, 0.0f, 0.0f),  // Always keep target at origin by default
      scale(0.5f),
      minBounds(-50.0f, -50.0f, -50.0f),
      maxBounds(50.0f, 50.0f, 50.0f),
      pathNeedsUpdate(false),
      autoScaleEnabled(false),
      hasValidToolPath(false),
      m_hoveredFaceId(-1),
      m_cubeViewVisible(true),
      m_isDraggingCube(false),
      m_isAnimating(false),
      m_animationProgress(0.0f),
      m_animationDuration(0.5f), // 500ms animation
      m_rotationCenter(0.0f, 0.0f, 0.0f) // Set rotation center to origin
{
    setFocusPolicy(Qt::StrongFocus);
    
    // Initialize with identity quaternions
    m_cubeOrientation = QQuaternion();
    m_viewOrientation = QQuaternion();
    
    // Set up a timer to handle batched updates
    updateTimer = new QTimer(this);
    updateTimer->setSingleShot(true);
    connect(updateTimer, &QTimer::timeout, [this]() {
        if (pathNeedsUpdate) {
            makePathVertices();
            if (autoScaleEnabled && hasValidToolPath) {
                autoScaleToFit();
            }
            update();
            pathNeedsUpdate = false;
        }
    });
    
    // Set up animation timer for view transitions
    m_animationTimer = new QTimer(this);
    connect(m_animationTimer, &QTimer::timeout, this, &GCodeViewer3D::updateAnimation);
}

GCodeViewer3D::~GCodeViewer3D()
{
    // Clean up OpenGL resources
    makeCurrent();
    gridVbo.destroy();
    pathVbo.destroy();
    gridVao.destroy();
    pathVao.destroy();
    doneCurrent();
}

void GCodeViewer3D::initializeGL()
{
    initializeOpenGLFunctions();
    
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    
    setupGridShaders();
    setupPathShaders();
    
    // Initialize buffers
    gridVao.create();
    gridVao.bind();
    gridVbo.create();
    gridVbo.bind();
    gridVbo.allocate(nullptr, 4096 * sizeof(float)); // Allocate more space for grid lines
    gridVao.release();
    
    pathVao.create();
    pathVao.bind();
    pathVbo.create();
    pathVbo.bind();
    pathVbo.allocate(nullptr, 1024 * sizeof(float)); // Initial allocation, will resize as needed
    pathVao.release();
    
    // Initialize matrices
    model.setToIdentity();
    
    // Set default camera position for isometric view
    setIsometricView();
    
    // Empty tool path for initial display
    toolPath.clear();
    hasValidToolPath = false;
    
    // Fixed scale for empty view
    scale = 1.0f;
    
    // Initialize view cube
    initViewCube();
    
    makePathVertices();
}

void GCodeViewer3D::initViewCube() {
    // Create a more complex navigation cube with isometric view options
    // This creates a truncated cube with 8 triangular faces and 18 square faces
    m_cubeViewFaces.clear();
    
    // Define truncation factor (how much to cut off corners)
    float t = 0.35f; // Truncation factor
    float s = 1.0f - t; // Scaled coordinate for truncated vertices
    
    // Define cube size and position (in screen coordinates)
    m_cubeSize = 85.0f; // pixels - slightly larger to accommodate more faces
    int margin = 10;
    m_cubePosition = QPointF(width() - m_cubeSize - margin, margin);
    
    // Define 24 vertices of the truncated cube
    std::vector<QVector3D> vertices = {
        // Front face vertices (truncated)
        QVector3D(-s, -1, -s), QVector3D(s, -1, -s),  // 0, 1: bottom edge
        QVector3D(s, -1, s),   QVector3D(-s, -1, s),  // 2, 3: top edge
        
        // Back face vertices (truncated)
        QVector3D(-s, 1, -s),  QVector3D(s, 1, -s),   // 4, 5: bottom edge
        QVector3D(s, 1, s),    QVector3D(-s, 1, s),   // 6, 7: top edge
        
        // Connecting left face vertices
        QVector3D(-1, -s, -s), QVector3D(-1, s, -s),  // 8, 9: bottom edge
        QVector3D(-1, s, s),   QVector3D(-1, -s, s),  // 10, 11: top edge
        
        // Connecting right face vertices
        QVector3D(1, -s, -s),  QVector3D(1, s, -s),   // 12, 13: bottom edge
        QVector3D(1, s, s),    QVector3D(1, -s, s),   // 14, 15: top edge
        
        // Connecting top face vertices
        QVector3D(-s, -s, 1),  QVector3D(s, -s, 1),   // 16, 17: front edge
        QVector3D(s, s, 1),    QVector3D(-s, s, 1),   // 18, 19: back edge
        
        // Connecting bottom face vertices
        QVector3D(-s, -s, -1), QVector3D(s, -s, -1),  // 20, 21: front edge
        QVector3D(s, s, -1),   QVector3D(-s, s, -1)   // 22, 23: back edge
    };
    
    // Define the 6 main square faces (truncated original cube faces)
    
    // Front face (Z-)
    CubeFace frontFace;
    frontFace.id = 0;
    frontFace.normal = QVector3D(0, -1, 0);
    frontFace.vertices = {vertices[0], vertices[1], vertices[2], vertices[3]};
    frontFace.center = QVector3D(0, -1, 0);
    frontFace.color = QColor(200, 200, 200, 200);
    frontFace.hoverColor = QColor(255, 255, 0, 200);
    frontFace.label = "Front";
    frontFace.isHovered = false;
    m_cubeViewFaces.push_back(frontFace);
    
    // Back face (Z+)
    CubeFace backFace;
    backFace.id = 1;
    backFace.normal = QVector3D(0, 1, 0);
    backFace.vertices = {vertices[4], vertices[5], vertices[6], vertices[7]};
    backFace.center = QVector3D(0, 1, 0);
    backFace.color = QColor(180, 180, 180, 200);
    backFace.hoverColor = QColor(255, 255, 0, 200);
    backFace.label = "Back";
    backFace.isHovered = false;
    m_cubeViewFaces.push_back(backFace);
    
    // Left face (X-)
    CubeFace leftFace;
    leftFace.id = 2;
    leftFace.normal = QVector3D(-1, 0, 0);
    leftFace.vertices = {vertices[8], vertices[9], vertices[10], vertices[11]};
    leftFace.center = QVector3D(-1, 0, 0);
    leftFace.color = QColor(160, 160, 160, 200);
    leftFace.hoverColor = QColor(255, 255, 0, 200);
    leftFace.label = "Left";
    leftFace.isHovered = false;
    m_cubeViewFaces.push_back(leftFace);
    
    // Right face (X+)
    CubeFace rightFace;
    rightFace.id = 3;
    rightFace.normal = QVector3D(1, 0, 0);
    rightFace.vertices = {vertices[12], vertices[13], vertices[14], vertices[15]};
    rightFace.center = QVector3D(1, 0, 0);
    rightFace.color = QColor(160, 160, 160, 200);
    rightFace.hoverColor = QColor(255, 255, 0, 200);
    rightFace.label = "Right";
    rightFace.isHovered = false;
    m_cubeViewFaces.push_back(rightFace);
    
    // Top face (Y+)
    CubeFace topFace;
    topFace.id = 4;
    topFace.normal = QVector3D(0, 0, 1);
    topFace.vertices = {vertices[16], vertices[17], vertices[18], vertices[19]};
    topFace.center = QVector3D(0, 0, 1);
    topFace.color = QColor(140, 140, 140, 200);
    topFace.hoverColor = QColor(255, 255, 0, 200);
    topFace.label = "Top";
    topFace.isHovered = false;
    m_cubeViewFaces.push_back(topFace);
    
    // Bottom face (Y-)
    CubeFace bottomFace;
    bottomFace.id = 5;
    bottomFace.normal = QVector3D(0, 0, -1);
    bottomFace.vertices = {vertices[20], vertices[21], vertices[22], vertices[23]};
    bottomFace.center = QVector3D(0, 0, -1);
    bottomFace.color = QColor(140, 140, 140, 200);
    bottomFace.hoverColor = QColor(255, 255, 0, 200);
    bottomFace.label = "Bottom";
    bottomFace.isHovered = false;
    m_cubeViewFaces.push_back(bottomFace);
    
    // Define the 12 additional square faces (edge truncations)
    
    // Front-Left edge face
    CubeFace frontLeftFace;
    frontLeftFace.id = 6;
    frontLeftFace.normal = QVector3D(-0.7071f, -0.7071f, 0);
    frontLeftFace.vertices = {vertices[0], vertices[3], vertices[11], vertices[8]};
    frontLeftFace.center = QVector3D(-0.7071f, -0.7071f, 0);
    frontLeftFace.color = QColor(170, 170, 170, 200);
    frontLeftFace.hoverColor = QColor(255, 255, 0, 200);
    frontLeftFace.label = "";
    frontLeftFace.isHovered = false;
    m_cubeViewFaces.push_back(frontLeftFace);
    
    // Front-Right edge face
    CubeFace frontRightFace;
    frontRightFace.id = 7;
    frontRightFace.normal = QVector3D(0.7071f, -0.7071f, 0);
    frontRightFace.vertices = {vertices[1], vertices[12], vertices[15], vertices[2]};
    frontRightFace.center = QVector3D(0.7071f, -0.7071f, 0);
    frontRightFace.color = QColor(170, 170, 170, 200);
    frontRightFace.hoverColor = QColor(255, 255, 0, 200);
    frontRightFace.label = "";
    frontRightFace.isHovered = false;
    m_cubeViewFaces.push_back(frontRightFace);
    
    // Front-Top edge face
    CubeFace frontTopFace;
    frontTopFace.id = 8;
    frontTopFace.normal = QVector3D(0, -0.7071f, 0.7071f);
    frontTopFace.vertices = {vertices[3], vertices[2], vertices[17], vertices[16]};
    frontTopFace.center = QVector3D(0, -0.7071f, 0.7071f);
    frontTopFace.color = QColor(170, 170, 170, 200);
    frontTopFace.hoverColor = QColor(255, 255, 0, 200);
    frontTopFace.label = "";
    frontTopFace.isHovered = false;
    m_cubeViewFaces.push_back(frontTopFace);
    
    // Front-Bottom edge face
    CubeFace frontBottomFace;
    frontBottomFace.id = 9;
    frontBottomFace.normal = QVector3D(0, -0.7071f, -0.7071f);
    frontBottomFace.vertices = {vertices[0], vertices[1], vertices[21], vertices[20]};
    frontBottomFace.center = QVector3D(0, -0.7071f, -0.7071f);
    frontBottomFace.color = QColor(170, 170, 170, 200);
    frontBottomFace.hoverColor = QColor(255, 255, 0, 200);
    frontBottomFace.label = "";
    frontBottomFace.isHovered = false;
    m_cubeViewFaces.push_back(frontBottomFace);
    
    // Back-Left edge face
    CubeFace backLeftFace;
    backLeftFace.id = 10;
    backLeftFace.normal = QVector3D(-0.7071f, 0.7071f, 0);
    backLeftFace.vertices = {vertices[4], vertices[9], vertices[10], vertices[7]};
    backLeftFace.center = QVector3D(-0.7071f, 0.7071f, 0);
    backLeftFace.color = QColor(170, 170, 170, 200);
    backLeftFace.hoverColor = QColor(255, 255, 0, 200);
    backLeftFace.label = "";
    backLeftFace.isHovered = false;
    m_cubeViewFaces.push_back(backLeftFace);
    
    // Back-Right edge face
    CubeFace backRightFace;
    backRightFace.id = 11;
    backRightFace.normal = QVector3D(0.7071f, 0.7071f, 0);
    backRightFace.vertices = {vertices[5], vertices[6], vertices[14], vertices[13]};
    backRightFace.center = QVector3D(0.7071f, 0.7071f, 0);
    backRightFace.color = QColor(170, 170, 170, 200);
    backRightFace.hoverColor = QColor(255, 255, 0, 200);
    backRightFace.label = "";
    backRightFace.isHovered = false;
    m_cubeViewFaces.push_back(backRightFace);
    
    // Back-Top edge face
    CubeFace backTopFace;
    backTopFace.id = 12;
    backTopFace.normal = QVector3D(0, 0.7071f, 0.7071f);
    backTopFace.vertices = {vertices[7], vertices[6], vertices[18], vertices[19]};
    backTopFace.center = QVector3D(0, 0.7071f, 0.7071f);
    backTopFace.color = QColor(170, 170, 170, 200);
    backTopFace.hoverColor = QColor(255, 255, 0, 200);
    backTopFace.label = "";
    backTopFace.isHovered = false;
    m_cubeViewFaces.push_back(backTopFace);
    
    // Back-Bottom edge face
    CubeFace backBottomFace;
    backBottomFace.id = 13;
    backBottomFace.normal = QVector3D(0, 0.7071f, -0.7071f);
    backBottomFace.vertices = {vertices[4], vertices[5], vertices[22], vertices[23]};
    backBottomFace.center = QVector3D(0, 0.7071f, -0.7071f);
    backBottomFace.color = QColor(170, 170, 170, 200);
    backBottomFace.hoverColor = QColor(255, 255, 0, 200);
    backBottomFace.label = "";
    backBottomFace.isHovered = false;
    m_cubeViewFaces.push_back(backBottomFace);
    
    // Left-Top edge face
    CubeFace leftTopFace;
    leftTopFace.id = 14;
    leftTopFace.normal = QVector3D(-0.7071f, 0, 0.7071f);
    leftTopFace.vertices = {vertices[11], vertices[10], vertices[19], vertices[16]};
    leftTopFace.center = QVector3D(-0.7071f, 0, 0.7071f);
    leftTopFace.color = QColor(170, 170, 170, 200);
    leftTopFace.hoverColor = QColor(255, 255, 0, 200);
    leftTopFace.label = "";
    leftTopFace.isHovered = false;
    m_cubeViewFaces.push_back(leftTopFace);
    
    // Left-Bottom edge face
    CubeFace leftBottomFace;
    leftBottomFace.id = 15;
    leftBottomFace.normal = QVector3D(-0.7071f, 0, -0.7071f);
    leftBottomFace.vertices = {vertices[8], vertices[9], vertices[23], vertices[20]};
    leftBottomFace.center = QVector3D(-0.7071f, 0, -0.7071f);
    leftBottomFace.color = QColor(170, 170, 170, 200);
    leftBottomFace.hoverColor = QColor(255, 255, 0, 200);
    leftBottomFace.label = "";
    leftBottomFace.isHovered = false;
    m_cubeViewFaces.push_back(leftBottomFace);
    
    // Right-Top edge face
    CubeFace rightTopFace;
    rightTopFace.id = 16;
    rightTopFace.normal = QVector3D(0.7071f, 0, 0.7071f);
    rightTopFace.vertices = {vertices[15], vertices[14], vertices[18], vertices[17]};
    rightTopFace.center = QVector3D(0.7071f, 0, 0.7071f);
    rightTopFace.color = QColor(170, 170, 170, 200);
    rightTopFace.hoverColor = QColor(255, 255, 0, 200);
    rightTopFace.label = "";
    rightTopFace.isHovered = false;
    m_cubeViewFaces.push_back(rightTopFace);
    
    // Right-Bottom edge face
    CubeFace rightBottomFace;
    rightBottomFace.id = 17;
    rightBottomFace.normal = QVector3D(0.7071f, 0, -0.7071f);
    rightBottomFace.vertices = {vertices[12], vertices[13], vertices[22], vertices[21]};
    rightBottomFace.center = QVector3D(0.7071f, 0, -0.7071f);
    rightBottomFace.color = QColor(170, 170, 170, 200);
    rightBottomFace.hoverColor = QColor(255, 255, 0, 200);
    rightBottomFace.label = "";
    rightBottomFace.isHovered = false;
    m_cubeViewFaces.push_back(rightBottomFace);
    
    // Define the 8 triangular faces at the corners
    
    // Front-Left-Bottom corner
    CubeFace frontLeftBottomFace;
    frontLeftBottomFace.id = 18;
    frontLeftBottomFace.normal = QVector3D(-0.577f, -0.577f, -0.577f);
    frontLeftBottomFace.vertices = {vertices[0], vertices[8], vertices[20]};
    frontLeftBottomFace.center = QVector3D(-0.577f, -0.577f, -0.577f);
    frontLeftBottomFace.color = QColor(220, 180, 180, 200);
    frontLeftBottomFace.hoverColor = QColor(255, 255, 0, 200);
    frontLeftBottomFace.label = "ISO1";
    frontLeftBottomFace.isHovered = false;
    m_cubeViewFaces.push_back(frontLeftBottomFace);
    
    // Front-Right-Bottom corner
    CubeFace frontRightBottomFace;
    frontRightBottomFace.id = 19;
    frontRightBottomFace.normal = QVector3D(0.577f, -0.577f, -0.577f);
    frontRightBottomFace.vertices = {vertices[1], vertices[21], vertices[12]};
    frontRightBottomFace.center = QVector3D(0.577f, -0.577f, -0.577f);
    frontRightBottomFace.color = QColor(220, 180, 180, 200);
    frontRightBottomFace.hoverColor = QColor(255, 255, 0, 200);
    frontRightBottomFace.label = "ISO2";
    frontRightBottomFace.isHovered = false;
    m_cubeViewFaces.push_back(frontRightBottomFace);
    
    // Front-Left-Top corner
    CubeFace frontLeftTopFace;
    frontLeftTopFace.id = 20;
    frontLeftTopFace.normal = QVector3D(-0.577f, -0.577f, 0.577f);
    frontLeftTopFace.vertices = {vertices[3], vertices[16], vertices[11]};
    frontLeftTopFace.center = QVector3D(-0.577f, -0.577f, 0.577f);
    frontLeftTopFace.color = QColor(220, 180, 180, 200);
    frontLeftTopFace.hoverColor = QColor(255, 255, 0, 200);
    frontLeftTopFace.label = "ISO3";
    frontLeftTopFace.isHovered = false;
    m_cubeViewFaces.push_back(frontLeftTopFace);
    
    // Front-Right-Top corner
    CubeFace frontRightTopFace;
    frontRightTopFace.id = 21;
    frontRightTopFace.normal = QVector3D(0.577f, -0.577f, 0.577f);
    frontRightTopFace.vertices = {vertices[2], vertices[15], vertices[17]};
    frontRightTopFace.center = QVector3D(0.577f, -0.577f, 0.577f);
    frontRightTopFace.color = QColor(220, 180, 180, 200);
    frontRightTopFace.hoverColor = QColor(255, 255, 0, 200);
    frontRightTopFace.label = "ISO4";
    frontRightTopFace.isHovered = false;
    m_cubeViewFaces.push_back(frontRightTopFace);
    
    // Back-Left-Bottom corner
    CubeFace backLeftBottomFace;
    backLeftBottomFace.id = 22;
    backLeftBottomFace.normal = QVector3D(-0.577f, 0.577f, -0.577f);
    backLeftBottomFace.vertices = {vertices[4], vertices[23], vertices[9]};
    backLeftBottomFace.center = QVector3D(-0.577f, 0.577f, -0.577f);
    backLeftBottomFace.color = QColor(180, 220, 180, 200);
    backLeftBottomFace.hoverColor = QColor(255, 255, 0, 200);
    backLeftBottomFace.label = "ISO5";
    backLeftBottomFace.isHovered = false;
    m_cubeViewFaces.push_back(backLeftBottomFace);
    
    // Back-Right-Bottom corner
    CubeFace backRightBottomFace;
    backRightBottomFace.id = 23;
    backRightBottomFace.normal = QVector3D(0.577f, 0.577f, -0.577f);
    backRightBottomFace.vertices = {vertices[5], vertices[13], vertices[22]};
    backRightBottomFace.center = QVector3D(0.577f, 0.577f, -0.577f);
    backRightBottomFace.color = QColor(180, 220, 180, 200);
    backRightBottomFace.hoverColor = QColor(255, 255, 0, 200);
    backRightBottomFace.label = "ISO6";
    backRightBottomFace.isHovered = false;
    m_cubeViewFaces.push_back(backRightBottomFace);
    
    // Back-Left-Top corner
    CubeFace backLeftTopFace;
    backLeftTopFace.id = 24;
    backLeftTopFace.normal = QVector3D(-0.577f, 0.577f, 0.577f);
    backLeftTopFace.vertices = {vertices[7], vertices[10], vertices[19]};
    backLeftTopFace.center = QVector3D(-0.577f, 0.577f, 0.577f);
    backLeftTopFace.color = QColor(180, 220, 180, 200);
    backLeftTopFace.hoverColor = QColor(255, 255, 0, 200);
    backLeftTopFace.label = "ISO7";
    backLeftTopFace.isHovered = false;
    m_cubeViewFaces.push_back(backLeftTopFace);
    
    // Back-Right-Top corner
    CubeFace backRightTopFace;
    backRightTopFace.id = 25;
    backRightTopFace.normal = QVector3D(0.577f, 0.577f, 0.577f);
    backRightTopFace.vertices = {vertices[6], vertices[18], vertices[14]};
    backRightTopFace.center = QVector3D(0.577f, 0.577f, 0.577f);
    backRightTopFace.color = QColor(180, 220, 180, 200);
    backRightTopFace.hoverColor = QColor(255, 255, 0, 200);
    backRightTopFace.label = "ISO8";
    backRightTopFace.isHovered = false;
    m_cubeViewFaces.push_back(backRightTopFace);
}

void GCodeViewer3D::drawViewCube(QPainter* painter) {
    if (!m_cubeViewVisible) return;
    
    // Save painter state
    painter->save();
    
    // Translate to cube position in screen space
    painter->translate(m_cubePosition.x() + m_cubeSize/2, m_cubePosition.y() + m_cubeSize/2);
    
    // Calculate orientation that matches the main view
    // This ensures the cube stays oriented the same as the axes
    QMatrix4x4 viewMatrix = view;
    QMatrix4x4 cubeMatrix;
    cubeMatrix.setToIdentity();
    
    // Extract just the rotation from the view matrix
    QMatrix3x3 rotationOnly;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            rotationOnly(i, j) = viewMatrix(i, j);
        }
    }
    
    // Apply inverse rotation so cube follows the view
    cubeMatrix.setToIdentity();
    cubeMatrix.rotate(m_cubeOrientation);
    
    // Draw a subtle semi-transparent background sphere for the cube
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(100, 100, 100, 80));
    painter->drawEllipse(QPointF(0, 0), m_cubeSize/2 * 1.1, m_cubeSize/2 * 1.1);
    
    // Sort faces by depth (z-value) for proper rendering order
    std::vector<int> faceOrder;
    for (int i = 0; i < m_cubeViewFaces.size(); i++) {
        faceOrder.push_back(i);
    }
    
    std::sort(faceOrder.begin(), faceOrder.end(), [this, &cubeMatrix](int a, int b) {
        // Calculate center z-value of each face for depth sorting
        QVector3D centerA = cubeMatrix.map(m_cubeViewFaces[a].center);
        QVector3D centerB = cubeMatrix.map(m_cubeViewFaces[b].center);
        return centerA.z() < centerB.z(); // Sort from back to front
    });
    
    // Draw each face in the sorted order
    for (int idx : faceOrder) {
        const CubeFace& face = m_cubeViewFaces[idx];
        
        // Create a polygon for the face
        QPolygonF polygon;
        for (const QVector3D& v : face.vertices) {
            // Apply rotation
            QVector3D rotated = cubeMatrix.map(v);
            polygon << QPointF(rotated.x() * m_cubeSize/2, rotated.y() * m_cubeSize/2);
        }
        
        // Determine if face is oriented toward camera (for label visibility)
        QVector3D normalRotated = cubeMatrix.map(face.normal);
        bool isFaceVisible = normalRotated.z() < -0.05; // Only visible if somewhat facing camera
        
        // Draw face with appropriate color
        QColor color;
        color = face.color;
        painter->setPen(QPen(QColor(0, 0, 0, 100), 1));
        
        // Make more prominent if this is a triangular isometric face
        if (idx >= 18) { // Isometric faces are ID 18-25
            if (!face.isHovered) {
                // Make non-hovered isometric faces slightly more vibrant
                color.setAlpha(230);
            }
        }
        
        painter->setBrush(QBrush(color));
        painter->drawPolygon(polygon);
        
        // Draw label in the center of the face (only if facing forward enough)
        if (isFaceVisible && !face.label.isEmpty()) {
            QVector3D labelPos = cubeMatrix.map(face.center);
            QFont font = painter->font();
            // Make isometric labels more visible
            if (idx >= 18) {
                font.setBold(true);
                font.setPointSize(7);
            } else {
                font.setPointSize(8);
            }
            painter->setFont(font);
            painter->setPen(face.isHovered ? Qt::black : Qt::black);
            
            // Calculate text width for centering
            QFontMetrics fm(font);
            int textWidth = fm.horizontalAdvance(face.label);
            
            painter->drawText(
                QPointF(labelPos.x() * m_cubeSize/2 - textWidth/2,
                        labelPos.y() * m_cubeSize/2 + 4),
                face.label
            );
        }
    }
    
    // Draw axes indicators
    // painter->setPen(QPen(Qt::red, 2));
    // QVector3D xAxis = cubeMatrix.map(QVector3D(1.5, 0, 0));
    // painter->drawLine(QPointF(0, 0), QPointF(xAxis.x() * m_cubeSize/2, xAxis.y() * m_cubeSize/2));
    
    // painter->setPen(QPen(Qt::green, 2));
    // QVector3D yAxis = cubeMatrix.map(QVector3D(0, 1.5, 0));
    // painter->drawLine(QPointF(0, 0), QPointF(yAxis.x() * m_cubeSize/2, yAxis.y() * m_cubeSize/2));
    
    // painter->setPen(QPen(Qt::blue, 2));
    // QVector3D zAxis = cubeMatrix.map(QVector3D(0, 0, 1.5));
    // painter->drawLine(QPointF(0, 0), QPointF(zAxis.x() * m_cubeSize/2, zAxis.y() * m_cubeSize/2));
    
    // Restore painter state
    painter->restore();
}

bool GCodeViewer3D::isPointInViewCube(const QPoint& point) {
    // Get the center of the cube
    QPointF center(m_cubePosition.x() + m_cubeSize/2, m_cubePosition.y() + m_cubeSize/2);
    
    // Calculate distance from center to point
    QPointF delta = QPointF(point) - center;
    double distance = sqrt(delta.x() * delta.x() + delta.y() * delta.y());
    
    // Slightly larger detection radius for better usability
    return distance <= (m_cubeSize/2 * 1.25);
}

int GCodeViewer3D::getCubeFaceAtPoint(const QPoint& point) {
    if (!isPointInViewCube(point)) return -1;
    
    // Convert to local cube coordinates
    QPointF localPoint(
        point.x() - (m_cubePosition.x() + m_cubeSize/2),
        point.y() - (m_cubePosition.y() + m_cubeSize/2)
    );
    
    // Use the current cube orientation
    QMatrix4x4 cubeMatrix;
    cubeMatrix.setToIdentity();
    cubeMatrix.rotate(m_cubeOrientation);
    
    // Find the visible face at this point
    int closestFace = -1;
    float closestDepth = std::numeric_limits<float>::max();
    
    // Debug vector to collect face info
    QVector<QPair<int, float>> faceDepths;
    
    for (int i = 0; i < m_cubeViewFaces.size(); i++) {
        const CubeFace& face = m_cubeViewFaces[i];
        
        // Create a polygon for the face
        QPolygonF polygon;
        for (const QVector3D& v : face.vertices) {
            // Apply rotation
            QVector3D rotated = cubeMatrix.map(v);
            polygon << QPointF(rotated.x() * m_cubeSize/2, rotated.y() * m_cubeSize/2);
        }
        
        // Check if the point is inside the polygon
        if (polygon.containsPoint(localPoint, Qt::OddEvenFill)) {
            // Get the z-coordinate of face center to determine depth
            QVector3D centerRotated = cubeMatrix.map(face.center);
            
            // Store all matching faces and their depths for debugging
            faceDepths.append(qMakePair(i, centerRotated.z()));
            
            // Update closest face if this one is closer to the camera (smaller z-value)
            if (centerRotated.z() < closestDepth) {
                if (centerRotated.z() < 0) {
                    // If the face is behind the camera, ignore it
                    continue;
                }
                closestDepth = centerRotated.z();
                closestFace = i;
            }
        }
    }
    
    return closestFace;
}

QVector3D GCodeViewer3D::mapToSphere(const QPointF& point) {
    // Convert from view cube's screen position to [-1, 1] range
    QPointF cubeCenter(m_cubePosition.x() + m_cubeSize/2, m_cubePosition.y() + m_cubeSize/2);
    QPointF p = (point - cubeCenter) / (m_cubeSize/2);
    
    // Clamp to boundary of sphere
    double lengthSquared = p.x() * p.x() + p.y() * p.y();
    if (lengthSquared > 1.0) {
        double norm = 1.0 / sqrt(lengthSquared);
        p *= norm;
        lengthSquared = 1.0;
    }
    
    // Convert to 3D point on arcball sphere
    double z = sqrt(1.0 - lengthSquared);
    return QVector3D(p.x(), p.y(), z);
}

void GCodeViewer3D::rotateCubeByMouse(const QPoint& from, const QPoint& to)
{
    // Convert screen points to points on the virtual sphere
    QVector3D v1 = mapToSphere(from);
    QVector3D v2 = mapToSphere(to);
    
    // Compute the axis of rotation (cross product of vectors)
    QVector3D axis = QVector3D::crossProduct(v1, v2);
    if (axis.length() < 0.00001) return; // No rotation needed
    
    // Compute the angle of rotation
    axis.normalize();
    double dot = QVector3D::dotProduct(v1, v2);
    dot = qBound(-1.0, dot, 1.0); // Clamp to avoid domain errors
    
    double angle = acos(dot) * 180.0 / M_PI;
    
    // Create quaternion for this rotation
    QQuaternion newRotation = QQuaternion::fromAxisAndAngle(axis, angle);
    
    // Apply rotation to current orientation - both cube and view orientation
    m_cubeOrientation = newRotation * m_cubeOrientation;
    m_viewOrientation = m_cubeOrientation; // Keep these in sync
    
    // Calculate new camera position based on orientation
    float distance = (cameraPosition - cameraTarget).length();
    
    // Start with a base direction vector (0, 0, distance)
    QVector3D baseDir(0, -distance, 0);
    
    // Create rotation matrix from view orientation
    QMatrix4x4 rotMat;
    rotMat.setToIdentity();
    rotMat.rotate(m_viewOrientation);
    
    // Apply rotation to get new camera position
    QVector3D newDir = rotMat.map(baseDir);
    
    // Update camera position while keeping target fixed at rotation center
    cameraPosition = m_rotationCenter + newDir;
    cameraTarget = m_rotationCenter; // Ensure target stays at rotation center
    
    // Update view matrix
    view.setToIdentity();
    view.lookAt(cameraPosition, cameraTarget, QVector3D(0.0f, 0.0f, 1.0f));
    
    update();
}

void GCodeViewer3D::applyCubeRotation(const QQuaternion& rotation)
{
    // Apply the rotation to BOTH the cube and scene view
    m_viewOrientation = rotation * m_viewOrientation;
    m_cubeOrientation = m_viewOrientation; // Keep both in sync
    
    // Calculate new camera position based on orientation
    float distance = (cameraPosition - cameraTarget).length();
    
    // Start with a base direction vector (0, 0, distance)
    QVector3D baseDir(0, -distance, 0);
    
    // Rotate by the current view orientation
    QMatrix4x4 rotMat;
    rotMat.setToIdentity();
    rotMat.rotate(m_viewOrientation);
    
    QVector3D newDir = rotMat.map(baseDir);
    
    // Update camera position while keeping target fixed at rotation center
    cameraPosition = m_rotationCenter + newDir;
    cameraTarget = m_rotationCenter; // Ensure target stays at the rotation center
    
    // Update view matrix
    view.setToIdentity();
    view.lookAt(cameraPosition, cameraTarget, QVector3D(0.0f, 0.0f, 1.0f));
    
    update();
}

void GCodeViewer3D::setViewDirection(const QVector3D& direction) {
    // Call the animation method instead of instantly changing the view
    animateToViewDirection(direction);
}

void GCodeViewer3D::animateToView(const QVector3D& targetPosition, const QVector3D& targetTarget) {
    // Set up animation parameters
    m_isAnimating = true;
    m_startCameraPosition = cameraPosition;
    m_targetCameraPosition = targetPosition;
    m_startCameraTarget = cameraTarget;
    m_targetCameraTarget = targetTarget;
    m_animationProgress = 0.0f;
    
    // Start the animation timer if not already running
    if (!m_animationTimer->isActive()) {
        m_animationTimer->start(16); // ~60 fps
    }
}

void GCodeViewer3D::updateAnimation() 
{
    if (!m_isAnimating) return;
    
    // Update animation progress
    m_animationProgress += 0.016f / m_animationDuration; // 16ms per frame
    
    if (m_animationProgress >= 1.0f) {
        // Animation complete
        m_animationProgress = 1.0f;
        m_isAnimating = false;
        m_animationTimer->stop();
    }
    
    // Use a smoothstep curve for more natural animation
    float t = m_animationProgress;
    float smoothT = t * t * (3.0f - 2.0f * t);
    
    // Interpolate camera position and target
    cameraPosition = m_startCameraPosition + smoothT * (m_targetCameraPosition - m_startCameraPosition);
    cameraTarget = m_rotationCenter; // Always keep target at rotation center
    
    // Interpolate orientation (slerp)
    m_viewOrientation = QQuaternion::slerp(m_startOrientation, m_targetOrientation, smoothT);
    
    // Keep cube orientation in sync with view orientation
    m_cubeOrientation = m_viewOrientation;
    
    // Update view matrix
    view.setToIdentity();
    view.lookAt(cameraPosition, cameraTarget, QVector3D(0.0f, 0.0f, 1.0f));
    
    // Trigger a redraw
    update();
}

void GCodeViewer3D::setIsometricView()
{
    // Distance from origin
    float distance = 500.0f;
    
    // Create an isometric view
    QVector3D isoDirection(1.0f, -1.0f, 1.0f);
    isoDirection.normalize();
    
    // Position camera for isometric projection with origin as target
    cameraTarget = m_rotationCenter; // Always set to origin or defined rotation center
    cameraPosition = cameraTarget + isoDirection * distance;
    
    // Calculate the corresponding orientation quaternion
    QVector3D forward = (cameraTarget - cameraPosition).normalized();
    QVector3D worldForward(0.0f, -1.0f, 0.0f); // Initial view direction
    
    // Find the axis and angle to rotate from worldForward to forward
    QVector3D axis = QVector3D::crossProduct(worldForward, forward);
    if (axis.length() < 0.001f) {
        // Vectors are parallel or opposite - find another axis
        if (QVector3D::dotProduct(worldForward, forward) < 0) {
            // Opposite directions - rotate 180 degrees around any perpendicular axis
            axis = QVector3D(0, 0, 1);
            m_viewOrientation = QQuaternion::fromAxisAndAngle(axis, 180.0f);
        } else {
            // Same direction - no rotation needed
            m_viewOrientation = QQuaternion();
        }
    } else {
        // Normal case - calculate rotation
        axis.normalize();
        float angle = acos(QVector3D::dotProduct(worldForward, forward)) * 180.0f / M_PI;
        m_viewOrientation = QQuaternion::fromAxisAndAngle(axis, angle);
    }
    
    // Keep cube orientation in sync with view orientation
    m_cubeOrientation = m_viewOrientation;
    
    // Update view matrix
    view.setToIdentity();
    view.lookAt(cameraPosition, cameraTarget, QVector3D(0.0f, 0.0f, 1.0f));
}

void GCodeViewer3D::setupGridShaders()
{
    // Vertex shader for grid
    const char *gridVertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 position;
        layout (location = 1) in vec3 color;
        
        uniform mat4 projection;
        uniform mat4 view;
        uniform mat4 model;
        
        out vec3 vertexColor;
        
        void main()
        {
            gl_Position = projection * view * model * vec4(position, 1.0);
            vertexColor = color;
        }
    )";
    
    // Fragment shader for grid
    const char *gridFragmentShaderSource = R"(
        #version 330 core
        in vec3 vertexColor;
        out vec4 fragColor;
        
        void main()
        {
            fragColor = vec4(vertexColor, 1.0);
        }
    )";
    
    // Compile grid shaders
    if (!gridProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, gridVertexShaderSource)) {
        qDebug() << "Failed to compile grid vertex shader";
    }
    
    if (!gridProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, gridFragmentShaderSource)) {
        qDebug() << "Failed to compile grid fragment shader";
    }
    
    if (!gridProgram.link()) {
        qDebug() << "Failed to link grid shader program";
    }
}

void GCodeViewer3D::setupPathShaders()
{
    // Vertex shader for tool path
    const char *pathVertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 position;
        layout (location = 1) in vec3 color;
        
        uniform mat4 projection;
        uniform mat4 view;
        uniform mat4 model;
        
        out vec3 vertexColor;
        
        void main()
        {
            gl_Position = projection * view * model * vec4(position, 1.0);
            vertexColor = color;
        }
    )";
    
    // Fragment shader for tool path
    const char *pathFragmentShaderSource = R"(
        #version 330 core
        in vec3 vertexColor;
        out vec4 fragColor;
        
        void main()
        {
            fragColor = vec4(vertexColor, 1.0);
        }
    )";
    
    // Compile path shaders
    if (!pathProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, pathVertexShaderSource)) {
        qDebug() << "Failed to compile path vertex shader";
    }
    
    if (!pathProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, pathFragmentShaderSource)) {
        qDebug() << "Failed to compile path fragment shader";
    }
    
    if (!pathProgram.link()) {
        qDebug() << "Failed to link path shader program";
    }
}

void GCodeViewer3D::resizeGL(int width, int height)
{
    float aspectRatio = static_cast<float>(width) / static_cast<float>(height ? height : 1);
    
    // Update projection matrix
    projection.setToIdentity();
    projection.perspective(45.0f, aspectRatio, 0.1f, 2000.0f);
    
    // Re-fit the model to the new view size if appropriate
    if (autoScaleEnabled && hasValidToolPath) {
        autoScaleToFit();
    }
    
    // Update cube position
    int margin = 20;
    m_cubePosition = QPointF(width - m_cubeSize - margin, margin);
}

void GCodeViewer3D::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Use the viewDirection with the rotation center as reference
    QVector3D viewDirection = (cameraPosition - m_rotationCenter).normalized();
    float viewDistance = (cameraPosition - m_rotationCenter).length() / scale;
    
    QVector3D scaledPosition = m_rotationCenter + viewDirection * viewDistance;
    
    // Update view matrix with the correct target
    view.setToIdentity();
    view.lookAt(scaledPosition, m_rotationCenter, QVector3D(0.0f, 0.0f, 1.0f));
    
    // Apply rotation to the model matrix instead
    model.setToIdentity();
    
    drawGrid();
    
    // Only draw tool path if it exists
    if (hasValidToolPath) {
        drawToolPath();
    }
    
    // After all OpenGL rendering, switch to painter for the UI elements
    glDisable(GL_DEPTH_TEST);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    
    // Draw the view cube
    drawViewCube(&painter);
    
    // End painting
    painter.end();
    
    // Re-enable depth testing for subsequent frames
    glEnable(GL_DEPTH_TEST);
}

void GCodeViewer3D::drawGrid()
{
    if (!gridProgram.bind()) {
        return;
    }
    
    gridProgram.setUniformValue("projection", projection);
    gridProgram.setUniformValue("view", view);
    gridProgram.setUniformValue("model", model);
    
    gridVao.bind();
    
    // Create grid that appears infinite
    float gridSize;
    float step;
    
    // Use consistent grid size and spacing
    gridSize = 500.0f; // Large grid to appear "infinite"
    step = 10.0f;      // Consistent spacing of 10mm
    
    std::vector<float> gridVertices;
    
    // X-axis (red)
    gridVertices.push_back(-gridSize); gridVertices.push_back(0); gridVertices.push_back(0);
    gridVertices.push_back(1.0f); gridVertices.push_back(0.0f); gridVertices.push_back(0.0f);
    
    gridVertices.push_back(gridSize); gridVertices.push_back(0); gridVertices.push_back(0);
    gridVertices.push_back(1.0f); gridVertices.push_back(0.0f); gridVertices.push_back(0.0f);
    
    // Y-axis (green)
    gridVertices.push_back(0); gridVertices.push_back(-gridSize); gridVertices.push_back(0);
    gridVertices.push_back(0.0f); gridVertices.push_back(1.0f); gridVertices.push_back(0.0f);
    
    gridVertices.push_back(0); gridVertices.push_back(gridSize); gridVertices.push_back(0);
    gridVertices.push_back(0.0f); gridVertices.push_back(1.0f); gridVertices.push_back(0.0f);
    
    // Z-axis (blue)
    gridVertices.push_back(0); gridVertices.push_back(0); gridVertices.push_back(-gridSize);
    gridVertices.push_back(0.0f); gridVertices.push_back(0.0f); gridVertices.push_back(1.0f);
    
    gridVertices.push_back(0); gridVertices.push_back(0); gridVertices.push_back(gridSize);
    gridVertices.push_back(0.0f); gridVertices.push_back(0.0f); gridVertices.push_back(1.0f);
    
    // Grid lines (light gray)
    // Create more grid lines for an "infinite" appearance
    int lineCount = static_cast<int>(gridSize / step) * 2;
    
    float start = -gridSize;
    for (int i = 0; i <= lineCount; i++) {
        float pos = start + i * step;
        
        if (std::abs(pos) < 0.001f) continue; // Skip center lines (already drawn as axes)
        
        // Lines parallel to X-axis
        gridVertices.push_back(-gridSize); gridVertices.push_back(pos); gridVertices.push_back(0);
        gridVertices.push_back(0.3f); gridVertices.push_back(0.3f); gridVertices.push_back(0.3f);
        
        gridVertices.push_back(gridSize); gridVertices.push_back(pos); gridVertices.push_back(0);
        gridVertices.push_back(0.3f); gridVertices.push_back(0.3f); gridVertices.push_back(0.3f);
        
        // Lines parallel to Y-axis
        gridVertices.push_back(pos); gridVertices.push_back(-gridSize); gridVertices.push_back(0);
        gridVertices.push_back(0.3f); gridVertices.push_back(0.3f); gridVertices.push_back(0.3f);
        
        gridVertices.push_back(pos); gridVertices.push_back(gridSize); gridVertices.push_back(0);
        gridVertices.push_back(0.3f); gridVertices.push_back(0.3f); gridVertices.push_back(0.3f);
    }
    
    // Update grid VBO with new vertices
    gridVbo.bind();
    gridVbo.allocate(gridVertices.data(), gridVertices.size() * sizeof(float));
    
    // Set up vertex attributes
    gridProgram.enableAttributeArray(0);
    gridProgram.setAttributeBuffer(0, GL_FLOAT, 0, 3, 6 * sizeof(float));
    
    gridProgram.enableAttributeArray(1);
    gridProgram.setAttributeBuffer(1, GL_FLOAT, 3 * sizeof(float), 3, 6 * sizeof(float));
    
    // Draw grid lines
    glLineWidth(1.0f);
    glDrawArrays(GL_LINES, 0, gridVertices.size() / 6);
    
    // Draw axes with thicker lines
    glLineWidth(2.0f);
    glDrawArrays(GL_LINES, 0, 6); // Just the 3 main axes (6 vertices)
    
    gridVao.release();
    gridProgram.release();
}

void GCodeViewer3D::updatePathVertices()
{
    // Just mark that the vertices need to be updated
    // The actual update will happen in makePathVertices
    pathNeedsUpdate = true;
    if (!updateTimer->isActive()) {
        updateTimer->start(100);
    }
}

void GCodeViewer3D::makePathVertices()
{
    // Don't try to update if not initialized
    if (!isValid() || !pathVbo.isCreated()) {
        return;
    }

    // Check if we have tool path points
    if (toolPath.empty() || !hasValidToolPath) {
        pathVertices.clear();
        return;
    }
    
    // Reset our data structures
    pathVertices.clear();
    m_pathSegments.clear();
    
    // Count how many rapid vs cutting moves we have (for debugging)
    int rapidMoveCount = 0;
    int cuttingMoveCount = 0;
    
    // For tracking segments
    int currentSegmentStart = 0;
    bool isCurrentRapid = toolPath[0].isRapid;
    int pointCount = 0;
    
    // Process all points and create segments where movement type changes
    for (size_t i = 0; i < toolPath.size(); i++) {
        const auto &point = toolPath[i];
        
        // If movement type changed, end current segment and start a new one
        if (i > 0 && point.isRapid != isCurrentRapid) {
            // End the current segment
            m_pathSegments.push_back(currentSegmentStart);
            m_pathSegments.push_back(pointCount);
            
            // Update counters based on segment type
            if (isCurrentRapid) {
                rapidMoveCount++;
            } else {
                cuttingMoveCount++;
            }
            
            // Start a new segment
            currentSegmentStart = pointCount;
            isCurrentRapid = point.isRapid;
        }
        
        // Add the point to our vertex array
        pathVertices.push_back(point.position.x());
        pathVertices.push_back(point.position.y());
        pathVertices.push_back(point.position.z());
        
        // Add color data
        if (point.isRapid) {
            // Orange for rapid moves
            pathVertices.push_back(1.0f);
            pathVertices.push_back(0.5f);
            pathVertices.push_back(0.0f);
        } else {
            // Blue for cutting moves
            pathVertices.push_back(0.0f);
            pathVertices.push_back(0.3f);
            pathVertices.push_back(1.0f);
        }
        
        pointCount++;
    }
    
    // Add the final segment
    if (pointCount > 0) {
        m_pathSegments.push_back(currentSegmentStart);
        m_pathSegments.push_back(pointCount);
        
        // Update counters based on final segment type
        if (isCurrentRapid) {
            rapidMoveCount++;
        } else {
            cuttingMoveCount++;
        }
    }
    
    // Debug output
    qDebug() << "Created" << rapidMoveCount << "rapid move segments and" 
             << cuttingMoveCount << "cutting move segments";
    
    // Only update GPU buffer if the context is valid
    if (isValid() && !pathVertices.empty()) {
        makeCurrent();
        pathVao.bind();
        pathVbo.bind();
        
        // Make sure buffer is large enough
        pathVbo.allocate(pathVertices.data(), pathVertices.size() * sizeof(float));
        
        // Update vertex attributes
        pathProgram.bind();
        pathProgram.enableAttributeArray(0);
        pathProgram.setAttributeBuffer(0, GL_FLOAT, 0, 3, 6 * sizeof(float));
        
        pathProgram.enableAttributeArray(1);
        pathProgram.setAttributeBuffer(1, GL_FLOAT, 3 * sizeof(float), 3, 6 * sizeof(float));
        pathProgram.release();
        
        pathVao.release();
        doneCurrent();
    }
    
    // Store the total visible points
    m_visiblePointCount = pointCount;
}

void GCodeViewer3D::drawToolPath()
{
    if (toolPath.empty() || pathVertices.empty() || m_visiblePointCount == 0) {
        return;
    }
    
    if (!pathProgram.bind()) {
        return;
    }
    
    // Set shader uniforms
    pathProgram.setUniformValue("projection", projection);
    pathProgram.setUniformValue("view", view);
    pathProgram.setUniformValue("model", model);
    
    pathVao.bind();
    
    // Use a separate draw call for each line segment
    // This ensures we don't connect points that shouldn't be connected
    glLineWidth(3.0f);
    
    for (size_t i = 0; i < toolPath.size() - 1; i++) {
        glLineWidth(3.0f);
        glDrawArrays(GL_LINE_STRIP, 0, m_visiblePointCount);
    }
    
    pathVao.release();
    pathProgram.release();
}

void GCodeViewer3D::mousePressEvent(QMouseEvent *event)
{
    // Only respond to left button clicks
    if (event->button() == Qt::LeftButton) {
        m_dragStartPos = event->pos();
        
        // Check if we're clicking on the view cube
        if (isPointInViewCube(event->pos())) {
            // Start a drag operation on the cube
            m_isDraggingCube = true;
            m_lastCubeDragPos = event->pos();
            setCursor(Qt::ClosedHandCursor);
            event->accept();
            return;
        }
        
        // Otherwise, store position for standard camera panning
        lastMousePos = event->pos();
    }
}

void GCodeViewer3D::mouseMoveEvent(QMouseEvent *event)
{
    // First check if we're hovering over the cube, regardless of drag state
    bool isOverCube = isPointInViewCube(event->pos());
    int hoveredFace = -1;
    
    if (isOverCube) {
        // Determine which face we're hovering over
        hoveredFace = getCubeFaceAtPoint(event->pos());
        
        // Update cursor
        if (hoveredFace >= 0) {
            setCursor(Qt::PointingHandCursor);
        } else {
            setCursor(Qt::ArrowCursor);
        }
    } else {
        setCursor(Qt::ArrowCursor);
    }
    
    // Update hover states regardless of drag state
    bool needsUpdate = false;
    
    // Update hover state on each face
    for (int i = 0; i < m_cubeViewFaces.size(); i++) {
        bool shouldBeHovered = (i == hoveredFace);
        if (m_cubeViewFaces[i].isHovered != shouldBeHovered) {
            m_cubeViewFaces[i].isHovered = shouldBeHovered;
            needsUpdate = true;
        }
    }
    
    // Update global hover state
    if (m_hoveredFaceId != hoveredFace) {
        m_hoveredFaceId = hoveredFace;
        needsUpdate = true;
    }
    
    // Now handle dragging if it's happening
    if (m_isDraggingCube) {
        // When dragging the cube, update its orientation
        rotateCubeByMouse(m_lastCubeDragPos, event->pos());
        m_lastCubeDragPos = event->pos();
        event->accept();
        
        // Always update after a drag
        update();
        return;
    } else if (event->buttons() & Qt::LeftButton) {
        // Handle regular view panning, NOT cube dragging
        // This runs when the mouse is away from the cube
        
        // Calculate mouse movement delta
        int dx = event->pos().x() - lastMousePos.x();
        int dy = event->pos().y() - lastMousePos.y();
        
        // Calculate pan speed based on scale (slower pan when zoomed in)
        float panSpeed = 1.0f / scale;
        
        // Extract the camera right and up vectors from the view matrix
        QVector3D rightVector(view(0, 0), view(0, 1), view(0, 2));
        QVector3D upVector(view(1, 0), view(1, 1), view(1, 2));
        
        // Project these vectors onto the XY plane to prevent Z drift
        rightVector.setZ(0);
        upVector.setZ(0);
        
        // Normalize if they're not zero vectors
        if (rightVector.length() > 0.001f) {
            rightVector.normalize();
        }
        if (upVector.length() > 0.001f) {
            upVector.normalize();
        }
        
        // Get movement vector for pan
        QVector3D movement = -rightVector * dx * panSpeed + upVector * dy * panSpeed;
        
        // Apply the movement to camera position and target
        cameraPosition += movement;
        cameraTarget += movement;
        m_rotationCenter += movement; // Move the rotation center when panning
        
        // Always update when panning
        update();
    }
    
    // Update the view if hover states changed
    if (needsUpdate) {
        update();
    }
    
    lastMousePos = event->pos();
}

void GCodeViewer3D::animateToViewDirection(const QVector3D& direction)
{
    // Normalize the direction
    QVector3D viewDir = direction.normalized();
    
    // Calculate distance from target to camera (preserve current distance)
    float distance = (cameraPosition - cameraTarget).length();
    
    // Calculate target camera position based on direction and distance
    QVector3D targetCameraPos = m_rotationCenter - viewDir * distance;
    
    // Calculate rotation needed to look from this position toward target
    QVector3D lookDir = (m_rotationCenter - targetCameraPos).normalized();
    
    // Find the axis and angle to align with this direction
    QVector3D initialDir(0, -1, 0); // Initial view direction
    QVector3D rotAxis = QVector3D::crossProduct(initialDir, lookDir);
    
    // Store the starting orientation
    m_startOrientation = m_viewOrientation;
    
    if (rotAxis.length() < 0.001f) {
        // Vectors are parallel or opposite
        if (QVector3D::dotProduct(initialDir, lookDir) < 0) {
            // Opposite directions - rotate 180 degrees around any perpendicular axis
            rotAxis = QVector3D(0, 0, 1);
            m_targetOrientation = QQuaternion::fromAxisAndAngle(rotAxis, 180.0f);
        } else {
            // Same direction - no rotation needed
            m_targetOrientation = QQuaternion();
        }
    } else {
        // Calculate rotation
        rotAxis.normalize();
        float angle = acos(QVector3D::dotProduct(initialDir, lookDir)) * 180.0f / M_PI;
        m_targetOrientation = QQuaternion::fromAxisAndAngle(rotAxis, angle);
    }
    
    // Start animation - target is always the rotation center now
    animateToView(targetCameraPos, m_rotationCenter);
}

void GCodeViewer3D::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        // Check if we were dragging the cube
        if (m_isDraggingCube) {
            // End drag operation
            m_isDraggingCube = false;
            
            // Reset cursor - it will be updated on next mouse move
            setCursor(Qt::ArrowCursor);
            
            // Check if this was a click (minimal movement)
            if ((event->pos() - m_dragStartPos).manhattanLength() < 5) {
                // This was a click rather than a drag
                // Get the face ID at this point
                int faceId = getCubeFaceAtPoint(event->pos());

                // If we have a valid face, navigate to it
                if (faceId >= 0 && faceId < m_cubeViewFaces.size()) {
                    animateToViewDirection(m_cubeViewFaces[faceId].normal);
                }
            }
            
            event->accept();
        }
        // We don't do anything on a regular mouse release - camera stays where it is
    }
}

void GCodeViewer3D::wheelEvent(QWheelEvent *event)
{
    float delta = event->angleDelta().y() / 120.0f;
    
    // Adjust zoom speed based on current scale
    float zoomFactor = 0.1f;
    if (scale < 0.1f) zoomFactor = 0.01f; // Finer control at extreme zoom levels
    
    scale *= (1.0f + zoomFactor * delta);
    
    // Expanded zoom range
    if (scale < 0.001f) scale = 0.001f; // Allow much closer zoom
    if (scale > 50.0f) scale = 50.0f;   // Allow much further zoom out
    
    update();
}

void GCodeViewer3D::processGCode(const QString &gcode)
{
    // Clear existing tool path first
    toolPath.clear();
    hasValidToolPath = false;
    
    // Check if the GCode is empty
    if (gcode.trimmed().isEmpty()) {
        // If empty, just show the grid with no tool path
        setIsometricView();
        scale = 0.5f;
        pathNeedsUpdate = true;
        updateTimer->start(100);
        return;
    }
    
    // Process normal GCode safely
    try {
        parseGCode(gcode);
        pathNeedsUpdate = true;
        updateTimer->start(100); // Wait a bit before updating to avoid multiple updates when editing
    } catch (const std::exception& e) {
        qDebug() << "Exception during GCode parsing:" << e.what();
        // Reset to a safe state
        toolPath.clear();
        hasValidToolPath = false;
        setIsometricView();
        scale = 0.5f;
        pathNeedsUpdate = true;
        updateTimer->start(100);
    }
}

void GCodeViewer3D::autoScaleToFit()
{
    if (toolPath.empty() || !hasValidToolPath) {
        return;
    }
    
    // Calculate model dimensions
    QVector3D modelDimensions = maxBounds - minBounds;
    
    // Avoid division by zero
    if (modelDimensions.length() < 0.001f) {
        scale = 1.0f;
        return;
    }
    
    // Calculate diagonal length of model
    float diagonal = modelDimensions.length();
    
    // Calculate scale factor to fit model in view with margin
    float targetScale = 250.0f / diagonal;
    
    // Add margin (making scale smaller means more zoomed out)
    targetScale *= 0.8f;
    
    // Adjust scale limits for very large models
    float minScale = 0.001f;
    float maxScale = 50.0f;
    
    // Limit scale to reasonable bounds
    if (targetScale < minScale) targetScale = minScale;
    if (targetScale > maxScale) targetScale = maxScale;
    
    // Apply new scale
    scale = targetScale;
    
    // Calculate model center and set as rotation center
    QVector3D modelCenter = (minBounds + maxBounds) * 0.5f;
    m_rotationCenter = modelCenter; // Set rotation center to model center
    cameraTarget = m_rotationCenter; // Update camera target to match
    
    // Maintain current orientation but adjust distance
    QVector3D viewDir = (cameraPosition - cameraTarget).normalized();
    float newDistance = diagonal / (targetScale * 0.8f);
    cameraPosition = cameraTarget + viewDir * newDistance;
    
    qDebug() << "Auto-scaled to fit. Model size:" << diagonal << "New scale:" << scale;
    
    update();
}

void GCodeViewer3D::handleResize()
{
    // Update the projection matrix based on the new size
    float aspectRatio = static_cast<float>(width()) / static_cast<float>(height() ? height() : 1);
    
    // Update projection matrix without changing other view parameters
    projection.setToIdentity();
    projection.perspective(45.0f, aspectRatio, 0.01f, 10000.0f);
    
    // Only update the display, don't change any view parameters
    update();
}

void GCodeViewer3D::parseGCode(const QString &gcode)
{
    // Clear existing tool path
    toolPath.clear();
    hasValidToolPath = false;
    
    // Initialize position and state
    QVector3D currentPos(0, 0, 0);
    bool isMetric = true;  // G21 is default (metric)
    bool isAbsolute = true;  // G90 is default (absolute coordinates)
    bool isRapid = true;  // Start with rapid movement (G00)
    
    // Initialize bounds with first point
    minBounds = QVector3D(0, 0, 0);
    maxBounds = QVector3D(0, 0, 0);
    bool boundsInitialized = false;
    
    // Reserve space for efficiency - guess at a reasonable size
    toolPath.reserve(10000);
    
    // Add initial position
    GCodePoint startPoint;
    startPoint.position = currentPos;
    startPoint.isRapid = true;
    toolPath.push_back(startPoint);
    
    // Process each line
    const QStringList lines = gcode.split(QRegularExpression("[\r\n]"), Qt::SkipEmptyParts);
    
    // Pre-compile regular expressions for better performance
    static QRegularExpression g0Regex("G0[\\s]|G00[\\s]", QRegularExpression::CaseInsensitiveOption);
    static QRegularExpression g1Regex("G1[\\s]|G01[\\s]", QRegularExpression::CaseInsensitiveOption);
    static QRegularExpression g20Regex("G20[\\s]", QRegularExpression::CaseInsensitiveOption);
    static QRegularExpression g21Regex("G21[\\s]", QRegularExpression::CaseInsensitiveOption);
    static QRegularExpression g90Regex("G90[\\s]", QRegularExpression::CaseInsensitiveOption);
    static QRegularExpression g91Regex("G91[\\s]", QRegularExpression::CaseInsensitiveOption);
    static QRegularExpression coordPattern("([XYZ])\\s*(-?\\d*\\.?\\d+)");
    
    // Debug counters
    int g00Count = 0;
    int g01Count = 0;
    
    for (const QString &line : lines) {
        QString trimmedLine = line.trimmed();
        
        // Skip empty lines and comments
        if (trimmedLine.isEmpty() || trimmedLine.startsWith(';')) {
            continue;
        }
        
        // Remove comments more efficiently
        int commentIdx = trimmedLine.indexOf(';');
        if (commentIdx >= 0) {
            trimmedLine = trimmedLine.left(commentIdx).trimmed();
        }
        
        // Skip if line is now empty
        if (trimmedLine.isEmpty()) {
            continue;
        }
        
        // Check for G-commands first - CRITICAL FIX HERE
        bool prevIsRapid = isRapid; // Remember previous state
        
        if (trimmedLine.contains(g0Regex)) {
            isRapid = true;
            g00Count++;
        }
        else if (trimmedLine.contains(g1Regex)) {
            isRapid = false;
            g01Count++;
        }
        // Don't reset isRapid for other commands - mode stays active until changed
        
        // Check for other G commands (unit settings, positioning mode)
        if (trimmedLine.contains(g20Regex)) {
            isMetric = false;  // Inch mode
        }
        else if (trimmedLine.contains(g21Regex)) {
            isMetric = true;   // Metric mode
        }
        else if (trimmedLine.contains(g90Regex)) {
            isAbsolute = true;  // Absolute positioning
        }
        else if (trimmedLine.contains(g91Regex)) {
            isAbsolute = false; // Relative positioning
        }
        
        // Check for X, Y, Z coordinates
        QVector3D newPos = currentPos;
        bool hasMovement = false;
        
        QRegularExpressionMatchIterator coordMatches = coordPattern.globalMatch(trimmedLine);
        
        while (coordMatches.hasNext()) {
            QRegularExpressionMatch match = coordMatches.next();
            QString axis = match.captured(1).toUpper();
            float value = match.captured(2).toFloat();
            
            // Convert inches to mm if needed
            if (!isMetric) {
                value *= 25.4f;
            }
            
            if (axis == "X") {
                newPos.setX(isAbsolute ? value : currentPos.x() + value);
                hasMovement = true;
            } 
            else if (axis == "Y") {
                newPos.setY(isAbsolute ? value : currentPos.y() + value);
                hasMovement = true;
            } 
            else if (axis == "Z") {
                newPos.setZ(isAbsolute ? value : currentPos.z() + value);
                hasMovement = true;
            }
        }
        
        // Only add point to path if there's actual movement
        if (hasMovement) {
            // If movement type changed or we have significant movement, add a new point
            float distance = (newPos - currentPos).length();
            
            // Always add if mode changed or moved at least 0.01mm
            if (isRapid != prevIsRapid || distance > 0.01f) {
                GCodePoint point;
                point.position = newPos;
                point.isRapid = isRapid;
                toolPath.push_back(point);
                hasValidToolPath = true;
                
                // Update bounds
                if (!boundsInitialized) {
                    minBounds = newPos;
                    maxBounds = newPos;
                    boundsInitialized = true;
                } else {
                    // Update mins and maxes
                    minBounds.setX(std::min(minBounds.x(), newPos.x()));
                    minBounds.setY(std::min(minBounds.y(), newPos.y()));
                    minBounds.setZ(std::min(minBounds.z(), newPos.z()));
                    
                    maxBounds.setX(std::max(maxBounds.x(), newPos.x()));
                    maxBounds.setY(std::max(maxBounds.y(), newPos.y()));
                    maxBounds.setZ(std::max(maxBounds.z(), newPos.z()));
                }
            }
            
            // Update current position
            currentPos = newPos;
        }
    }
    
    // If we have a valid tool path, ensure we have some bounds and add padding
    if (hasValidToolPath && boundsInitialized) {
        // Add a small padding around the model
        QVector3D padding = (maxBounds - minBounds) * 0.1f;
        if (padding.length() < 5.0f) {
            padding = QVector3D(5.0f, 5.0f, 5.0f);
        }
        minBounds -= padding;
        maxBounds += padding;
    } else {
        // If no valid path was found, clear the tool path
        toolPath.clear();
        hasValidToolPath = false;
    }
}