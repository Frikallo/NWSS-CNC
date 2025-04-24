// gcodeoptionspanel.cpp
#include "gcodeoptionspanel.h"

GCodeOptionsPanel::GCodeOptionsPanel(QWidget *parent)
    : QWidget(parent),
      isMetric(true)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    
    // Set up the various option groups
    setupToolGroup();
    setupCutSettingsGroup();
    setupSafetyGroup();
    setupOptimizationGroup();
    
    // Generate button
    generateButton = new QPushButton("Generate GCode");
    generateButton->setStyleSheet("background-color: #4CAF50; color: white; font-weight: bold;");
    mainLayout->addWidget(generateButton);
    
    // Add a stretch at the bottom
    mainLayout->addStretch();
    
    // Connect signals
    connect(generateButton, &QPushButton::clicked, this, &GCodeOptionsPanel::generateGCode);
    connect(toolTypeComboBox, &QComboBox::currentTextChanged, this, &GCodeOptionsPanel::onToolTypeChanged);
    connect(metricUnitsCheckBox, &QCheckBox::toggled, this, &GCodeOptionsPanel::updateUnitDisplay);
    
    // Set fixed width for panel
    setMinimumWidth(250);
    setMaximumWidth(300);
}

void GCodeOptionsPanel::setupToolGroup()
{
    QGroupBox *toolGroup = new QGroupBox("Tool Settings", this);
    QVBoxLayout *groupLayout = new QVBoxLayout(toolGroup);
    
    // Tool type
    QHBoxLayout *toolTypeLayout = new QHBoxLayout();
    toolTypeLayout->addWidget(new QLabel("Tool Type:"));
    toolTypeComboBox = new QComboBox();
    toolTypeComboBox->addItems({"End Mill", "Ball End Mill", "V-Bit", "Drill Bit"});
    toolTypeLayout->addWidget(toolTypeComboBox);
    groupLayout->addLayout(toolTypeLayout);
    
    // Tool diameter
    QHBoxLayout *diameterLayout = new QHBoxLayout();
    diameterLayout->addWidget(new QLabel("Diameter:"));
    toolDiameterSpinBox = new QDoubleSpinBox();
    toolDiameterSpinBox->setRange(0.1, 25.0);
    toolDiameterSpinBox->setValue(3.175);  // 1/8" in mm
    toolDiameterSpinBox->setSuffix(" mm");
    diameterLayout->addWidget(toolDiameterSpinBox);
    groupLayout->addLayout(diameterLayout);
    
    // Flute count (only relevant for some tool types)
    QHBoxLayout *fluteLayout = new QHBoxLayout();
    fluteLayout->addWidget(new QLabel("Flutes:"));
    toolFluteCountSpinBox = new QSpinBox();
    toolFluteCountSpinBox->setRange(1, 8);
    toolFluteCountSpinBox->setValue(2);
    fluteLayout->addWidget(toolFluteCountSpinBox);
    groupLayout->addLayout(fluteLayout);
    
    // Add to main layout
    static_cast<QVBoxLayout*>(layout())->addWidget(toolGroup);
}

void GCodeOptionsPanel::setupCutSettingsGroup()
{
    QGroupBox *cutGroup = new QGroupBox("Cut Settings", this);
    QVBoxLayout *groupLayout = new QVBoxLayout(cutGroup);
    
    // Units selection
    QHBoxLayout *unitsLayout = new QHBoxLayout();
    unitsLayout->addWidget(new QLabel("Units:"));
    metricUnitsCheckBox = new QCheckBox("Metric (mm)");
    metricUnitsCheckBox->setChecked(true);
    unitsLayout->addWidget(metricUnitsCheckBox);
    groupLayout->addLayout(unitsLayout);
    
    // Feed rate
    QHBoxLayout *feedLayout = new QHBoxLayout();
    feedLayout->addWidget(new QLabel("Feed Rate:"));
    feedRateSpinBox = new QDoubleSpinBox();
    feedRateSpinBox->setRange(1, 5000);
    feedRateSpinBox->setValue(500);
    feedRateSpinBox->setSuffix(" mm/min");
    feedLayout->addWidget(feedRateSpinBox);
    groupLayout->addLayout(feedLayout);
    
    // Plunge rate
    QHBoxLayout *plungeLayout = new QHBoxLayout();
    plungeLayout->addWidget(new QLabel("Plunge Rate:"));
    plungeRateSpinBox = new QDoubleSpinBox();
    plungeRateSpinBox->setRange(1, 1000);
    plungeRateSpinBox->setValue(100);
    plungeRateSpinBox->setSuffix(" mm/min");
    plungeLayout->addWidget(plungeRateSpinBox);
    groupLayout->addLayout(plungeLayout);
    
    // Spindle speed
    QHBoxLayout *spindleLayout = new QHBoxLayout();
    spindleLayout->addWidget(new QLabel("Spindle:"));
    spindleSpeedSpinBox = new QDoubleSpinBox();
    spindleSpeedSpinBox->setRange(1000, 24000);
    spindleSpeedSpinBox->setValue(10000);
    spindleSpeedSpinBox->setSuffix(" RPM");
    spindleLayout->addWidget(spindleSpeedSpinBox);
    groupLayout->addLayout(spindleLayout);
    
    // Pass depth
    QHBoxLayout *passLayout = new QHBoxLayout();
    passLayout->addWidget(new QLabel("Pass Depth:"));
    passDepthSpinBox = new QDoubleSpinBox();
    passDepthSpinBox->setRange(0.1, 10);
    passDepthSpinBox->setValue(1.0);
    passDepthSpinBox->setSuffix(" mm");
    passLayout->addWidget(passDepthSpinBox);
    groupLayout->addLayout(passLayout);
    
    // Total depth
    QHBoxLayout *totalLayout = new QHBoxLayout();
    totalLayout->addWidget(new QLabel("Total Depth:"));
    totalDepthSpinBox = new QDoubleSpinBox();
    totalDepthSpinBox->setRange(0.1, 50);
    totalDepthSpinBox->setValue(5.0);
    totalDepthSpinBox->setSuffix(" mm");
    totalLayout->addWidget(totalDepthSpinBox);
    groupLayout->addLayout(totalLayout);
    
    // Add to main layout
    static_cast<QVBoxLayout*>(layout())->addWidget(cutGroup);
}

