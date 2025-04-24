// mainwindow.cpp (updated)
#include "mainwindow.h"
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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), isUntitled(true)
{
    // Create all the components
    gCodeEditor = new GCodeEditor(this);
    gCodeViewer = new GCodeViewer3D(this);
    svgViewer = new SVGViewer(this);
    gcodeOptionsPanel = new GCodeOptionsPanel(this);
    svgToGCode = new SvgToGCode(this);

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
    connect(svgViewer, &SVGViewer::convertToGCode,
            this, &MainWindow::convertSvgToGCode);

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
    // Any necessary cleanup
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
    tabWidget->addTab(svgViewer, tr("SVG Viewer"));
    
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
    
    toolsMenu = menuBar()->addMenu(tr("&Tools"));
    // Add tools menu actions later

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
    viewToolBar->addWidget(tabLabel);
    
    QComboBox *tabComboBox = new QComboBox();
    tabComboBox->addItem(tr("G-Code Editor"));
    tabComboBox->addItem(tr("3D Preview"));
    tabComboBox->addItem(tr("SVG Viewer"));
    connect(tabComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            tabWidget, &QTabWidget::setCurrentIndex);
    viewToolBar->addWidget(tabComboBox);
    
    // Connect tab changes to update the combo box
    connect(tabWidget, &QTabWidget::currentChanged, tabComboBox, &QComboBox::setCurrentIndex);
}

void MainWindow::createDockPanels()
{   
    // G-Code options panel dock
    gcodeOptionsDock = new QDockWidget(tr("G-Code Options"), this);
    gcodeOptionsDock->setWidget(gcodeOptionsPanel);
    gcodeOptionsDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    addDockWidget(Qt::LeftDockWidgetArea, gcodeOptionsDock);
    
    // Connect the show/hide action
    connect(showGCodeOptionsPanelAct, &QAction::toggled, gcodeOptionsDock, &QDockWidget::setVisible);
    connect(gcodeOptionsDock, &QDockWidget::visibilityChanged, showGCodeOptionsPanelAct, &QAction::setChecked);
}

void MainWindow::createStatusBar()
{
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
    
    gcodeOptionsDock->setVisible(gcodeOptionsPanelVisible);
    
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
        svgViewer->loadSvgFile(fileName);
        tabWidget->setCurrentIndex(2); // Switch to SVG viewer tab
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
    gCodeEditor->setPlainText(in.readAll());

    setCurrentFile(fileName);
    statusBar()->showMessage(tr("File loaded"), 2000);
    updateGCodePreview();
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
    // Parse GCode and update the 3D viewer
    QString gcode = gCodeEditor->toPlainText();
    gCodeViewer->processGCode(gcode);
}

void MainWindow::convertSvgToGCode(const QString &svgFile)
{
    if (svgFile.isEmpty()) {
        return;
    }
    
    // Get parameters from the GCode options panel
    double toolDiameter = 3.0; // Default value
    double feedRate = gcodeOptionsPanel->getFeedRate();
    double plungeRate = gcodeOptionsPanel->getPlungeRate();
    double passDepth = gcodeOptionsPanel->getPassDepth();
    double totalDepth = gcodeOptionsPanel->getTotalDepth();
    double safetyHeight = gcodeOptionsPanel->getSafetyHeightEnabled() ? 
                           gcodeOptionsPanel->getSafetyHeight() : 5.0;
    
    // Determine conversion mode
    SvgToGCode::ConversionMode mode = SvgToGCode::Outline; // Default
    
    // Convert SVG to GCode
    QString gcode = svgToGCode->convertSvgToGCode(
        svgFile, mode, toolDiameter, feedRate, plungeRate, 
        passDepth, totalDepth, safetyHeight
    );
    
    if (gcode.isEmpty()) {
        QMessageBox::warning(this, tr("Conversion Error"),
                            tr("Failed to convert SVG to GCode: %1")
                            .arg(svgToGCode->lastError()));
        return;
    }
    
    // Update the GCode editor with the generated code
    gCodeEditor->setPlainText(gcode);
    
    // Switch to the GCode editor tab
    tabWidget->setCurrentIndex(0);
    
    // Mark as untitled so user can save it
    setCurrentFile("");
    
    statusBar()->showMessage(tr("SVG converted to GCode"), 3000);
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

void MainWindow::about()
{
    QMessageBox::about(this, tr("About NWSS-CNC"),
             tr("<h3>NWSS-CNC</h3>"
                "<p>Version 1.0</p>"
                "<p>A modern CNC control application with G-Code editing, "
                "3D previewing, and SVG import capabilities.</p>"));
}