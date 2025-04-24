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

class GCodeOptionsPanel : public QWidget
{
    Q_OBJECT

public:
    explicit GCodeOptionsPanel(QWidget *parent = nullptr);
    
    // Getters for the current settings
    QString getToolType() const;
    double getFeedRate() const;
    double getPlungeRate() const;
    double getSpindleSpeed() const;
    int getPassDepth() const;
    int getTotalDepth() const;
    bool getSafetyHeightEnabled() const;
    double getSafetyHeight() const;
    
signals:
    void optionsChanged();
    void generateGCode();

private slots:
    void onToolTypeChanged(const QString &toolType);
    void updateUnitDisplay(bool isMetric);

private:
    // Tool settings
    QComboBox *toolTypeComboBox;
    QDoubleSpinBox *toolDiameterSpinBox;
    QSpinBox *toolFluteCountSpinBox;
    
    // Cut settings
    QDoubleSpinBox *feedRateSpinBox;
    QDoubleSpinBox *plungeRateSpinBox;
    QDoubleSpinBox *spindleSpeedSpinBox;
    QDoubleSpinBox *passDepthSpinBox;
    QDoubleSpinBox *totalDepthSpinBox;
    
    // Safety settings
    QCheckBox *safetyHeightCheckBox;
    QDoubleSpinBox *safetyHeightSpinBox;
    
    // Unit settings
    QCheckBox *metricUnitsCheckBox;
    
    // Generator settings
    QComboBox *optimizationComboBox;
    
    // Generate button
    QPushButton *generateButton;
    
    void setupToolGroup();
    void setupCutSettingsGroup();
    void setupSafetyGroup();
    void setupOptimizationGroup();
    
    bool isMetric;
};

#endif // GCODEOPTIONSPANEL_H