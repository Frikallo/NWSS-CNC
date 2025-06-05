// mainwindow.h (updated)
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSplitter>
#include <QAction>
#include <QMenu>
#include <QToolBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QTabWidget>
#include <QDockWidget>
#include <QStackedWidget>
#include "gcodeeditor.h"
#include "gcodeviewer3d.h"
#include "gcodeoptionspanel.h"
#include "svgtogcode.h"
#include "svgdesigner.h"
#include "toolmanager.h"
#include "core/tool.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void newFile();
    void openFile();
    bool saveFile();
    bool saveAsFile();
    void about();
    void documentWasModified();
    void updateGCodePreview();
    void showWelcomeDialog();
    void importSvgFile();
    void convertSvgToGCode(const QString &svgFile);
    void onTabChanged(int index);
    void showToolManager();
    void onToolSelected(int toolId);
    void onToolRegistryChanged();

private:
    void createActions();
    void createMenus();
    void createToolBars();
    void createDockPanels();
    void createStatusBar();
    void readSettings();
    void writeSettings();
    bool maybeSave();
    void loadFile(const QString &fileName);
    bool saveFile(const QString &fileName);
    void setCurrentFile(const QString &fileName);
    QString strippedName(const QString &fullFileName);
    void setupTabWidget();
    void closeEvent(QCloseEvent *event);

    QString currentFile;
    bool isUntitled;

    QTabWidget *tabWidget;
    GCodeOptionsPanel *gcodeOptionsPanel;
    GCodeEditor *gCodeEditor;
    GCodeViewer3D *gCodeViewer;
    SVGDesigner *svgDesigner;
    SvgToGCode *svgToGCode;
    
    // Tool management
    nwss::cnc::ToolRegistry *toolRegistry;
    nwss::cnc::ToolSelector *toolSelector;

    QDockWidget *machineDock;
    QDockWidget *gcodeOptionsDock;
    QDockWidget *toolDock;

    QMenu *fileMenu;
    QMenu *editMenu;
    QMenu *viewMenu;
    QMenu *toolsMenu;
    QMenu *helpMenu;

    QToolBar *fileToolBar;
    QToolBar *editToolBar;
    QToolBar *viewToolBar;

    QAction *newAct;
    QAction *openAct;
    QAction *saveAct;
    QAction *saveAsAct;
    QAction *importSvgAct;
    QAction *exitAct;
    QAction *cutAct;
    QAction *copyAct;
    QAction *pasteAct;
    QAction *aboutAct;
    QAction *aboutQtAct;
    QAction *showMachinePanelAct;
    QAction *showGCodeOptionsPanelAct;
    QAction *showToolPanelAct;
    QAction *manageToolsAct;
    QLabel *timeEstimateLabel;
};

#endif // MAINWINDOW_H