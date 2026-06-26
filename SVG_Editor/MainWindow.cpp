// =====================================================================
// MainWindow.cpp
// ---------------------------------------------------------------------
// @brief MainWindow 的实现
// @details 启动顺序（构造时）：
//          loadLanguage() → setupUi → setupActions → setupToolbar → setupMenus
//          → connectSignals → setLanguage(m_language) → 状态栏 "Ready"
// @layer   ui
// =====================================================================

#include "MainWindow.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QGuiApplication>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>
#include <QStatusBar>
#include <QStyleHints>
#include <QToolBar>

#include "FileManager.h"
#include "PropertyPanel.h"
#include "ShapeItem.h"
#include "ThemeUtils.h"
#include "TutorialDialog.h"

namespace {

/// @brief 二选一返回字符串。
QString textForLanguage(AppLanguage language, const QString& english, const QString& chinese) {
    return language == AppLanguage::SimplifiedChinese ? chinese : english;
}

/// @brief 工具枚举的本地化标签（与 shapeDisplayName 类似，但语言切换时是 QAction 文本）。
QString toolLabel(CanvasView::Tool tool, AppLanguage language) {
    switch (tool) {
    case CanvasView::Tool::Select:
        return textForLanguage(language, "Select", "选择");
    case CanvasView::Tool::Point:
        return textForLanguage(language, "Point", "点");
    case CanvasView::Tool::Line:
        return textForLanguage(language, "Line", "直线");
    case CanvasView::Tool::Polyline:
        return textForLanguage(language, "Polyline", "折线");
    case CanvasView::Tool::Circle:
        return textForLanguage(language, "Circle", "圆");
    case CanvasView::Tool::Ellipse:
        return textForLanguage(language, "Ellipse", "椭圆");
    case CanvasView::Tool::Rectangle:
        return textForLanguage(language, "Rectangle", "矩形");
    case CanvasView::Tool::Polygon:
        return textForLanguage(language, "Polygon", "多边形");
    }
    return textForLanguage(language, "Unknown", "未知");
}

} // namespace

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    // 1) 读取上次保存的语言（默认 zh-CN）
    m_language = loadLanguage();
    m_themeMode = loadThemeMode();
    // 2) 构造 UI 子模块与 dock
    setupUi();
    // 3) 创建所有 QAction 与 ActionGroup（含快捷键）
    setupActions();
    // 4) 工具栏（先于菜单：工具按钮的 checked 状态需要 ActionGroup 已有成员）
    setupToolbar();
    // 5) 菜单栏
    setupMenus();
    // 6) 桥接所有信号槽
    connectSignals();
    setThemeMode(m_themeMode);
    // 7) 通知所有子模块切换语言并刷新自身
    setLanguage(m_language);
    statusBar()->showMessage(textForLanguage(m_language, "Ready", "就绪"));
}

AppLanguage MainWindow::loadLanguage() const {
    const QSettings settings;
    const QString value = settings.value("ui/language", "zh-CN").toString();
    // "en" 视为英文；其他（含缺失、zh-CN、其他）一律回退到简体中文
    return value == "en" ? AppLanguage::English : AppLanguage::SimplifiedChinese;
}

void MainWindow::saveLanguage() const {
    QSettings settings;
    settings.setValue("ui/language", m_language == AppLanguage::English ? "en" : "zh-CN");
}

ThemeMode MainWindow::loadThemeMode() const {
    const QSettings settings;
    return themeModeFromSettingsValue(settings.value("ui/themeMode", "system").toString());
}

void MainWindow::saveThemeMode() const {
    QSettings settings;
    settings.setValue("ui/themeMode", themeModeToSettingsValue(m_themeMode));
}

void MainWindow::setLanguage(AppLanguage language) {
    m_language = language;
    saveLanguage();

    // 通知子模块
    m_canvasView->setLanguage(language);
    m_propertyPanel->setLanguage(language);
    m_tutorialDialog->setLanguage(language);

    // 同步语言菜单的勾选态
    if (m_englishAction != nullptr) {
        m_englishAction->setChecked(language == AppLanguage::English);
    }
    if (m_simplifiedChineseAction != nullptr) {
        m_simplifiedChineseAction->setChecked(language == AppLanguage::SimplifiedChinese);
    }

    retranslateUi();
    statusBar()->showMessage(textForLanguage(m_language, "Language updated.", "界面语言已更新。"), 2500);
}

