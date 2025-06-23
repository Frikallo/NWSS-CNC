// gcodeoptionspanel.h
#ifndef GCODEOPTIONSPANEL_H
#define GCODEOPTIONSPANEL_H

#include <QWidget>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QTabWidget>
#include <QSettings>
#include <QFileDialog>

class GCodeOptionsPanel : public QWidget
{
    Q_OBJECT

public:
    explicit GCodeOptionsPanel(QWidget *parent = nullptr);
    
    // Getters for the current settings
    // Machine settings
    double getBedWidth() const;
    double getBedHeight() const;
    bool isMetricUnits() const;
    
    // Material settings
    double getMaterialWidth() const;
    double getMaterialHeight() const;
    double getMaterialThickness() const;
    
    // Tool settings
    QString getToolType() const;
    double getToolDiameter() const;
    int getToolFluteCount() const;
    
    // Cutting settings
    double getFeedRate() const;
    double getPlungeRate() const;
    double getSpindleSpeed() const;
    double getCutDepth() const;
    int getPassCount() const;
    bool getSafetyHeightEnabled() const;
    double getSafetyHeight() const;
    
    // Backward compatibility methods (aliasing new methods)
    double getPassDepth() const;  // Alias for getCutDepth
    double getTotalDepth() const; // Material thickness (total depth to cut)
    
    // Discretization options
    int getBezierSamples() const;
    double getAdaptiveSampling() const;
    double getSimplifyTolerance() const;
    double getMaxPointDistance() const;
    
    // Path transformation options
    bool getPreserveAspectRatio() const;
    bool getCenterX() const;
    bool getCenterY() const;
    bool getFlipY() const;
    
    // G-Code generation options
    bool getOptimizePaths() const;
    bool getLinearizePaths() const;
    
    // Cutout mode settings
    int getCutoutMode() const;
    double getStepover() const;
    double getMaxStepover() const;
    bool getSpiralIn() const;
    
    // Settings management
    void saveSettings();
    void loadSettings();
    
    // Profile management
    void saveProfileAs();
    void loadProfile();

    void setCurrentSvgFile(const QString& filename);
    
signals:
    void optionsChanged();
    void generateGCode(const QString &svgFile);
    void settingsLoaded();

private slots:
    void onToolTypeChanged(const QString &toolType);
    void updateUnitDisplay(bool isMetric);
    void onMaterialChanged();
    void onCutoutModeChanged(int index);

private:
    // Tab widget for organizing settings
    QTabWidget *tabWidget;
    
    // Machine settings
    QDoubleSpinBox *bedWidthSpinBox;
    QDoubleSpinBox *bedHeightSpinBox;
    QCheckBox *metricUnitsCheckBox;
    
    // Material settings
    QDoubleSpinBox *materialWidthSpinBox;
    QDoubleSpinBox *materialHeightSpinBox;
    QDoubleSpinBox *materialThicknessSpinBox;
    
    // Tool settings
    QComboBox *toolTypeComboBox;
    QDoubleSpinBox *toolDiameterSpinBox;
    QSpinBox *toolFluteCountSpinBox;
    
    // Cutting settings
    QDoubleSpinBox *feedRateSpinBox;
    QDoubleSpinBox *plungeRateSpinBox;
    QDoubleSpinBox *spindleSpeedSpinBox;
    QDoubleSpinBox *cutDepthSpinBox;
    QSpinBox *passCountSpinBox;
    QCheckBox *safetyHeightCheckBox;
    QDoubleSpinBox *safetyHeightSpinBox;
    
    // Cutout mode settings
    QComboBox *cutoutModeComboBox;
    QDoubleSpinBox *stepoverSpinBox;
    QDoubleSpinBox *maxStepoverSpinBox;
    QCheckBox *spiralInCheckBox;
    
    // Discretization options
    QSpinBox *bezierSamplesSpinBox;
    QDoubleSpinBox *adaptiveSpinBox;
    QDoubleSpinBox *simplifySpinBox;
    QDoubleSpinBox *maxPointDistanceSpinBox;
    
    // Path transformation options
    QCheckBox *preserveAspectRatioCheckBox;
    QCheckBox *centerXCheckBox;
    QCheckBox *centerYCheckBox;
    QCheckBox *flipYCheckBox;
    
    // G-Code generation options
    QCheckBox *optimizePathsCheckBox;
    QCheckBox *linearizePathsCheckBox;
    
    // Profile buttons
    QPushButton *saveProfileButton;
    QPushButton *loadProfileButton;
    
    // Generate button
    QPushButton *generateButton;
    
    // Setup methods for different sections
    void setupMachineTab();
    void setupToolTab();
    void setupCuttingTab();
    void setupTransformTab();
    void setupAdvancedTab();
    void setupProfileButtons();
    
    void recalculatePassCount();
    
    bool isMetric;
    QString m_currentSvgFile;
    QVBoxLayout *contentLayout;  // Layout for the centered content
};

#endif // GCODEOPTIONSPANEL_H