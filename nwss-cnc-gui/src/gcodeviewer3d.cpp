#include "gcodeviewer3d.h"
#include <QRegularExpression>
#include <QDebug>
#include <cmath>

GCodeViewer3D::GCodeViewer3D(QWidget *parent)
    : QOpenGLWidget(parent),
      gridVbo(QOpenGLBuffer::VertexBuffer),
      pathVbo(QOpenGLBuffer::VertexBuffer),
      cameraPosition(0.0f, 0.0f, 0.0f),
      cameraTarget(0.0f, 0.0f, 0.0f),
      scale(0.5f),
      minBounds(-50.0f, -50.0f, -50.0f),
      maxBounds(50.0f, 50.0f, 50.0f),
      pathNeedsUpdate(false),
      autoScaleEnabled(false),
      hasValidToolPath(false)
{
    setFocusPolicy(Qt::StrongFocus);
    
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
    
    makePathVertices();
}

void GCodeViewer3D::setIsometricView()
{
    // Distance from origin
    float distance = 500.0f;
    
    // Create an isometric view rotated 90 degrees around Z
    // Instead of (1,1,1), use a different direction vector
    QVector3D isoDirection(1.0f, -1.0f, 1.0f); // Rotated 90 degrees around Z
    isoDirection.normalize();
    
    // Position camera for isometric projection
    cameraPosition = isoDirection * distance;
    cameraTarget = QVector3D(0.0f, 0.0f, 0.0f);
    
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
}

void GCodeViewer3D::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Maintain isometric view while applying scale
    QVector3D viewDirection = (cameraPosition - cameraTarget).normalized();
    float viewDistance = 300.0f / scale; // Scale affects how far away we are
    
    QVector3D scaledPosition = cameraTarget + viewDirection * viewDistance;
    
    // Update view matrix without rotation
    view.setToIdentity();
    view.lookAt(scaledPosition, cameraTarget, QVector3D(0.0f, 0.0f, 1.0f));
    
    // Apply rotation to the model matrix instead
    model.setToIdentity();
    
    drawGrid();
    
    // Only draw tool path if it exists
    if (hasValidToolPath) {
        drawToolPath();
    }
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

    pathVertices.clear();
    
    // Check if we have tool path points
    if (toolPath.empty() || !hasValidToolPath) {
        return;
    }
    
    // For each point in the tool path
    for (size_t i = 0; i < toolPath.size(); i++) {
        const auto &point = toolPath[i];
        
        // Add position coordinates
        pathVertices.push_back(point.position.x());
        pathVertices.push_back(point.position.y());
        pathVertices.push_back(point.position.z());
        
        // Add color data
        if (point.isRapid) {
            // Blue for rapid moves
            pathVertices.push_back(0.0f);
            pathVertices.push_back(0.5f);
            pathVertices.push_back(1.0f);
        } else {
            // Green for cutting moves
            pathVertices.push_back(0.0f);
            pathVertices.push_back(0.8f);
            pathVertices.push_back(0.2f);
        }
    }
    
    // Only update GPU buffer if the context is valid
    if (isValid()) {
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
}

void GCodeViewer3D::drawToolPath()
{
    if (toolPath.empty() || pathVertices.empty()) {
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
        // Draw just this segment
        glDrawArrays(GL_LINES, i, 2);
    }
    
    pathVao.release();
    pathProgram.release();
}

void GCodeViewer3D::mousePressEvent(QMouseEvent *event)
{
    lastMousePos = event->pos();
}

