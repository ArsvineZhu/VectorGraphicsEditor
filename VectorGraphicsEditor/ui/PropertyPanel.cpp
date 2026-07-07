// =====================================================================
// PropertyPanel.cpp
// ---------------------------------------------------------------------
// @brief PropertyPanel 的实现
// @details 本文件实现"无 Apply/Cancel 的实时编辑器"：
//          - 任意 valueChanged / 按钮点击都会立即构造完整 ShapeData 并 emit；
//          - 程序侧更新（setShapeData / rebuildLineStyleCombo）通过
//            m_updatingWidgets 标志位抑制 emit 反馈。
// @layer   ui
// =====================================================================

#include "ui/PropertyPanel.h"

#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include <algorithm>
#include <array>

#include "core/I18n.h"
#include "ui/PropertyPanelGeometryFields.h"

namespace {

/// @brief 在 QFormLayout 中按 field 找到其左侧的 QLabel 并设置文本。
void setFormLabelText(QFormLayout* layout, QWidget* field, const QString& text) {
    if (layout == nullptr || field == nullptr) {
        return;
    }

    auto* label = qobject_cast<QLabel*>(layout->labelForField(field));
    if (label != nullptr) {
        label->setText(text);
    }
}

/// @brief 把颜色渲染为 QSS 字符串，作为颜色按钮的背景。
QString colorButtonStyle(const QColor& color) {
    return QString("background:%1; border:1px solid #909090; min-height:24px;").arg(color.name(QColor::HexArgb));
}

} // namespace

PropertyPanel::PropertyPanel(QWidget* parent) : QWidget(parent) {
    setupUi();
    // 构造完毕先进入"无选中"态，避免显示空数据
    clearSelection();
}

void PropertyPanel::setLanguage(AppLanguage language) {
    if (m_language == language) {
        return;
    }

    m_language = language;
    retranslateUi();
}

void PropertyPanel::clearSelection() {
    m_selectedCount = 0;
    m_hasSelection = false;
    updateSelectionTexts();

    // 隐藏所有几何编辑字段
    for (int index = 0; index < 6; ++index) {
        m_geometryLabels[index]->hide();
        m_geometryEdits[index]->hide();
    }

    // 禁用所有可交互控件
    m_strokeEnabledCheck->setEnabled(false);
    m_strokeColorButton->setEnabled(false);
    m_fillColorButton->setEnabled(false);
    m_lineWidthSpin->setEnabled(false);
    m_lineStyleCombo->setEnabled(false);
    m_fillEnabledCheck->setEnabled(false);
    updateButtons();
}

void PropertyPanel::setShapeData(const ShapeData& data) {
    // 关键：进入"程序更新中"状态，避免 setValue 触发的 valueChanged 形成回环
    m_updatingWidgets = true;
    m_selectedCount = 1;
    m_currentData = normalizedShapeData(data);
    m_hasSelection = true;

    updateSelectionTexts();
    updateGeometryControls();

    // 启用样式相关控件；填充相关控件还要看 type 是否支持
    m_strokeEnabledCheck->setEnabled(true);
    m_strokeEnabledCheck->setChecked(m_currentData.style.strokeEnabled);
    m_lineWidthSpin->setEnabled(m_currentData.style.strokeEnabled);
    m_lineStyleCombo->setEnabled(m_currentData.style.strokeEnabled);
    m_strokeColorButton->setEnabled(m_currentData.style.strokeEnabled);
    m_fillEnabledCheck->setEnabled(shapeSupportsFill(m_currentData.type));
    m_fillColorButton->setEnabled(shapeSupportsFill(m_currentData.type));

    rebuildLineStyleCombo();
    m_lineWidthSpin->setValue(m_currentData.style.strokeWidth);
    const int currentIndex = m_lineStyleCombo->findData(static_cast<int>(m_currentData.style.strokeStyle));
    if (currentIndex >= 0) {
        m_lineStyleCombo->setCurrentIndex(currentIndex);
    }
    m_fillEnabledCheck->setChecked(shapeSupportsFill(m_currentData.type) && m_currentData.style.fillEnabled);
    for (int index = 0; index < 6; ++index) {
        m_geometryEdits[index]->setEnabled(geometryEditingEnabled());
    }
    updateButtons();
    m_updatingWidgets = false;
}

