#include "ui/MainWindow.h"

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

#include "core/FileManager.h"
#include "core/I18n.h"
#include "ui/MainWindowActions.h"
#include "ui/PropertyPanel.h"
#include "graphics/ShapeItem.h"
#include "ui/ThemeUtils.h"
#include "ui/TutorialDialog.h"

namespace {

QString toolLabel(CanvasView::Tool tool, AppLanguage language) {
    const auto& table = i18n::shapeDisplayNames();
    switch (tool) {
    case CanvasView::Tool::Select:
        return i18n::tr(language, "tool.select", "Select", "选择");
    case CanvasView::Tool::Point:
        return pickLocalized(language, "tool.point", table[0]);
    case CanvasView::Tool::Line:
        return pickLocalized(language, "tool.line", table[1]);
    case CanvasView::Tool::Polyline:
        return pickLocalized(language, "tool.polyline", table[2]);
    case CanvasView::Tool::Circle:
        return pickLocalized(language, "tool.circle", table[3]);
    case CanvasView::Tool::Ellipse:
        return pickLocalized(language, "tool.ellipse", table[4]);
    case CanvasView::Tool::Rectangle:
        return pickLocalized(language, "tool.rectangle", table[5]);
    case CanvasView::Tool::Polygon:
        return pickLocalized(language, "tool.polygon", table[6]);
    }

    return i18n::tr(language, "tool.unknown", "Unknown", "未知");
}

} // namespace

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    m_language = loadLanguage();
    m_themeMode = loadThemeMode();
    setupUi();
    setupActions();
    setupToolbar();
    setupMenus();
    connectSignals();
    setThemeMode(m_themeMode);
    setLanguage(m_language);
    statusBar()->showMessage(i18n::tr(m_language, "status.ready", "Ready", "就绪"));
}

MainWindow::~MainWindow() {
    disconnect(m_canvasView, nullptr, this, nullptr);
    disconnect(m_canvasView, nullptr, m_propertyPanel, nullptr);
}

AppLanguage MainWindow::loadLanguage() const {
    const QSettings settings;
    return appLanguageFromSettingsValue(settings.value("ui/language", appLanguageToSettingsValue(kDefaultLanguage)).toString());
}

void MainWindow::saveLanguage() const {
    QSettings settings;
    settings.setValue("ui/language", appLanguageToSettingsValue(m_language));
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

    m_canvasView->setLanguage(language);
    m_propertyPanel->setLanguage(language);
    m_tutorialDialog->setLanguage(language);

    if (m_englishAction != nullptr) {
        m_englishAction->setChecked(language == AppLanguage::English);
    }
    if (m_simplifiedChineseAction != nullptr) {
        m_simplifiedChineseAction->setChecked(language == AppLanguage::SimplifiedChinese);
    }

    retranslateUi();
    statusBar()->showMessage(i18n::tr(m_language, "status.language_updated", "Language updated.", "界面语言已更新。"), 2500);
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
    m_propertyDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_propertyDock->setWidget(m_propertyPanel);
    addDockWidget(Qt::RightDockWidgetArea, m_propertyDock);
    resizeDocks({m_propertyDock}, {340}, Qt::Horizontal);
    m_togglePropertyDockAction = m_propertyDock->toggleViewAction();

    m_tutorialDialog = new TutorialDialog(this);
}

