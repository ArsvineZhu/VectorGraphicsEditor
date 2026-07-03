#include <QtTest>

#include <cmath>

#include "core/SelectionFrame.h"
#include "core/ShapeData.h"

namespace {

constexpr qreal kTolerance = 0.0001;

bool pointsNear(const QPointF& lhs, const QPointF& rhs) {
    return std::abs(lhs.x() - rhs.x()) < kTolerance && std::abs(lhs.y() - rhs.y()) < kTolerance;
}

} // namespace

class ShapeDataTests : public QObject {
    Q_OBJECT

  private slots:
    void circleSerializationRoundTrip();
    void rectangleSerializationRoundTrip();
    void polylineSerializationRoundTrip();
    void ellipseSerializationRoundTrip();
    void polylineTranslationMovesEveryPoint();
    void applyTransformComposesOntoShapeData();
    void normalizedShapeDataClampsNegativeDimensions();
    void normalizedShapeDataForcesCircleToSquare();
    void normalizedShapeDataIsIdempotentOnRectangle();
    void normalizedShapeDataIsIdempotentOnPoint();
    void missingTransformOrStrokeEnabledFailsToDeserialize();
    void selectionFrameFollowsAffineTransform();
    void selectionFrameTranslationMovesAllCorners();
};

void ShapeDataTests::circleSerializationRoundTrip() {
    ShapeData source;
    source.id = "circle-1";
    source.type = ShapeType::Circle;
    source.rect = QRectF(10.0, 20.0, 80.0, 80.0);
    source.style.strokeEnabled = false;
    source.style.strokeColor = QColor("#ff112233");
    source.style.strokeWidth = 3.5;
    source.style.strokeStyle = Qt::DashDotLine;
    source.style.fillColor = QColor("#ff445566");
    source.style.fillEnabled = true;
    source.transform = QTransform::fromTranslate(15.0, -12.0);
    source.zValue = 7.0;

    const QJsonObject json = shapeDataToJson(source);
    const std::optional<ShapeData> restored = shapeDataFromJson(json);

    QVERIFY(restored.has_value());
    const ShapeData& restoredData = restored.value();
    QCOMPARE(restoredData.id, source.id);
    QCOMPARE(restoredData.type, source.type);
    QCOMPARE(restoredData.rect.center(), source.rect.center());
    QCOMPARE(restoredData.rect.width(), source.rect.width());
    QCOMPARE(restoredData.style.strokeEnabled, source.style.strokeEnabled);
    QCOMPARE(restoredData.style.strokeColor, source.style.strokeColor);
    QCOMPARE(restoredData.style.strokeStyle, source.style.strokeStyle);
    QCOMPARE(restoredData.style.fillEnabled, source.style.fillEnabled);
    QCOMPARE(restoredData.transform, source.transform);
    QCOMPARE(restoredData.zValue, source.zValue);
}

void ShapeDataTests::rectangleSerializationRoundTrip() {
    ShapeData source;
    source.id = "rect-1";
    source.type = ShapeType::Rectangle;
    source.rect = QRectF(12.0, 24.0, 80.0, 60.0);
    source.style.strokeEnabled = true;
    source.style.strokeColor = QColor("#ffaa5500");
    source.style.fillEnabled = true;
    source.style.fillColor = QColor("#ff00aacc");

    const std::optional<ShapeData> restored = shapeDataFromJson(shapeDataToJson(source));
    QVERIFY(restored.has_value());
    QCOMPARE(restored->rect, source.rect);
    QCOMPARE(restored->style.fillColor, source.style.fillColor);
}

void ShapeDataTests::polylineSerializationRoundTrip() {
    ShapeData source;
    source.id = "polyline-1";
    source.type = ShapeType::Polyline;
    source.points = {QPointF(1.0, 2.0), QPointF(3.0, 4.0), QPointF(5.0, 6.0)};

    const std::optional<ShapeData> restored = shapeDataFromJson(shapeDataToJson(source));
    QVERIFY(restored.has_value());
    QCOMPARE(restored->points, source.points);
}

void ShapeDataTests::ellipseSerializationRoundTrip() {
    ShapeData source;
    source.id = "ellipse-1";
    source.type = ShapeType::Ellipse;
    source.rect = QRectF(10.0, 20.0, 40.0, 30.0);

    const std::optional<ShapeData> restored = shapeDataFromJson(shapeDataToJson(source));
    QVERIFY(restored.has_value());
    QCOMPARE(restored->rect, source.rect);
}

