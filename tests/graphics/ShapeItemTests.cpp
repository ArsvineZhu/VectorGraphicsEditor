#include <QtTest>

#include <cmath>
#include <QImage>
#include <QPainter>

#include "graphics/ShapeItem.h"

namespace {

constexpr qreal kTolerance = 0.0001;

bool near(qreal lhs, qreal rhs) { return std::abs(lhs - rhs) < kTolerance; }

ShapeData makeLineData(qreal strokeWidth = 2.0) {
    ShapeData data;
    data.type = ShapeType::Line;
    data.points = {QPointF(0.0, 0.0), QPointF(10.0, 0.0)};
    data.style.strokeWidth = strokeWidth;
    return data;
}

} // namespace

class ShapeItemTests : public QObject {
    Q_OBJECT

  private slots:
    void pointBuildPathHasEllipseOfExpectedRadius();
    void lineBuildPathHasMoveAndLine();
    void polylineBuildPathHasMultipleSegments();
    void polygonBuildPathClosedWhenNotPreview();
    void polygonBuildPathOpenInPreview();
    void circleAndEllipseBuildPathHasEllipse();
    void rectangleBuildPathHasRect();
    void interactionPathExpandsByStrokePlus6WithMin8();
    void previewModePenIsLighterAndDashed();
    void emptyPathPassesThroughInteractionPath();
};

void ShapeItemTests::pointBuildPathHasEllipseOfExpectedRadius() {
    ShapeData data;
    data.type = ShapeType::Point;
    data.points = {QPointF(12.0, 34.0)};
    data.style.strokeWidth = 4.0;

    ShapeItem item(data);
    const QPainterPath path = item.buildBasePath();
    const QRectF bounds = path.boundingRect();

    QVERIFY(near(bounds.width(), 12.0));
    QVERIFY(near(bounds.height(), 12.0));
    QVERIFY(near(bounds.center().x(), 12.0));
    QVERIFY(near(bounds.center().y(), 34.0));
}

void ShapeItemTests::lineBuildPathHasMoveAndLine() {
    ShapeItem item(makeLineData());
    const QPainterPath path = item.buildBasePath();

    QCOMPARE(path.elementCount(), 2);
    QCOMPARE(path.elementAt(0).type, QPainterPath::MoveToElement);
    QCOMPARE(path.elementAt(1).type, QPainterPath::LineToElement);
    QVERIFY(near(path.elementAt(1).x, 10.0));
}

void ShapeItemTests::polylineBuildPathHasMultipleSegments() {
    ShapeData data;
    data.type = ShapeType::Polyline;
    data.points = {QPointF(0.0, 0.0), QPointF(10.0, 0.0), QPointF(10.0, 10.0)};

    ShapeItem item(data);
    const QPainterPath path = item.buildBasePath();

    QCOMPARE(path.elementCount(), 3);
    QCOMPARE(path.elementAt(0).type, QPainterPath::MoveToElement);
    QCOMPARE(path.elementAt(1).type, QPainterPath::LineToElement);
    QCOMPARE(path.elementAt(2).type, QPainterPath::LineToElement);
}

void ShapeItemTests::polygonBuildPathClosedWhenNotPreview() {
    ShapeData data;
    data.type = ShapeType::Polygon;
    data.points = {QPointF(0.0, 0.0), QPointF(10.0, 0.0), QPointF(10.0, 10.0)};

    ShapeItem item(data);
    const QPainterPath path = item.buildBasePath();

    QCOMPARE(path.elementCount(), 3);
    const QRectF bounds = path.boundingRect();
    QVERIFY(near(bounds.width(), 10.0));
    QVERIFY(near(bounds.height(), 10.0));
}

void ShapeItemTests::polygonBuildPathOpenInPreview() {
    ShapeData data;
    data.type = ShapeType::Polygon;
    data.points = {QPointF(0.0, 0.0), QPointF(10.0, 0.0), QPointF(10.0, 10.0)};

    ShapeItem item(data);
    item.setPreviewMode(true);
    const QPainterPath path = item.buildBasePath();

    QCOMPARE(path.elementCount(), 3);
    QCOMPARE(path.elementAt(2).type, QPainterPath::LineToElement);
    QVERIFY(near(path.elementAt(2).x, 10.0));
    QVERIFY(near(path.elementAt(2).y, 10.0));
    QVERIFY(near(path.length(), 20.0));
}

void ShapeItemTests::circleAndEllipseBuildPathHasEllipse() {
    ShapeData circle;
    circle.type = ShapeType::Circle;
    circle.rect = QRectF(0.0, 0.0, 20.0, 20.0);
    ShapeItem circleItem(circle);
    QVERIFY(near(circleItem.buildBasePath().boundingRect().width(), 20.0));

    ShapeData ellipse;
    ellipse.type = ShapeType::Ellipse;
    ellipse.rect = QRectF(0.0, 0.0, 30.0, 20.0);
    ShapeItem ellipseItem(ellipse);
    const QRectF bounds = ellipseItem.buildBasePath().boundingRect();
    QVERIFY(near(bounds.width(), 30.0));
    QVERIFY(near(bounds.height(), 20.0));
}

void ShapeItemTests::rectangleBuildPathHasRect() {
    ShapeData data;
    data.type = ShapeType::Rectangle;
    data.rect = QRectF(5.0, 6.0, 20.0, 15.0);

    ShapeItem item(data);
    const QRectF bounds = item.buildBasePath().boundingRect();
    QVERIFY(near(bounds.x(), 5.0));
    QVERIFY(near(bounds.y(), 6.0));
    QVERIFY(near(bounds.width(), 20.0));
    QVERIFY(near(bounds.height(), 15.0));
}

void ShapeItemTests::interactionPathExpandsByStrokePlus6WithMin8() {
    ShapeItem thinItem(makeLineData(1.0));
    const QRectF thinBounds = thinItem.buildInteractionPath(thinItem.buildBasePath()).boundingRect();
    QVERIFY(near(thinBounds.height(), 8.0));

    ShapeItem thickItem(makeLineData(10.0));
    const QRectF thickBounds = thickItem.buildInteractionPath(thickItem.buildBasePath()).boundingRect();
    QVERIFY(near(thickBounds.height(), 16.0));
}

void ShapeItemTests::previewModePenIsLighterAndDashed() {
    ShapeData data = makeLineData();
    data.style.strokeColor = QColor("#ff445566");

    ShapeItem item(data);
    item.setPreviewMode(true);
    const QPen pen = item.buildPen();

    QCOMPARE(pen.style(), Qt::DashLine);
    QCOMPARE(pen.color(), data.style.strokeColor.lighter(115));
}

void ShapeItemTests::emptyPathPassesThroughInteractionPath() {
    ShapeItem item(makeLineData());
    QVERIFY(item.buildInteractionPath(QPainterPath()).isEmpty());
}

QTEST_APPLESS_MAIN(ShapeItemTests)

#include "ShapeItemTests.moc"