void MainWindow::setupActions() {
    m_toolActionGroup = new QActionGroup(this);
    m_toolActionGroup->setExclusive(true);

    m_themeActionGroup = new QActionGroup(this);
    m_themeActionGroup->setExclusive(true);

    m_languageActionGroup = new QActionGroup(this);
    m_languageActionGroup->setExclusive(true);

    const QList<ActionDescriptor> descriptors = {
        {"open", QKeySequence::Open, false, {}, [this]() { openDocument(); }},
        {"save", QKeySequence::Save, false, {}, [this]() { saveDocument(); }},
        {"save_as", QKeySequence::SaveAs, false, {}, [this]() { saveDocumentAs(); }},
        {"export", std::nullopt, false, {}, [this]() { exportImage(); }},
        {"exit", QKeySequence::Quit, false, {}, [this]() { close(); }},
        {"delete", QKeySequence::Delete, false, {}, [this]() { deleteSelection(); }},
        {"copy", QKeySequence::Copy, false, {}, [this]() { m_canvasView->copySelectedItem(); }},
        {"paste", QKeySequence::Paste, false, {}, [this]() { m_canvasView->pasteCopiedItem(); }},
        {"clear", std::nullopt, false, {}, [this]() {
             m_canvasView->clearCanvas();
             m_currentFilePath.clear();
             updateWindowTitle();
         }},
        {"tutorial", std::nullopt, false, {}, [this]() { showTutorial(); }},
        {"theme_system", std::nullopt, true, {}, [this]() { setThemeMode(ThemeMode::System); }},
        {"theme_light", std::nullopt, true, {}, [this]() { setThemeMode(ThemeMode::Light); }},
        {"theme_dark", std::nullopt, true, {}, [this]() { setThemeMode(ThemeMode::Dark); }},
        {"language_en", std::nullopt, true, {}, [this]() { setLanguage(AppLanguage::English); }},
        {"language_zh", std::nullopt, true, {}, [this]() { setLanguage(AppLanguage::SimplifiedChinese); }},
    };

    setupActionsFromDescriptors(this, descriptors, [this](const char* id, QAction* action) {
        const QString key = QString::fromLatin1(id);
        if (key == "open") {
            m_openAction = action;
        } else if (key == "save") {
            m_saveAction = action;
        } else if (key == "save_as") {
            m_saveAsAction = action;
        } else if (key == "export") {
            m_exportAction = action;
        } else if (key == "exit") {
            m_exitAction = action;
        } else if (key == "delete") {
            m_deleteAction = action;
        } else if (key == "copy") {
            m_copyAction = action;
        } else if (key == "paste") {
            m_pasteAction = action;
        } else if (key == "clear") {
            m_clearAction = action;
        } else if (key == "tutorial") {
            m_showTutorialAction = action;
        } else if (key == "theme_system") {
            m_themeSystemAction = action;
        } else if (key == "theme_light") {
            m_themeLightAction = action;
        } else if (key == "theme_dark") {
            m_themeDarkAction = action;
        } else if (key == "language_en") {
            m_englishAction = action;
        } else if (key == "language_zh") {
            m_simplifiedChineseAction = action;
        }
    });

    m_themeActionGroup->addAction(m_themeSystemAction);
    m_themeActionGroup->addAction(m_themeLightAction);
    m_themeActionGroup->addAction(m_themeDarkAction);
    m_languageActionGroup->addAction(m_englishAction);
    m_languageActionGroup->addAction(m_simplifiedChineseAction);

    for (const CanvasView::Tool tool :
         {CanvasView::Tool::Select, CanvasView::Tool::Point, CanvasView::Tool::Line, CanvasView::Tool::Polyline,
          CanvasView::Tool::Circle, CanvasView::Tool::Ellipse, CanvasView::Tool::Rectangle, CanvasView::Tool::Polygon}) {
        createToolAction(tool);
    }
}

void MainWindow::setupMenus() {
    m_fileMenu = menuBar()->addMenu(QString());
    m_editMenu = menuBar()->addMenu(QString());
    m_viewMenu = menuBar()->addMenu(QString());
    m_themeMenu = new QMenu(this);
    m_toolMenu = menuBar()->addMenu(QString());
    m_selectionToolMenu = new QMenu(this);
    m_openShapeToolMenu = new QMenu(this);
    m_closedShapeToolMenu = new QMenu(this);
    m_tutorialMenu = menuBar()->addMenu("Tutorial");
    m_languageMenu = new QMenu(this);

    populateMenu({m_fileMenu,
                  {actionItem(m_openAction), actionItem(m_saveAction), actionItem(m_saveAsAction), actionItem(m_exportAction),
                   separatorItem(), actionItem(m_exitAction)}});
    populateMenu({m_editMenu, {actionItem(m_copyAction), actionItem(m_pasteAction), actionItem(m_deleteAction), actionItem(m_clearAction)}});
    populateMenu({m_themeMenu, {actionItem(m_themeSystemAction), actionItem(m_themeLightAction), actionItem(m_themeDarkAction)}});
    populateMenu({m_selectionToolMenu, {actionItem(findToolAction(CanvasView::Tool::Select))}});
    populateMenu({m_openShapeToolMenu,
                  {actionItem(findToolAction(CanvasView::Tool::Point)), actionItem(findToolAction(CanvasView::Tool::Line)),
                   actionItem(findToolAction(CanvasView::Tool::Polyline))}});
    populateMenu({m_closedShapeToolMenu,
                  {actionItem(findToolAction(CanvasView::Tool::Circle)), actionItem(findToolAction(CanvasView::Tool::Ellipse)),
                   actionItem(findToolAction(CanvasView::Tool::Rectangle)), actionItem(findToolAction(CanvasView::Tool::Polygon))}});
    populateMenu({m_toolMenu,
                  {submenuItem(m_selectionToolMenu), submenuItem(m_openShapeToolMenu), submenuItem(m_closedShapeToolMenu)}});
    populateMenu({m_viewMenu, {actionItem(m_togglePropertyDockAction), submenuItem(m_themeMenu)}});
    populateMenu({m_languageMenu, {actionItem(m_englishAction), actionItem(m_simplifiedChineseAction)}});
    populateMenu({m_tutorialMenu, {actionItem(m_showTutorialAction), separatorItem(), submenuItem(m_languageMenu)}});
}

