#include <QtTest>

#include <cmath>

#include "SelectionFrame.h"
#include "ShapeData.h"

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
    void polylineTranslationMovesEveryPoint();
    void applyTransformComposesOntoShapeData();
    void missingTransformOrStrokeEnabledFailsToDeserialize();
    void selectionFrameFollowsAffineRotation();
    void selectionFrameTranslationMovesAllCorners();
    void selectionFrameRotateHandleUsesTopNormal();
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
    delta.rotate(30.0);
    applyTransformToShapeData(data, delta);

    QCOMPARE(data.transform, delta * QTransform::fromTranslate(5.0, 8.0));
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

void ShapeDataTests::selectionFrameFollowsAffineRotation() {
    const SelectionFrame frame = SelectionFrame::fromRect(QRectF(10.0, 20.0, 40.0, 20.0));

    QTransform transform;
    transform.translate(30.0, 50.0);
    transform.rotate(90.0);

    const SelectionFrame rotated = frame.mapped(transform);
    QVERIFY(pointsNear(rotated.topLeft, transform.map(frame.topLeft)));
    QVERIFY(pointsNear(rotated.topRight(), transform.map(frame.topRight())));
    QVERIFY(pointsNear(rotated.bottomLeft(), transform.map(frame.bottomLeft())));
    QVERIFY(pointsNear(rotated.bottomRight(), transform.map(frame.bottomRight())));
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

void ShapeDataTests::selectionFrameRotateHandleUsesTopNormal() {
    SelectionFrame frame = SelectionFrame::fromRect(QRectF(0.0, 0.0, 40.0, 20.0));

    QTransform transform;
    transform.rotate(90.0);
    frame = frame.mapped(transform);

    const QPointF topCenter = frame.topCenter();
    const QPointF normal = frame.topNormal();
    const QPointF rotateHandle = topCenter + normal * 26.0;
    const QPointF delta = rotateHandle - topCenter;
    const qreal projection = delta.x() * normal.x() + delta.y() * normal.y();

    QVERIFY(std::abs(projection - 26.0) < kTolerance);
}

QTEST_APPLESS_MAIN(ShapeDataTests)

#include "ShapeDataTests.moc"
