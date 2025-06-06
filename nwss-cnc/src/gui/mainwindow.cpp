// mainwindow.cpp (updated)
#include "mainwindow.h"
#include "gcodeoptionspanel.h"
#include <QApplication>
#include <QSettings>
#include <QCloseEvent>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QStatusBar>
#include <QToolBar>
#include <QMenuBar>
#include <QTimer>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QGridLayout>
#include <QDialogButtonBox>
#include <QDialog>
#include <QCheckBox>
#include <QFrame>
#include <QSet>
#include "svg_parser.h"
#include "discretizer.h"
#include "config.h"
#include "transform.h"
#include "gcode_generator.h"

class WelcomeDialog : public QDialog
{
public:
    WelcomeDialog(QWidget *parent = nullptr) : QDialog(parent)
    {
        setWindowTitle(tr("Welcome to NWSS-CNC"));
        resize(500, 300);
        
        QGridLayout *layout = new QGridLayout(this);
        
        QLabel *titleLabel = new QLabel(tr("<h1>Welcome to NWSS-CNC</h1>"));
        titleLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(titleLabel, 0, 0, 1, 2);
        
        QLabel *subtitleLabel = new QLabel(tr("Choose how you want to get started:"));
        subtitleLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(subtitleLabel, 1, 0, 1, 2);
        
        // Create option buttons with icons
        QPushButton *newFileButton = new QPushButton(tr("New Empty Project"));
        newFileButton->setMinimumHeight(100);
        connect(newFileButton, &QPushButton::clicked, this, [this]() {
            done(1); // New file
        });
        
        QPushButton *openGCodeButton = new QPushButton(tr("Open G-Code File"));
        openGCodeButton->setMinimumHeight(100);
        connect(openGCodeButton, &QPushButton::clicked, this, [this]() {
            done(2); // Open GCode
        });
        
        QPushButton *importSvgButton = new QPushButton(tr("Import SVG File"));
        importSvgButton->setMinimumHeight(100);
        connect(importSvgButton, &QPushButton::clicked, this, [this]() {
            done(3); // Import SVG
        });
        
        // Add buttons to layout
        layout->addWidget(newFileButton, 2, 0);
        layout->addWidget(openGCodeButton, 2, 1);
        layout->addWidget(importSvgButton, 3, 0, 1, 2);
        
        // Add checkbox to not show this dialog again
        QCheckBox *dontShowAgainCheckbox = new QCheckBox(tr("Don't show this dialog on startup"));
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
    : QMainWindow(parent), isUntitled(true)
{
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
    connect(gCodeEditor->document(), &QTextDocument::contentsChanged,
            this, &MainWindow::documentWasModified);
    connect(gCodeEditor, &GCodeEditor::textChanged,
            this, &MainWindow::updateGCodePreview);
    connect(tabWidget, &QTabWidget::currentChanged,
            this, &MainWindow::onTabChanged);
    connect(gcodeOptionsPanel, &GCodeOptionsPanel::generateGCode,
            this, &MainWindow::convertSvgToGCode);
    
    // Update these connections for svgDesigner
    connect(svgDesigner, &SVGDesigner::convertToGCode,
            this, &MainWindow::convertSvgToGCode);
    connect(svgDesigner, &SVGDesigner::svgLoaded, 
        [this](const QString& filePath) {
            gcodeOptionsPanel->setCurrentSvgFile(filePath);
        });
            
    // Connect material size updates to designer
    connect(gcodeOptionsPanel, &GCodeOptionsPanel::settingsLoaded,
        [this]() {
            // Update material and bed size in the designer
            svgDesigner->setMaterialSize(
                gcodeOptionsPanel->getMaterialWidth(),
                gcodeOptionsPanel->getMaterialHeight());
            
            svgDesigner->setBedSize(
                gcodeOptionsPanel->getBedWidth(),
                gcodeOptionsPanel->getBedHeight());
        });
    
    // Update designer when material settings change
    connect(gcodeOptionsPanel, &GCodeOptionsPanel::optionsChanged,
        [this]() {
            svgDesigner->setMaterialSize(
                gcodeOptionsPanel->getMaterialWidth(),
                gcodeOptionsPanel->getMaterialHeight());
            
            svgDesigner->setBedSize(
                gcodeOptionsPanel->getBedWidth(),
                gcodeOptionsPanel->getBedHeight());
        });
    
    // Connect tool management signals
    connect(toolSelector, &nwss::cnc::ToolSelector::toolSelected,
            this, &MainWindow::onToolSelected);

    // Initial state
    setCurrentFile("");
    readSettings();
    
    // Show welcome dialog (if not disabled)
    QSettings settings("NWSS", "NWSS-CNC");
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

MainWindow::~MainWindow()
{
    delete toolRegistry;
}

void MainWindow::setupTabWidget()
{
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

void MainWindow::createActions()
{
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
    cutAct->setStatusTip(tr("Cut the current selection's contents to the clipboard"));
    connect(cutAct, &QAction::triggered, gCodeEditor, &GCodeEditor::cut);

    copyAct = new QAction(tr("&Copy"), this);
    copyAct->setShortcuts(QKeySequence::Copy);
    copyAct->setStatusTip(tr("Copy the current selection's contents to the clipboard"));
    connect(copyAct, &QAction::triggered, gCodeEditor, &GCodeEditor::copy);

    pasteAct = new QAction(tr("&Paste"), this);
    pasteAct->setShortcuts(QKeySequence::Paste);
    pasteAct->setStatusTip(tr("Paste the clipboard's contents into the current selection"));
    connect(pasteAct, &QAction::triggered, gCodeEditor, &GCodeEditor::paste);
    
    showGCodeOptionsPanelAct = new QAction(tr("Show G-Code Options Panel"), this);
    showGCodeOptionsPanelAct->setCheckable(true);
    showGCodeOptionsPanelAct->setChecked(true);
    
    showToolPanelAct = new QAction(tr("Show Tool Panel"), this);
    showToolPanelAct->setCheckable(true);
    showToolPanelAct->setChecked(true);
    
    // Tools menu actions
    manageToolsAct = new QAction(tr("&Manage Tools..."), this);
    manageToolsAct->setStatusTip(tr("Open the tool management dialog"));
    connect(manageToolsAct, &QAction::triggered, this, &MainWindow::showToolManager);

    // Help menu actions
    aboutAct = new QAction(tr("&About"), this);
    aboutAct->setStatusTip(tr("Show the application's About box"));
    connect(aboutAct, &QAction::triggered, this, &MainWindow::about);

    aboutQtAct = new QAction(tr("About &Qt"), this);
    aboutQtAct->setStatusTip(tr("Show the Qt library's About box"));
    connect(aboutQtAct, &QAction::triggered, qApp, &QApplication::aboutQt);
}

void MainWindow::createMenus()
{
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
    viewMenu->addAction(showGCodeOptionsPanelAct);
    viewMenu->addAction(showToolPanelAct);
    
    toolsMenu = menuBar()->addMenu(tr("&Tools"));
    toolsMenu->addAction(manageToolsAct);

    menuBar()->addSeparator();

    helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(aboutAct);
    helpMenu->addAction(aboutQtAct);
}

void MainWindow::createToolBars()
{
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
    connect(tabWidget, &QTabWidget::currentChanged, tabComboBox, &QComboBox::setCurrentIndex);
}

void MainWindow::createDockPanels()
{   
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
    
    // Connect the show/hide actions
    connect(showGCodeOptionsPanelAct, &QAction::toggled, gcodeOptionsDock, &QDockWidget::setVisible);
    connect(gcodeOptionsDock, &QDockWidget::visibilityChanged, showGCodeOptionsPanelAct, &QAction::setChecked);
    
    connect(showToolPanelAct, &QAction::toggled, toolDock, &QDockWidget::setVisible);
    connect(toolDock, &QDockWidget::visibilityChanged, showToolPanelAct, &QAction::setChecked);
}

void MainWindow::createStatusBar()
{
    // Create time estimate label
    timeEstimateLabel = new QLabel(tr("Est. time: --:--:--"));
    timeEstimateLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    timeEstimateLabel->setContentsMargins(5, 0, 5, 0);
    timeEstimateLabel->setMinimumWidth(150);
    
    statusBar()->addPermanentWidget(timeEstimateLabel);
    statusBar()->showMessage(tr("Ready"));
}

void MainWindow::readSettings()
{
    QSettings settings("NWSS", "NWSS-CNC");
    QPoint pos = settings.value("pos", QPoint(200, 200)).toPoint();
    QSize size = settings.value("size", QSize(1200, 800)).toSize();
    move(pos);
    resize(size);
    
    // Restore dock widget states
    bool gcodeOptionsPanelVisible = settings.value("gcodeOptionsPanelVisible", true).toBool();
    bool toolPanelVisible = settings.value("toolPanelVisible", true).toBool();
    
    gcodeOptionsDock->setVisible(gcodeOptionsPanelVisible);
    toolDock->setVisible(toolPanelVisible);
    
    // Restore tab index
    int tabIndex = settings.value("activeTabIndex", 0).toInt();
    tabWidget->setCurrentIndex(tabIndex);
}

void MainWindow::writeSettings()
{
    QSettings settings("NWSS", "NWSS-CNC");
    settings.setValue("pos", pos());
    settings.setValue("size", size());
    
    // Save dock widget states
    settings.setValue("gcodeOptionsPanelVisible", gcodeOptionsDock->isVisible());
    settings.setValue("toolPanelVisible", toolDock->isVisible());
    
    // Save active tab
    settings.setValue("activeTabIndex", tabWidget->currentIndex());
}

void MainWindow::showWelcomeDialog()
{
    WelcomeDialog dialog(this);
    int result = dialog.exec();
    
    switch (result) {
    case 1: // New file
        newFile();
        break;
    case 2: // Open GCode
        openFile();
        break;
    case 3: // Import SVG
        importSvgFile();
        break;
    default:
        // User closed the dialog
        break;
    }
}

void MainWindow::newFile()
{
    if (maybeSave()) {
        gCodeEditor->clear();
        setCurrentFile("");
        tabWidget->setCurrentIndex(0); // Switch to editor tab
    }
}

void MainWindow::openFile()
{
    if (maybeSave()) {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Open GCode File"), "",
                                                       tr("GCode Files (*.gcode *.nc *.ngc);;All Files (*)"));
        if (!fileName.isEmpty()) {
            loadFile(fileName);
            tabWidget->setCurrentIndex(0); // Switch to editor tab
        }
    }
}

void MainWindow::importSvgFile()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Import SVG File"), "",
                                                   tr("SVG Files (*.svg);;All Files (*)"));
    if (!fileName.isEmpty()) {
        svgDesigner->loadSvgFile(fileName);  // Changed from svgViewer
        tabWidget->setCurrentIndex(2);
    }
}

bool MainWindow::saveFile()
{
    if (isUntitled) {
        return saveAsFile();
    } else {
        return saveFile(currentFile);
    }
}

bool MainWindow::saveAsFile()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save GCode File"), "",
                                                  tr("GCode Files (*.gcode *.nc *.ngc);;All Files (*)"));
    if (fileName.isEmpty())
        return false;

    return saveFile(fileName);
}

