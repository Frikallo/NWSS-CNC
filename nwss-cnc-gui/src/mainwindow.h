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
#include "svgviewer.h"
#include "gcodeoptionspanel.h"
#include "svgtogcode.h"

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
    SVGViewer *svgViewer;
    SvgToGCode *svgToGCode;

    QDockWidget *machineDock;
    QDockWidget *gcodeOptionsDock;

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
};

#endif // MAINWINDOW_H