void MainWindow::setThemeMode(ThemeMode mode) {
    m_themeMode = mode;
    saveThemeMode();
    applyApplicationTheme(*static_cast<QApplication*>(QApplication::instance()), mode);

    if (m_themeSystemAction != nullptr) {
        m_themeSystemAction->setChecked(mode == ThemeMode::System);
    }
    if (m_themeLightAction != nullptr) {
        m_themeLightAction->setChecked(mode == ThemeMode::Light);
    }
    if (m_themeDarkAction != nullptr) {
        m_themeDarkAction->setChecked(mode == ThemeMode::Dark);
    }
}

void MainWindow::setupUi() {
    resize(1400, 900);
    setMinimumSize(1100, 720);

    m_canvasView = new CanvasView(this);
    setCentralWidget(m_canvasView);

    m_propertyPanel = new PropertyPanel(this);
    m_propertyPanel->setMinimumWidth(300);
    m_propertyDock = new QDockWidget(this);
    m_propertyDock->setObjectName("propertyDock");
    m_propertyDock->setMinimumWidth(320);
    // 属性面板允许左右停靠
    m_propertyDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_propertyDock->setWidget(m_propertyPanel);
    addDockWidget(Qt::RightDockWidgetArea, m_propertyDock);
    resizeDocks({m_propertyDock}, {340}, Qt::Horizontal);
    m_togglePropertyDockAction = m_propertyDock->toggleViewAction();

    m_tutorialDialog = new TutorialDialog(this);
}

void MainWindow::setupActions() {
    m_openAction = new QAction(this);
    m_saveAction = new QAction(this);
    m_saveAsAction = new QAction(this);
    m_exportAction = new QAction(this);
    m_exitAction = new QAction(this);
    m_deleteAction = new QAction(this);
    m_copyAction = new QAction(this);
    m_pasteAction = new QAction(this);
    m_clearAction = new QAction(this);
    m_showTutorialAction = new QAction(this);
    m_themeSystemAction = new QAction(this);
    m_themeLightAction = new QAction(this);
    m_themeDarkAction = new QAction(this);
    m_englishAction = new QAction(this);
    m_simplifiedChineseAction = new QAction(this);

    // 用 Qt 标准快捷键（平台自适应：macOS 走 Cmd，其余走 Ctrl）
    m_openAction->setShortcut(QKeySequence::Open);
    m_saveAction->setShortcut(QKeySequence::Save);
    m_saveAsAction->setShortcut(QKeySequence::SaveAs);
    m_deleteAction->setShortcut(QKeySequence::Delete);
    m_copyAction->setShortcut(QKeySequence::Copy);
    m_pasteAction->setShortcut(QKeySequence::Paste);
    m_exitAction->setShortcut(QKeySequence::Quit);

    m_toolActionGroup = new QActionGroup(this);
    m_toolActionGroup->setExclusive(true);

    m_themeActionGroup = new QActionGroup(this);
    m_themeActionGroup->setExclusive(true);
    m_themeSystemAction->setCheckable(true);
    m_themeLightAction->setCheckable(true);
    m_themeDarkAction->setCheckable(true);
    m_themeActionGroup->addAction(m_themeSystemAction);
    m_themeActionGroup->addAction(m_themeLightAction);
    m_themeActionGroup->addAction(m_themeDarkAction);

    m_languageActionGroup = new QActionGroup(this);
    m_languageActionGroup->setExclusive(true);
    m_englishAction->setCheckable(true);
    m_simplifiedChineseAction->setCheckable(true);
    m_languageActionGroup->addAction(m_englishAction);
    m_languageActionGroup->addAction(m_simplifiedChineseAction);
}