void PropertyPanel::setMultipleSelection(int selectedCount) {
    m_updatingWidgets = true;
    m_selectedCount = std::max(selectedCount, 2);
    m_hasSelection = false;
    updateSelectionTexts();

    for (int index = 0; index < 6; ++index) {
        m_geometryLabels[index]->hide();
        m_geometryEdits[index]->hide();
    }

    m_strokeEnabledCheck->setEnabled(false);
    m_strokeColorButton->setEnabled(false);
    m_fillColorButton->setEnabled(false);
    m_lineWidthSpin->setEnabled(false);
    m_lineStyleCombo->setEnabled(false);
    m_fillEnabledCheck->setEnabled(false);
    updateButtons();
    m_updatingWidgets = false;
}

void PropertyPanel::setupUi() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(12);

    m_titleLabel = new QLabel(this);
    m_titleLabel->setStyleSheet("font-weight:600; font-size:16px;");
    layout->addWidget(m_titleLabel);

    m_hintLabel = new QLabel(this);
    m_hintLabel->setWordWrap(true);
    layout->addWidget(m_hintLabel);

    m_formLayout = new QFormLayout();
    m_formLayout->setLabelAlignment(Qt::AlignLeft);
    m_formLayout->setFormAlignment(Qt::AlignTop);
    m_formLayout->setSpacing(8);

    // 创建 6 个几何字段；其可见性由 updateGeometryControls 决定
    for (int index = 0; index < 6; ++index) {
        m_geometryLabels[index] = new QLabel(this);
        m_geometryEdits[index] = new QDoubleSpinBox(this);
        m_geometryEdits[index]->setRange(-100000.0, 100000.0);
        m_geometryEdits[index]->setDecimals(2);
        m_geometryEdits[index]->setSingleStep(1.0);
        m_formLayout->addRow(m_geometryLabels[index], m_geometryEdits[index]);

        // 任意字段变化都触发重新组装 + emit
        connect(m_geometryEdits[index], QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
                [this](double) { emitEditedShape(); });
    }

    m_strokeColorButton = new QPushButton(this);
    connect(m_strokeColorButton, &QPushButton::clicked, this, [this]() {
        if (!m_hasSelection) {
            return;
        }

        const QColor color = QColorDialog::getColor(m_currentData.style.strokeColor, this,
                                                    i18n::tr(m_language, "dialog.stroke_color", "Select Stroke Color", "选择描边颜色"));
        if (!color.isValid()) {
            return;
        }

        m_currentData.style.strokeColor = color;
        updateButtons();
        emit shapeEdited(m_currentData);
    });
    m_formLayout->addRow(QString(), m_strokeColorButton);

    m_strokeEnabledCheck = new QCheckBox(this);
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    connect(m_strokeEnabledCheck, &QCheckBox::checkStateChanged, this, [this](Qt::CheckState state) {
        if (m_updatingWidgets || !m_hasSelection) return;
        m_currentData.style.strokeEnabled = state == Qt::Checked;
        updateButtons();
        emit shapeEdited(m_currentData);
    });
#else
    connect(m_strokeEnabledCheck, &QCheckBox::stateChanged, this, [this](int state) {
        if (m_updatingWidgets || !m_hasSelection) return;
        m_currentData.style.strokeEnabled = state == Qt::Checked;
        updateButtons();
        emit shapeEdited(m_currentData);
    });
#endif
    m_formLayout->addRow(QString(), m_strokeEnabledCheck);

    m_lineWidthSpin = new QDoubleSpinBox(this);
    m_lineWidthSpin->setRange(0.5, 20.0);
    m_lineWidthSpin->setDecimals(1);
    m_lineWidthSpin->setSingleStep(0.5);
    connect(m_lineWidthSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        if (m_updatingWidgets || !m_hasSelection) {
            return;
        }
        m_currentData.style.strokeWidth = value;
        emit shapeEdited(m_currentData);
    });
    m_formLayout->addRow(QString(), m_lineWidthSpin);

    m_lineStyleCombo = new QComboBox(this);
    connect(m_lineStyleCombo, &QComboBox::currentTextChanged, this, [this](const QString& text) {
        if (m_updatingWidgets || !m_hasSelection) {
            return;
        }
        Q_UNUSED(text);
        m_currentData.style.strokeStyle = static_cast<Qt::PenStyle>(m_lineStyleCombo->currentData().toInt());
        emit shapeEdited(m_currentData);
    });
    m_formLayout->addRow(QString(), m_lineStyleCombo);

    m_fillEnabledCheck = new QCheckBox(this);
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    connect(m_fillEnabledCheck, &QCheckBox::checkStateChanged, this, [this](Qt::CheckState state) {
        if (m_updatingWidgets || !m_hasSelection) return;
        m_currentData.style.fillEnabled = shapeSupportsFill(m_currentData.type) && state == Qt::Checked;
        updateButtons();
        emit shapeEdited(m_currentData);
    });
