#include <QtTest>

#include <cmath>

#include "core/CanvasGeometry.h"
#include "core/SelectionFrame.h"

namespace {
constexpr qreal kTolerance = 0.0001;

bool pointsNear(const QPointF& lhs, const QPointF& rhs) {
    return std::abs(lhs.x() - rhs.x()) < kTolerance && std::abs(lhs.y() - rhs.y()) < kTolerance;
}

qreal dotProduct(const QPointF& lhs, const QPointF& rhs) { return lhs.x() * rhs.x() + lhs.y() * rhs.y(); }

qreal vectorLength(const QPointF& vector) { return std::hypot(vector.x(), vector.y()); }
} // namespace

class CanvasGeometryTests : public QObject {
    Q_OBJECT

  private slots:
    void circleRectFromDiagonalDrag();
    void circleRectFromAntiDiagonalDrag();
    void orthonormalizedFrameKeepsCenterAndRemovesShear();
    void scaleTransformDoublesFrameSizeFromTopLeft();
    void draggedHandleFollowsMouseOnAxisAlignedFrame();
    void draggedHandleFollowsMouseOnOrientedFrame();
    void scaledOrientedFrameKeepsAxesPerpendicular();
    void scaleClampPreventsFlipAcrossPivot();
    void keepAspectRatioPreservesPivotAndUniformScale();
    void transformBetweenFramesMapsCornersExactly();
};

void CanvasGeometryTests::circleRectFromDiagonalDrag() {
    const QRectF rect = canvas_geometry::circleRectFromPoints(QPointF(10.0, 20.0), QPointF(50.0, 80.0));
    // dx=40, dy=60, side=60, 起点在左上角
    QCOMPARE(rect.x(), 10.0);
    QCOMPARE(rect.y(), 20.0);
    QCOMPARE(rect.width(), 60.0);
    QCOMPARE(rect.height(), 60.0);
}

void CanvasGeometryTests::circleRectFromAntiDiagonalDrag() {
    const QRectF rect = canvas_geometry::circleRectFromPoints(QPointF(50.0, 80.0), QPointF(10.0, 20.0));
    // dx=-40, dy=-60, side=60, 起点在右下角 → 反推左上角
    QCOMPARE(rect.x(), -10.0);
    QCOMPARE(rect.y(), 20.0);
    QCOMPARE(rect.width(), 60.0);
    QCOMPARE(rect.height(), 60.0);
}

void CanvasGeometryTests::orthonormalizedFrameKeepsCenterAndRemovesShear() {
    SelectionFrame frame;
    frame.topLeft = QPointF(0.0, 0.0);
    frame.xAxis = QPointF(10.0, 0.0);
    frame.yAxis = QPointF(3.0, 8.0);

    const QPointF center = frame.center();
    const SelectionFrame normalized = frame.orthonormalized();

    QVERIFY(pointsNear(normalized.center(), center));
    QVERIFY(std::abs(dotProduct(normalized.xAxis, normalized.yAxis)) < kTolerance);
}

void CanvasGeometryTests::scaleTransformDoublesFrameSizeFromTopLeft() {
    const SelectionFrame frame = SelectionFrame::fromRect(QRectF(0.0, 0.0, 100.0, 50.0));

    // 从 BottomRight 拖到 (200,100) = 局部 (2,2)，pivot=TopLeft=(0,0)
    // scaleX = 2/1 = 2, scaleY = 2/1 = 2 → frame 在两个轴都翻倍
    const QTransform transform =
        canvas_geometry::scaleTransformFromHandle(CanvasHandle::BottomRight, frame, QPointF(200.0, 100.0), false);

    const QPointF tl = transform.map(QPointF(0.0, 0.0));
    const QPointF br = transform.map(QPointF(100.0, 50.0));
    QVERIFY(pointsNear(tl, QPointF(0.0, 0.0)));
    QVERIFY(pointsNear(br, QPointF(200.0, 100.0)));
}

void CanvasGeometryTests::draggedHandleFollowsMouseOnAxisAlignedFrame() {
    const SelectionFrame frame = SelectionFrame::fromRect(QRectF(10.0, 20.0, 100.0, 60.0));
    const QPointF targetHandlePoint(35.0, 55.0);

    const SelectionFrame scaled =
        canvas_geometry::scaledFrameFromHandle(CanvasHandle::TopLeft, frame, targetHandlePoint, false);

    QVERIFY(pointsNear(scaled.topLeft, targetHandlePoint));
    QVERIFY(pointsNear(scaled.bottomRight(), frame.bottomRight()));
}

