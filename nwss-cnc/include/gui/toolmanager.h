#ifndef NWSS_CNC_TOOLMANAGER_H
#define NWSS_CNC_TOOLMANAGER_H

#include <QDialog>
#include <QTableWidget>
#include <QPushButton>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QHeaderView>
#include <QMessageBox>
#include <QFileDialog>
#include "core/tool.h"

namespace nwss {
namespace cnc {

/**
 * Dialog for managing CNC tools
 */
class ToolManager : public QDialog {
    Q_OBJECT

public:
    explicit ToolManager(ToolRegistry& toolRegistry, QWidget* parent = nullptr);
    ~ToolManager();

    /**
     * Get the currently selected tool ID
     * @return Selected tool ID, or 0 if none selected
     */
    int getSelectedToolId() const;

    /**
     * Set the selected tool by ID
     * @param toolId The tool ID to select
     */
    void setSelectedTool(int toolId);

signals:
    /**
     * Emitted when a tool is selected
     * @param toolId The ID of the selected tool
     */
    void toolSelected(int toolId);

    /**
     * Emitted when the tool registry is modified
     */
    void toolRegistryChanged();

private slots:
    void onAddTool();
    void onEditTool();
    void onDeleteTool();
    void onDuplicateTool();
    void onImportTools();
    void onExportTools();
    void onToolSelectionChanged();
    void onToolDoubleClicked(int row, int column);

private:
    void setupUI();
    void setupToolTable();
    void setupToolEditor();
    void setupButtons();
    void refreshToolTable();
    void clearToolEditor();
    void loadToolToEditor(const Tool& tool);
    Tool getToolFromEditor() const;
    bool validateToolEditor() const;
    void updateToolInTable(int row, const Tool& tool);
    int findToolRow(int toolId) const;
    void selectToolInTable(int toolId);

    // UI Components
    QVBoxLayout* m_mainLayout;
    QHBoxLayout* m_contentLayout;
    
    // Tool table
    QGroupBox* m_tableGroup;
    QTableWidget* m_toolTable;
    QVBoxLayout* m_tableLayout;
    
    // Tool editor
    QGroupBox* m_editorGroup;
    QFormLayout* m_editorLayout;
    QLineEdit* m_nameEdit;
    QComboBox* m_typeCombo;
    QDoubleSpinBox* m_diameterSpin;
    QDoubleSpinBox* m_lengthSpin;
    QDoubleSpinBox* m_fluteLengthSpin;
    QSpinBox* m_fluteCountSpin;
    QComboBox* m_materialCombo;
    QComboBox* m_coatingCombo;
    QDoubleSpinBox* m_maxDepthSpin;
    QDoubleSpinBox* m_maxFeedRateSpin;
    QSpinBox* m_maxSpindleSpeedSpin;
    QSpinBox* m_minSpindleSpeedSpin;
    QTextEdit* m_notesEdit;
    QCheckBox* m_activeCheck;
    
    // Buttons
    QHBoxLayout* m_buttonLayout;
    QPushButton* m_addButton;
    QPushButton* m_editButton;
    QPushButton* m_deleteButton;
    QPushButton* m_duplicateButton;
    QPushButton* m_importButton;
    QPushButton* m_exportButton;
    QPushButton* m_selectButton;
    QPushButton* m_cancelButton;

    // Data
    ToolRegistry& m_toolRegistry;
    int m_selectedToolId;
    bool m_editingTool;
    int m_editingToolId;

    // Constants
    static const int COLUMN_ID = 0;
    static const int COLUMN_NAME = 1;
    static const int COLUMN_TYPE = 2;
    static const int COLUMN_DIAMETER = 3;
    static const int COLUMN_MATERIAL = 4;
    static const int COLUMN_ACTIVE = 5;
    static const int COLUMN_COUNT = 6;
};

/**
 * Widget for tool selection in other dialogs
 */
class ToolSelector : public QWidget {
    Q_OBJECT

public:
    explicit ToolSelector(ToolRegistry& toolRegistry, QWidget* parent = nullptr);
    ~ToolSelector();

    /**
     * Get the currently selected tool ID
     * @return Selected tool ID, or 0 if none selected
     */
    int getSelectedToolId() const;

    /**
     * Set the selected tool by ID
     * @param toolId The tool ID to select
     */
    void setSelectedTool(int toolId);

    /**
     * Refresh the tool list
     */
    void refreshTools();

signals:
    /**
     * Emitted when a tool is selected
     * @param toolId The ID of the selected tool
     */
    void toolSelected(int toolId);

private slots:
    void onToolChanged();
    void onManageTools();

private:
    void setupUI();
    void populateToolCombo();

    // UI Components
    QHBoxLayout* m_layout;
    QLabel* m_label;
    QComboBox* m_toolCombo;
    QPushButton* m_manageButton;

    // Data
    ToolRegistry& m_toolRegistry;
};

} // namespace cnc
} // namespace nwss

#endif // NWSS_CNC_TOOLMANAGER_H 