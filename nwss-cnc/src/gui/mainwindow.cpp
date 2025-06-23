// mainwindow.cpp (updated)
#include "mainwindow.h"

#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QSet>
#include <QSettings>
#include <QStatusBar>
#include <QTextStream>
#include <QTimer>
#include <QToolBar>

#include "config.h"
#include "discretizer.h"
#include "gcode_generator.h"
#include "gcodeoptionspanel.h"
#include "svg_parser.h"
#include "transform.h"

class WelcomeDialog : public QDialog {
 public:
  WelcomeDialog(QWidget *parent = nullptr) : QDialog(parent) {
    setWindowTitle(tr("Welcome to NWSS-CNC"));
    resize(500, 300);

    QGridLayout *layout = new QGridLayout(this);

    QLabel *titleLabel = new QLabel(tr("<h1>Welcome to NWSS-CNC</h1>"));
    titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(titleLabel, 0, 0, 1, 2);

    QLabel *subtitleLabel =
        new QLabel(tr("Choose how you want to get started:"));
    subtitleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(subtitleLabel, 1, 0, 1, 2);

    // Create option buttons with icons
    QPushButton *newFileButton = new QPushButton(tr("New Empty Project"));
    newFileButton->setMinimumHeight(100);
    connect(newFileButton, &QPushButton::clicked, this, [this]() {
      done(1);  // New file
    });

    QPushButton *openGCodeButton = new QPushButton(tr("Open G-Code File"));
    openGCodeButton->setMinimumHeight(100);
    connect(openGCodeButton, &QPushButton::clicked, this, [this]() {
      done(2);  // Open GCode
    });

    QPushButton *importSvgButton = new QPushButton(tr("Import SVG File"));
    importSvgButton->setMinimumHeight(100);
    connect(importSvgButton, &QPushButton::clicked, this, [this]() {
      done(3);  // Import SVG
    });

    // Add buttons to layout
    layout->addWidget(newFileButton, 2, 0);
    layout->addWidget(openGCodeButton, 2, 1);
    layout->addWidget(importSvgButton, 3, 0, 1, 2);

    // Add checkbox to not show this dialog again
    QCheckBox *dontShowAgainCheckbox =
        new QCheckBox(tr("Don't show this dialog on startup"));
    layout->addWidget(dontShowAgainCheckbox, 4, 0, 1, 2);
    connect(dontShowAgainCheckbox, &QCheckBox::toggled, this, [](bool checked) {
      QSettings settings("NWSS", "NWSS-CNC");
      settings.setValue("hideWelcomeDialog", checked);
    });

    // Add a separator line
    QFrame *line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    layout->addWidget(line, 5, 0, 1, 2);

    // Add Close button at bottom
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttonBox, 6, 0, 1, 2);
  }
};

// In MainWindow constructor:
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), isUntitled(true) {
  // Create all the components
  gCodeEditor = new GCodeEditor(this);
  gCodeViewer = new GCodeViewer3D(this);
  svgDesigner = new SVGDesigner(this);  // Changed from svgViewer
  gcodeOptionsPanel = new GCodeOptionsPanel(this);
  svgToGCode = new SvgToGCode(this);

  // Create tool management components
  toolRegistry = new nwss::cnc::ToolRegistry();
  toolSelector = new nwss::cnc::ToolSelector(*toolRegistry, this);

  // Set up the tab widget for the main content area
  setupTabWidget();

  // Create the UI elements
  createActions();
  createMenus();
  createToolBars();
  createDockPanels();
  createStatusBar();

  // Set up signals and slots
  connect(gCodeEditor->document(), &QTextDocument::contentsChanged, this,
          &MainWindow::documentWasModified);
  connect(gCodeEditor, &GCodeEditor::textChanged, this,
          &MainWindow::updateGCodePreview);
  connect(tabWidget, &QTabWidget::currentChanged, this,
          &MainWindow::onTabChanged);
  connect(gcodeOptionsPanel, &GCodeOptionsPanel::generateGCode,
          [this](const QString &svgFile) {
            // Use default conversion without design bounds for the options
            // panel
            this->convertSvgToGCode(svgFile);
          });

  // Note: SVG designer now integrates with the unified G-code generation system
  // All G-code generation should go through the options panel button
  connect(svgDesigner, &SVGDesigner::svgLoaded,
          [this](const QString &filePath) {
            gcodeOptionsPanel->setCurrentSvgFile(filePath);
          });

  // Connect material size updates to designer
  connect(gcodeOptionsPanel, &GCodeOptionsPanel::settingsLoaded, [this]() {
    // Update material and bed size in the designer
    svgDesigner->setMaterialSize(gcodeOptionsPanel->getMaterialWidth(),
                                 gcodeOptionsPanel->getMaterialHeight());

    svgDesigner->setBedSize(gcodeOptionsPanel->getBedWidth(),
                            gcodeOptionsPanel->getBedHeight());
  });

  // Update designer when material settings change
  connect(gcodeOptionsPanel, &GCodeOptionsPanel::optionsChanged, [this]() {
    svgDesigner->setMaterialSize(gcodeOptionsPanel->getMaterialWidth(),
                                 gcodeOptionsPanel->getMaterialHeight());

    svgDesigner->setBedSize(gcodeOptionsPanel->getBedWidth(),
                            gcodeOptionsPanel->getBedHeight());
  });

  // Connect tool management signals
  connect(toolSelector, &nwss::cnc::ToolSelector::toolSelected, this,
          &MainWindow::onToolSelected);

  // Initial state
  setCurrentFile("");
  readSettings();

  // Load the last selected tool from settings
  QSettings settings("NWSS", "NWSS-CNC");
  int lastSelectedToolId = settings.value("lastSelectedToolId", 0).toInt();
  if (lastSelectedToolId > 0) {
    // Check if the tool still exists in the registry
    const nwss::cnc::Tool *tool = toolRegistry->getTool(lastSelectedToolId);
    if (tool) {
      toolSelector->setSelectedTool(lastSelectedToolId);
      statusBar()->showMessage(tr("Restored last selected tool: %1")
                                   .arg(QString::fromStdString(tool->name)),
                               3000);
    }
  }

  // Show welcome dialog (if not disabled)
  bool hideWelcomeDialog = settings.value("hideWelcomeDialog", false).toBool();

  if (!hideWelcomeDialog) {
    QTimer::singleShot(100, this, &MainWindow::showWelcomeDialog);
  }

  // Delay initial refresh to allow UI to be fully constructed
  QTimer::singleShot(200, this, &MainWindow::updateGCodePreview);

  // Set window properties
  setWindowTitle(tr("NWSS-CNC"));
  setMinimumSize(800, 600);
  resize(1200, 800);
}

