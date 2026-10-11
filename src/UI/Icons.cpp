#include "Icons.h"
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <functional>
#include "DataModel/BrickColor.h"

namespace {

QPixmap makePixmap(const std::function<void(QPainter&)>& draw) {
    QPixmap pm(32, 32);                 // 16x16 logical at devicePixelRatio 2
    pm.setDevicePixelRatio(2.0);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    draw(p);
    p.end();
    return pm;
}

QIcon makeIcon(const std::function<void(QPainter&)>& draw) { return QIcon(makePixmap(draw)); }

void drawCube(QPainter& p, const QColor& base) {
    const QColor outline = base.darker(210);
    p.setPen(QPen(outline, 0.9));
    p.setBrush(base.lighter(130));
    p.drawPolygon(QPolygonF({QPointF(8, 1.5), QPointF(14.5, 4.8), QPointF(8, 8.2), QPointF(1.5, 4.8)}));
    p.setBrush(base);
    p.drawPolygon(QPolygonF({QPointF(1.5, 4.8), QPointF(8, 8.2), QPointF(8, 14.8), QPointF(1.5, 11.4)}));
    p.setBrush(base.darker(135));
    p.drawPolygon(QPolygonF({QPointF(8, 8.2), QPointF(14.5, 4.8), QPointF(14.5, 11.4), QPointF(8, 14.8)}));
}

void arrowHead(QPainter& p, QPointF tip, QPointF dir, const QColor& c) {
    // dir is a unit vector pointing from the shaft toward the tip
    const QPointF n(-dir.y(), dir.x());
    p.setPen(Qt::NoPen);
    p.setBrush(c);
    p.drawPolygon(QPolygonF({tip, tip - dir * 3.4 + n * 2.2, tip - dir * 3.4 - n * 2.2}));
}

QColor c3(int r, int g, int b) { return QColor(r, g, b); }

}  // namespace