void MainWindow::setupToolbar() {
    m_toolBar = addToolBar(QString());
    m_toolBar->setMovable(false);

    populateToolbar({m_toolBar,
                     {actionItem(findToolAction(CanvasView::Tool::Select)), separatorItem(),
                      actionItem(findToolAction(CanvasView::Tool::Point)), actionItem(findToolAction(CanvasView::Tool::Line)),
                      actionItem(findToolAction(CanvasView::Tool::Polyline)), separatorItem(),
                      actionItem(findToolAction(CanvasView::Tool::Circle)), actionItem(findToolAction(CanvasView::Tool::Ellipse)),
                      actionItem(findToolAction(CanvasView::Tool::Rectangle)), actionItem(findToolAction(CanvasView::Tool::Polygon)),
                      separatorItem(), actionItem(m_deleteAction), actionItem(m_copyAction), actionItem(m_pasteAction),
                      actionItem(m_openAction), actionItem(m_saveAction), actionItem(m_clearAction), separatorItem(),
                      actionItem(m_togglePropertyDockAction)}});

    if (!m_toolActionGroup->actions().isEmpty()) {
        m_toolActionGroup->actions().first()->setChecked(true);
    }
}

void MainWindow::connectSignals() {
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
    connect(m_propertyPanel, &PropertyPanel::shapeEdited, m_canvasView, &CanvasView::updateSelectedShape);
    connect(m_canvasView, &CanvasView::deleteSelectionRequested, this, &MainWindow::deleteSelection);

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, this, [this](Qt::ColorScheme) {
        if (m_themeMode == ThemeMode::System) {
            applyApplicationTheme(*static_cast<QApplication*>(QApplication::instance()), ThemeMode::System);
        }
    });
#endif
}

void MainWindow::retranslateUi() {
    m_fileMenu->setTitle(i18n::tr(m_language, "menu.file", "File", "文件"));
    m_editMenu->setTitle(i18n::tr(m_language, "menu.edit", "Edit", "编辑"));
    m_viewMenu->setTitle(i18n::tr(m_language, "menu.view", "View", "视图"));
    m_toolMenu->setTitle(i18n::tr(m_language, "menu.tools", "Tools", "工具"));
    m_selectionToolMenu->setTitle(i18n::tr(m_language, "menu.selection_tools", "Selection", "选择工具"));
    m_openShapeToolMenu->setTitle(i18n::tr(m_language, "menu.open_shapes", "Open Shapes", "开放图形"));
    m_closedShapeToolMenu->setTitle(i18n::tr(m_language, "menu.closed_shapes", "Closed Shapes", "封闭图形"));
    m_themeMenu->setTitle(i18n::tr(m_language, "menu.theme", "Theme", "主题"));
    m_tutorialMenu->setTitle("Tutorial");
    m_languageMenu->setTitle(i18n::tr(m_language, "menu.language", "Language", "语言"));

    m_openAction->setText(i18n::tr(m_language, "action.open", "Open", "打开"));
    m_saveAction->setText(i18n::tr(m_language, "action.save", "Save", "保存"));
    m_saveAsAction->setText(i18n::tr(m_language, "action.save_as", "Save As", "另存为"));
    m_exportAction->setText(i18n::tr(m_language, "action.export_image", "Export Image", "导出图片"));
    m_exitAction->setText(i18n::tr(m_language, "action.exit", "Exit", "退出"));
    m_deleteAction->setText(i18n::tr(m_language, "action.delete", "Delete", "删除"));
    m_copyAction->setText(i18n::tr(m_language, "action.copy", "Copy", "复制"));
    m_pasteAction->setText(i18n::tr(m_language, "action.paste", "Paste", "粘贴"));
    m_clearAction->setText(i18n::tr(m_language, "action.clear", "Clear Canvas", "清空画布"));
    m_showTutorialAction->setText(i18n::tr(m_language, "action.tutorial", "Operation Manual", "操作手册"));
    if (m_togglePropertyDockAction != nullptr) {
        m_togglePropertyDockAction->setText(i18n::tr(m_language, "action.toggle_panel", "Properties Panel", "属性面板"));
    }
    m_themeSystemAction->setText(i18n::tr(m_language, "action.theme_auto", "Auto Theme", "自动主题"));
    m_themeLightAction->setText(i18n::tr(m_language, "action.theme_light", "Light", "浅色"));
    m_themeDarkAction->setText(i18n::tr(m_language, "action.theme_dark", "Dark", "深色"));
    m_englishAction->setText("English");
    m_simplifiedChineseAction->setText(QString::fromUtf8("简体中文"));

    for (QAction* action : m_toolActionGroup->actions()) {
        if (action == nullptr || !action->data().isValid()) {
            continue;
        }
        action->setText(toolLabel(static_cast<CanvasView::Tool>(action->data().toInt()), m_language));
    }

    m_propertyDock->setWindowTitle(i18n::tr(m_language, "dock.properties", "Properties", "属性"));
    m_toolBar->setWindowTitle(i18n::tr(m_language, "toolbar.tools", "Tools", "工具"));
    updateWindowTitle();
}

