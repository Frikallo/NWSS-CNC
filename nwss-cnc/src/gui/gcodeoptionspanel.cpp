// gcodeoptionspanel.cpp
#include "gcodeoptionspanel.h"
#include <QTimer>

GCodeOptionsPanel::GCodeOptionsPanel(QWidget *parent)
    : QWidget(parent),
      isMetric(true)
{
    // Create main horizontal layout to center the content
    QHBoxLayout *horizontalLayout = new QHBoxLayout(this);
    horizontalLayout->setContentsMargins(10, 0, 10, 0);
    
    // Add very small stretches on both sides to center the content
    horizontalLayout->addStretch(1);
    
    // Create the content widget that contains all the controls
    QWidget *contentWidget = new QWidget();
    contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setSpacing(10);
    
    // Create tab widget
    tabWidget = new QTabWidget(contentWidget);
    
    // Set up the various option groups
    setupMachineTab();
    setupToolTab();
    setupCuttingTab();
    setupTransformTab();
    setupAdvancedTab();
    
    contentLayout->addWidget(tabWidget);
    
    // Setup profile buttons
    setupProfileButtons();
    
    // Generate button
    generateButton = new QPushButton("Generate GCode");
    generateButton->setStyleSheet("background-color: #4CAF50; color: white; font-weight: bold;");
    contentLayout->addWidget(generateButton);
    
    // Add a stretch at the bottom
    contentLayout->addStretch();
    
    // Add the content widget to the horizontal layout (give it most of the space)
    horizontalLayout->addWidget(contentWidget, 39);  // Give content widget 38 parts (95% of space)
    horizontalLayout->addStretch(1);  // Very small stretch on the right
    
    // Connect signals
    connect(generateButton, &QPushButton::clicked, this, [this]() {
        emit generateGCode(m_currentSvgFile);
    });
    connect(toolTypeComboBox, &QComboBox::currentTextChanged, this, &GCodeOptionsPanel::onToolTypeChanged);
    connect(metricUnitsCheckBox, &QCheckBox::toggled, this, &GCodeOptionsPanel::updateUnitDisplay);
    connect(materialThicknessSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &GCodeOptionsPanel::onMaterialChanged);
    connect(cutDepthSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &GCodeOptionsPanel::recalculatePassCount);
    
    // Load settings from the last session
    QTimer::singleShot(100, this, &GCodeOptionsPanel::loadSettings);
}

void GCodeOptionsPanel::setCurrentSvgFile(const QString& filename) {
    m_currentSvgFile = filename;
}