void MainWindow::setupMenus() {
    m_fileMenu = menuBar()->addMenu(QString());
    m_fileMenu->addAction(m_openAction);
    m_fileMenu->addAction(m_saveAction);
    m_fileMenu->addAction(m_saveAsAction);
    m_fileMenu->addAction(m_exportAction);
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_exitAction);

    m_editMenu = menuBar()->addMenu(QString());
    m_editMenu->addAction(m_copyAction);
    m_editMenu->addAction(m_pasteAction);
    m_editMenu->addAction(m_deleteAction);
    m_editMenu->addAction(m_clearAction);

    m_viewMenu = menuBar()->addMenu(QString());
    m_viewMenu->addAction(m_togglePropertyDockAction);
    m_themeMenu = m_viewMenu->addMenu(QString());
    m_themeMenu->addAction(m_themeSystemAction);
    m_themeMenu->addAction(m_themeLightAction);
    m_themeMenu->addAction(m_themeDarkAction);

    m_toolMenu = menuBar()->addMenu(QString());
    m_selectionToolMenu = m_toolMenu->addMenu(QString());
    m_selectionToolMenu->addAction(findToolAction(CanvasView::Tool::Select));

    m_openShapeToolMenu = m_toolMenu->addMenu(QString());
    m_openShapeToolMenu->addAction(findToolAction(CanvasView::Tool::Point));
    m_openShapeToolMenu->addAction(findToolAction(CanvasView::Tool::Line));
    m_openShapeToolMenu->addAction(findToolAction(CanvasView::Tool::Polyline));

    m_closedShapeToolMenu = m_toolMenu->addMenu(QString());
    m_closedShapeToolMenu->addAction(findToolAction(CanvasView::Tool::Circle));
    m_closedShapeToolMenu->addAction(findToolAction(CanvasView::Tool::Ellipse));
    m_closedShapeToolMenu->addAction(findToolAction(CanvasView::Tool::Rectangle));
    m_closedShapeToolMenu->addAction(findToolAction(CanvasView::Tool::Polygon));

    // Tutorial 菜单标题在英文 / 中文下都保留 "Tutorial"（与 HTML 手册保持一致）
    m_tutorialMenu = menuBar()->addMenu("Tutorial");
    m_tutorialMenu->addAction(m_showTutorialAction);
    m_tutorialMenu->addSeparator();
    m_languageMenu = m_tutorialMenu->addMenu(QString());
    m_languageMenu->addAction(m_englishAction);
    m_languageMenu->addAction(m_simplifiedChineseAction);
}

void MainWindow::setupToolbar() {
    m_toolBar = addToolBar(QString());
    m_toolBar->setMovable(false);

    // 顶层工具栏按类别分段：选择 -> 开放图形 -> 封闭图形 -> 编辑 -> 文件 -> 面板
    m_toolBar->addAction(createToolAction(CanvasView::Tool::Select));
    m_toolBar->addSeparator();
    m_toolBar->addAction(createToolAction(CanvasView::Tool::Point));
    m_toolBar->addAction(createToolAction(CanvasView::Tool::Line));
    m_toolBar->addAction(createToolAction(CanvasView::Tool::Polyline));
    m_toolBar->addSeparator();
    m_toolBar->addAction(createToolAction(CanvasView::Tool::Circle));
    m_toolBar->addAction(createToolAction(CanvasView::Tool::Ellipse));
    m_toolBar->addAction(createToolAction(CanvasView::Tool::Rectangle));
    m_toolBar->addAction(createToolAction(CanvasView::Tool::Polygon));
    m_toolBar->addSeparator();
    // 常用动作快捷入口
    m_toolBar->addAction(m_deleteAction);
    m_toolBar->addAction(m_copyAction);
    m_toolBar->addAction(m_pasteAction);
    m_toolBar->addAction(m_openAction);
    m_toolBar->addAction(m_saveAction);
    m_toolBar->addAction(m_clearAction);
    m_toolBar->addSeparator();
    m_toolBar->addAction(m_togglePropertyDockAction);

    // 默认选中 Select 工具（ActionGroup 内的第一个）
    if (!m_toolActionGroup->actions().isEmpty()) {
        m_toolActionGroup->actions().first()->setChecked(true);
    }
}

void MainWindow::connectSignals() {
    // File 菜单
    connect(m_openAction, &QAction::triggered, this, &MainWindow::openDocument);
    connect(m_saveAction, &QAction::triggered, this, [this]() { saveDocument(); });
    connect(m_saveAsAction, &QAction::triggered, this, [this]() { saveDocumentAs(); });
    connect(m_exportAction, &QAction::triggered, this, &MainWindow::exportImage);
    connect(m_exitAction, &QAction::triggered, this, &QWidget::close);

    // Tutorial 菜单
    connect(m_showTutorialAction, &QAction::triggered, this, &MainWindow::showTutorial);

    // Edit 菜单（直接桥到 CanvasView）
    connect(m_deleteAction, &QAction::triggered, this, &MainWindow::deleteSelection);
    connect(m_copyAction, &QAction::triggered, m_canvasView, &CanvasView::copySelectedItem);
    connect(m_pasteAction, &QAction::triggered, m_canvasView, &CanvasView::pasteCopiedItem);
    // Clear Canvas 还需要清空当前文件路径和更新窗口标题，所以在 MainWindow 内联处理
    connect(m_clearAction, &QAction::triggered, this, [this]() {
        m_canvasView->clearCanvas();
        m_currentFilePath.clear();
        updateWindowTitle();
    });

    // 画布 → 属性面板 / 状态栏
    connect(m_canvasView, &CanvasView::selectionStateChanged, this, [this](ShapeItem* item, int selectedCount) {
        if (selectedCount > 1) {
            m_propertyPanel->setMultipleSelection(selectedCount);
        } else if (item != nullptr) {
            m_propertyPanel->setShapeData(item->shapeData());
        } else {
            m_propertyPanel->clearSelection();
        }
    });

    connect(m_canvasView, &CanvasView::statusMessageChanged, this,
            [this](const QString& message) { statusBar()->showMessage(message, 2500); });

    // 属性面板 → 画布（回写修改）
    connect(m_propertyPanel, &PropertyPanel::shapeEdited, m_canvasView, &CanvasView::updateSelectedShape);

    connect(m_canvasView, &CanvasView::deleteSelectionRequested, this, &MainWindow::deleteSelection);

    // 语言菜单
    connect(m_englishAction, &QAction::triggered, this, [this]() { setLanguage(AppLanguage::English); });
    connect(m_simplifiedChineseAction, &QAction::triggered, this,
            [this]() { setLanguage(AppLanguage::SimplifiedChinese); });

    connect(m_themeSystemAction, &QAction::triggered, this, [this]() { setThemeMode(ThemeMode::System); });
    connect(m_themeLightAction, &QAction::triggered, this, [this]() { setThemeMode(ThemeMode::Light); });
    connect(m_themeDarkAction, &QAction::triggered, this, [this]() { setThemeMode(ThemeMode::Dark); });

    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, this, [this](Qt::ColorScheme) {
        if (m_themeMode == ThemeMode::System) {
            applyApplicationTheme(*static_cast<QApplication*>(QApplication::instance()), ThemeMode::System);
        }
    });
}