MainWindow::~MainWindow() { delete toolRegistry; }

void MainWindow::setupTabWidget() {
  // Create tab widget
  tabWidget = new QTabWidget(this);
  tabWidget->setTabPosition(QTabWidget::North);
  tabWidget->setMovable(true);
  tabWidget->setDocumentMode(true);

  // Add tabs
  tabWidget->addTab(gCodeEditor, tr("G-Code Editor"));
  tabWidget->addTab(gCodeViewer, tr("3D Preview"));
  tabWidget->addTab(svgDesigner, tr("Designer"));

  // Set as central widget
  setCentralWidget(tabWidget);
}

void MainWindow::createActions() {
  // File menu actions
  newAct = new QAction(tr("&New"), this);
  newAct->setShortcuts(QKeySequence::New);
  newAct->setStatusTip(tr("Create a new file"));
  connect(newAct, &QAction::triggered, this, &MainWindow::newFile);

  openAct = new QAction(tr("&Open G-Code..."), this);
  openAct->setShortcuts(QKeySequence::Open);
  openAct->setStatusTip(tr("Open an existing G-Code file"));
  connect(openAct, &QAction::triggered, this, &MainWindow::openFile);

  importSvgAct = new QAction(tr("&Import SVG..."), this);
  importSvgAct->setStatusTip(tr("Import an SVG file for conversion to G-Code"));
  connect(importSvgAct, &QAction::triggered, this, &MainWindow::importSvgFile);

  saveAct = new QAction(tr("&Save"), this);
  saveAct->setShortcuts(QKeySequence::Save);
  saveAct->setStatusTip(tr("Save the document to disk"));
  connect(saveAct, &QAction::triggered, this, [this]() { saveFile(); });

  saveAsAct = new QAction(tr("Save &As..."), this);
  saveAsAct->setShortcuts(QKeySequence::SaveAs);
  saveAsAct->setStatusTip(tr("Save the document under a new name"));
  connect(saveAsAct, &QAction::triggered, this, &MainWindow::saveAsFile);

  exitAct = new QAction(tr("E&xit"), this);
  exitAct->setShortcuts(QKeySequence::Quit);
  exitAct->setStatusTip(tr("Exit the application"));
  connect(exitAct, &QAction::triggered, this, &QWidget::close);

  // Edit menu actions
  cutAct = new QAction(tr("Cu&t"), this);
  cutAct->setShortcuts(QKeySequence::Cut);
  cutAct->setStatusTip(
      tr("Cut the current selection's contents to the clipboard"));
  connect(cutAct, &QAction::triggered, gCodeEditor, &GCodeEditor::cut);

  copyAct = new QAction(tr("&Copy"), this);
  copyAct->setShortcuts(QKeySequence::Copy);
  copyAct->setStatusTip(
      tr("Copy the current selection's contents to the clipboard"));
  connect(copyAct, &QAction::triggered, gCodeEditor, &GCodeEditor::copy);

  pasteAct = new QAction(tr("&Paste"), this);
  pasteAct->setShortcuts(QKeySequence::Paste);
  pasteAct->setStatusTip(
      tr("Paste the clipboard's contents into the current selection"));
  connect(pasteAct, &QAction::triggered, gCodeEditor, &GCodeEditor::paste);

  // Dock panels are now always visible - removed toggle actions

  // Tools menu actions
  manageToolsAct = new QAction(tr("&Manage Tools..."), this);
  manageToolsAct->setStatusTip(tr("Open the tool management dialog"));
  connect(manageToolsAct, &QAction::triggered, this,
          &MainWindow::showToolManager);

  // Help menu actions
  aboutAct = new QAction(tr("&About"), this);
  aboutAct->setStatusTip(tr("Show the application's About box"));
  connect(aboutAct, &QAction::triggered, this, &MainWindow::about);

  aboutQtAct = new QAction(tr("About &Qt"), this);
  aboutQtAct->setStatusTip(tr("Show the Qt library's About box"));
  connect(aboutQtAct, &QAction::triggered, qApp, &QApplication::aboutQt);
}

