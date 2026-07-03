// =====================================================================
// MainWindowActions.h
// ---------------------------------------------------------------------
// @brief   MainWindow 的动作描述 DSL 与菜单/工具栏组装 helper
// @details 这些结构把 QAction 的创建、注册、菜单填充、工具栏填充抽离出来，
//          让 MainWindow 可以用声明式表格而不是重复的 imperative 代码组装 UI。
// @layer   ui
// =====================================================================

#pragma once

#include <QList>
#include <QKeySequence>
#include <QToolBar>

#include <functional>
#include <optional>

class QAction;
class QMenu;
class QWidget;

/// @brief 描述一个待创建 QAction 的最小配置。
/// @details `configure` 用于做 triggered 之外的附加设置（如 data、icon、tooltip），
///          `registerAction` 则由调用方把生成出的 QAction 保存到成员指针。
struct ActionDescriptor {
    const char* id = "";
    std::optional<QKeySequence::StandardKey> shortcut;
    bool checkable = false;
    std::function<void(QAction&)> configure;
    std::function<void()> triggered;
};

/// @brief 菜单或工具栏中的一个条目：action、submenu 或 separator 三选一。
/// @warning 约定同一实例只应启用三者之一；populate helper 不做运行时冲突校验。
struct MenuItemDescriptor {
    const char* id = "";
    bool separator = false;
    QMenu* submenu = nullptr;
    QAction* action = nullptr;
};

/// @brief 单个菜单的填充描述。
struct MenuDescriptor {
    QMenu* menu = nullptr;
    QList<MenuItemDescriptor> items;
};

/// @brief 单个工具栏的填充描述。
struct ToolbarDescriptor {
    QToolBar* toolbar = nullptr;
    QList<MenuItemDescriptor> items;
};

/// @brief 批量创建 QAction，并交给调用方登记。
void setupActionsFromDescriptors(QWidget* owner, const QList<ActionDescriptor>& descriptors,
                                 const std::function<void(const char*, QAction*)>& registerAction);

/// @brief 便捷构造 action 菜单项。
MenuItemDescriptor actionItem(QAction* action);
/// @brief 便捷构造 submenu 菜单项。
MenuItemDescriptor submenuItem(QMenu* submenu);
/// @brief 便捷构造分隔线菜单项。
MenuItemDescriptor separatorItem();

/// @brief 根据描述批量填充 QMenu。
void populateMenu(const MenuDescriptor& descriptor);
/// @brief 根据描述批量填充 QToolBar。
void populateToolbar(const ToolbarDescriptor& descriptor);