#else
    connect(m_fillEnabledCheck, &QCheckBox::stateChanged, this, [this](int state) {
        if (m_updatingWidgets || !m_hasSelection) return;
        m_currentData.style.fillEnabled = shapeSupportsFill(m_currentData.type) && state == Qt::Checked;
        updateButtons();
        emit shapeEdited(m_currentData);
    });
#endif
    m_formLayout->addRow(QString(), m_fillEnabledCheck);

    m_fillColorButton = new QPushButton(this);
    connect(m_fillColorButton, &QPushButton::clicked, this, [this]() {
        if (!m_hasSelection || !shapeSupportsFill(m_currentData.type)) {
            return;
        }

        const QColor color = QColorDialog::getColor(m_currentData.style.fillColor, this,
                                                    i18n::tr(m_language, "dialog.fill_color", "Select Fill Color", "选择填充颜色"));
        if (!color.isValid()) {
            return;
        }

        m_currentData.style.fillColor = color;
        updateButtons();
        emit shapeEdited(m_currentData);
    });
    m_formLayout->addRow(QString(), m_fillColorButton);

    layout->addLayout(m_formLayout);
    layout->addStretch();
    retranslateUi();
}

void PropertyPanel::updateButtons() {
    // 把当前颜色同步到按钮的 QSS，方便用户直接看到选中色
    m_strokeColorButton->setStyleSheet(colorButtonStyle(m_currentData.style.strokeColor));
    m_fillColorButton->setStyleSheet(colorButtonStyle(m_currentData.style.fillColor));
    m_strokeColorButton->setEnabled(m_hasSelection && m_currentData.style.strokeEnabled);
    m_lineWidthSpin->setEnabled(m_hasSelection && m_currentData.style.strokeEnabled);
    m_lineStyleCombo->setEnabled(m_hasSelection && m_currentData.style.strokeEnabled);
    m_fillColorButton->setEnabled(m_hasSelection && shapeSupportsFill(m_currentData.type));
}

void PropertyPanel::retranslateUi() {
    rebuildLineStyleCombo();
    updateSelectionTexts();

    m_strokeEnabledCheck->setText(i18n::tr(m_language, "panel.stroke_enabled", "Enable Stroke", "启用描边"));
    m_strokeColorButton->setText(i18n::tr(m_language, "panel.stroke_color", "Stroke Color", "描边颜色"));
    m_fillColorButton->setText(i18n::tr(m_language, "panel.fill_color", "Fill Color", "填充颜色"));
    m_fillEnabledCheck->setText(i18n::tr(m_language, "panel.fill_enabled", "Enable Fill", "启用填充"));

    // 表单左侧标签需要单独刷新
    if (m_formLayout != nullptr) {
        setFormLabelText(m_formLayout, m_strokeEnabledCheck, i18n::tr(m_language, "panel.stroke_enabled_label", "Stroke Enabled", "描边开关"));
        setFormLabelText(m_formLayout, m_strokeColorButton, i18n::tr(m_language, "panel.stroke_label", "Stroke", "描边"));
        setFormLabelText(m_formLayout, m_lineWidthSpin, i18n::tr(m_language, "panel.line_width", "Line Width", "线宽"));
        setFormLabelText(m_formLayout, m_lineStyleCombo, i18n::tr(m_language, "panel.line_style", "Line Style", "线型"));
        setFormLabelText(m_formLayout, m_fillEnabledCheck, i18n::tr(m_language, "panel.fill_label", "Fill", "填充"));
        setFormLabelText(m_formLayout, m_fillColorButton, i18n::tr(m_language, "panel.fill_color_label", "Fill Color", "填充颜色"));
    }
}

void PropertyPanel::updateSelectionTexts() {
    if (m_selectedCount > 1) {
        m_titleLabel->setText(i18n::tr(m_language, "panel.title_multi_count", "%1 shapes selected", "已选中 %1 个图形")
                                  .arg(m_selectedCount));
        m_hintLabel->setText(
            i18n::tr(m_language, "panel.hint_multi",
                     "Multiple selection supports transform and delete in this version.",
                     "当前多选仅支持统一变换与删除。"));
        return;
    }

    if (!m_hasSelection) {
        m_titleLabel->setText(i18n::tr(m_language, "panel.title_none", "No selection", "未选中图形"));
        m_hintLabel->setText(i18n::tr(m_language, "panel.hint_none",
                                      "Select a shape to edit geometry and style.",
                                      "请选择一个图形后再编辑几何参数和样式。"));
        return;
    }

    m_titleLabel->setText(shapeDisplayName(m_currentData.type, m_language));
    if (geometryEditingEnabled()) {
        m_hintLabel->setText(
            i18n::tr(m_language, "panel.hint_geometry",
                     "Geometry updates are applied immediately.", "修改参数后会立即更新画布。"));
        return;
    }

    m_hintLabel->setText(i18n::tr(m_language, "panel.hint_transformed",
                                  "This transformed shape only supports style edits.",
                                  "当前图形已发生不受几何面板支持的变换，仅支持样式编辑。"));
}