void GCodeOptionsPanel::setupMachineTab()
{
    QWidget *machineTab = new QWidget();
    QVBoxLayout *tabLayout = new QVBoxLayout(machineTab);
    
    // Machine Settings Group
    QGroupBox *machineGroup = new QGroupBox("Machine Settings", machineTab);
    QVBoxLayout *machineLayout = new QVBoxLayout(machineGroup);
    
    // Bed width
    QHBoxLayout *bedWidthLayout = new QHBoxLayout();
    bedWidthLayout->addWidget(new QLabel("Bed Width:"));
    bedWidthSpinBox = new QDoubleSpinBox();
    bedWidthSpinBox->setRange(10, 2000);
    bedWidthSpinBox->setValue(300);
    bedWidthSpinBox->setSuffix(" mm");
    bedWidthLayout->addWidget(bedWidthSpinBox);
    machineLayout->addLayout(bedWidthLayout);
    
    // Bed height
    QHBoxLayout *bedHeightLayout = new QHBoxLayout();
    bedHeightLayout->addWidget(new QLabel("Bed Height:"));
    bedHeightSpinBox = new QDoubleSpinBox();
    bedHeightSpinBox->setRange(10, 2000);
    bedHeightSpinBox->setValue(200);
    bedHeightSpinBox->setSuffix(" mm");
    bedHeightLayout->addWidget(bedHeightSpinBox);
    machineLayout->addLayout(bedHeightLayout);
    
    // Units selection
    QHBoxLayout *unitsLayout = new QHBoxLayout();
    unitsLayout->addWidget(new QLabel("Units:"));
    metricUnitsCheckBox = new QCheckBox("Metric (mm)");
    metricUnitsCheckBox->setChecked(true);
    unitsLayout->addWidget(metricUnitsCheckBox);
    machineLayout->addLayout(unitsLayout);
    
    tabLayout->addWidget(machineGroup);
    
    // Material Settings Group
    QGroupBox *materialGroup = new QGroupBox("Material Settings", machineTab);
    QVBoxLayout *materialLayout = new QVBoxLayout(materialGroup);
    
    // Material width
    QHBoxLayout *materialWidthLayout = new QHBoxLayout();
    materialWidthLayout->addWidget(new QLabel("Width:"));
    materialWidthSpinBox = new QDoubleSpinBox();
    materialWidthSpinBox->setRange(1, 2000);
    materialWidthSpinBox->setValue(100);
    materialWidthSpinBox->setSuffix(" mm");
    materialWidthLayout->addWidget(materialWidthSpinBox);
    materialLayout->addLayout(materialWidthLayout);
    
    // Material height
    QHBoxLayout *materialHeightLayout = new QHBoxLayout();
    materialHeightLayout->addWidget(new QLabel("Height:"));
    materialHeightSpinBox = new QDoubleSpinBox();
    materialHeightSpinBox->setRange(1, 2000);
    materialHeightSpinBox->setValue(100);
    materialHeightSpinBox->setSuffix(" mm");
    materialHeightLayout->addWidget(materialHeightSpinBox);
    materialLayout->addLayout(materialHeightLayout);
    
    // Material thickness
    QHBoxLayout *materialThicknessLayout = new QHBoxLayout();
    materialThicknessLayout->addWidget(new QLabel("Thickness:"));
    materialThicknessSpinBox = new QDoubleSpinBox();
    materialThicknessSpinBox->setRange(0.1, 100);
    materialThicknessSpinBox->setValue(6.0);
    materialThicknessSpinBox->setSuffix(" mm");
    materialThicknessLayout->addWidget(materialThicknessSpinBox);
    materialLayout->addLayout(materialThicknessLayout);
    
    tabLayout->addWidget(materialGroup);
    
    // Add a spacer at the bottom
    tabLayout->addStretch();
    
    // Add the tab to the tab widget
    tabWidget->addTab(machineTab, "Machine");
}

void GCodeOptionsPanel::setupToolTab()
{
    QWidget *toolTab = new QWidget();
    QVBoxLayout *tabLayout = new QVBoxLayout(toolTab);
    
    // Tool Settings Group
    QGroupBox *toolGroup = new QGroupBox("Tool Settings", toolTab);
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
    
    tabLayout->addWidget(toolGroup);
    
    // Add a spacer at the bottom
    tabLayout->addStretch();
    
    // Add the tab to the tab widget
    // tabWidget->addTab(toolTab, "Tool");
}

void GCodeOptionsPanel::setupCuttingTab()
{
    QWidget *cuttingTab = new QWidget();
    QVBoxLayout *tabLayout = new QVBoxLayout(cuttingTab);
    
    // Cutting Settings Group
    QGroupBox *cutGroup = new QGroupBox("Cutting Settings", cuttingTab);
    QVBoxLayout *groupLayout = new QVBoxLayout(cutGroup);
    
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
    
    // Cut depth
    QHBoxLayout *cutDepthLayout = new QHBoxLayout();
    cutDepthLayout->addWidget(new QLabel("Cut Depth:"));
    cutDepthSpinBox = new QDoubleSpinBox();
    cutDepthSpinBox->setRange(0.1, 10);
    cutDepthSpinBox->setValue(1.0);
    cutDepthSpinBox->setSuffix(" mm");
    cutDepthLayout->addWidget(cutDepthSpinBox);
    groupLayout->addLayout(cutDepthLayout);
    
    // Pass count
    QHBoxLayout *passCountLayout = new QHBoxLayout();
    passCountLayout->addWidget(new QLabel("Pass Count:"));
    passCountSpinBox = new QSpinBox();
    passCountSpinBox->setRange(1, 50);
    passCountSpinBox->setValue(6);
    passCountLayout->addWidget(passCountSpinBox);
    groupLayout->addLayout(passCountLayout);
    
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
    
    tabLayout->addWidget(cutGroup);
    
    // Add a spacer at the bottom
    tabLayout->addStretch();
    
    // Add the tab to the tab widget
    tabWidget->addTab(cuttingTab, "Cutting");
    
    // Connect signals
    connect(safetyHeightCheckBox, &QCheckBox::toggled, safetyHeightSpinBox, &QDoubleSpinBox::setEnabled);
}