void MainWindow::retranslateUi() {
    m_fileMenu->setTitle(textForLanguage(m_language, "File", "文件"));
    m_editMenu->setTitle(textForLanguage(m_language, "Edit", "编辑"));
    m_viewMenu->setTitle(textForLanguage(m_language, "View", "视图"));
    m_toolMenu->setTitle(textForLanguage(m_language, "Tools", "工具"));
    m_selectionToolMenu->setTitle(textForLanguage(m_language, "Selection", "选择工具"));
    m_openShapeToolMenu->setTitle(textForLanguage(m_language, "Open Shapes", "开放图形"));
    m_closedShapeToolMenu->setTitle(textForLanguage(m_language, "Closed Shapes", "封闭图形"));
    m_themeMenu->setTitle(textForLanguage(m_language, "Theme", "主题"));
    // Tutorial 标题保持原样（与 HTML 手册标题一致）
    m_tutorialMenu->setTitle("Tutorial");
    m_languageMenu->setTitle(textForLanguage(m_language, "Language", "语言"));

    m_openAction->setText(textForLanguage(m_language, "Open", "打开"));
    m_saveAction->setText(textForLanguage(m_language, "Save", "保存"));
    m_saveAsAction->setText(textForLanguage(m_language, "Save As", "另存为"));
    m_exportAction->setText(textForLanguage(m_language, "Export Image", "导出图片"));
    m_exitAction->setText(textForLanguage(m_language, "Exit", "退出"));
    m_deleteAction->setText(textForLanguage(m_language, "Delete", "删除"));
    m_copyAction->setText(textForLanguage(m_language, "Copy", "复制"));
    m_pasteAction->setText(textForLanguage(m_language, "Paste", "粘贴"));
    m_clearAction->setText(textForLanguage(m_language, "Clear Canvas", "清空画布"));
    m_showTutorialAction->setText(textForLanguage(m_language, "Operation Manual", "操作手册"));
    if (m_togglePropertyDockAction != nullptr) {
        m_togglePropertyDockAction->setText(textForLanguage(m_language, "Properties Panel", "属性面板"));
    }
    m_themeSystemAction->setText(textForLanguage(m_language, "Auto Theme", "自动主题"));
    m_themeLightAction->setText(textForLanguage(m_language, "Light", "浅色"));
    m_themeDarkAction->setText(textForLanguage(m_language, "Dark", "深色"));
    m_englishAction->setText("English");
    m_simplifiedChineseAction->setText(QString::fromUtf8("简体中文"));

    // 工具按钮文本（按 QAction::data() 存取的 Tool 枚举值回查）
    for (QAction* action : m_toolActionGroup->actions()) {
        const QVariant data = action->data();
        if (data.isValid()) {
            action->setText(toolLabel(static_cast<CanvasView::Tool>(data.toInt()), m_language));
        }
    }

    m_propertyDock->setWindowTitle(textForLanguage(m_language, "Properties", "属性"));
    m_toolBar->setWindowTitle(textForLanguage(m_language, "Tools", "工具"));
    updateWindowTitle();
}

