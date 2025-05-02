#ifndef GCODEVIEWER3D_H
#define GCODEVIEWER3D_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QMatrix4x4>
#include <QVector3D>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QString>
#include <QTimer>
#include <QPainter>
#include <vector>
#include <QQuaternion>

// GCode Tool Path Point
struct GCodePoint {
    QVector3D position;
    bool isRapid;  // true for G0, false for G1/G2/G3
};

// GCode Viewer 3D Widget
class GCodeViewer3D : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit GCodeViewer3D(QWidget *parent = nullptr);
    ~GCodeViewer3D();

    void processGCode(const QString &gcode);
    void handleResize();

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;
    
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    void setupGridShaders();
    void setupPathShaders();
    void drawGrid();
    void drawToolPath();
    void parseGCode(const QString &gcode);
    void makePathVertices();
    void updatePathVertices();
    void autoScaleToFit();
    void setIsometricView();
    
    // Navigation cube methods
    void initViewCube();
    void drawViewCube(QPainter* painter);
    bool isPointInViewCube(const QPoint& point);
    int getCubeFaceAtPoint(const QPoint& point);
    void setViewDirection(const QVector3D& direction);
    void animateToView(const QVector3D& targetPosition, const QVector3D& targetTarget);
    void updateAnimation();
    
    // Arcball rotation methods
    QVector3D mapToSphere(const QPointF& point);
    void rotateCubeByMouse(const QPoint& from, const QPoint& to);
    void applyCubeRotation(const QQuaternion& rotation);
    void animateToViewDirection(const QVector3D& direction);

    bool m_useBufferSubData;        // Use more efficient buffer updates
    int m_visiblePointCount = 0;         // Track visible points
    std::vector<int> m_pathSegments; // Track where paths start/end
    float m_viewportScale;           // Current viewport scale for LOD
    QVector3D m_viewportCenter;      // Current viewport center for culling
    std::vector<int> m_segmentTypes;
    
    // New optimization-related methods
    void updatePathBuffersEfficiently();
    void batchRenderPaths();
    int simplifyPathForViewport(const std::vector<GCodePoint>& path, 
                               std::vector<float>& outVertices,
                               float simplificationFactor);
    bool isPointVisible(const QVector3D& point);
    
    QOpenGLShaderProgram gridProgram;
    QOpenGLShaderProgram pathProgram;
    
    QOpenGLBuffer gridVbo;
    QOpenGLVertexArrayObject gridVao;
    
    QOpenGLBuffer pathVbo;
    QOpenGLVertexArrayObject pathVao;
    
    QMatrix4x4 projection;
    QMatrix4x4 view;
    QMatrix4x4 model;
    
    // Camera control variables
    QVector3D cameraPosition;
    QVector3D cameraTarget;
    float scale;
    QPoint lastMousePos;
    QPoint m_dragStartPos;
    bool m_ignorePanOnRelease;
    QQuaternion m_startOrientation;
    QQuaternion m_targetOrientation;
    QVector3D m_rotationCenter;
    
    std::vector<GCodePoint> toolPath;
    std::vector<float> pathVertices;
    
    QVector3D minBounds;
    QVector3D maxBounds;
    
    bool pathNeedsUpdate;
    bool autoScaleEnabled;  // Controls whether auto-scaling is enabled
    bool hasValidToolPath;  // Flag to track if we have valid toolpath data
    QTimer *updateTimer;
    
    // View cube structures and variables
    struct CubeFace {
        int id;
        QVector3D normal;
        std::vector<QVector3D> vertices;
        QVector3D center;
        QColor color;
        QColor hoverColor;
        QString label;
        bool isHovered;
    };
    
    std::vector<CubeFace> m_cubeViewFaces;
    int m_hoveredFaceId;
    bool m_cubeViewVisible;
    QPointF m_cubePosition; // Screen position of the cube
    float m_cubeSize; // Size of the cube in pixels
    
    // New variables for free cube rotation
    bool m_isDraggingCube;
    QPoint m_lastCubeDragPos;
    QQuaternion m_cubeOrientation;
    QQuaternion m_viewOrientation;
    
    // Animation for camera transitions
    bool m_isAnimating;
    QVector3D m_startCameraPosition;
    QVector3D m_targetCameraPosition;
    QVector3D m_startCameraTarget;
    QVector3D m_targetCameraTarget;
    float m_animationProgress;
    float m_animationDuration; // seconds
    QTimer* m_animationTimer;

    QOpenGLBuffer pathIndexBuffer;
    bool useIndexedRendering;
    int m_rapidIndexCount;
    int m_cuttingIndexCount;
    int m_rapidIndexOffset;
    int m_cuttingIndexOffset;
    bool m_useSimplifiedGeometry;
    int m_simplificationFactor;
    float m_lastScale;
    bool m_pathGeometryNeedsRebuilding;
    
    // For LOD (Level of Detail)
    std::vector<GCodePoint> m_simplifiedToolPath;
    std::vector<int> m_indicesRapid;
    std::vector<int> m_indicesCutting;
    void rebuildGeometryForCurrentLOD();
    GCodePoint interpolatePoints(const GCodePoint& a, const GCodePoint& b, float t);
};

#endif // GCODEVIEWER3D_H