void GCodeOptionsPanel::setupTransformTab()
{
    QWidget *transformTab = new QWidget();
    QVBoxLayout *tabLayout = new QVBoxLayout(transformTab);
    
    // Path Transformation Group
    QGroupBox *transformGroup = new QGroupBox("Path Transformation", transformTab);
    QVBoxLayout *groupLayout = new QVBoxLayout(transformGroup);
    
    // Preserve aspect ratio option
    preserveAspectRatioCheckBox = new QCheckBox("Preserve Aspect Ratio");
    preserveAspectRatioCheckBox->setChecked(true);
    groupLayout->addWidget(preserveAspectRatioCheckBox);
    
    // Center X option
    centerXCheckBox = new QCheckBox("Center Horizontally");
    centerXCheckBox->setChecked(true);
    groupLayout->addWidget(centerXCheckBox);
    
    // Center Y option
    centerYCheckBox = new QCheckBox("Center Vertically");
    centerYCheckBox->setChecked(true);
    groupLayout->addWidget(centerYCheckBox);
    
    // Flip Y option
    flipYCheckBox = new QCheckBox("Flip Y Coordinates");
    flipYCheckBox->setChecked(true);
    groupLayout->addWidget(flipYCheckBox);
    
    tabLayout->addWidget(transformGroup);
    
    // G-Code Generation Options Group
    QGroupBox *gcodeGroup = new QGroupBox("G-Code Generation", transformTab);
    QVBoxLayout *gcodeLayout = new QVBoxLayout(gcodeGroup);
    
    // Optimize paths option
    optimizePathsCheckBox = new QCheckBox("Optimize Path Ordering");
    optimizePathsCheckBox->setChecked(true);
    gcodeLayout->addWidget(optimizePathsCheckBox);
    
    // Linearize paths option
    linearizePathsCheckBox = new QCheckBox("Combine Straight Lines");
    linearizePathsCheckBox->setChecked(true);
    gcodeLayout->addWidget(linearizePathsCheckBox);
    
    tabLayout->addWidget(gcodeGroup);
    
    // Add a spacer at the bottom
    tabLayout->addStretch();
    
    // Add the tab to the tab widget
    tabWidget->addTab(transformTab, "Transform");
}