bool MainWindow::maybeSave()
{
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

void MainWindow::loadFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("NWSS-CNC"),
                             tr("Cannot read file %1:\n%2.")
                             .arg(fileName)
                             .arg(file.errorString()));
        return;
    }

    QTextStream in(&file);
    QString content = in.readAll();
    
    // Set the content in the editor
    gCodeEditor->setPlainText(content);

    // Wait a moment before updating the preview to ensure the editor has completed its setup
    QTimer::singleShot(100, this, [this]() {
        updateGCodePreview();
    });

    setCurrentFile(fileName);
    statusBar()->showMessage(tr("File loaded"), 2000);
}

bool MainWindow::saveFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("NWSS-CNC"),
                             tr("Cannot write file %1:\n%2.")
                             .arg(fileName)
                             .arg(file.errorString()));
        return false;
    }

    QTextStream out(&file);
    out << gCodeEditor->toPlainText();

    setCurrentFile(fileName);
    statusBar()->showMessage(tr("File saved"), 2000);
    return true;
}

void MainWindow::setCurrentFile(const QString &fileName)
{
    currentFile = fileName;
    isUntitled = fileName.isEmpty();
    gCodeEditor->document()->setModified(false);
    setWindowModified(false);

    QString shownName = isUntitled ? "untitled.gcode" : strippedName(fileName);
    setWindowTitle(tr("%1[*] - %2").arg(shownName).arg(tr("NWSS-CNC")));
}

