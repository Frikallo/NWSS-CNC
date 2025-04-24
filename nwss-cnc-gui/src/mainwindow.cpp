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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), isUntitled(true)
{
    // Create the main widgets
    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    gCodeEditor = new GCodeEditor(this);
    gCodeViewer = new GCodeViewer3D(this);

    // Set up splitter to hold the editor and viewer
    mainSplitter = new QSplitter(Qt::Horizontal, this);
    mainSplitter->addWidget(gCodeEditor);
    mainSplitter->addWidget(gCodeViewer);
    
    // Make sure both widgets start with equal width
    // We'll use a QTimer to defer this until after the window is shown
    setCentralWidget(mainSplitter);

    // Set up UI elements
    createActions();
    createMenus();
    createToolBars();
    createStatusBar();

    // Set up signals and slots
    connect(gCodeEditor->document(), &QTextDocument::contentsChanged,
            this, &MainWindow::documentWasModified);
    connect(gCodeEditor, &GCodeEditor::textChanged,
            this, &MainWindow::updateGCodePreview);

    // Initial state
    setCurrentFile("");
    readSettings();
    statusBar()->showMessage(tr("Ready"), 2000);
    
    // Delay initial refresh to allow UI to be fully constructed
    QTimer::singleShot(100, this, &MainWindow::updateGCodePreview);
    
    // Use a timer to set the split after the window is shown and sized
    QTimer::singleShot(0, this, [this]() {
        QList<int> sizes;
        int halfWidth = width() / 2;
        sizes << halfWidth << halfWidth;
        mainSplitter->setSizes(sizes);
    });
}

MainWindow::~MainWindow()
{
    // Clean up any resources that need explicit deletion
}

void MainWindow::createActions()
{
    // File menu actions
    newAct = new QAction(tr("&New"), this);
    newAct->setShortcuts(QKeySequence::New);
    newAct->setStatusTip(tr("Create a new file"));
    connect(newAct, &QAction::triggered, this, &MainWindow::newFile);

    openAct = new QAction(tr("&Open..."), this);
    openAct->setShortcuts(QKeySequence::Open);
    openAct->setStatusTip(tr("Open an existing file"));
    connect(openAct, &QAction::triggered, this, &MainWindow::openFile);

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
    fileMenu->addAction(saveAct);
    fileMenu->addAction(saveAsAct);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAct);

    editMenu = menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(cutAct);
    editMenu->addAction(copyAct);
    editMenu->addAction(pasteAct);

    viewMenu = menuBar()->addMenu(tr("&View"));
    // Add view actions here later

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
    fileToolBar->addAction(saveAct);

    editToolBar = addToolBar(tr("Edit"));
    editToolBar->addAction(cutAct);
    editToolBar->addAction(copyAct);
    editToolBar->addAction(pasteAct);
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
}

void MainWindow::writeSettings()
{
    QSettings settings("NWSS", "NWSS-CNC");
    settings.setValue("pos", pos());
    settings.setValue("size", size());
}

void MainWindow::newFile()
{
    if (maybeSave()) {
        gCodeEditor->clear();
        setCurrentFile("");
    }
}

void MainWindow::openFile()
{
    if (maybeSave()) {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Open GCode File"), "",
                                                       tr("GCode Files (*.gcode *.nc *.ngc);;All Files (*)"));
        if (!fileName.isEmpty())
            loadFile(fileName);
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

void MainWindow::about()
{
    QMessageBox::about(this, tr("About NWSS-CNC"),
             tr("<b>NWSS-CNC</b> is a GCode editor and 3D previewer for CNC operations."));
}