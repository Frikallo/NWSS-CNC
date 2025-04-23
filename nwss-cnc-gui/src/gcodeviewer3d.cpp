#include "gcodeviewer3d.h"
#include <QRegularExpression>
#include <QDebug>
#include <cmath>

GCodeViewer3D::GCodeViewer3D(QWidget *parent)
    : QOpenGLWidget(parent),
      gridVbo(QOpenGLBuffer::VertexBuffer),
      pathVbo(QOpenGLBuffer::VertexBuffer),
      rotationX(45.0f),
      rotationY(45.0f),
      scale(1.0f),
      minBounds(0.0f, 0.0f, 0.0f),
      maxBounds(100.0f, 100.0f, 100.0f),
      pathNeedsUpdate(false)
{
    setFocusPolicy(Qt::StrongFocus);
    
    // Set up a timer to handle batched updates
    updateTimer = new QTimer(this);
    updateTimer->setSingleShot(true);
    connect(updateTimer, &QTimer::timeout, [this]() {
        if (pathNeedsUpdate) {
            updatePathVertices();
            update();
            pathNeedsUpdate = false;
        }
    });
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
    gridVbo.allocate(nullptr, 1024 * sizeof(float)); // Allocate space for grid lines
    gridVao.release();
    
    pathVao.create();
    pathVao.bind();
    pathVbo.create();
    pathVbo.bind();
    pathVbo.allocate(nullptr, 1024 * sizeof(float)); // Initial allocation, will resize as needed
    pathVao.release();
    
    // Initialize matrices
    view.setToIdentity();
    view.translate(0.0f, 0.0f, -500.0f);
    
    model.setToIdentity();
    
    // Set up default tool path for testing
    toolPath.clear();
    GCodePoint p;
    p.position = QVector3D(0, 0, 0);
    p.isRapid = true;
    toolPath.push_back(p);
    p.position = QVector3D(50, 0, 0);
    p.isRapid = true;
    toolPath.push_back(p);
    p.position = QVector3D(50, 50, 0);
    p.isRapid = false;
    toolPath.push_back(p);
    p.position = QVector3D(0, 50, 0);
    p.isRapid = false;
    toolPath.push_back(p);
    p.position = QVector3D(0, 0, 0);
    p.isRapid = false;
    toolPath.push_back(p);
    
    updatePathVertices();
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
    projection.perspective(45.0f, aspectRatio, 0.1f, 1000.0f);
}

void GCodeViewer3D::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Update view matrix with current rotation and scale
    view.setToIdentity();
    view.translate(0.0f, 0.0f, -500.0f * scale);
    view.rotate(rotationX, 1.0f, 0.0f, 0.0f);
    view.rotate(rotationY, 0.0f, 1.0f, 0.0f);
    
    // Center the model
    QVector3D center = (maxBounds + minBounds) * 0.5f;
    model.setToIdentity();
    model.translate(-center);
    
    drawGrid();
    drawToolPath();
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
    
    // Create grid data dynamically based on tool path bounds
    float size = std::max({
        std::abs(maxBounds.x() - minBounds.x()),
        std::abs(maxBounds.y() - minBounds.y()),
        std::abs(maxBounds.z() - minBounds.z())
    }) * 1.5f;
    
    size = std::max(size, 100.0f); // Ensure minimum grid size
    
    float step = std::max(size / 20.0f, 10.0f); // Grid line spacing
    float halfSize = size / 2.0f;
    
    std::vector<float> gridVertices;
    
    // X-axis (red)
    gridVertices.push_back(-halfSize); gridVertices.push_back(0); gridVertices.push_back(0);
    gridVertices.push_back(1.0f); gridVertices.push_back(0.0f); gridVertices.push_back(0.0f);
    
    gridVertices.push_back(halfSize); gridVertices.push_back(0); gridVertices.push_back(0);
    gridVertices.push_back(1.0f); gridVertices.push_back(0.0f); gridVertices.push_back(0.0f);
    
    // Y-axis (green)
    gridVertices.push_back(0); gridVertices.push_back(-halfSize); gridVertices.push_back(0);
    gridVertices.push_back(0.0f); gridVertices.push_back(1.0f); gridVertices.push_back(0.0f);
    
    gridVertices.push_back(0); gridVertices.push_back(halfSize); gridVertices.push_back(0);
    gridVertices.push_back(0.0f); gridVertices.push_back(1.0f); gridVertices.push_back(0.0f);
    
    // Z-axis (blue)
    gridVertices.push_back(0); gridVertices.push_back(0); gridVertices.push_back(-halfSize);
    gridVertices.push_back(0.0f); gridVertices.push_back(0.0f); gridVertices.push_back(1.0f);
    
    gridVertices.push_back(0); gridVertices.push_back(0); gridVertices.push_back(halfSize);
    gridVertices.push_back(0.0f); gridVertices.push_back(0.0f); gridVertices.push_back(1.0f);
    
    // Grid lines (light gray)
    for (float i = -halfSize; i <= halfSize; i += step) {
        if (std::abs(i) < 0.001f) continue; // Skip center lines (already drawn as axes)
        
        // Lines parallel to X-axis
        gridVertices.push_back(-halfSize); gridVertices.push_back(i); gridVertices.push_back(0);
        gridVertices.push_back(0.3f); gridVertices.push_back(0.3f); gridVertices.push_back(0.3f);
        
        gridVertices.push_back(halfSize); gridVertices.push_back(i); gridVertices.push_back(0);
        gridVertices.push_back(0.3f); gridVertices.push_back(0.3f); gridVertices.push_back(0.3f);
        
        // Lines parallel to Y-axis
        gridVertices.push_back(i); gridVertices.push_back(-halfSize); gridVertices.push_back(0);
        gridVertices.push_back(0.3f); gridVertices.push_back(0.3f); gridVertices.push_back(0.3f);
        
        gridVertices.push_back(i); gridVertices.push_back(halfSize); gridVertices.push_back(0);
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
    glLineWidth(2.0f);
    glDrawArrays(GL_LINES, 0, gridVertices.size() / 6);
    
    gridVao.release();
    gridProgram.release();
}