void MainWindow::createMenus() {
  fileMenu = menuBar()->addMenu(tr("&File"));
  fileMenu->addAction(newAct);
  fileMenu->addAction(openAct);
  fileMenu->addAction(importSvgAct);
  fileMenu->addSeparator();
  fileMenu->addAction(saveAct);
  fileMenu->addAction(saveAsAct);
  fileMenu->addSeparator();
  fileMenu->addAction(exitAct);

  editMenu = menuBar()->addMenu(tr("&Edit"));
  editMenu->addAction(cutAct);
  editMenu->addAction(copyAct);
  editMenu->addAction(pasteAct);

  viewMenu = menuBar()->addMenu(tr("&View"));
  // Dock panels are now always visible - removed toggle actions

  toolsMenu = menuBar()->addMenu(tr("&Tools"));
  toolsMenu->addAction(manageToolsAct);

  menuBar()->addSeparator();

  helpMenu = menuBar()->addMenu(tr("&Help"));
  helpMenu->addAction(aboutAct);
  helpMenu->addAction(aboutQtAct);
}

void MainWindow::createToolBars() {
  fileToolBar = addToolBar(tr("File"));
  fileToolBar->addAction(newAct);
  fileToolBar->addAction(openAct);
  fileToolBar->addAction(importSvgAct);
  fileToolBar->addAction(saveAct);

  editToolBar = addToolBar(tr("Edit"));
  editToolBar->addAction(cutAct);
  editToolBar->addAction(copyAct);
  editToolBar->addAction(pasteAct);

  viewToolBar = addToolBar(tr("View"));

  // Add a tab selector to the view toolbar
  QLabel *tabLabel = new QLabel(tr("View: "));
  tabLabel->setMargin(int(5));
  // Use application default font for consistency
  viewToolBar->addWidget(tabLabel);

  QComboBox *tabComboBox = new QComboBox();
  tabComboBox->addItem(tr("G-Code Editor"));
  tabComboBox->addItem(tr("3D Preview"));
  tabComboBox->addItem(tr("Designer"));
  connect(tabComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
          tabWidget, &QTabWidget::setCurrentIndex);
  viewToolBar->addWidget(tabComboBox);

  // Connect tab changes to update the combo box
  connect(tabWidget, &QTabWidget::currentChanged, tabComboBox,
          &QComboBox::setCurrentIndex);
}

void MainWindow::createDockPanels() {
  // Options panel dock
  gcodeOptionsDock = new QDockWidget(tr(""), this);
  gcodeOptionsDock->setWidget(gcodeOptionsPanel);
  gcodeOptionsDock->setFeatures(QDockWidget::NoDockWidgetFeatures);
  addDockWidget(Qt::LeftDockWidgetArea, gcodeOptionsDock);

  // Tool panel dock
  toolDock = new QDockWidget(tr(""), this);
  toolDock->setWidget(toolSelector);
  toolDock->setFeatures(QDockWidget::NoDockWidgetFeatures);
  addDockWidget(Qt::LeftDockWidgetArea, toolDock);

  // Dock panels are always visible - no toggle connections needed
  gcodeOptionsDock->setVisible(true);
  toolDock->setVisible(true);
}

void MainWindow::createStatusBar() {
  // Create time estimate label
  timeEstimateLabel = new QLabel(tr("Est. time: --:--:--"));
  timeEstimateLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  timeEstimateLabel->setContentsMargins(5, 0, 5, 0);
  timeEstimateLabel->setMinimumWidth(150);

  statusBar()->addPermanentWidget(timeEstimateLabel);
  statusBar()->showMessage(tr("Ready"));
}

void MainWindow::readSettings() {
  QSettings settings("NWSS", "NWSS-CNC");
  QPoint pos = settings.value("pos", QPoint(200, 200)).toPoint();
  QSize size = settings.value("size", QSize(1200, 800)).toSize();
  move(pos);
  resize(size);

  // Dock panels are always visible - no need to restore visibility state
  gcodeOptionsDock->setVisible(true);
  toolDock->setVisible(true);

  // Restore tab index
  int tabIndex = settings.value("activeTabIndex", 0).toInt();
  tabWidget->setCurrentIndex(tabIndex);
}

void MainWindow::writeSettings() {
  QSettings settings("NWSS", "NWSS-CNC");
  settings.setValue("pos", pos());
  settings.setValue("size", size());

  // Dock panels are always visible - no need to save visibility state

  // Save active tab
  settings.setValue("activeTabIndex", tabWidget->currentIndex());
}

void MainWindow::showWelcomeDialog() {
  WelcomeDialog dialog(this);
  int result = dialog.exec();

  switch (result) {
    case 1:  // New file
      newFile();
      break;
    case 2:  // Open GCode
      openFile();
      break;
    case 3:  // Import SVG
      importSvgFile();
      break;
    default:
      // User closed the dialog
      break;
  }
}

void MainWindow::newFile() {
  if (maybeSave()) {
    gCodeEditor->clear();
    setCurrentFile("");
    updateTimeEstimateLabel(0);     // Reset time estimate
    tabWidget->setCurrentIndex(0);  // Switch to editor tab
  }
}

