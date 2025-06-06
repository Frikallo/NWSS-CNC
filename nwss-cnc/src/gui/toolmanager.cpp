#include "gui/toolmanager.h"
#include <QApplication>
#include <QStandardPaths>
#include <QDir>

namespace nwss {
namespace cnc {

// ToolManager implementation
ToolManager::ToolManager(ToolRegistry& toolRegistry, QWidget* parent)
    : QDialog(parent)
    , m_toolRegistry(toolRegistry)
    , m_selectedToolId(0)
    , m_editingTool(false)
    , m_editingToolId(0)
{
    setWindowTitle("Tool Manager");
    setModal(true);
    resize(900, 600);
    
    setupUI();
    refreshToolTable();
}

ToolManager::~ToolManager() {
    // Ensure tool registry is saved when dialog is closed
    m_toolRegistry.saveToDefaultLocation();
}

int ToolManager::getSelectedToolId() const {
    return m_selectedToolId;
}

void ToolManager::setSelectedTool(int toolId) {
    m_selectedToolId = toolId;
    selectToolInTable(toolId);
}

void ToolManager::setupUI() {
    m_mainLayout = new QVBoxLayout(this);
    m_contentLayout = new QHBoxLayout();
    
    setupToolTable();
    setupToolEditor();
    setupButtons();
    
    m_contentLayout->addWidget(m_tableGroup, 2);
    m_contentLayout->addWidget(m_editorGroup, 1);
    
    m_mainLayout->addLayout(m_contentLayout);
    m_mainLayout->addLayout(m_buttonLayout);
}

void ToolManager::setupToolTable() {
    m_tableGroup = new QGroupBox("Tools", this);
    m_tableLayout = new QVBoxLayout(m_tableGroup);
    
    m_toolTable = new QTableWidget(this);
    m_toolTable->setColumnCount(COLUMN_COUNT);
    
    QStringList headers;
    headers << "ID" << "Name" << "Type" << "Diameter (mm)" << "Material" << "Active";
    m_toolTable->setHorizontalHeaderLabels(headers);
    
    m_toolTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_toolTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_toolTable->setAlternatingRowColors(true);
    m_toolTable->setSortingEnabled(true);
    
    // Set column widths
    m_toolTable->horizontalHeader()->setStretchLastSection(true);
    m_toolTable->setColumnWidth(COLUMN_ID, 50);
    m_toolTable->setColumnWidth(COLUMN_NAME, 200);
    m_toolTable->setColumnWidth(COLUMN_TYPE, 120);
    m_toolTable->setColumnWidth(COLUMN_DIAMETER, 100);
    m_toolTable->setColumnWidth(COLUMN_MATERIAL, 80);
    
    connect(m_toolTable, &QTableWidget::itemSelectionChanged,
            this, &ToolManager::onToolSelectionChanged);
    connect(m_toolTable, &QTableWidget::cellDoubleClicked,
            this, &ToolManager::onToolDoubleClicked);
    
    m_tableLayout->addWidget(m_toolTable);
}

void ToolManager::setupToolEditor() {
    m_editorGroup = new QGroupBox("Tool Properties", this);
    m_editorLayout = new QFormLayout(m_editorGroup);
    
    // Basic properties
    m_nameEdit = new QLineEdit(this);
    m_editorLayout->addRow("Name:", m_nameEdit);
    
    m_typeCombo = new QComboBox(this);
    m_typeCombo->addItem("End Mill", static_cast<int>(ToolType::END_MILL));
    m_typeCombo->addItem("Ball Nose", static_cast<int>(ToolType::BALL_NOSE));
    m_typeCombo->addItem("V-Bit", static_cast<int>(ToolType::V_BIT));
    m_typeCombo->addItem("Drill", static_cast<int>(ToolType::DRILL));
    m_typeCombo->addItem("Router Bit", static_cast<int>(ToolType::ROUTER_BIT));
    m_typeCombo->addItem("Engraving Bit", static_cast<int>(ToolType::ENGRAVING_BIT));
    m_typeCombo->addItem("Custom", static_cast<int>(ToolType::CUSTOM));
    m_editorLayout->addRow("Type:", m_typeCombo);
    
    // Dimensions
    m_diameterSpin = new QDoubleSpinBox(this);
    m_diameterSpin->setRange(0.01, 50.0);
    m_diameterSpin->setDecimals(3);
    m_diameterSpin->setSuffix(" mm");
    m_editorLayout->addRow("Diameter:", m_diameterSpin);
    
    m_lengthSpin = new QDoubleSpinBox(this);
    m_lengthSpin->setRange(0.0, 200.0);
    m_lengthSpin->setDecimals(1);
    m_lengthSpin->setSuffix(" mm");
    m_editorLayout->addRow("Length:", m_lengthSpin);
    
    m_fluteLengthSpin = new QDoubleSpinBox(this);
    m_fluteLengthSpin->setRange(0.0, 100.0);
    m_fluteLengthSpin->setDecimals(1);
    m_fluteLengthSpin->setSuffix(" mm");
    m_editorLayout->addRow("Flute Length:", m_fluteLengthSpin);
    
    m_fluteCountSpin = new QSpinBox(this);
    m_fluteCountSpin->setRange(1, 8);
    m_editorLayout->addRow("Flute Count:", m_fluteCountSpin);
    
    // Material and coating
    m_materialCombo = new QComboBox(this);
    m_materialCombo->addItem("HSS", static_cast<int>(ToolMaterial::HSS));
    m_materialCombo->addItem("Carbide", static_cast<int>(ToolMaterial::CARBIDE));
    m_materialCombo->addItem("Ceramic", static_cast<int>(ToolMaterial::CERAMIC));
    m_materialCombo->addItem("Diamond", static_cast<int>(ToolMaterial::DIAMOND));
    m_materialCombo->addItem("Cobalt", static_cast<int>(ToolMaterial::COBALT));
    m_materialCombo->addItem("Unknown", static_cast<int>(ToolMaterial::UNKNOWN));
    m_editorLayout->addRow("Material:", m_materialCombo);
    
    m_coatingCombo = new QComboBox(this);
    m_coatingCombo->addItem("None", static_cast<int>(ToolCoating::NONE));
    m_coatingCombo->addItem("TiN", static_cast<int>(ToolCoating::TIN));
    m_coatingCombo->addItem("TiCN", static_cast<int>(ToolCoating::TICN));
    m_coatingCombo->addItem("TiAlN", static_cast<int>(ToolCoating::TIALN));
    m_coatingCombo->addItem("DLC", static_cast<int>(ToolCoating::DLC));
    m_coatingCombo->addItem("Unknown", static_cast<int>(ToolCoating::UNKNOWN));
    m_editorLayout->addRow("Coating:", m_coatingCombo);
    
    // Cutting parameters
    m_maxDepthSpin = new QDoubleSpinBox(this);
    m_maxDepthSpin->setRange(0.0, 20.0);
    m_maxDepthSpin->setDecimals(2);
    m_maxDepthSpin->setSuffix(" mm");
    m_editorLayout->addRow("Max Depth of Cut:", m_maxDepthSpin);
    
    m_maxFeedRateSpin = new QDoubleSpinBox(this);
    m_maxFeedRateSpin->setRange(0.0, 10000.0);
    m_maxFeedRateSpin->setDecimals(0);
    m_maxFeedRateSpin->setSuffix(" mm/min");
    m_editorLayout->addRow("Max Feed Rate:", m_maxFeedRateSpin);
    
    m_maxSpindleSpeedSpin = new QSpinBox(this);
    m_maxSpindleSpeedSpin->setRange(0, 50000);
    m_maxSpindleSpeedSpin->setSuffix(" RPM");
    m_editorLayout->addRow("Max Spindle Speed:", m_maxSpindleSpeedSpin);
    
    m_minSpindleSpeedSpin = new QSpinBox(this);
    m_minSpindleSpeedSpin->setRange(0, 50000);
    m_minSpindleSpeedSpin->setSuffix(" RPM");
    m_editorLayout->addRow("Min Spindle Speed:", m_minSpindleSpeedSpin);
    
    // Notes and status
    m_notesEdit = new QTextEdit(this);
    m_notesEdit->setMaximumHeight(60);
    m_editorLayout->addRow("Notes:", m_notesEdit);
    
    m_activeCheck = new QCheckBox("Tool is active", this);
    m_activeCheck->setChecked(true);
    m_editorLayout->addRow("", m_activeCheck);
    
    clearToolEditor();
}

void ToolManager::setupButtons() {
    m_buttonLayout = new QHBoxLayout();
    
    // Tool management buttons
    m_addButton = new QPushButton("Add Tool", this);
    m_editButton = new QPushButton("Edit Tool", this);
    m_deleteButton = new QPushButton("Delete Tool", this);
    m_duplicateButton = new QPushButton("Duplicate Tool", this);
    
    // Import/Export buttons
    m_importButton = new QPushButton("Import Tools", this);
    m_exportButton = new QPushButton("Export Tools", this);
    
    // Dialog buttons
    m_selectButton = new QPushButton("Select Tool", this);
    m_cancelButton = new QPushButton("Cancel", this);
    
    // Connect signals
    connect(m_addButton, &QPushButton::clicked, this, &ToolManager::onAddTool);
    connect(m_editButton, &QPushButton::clicked, this, &ToolManager::onEditTool);
    connect(m_deleteButton, &QPushButton::clicked, this, &ToolManager::onDeleteTool);
    connect(m_duplicateButton, &QPushButton::clicked, this, &ToolManager::onDuplicateTool);
    connect(m_importButton, &QPushButton::clicked, this, &ToolManager::onImportTools);
    connect(m_exportButton, &QPushButton::clicked, this, &ToolManager::onExportTools);
    connect(m_selectButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    
    // Layout buttons
    m_buttonLayout->addWidget(m_addButton);
    m_buttonLayout->addWidget(m_editButton);
    m_buttonLayout->addWidget(m_deleteButton);
    m_buttonLayout->addWidget(m_duplicateButton);
    m_buttonLayout->addStretch();
    m_buttonLayout->addWidget(m_importButton);
    m_buttonLayout->addWidget(m_exportButton);
    m_buttonLayout->addStretch();
    m_buttonLayout->addWidget(m_selectButton);
    m_buttonLayout->addWidget(m_cancelButton);
    
    // Initial button states
    m_editButton->setEnabled(false);
    m_deleteButton->setEnabled(false);
    m_duplicateButton->setEnabled(false);
    m_selectButton->setEnabled(false);
}

void ToolManager::refreshToolTable() {
    m_toolTable->setRowCount(0);
    
    auto tools = m_toolRegistry.getAllTools();
    m_toolTable->setRowCount(tools.size());
    
    for (size_t i = 0; i < tools.size(); ++i) {
        const Tool& tool = tools[i];
        updateToolInTable(i, tool);
    }
    
    m_toolTable->resizeColumnsToContents();
}

void ToolManager::clearToolEditor() {
    m_nameEdit->clear();
    m_typeCombo->setCurrentIndex(0);
    m_diameterSpin->setValue(0.0);
    m_lengthSpin->setValue(0.0);
    m_fluteLengthSpin->setValue(0.0);
    m_fluteCountSpin->setValue(2);
    m_materialCombo->setCurrentIndex(0);
    m_coatingCombo->setCurrentIndex(0);
    m_maxDepthSpin->setValue(0.0);
    m_maxFeedRateSpin->setValue(0.0);
    m_maxSpindleSpeedSpin->setValue(0);
    m_minSpindleSpeedSpin->setValue(0);
    m_notesEdit->clear();
    m_activeCheck->setChecked(true);
}

void ToolManager::loadToolToEditor(const Tool& tool) {
    m_nameEdit->setText(QString::fromStdString(tool.name));
    m_typeCombo->setCurrentIndex(m_typeCombo->findData(static_cast<int>(tool.type)));
    m_diameterSpin->setValue(tool.diameter);
    m_lengthSpin->setValue(tool.length);
    m_fluteLengthSpin->setValue(tool.fluteLength);
    m_fluteCountSpin->setValue(tool.fluteCount);
    m_materialCombo->setCurrentIndex(m_materialCombo->findData(static_cast<int>(tool.material)));
    m_coatingCombo->setCurrentIndex(m_coatingCombo->findData(static_cast<int>(tool.coating)));
    m_maxDepthSpin->setValue(tool.maxDepthOfCut);
    m_maxFeedRateSpin->setValue(tool.maxFeedRate);
    m_maxSpindleSpeedSpin->setValue(tool.maxSpindleSpeed);
    m_minSpindleSpeedSpin->setValue(tool.minSpindleSpeed);
    m_notesEdit->setPlainText(QString::fromStdString(tool.notes));
    m_activeCheck->setChecked(tool.isActive);
}

Tool ToolManager::getToolFromEditor() const {
    Tool tool;
    tool.name = m_nameEdit->text().toStdString();
    tool.type = static_cast<ToolType>(m_typeCombo->currentData().toInt());
    tool.diameter = m_diameterSpin->value();
    tool.length = m_lengthSpin->value();
    tool.fluteLength = m_fluteLengthSpin->value();
    tool.fluteCount = m_fluteCountSpin->value();
    tool.material = static_cast<ToolMaterial>(m_materialCombo->currentData().toInt());
    tool.coating = static_cast<ToolCoating>(m_coatingCombo->currentData().toInt());
    tool.maxDepthOfCut = m_maxDepthSpin->value();
    tool.maxFeedRate = m_maxFeedRateSpin->value();
    tool.maxSpindleSpeed = m_maxSpindleSpeedSpin->value();
    tool.minSpindleSpeed = m_minSpindleSpeedSpin->value();
    tool.notes = m_notesEdit->toPlainText().toStdString();
    tool.isActive = m_activeCheck->isChecked();
    return tool;
}

bool ToolManager::validateToolEditor() const {
    if (m_nameEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(const_cast<ToolManager*>(this), "Validation Error", 
                           "Tool name cannot be empty.");
        return false;
    }
    
    if (m_diameterSpin->value() <= 0) {
        QMessageBox::warning(const_cast<ToolManager*>(this), "Validation Error", 
                           "Tool diameter must be greater than 0.");
        return false;
    }
    
    return true;
}

void ToolManager::updateToolInTable(int row, const Tool& tool) {
    m_toolTable->setItem(row, COLUMN_ID, new QTableWidgetItem(QString::number(tool.id)));
    m_toolTable->setItem(row, COLUMN_NAME, new QTableWidgetItem(QString::fromStdString(tool.name)));
    m_toolTable->setItem(row, COLUMN_TYPE, new QTableWidgetItem(QString::fromStdString(tool.getTypeString())));
    m_toolTable->setItem(row, COLUMN_DIAMETER, new QTableWidgetItem(QString::number(tool.diameter, 'f', 3)));
    m_toolTable->setItem(row, COLUMN_MATERIAL, new QTableWidgetItem(QString::fromStdString(tool.getMaterialString())));
    m_toolTable->setItem(row, COLUMN_ACTIVE, new QTableWidgetItem(tool.isActive ? "Yes" : "No"));
    
    // Store tool ID in the first column for easy retrieval
    m_toolTable->item(row, COLUMN_ID)->setData(Qt::UserRole, tool.id);
}

int ToolManager::findToolRow(int toolId) const {
    for (int row = 0; row < m_toolTable->rowCount(); ++row) {
        QTableWidgetItem* item = m_toolTable->item(row, COLUMN_ID);
        if (item && item->data(Qt::UserRole).toInt() == toolId) {
            return row;
        }
    }
    return -1;
}

void ToolManager::selectToolInTable(int toolId) {
    int row = findToolRow(toolId);
    if (row >= 0) {
        m_toolTable->selectRow(row);
        m_selectedToolId = toolId;
    }
}

void ToolManager::onAddTool() {
    if (!validateToolEditor()) return;
    
    Tool tool = getToolFromEditor();
    int toolId = m_toolRegistry.addTool(tool);
    
    refreshToolTable();
    selectToolInTable(toolId);
    clearToolEditor();
    
    // Save to persistent storage
    m_toolRegistry.saveToDefaultLocation();
    
    emit toolRegistryChanged();
}

void ToolManager::onEditTool() {
    if (m_selectedToolId == 0) return;
    
    if (!validateToolEditor()) return;
    
    Tool tool = getToolFromEditor();
    tool.id = m_selectedToolId;
    
    if (m_toolRegistry.updateTool(tool)) {
        refreshToolTable();
        selectToolInTable(m_selectedToolId);
        clearToolEditor();
        
        // Save to persistent storage
        m_toolRegistry.saveToDefaultLocation();
        
        emit toolRegistryChanged();
    }
}

void ToolManager::onDeleteTool() {
    if (m_selectedToolId == 0) return;
    
    const Tool* tool = m_toolRegistry.getTool(m_selectedToolId);
    if (!tool) return;
    
    int ret = QMessageBox::question(this, "Delete Tool",
                                  QString("Are you sure you want to delete tool '%1'?")
                                  .arg(QString::fromStdString(tool->name)),
                                  QMessageBox::Yes | QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        m_toolRegistry.removeTool(m_selectedToolId);
        refreshToolTable();
        clearToolEditor();
        m_selectedToolId = 0;
        
        // Save to persistent storage
        m_toolRegistry.saveToDefaultLocation();
        
        emit toolRegistryChanged();
    }
}

void ToolManager::onDuplicateTool() {
    if (m_selectedToolId == 0) return;
    
    const Tool* originalTool = m_toolRegistry.getTool(m_selectedToolId);
    if (!originalTool) return;
    
    Tool newTool = *originalTool;
    newTool.id = 0; // Will be assigned by registry
    newTool.name += " (Copy)";
    
    int toolId = m_toolRegistry.addTool(newTool);
    refreshToolTable();
    selectToolInTable(toolId);
    
    // Save to persistent storage
    m_toolRegistry.saveToDefaultLocation();
    
    emit toolRegistryChanged();
}

void ToolManager::onImportTools() {
    QString fileName = QFileDialog::getOpenFileName(this,
        "Import Tools", "", "Tool Files (*.tools);;All Files (*)");
    
    if (!fileName.isEmpty()) {
        if (m_toolRegistry.loadFromFile(fileName.toStdString())) {
            refreshToolTable();
            
            // Save to persistent storage
            m_toolRegistry.saveToDefaultLocation();
            
            emit toolRegistryChanged();
            QMessageBox::information(this, "Import Successful", "Tools imported successfully.");
        } else {
            QMessageBox::warning(this, "Import Failed", "Failed to import tools from file.");
        }
    }
}

void ToolManager::onExportTools() {
    QString fileName = QFileDialog::getSaveFileName(this,
        "Export Tools", "tools.tools", "Tool Files (*.tools);;All Files (*)");
    
    if (!fileName.isEmpty()) {
        if (m_toolRegistry.saveToFile(fileName.toStdString())) {
            QMessageBox::information(this, "Export Successful", "Tools exported successfully.");
        } else {
            QMessageBox::warning(this, "Export Failed", "Failed to export tools to file.");
        }
    }
}

void ToolManager::onToolSelectionChanged() {
    QList<QTableWidgetItem*> selectedItems = m_toolTable->selectedItems();
    
    if (!selectedItems.isEmpty()) {
        int row = selectedItems.first()->row();
        QTableWidgetItem* idItem = m_toolTable->item(row, COLUMN_ID);
        
        if (idItem) {
            m_selectedToolId = idItem->data(Qt::UserRole).toInt();
            
            const Tool* tool = m_toolRegistry.getTool(m_selectedToolId);
            if (tool) {
                loadToolToEditor(*tool);
            }
            
            // Enable buttons
            m_editButton->setEnabled(true);
            m_deleteButton->setEnabled(true);
            m_duplicateButton->setEnabled(true);
            m_selectButton->setEnabled(true);
            
            emit toolSelected(m_selectedToolId);
        }
    } else {
        m_selectedToolId = 0;
        clearToolEditor();
        
        // Disable buttons
        m_editButton->setEnabled(false);
        m_deleteButton->setEnabled(false);
        m_duplicateButton->setEnabled(false);
        m_selectButton->setEnabled(false);
    }
}

void ToolManager::onToolDoubleClicked(int row, int column) {
    Q_UNUSED(column)
    
    QTableWidgetItem* idItem = m_toolTable->item(row, COLUMN_ID);
    if (idItem) {
        m_selectedToolId = idItem->data(Qt::UserRole).toInt();
        emit toolSelected(m_selectedToolId);
        accept();
    }
}

// ToolSelector implementation
ToolSelector::ToolSelector(ToolRegistry& toolRegistry, QWidget* parent)
    : QWidget(parent)
    , m_toolRegistry(toolRegistry)
{
    setupUI();
    populateToolCombo();
}

ToolSelector::~ToolSelector() = default;

int ToolSelector::getSelectedToolId() const {
    return m_toolCombo->currentData().toInt();
}

void ToolSelector::setSelectedTool(int toolId) {
    int index = m_toolCombo->findData(toolId);
    if (index >= 0) {
        m_toolCombo->setCurrentIndex(index);
    }
}

void ToolSelector::refreshTools() {
    populateToolCombo();
}

void ToolSelector::setupUI() {
    m_layout = new QHBoxLayout(this);
    m_layout->setSizeConstraint(QLayout::SetFixedSize);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(10);
    m_label = new QLabel("Tool:", this);
    // add a left margin of 10px
    m_label->setContentsMargins(10, 0, 0, 0);
    m_toolCombo = new QComboBox(this);
    m_manageButton = new QPushButton("Manage...", this);
    
    m_layout->addWidget(m_label);
    m_layout->addWidget(m_toolCombo, 1);
    m_layout->addWidget(m_manageButton);
    
    connect(m_toolCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ToolSelector::onToolChanged);
    connect(m_manageButton, &QPushButton::clicked, this, &ToolSelector::onManageTools);
}

void ToolSelector::populateToolCombo() {
    m_toolCombo->clear();
    m_toolCombo->addItem("No tool selected", 0);
    
    auto tools = m_toolRegistry.getActiveTools();
    for (const auto& tool : tools) {
        QString text = QString("%1 - %2 (%3mm)")
                      .arg(tool.id)
                      .arg(QString::fromStdString(tool.name))
                      .arg(tool.diameter, 0, 'f', 3);
        m_toolCombo->addItem(text, tool.id);
    }
}

void ToolSelector::onToolChanged() {
    emit toolSelected(getSelectedToolId());
}

void ToolSelector::onManageTools() {
    ToolManager manager(m_toolRegistry, this);
    
    // Connect signals to handle changes during the manager session
    connect(&manager, &ToolManager::toolRegistryChanged,
            this, &ToolSelector::refreshTools);
    
    int result = manager.exec();
    
    // Always refresh after the dialog closes to ensure we have the latest tools
    populateToolCombo();
    
    if (result == QDialog::Accepted) {
        int selectedId = manager.getSelectedToolId();
        setSelectedTool(selectedId);
    }
}

} // namespace cnc
} // namespace nwss 