void GCodeViewer3D::drawToolPath()
{
    if (toolPath.empty() || !pathProgram.bind()) {
        return;
    }
    
    pathProgram.setUniformValue("projection", projection);
    pathProgram.setUniformValue("view", view);
    pathProgram.setUniformValue("model", model);
    
    pathVao.bind();
    
    // Set up vertex attributes
    pathProgram.enableAttributeArray(0);
    pathProgram.setAttributeBuffer(0, GL_FLOAT, 0, 3, 6 * sizeof(float));
    
    pathProgram.enableAttributeArray(1);
    pathProgram.setAttributeBuffer(1, GL_FLOAT, 3 * sizeof(float), 3, 6 * sizeof(float));
    
    // Draw tool path
    glLineWidth(3.0f);
    glDrawArrays(GL_LINE_STRIP, 0, pathVertices.size() / 6);
    
    pathVao.release();
    pathProgram.release();
}

void GCodeViewer3D::updatePathVertices()
{
    if (toolPath.empty()) {
        return;
    }
    
    pathVertices.clear();
    
    for (const auto &point : toolPath) {
        // Position
        pathVertices.push_back(point.position.x());
        pathVertices.push_back(point.position.y());
        pathVertices.push_back(point.position.z());
        
        // Color (rapid moves are blue, cutting moves are green)
        if (point.isRapid) {
            pathVertices.push_back(0.2f);
            pathVertices.push_back(0.6f);
            pathVertices.push_back(1.0f);
        } else {
            pathVertices.push_back(0.2f);
            pathVertices.push_back(0.8f);
            pathVertices.push_back(0.2f);
        }
    }
    
    // Update path VBO with new vertices
    makeCurrent();
    pathVao.bind();
    pathVbo.bind();
    pathVbo.allocate(pathVertices.data(), pathVertices.size() * sizeof(float));
    pathVao.release();
    doneCurrent();
}

void GCodeViewer3D::mousePressEvent(QMouseEvent *event)
{
    lastMousePos = event->pos();
}

void GCodeViewer3D::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        int dx = event->pos().x() - lastMousePos.x();
        int dy = event->pos().y() - lastMousePos.y();
        
        rotationY += dx * 0.5f;
        rotationX += dy * 0.5f;
        
        // Limit X rotation to avoid flipping
        if (rotationX > 89.0f) rotationX = 89.0f;
        if (rotationX < -89.0f) rotationX = -89.0f;
        
        update();
    }
    
    lastMousePos = event->pos();
}

void GCodeViewer3D::wheelEvent(QWheelEvent *event)
{
    float delta = event->angleDelta().y() / 120.0f;
    
    scale *= (1.0f + 0.1f * delta);
    
    // Limit zoom levels
    if (scale < 0.1f) scale = 0.1f;
    if (scale > 10.0f) scale = 10.0f;
    
    update();
}

void GCodeViewer3D::processGCode(const QString &gcode)
{
    parseGCode(gcode);
    pathNeedsUpdate = true;
    updateTimer->start(100); // Wait a bit before updating to avoid multiple updates when editing
}