void GCodeOptionsPanel::setupAdvancedTab()
{
    QWidget *advancedTab = new QWidget();
    QVBoxLayout *tabLayout = new QVBoxLayout(advancedTab);
    
    // Discretization Options Group
    QGroupBox *discretizationGroup = new QGroupBox("Discretization Options", advancedTab);
    QVBoxLayout *groupLayout = new QVBoxLayout(discretizationGroup);
    
    // Bezier samples
    QHBoxLayout *bezierLayout = new QHBoxLayout();
    bezierLayout->addWidget(new QLabel("Bezier Samples:"));
    bezierSamplesSpinBox = new QSpinBox();
    bezierSamplesSpinBox->setRange(4, 100);
    bezierSamplesSpinBox->setValue(12);
    bezierLayout->addWidget(bezierSamplesSpinBox);
    groupLayout->addLayout(bezierLayout);
    
    // Adaptive sampling
    QHBoxLayout *adaptiveLayout = new QHBoxLayout();
    adaptiveLayout->addWidget(new QLabel("Adaptive Sampling:"));
    adaptiveSpinBox = new QDoubleSpinBox();
    adaptiveSpinBox->setRange(0, 1.0);
    adaptiveSpinBox->setValue(0.1);
    adaptiveSpinBox->setSingleStep(0.01);
    adaptiveSpinBox->setDecimals(2);
    adaptiveLayout->addWidget(adaptiveSpinBox);
    groupLayout->addLayout(adaptiveLayout);
    
    // Simplify tolerance
    QHBoxLayout *simplifyLayout = new QHBoxLayout();
    simplifyLayout->addWidget(new QLabel("Simplify Tolerance:"));
    simplifySpinBox = new QDoubleSpinBox();
    simplifySpinBox->setRange(0, 1.0);
    simplifySpinBox->setValue(0.05);
    simplifySpinBox->setSingleStep(0.01);
    simplifySpinBox->setDecimals(2);
    simplifyLayout->addWidget(simplifySpinBox);
    groupLayout->addLayout(simplifyLayout);
    
    // Max point distance
    QHBoxLayout *maxDistanceLayout = new QHBoxLayout();
    maxDistanceLayout->addWidget(new QLabel("Max Point Distance:"));
    maxPointDistanceSpinBox = new QDoubleSpinBox();
    maxPointDistanceSpinBox->setRange(0.1, 10.0);
    maxPointDistanceSpinBox->setValue(1.0);
    maxPointDistanceSpinBox->setSuffix(" mm");
    maxDistanceLayout->addWidget(maxPointDistanceSpinBox);
    groupLayout->addLayout(maxDistanceLayout);
    
    tabLayout->addWidget(discretizationGroup);
    
    // Add a spacer at the bottom
    tabLayout->addStretch();
    
    // Add the tab to the tab widget
    tabWidget->addTab(advancedTab, "Advanced");
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
    
    // Update suffix for all dimension-related spinboxes
    if (isMetric) {
        // Machine settings
        bedWidthSpinBox->setSuffix(" mm");
        bedHeightSpinBox->setSuffix(" mm");
        
        // Material settings
        materialWidthSpinBox->setSuffix(" mm");
        materialHeightSpinBox->setSuffix(" mm");
        materialThicknessSpinBox->setSuffix(" mm");
        
        // Tool settings
        toolDiameterSpinBox->setSuffix(" mm");
        
        // Cutting settings
        feedRateSpinBox->setSuffix(" mm/min");
        plungeRateSpinBox->setSuffix(" mm/min");
        cutDepthSpinBox->setSuffix(" mm");
        safetyHeightSpinBox->setSuffix(" mm");
        
        // Advanced settings
        maxPointDistanceSpinBox->setSuffix(" mm");
    } else {
        // Machine settings
        bedWidthSpinBox->setSuffix(" in");
        bedHeightSpinBox->setSuffix(" in");
        
        // Material settings
        materialWidthSpinBox->setSuffix(" in");
        materialHeightSpinBox->setSuffix(" in");
        materialThicknessSpinBox->setSuffix(" in");
        
        // Tool settings
        toolDiameterSpinBox->setSuffix(" in");
        
        // Cutting settings
        feedRateSpinBox->setSuffix(" in/min");
        plungeRateSpinBox->setSuffix(" in/min");
        cutDepthSpinBox->setSuffix(" in");
        safetyHeightSpinBox->setSuffix(" in");
        
        // Advanced settings
        maxPointDistanceSpinBox->setSuffix(" in");
    }
    
    // Also would need to convert values here in a real implementation
    
    // Signal that options have changed
    emit optionsChanged();
}

void GCodeOptionsPanel::onMaterialChanged()
{
    recalculatePassCount();
}

void GCodeOptionsPanel::recalculatePassCount()
{
    // Calculate how many passes are needed based on material thickness and cut depth
    double materialThickness = materialThicknessSpinBox->value();
    double cutDepth = cutDepthSpinBox->value();
    
    if (cutDepth > 0) {
        int passes = qCeil(materialThickness / cutDepth);
        passCountSpinBox->setValue(passes);
    }
    
    emit optionsChanged();
}

// Getters for all settings

// Machine settings
double GCodeOptionsPanel::getBedWidth() const
{
    return bedWidthSpinBox->value();
}

double GCodeOptionsPanel::getBedHeight() const
{
    return bedHeightSpinBox->value();
}

bool GCodeOptionsPanel::isMetricUnits() const
{
    return metricUnitsCheckBox->isChecked();
}

// Material settings
double GCodeOptionsPanel::getMaterialWidth() const
{
    return materialWidthSpinBox->value();
}

double GCodeOptionsPanel::getMaterialHeight() const
{
    return materialHeightSpinBox->value();
}

double GCodeOptionsPanel::getMaterialThickness() const
{
    return materialThicknessSpinBox->value();
}

// Tool settings
QString GCodeOptionsPanel::getToolType() const
{
    return toolTypeComboBox->currentText();
}

double GCodeOptionsPanel::getToolDiameter() const
{
    return toolDiameterSpinBox->value();
}

