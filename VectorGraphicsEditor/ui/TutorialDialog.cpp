// =====================================================================
// TutorialDialog.cpp
// ---------------------------------------------------------------------
// @brief TutorialDialog 的实现
// @details buildManualHtml() 内部包含大段 HTML 字符串（中英两版），
//          它们是用户文档而非代码逻辑，因此本文件不逐行注释其内容。
// @layer   ui
// =====================================================================

#include "ui/TutorialDialog.h"

#include <QPushButton>
#include <QTextBrowser>
#include <QVBoxLayout>

#include "core/I18n.h"
#include "ui/TutorialManualContent.h"

TutorialDialog::TutorialDialog(QWidget* parent) : QDialog(parent) {
    setupUi();
    retranslateUi();
}

void TutorialDialog::setLanguage(AppLanguage language) {
    if (m_language == language) {
        return;
    }

    m_language = language;
    retranslateUi();
}

void TutorialDialog::setupUi() {
    resize(900, 720);
    setModal(true);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(12);

    m_browser = new QTextBrowser(this);
    // 关闭外链跳转，避免用户点击 HTML 中的外部链接被强制打开浏览器
    m_browser->setOpenExternalLinks(false);
    layout->addWidget(m_browser, 1);

    m_closeButton = new QPushButton(this);
    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::accept);
    layout->addWidget(m_closeButton, 0, Qt::AlignRight);
}

void TutorialDialog::retranslateUi() {
    setWindowTitle(i18n::tr(m_language, "dialog.tutorial_title", "Tutorial", "Tutorial"));
    m_browser->setHtml(buildManualHtml());
    m_closeButton->setText(i18n::tr(m_language, "dialog.tutorial_close", "Close", "关闭"));
}

/// @brief 构造当前语言版本的手册 HTML。内容覆盖：
///        1) 界面布局、2) 绘图工具、3) Polyline/Polygon 详解、
///        4) 选择与编辑、5) 文件与图片、6) 复制粘贴删除、7) 快捷键、
///        8) 语言切换、9) 当前实现边界。
QString TutorialDialog::buildManualHtml() const {
    return m_language == AppLanguage::SimplifiedChinese ? QString::fromUtf8(tutorial::kManualHtmlZh)
                                                        : QString::fromUtf8(tutorial::kManualHtmlEn);
}
