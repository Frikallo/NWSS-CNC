#include "mainwindow.h"
#include <QApplication>
#include <QSurfaceFormat>
#include <QIcon>
#include <QStyleFactory>
#include <QFontDatabase>
#include <QDebug>

void setBlackOrangeTheme(QApplication &app)
{
    // Black and orange theme palette
    QPalette darkPalette;
    
    // Base colors - Pure black theme
    QColor pureBlack(0, 0, 0);                    // Deep black background
    QColor darkGray(25, 25, 25);                  // Slightly lighter black
    QColor mediumGray(40, 40, 40);                // Medium gray for contrast
    QColor lightGray(60, 60, 60);                 // Lighter elements
    QColor veryLightGray(200, 200, 200);          // Main text color
    QColor white(255, 255, 255);                  // Bright text
    
    // Orange accent colors
    QColor primaryOrange(255, 140, 0);            // Main orange accent (#FF8C00)
    QColor brightOrange(255, 165, 0);             // Brighter orange (#FFA500)
    QColor darkOrange(255, 100, 0);               // Darker orange
    QColor lightOrange(255, 180, 50);             // Light orange for highlights
    
    // Status colors with orange theme
    QColor successOrange(255, 165, 0);            // Success state
    QColor warningOrange(255, 69, 0);             // Warning (red-orange)
    QColor errorRed(220, 20, 20);                 // Error (keep red for clarity)
    
    // Window and base colors
    darkPalette.setColor(QPalette::Window, pureBlack);
    darkPalette.setColor(QPalette::WindowText, veryLightGray);
    darkPalette.setColor(QPalette::Base, darkGray);
    darkPalette.setColor(QPalette::AlternateBase, mediumGray);
    darkPalette.setColor(QPalette::ToolTipBase, pureBlack);
    darkPalette.setColor(QPalette::ToolTipText, primaryOrange);
    
    // Text colors
    darkPalette.setColor(QPalette::Text, veryLightGray);
    darkPalette.setColor(QPalette::BrightText, white);
    
    // Button colors
    darkPalette.setColor(QPalette::Button, mediumGray);
    darkPalette.setColor(QPalette::ButtonText, veryLightGray);
    
    // Selection and highlight colors - Orange theme
    darkPalette.setColor(QPalette::Highlight, primaryOrange);
    darkPalette.setColor(QPalette::HighlightedText, pureBlack);
    
    // Link colors
    darkPalette.setColor(QPalette::Link, brightOrange);
    darkPalette.setColor(QPalette::LinkVisited, darkOrange);
    
    // Disabled states - darker and muted
    darkPalette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(100, 100, 100));
    darkPalette.setColor(QPalette::Disabled, QPalette::Text, QColor(100, 100, 100));
    darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(100, 100, 100));
    darkPalette.setColor(QPalette::Disabled, QPalette::Base, QColor(15, 15, 15));
    darkPalette.setColor(QPalette::Disabled, QPalette::Button, QColor(20, 20, 20));
    darkPalette.setColor(QPalette::Disabled, QPalette::Window, QColor(10, 10, 10));
    
    // Active state colors (when window is focused)
    darkPalette.setColor(QPalette::Active, QPalette::Highlight, primaryOrange);
    darkPalette.setColor(QPalette::Active, QPalette::HighlightedText, pureBlack);
    
    // Inactive state colors (when window loses focus)
    darkPalette.setColor(QPalette::Inactive, QPalette::Highlight, QColor(180, 100, 0));
    darkPalette.setColor(QPalette::Inactive, QPalette::HighlightedText, veryLightGray);
    
    // Additional color roles for modern Qt
    darkPalette.setColor(QPalette::PlaceholderText, QColor(120, 120, 120));
    
    // Set the palette
    app.setPalette(darkPalette);
}

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
    
    // Use Fusion style as base
    app.setStyle(QStyleFactory::create("Fusion"));

    // Apply modern dark theme
    // setDarkTheme(app);
    setBlackOrangeTheme(app);

    // Load and set SF Pro as the default application font
    QFont defaultFont;
    
    // Try to load SF Pro fonts
    int sfRegularId = QFontDatabase::addApplicationFont(":/fonts/SF-Pro-Display-Regular.otf");
    int sfBoldId = QFontDatabase::addApplicationFont(":/fonts/SF-Pro-Display-Bold.otf");
    int sfMediumId = QFontDatabase::addApplicationFont(":/fonts/SF-Pro-Display-Medium.otf");
    
    if (sfRegularId != -1) {
        // SF Pro fonts loaded successfully
        QStringList fontFamilies = QFontDatabase::applicationFontFamilies(sfRegularId);
        if (!fontFamilies.isEmpty()) {
            defaultFont.setFamily(fontFamilies.at(0));
            qDebug() << "SF Pro font loaded successfully:" << fontFamilies.at(0);
        }
    } else {
        // Fallback to system SF font or Helvetica
        #ifdef Q_OS_MACOS
            defaultFont.setFamily("SF Pro Display");
        #else
            defaultFont.setFamily("Segoe UI");  // Better Windows fallback
        #endif
        qDebug() << "Using system default font:" << defaultFont.family();
    }
    
    // Set consistent font size across all platforms
    defaultFont.setPointSize(13);
    defaultFont.setWeight(QFont::Normal);
    app.setFont(defaultFont);
    
    // Set application icon
    app.setWindowIcon(QIcon(":/icons/nwss-cnc-icon.png"));
    
    // Create and show the main window
    MainWindow mainWindow;
    mainWindow.show();
    
    return app.exec();
}