int GCodeOptionsPanel::getToolFluteCount() const
{
    return toolFluteCountSpinBox->value();
}

// Cutting settings
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

double GCodeOptionsPanel::getCutDepth() const
{
    return cutDepthSpinBox->value();
}

int GCodeOptionsPanel::getPassCount() const
{
    return passCountSpinBox->value();
}

bool GCodeOptionsPanel::getSafetyHeightEnabled() const
{
    return safetyHeightCheckBox->isChecked();
}

double GCodeOptionsPanel::getSafetyHeight() const
{
    return safetyHeightSpinBox->value();
}

// Discretization options
int GCodeOptionsPanel::getBezierSamples() const
{
    return bezierSamplesSpinBox->value();
}

double GCodeOptionsPanel::getAdaptiveSampling() const
{
    return adaptiveSpinBox->value();
}

double GCodeOptionsPanel::getSimplifyTolerance() const
{
    return simplifySpinBox->value();
}

double GCodeOptionsPanel::getMaxPointDistance() const
{
    return maxPointDistanceSpinBox->value();
}

// Path transformation options
bool GCodeOptionsPanel::getPreserveAspectRatio() const
{
    return preserveAspectRatioCheckBox->isChecked();
}

bool GCodeOptionsPanel::getCenterX() const
{
    return centerXCheckBox->isChecked();
}

bool GCodeOptionsPanel::getCenterY() const
{
    return centerYCheckBox->isChecked();
}

bool GCodeOptionsPanel::getFlipY() const
{
    return flipYCheckBox->isChecked();
}

// G-Code generation options
bool GCodeOptionsPanel::getOptimizePaths() const
{
    return optimizePathsCheckBox->isChecked();
}

bool GCodeOptionsPanel::getLinearizePaths() const
{
    return linearizePathsCheckBox->isChecked();
}

double GCodeOptionsPanel::getPassDepth() const
{
    // This was renamed to getCutDepth in the new implementation
    return getCutDepth();
}

double GCodeOptionsPanel::getTotalDepth() const
{
    // Total depth is now material thickness
    return getMaterialThickness();
}   

void GCodeOptionsPanel::setupProfileButtons()
{
    // Create a horizontal layout for the profile buttons
    QHBoxLayout *profileLayout = new QHBoxLayout();
    
    // Save profile button
    saveProfileButton = new QPushButton("Save Profile");
    saveProfileButton->setToolTip("Save current settings to a profile file");
    connect(saveProfileButton, &QPushButton::clicked, this, &GCodeOptionsPanel::saveProfileAs);
    profileLayout->addWidget(saveProfileButton);
    
    // Load profile button
    loadProfileButton = new QPushButton("Load Profile");
    loadProfileButton->setToolTip("Load settings from a profile file");
    connect(loadProfileButton, &QPushButton::clicked, this, &GCodeOptionsPanel::loadProfile);
    profileLayout->addWidget(loadProfileButton);
    
    contentLayout->insertLayout(1, profileLayout);
}