void GCodeOptionsPanel::setupSafetyGroup()
{
    QGroupBox *safetyGroup = new QGroupBox("Safety Settings", this);
    QVBoxLayout *groupLayout = new QVBoxLayout(safetyGroup);
    
    // Safety height
    QHBoxLayout *safetyLayout = new QHBoxLayout();
    safetyHeightCheckBox = new QCheckBox("Safety Height:");
    safetyHeightCheckBox->setChecked(true);
    safetyLayout->addWidget(safetyHeightCheckBox);
    
    safetyHeightSpinBox = new QDoubleSpinBox();
    safetyHeightSpinBox->setRange(1, 50);
    safetyHeightSpinBox->setValue(5.0);
    safetyHeightSpinBox->setSuffix(" mm");
    safetyLayout->addWidget(safetyHeightSpinBox);
    groupLayout->addLayout(safetyLayout);
    
    // Add to main layout
    static_cast<QVBoxLayout*>(layout())->addWidget(safetyGroup);
    
    // Connect signals
    connect(safetyHeightCheckBox, &QCheckBox::toggled, safetyHeightSpinBox, &QDoubleSpinBox::setEnabled);
}

void GCodeOptionsPanel::setupOptimizationGroup()
{
    QGroupBox *optimizationGroup = new QGroupBox("Optimization", this);
    QVBoxLayout *groupLayout = new QVBoxLayout(optimizationGroup);
    
    // Optimization strategy
    QHBoxLayout *optimizationLayout = new QHBoxLayout();
    optimizationLayout->addWidget(new QLabel("Strategy:"));
    optimizationComboBox = new QComboBox();
    optimizationComboBox->addItems({"None", "Minimize Travel", "Minimize Tool Changes", "Optimized Depth", "Custom"});
    optimizationLayout->addWidget(optimizationComboBox);
    groupLayout->addLayout(optimizationLayout);
    
    // Add to main layout
    static_cast<QVBoxLayout*>(layout())->addWidget(optimizationGroup);
}

void GCodeOptionsPanel::onToolTypeChanged(const QString &toolType)
{
    // Enable/disable and adjust relevant controls based on tool type
    bool isEndMill = (toolType == "End Mill" || toolType == "Ball End Mill");
    bool isVBit = (toolType == "V-Bit");
    
    // Adjust UI based on tool type
    toolFluteCountSpinBox->setEnabled(isEndMill);
    
    // Signal that options have changed
    emit optionsChanged();
}

void GCodeOptionsPanel::updateUnitDisplay(bool isMetric)
{
    this->isMetric = isMetric;
    
    // Update suffix for all spinboxes
    if (isMetric) {
        toolDiameterSpinBox->setSuffix(" mm");
        feedRateSpinBox->setSuffix(" mm/min");
        plungeRateSpinBox->setSuffix(" mm/min");
        passDepthSpinBox->setSuffix(" mm");
        totalDepthSpinBox->setSuffix(" mm");
        safetyHeightSpinBox->setSuffix(" mm");
    } else {
        toolDiameterSpinBox->setSuffix(" in");
        feedRateSpinBox->setSuffix(" in/min");
        plungeRateSpinBox->setSuffix(" in/min");
        passDepthSpinBox->setSuffix(" in");
        totalDepthSpinBox->setSuffix(" in");
        safetyHeightSpinBox->setSuffix(" in");
    }
    
    // Also would need to convert values here in a real implementation
    
    // Signal that options have changed
    emit optionsChanged();
}

QString GCodeOptionsPanel::getToolType() const
{
    return toolTypeComboBox->currentText();
}

double GCodeOptionsPanel::getFeedRate() const
{
    return feedRateSpinBox->value();
}

double GCodeOptionsPanel::getPlungeRate() const
{
    return plungeRateSpinBox->value();
}

double GCodeOptionsPanel::getSpindleSpeed() const
{
    return spindleSpeedSpinBox->value();
}

int GCodeOptionsPanel::getPassDepth() const
{
    return static_cast<int>(passDepthSpinBox->value());
}

int GCodeOptionsPanel::getTotalDepth() const
{
    return static_cast<int>(totalDepthSpinBox->value());
}

bool GCodeOptionsPanel::getSafetyHeightEnabled() const
{
    return safetyHeightCheckBox->isChecked();
}

double GCodeOptionsPanel::getSafetyHeight() const
{
    return safetyHeightSpinBox->value();
}