void GCodeViewer3D::parseGCode(const QString &gcode)
{
    // Clear existing tool path
    toolPath.clear();
    
    // Initialize position and state
    QVector3D currentPos(0, 0, 0);
    bool isMetric = true; // G21 is default (metric)
    bool isAbsolute = true; // G90 is default (absolute coordinates)
    bool isRapid = true; // Start with rapid movement
    
    // Regex for G-code commands
    QRegularExpression reCommand("([GM])\\s*(\\d+)(?:\\.\\d+)?");
    QRegularExpression reCoord("([XYZIJKFRS])\\s*(-?\\d*\\.?\\d+)");
    
    // Reset bounds
    minBounds = QVector3D(0, 0, 0);
    maxBounds = QVector3D(100, 100, 0);
    
    // Add initial position
    GCodePoint startPoint;
    startPoint.position = currentPos;
    startPoint.isRapid = true;
    toolPath.push_back(startPoint);
    
    // Process each line
    const QStringList lines = gcode.split(QRegularExpression("[\r\n]"), Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        // Skip comments
        if (line.trimmed().startsWith(';') || line.trimmed().isEmpty()) {
            continue;
        }
        
        QString processedLine = line;
        // Remove comments in parentheses and after semicolons
        int commentStart = processedLine.indexOf(';');
        if (commentStart >= 0) {
            processedLine = processedLine.left(commentStart);
        }
        
        commentStart = processedLine.indexOf('(');
        if (commentStart >= 0) {
            int commentEnd = processedLine.indexOf(')', commentStart);
            if (commentEnd >= 0) {
                processedLine.remove(commentStart, commentEnd - commentStart + 1);
            }
        }
        
        // Extract commands
        QRegularExpressionMatchIterator matchCommand = reCommand.globalMatch(processedLine);
        while (matchCommand.hasNext()) {
            QRegularExpressionMatch match = matchCommand.next();
            QString type = match.captured(1);
            int code = match.captured(2).toInt();
            
            if (type == "G") {
                switch (code) {
                    case 0: // Rapid move
                        isRapid = true;
                        break;
                    case 1: // Linear move
                    case 2: // CW arc (simplified as linear for now)
                    case 3: // CCW arc (simplified as linear for now)
                        isRapid = false;
                        break;
                    case 20: // Inch units
                        isMetric = false;
                        break;
                    case 21: // Millimeter units
                        isMetric = true;
                        break;
                    case 90: // Absolute coordinates
                        isAbsolute = true;
                        break;
                    case 91: // Relative coordinates
                        isAbsolute = false;
                        break;
                }
            }
        }
        
        // Extract coordinates
        QVector3D newPos = currentPos;
        bool hasPosition = false;
        
        QRegularExpressionMatchIterator matchCoord = reCoord.globalMatch(processedLine);
        while (matchCoord.hasNext()) {
            QRegularExpressionMatch match = matchCoord.next();
            QString axis = match.captured(1);
            float value = match.captured(2).toFloat();
            
            // Convert inches to mm if needed
            if (!isMetric) {
                value *= 25.4f;
            }
            
            if (axis == "X") {
                newPos.setX(isAbsolute ? value : currentPos.x() + value);
                hasPosition = true;
            } else if (axis == "Y") {
                newPos.setY(isAbsolute ? value : currentPos.y() + value);
                hasPosition = true;
            } else if (axis == "Z") {
                newPos.setZ(isAbsolute ? value : currentPos.z() + value);
                hasPosition = true;
            }
        }
        
        // Add new position to tool path if there's a position command
        if (hasPosition && (newPos != currentPos)) {
            GCodePoint point;
            point.position = newPos;
            point.isRapid = isRapid;
            toolPath.push_back(point);
            
            // Update bounds
            minBounds.setX(std::min(minBounds.x(), newPos.x()));
            minBounds.setY(std::min(minBounds.y(), newPos.y()));
            minBounds.setZ(std::min(minBounds.z(), newPos.z()));
            
            maxBounds.setX(std::max(maxBounds.x(), newPos.x()));
            maxBounds.setY(std::max(maxBounds.y(), newPos.y()));
            maxBounds.setZ(std::max(maxBounds.z(), newPos.z()));
            
            currentPos = newPos;
        }
    }
    
    // Ensure we have some bounds
    if (minBounds == maxBounds) {
        minBounds = QVector3D(-10, -10, -10);
        maxBounds = QVector3D(10, 10, 10);
    }
}