void GCodeOptionsPanel::saveSettings()
{
    QSettings settings("NWSS", "NWSS-CNC");
    
    // Save Machine settings
    settings.beginGroup("MachineSettings");
    settings.setValue("BedWidth", bedWidthSpinBox->value());
    settings.setValue("BedHeight", bedHeightSpinBox->value());
    settings.setValue("MetricUnits", metricUnitsCheckBox->isChecked());
    settings.endGroup();
    
    // Save Material settings
    settings.beginGroup("MaterialSettings");
    settings.setValue("Width", materialWidthSpinBox->value());
    settings.setValue("Height", materialHeightSpinBox->value());
    settings.setValue("Thickness", materialThicknessSpinBox->value());
    settings.endGroup();
    
    // Save Tool settings
    settings.beginGroup("ToolSettings");
    settings.setValue("ToolType", toolTypeComboBox->currentText());
    settings.setValue("ToolDiameter", toolDiameterSpinBox->value());
    settings.setValue("FluteCount", toolFluteCountSpinBox->value());
    settings.endGroup();
    
    // Save Cutting settings
    settings.beginGroup("CuttingSettings");
    settings.setValue("FeedRate", feedRateSpinBox->value());
    settings.setValue("PlungeRate", plungeRateSpinBox->value());
    settings.setValue("SpindleSpeed", spindleSpeedSpinBox->value());
    settings.setValue("CutDepth", cutDepthSpinBox->value());
    settings.setValue("PassCount", passCountSpinBox->value());
    settings.setValue("SafetyHeightEnabled", safetyHeightCheckBox->isChecked());
    settings.setValue("SafetyHeight", safetyHeightSpinBox->value());
    settings.endGroup();
    
    // Save Discretization options
    settings.beginGroup("DiscretizationSettings");
    settings.setValue("BezierSamples", bezierSamplesSpinBox->value());
    settings.setValue("AdaptiveSampling", adaptiveSpinBox->value());
    settings.setValue("SimplifyTolerance", simplifySpinBox->value());
    settings.setValue("MaxPointDistance", maxPointDistanceSpinBox->value());
    settings.endGroup();
    
    // Save Path Transformation options
    settings.beginGroup("PathTransformationSettings");
    settings.setValue("PreserveAspectRatio", preserveAspectRatioCheckBox->isChecked());
    settings.setValue("CenterX", centerXCheckBox->isChecked());
    settings.setValue("CenterY", centerYCheckBox->isChecked());
    settings.setValue("FlipY", flipYCheckBox->isChecked());
    settings.endGroup();
    
    // Save G-Code Generation options
    settings.beginGroup("GCodeGenerationSettings");
    settings.setValue("OptimizePaths", optimizePathsCheckBox->isChecked());
    settings.setValue("LinearizePaths", linearizePathsCheckBox->isChecked());
    settings.endGroup();
}

void GCodeOptionsPanel::loadSettings()
{
    QSettings settings("NWSS", "NWSS-CNC");
    
    // Load Machine settings
    settings.beginGroup("MachineSettings");
    bedWidthSpinBox->setValue(settings.value("BedWidth", 300.0).toDouble());
    bedHeightSpinBox->setValue(settings.value("BedHeight", 200.0).toDouble());
    metricUnitsCheckBox->setChecked(settings.value("MetricUnits", true).toBool());
    settings.endGroup();
    
    // Load Material settings
    settings.beginGroup("MaterialSettings");
    materialWidthSpinBox->setValue(settings.value("Width", 100.0).toDouble());
    materialHeightSpinBox->setValue(settings.value("Height", 100.0).toDouble());
    materialThicknessSpinBox->setValue(settings.value("Thickness", 6.0).toDouble());
    settings.endGroup();
    
    // Load Tool settings
    settings.beginGroup("ToolSettings");
    toolTypeComboBox->setCurrentText(settings.value("ToolType", "End Mill").toString());
    toolDiameterSpinBox->setValue(settings.value("ToolDiameter", 3.175).toDouble());
    toolFluteCountSpinBox->setValue(settings.value("FluteCount", 2).toInt());
    settings.endGroup();
    
    // Load Cutting settings
    settings.beginGroup("CuttingSettings");
    feedRateSpinBox->setValue(settings.value("FeedRate", 500.0).toDouble());
    plungeRateSpinBox->setValue(settings.value("PlungeRate", 100.0).toDouble());
    spindleSpeedSpinBox->setValue(settings.value("SpindleSpeed", 10000.0).toDouble());
    cutDepthSpinBox->setValue(settings.value("CutDepth", 1.0).toDouble());
    passCountSpinBox->setValue(settings.value("PassCount", 6).toInt());
    safetyHeightCheckBox->setChecked(settings.value("SafetyHeightEnabled", true).toBool());
    safetyHeightSpinBox->setValue(settings.value("SafetyHeight", 5.0).toDouble());
    settings.endGroup();
    
    // Load Discretization options
    settings.beginGroup("DiscretizationSettings");
    bezierSamplesSpinBox->setValue(settings.value("BezierSamples", 12).toInt());
    adaptiveSpinBox->setValue(settings.value("AdaptiveSampling", 0.1).toDouble());
    simplifySpinBox->setValue(settings.value("SimplifyTolerance", 0.05).toDouble());
    maxPointDistanceSpinBox->setValue(settings.value("MaxPointDistance", 1.0).toDouble());
    settings.endGroup();
    
    // Load Path Transformation options
    settings.beginGroup("PathTransformationSettings");
    preserveAspectRatioCheckBox->setChecked(settings.value("PreserveAspectRatio", true).toBool());
    centerXCheckBox->setChecked(settings.value("CenterX", true).toBool());
    centerYCheckBox->setChecked(settings.value("CenterY", true).toBool());
    flipYCheckBox->setChecked(settings.value("FlipY", true).toBool());
    settings.endGroup();
    
    // Load G-Code Generation options
    settings.beginGroup("GCodeGenerationSettings");
    optimizePathsCheckBox->setChecked(settings.value("OptimizePaths", true).toBool());
    linearizePathsCheckBox->setChecked(settings.value("LinearizePaths", true).toBool());
    settings.endGroup();
    
    // Update UI
    updateUnitDisplay(metricUnitsCheckBox->isChecked());
    
    // Emit signal that settings were loaded
    emit settingsLoaded();
    emit optionsChanged();
}