QString MainWindow::strippedName(const QString &fullFileName)
{
    return QFileInfo(fullFileName).fileName();
}

void MainWindow::documentWasModified()
{
    setWindowModified(gCodeEditor->document()->isModified());
}

void MainWindow::updateGCodePreview()
{
    // Ensure the viewer is properly updated with the current G-code
    QString gcode = gCodeEditor->toPlainText();
    
    // Use a single-shot timer to ensure the OpenGL context is ready
    QTimer::singleShot(50, this, [this, gcode]() {
        if (gCodeViewer && isVisible()) {
            gCodeViewer->processGCode(gcode);
        }
    });
}

void MainWindow::convertSvgToGCode(const QString &svgFile)
{
    if (svgFile.isEmpty()) {
        return;
    }
    
    // Get the selected tool
    int selectedToolId = toolSelector->getSelectedToolId();
    const nwss::cnc::Tool* selectedTool = toolRegistry->getTool(selectedToolId);
    
    if (!selectedTool && selectedToolId != 0) {
        QMessageBox::warning(this, tr("Tool Error"),
                            tr("Selected tool not found. Please select a valid tool."));
        return;
    }
    
    try {
        // Step 1: Parse SVG
        nwss::cnc::SVGParser parser;
        if (!parser.loadFromFile(svgFile.toStdString(), "mm", 96.0f)) {
            QMessageBox::warning(this, tr("SVG Error"),
                                tr("Failed to load SVG file."));
            return;
        }
        
        // Step 2: Setup discretizer
        nwss::cnc::Discretizer discretizer;
        nwss::cnc::DiscretizerConfig discretizerConfig;
        discretizerConfig.bezierSamples = gcodeOptionsPanel->getBezierSamples();
        discretizerConfig.simplifyTolerance = gcodeOptionsPanel->getSimplifyTolerance();
        discretizerConfig.adaptiveSampling = gcodeOptionsPanel->getAdaptiveSampling();
        discretizerConfig.maxPointDistance = gcodeOptionsPanel->getMaxPointDistance();
        discretizer.setConfig(discretizerConfig);
        
        // Step 3: Discretize paths
        std::vector<nwss::cnc::Path> paths = discretizer.discretizeImage(parser.getRawImage());
        
                 // Step 4: Setup CNC configuration
         nwss::cnc::CNConfig config;
        config.setBedWidth(gcodeOptionsPanel->getBedWidth());
        config.setBedHeight(gcodeOptionsPanel->getBedHeight());
        config.setUnitsFromString("mm");
        config.setMaterialWidth(gcodeOptionsPanel->getMaterialWidth());
        config.setMaterialHeight(gcodeOptionsPanel->getMaterialHeight());
        config.setMaterialThickness(gcodeOptionsPanel->getMaterialThickness());
        config.setFeedRate(gcodeOptionsPanel->getFeedRate());
        config.setPlungeRate(gcodeOptionsPanel->getPlungeRate());
        config.setSpindleSpeed(gcodeOptionsPanel->getSpindleSpeed());
        config.setCutDepth(gcodeOptionsPanel->getCutDepth());
        config.setPassCount(gcodeOptionsPanel->getPassCount());
        config.setSafeHeight(gcodeOptionsPanel->getSafetyHeight());
        
        // Step 5: Transform paths to fit material
        nwss::cnc::TransformInfo transformInfo;
        if (!nwss::cnc::Transform::fitToMaterial(paths, config, 
                                                gcodeOptionsPanel->getPreserveAspectRatio(),
                                                gcodeOptionsPanel->getCenterX(),
                                                gcodeOptionsPanel->getCenterX(),  // Use same for Y
                                                gcodeOptionsPanel->getFlipY(),
                                                &transformInfo)) {
            QMessageBox::warning(this, tr("Transform Error"),
                                tr("Failed to fit paths to material."));
            return;
        }
        
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
        gCodeOptions.includeComments = true;
        gCodeOptions.includeHeader = true;
        gCodeOptions.returnToOrigin = true;
        
        generator.setOptions(gCodeOptions);
        
        // Step 7: Validate tool if selected (show warnings but don't block)
        if (selectedTool) {
            std::vector<std::string> warnings;
            if (!generator.validatePaths(paths, warnings)) {
                // Summarize warnings instead of listing every single one
                QString warningMsg = tr("Tool Validation Summary:\n\n");
                warningMsg += tr("• Tool diameter: %1mm\n").arg(selectedTool->diameter);
                warningMsg += tr("• Number of paths with issues: %1 out of %2\n").arg(warnings.size()).arg(paths.size());
                
                // Show only the first few unique warning types
                QSet<QString> uniqueWarnings;
                for (const auto& warning : warnings) {
                    QString warningStr = QString::fromStdString(warning);
                    // Extract the warning type (remove path-specific details)
                    if (warningStr.contains("Feature too small")) {
                        uniqueWarnings.insert("Features are smaller than tool diameter");
                    } else if (warningStr.contains("radius")) {
                        uniqueWarnings.insert("Radius constraints violated");
                    } else {
                        uniqueWarnings.insert(warningStr);
                    }
                    
                    // Limit to avoid huge dialogs
                    if (uniqueWarnings.size() >= 3) break;
                }
                
                warningMsg += tr("\nIssues found:\n");
                for (const QString& uniqueWarning : uniqueWarnings) {
                    warningMsg += QString("• %1\n").arg(uniqueWarning);
                }
                
                warningMsg += tr("\nRecommendation: Consider using a smaller tool (like 1/16\" or 1/32\" for fine details).");
                warningMsg += tr("\n\nG-code will still be generated, but may not produce optimal results.");
                
                // Create a properly sized message box
                QMessageBox msgBox(this);
                msgBox.setWindowTitle(tr("Tool Validation Notice"));
                msgBox.setText(warningMsg);
                msgBox.setIcon(QMessageBox::Warning);
                msgBox.setStandardButtons(QMessageBox::Ok);
                msgBox.setDefaultButton(QMessageBox::Ok);
                
                // Ensure dialog is reasonably sized and scrollable if needed
                msgBox.setDetailedText(tr("Click 'Show Details' to see all %1 specific warnings if needed.").arg(warnings.size()));
                
                msgBox.exec();
                
                // Also add a warning to the status bar
                statusBar()->showMessage(tr("Warning: Tool may be too large for %1 features").arg(warnings.size()), 5000);
            }
        }
        
        // Step 8: Generate G-code
        std::string gcode = generator.generateGCodeString(paths);
        
        if (gcode.empty()) {
            QMessageBox::warning(this, tr("G-code Error"),
                                tr("Failed to generate G-code."));
            return;
        }
        
        // Step 9: Calculate time estimate
        auto timeEstimate = generator.calculateTimeEstimate(paths);
        
        // Format time as HH:MM:SS
        int totalSeconds = static_cast<int>(timeEstimate.totalTime);
        int hours = totalSeconds / 3600;
        int minutes = (totalSeconds % 3600) / 60;
        int seconds = totalSeconds % 60;
        
        QString timeString = QString("%1:%2:%3")
                              .arg(hours, 2, 10, QChar('0'))
                              .arg(minutes, 2, 10, QChar('0'))
                              .arg(seconds, 2, 10, QChar('0'));
        
        // Update time estimate label
        timeEstimateLabel->setText(tr("Est. time: %1").arg(timeString));
        
        // Update the GCode editor with the generated code
        gCodeEditor->setPlainText(QString::fromStdString(gcode));
        
        // Switch to the 3d preview tab
        tabWidget->setCurrentIndex(1);
        
        // Mark as untitled so user can save it
        setCurrentFile("");
        
        QString successMsg = tr("SVG converted to GCode");
        if (selectedTool) {
            successMsg += tr(" using %1").arg(QString::fromStdString(selectedTool->name));
        }
        statusBar()->showMessage(successMsg, 3000);
        
    } catch (const std::exception& e) {
        QMessageBox::critical(this, tr("Conversion Error"),
                            tr("An error occurred during conversion: %1")
                            .arg(QString::fromStdString(e.what())));
    }
}

