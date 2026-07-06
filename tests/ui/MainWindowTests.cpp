#include <QtTest>

#include <QAction>
#include <QApplication>
#include <QList>
#include <QMenu>
#include <QMenuBar>
#include <QSettings>
#include <QTemporaryDir>

#include "ui/MainWindow.h"

namespace {

bool isToolsMenuTitle(const QString& title) { return title == "Tools" || title == QString::fromUtf8("工具"); }

} // namespace

class MainWindowTests : public QObject {
    Q_OBJECT

  private slots:
    void initTestCase();
    void toolsMenuContainsExpectedGroupedSubmenus();

  private:
    QTemporaryDir m_settingsDir;
};

void MainWindowTests::initTestCase() {
    QVERIFY2(m_settingsDir.isValid(), "Temporary settings directory must be available for MainWindow tests.");

    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settingsDir.path());
    QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope, m_settingsDir.path());

    QApplication::setOrganizationName("CourseProject");
    QApplication::setApplicationName("VectorGraphicsEditor");
}

void MainWindowTests::toolsMenuContainsExpectedGroupedSubmenus() {
    MainWindow window;

    QAction* toolsAction = nullptr;
    for (QAction* action : window.menuBar()->actions()) {
        if (action != nullptr && isToolsMenuTitle(action->text())) {
            toolsAction = action;
            break;
        }
    }

    if (toolsAction == nullptr) {
        QFAIL("Top-level Tools menu should exist.");
        return;
    }

    QMenu* toolsMenu = toolsAction->menu();
    if (toolsMenu == nullptr) {
        QFAIL("Top-level Tools action should expose a menu.");
        return;
    }

    const QList<QAction*> groupedToolActions = toolsMenu->actions();
    QCOMPARE(groupedToolActions.size(), 3);

    const QList<int> expectedActionCounts = {1, 3, 4};
    for (int index = 0; index < groupedToolActions.size(); ++index) {
        QAction* action = groupedToolActions.at(index);
        if (action == nullptr) {
            QFAIL("Grouped tool menu action should not be null.");
            return;
        }
        QMenu* submenu = action->menu();
        if (submenu == nullptr) {
            QFAIL("Grouped tool menu action should own a submenu.");
            return;
        }
        QCOMPARE(submenu->actions().size(), expectedActionCounts.at(index));
    }
}

QTEST_MAIN(MainWindowTests)

#include "MainWindowTests.moc"
