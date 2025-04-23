#include "mainwindow.h"
#include <QApplication>
#include <QSurfaceFormat>

int main(int argc, char *argv[])
{
    // Set up OpenGL format for 3D rendering
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    QSurfaceFormat::setDefaultFormat(format);
    
    QApplication app(argc, argv);
    
    MainWindow mainWindow;
    mainWindow.setWindowTitle("NWSS-CNC: GCode Editor and Previewer");
    mainWindow.resize(1200, 800);
    mainWindow.show();
    
    return app.exec();
}