void MainWindow::openFile() {
  if (maybeSave()) {
    QString fileName = QFileDialog::getOpenFileName(
        this, tr("Open GCode File"), "",
        tr("GCode Files (*.gcode *.nc *.ngc);;All Files (*)"));
    if (!fileName.isEmpty()) {
      loadFile(fileName);
      tabWidget->setCurrentIndex(0);  // Switch to editor tab
    }
  }
}

void MainWindow::importSvgFile() {
  QString fileName = QFileDialog::getOpenFileName(
      this, tr("Import SVG File"), "", tr("SVG Files (*.svg);;All Files (*)"));
  if (!fileName.isEmpty()) {
    svgDesigner->loadSvgFile(fileName);  // Changed from svgViewer
    tabWidget->setCurrentIndex(2);
  }
}

bool MainWindow::saveFile() {
  if (isUntitled) {
    return saveAsFile();
  } else {
    return saveFile(currentFile);
  }
}

bool MainWindow::saveAsFile() {
  QString fileName = QFileDialog::getSaveFileName(
      this, tr("Save GCode File"), "",
      tr("GCode Files (*.gcode *.nc *.ngc);;All Files (*)"));
  if (fileName.isEmpty()) return false;

  return saveFile(fileName);
}

bool MainWindow::maybeSave() {
  if (gCodeEditor->document()->isModified()) {
    QMessageBox::StandardButton ret = QMessageBox::warning(
        this, tr("NWSS-CNC"),
        tr("The document has been modified.\n"
           "Do you want to save your changes?"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    if (ret == QMessageBox::Save)
      return saveFile();
    else if (ret == QMessageBox::Cancel)
      return false;
  }
  return true;
}

void MainWindow::loadFile(const QString &fileName) {
  QFile file(fileName);
  if (!file.open(QFile::ReadOnly | QFile::Text)) {
    QMessageBox::warning(
        this, tr("NWSS-CNC"),
        tr("Cannot read file %1:\n%2.").arg(fileName).arg(file.errorString()));
    return;
  }

  QTextStream in(&file);
  QString content = in.readAll();

  // Set the content in the editor
  gCodeEditor->setPlainText(content);

  // Wait a moment before updating the preview to ensure the editor has
  // completed its setup
  QTimer::singleShot(100, this, [this]() { updateGCodePreview(); });

  // Reset time estimate when loading a file (since we can't calculate it from
  // GCode text)
  updateTimeEstimateLabel(0);

  setCurrentFile(fileName);
  statusBar()->showMessage(tr("File loaded"), 2000);
}

bool MainWindow::saveFile(const QString &fileName) {
  QFile file(fileName);
  if (!file.open(QFile::WriteOnly | QFile::Text)) {
    QMessageBox::warning(
        this, tr("NWSS-CNC"),
        tr("Cannot write file %1:\n%2.").arg(fileName).arg(file.errorString()));
    return false;
  }

  QTextStream out(&file);
  out << gCodeEditor->toPlainText();

  setCurrentFile(fileName);
  statusBar()->showMessage(tr("File saved"), 2000);
  return true;
}

void MainWindow::setCurrentFile(const QString &fileName) {
  currentFile = fileName;
  isUntitled = fileName.isEmpty();
  gCodeEditor->document()->setModified(false);
  setWindowModified(false);

  QString shownName = isUntitled ? "untitled.gcode" : strippedName(fileName);
  setWindowTitle(tr("%1[*] - %2").arg(shownName).arg(tr("NWSS-CNC")));
}

QString MainWindow::strippedName(const QString &fullFileName) {
  return QFileInfo(fullFileName).fileName();
}

void MainWindow::documentWasModified() {
  setWindowModified(gCodeEditor->document()->isModified());
}

void MainWindow::updateGCodePreview() {
  // Ensure the viewer is properly updated with the current G-code
  QString gcode = gCodeEditor->toPlainText();

  // Use a single-shot timer to ensure the OpenGL context is ready
  QTimer::singleShot(50, this, [this, gcode]() {
    if (gCodeViewer && isVisible()) {
      gCodeViewer->processGCode(gcode);
    }
  });
}

void MainWindow::convertSvgToGCode(const QString &svgFile) {
  if (svgFile.isEmpty()) {
    return;
  }

  // Get the selected tool
  int selectedToolId = toolSelector->getSelectedToolId();
  const nwss::cnc::Tool *selectedTool = toolRegistry->getTool(selectedToolId);

  // Check if no tool is selected and show warning dialog
  if (selectedToolId == 0) {
    QMessageBox warningBox(this);
    warningBox.setIcon(QMessageBox::Warning);
    warningBox.setWindowTitle(tr("No Tool Selected"));
    warningBox.setText(
        tr("You must select a tool before generating G-Code for machining."));
    warningBox.setInformativeText(
        tr("A tool is required to calculate proper cutting parameters, tool "
           "offsets, and feature validation."));

    QPushButton *selectToolButton =
        warningBox.addButton(tr("Select Tool"), QMessageBox::ActionRole);
    QPushButton *continueAnywayButton = warningBox.addButton(
        tr("Continue Without Tool"), QMessageBox::DestructiveRole);
    QPushButton *cancelButton = warningBox.addButton(QMessageBox::Cancel);

    warningBox.setDefaultButton(selectToolButton);

    int result = warningBox.exec();

    if (warningBox.clickedButton() == selectToolButton) {
      // Open tool manager dialog
      showToolManager();

      // Check if a tool was selected after opening the tool manager
      selectedToolId = toolSelector->getSelectedToolId();
      selectedTool = toolRegistry->getTool(selectedToolId);

      if (selectedToolId == 0) {
        // Still no tool selected, abort
        statusBar()->showMessage(
            tr("G-Code generation cancelled - no tool selected."), 3000);
        return;
      }

      // Save the selected tool for future sessions
      QSettings settings("NWSS", "NWSS-CNC");
      settings.setValue("lastSelectedToolId", selectedToolId);
    } else if (warningBox.clickedButton() == continueAnywayButton) {
      // User chose to continue without a tool - show additional warning
      int confirmResult = QMessageBox::warning(
          this, tr("Confirm Continue Without Tool"),
          tr("Continuing without a tool will disable:\n"
             "• Tool offset calculations\n"
             "• Feature size validation\n"
             "• Optimal cutting parameters\n\n"
             "This may result in poor machining results or tool damage.\n\n"
             "Are you sure you want to continue?"),
          QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

      if (confirmResult != QMessageBox::Yes) {
        return;
      }
    } else {
      // User cancelled
      return;
    }
  } else if (!selectedTool) {
    QMessageBox::warning(
        this, tr("Tool Error"),
        tr("Selected tool not found. Please select a valid tool."));
    return;
  } else {
    // Valid tool selected, save it for future sessions
    QSettings settings("NWSS", "NWSS-CNC");
    settings.setValue("lastSelectedToolId", selectedToolId);
  }

  try {
    // Step 1: Parse SVG with the design transformation parameters
    nwss::cnc::SVGParser parser;
    if (!parser.loadFromFile(svgFile.toStdString(), "mm", 96.0f)) {
      QMessageBox::warning(this, tr("SVG Error"),
                           tr("Failed to load SVG file: %1").arg(svgFile));
      return;
    }

    // Step 2: Configure discretizer
    nwss::cnc::DiscretizerConfig discretizerConfig;
    discretizerConfig.bezierSamples = gcodeOptionsPanel->getBezierSamples();
    discretizerConfig.simplifyTolerance =
        gcodeOptionsPanel->getSimplifyTolerance();
    discretizerConfig.adaptiveSampling =
        gcodeOptionsPanel->getAdaptiveSampling();
    discretizerConfig.maxPointDistance =
        gcodeOptionsPanel->getMaxPointDistance();

    // Step 3: Configure CNC parameters
    nwss::cnc::CNConfig config;
    config.setBedWidth(gcodeOptionsPanel->getBedWidth());
    config.setBedHeight(gcodeOptionsPanel->getBedHeight());
    config.setUnitsFromString(gcodeOptionsPanel->isMetricUnits() ? "mm" : "in");
    config.setMaterialWidth(gcodeOptionsPanel->getMaterialWidth());
    config.setMaterialHeight(gcodeOptionsPanel->getMaterialHeight());
    config.setMaterialThickness(gcodeOptionsPanel->getMaterialThickness());
    config.setFeedRate(gcodeOptionsPanel->getFeedRate());
    config.setPlungeRate(gcodeOptionsPanel->getPlungeRate());
    config.setSpindleSpeed(gcodeOptionsPanel->getSpindleSpeed());
    config.setCutDepth(gcodeOptionsPanel->getCutDepth());
    config.setPassCount(gcodeOptionsPanel->getPassCount());
    config.setSafeHeight(gcodeOptionsPanel->getSafetyHeight());

    // Step 4: Discretize paths
    nwss::cnc::Discretizer discretizer;
    discretizer.setConfig(discretizerConfig);
    std::vector<nwss::cnc::Path> paths =
        discretizer.discretizeImage(parser.getRawImage());

    if (paths.empty()) {
      QMessageBox::warning(this, tr("Conversion Error"),
                           tr("No paths found in the SVG file."));
      return;
    }

    // Step 5: Apply transformations based on designer settings
    nwss::cnc::TransformInfo transformInfo;
    transformInfo.success = true;
    transformInfo.wasScaled = false;
    transformInfo.wasCropped = false;
    transformInfo.message = "Using designer transformations";

    // Get design properties from the SVG designer
    QRectF designBounds = svgDesigner->getDesignBounds();
    double designScale = svgDesigner->getDesignScale();
    QPointF designOffset = svgDesigner->getDesignOffset();

    qDebug() << "Converting SVG with design bounds:" << designBounds;
    qDebug() << "Design scale:" << designScale;
    qDebug() << "Design offset:" << designOffset;

    // Check if we have meaningful designer transformations
    bool hasDesignerTransforms =
        (designScale != 1.0 || !designOffset.isNull() ||
         (designBounds != QRectF() && !designBounds.isEmpty()));

    if (hasDesignerTransforms) {
      qDebug() << "Applying designer transformations...";

      // Get original bounds of the paths
      double origMinX, origMinY, origMaxX, origMaxY;
      if (nwss::cnc::Transform::getBounds(paths, origMinX, origMinY, origMaxX,
                                          origMaxY)) {
        double origWidth = origMaxX - origMinX;
        double origHeight = origMaxY - origMinY;

        qDebug() << "Original path bounds:" << origMinX << origMinY << origMaxX
                 << origMaxY;
        qDebug() << "Original dimensions:" << origWidth << "x" << origHeight;

        // The designer transformations work like this:
        // 1. The designBounds represents where the user wants the design to be
        // positioned and sized
        // 2. The designScale represents the uniform scale factor applied to
        // maintain aspect ratio
        // 3. We need to transform the paths to match the designer's intended
        // result

        // Calculate how to transform the original paths to match the design
        // bounds The designer uses uniform scaling, so we need to calculate the
        // actual scale
        double actualScaleX = designBounds.width() / origWidth;
        double actualScaleY = designBounds.height() / origHeight;

        // Use uniform scaling to maintain aspect ratio (like the designer does)
        double uniformScale = qMin(actualScaleX, actualScaleY);

        qDebug() << "Calculated uniform scale:" << uniformScale;
        qDebug() << "Designer reported scale:" << designScale;

        // Apply the uniform scale and position transformation
        for (auto &path : paths) {
          std::vector<nwss::cnc::Point2D> &points =
              const_cast<std::vector<nwss::cnc::Point2D> &>(path.getPoints());
          for (auto &point : points) {
            // Translate to origin
            point.x -= origMinX;
            point.y -= origMinY;

            // Apply uniform scale
            point.x *= uniformScale;
            point.y *= uniformScale;

            // Calculate centered position within design bounds
            double scaledWidth = origWidth * uniformScale;
            double scaledHeight = origHeight * uniformScale;

            // Center the scaled design within the design bounds
            double centerOffsetX = (designBounds.width() - scaledWidth) / 2.0;
            double centerOffsetY = (designBounds.height() - scaledHeight) / 2.0;

            // Apply final position
            point.x += designBounds.left() + centerOffsetX;
            point.y += designBounds.top() + centerOffsetY;
          }
        }

        // Update transform info with correct values
        transformInfo.origWidth = origWidth;
        transformInfo.origHeight = origHeight;
        transformInfo.origMinX = origMinX;
        transformInfo.origMinY = origMinY;
        transformInfo.newWidth = origWidth * uniformScale;
        transformInfo.newHeight = origHeight * uniformScale;
        transformInfo.newMinX =
            designBounds.left() +
            (designBounds.width() - transformInfo.newWidth) / 2.0;
        transformInfo.newMinY =
            designBounds.top() +
            (designBounds.height() - transformInfo.newHeight) / 2.0;
        transformInfo.scaleX = transformInfo.scaleY = uniformScale;
        transformInfo.offsetX = transformInfo.newMinX;
        transformInfo.offsetY = transformInfo.newMinY;
        transformInfo.wasScaled = (uniformScale != 1.0);

        qDebug() << "Applied uniform scale:" << uniformScale;
        qDebug() << "Final design position:" << transformInfo.newMinX
                 << transformInfo.newMinY;
        qDebug() << "Final design size:" << transformInfo.newWidth << "x"
                 << transformInfo.newHeight;

        // Check if design exceeds material or bed bounds (using SVG designer's
        // format)
        double materialWidth = config.getMaterialWidth();
        double materialHeight = config.getMaterialHeight();
        double bedWidth = config.getBedWidth();
        double bedHeight = config.getBedHeight();

        bool fitsInMaterial = true;
        QStringList warnings;

        // Check if design extends beyond material boundaries (same as SVG
        // designer)
        if (designBounds.left() < 0) {
          warnings
              << tr("Design extends %1 mm beyond the left edge of the material")
                     .arg(-designBounds.left(), 0, 'f', 2);
          fitsInMaterial = false;
        }
        if (designBounds.top() < 0) {
          warnings
              << tr("Design extends %1 mm beyond the top edge of the material")
                     .arg(-designBounds.top(), 0, 'f', 2);
          fitsInMaterial = false;
        }
        if (designBounds.right() > materialWidth) {
          warnings << tr("Design extends %1 mm beyond the right edge of the "
                         "material")
                          .arg(designBounds.right() - materialWidth, 0, 'f', 2);
          fitsInMaterial = false;
        }
        if (designBounds.bottom() > materialHeight) {
          warnings << tr("Design extends %1 mm beyond the bottom edge of the "
                         "material")
                          .arg(designBounds.bottom() - materialHeight, 0, 'f',
                               2);
          fitsInMaterial = false;
        }

        // Also check bed dimensions
        if (designBounds.width() > bedWidth) {
          warnings << tr("Design width (%1 mm) exceeds bed width (%2 mm)")
                          .arg(designBounds.width(), 0, 'f', 2)
                          .arg(bedWidth, 0, 'f', 2);
          fitsInMaterial = false;
        }
        if (designBounds.height() > bedHeight) {
          warnings << tr("Design height (%1 mm) exceeds bed height (%2 mm)")
                          .arg(designBounds.height(), 0, 'f', 2)
                          .arg(bedHeight, 0, 'f', 2);
          fitsInMaterial = false;
        }

        // Show warning dialog if design doesn't fit (same format as SVG
        // designer)
        if (!fitsInMaterial) {
          QMessageBox warningBox(this);
          warningBox.setIcon(QMessageBox::Warning);
          warningBox.setWindowTitle(tr("Design Size Warning"));
          warningBox.setText(
              tr("The design does not fit within the material boundaries."));

          QString detailText = tr("Material size: %1 x %2 mm\n")
                                   .arg(materialWidth, 0, 'f', 1)
                                   .arg(materialHeight, 0, 'f', 1);
          detailText += tr("Design size: %1 x %2 mm\n")
                            .arg(designBounds.width(), 0, 'f', 1)
                            .arg(designBounds.height(), 0, 'f', 1);
          detailText += tr("Design position: (%1, %2) mm\n")
                            .arg(designBounds.left(), 0, 'f', 1)
                            .arg(designBounds.top(), 0, 'f', 1);
          detailText += tr("Bed size: %1 x %2 mm\n\n")
                            .arg(bedWidth, 0, 'f', 1)
                            .arg(bedHeight, 0, 'f', 1);
          detailText += tr("Issues found:\n• %1").arg(warnings.join("\n• "));

          warningBox.setDetailedText(detailText);
          warningBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
          warningBox.setDefaultButton(QMessageBox::Cancel);
          warningBox.button(QMessageBox::Yes)->setText(tr("Continue Anyway"));
          warningBox.button(QMessageBox::Cancel)->setText(tr("Cancel"));

          int result = warningBox.exec();
          if (result != QMessageBox::Yes) {
            return;  // User cancelled
          }

          // User chose to continue, add warnings to transform info
          transformInfo.message += " - WARNING: Design exceeds bounds!";
          transformInfo.wasCropped = true;
        }

        // Apply Y-flip if requested (after designer transformations)
        if (gcodeOptionsPanel->getFlipY()) {
          for (auto &path : paths) {
            std::vector<nwss::cnc::Point2D> &points =
                const_cast<std::vector<nwss::cnc::Point2D> &>(path.getPoints());
            for (auto &point : points) {
              point.y = materialHeight - point.y;
            }
          }
          transformInfo.message += " (Y-flipped)";
        }
      }
    } else {
      // No meaningful designer transformations, use standard material fitting
      qDebug() << "No designer transformations found, using standard material "
                  "fitting...";
      if (!nwss::cnc::Transform::fitToMaterial(
              paths, config, gcodeOptionsPanel->getPreserveAspectRatio(),
              gcodeOptionsPanel->getCenterX(), gcodeOptionsPanel->getCenterY(),
              gcodeOptionsPanel->getFlipY(), &transformInfo)) {
        QMessageBox::warning(this, tr("Transform Error"),
                             tr("Failed to fit paths to material."));
        return;
      }
    }

    qDebug() << "Transform Info:" << transformInfo.message.c_str();

    // Step 6: Setup G-code generator with tool integration
    nwss::cnc::GCodeGenerator generator;
    generator.setConfig(config);
    generator.setToolRegistry(*toolRegistry);

    nwss::cnc::GCodeOptions gCodeOptions;
    gCodeOptions.selectedToolId = selectedToolId;
    gCodeOptions.enableToolOffsets = (selectedTool != nullptr);
    gCodeOptions.validateFeatureSizes = (selectedTool != nullptr);
    gCodeOptions.offsetDirection = nwss::cnc::ToolOffsetDirection::AUTO;
    gCodeOptions.materialType = "Unknown";  // Could be made configurable
    gCodeOptions.optimizePaths = gcodeOptionsPanel->getOptimizePaths();
    gCodeOptions.linearizePaths = gcodeOptionsPanel->getLinearizePaths();
    gCodeOptions.linearizeTolerance = 0.01;
    gCodeOptions.includeComments = false;
    gCodeOptions.includeHeader = true;
    gCodeOptions.returnToOrigin = true;

    // Set cutout mode parameters
    gCodeOptions.cutoutMode =
        static_cast<nwss::cnc::CutoutMode>(gcodeOptionsPanel->getCutoutMode());
    gCodeOptions.stepover = gcodeOptionsPanel->getStepover();
    gCodeOptions.maxStepover = gcodeOptionsPanel->getMaxStepover();
    gCodeOptions.spiralIn = gcodeOptionsPanel->getSpiralIn();

    generator.setOptions(gCodeOptions);

    // Step 7: Validate tool if selected (show warnings but don't block)
    if (selectedTool) {
      std::vector<std::string> warnings;
      int maxWarnings = 10;
      int warningCount = 0;
      if (!generator.validatePaths(paths, warnings)) {
        QString warningText =
            tr("Some features may be too small for the selected tool:\n");
        for (const auto &warning : warnings) {
          warningText += QString("• %1\n").arg(QString::fromStdString(warning));
          warningCount++;
          if (warningCount >= maxWarnings) {
            warningText += tr("\n...and more warnings.");
            break;
          }
        }
        warningText += tr("\nContinue with G-code generation?");

        int result = QMessageBox::question(
            this, tr("Tool Validation Warning"), warningText,
            QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        if (result != QMessageBox::Yes) {
          return;
        }
      }
    }

    // Step 8: Generate G-code
    std::string gCodeStd = generator.generateGCodeString(paths);
    QString gCode = QString::fromStdString(gCodeStd);

    if (gCode.isEmpty()) {
      QMessageBox::warning(this, tr("G-Code Generation Error"),
                           tr("Failed to generate G-code from the SVG file."));
      return;
    }

    // Step 9: Display the generated G-code
    gCodeEditor->setPlainText(gCode);
    setCurrentFile("");

    // Calculate time estimate
    nwss::cnc::GCodeGenerator::TimeEstimate estimate =
        generator.calculateTimeEstimate(paths);

    // Update the time estimate label
    updateTimeEstimateLabel(estimate.totalTime);

    statusBar()->showMessage("Generated successfully.");
  } catch (const std::exception &e) {
    QMessageBox::critical(
        this, tr("Conversion Error"),
        tr("An error occurred during conversion: %1").arg(e.what()));
  }

  // Update the viewers with the new G-code
  updateGCodePreview();
  // Change view to 3D preview
  tabWidget->setCurrentIndex(1);
}

// Note: The overloaded convertSvgToGCode method has been removed.
// All G-code generation now goes through the unified convertSvgToGCode(const
// QString &svgFile) method which gets design properties directly from the SVG
// designer.

void MainWindow::onTabChanged(int index) {
  // Update the UI based on the selected tab
  if (index == 0) {
    // GCode Editor tab
    cutAct->setEnabled(true);
    copyAct->setEnabled(true);
    pasteAct->setEnabled(true);
  } else {
    // Other tabs - disable edit actions
    cutAct->setEnabled(false);
    copyAct->setEnabled(false);
    pasteAct->setEnabled(false);
  }

  // If switching to 3D preview, update it
  if (index == 1) {
    updateGCodePreview();
  }
}

void MainWindow::onGCodeChanged() {
  // Called when the G-Code editor content changes
  documentWasModified();

  // Reset time estimate when GCode is manually edited (since we can't
  // recalculate from GCode text)
  updateTimeEstimateLabel(0);

  // Update the 3D preview if it's currently visible
  if (tabWidget->currentIndex() == 1) {  // 3D Preview tab
    updateGCodePreview();
  }
}

void MainWindow::closeEvent(QCloseEvent *event) {
  if (maybeSave()) {
    writeSettings();
    gcodeOptionsPanel->saveSettings();  // Save GCode options
    event->accept();
  } else {
    event->ignore();
  }
}

void MainWindow::about() {
  QMessageBox::about(
      this, tr("About NWSS-CNC"),
      tr("<h3>NWSS-CNC</h3>"
         "<p>Version 1.0</p>"
         "<p>A modern CNC control application with G-Code editing, "
         "3D previewing, and SVG import capabilities.</p>"));
}

void MainWindow::showToolManager() {
  nwss::cnc::ToolManager toolManager(*toolRegistry, this);

  // Connect the tool registry changed signal to refresh our tool selector
  connect(&toolManager, &nwss::cnc::ToolManager::toolRegistryChanged,
          toolSelector, &nwss::cnc::ToolSelector::refreshTools);

  // Connect to our own handler for registry changes
  connect(&toolManager, &nwss::cnc::ToolManager::toolRegistryChanged, this,
          &MainWindow::onToolRegistryChanged);

  // If a tool is selected in the manager dialog, update our selector and save
  // it
  connect(&toolManager, &nwss::cnc::ToolManager::toolSelected, this,
          [this](int toolId) {
            toolSelector->setSelectedTool(toolId);

            // Save the selected tool to settings
            QSettings settings("NWSS", "NWSS-CNC");
            settings.setValue("lastSelectedToolId", toolId);

            // Update status bar
            const nwss::cnc::Tool *tool = toolRegistry->getTool(toolId);
            if (tool) {
              statusBar()->showMessage(
                  tr("Selected tool: %1 (diameter: %2mm)")
                      .arg(QString::fromStdString(tool->name))
                      .arg(tool->diameter),
                  3000);
            }
          });

  toolManager.exec();
}

void MainWindow::onToolSelected(int toolId) {
  // Get the selected tool information
  const nwss::cnc::Tool *tool = toolRegistry->getTool(toolId);
  if (tool) {
    statusBar()->showMessage(tr("Selected tool: %1 (diameter: %2mm)")
                                 .arg(QString::fromStdString(tool->name))
                                 .arg(tool->diameter),
                             3000);

    // Save the selected tool to settings for persistence
    QSettings settings("NWSS", "NWSS-CNC");
    settings.setValue("lastSelectedToolId", toolId);

    // TODO: Update G-code generation options with selected tool
    // This could be integrated with the GCodeOptionsPanel
  } else if (toolId == 0) {
    statusBar()->showMessage(tr("No tool selected"), 2000);

    // Clear the saved tool selection
    QSettings settings("NWSS", "NWSS-CNC");
    settings.setValue("lastSelectedToolId", 0);
  }
}

void MainWindow::onToolRegistryChanged() {
  // The tool registry has been modified, refresh the tool selector
  toolSelector->refreshTools();
}

void MainWindow::updateTimeEstimateLabel(double totalTimeSeconds) {
  if (totalTimeSeconds <= 0) {
    timeEstimateLabel->setText(tr("Est. time: --:--:--"));
    return;
  }

  int hours = static_cast<int>(totalTimeSeconds / 3600);
  int minutes = static_cast<int>((totalTimeSeconds - hours * 3600) / 60);
  int seconds = static_cast<int>(totalTimeSeconds) % 60;

  QString timeString;
  if (hours > 0) {
    timeString = QString("%1:%2:%3")
                     .arg(hours, 2, 10, QChar('0'))
                     .arg(minutes, 2, 10, QChar('0'))
                     .arg(seconds, 2, 10, QChar('0'));
  } else {
    timeString = QString("%1:%2")
                     .arg(minutes, 2, 10, QChar('0'))
                     .arg(seconds, 2, 10, QChar('0'));
  }

  timeEstimateLabel->setText(tr("Est. time: %1").arg(timeString));
}