void GCodeOptionsPanel::saveProfileAs()
{
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Save Settings Profile"), "",
        tr("NWSS-CNC Profile (*.ncp);;All Files (*)"));
        
    if (fileName.isEmpty())
        return;
        
    // Make sure the file has the correct extension
    if (!fileName.endsWith(".ncp", Qt::CaseInsensitive))
        fileName += ".ncp";
        
    QSettings profile(fileName, QSettings::IniFormat);
    
    // Save Machine settings
    profile.beginGroup("MachineSettings");
    profile.setValue("BedWidth", bedWidthSpinBox->value());
    profile.setValue("BedHeight", bedHeightSpinBox->value());
    profile.setValue("MetricUnits", metricUnitsCheckBox->isChecked());
    profile.endGroup();
    
    // Save Material settings
    profile.beginGroup("MaterialSettings");
    profile.setValue("Width", materialWidthSpinBox->value());
    profile.setValue("Height", materialHeightSpinBox->value());
    profile.setValue("Thickness", materialThicknessSpinBox->value());
    profile.endGroup();
    
    // Save Tool settings
    profile.beginGroup("ToolSettings");
    profile.setValue("ToolType", toolTypeComboBox->currentText());
    profile.setValue("ToolDiameter", toolDiameterSpinBox->value());
    profile.setValue("FluteCount", toolFluteCountSpinBox->value());
    profile.endGroup();
    
    // Save Cutting settings
    profile.beginGroup("CuttingSettings");
    profile.setValue("FeedRate", feedRateSpinBox->value());
    profile.setValue("PlungeRate", plungeRateSpinBox->value());
    profile.setValue("SpindleSpeed", spindleSpeedSpinBox->value());
    profile.setValue("CutDepth", cutDepthSpinBox->value());
    profile.setValue("PassCount", passCountSpinBox->value());
    profile.setValue("SafetyHeightEnabled", safetyHeightCheckBox->isChecked());
    profile.setValue("SafetyHeight", safetyHeightSpinBox->value());
    profile.endGroup();
    
    // Save Discretization options
    profile.beginGroup("DiscretizationSettings");
    profile.setValue("BezierSamples", bezierSamplesSpinBox->value());
    profile.setValue("AdaptiveSampling", adaptiveSpinBox->value());
    profile.setValue("SimplifyTolerance", simplifySpinBox->value());
    profile.setValue("MaxPointDistance", maxPointDistanceSpinBox->value());
    profile.endGroup();
    
    // Save Path Transformation options
    profile.beginGroup("PathTransformationSettings");
    profile.setValue("PreserveAspectRatio", preserveAspectRatioCheckBox->isChecked());
    profile.setValue("CenterX", centerXCheckBox->isChecked());
    profile.setValue("CenterY", centerYCheckBox->isChecked());
    profile.setValue("FlipY", flipYCheckBox->isChecked());
    profile.endGroup();
    
    // Save G-Code Generation options
    profile.beginGroup("GCodeGenerationSettings");
    profile.setValue("OptimizePaths", optimizePathsCheckBox->isChecked());
    profile.setValue("LinearizePaths", linearizePathsCheckBox->isChecked());
    profile.endGroup();
}

