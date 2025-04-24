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
#include <vector>

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

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;
    
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    void setupGridShaders();
    void setupPathShaders();
    void drawGrid();
    void drawToolPath();
    void parseGCode(const QString &gcode);
    void updatePathVertices();
    void autoScaleToFit();
    
    QOpenGLShaderProgram gridProgram;
    QOpenGLShaderProgram pathProgram;
    
    QOpenGLBuffer gridVbo;
    QOpenGLVertexArrayObject gridVao;
    
    QOpenGLBuffer pathVbo;
    QOpenGLVertexArrayObject pathVao;
    
    QMatrix4x4 projection;
    QMatrix4x4 view;
    QMatrix4x4 model;
    
    float rotationX;
    float rotationY;
    float scale;
    QPoint lastMousePos;
    
    std::vector<GCodePoint> toolPath;
    std::vector<float> pathVertices;
    
    QVector3D minBounds;
    QVector3D maxBounds;
    
    bool pathNeedsUpdate;
    bool autoScaleEnabled;  // Controls whether auto-scaling is enabled
    bool hasValidToolPath;  // Flag to track if we have valid toolpath data
    QTimer *updateTimer;
};

#endif // GCODEVIEWER3D_H