namespace Icons {

QPixmap swatch(int brickColorId, int size) {
    const BrickColorInfo& bc = brickColorById(brickColorId);
    QPixmap pm(size, size);
    pm.fill(QColor(bc.r, bc.g, bc.b));
    QPainter p(&pm);
    p.setPen(QColor(64, 64, 64));
    p.drawRect(0, 0, size - 1, size - 1);
    return pm;
}

QIcon partIcon(int brickColorId) {
    const BrickColorInfo& bc = brickColorById(brickColorId);
    return makeIcon([&](QPainter& p) { drawCube(p, QColor(bc.r, bc.g, bc.b)); });
}

QIcon icon(Id id) {
    switch (id) {
    case New:
        return makeIcon([](QPainter& p) {
            p.setPen(QPen(c3(90, 90, 90), 0.9)); p.setBrush(Qt::white);
            p.drawPolygon(QPolygonF({QPointF(3, 1.5), QPointF(10, 1.5), QPointF(13, 4.5), QPointF(13, 14.5), QPointF(3, 14.5)}));
            p.setBrush(c3(210, 210, 210));
            p.drawPolygon(QPolygonF({QPointF(10, 1.5), QPointF(10, 4.5), QPointF(13, 4.5)}));
        });
    case Open:
        return makeIcon([](QPainter& p) {
            p.setPen(QPen(c3(120, 90, 20), 0.9)); p.setBrush(c3(236, 200, 80));
            p.drawPolygon(QPolygonF({QPointF(1.5, 3.5), QPointF(6, 3.5), QPointF(7.5, 5), QPointF(13, 5), QPointF(13, 13.5), QPointF(1.5, 13.5)}));
            p.setBrush(c3(250, 225, 120));
            p.drawPolygon(QPolygonF({QPointF(3.5, 7.5), QPointF(15, 7.5), QPointF(13, 13.5), QPointF(1.5, 13.5)}));
        });
    case Save:
        return makeIcon([](QPainter& p) {
            p.setPen(QPen(c3(20, 40, 110), 0.9)); p.setBrush(c3(60, 100, 200));
            p.drawRect(QRectF(2, 2, 12, 12));
            p.setBrush(Qt::white); p.drawRect(QRectF(4, 2.5, 8, 5));
            p.setBrush(c3(200, 200, 210)); p.drawRect(QRectF(5, 9.5, 6, 4));
            p.setPen(Qt::NoPen); p.setBrush(c3(40, 60, 140)); p.drawRect(QRectF(9.5, 10.5, 1.2, 2.5));
        });
    case Cut:
        return makeIcon([](QPainter& p) {
            p.setPen(QPen(c3(60, 60, 60), 1.1));
            p.drawLine(QPointF(5, 2), QPointF(11, 10)); p.drawLine(QPointF(11, 2), QPointF(5, 10));
            p.setBrush(Qt::NoBrush); p.setPen(QPen(c3(200, 40, 40), 1.1));
            p.drawEllipse(QPointF(4.5, 12), 2.2, 2.2); p.drawEllipse(QPointF(11.5, 12), 2.2, 2.2);
        });
    case Copy:
        return makeIcon([](QPainter& p) {
            p.setPen(QPen(c3(90, 90, 90), 0.9)); p.setBrush(Qt::white);
            p.drawRect(QRectF(2.5, 1.5, 7, 9));
            p.drawRect(QRectF(6.5, 5.5, 7, 9));
            p.setPen(QPen(c3(150, 150, 150), 0.8));
            p.drawLine(QPointF(8, 8), QPointF(12, 8)); p.drawLine(QPointF(8, 10), QPointF(12, 10)); p.drawLine(QPointF(8, 12), QPointF(12, 12));
        });
    case Paste:
        return makeIcon([](QPainter& p) {
            p.setPen(QPen(c3(110, 80, 30), 0.9)); p.setBrush(c3(200, 150, 80));
            p.drawRect(QRectF(2, 3, 9, 11));
            p.setBrush(c3(120, 120, 130)); p.drawRect(QRectF(4, 1.5, 5, 3));
            p.setPen(QPen(c3(90, 90, 90), 0.9)); p.setBrush(Qt::white);
            p.drawRect(QRectF(7, 7, 7, 8));
        });
    case Undo:
    case Redo: {
        const bool undo = (id == Undo);
        return makeIcon([undo](QPainter& p) {
            if (!undo) { p.translate(16, 0); p.scale(-1, 1); }
            const QColor c = c3(30, 80, 190);
            p.setPen(QPen(c, 1.8, Qt::SolidLine, Qt::RoundCap)); p.setBrush(Qt::NoBrush);
            QPainterPath path; path.moveTo(5.5, 6); path.cubicTo(13.5, 3.5, 15, 11.5, 8.5, 13);
            p.drawPath(path);
            arrowHead(p, QPointF(2, 6.5), QPointF(-1, 0.1), c);
        });
    }
    case Select:
        return makeIcon([](QPainter& p) {
            p.setPen(QPen(Qt::black, 0.9)); p.setBrush(Qt::white);
            p.drawPolygon(QPolygonF({QPointF(4, 1.5), QPointF(4, 13.5), QPointF(6.8, 10.8), QPointF(9, 15), QPointF(10.8, 14.1), QPointF(8.7, 9.9), QPointF(12.5, 9.8)}));
        });
    case Move:
        return makeIcon([](QPainter& p) {
            const QColor c = c3(30, 30, 30);
            p.setPen(QPen(c, 1.2)); p.drawLine(QPointF(8, 3), QPointF(8, 13)); p.drawLine(QPointF(3, 8), QPointF(13, 8));
            arrowHead(p, QPointF(8, 1), QPointF(0, -1), c); arrowHead(p, QPointF(8, 15), QPointF(0, 1), c);
            arrowHead(p, QPointF(1, 8), QPointF(-1, 0), c); arrowHead(p, QPointF(15, 8), QPointF(1, 0), c);
        });
    case Resize:
        return makeIcon([](QPainter& p) {
            const QColor c = c3(30, 30, 30);
            p.setPen(QPen(c3(90, 90, 90), 0.9, Qt::DashLine)); p.setBrush(Qt::NoBrush);
            p.drawRect(QRectF(2, 7, 7, 7));
            p.setPen(QPen(c, 1.3)); p.drawLine(QPointF(6, 10), QPointF(12, 4));
            arrowHead(p, QPointF(14.5, 1.5), QPointF(0.7, -0.7), c);
        });
    case Delete:
        return makeIcon([](QPainter& p) {
            p.setPen(QPen(c3(200, 30, 30), 2.2, Qt::SolidLine, Qt::RoundCap));
            p.drawLine(QPointF(3.5, 3.5), QPointF(12.5, 12.5)); p.drawLine(QPointF(12.5, 3.5), QPointF(3.5, 12.5));
        });
    case Workspace:
        return makeIcon([](QPainter& p) {
            p.setPen(QPen(c3(20, 60, 120), 0.9)); p.setBrush(c3(70, 140, 220));
            p.drawEllipse(QPointF(8, 8), 6.3, 6.3);
            p.setPen(Qt::NoPen); p.setBrush(c3(80, 170, 80));
            p.drawEllipse(QPointF(6, 6), 2.6, 2.2); p.drawEllipse(QPointF(10.5, 10), 2.4, 2.6);
        });
    case Brick:
        return makeIcon([](QPainter& p) { drawCube(p, c3(163, 162, 165)); });
    case About:
        return makeIcon([](QPainter& p) {
            p.setPen(QPen(c3(20, 60, 120), 0.9)); p.setBrush(c3(60, 120, 210));
            p.drawEllipse(QPointF(8, 8), 6.5, 6.5);
            p.setPen(QPen(Qt::white, 1.8, Qt::SolidLine, Qt::RoundCap));
            p.drawLine(QPointF(8, 7), QPointF(8, 11.5)); p.drawPoint(QPointF(8, 4.6));
        });
    }
    return QIcon();
}

}  // namespace Icons