void CanvasGeometryTests::draggedHandleFollowsMouseOnOrientedFrame() {
    SelectionFrame frame;
    frame.topLeft = QPointF(20.0, 30.0);
    frame.xAxis = QPointF(90.0, 30.0);
    frame.yAxis = QPointF(-30.0, 90.0);
    const QPointF targetHandlePoint = frame.topRight() + QPointF(25.0, -15.0);

    const SelectionFrame scaled =
        canvas_geometry::scaledFrameFromHandle(CanvasHandle::TopRight, frame, targetHandlePoint, false);

    QVERIFY(frame.isOrthogonal());
    QVERIFY(pointsNear(scaled.topRight(), targetHandlePoint));
    QVERIFY(pointsNear(scaled.bottomLeft(), frame.bottomLeft()));
}

void CanvasGeometryTests::scaledOrientedFrameKeepsAxesPerpendicular() {
    SelectionFrame frame;
    frame.topLeft = QPointF(-50.0, -50.0);
    frame.xAxis = QPointF(80.0, 20.0);
    frame.yAxis = QPointF(-20.0, 80.0);
    const SelectionFrame scaled = canvas_geometry::scaledFrameFromHandle(
        CanvasHandle::BottomRight, frame, frame.bottomRight() + QPointF(60.0, 20.0), false);

    QVERIFY(frame.isOrthogonal());
    QVERIFY(scaled.isOrthogonal());
}

void CanvasGeometryTests::scaleClampPreventsFlipAcrossPivot() {
    const SelectionFrame frame = SelectionFrame::fromRect(QRectF(0.0, 0.0, 100.0, 100.0));
    const SelectionFrame scaled =
        canvas_geometry::scaledFrameFromHandle(CanvasHandle::BottomRight, frame, QPointF(-20.0, -10.0), false);

    QVERIFY(scaled.isOrthogonal());
    QVERIFY(std::hypot(scaled.xAxis.x(), scaled.xAxis.y()) > 0.0);
    QVERIFY(std::hypot(scaled.yAxis.x(), scaled.yAxis.y()) > 0.0);
    QVERIFY(std::abs(dotProduct(scaled.xAxis, scaled.yAxis)) < kTolerance);
}

void CanvasGeometryTests::keepAspectRatioPreservesPivotAndUniformScale() {
    const SelectionFrame frame = SelectionFrame::fromRect(QRectF(0.0, 0.0, 100.0, 50.0));
    const QPointF targetHandlePoint(220.0, 90.0);

    const SelectionFrame scaled =
        canvas_geometry::scaledFrameFromHandle(CanvasHandle::BottomRight, frame, targetHandlePoint, true);
    const qreal scaleAlongX = vectorLength(scaled.xAxis) / vectorLength(frame.xAxis);
    const qreal scaleAlongY = vectorLength(scaled.yAxis) / vectorLength(frame.yAxis);

    QVERIFY(pointsNear(scaled.topLeft, frame.topLeft));
    QVERIFY(scaled.isOrthogonal());
    QVERIFY(std::abs(scaleAlongX - scaleAlongY) < kTolerance);
}

void CanvasGeometryTests::transformBetweenFramesMapsCornersExactly() {
    const SelectionFrame source = SelectionFrame::fromRect(QRectF(0.0, 0.0, 100.0, 80.0));
    SelectionFrame target;
    target.topLeft = QPointF(30.0, 40.0);
    target.xAxis = QPointF(0.0, 160.0);
    target.yAxis = QPointF(-120.0, 0.0);

    const QTransform delta = canvas_geometry::transformBetweenFrames(source, target);
    QVERIFY2(pointsNear(delta.map(source.topLeft), target.topLeft),
             qPrintable(QString("topLeft mismatch: actual=(%1,%2) expected=(%3,%4)")
                            .arg(delta.map(source.topLeft).x())
                            .arg(delta.map(source.topLeft).y())
                            .arg(target.topLeft.x())
                            .arg(target.topLeft.y())));
    QVERIFY2(pointsNear(delta.map(source.topRight()), target.topRight()),
             qPrintable(QString("topRight mismatch: actual=(%1,%2) expected=(%3,%4)")
                            .arg(delta.map(source.topRight()).x())
                            .arg(delta.map(source.topRight()).y())
                            .arg(target.topRight().x())
                            .arg(target.topRight().y())));
    QVERIFY2(pointsNear(delta.map(source.bottomLeft()), target.bottomLeft()),
             qPrintable(QString("bottomLeft mismatch: actual=(%1,%2) expected=(%3,%4)")
                            .arg(delta.map(source.bottomLeft()).x())
                            .arg(delta.map(source.bottomLeft()).y())
                            .arg(target.bottomLeft().x())
                            .arg(target.bottomLeft().y())));
    QVERIFY2(pointsNear(delta.map(source.bottomRight()), target.bottomRight()),
             qPrintable(QString("bottomRight mismatch: actual=(%1,%2) expected=(%3,%4)")
                            .arg(delta.map(source.bottomRight()).x())
                            .arg(delta.map(source.bottomRight()).y())
                            .arg(target.bottomRight().x())
                            .arg(target.bottomRight().y())));
}

QTEST_APPLESS_MAIN(CanvasGeometryTests)

#include "CanvasGeometryTests.moc"