void PropertyPanel::rebuildLineStyleCombo() {
    // 先记下当前选中值（用 data 而非 text，因为 text 跟语言相关）
    const QVariant currentData = m_lineStyleCombo->currentData();

    m_updatingWidgets = true;
    m_lineStyleCombo->clear();
    m_lineStyleCombo->addItem(penStyleDisplayName(Qt::SolidLine, m_language), static_cast<int>(Qt::SolidLine));
    m_lineStyleCombo->addItem(penStyleDisplayName(Qt::DashLine, m_language), static_cast<int>(Qt::DashLine));
    m_lineStyleCombo->addItem(penStyleDisplayName(Qt::DotLine, m_language), static_cast<int>(Qt::DotLine));
    m_lineStyleCombo->addItem(penStyleDisplayName(Qt::DashDotLine, m_language), static_cast<int>(Qt::DashDotLine));

    // 恢复原选中
    const int index = m_lineStyleCombo->findData(currentData);
    if (index >= 0) {
        m_lineStyleCombo->setCurrentIndex(index);
    }
    m_updatingWidgets = false;
}

void PropertyPanel::updateGeometryControls() {
    for (int index = 0; index < 6; ++index) {
        m_geometryLabels[index]->hide();
        m_geometryEdits[index]->hide();
        m_geometryEdits[index]->setRange(-100000.0, 100000.0);
    }

    if (!geometryEditingEnabled()) {
        return;
    }

    std::array<double, 6> values = {};
    switch (m_currentData.type) {
    case ShapeType::Point:
        if (!m_currentData.points.isEmpty()) {
            values[0] = m_currentData.points.first().x();
            values[1] = m_currentData.points.first().y();
        }
        break;
    case ShapeType::Line:
        if (m_currentData.points.size() >= 2) {
            values[0] = m_currentData.points.at(0).x();
            values[1] = m_currentData.points.at(0).y();
            values[2] = m_currentData.points.at(1).x();
            values[3] = m_currentData.points.at(1).y();
        }
        break;
    case ShapeType::Polyline:
    case ShapeType::Polygon: {
        const QRectF bounds = pointsBoundingRect(m_currentData.points);
        values[0] = bounds.x();
        values[1] = bounds.y();
        break;
    }
    case ShapeType::Circle:
        values[0] = m_currentData.rect.center().x();
        values[1] = m_currentData.rect.center().y();
        values[2] = m_currentData.rect.width() / 2.0;
        break;
    case ShapeType::Ellipse:
        values[0] = m_currentData.rect.center().x();
        values[1] = m_currentData.rect.center().y();
        values[2] = m_currentData.rect.width() / 2.0;
        values[3] = m_currentData.rect.height() / 2.0;
        break;
    case ShapeType::Rectangle:
        values[0] = m_currentData.rect.x();
        values[1] = m_currentData.rect.y();
        values[2] = m_currentData.rect.width();
        values[3] = m_currentData.rect.height();
        break;
    }

    const GeometryFieldSet& fieldSet = geometryFieldsFor(m_currentData.type);
    for (std::size_t index = 0; index < fieldSet.fields.size() && index < 6; ++index) {
        const GeometryField& field = fieldSet.fields[index];
        m_geometryLabels[index]->setText(QString::fromLatin1(field.label));
        m_geometryEdits[index]->setRange(field.minimum, field.maximum);
        m_geometryEdits[index]->setValue(values[index]);
        m_geometryLabels[index]->show();
        m_geometryEdits[index]->show();
    }
}

void PropertyPanel::emitEditedShape() {
    if (m_updatingWidgets || !m_hasSelection) {
        return;
    }

    ShapeData data = m_currentData;
    std::array<double, 6> values = {};
    for (int index = 0; index < 6; ++index) {
        values[index] = m_geometryEdits[index]->value();
    }
    geometryFieldsFor(data.type).applyTo(data, values);

    m_currentData = normalizedShapeData(data);
    emit shapeEdited(m_currentData);
}

bool PropertyPanel::geometryEditingEnabled() const { return m_hasSelection && m_currentData.transform.isIdentity(); }