void GCodeOptionsPanel::loadProfile()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Load Settings Profile"), "",
        tr("NWSS-CNC Profile (*.ncp);;All Files (*)"));
        
    if (fileName.isEmpty())
        return;
        
    QSettings profile(fileName, QSettings::IniFormat);
    
    // Load Machine settings
    profile.beginGroup("MachineSettings");
    bedWidthSpinBox->setValue(profile.value("BedWidth", bedWidthSpinBox->value()).toDouble());
    bedHeightSpinBox->setValue(profile.value("BedHeight", bedHeightSpinBox->value()).toDouble());
    metricUnitsCheckBox->setChecked(profile.value("MetricUnits", metricUnitsCheckBox->isChecked()).toBool());
    profile.endGroup();
    
    // Load Material settings
    profile.beginGroup("MaterialSettings");
    materialWidthSpinBox->setValue(profile.value("Width", materialWidthSpinBox->value()).toDouble());
    materialHeightSpinBox->setValue(profile.value("Height", materialHeightSpinBox->value()).toDouble());
    materialThicknessSpinBox->setValue(profile.value("Thickness", materialThicknessSpinBox->value()).toDouble());
    profile.endGroup();
    
    // Load Tool settings
    profile.beginGroup("ToolSettings");
    toolTypeComboBox->setCurrentText(profile.value("ToolType", toolTypeComboBox->currentText()).toString());
    toolDiameterSpinBox->setValue(profile.value("ToolDiameter", toolDiameterSpinBox->value()).toDouble());
    toolFluteCountSpinBox->setValue(profile.value("FluteCount", toolFluteCountSpinBox->value()).toInt());
    profile.endGroup();
    
    // Load Cutting settings
    profile.beginGroup("CuttingSettings");
    feedRateSpinBox->setValue(profile.value("FeedRate", feedRateSpinBox->value()).toDouble());
    plungeRateSpinBox->setValue(profile.value("PlungeRate", plungeRateSpinBox->value()).toDouble());
    spindleSpeedSpinBox->setValue(profile.value("SpindleSpeed", spindleSpeedSpinBox->value()).toDouble());
    cutDepthSpinBox->setValue(profile.value("CutDepth", cutDepthSpinBox->value()).toDouble());
    passCountSpinBox->setValue(profile.value("PassCount", passCountSpinBox->value()).toInt());
    safetyHeightCheckBox->setChecked(profile.value("SafetyHeightEnabled", safetyHeightCheckBox->isChecked()).toBool());
    safetyHeightSpinBox->setValue(profile.value("SafetyHeight", safetyHeightSpinBox->value()).toDouble());
    profile.endGroup();
    
    // Load Discretization options
    profile.beginGroup("DiscretizationSettings");
    bezierSamplesSpinBox->setValue(profile.value("BezierSamples", bezierSamplesSpinBox->value()).toInt());
    adaptiveSpinBox->setValue(profile.value("AdaptiveSampling", adaptiveSpinBox->value()).toDouble());
    simplifySpinBox->setValue(profile.value("SimplifyTolerance", simplifySpinBox->value()).toDouble());
    maxPointDistanceSpinBox->setValue(profile.value("MaxPointDistance", maxPointDistanceSpinBox->value()).toDouble());
    profile.endGroup();
    
    // Load Path Transformation options
    profile.beginGroup("PathTransformationSettings");
    preserveAspectRatioCheckBox->setChecked(profile.value("PreserveAspectRatio", preserveAspectRatioCheckBox->isChecked()).toBool());
    centerXCheckBox->setChecked(profile.value("CenterX", centerXCheckBox->isChecked()).toBool());
    centerYCheckBox->setChecked(profile.value("CenterY", centerYCheckBox->isChecked()).toBool());
    flipYCheckBox->setChecked(profile.value("FlipY", flipYCheckBox->isChecked()).toBool());
    profile.endGroup();
    
    // Load G-Code Generation options
    profile.beginGroup("GCodeGenerationSettings");
    optimizePathsCheckBox->setChecked(profile.value("OptimizePaths", optimizePathsCheckBox->isChecked()).toBool());
    linearizePathsCheckBox->setChecked(profile.value("LinearizePaths", linearizePathsCheckBox->isChecked()).toBool());
    profile.endGroup();
    
    // Update UI
    updateUnitDisplay(metricUnitsCheckBox->isChecked());
    
    // Emit signal that settings were loaded
    emit settingsLoaded();
    emit optionsChanged();
}