void MainWindow::updateWindowTitle() {
    const QString fileName = m_currentFilePath.isEmpty() ? textForLanguage(m_language, "Untitled", "未命名")
                                                         : QFileInfo(m_currentFilePath).fileName();
    setWindowTitle(QString("%1 - %2").arg(fileName, textForLanguage(m_language, "SVG Editor", "SVG 编辑器")));
}

void MainWindow::showTutorial() {
    m_tutorialDialog->setLanguage(m_language);
    m_tutorialDialog->exec();
}

void MainWindow::deleteSelection() {
    const int selectedCount = m_canvasView->selectedShapeCount();
    if (selectedCount <= 0) {
        return;
    }

    if (selectedCount > 1) {
        const QMessageBox::StandardButton reply = QMessageBox::question(
            this, textForLanguage(m_language, "Delete Multiple Shapes", "删除多个图形"),
            textForLanguage(m_language, "Delete %1 selected shapes?", "确定删除已选中的 %1 个图形吗？")
                .arg(selectedCount),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (reply != QMessageBox::Yes) {
            return;
        }
    }

    m_canvasView->deleteSelectedItems();
}

QAction* MainWindow::createToolAction(CanvasView::Tool tool) {
    QAction* action = new QAction(this);
    action->setCheckable(true);
    // 把 Tool 枚举值存到 action->data()，供 retranslateUi 时回查
    action->setData(static_cast<int>(tool));
    m_toolActionGroup->addAction(action);

    connect(action, &QAction::triggered, this, [this, tool]() { m_canvasView->setTool(tool); });

    return action;
}

QAction* MainWindow::findToolAction(CanvasView::Tool tool) const {
    for (QAction* action : m_toolActionGroup->actions()) {
        if (action != nullptr && action->data().toInt() == static_cast<int>(tool)) {
            return action;
        }
    }

    return nullptr;
}

bool MainWindow::saveToPath(const QString& filePath) {
    QString errorMessage;
    if (!FileManager::saveToFile(filePath, m_canvasView->documentData(), &errorMessage)) {
        QMessageBox::warning(this, textForLanguage(m_language, "Save Failed", "保存失败"), errorMessage);
        return false;
    }

    m_currentFilePath = filePath;
    updateWindowTitle();
    statusBar()->showMessage(textForLanguage(m_language, "File saved.", "文件已保存。"), 2500);
    return true;
}

bool MainWindow::saveDocument() {
    if (m_currentFilePath.isEmpty()) {
        // 还没保存过 → 走另存为
        return saveDocumentAs();
    }

    return saveToPath(m_currentFilePath);
}

bool MainWindow::saveDocumentAs() {
    const QString filePath = QFileDialog::getSaveFileName(
        this, textForLanguage(m_language, "Save Vector Document", "保存矢量图文件"),
        m_currentFilePath.isEmpty() ? "drawing.vgjson" : m_currentFilePath,
        textForLanguage(m_language, "Vector JSON (*.vgjson *.json)", "矢量 JSON 文件 (*.vgjson *.json)"));

    if (filePath.isEmpty()) {
        // 用户取消
        return false;
    }

    return saveToPath(filePath);
}

void MainWindow::openDocument() {
    const QString filePath = QFileDialog::getOpenFileName(
        this, textForLanguage(m_language, "Open Vector Document", "打开矢量图文件"), QString(),
        textForLanguage(m_language, "Vector JSON (*.vgjson *.json)", "矢量 JSON 文件 (*.vgjson *.json)"));
    if (filePath.isEmpty()) {
        return;
    }

    QString errorMessage;
    const std::optional<DocumentData> document = FileManager::loadFromFile(filePath, &errorMessage);
    if (!document.has_value()) {
        QMessageBox::warning(this, textForLanguage(m_language, "Open Failed", "打开失败"), errorMessage);
        return;
    }

    m_canvasView->loadDocument(*document);
    m_currentFilePath = filePath;
    updateWindowTitle();
    statusBar()->showMessage(textForLanguage(m_language, "File loaded.", "文件已加载。"), 2500);
}

void MainWindow::exportImage() {
    const QString filePath =
        QFileDialog::getSaveFileName(this, textForLanguage(m_language, "Export Image", "导出图片"), "canvas.png",
                                     textForLanguage(m_language, "PNG Image (*.png)", "PNG 图片 (*.png)"));
    if (filePath.isEmpty()) {
        return;
    }

    QString errorMessage;
    if (!m_canvasView->exportImage(filePath, &errorMessage)) {
        QMessageBox::warning(this, textForLanguage(m_language, "Export Failed", "导出失败"), errorMessage);
        return;
    }

    statusBar()->showMessage(textForLanguage(m_language, "Image exported.", "图片已导出。"), 2500);
}
