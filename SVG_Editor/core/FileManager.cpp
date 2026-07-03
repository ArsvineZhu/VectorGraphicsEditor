// =====================================================================
// FileManager.cpp
// ---------------------------------------------------------------------
// @brief FileManager.h 中声明的静态方法的实现
// @details 写入采用 QJsonDocument::Indented 便于人读/调试；
//          读取对缺省字段做兜底（canvas 默认 1200×800，shapes 缺失视为空）；
//          任何反序列化失败、缺 type 字段、非空 id 重复都会让整个文件加载失败，
//          并通过 errorMessage 把具体原因回给调用方。
// @layer   core
// =====================================================================

#include "core/FileManager.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>

#include <cmath>
#include <unordered_set>

#include "core/CanvasViewConstants.h"

namespace {

constexpr qreal kTransformEpsilon = 0.0001;

bool isSupportedTransform(const QTransform& transform) {
    return std::abs(transform.m12()) <= kTransformEpsilon && std::abs(transform.m21()) <= kTransformEpsilon &&
           transform.m11() > kTransformEpsilon && transform.m22() > kTransformEpsilon;
}

QString unsupportedTransformError(int shapeIndex) {
    return QString("Unsupported transform at shape index %1. Rotation is no longer supported.").arg(shapeIndex);
}

} // namespace

bool FileManager::saveToFile(const QString& filePath, const DocumentData& document, QString* errorMessage) {
    // 把所有 ShapeData 预序列化为 JSON 数组
    QJsonArray shapeArray;
    int shapeIndex = 0;
    for (const ShapeData& shape : document.shapes) {
        if (!isSupportedTransform(shape.transform)) {
            if (errorMessage != nullptr) {
                *errorMessage = unsupportedTransformError(shapeIndex);
            }
            return false;
        }
        shapeArray.append(shapeDataToJson(shape));
        ++shapeIndex;
    }

    // 文档根结构：version + canvas + shapes
    const QJsonObject root = {
        {"version", 2},
        {"canvas",
         QJsonObject{
             {"width", document.canvasSize.width()},
             {"height", document.canvasSize.height()},
         }},
        {"shapes", shapeArray},
    };

    // 打开文件（WriteOnly + Truncate 模式）；若路径不存在则创建
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errorMessage != nullptr) {
            *errorMessage = file.errorString();
        }
        return false;
    }

    // 写入缩进格式 JSON（QJsonDocument::Indented），便于人读
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return true;
}

std::optional<DocumentData> FileManager::loadFromFile(const QString& filePath, QString* errorMessage) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage != nullptr) {
            *errorMessage = file.errorString();
        }
        return std::nullopt;
    }

    // 解析整份 JSON；非 object 根视为格式错误
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject()) {
        if (errorMessage != nullptr) {
            *errorMessage = "Invalid JSON document.";
        }
        return std::nullopt;
    }

    const QJsonObject root = document.object();
    if (root.value("version").toInt(-1) != 2) {
        if (errorMessage != nullptr) {
            *errorMessage = "Unsupported document version. Expected version 2.";
        }
        return std::nullopt;
    }

    const QJsonObject canvas = root.value("canvas").toObject();

    DocumentData data;
    // 画布尺寸缺省回退到 1200×800（与 MainWindow 的默认值一致）
    data.canvasSize =
        QSizeF(canvas.value("width").toDouble(kDefaultCanvasWidth), canvas.value("height").toDouble(kDefaultCanvasHeight));

    const QJsonArray shapes = root.value("shapes").toArray();
    std::unordered_set<QString> seenIds;
    int shapeIndex = 0;
    for (const auto& value : shapes) {
        const std::optional<ShapeData> shape = shapeDataFromJson(value.toObject());
        if (!shape.has_value()) {
            if (errorMessage != nullptr) {
                *errorMessage = QString("Invalid shape data at index %1.").arg(shapeIndex);
            }
            return std::nullopt;
        }
        if (!isSupportedTransform(shape->transform)) {
            if (errorMessage != nullptr) {
                *errorMessage = unsupportedTransformError(shapeIndex);
            }
            return std::nullopt;
        }

        // id 可空但非空时必须全局唯一；空 id 视为"未指定"，跳过查重
        if (!shape->id.isEmpty() && !seenIds.insert(shape->id).second) {
            if (errorMessage != nullptr) {
                *errorMessage = QString("Duplicate shape id \"%1\" at index %2.").arg(shape->id, QString::number(shapeIndex));
            }
            return std::nullopt;
        }
        data.shapes.append(*shape);
        ++shapeIndex;
    }

    return data;
}
