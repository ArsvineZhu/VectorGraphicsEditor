#include <QtTest>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>

#include "FileManager.h"

class FileManagerTests : public QObject {
    Q_OBJECT

  private slots:
    void saveAndLoadDocument();
    void rejectOldOrIncompleteDocument();
};

void FileManagerTests::saveAndLoadDocument() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    DocumentData document;
    document.canvasSize = QSizeF(800.0, 600.0);

    ShapeData rectangle;
    rectangle.id = "rect-1";
    rectangle.type = ShapeType::Rectangle;
    rectangle.rect = QRectF(10.0, 20.0, 100.0, 60.0);
    rectangle.style.strokeEnabled = false;
    rectangle.style.fillEnabled = true;
    rectangle.style.fillColor = QColor("#ff0099cc");
    rectangle.transform.translate(12.0, 18.0);
    rectangle.zValue = 3.0;
    document.shapes.append(rectangle);

    const QString filePath = tempDir.filePath("sample.vgjson");
    QString saveError;
    QVERIFY(FileManager::saveToFile(filePath, document, &saveError));
    QVERIFY2(saveError.isEmpty(), qPrintable(saveError));

    QString loadError;
    const std::optional<DocumentData> loaded = FileManager::loadFromFile(filePath, &loadError);
    QVERIFY(loaded.has_value());
    QVERIFY2(loadError.isEmpty(), qPrintable(loadError));
    const DocumentData& loadedDocument = loaded.value();
    QCOMPARE(loadedDocument.canvasSize.width(), document.canvasSize.width());
    QCOMPARE(loadedDocument.canvasSize.height(), document.canvasSize.height());
    QCOMPARE(loadedDocument.shapes.size(), 1);
    QCOMPARE(loadedDocument.shapes.first().type, ShapeType::Rectangle);
    QCOMPARE(loadedDocument.shapes.first().rect, rectangle.rect);
    QCOMPARE(loadedDocument.shapes.first().style.strokeEnabled, rectangle.style.strokeEnabled);
    QCOMPARE(loadedDocument.shapes.first().style.fillEnabled, rectangle.style.fillEnabled);
    QCOMPARE(loadedDocument.shapes.first().transform, rectangle.transform);
    QCOMPARE(loadedDocument.shapes.first().zValue, rectangle.zValue);
}

void FileManagerTests::rejectOldOrIncompleteDocument() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString filePath = tempDir.filePath("invalid.vgjson");
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    file.write(QJsonDocument(QJsonObject{
                                 {"version", 1},
                                 {"canvas", QJsonObject{{"width", 800.0}, {"height", 600.0}}},
                                 {"shapes", QJsonArray{}},
                             })
                   .toJson(QJsonDocument::Indented));
    file.close();

    QString errorMessage;
    QVERIFY(!FileManager::loadFromFile(filePath, &errorMessage).has_value());
    QVERIFY(errorMessage.contains("version 2"));

    ShapeData shape;
    shape.id = "shape-1";
    shape.type = ShapeType::Rectangle;
    shape.rect = QRectF(0.0, 0.0, 10.0, 10.0);
    QJsonObject invalidShape = shapeDataToJson(shape);
    invalidShape.remove("transform");

    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    file.write(QJsonDocument(QJsonObject{
                                 {"version", 2},
                                 {"canvas", QJsonObject{{"width", 800.0}, {"height", 600.0}}},
                                 {"shapes", QJsonArray{invalidShape}},
                             })
                   .toJson(QJsonDocument::Indented));
    file.close();

    errorMessage.clear();
    QVERIFY(!FileManager::loadFromFile(filePath, &errorMessage).has_value());
    QVERIFY(errorMessage.contains("Invalid shape data"));
}

QTEST_APPLESS_MAIN(FileManagerTests)

#include "FileManagerTests.moc"