void MainWindow::onTabChanged(int index)
{
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

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (maybeSave()) {
        writeSettings();
        gcodeOptionsPanel->saveSettings();  // Save GCode options
        event->accept();
    } else {
        event->ignore();
    }
}

void MainWindow::about()
{
    QMessageBox::about(this, tr("About NWSS-CNC"),
             tr("<h3>NWSS-CNC</h3>"
                "<p>Version 1.0</p>"
                "<p>A modern CNC control application with G-Code editing, "
                "3D previewing, and SVG import capabilities.</p>"));
}

void MainWindow::showToolManager()
{
    nwss::cnc::ToolManager toolManager(*toolRegistry, this);
    
    // Connect the tool registry changed signal to refresh our tool selector
    connect(&toolManager, &nwss::cnc::ToolManager::toolRegistryChanged,
            toolSelector, &nwss::cnc::ToolSelector::refreshTools);
    
    // Connect to our own handler for registry changes
    connect(&toolManager, &nwss::cnc::ToolManager::toolRegistryChanged,
            this, &MainWindow::onToolRegistryChanged);
    
    // If a tool is selected in the manager dialog, update our selector
    connect(&toolManager, &nwss::cnc::ToolManager::toolSelected,
            this, [this](int toolId) {
                toolSelector->setSelectedTool(toolId);
            });
    
    toolManager.exec();
}

void MainWindow::onToolSelected(int toolId)
{
    // Get the selected tool information
    const nwss::cnc::Tool* tool = toolRegistry->getTool(toolId);
    if (tool) {
        statusBar()->showMessage(tr("Selected tool: %1 (diameter: %2mm)")
                                .arg(QString::fromStdString(tool->name))
                                .arg(tool->diameter), 3000);
        
        // TODO: Update G-code generation options with selected tool
        // This could be integrated with the GCodeOptionsPanel
    } else if (toolId == 0) {
        statusBar()->showMessage(tr("No tool selected"), 2000);
    }
}

void MainWindow::onToolRegistryChanged()
{
    // The tool registry has been modified, refresh the tool selector
    toolSelector->refreshTools();
}