void ShapeDataTests::polylineTranslationMovesEveryPoint() {
    ShapeData data;
    data.type = ShapeType::Polyline;
    data.points = {QPointF(0.0, 0.0), QPointF(20.0, 5.0), QPointF(30.0, 30.0)};

    translateShapeData(data, QPointF(5.0, -3.0));

    QCOMPARE(data.points.at(0), QPointF(5.0, -3.0));
    QCOMPARE(data.points.at(1), QPointF(25.0, 2.0));
    QCOMPARE(data.points.at(2), QPointF(35.0, 27.0));
}

void ShapeDataTests::applyTransformComposesOntoShapeData() {
    ShapeData data;
    data.type = ShapeType::Rectangle;
    data.rect = QRectF(10.0, 20.0, 30.0, 40.0);
    data.transform = QTransform::fromTranslate(5.0, 8.0);

    QTransform delta;
    delta.translate(50.0, 70.0);
    delta.scale(2.0, 3.0);
    applyTransformToShapeData(data, delta);

    QCOMPARE(data.transform, delta * QTransform::fromTranslate(5.0, 8.0));
}

void ShapeDataTests::normalizedShapeDataClampsNegativeDimensions() {
    ShapeData data;
    data.type = ShapeType::Rectangle;
    data.rect = QRectF(10.0, 20.0, -30.0, -40.0);

    const ShapeData normalized = normalizedShapeData(data);
    QCOMPARE(normalized.rect, QRectF(-20.0, -20.0, 30.0, 40.0));
}

void ShapeDataTests::normalizedShapeDataForcesCircleToSquare() {
    ShapeData data;
    data.type = ShapeType::Circle;
    data.rect = QRectF(10.0, 20.0, 20.0, 40.0);

    const ShapeData normalized = normalizedShapeData(data);
    QCOMPARE(normalized.rect, QRectF(10.0, 20.0, 40.0, 40.0));
}

void ShapeDataTests::normalizedShapeDataIsIdempotentOnRectangle() {
    ShapeData data;
    data.type = ShapeType::Rectangle;
    data.rect = QRectF(1.0, 2.0, 3.0, 4.0);
    QCOMPARE(normalizedShapeData(data).rect, data.rect);
}

void ShapeDataTests::normalizedShapeDataIsIdempotentOnPoint() {
    ShapeData data;
    data.type = ShapeType::Point;
    data.points = {QPointF(3.0, 4.0)};
    QCOMPARE(normalizedShapeData(data).points, data.points);
}

void ShapeDataTests::missingTransformOrStrokeEnabledFailsToDeserialize() {
    ShapeData source;
    source.id = "line-1";
    source.type = ShapeType::Line;
    source.points = {QPointF(0.0, 0.0), QPointF(10.0, 10.0)};

    QJsonObject json = shapeDataToJson(source);
    json.remove("transform");
    QVERIFY(!shapeDataFromJson(json).has_value());

    json = shapeDataToJson(source);
    json.remove("strokeEnabled");
    QVERIFY(!shapeDataFromJson(json).has_value());
}

void ShapeDataTests::selectionFrameFollowsAffineTransform() {
    const SelectionFrame frame = SelectionFrame::fromRect(QRectF(10.0, 20.0, 40.0, 20.0));

    QTransform transform;
    transform.translate(30.0, 50.0);
    transform.shear(0.25, -0.5);

    const SelectionFrame mapped = frame.mapped(transform);
    QVERIFY(pointsNear(mapped.topLeft, transform.map(frame.topLeft)));
    QVERIFY(pointsNear(mapped.topRight(), transform.map(frame.topRight())));
    QVERIFY(pointsNear(mapped.bottomLeft(), transform.map(frame.bottomLeft())));
    QVERIFY(pointsNear(mapped.bottomRight(), transform.map(frame.bottomRight())));
}

void ShapeDataTests::selectionFrameTranslationMovesAllCorners() {
    const SelectionFrame source = SelectionFrame::fromRect(QRectF(5.0, 8.0, 20.0, 10.0));
    const QPointF delta(12.0, -7.0);
    const SelectionFrame moved = source.translated(delta);

    QCOMPARE(moved.topLeft, source.topLeft + delta);
    QCOMPARE(moved.topRight(), source.topRight() + delta);
    QCOMPARE(moved.bottomLeft(), source.bottomLeft() + delta);
    QCOMPARE(moved.center(), source.center() + delta);
}

QTEST_APPLESS_MAIN(ShapeDataTests)

#include "ShapeDataTests.moc"
