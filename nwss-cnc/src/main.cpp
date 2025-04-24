// main.cpp (updated)
#include "mainwindow.h"
#include <QApplication>
#include <QSurfaceFormat>
#include <QIcon>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    // Set up OpenGL format for 3D viewer
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    QSurfaceFormat::setDefaultFormat(format);
    
    QApplication app(argc, argv);
    
    // Set application information
    app.setApplicationName("NWSS-CNC");
    app.setOrganizationName("NWSS");
    app.setOrganizationDomain("nwss.org");
    
    // Use a modern style
    app.setStyle(QStyleFactory::create("Fusion"));

    QFont defaultFont("Helvetica", 12);
    app.setFont(defaultFont);
    
    // Create a more modern palette
    QPalette palette;
    palette.setColor(QPalette::Window, QColor(53, 53, 53));
    palette.setColor(QPalette::WindowText, Qt::white);
    palette.setColor(QPalette::Base, QColor(35, 35, 35));
    palette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    palette.setColor(QPalette::ToolTipBase, Qt::white);
    palette.setColor(QPalette::ToolTipText, Qt::white);
    palette.setColor(QPalette::Text, Qt::white);
    palette.setColor(QPalette::Button, QColor(53, 53, 53));
    palette.setColor(QPalette::ButtonText, Qt::white);
    palette.setColor(QPalette::BrightText, Qt::red);
    palette.setColor(QPalette::Link, QColor(42, 130, 218));
    palette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    palette.setColor(QPalette::HighlightedText, Qt::black);
    
    // Uncomment to use the dark theme
    // app.setPalette(palette);
    
    // Set application icon
    app.setWindowIcon(QIcon(":/icons/nwss-cnc-icon.png"));
    
    // Create and show the main window
    MainWindow mainWindow;
    mainWindow.show();
    
    return app.exec();
}