void GCodeViewer3D::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
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
        
        // Since we rotate the model and not the view, we need to rotate our movement vector
        // to match the rotated coordinate system
        QVector3D movement = -rightVector * dx * panSpeed + upVector * dy * panSpeed;
        
        // Create a rotation matrix for our 90-degree Z rotation
        QMatrix4x4 rotMat;
        rotMat.setToIdentity();

        // Apply rotation to the movement vector
        QVector4D rotatedMovement = rotMat * QVector4D(movement, 0.0f);
        movement = QVector3D(rotatedMovement.x(), rotatedMovement.y(), rotatedMovement.z());
        
        // Apply the rotated movement to camera position and target
        cameraPosition += movement;
        cameraTarget += movement;
        
        update();
    }
    
    lastMousePos = event->pos();
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
    float minScale = 0.001f; // Previously 0.1f
    float maxScale = 50.0f;  // Previously 10.0f
    
    // Limit scale to reasonable bounds
    if (targetScale < minScale) targetScale = minScale;
    if (targetScale > maxScale) targetScale = maxScale;
    
    // Apply new scale
    scale = targetScale;
    
    // Center the view on the model
    QVector3D modelCenter = (minBounds + maxBounds) * 0.5f;
    cameraTarget = modelCenter;
    cameraPosition = cameraTarget + (cameraPosition - cameraTarget);
    
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
    bool isRapid = true;  // Start with rapid movement
    
    // Initialize bounds with first point
    minBounds = QVector3D(0, 0, 0);
    maxBounds = QVector3D(0, 0, 0);
    bool boundsInitialized = false;
    
    // Add initial position
    GCodePoint startPoint;
    startPoint.position = currentPos;
    startPoint.isRapid = true;
    toolPath.push_back(startPoint);
    
    // Process each line
    const QStringList lines = gcode.split(QRegularExpression("[\r\n]"), Qt::SkipEmptyParts);
    
    for (const QString &line : lines) {
        QString trimmedLine = line.trimmed();
        
        // Skip empty lines and comments
        if (trimmedLine.isEmpty() || trimmedLine.startsWith(';')) {
            continue;
        }
        
        // Remove comments
        int commentIdx = trimmedLine.indexOf(';');
        if (commentIdx >= 0) {
            trimmedLine = trimmedLine.left(commentIdx).trimmed();
        }
        
        // Skip if line is now empty
        if (trimmedLine.isEmpty()) {
            continue;
        }
        
        // Check for G-commands first
        if (trimmedLine.contains(QRegularExpression("G0[\\s]", QRegularExpression::CaseInsensitiveOption)) || 
            trimmedLine.contains(QRegularExpression("G00[\\s]", QRegularExpression::CaseInsensitiveOption))) {
            isRapid = true;
        }
        else if (trimmedLine.contains(QRegularExpression("G1[\\s]", QRegularExpression::CaseInsensitiveOption)) || 
                 trimmedLine.contains(QRegularExpression("G01[\\s]", QRegularExpression::CaseInsensitiveOption))) {
            isRapid = false;
        }
        else if (trimmedLine.contains(QRegularExpression("G20[\\s]", QRegularExpression::CaseInsensitiveOption))) {
            isMetric = false;  // Inch mode
        }
        else if (trimmedLine.contains(QRegularExpression("G21[\\s]", QRegularExpression::CaseInsensitiveOption))) {
            isMetric = true;   // Metric mode
        }
        else if (trimmedLine.contains(QRegularExpression("G90[\\s]", QRegularExpression::CaseInsensitiveOption))) {
            isAbsolute = true;  // Absolute positioning
        }
        else if (trimmedLine.contains(QRegularExpression("G91[\\s]", QRegularExpression::CaseInsensitiveOption))) {
            isAbsolute = false; // Relative positioning
        }
        
        // Check for X, Y, Z coordinates
        QVector3D newPos = currentPos;
        bool hasMovement = false;
        
        QRegularExpression coordPattern("([XYZ])\\s*(-?\\d*\\.?\\d+)");
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
        
        // If we have movement coordinates, add to the tool path
        if (hasMovement) {
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
                // Update mins
                minBounds.setX(std::min(minBounds.x(), newPos.x()));
                minBounds.setY(std::min(minBounds.y(), newPos.y()));
                minBounds.setZ(std::min(minBounds.z(), newPos.z()));
                
                // Update maxes
                maxBounds.setX(std::max(maxBounds.x(), newPos.x()));
                maxBounds.setY(std::max(maxBounds.y(), newPos.y()));
                maxBounds.setZ(std::max(maxBounds.z(), newPos.z()));
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
        
        qDebug() << "GCode parsed. Points: " << toolPath.size() 
                 << " Min bounds: " << minBounds 
                 << " Max bounds: " << maxBounds;
    } else {
        // If no valid path was found, clear the tool path
        toolPath.clear();
        hasValidToolPath = false;
    }
}