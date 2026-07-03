// =====================================================================
// MainWindowActions.cpp
// ---------------------------------------------------------------------
// @brief MainWindowActions.h 中声明的 UI 装配 helper 实现
// @details 这里刻意保持无业务状态：只负责 QAction 的标准化创建和容器填充，
//          避免 MainWindow.cpp 被重复样板代码淹没。
// @layer   ui
// =====================================================================

#include "ui/MainWindowActions.h"

#include <QAction>
#include <QMenu>

/// @brief 按描述表批量创建 QAction，并把生成结果交回调用方登记。
void setupActionsFromDescriptors(QWidget* owner, const QList<ActionDescriptor>& descriptors,
                                 const std::function<void(const char*, QAction*)>& registerAction) {
    for (const ActionDescriptor& descriptor : descriptors) {
        auto* action = new QAction(owner);
        action->setCheckable(descriptor.checkable);
        if (descriptor.shortcut.has_value()) {
            action->setShortcut(*descriptor.shortcut);
        }
        if (descriptor.configure) {
            descriptor.configure(*action);
        }
        if (descriptor.triggered) {
            // 只把触发回调绑定到 action；其余配置仍通过 descriptor.configure 注入。
            QObject::connect(action, &QAction::triggered, owner, [triggered = descriptor.triggered]() { triggered(); });
        }
        registerAction(descriptor.id, action);
    }
}

/// @brief 便捷构造 action 类型的菜单/工具栏条目。
MenuItemDescriptor actionItem(QAction* action) {
    MenuItemDescriptor descriptor;
    descriptor.action = action;
    return descriptor;
}

/// @brief 便捷构造 submenu 类型的菜单条目。
MenuItemDescriptor submenuItem(QMenu* submenu) {
    MenuItemDescriptor descriptor;
    descriptor.submenu = submenu;
    return descriptor;
}

/// @brief 便捷构造 separator 条目。
MenuItemDescriptor separatorItem() {
    MenuItemDescriptor descriptor;
    descriptor.separator = true;
    return descriptor;
}

/// @brief 按描述表向 QMenu 追加条目。
void populateMenu(const MenuDescriptor& descriptor) {
    if (descriptor.menu == nullptr) {
        return;
    }

    for (const MenuItemDescriptor& item : descriptor.items) {
        if (item.separator) {
            descriptor.menu->addSeparator();
        } else if (item.submenu != nullptr) {
            descriptor.menu->addMenu(item.submenu);
        } else if (item.action != nullptr) {
            descriptor.menu->addAction(item.action);
        }
    }
}

/// @brief 按描述表向 QToolBar 追加条目。
void populateToolbar(const ToolbarDescriptor& descriptor) {
    if (descriptor.toolbar == nullptr) {
        return;
    }

    for (const MenuItemDescriptor& item : descriptor.items) {
        if (item.separator) {
            descriptor.toolbar->addSeparator();
        } else if (item.action != nullptr) {
            descriptor.toolbar->addAction(item.action);
        }
    }
}
