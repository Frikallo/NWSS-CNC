// mainwindow.h (updated)
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QAction>
#include <QDockWidget>
#include <QFileDialog>
#include <QMainWindow>
#include <QMenu>
#include <QMessageBox>
#include <QProgressBar>
#include <QSplitter>
#include <QStackedWidget>
#include <QTabWidget>
#include <QTimer>
#include <QToolBar>
#include <QtWidgets>

#include "core/tool.h"
#include "gcodeeditor.h"
#include "gcodeoptionspanel.h"
#include "gcodeviewer3d.h"
#include "svgdesigner.h"
#include "svgtogcode.h"
#include "toolmanager.h"

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

 private slots:
  void newFile();
  void openFile();
  void importSvgFile();
  bool saveFile();
  bool saveAsFile();
  void about();
  void onGCodeChanged();
  void documentWasModified();
  void showWelcomeDialog();
  void onTabChanged(int index);
  void convertSvgToGCode(const QString &svgFile);
  void updateGCodePreview();
  void showToolManager();
  void onToolSelected(int toolId);
  void onToolRegistryChanged();
  void updateTimeEstimateLabel(double totalTimeSeconds);

 private:
  // Design transformation is now handled directly in convertSvgToGCode

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
  QAction *manageToolsAct;
  QLabel *timeEstimateLabel;
};

#endif  // MAINWINDOW_H