void MainWindow::updateWindowTitle() {
    const QString fileName = m_currentFilePath.isEmpty() ? i18n::tr(m_language, "title.untitled", "Untitled", "未命名")
                                                         : QFileInfo(m_currentFilePath).fileName();
    setWindowTitle(
        QString("%1 - %2").arg(fileName, i18n::tr(m_language, "title.app", "VectorGraphicsEditor", "矢量图形编辑器")));
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
            this, i18n::tr(m_language, "dialog.delete_multi_title", "Delete Multiple Shapes", "删除多个图形"),
            i18n::tr(m_language, "dialog.delete_multi_msg", "Delete %1 selected shapes?", "确定删除已选中的 %1 个图形吗？")
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
        QMessageBox::warning(this, i18n::tr(m_language, "dialog.save_failed", "Save Failed", "保存失败"), errorMessage);
        return false;
    }

    m_currentFilePath = filePath;
    updateWindowTitle();
    statusBar()->showMessage(i18n::tr(m_language, "status.file_saved", "File saved.", "文件已保存。"), 2500);
    return true;
}

bool MainWindow::saveDocument() {
    if (m_currentFilePath.isEmpty()) {
        return saveDocumentAs();
    }
    return saveToPath(m_currentFilePath);
}

bool MainWindow::saveDocumentAs() {
    const QString filePath = QFileDialog::getSaveFileName(
        this, i18n::tr(m_language, "dialog.save_doc", "Save Vector Document", "保存矢量图文件"),
        m_currentFilePath.isEmpty() ? "drawing.vgjson" : m_currentFilePath,
        i18n::tr(m_language, "filter.vgjson", "Vector JSON (*.vgjson *.json)", "矢量 JSON 文件 (*.vgjson *.json)"));
    if (filePath.isEmpty()) {
        return false;
    }
    return saveToPath(filePath);
}

void MainWindow::openDocument() {
    const QString filePath = QFileDialog::getOpenFileName(
        this, i18n::tr(m_language, "dialog.open_doc", "Open Vector Document", "打开矢量图文件"), QString(),
        i18n::tr(m_language, "filter.vgjson", "Vector JSON (*.vgjson *.json)", "矢量 JSON 文件 (*.vgjson *.json)"));
    if (filePath.isEmpty()) {
        return;
    }

    QString errorMessage;
    const std::optional<DocumentData> document = FileManager::loadFromFile(filePath, &errorMessage);
    if (!document.has_value()) {
        QMessageBox::warning(this, i18n::tr(m_language, "dialog.open_failed", "Open Failed", "打开失败"), errorMessage);
        return;
    }

    m_canvasView->loadDocument(*document);
    m_currentFilePath = filePath;
    updateWindowTitle();
    statusBar()->showMessage(i18n::tr(m_language, "status.file_loaded", "File loaded.", "文件已加载。"), 2500);
}

void MainWindow::exportImage() {
    const QString filePath =
        QFileDialog::getSaveFileName(this, i18n::tr(m_language, "dialog.export_image", "Export Image", "导出图片"), "canvas.png",
                                     i18n::tr(m_language, "filter.png", "PNG Image (*.png)", "PNG 图片 (*.png)"));
    if (filePath.isEmpty()) {
        return;
    }

    QString errorMessage;
    if (!m_canvasView->exportImage(filePath, &errorMessage)) {
        QMessageBox::warning(this, i18n::tr(m_language, "dialog.export_failed", "Export Failed", "导出失败"), errorMessage);
        return;
    }

    statusBar()->showMessage(i18n::tr(m_language, "status.image_exported", "Image exported.", "图片已导出。"), 2500);
}
