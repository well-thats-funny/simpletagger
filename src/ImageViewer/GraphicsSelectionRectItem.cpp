/*
    Copyright (C) 2024 fdresufdresu@gmail.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include "GraphicsSelectionRectItem.hpp"

#include "Utility.hpp"

namespace ImageViewer {
static float ACTIVE_RECT_SIZE = 50.f;

enum Corner {
    TopLeft,
    Top,
    TopRight,
    Left,
    Right,
    BottomLeft,
    Bottom,
    BottomRight
};

QPoint limitToRect(QPoint point, QRect const &rect) {
    ZoneScoped;

    if (point.x() < rect.left())
        point.setX(rect.left());

    if (point.x() > rect.right())
        point.setX(rect.right());

    if (point.y() < rect.top())
        point.setY(rect.top());

    if (point.y() > rect.bottom())
        point.setY(rect.bottom());

    return point;
}

bool fitsAspectRatio(QSize const &value, QSizeF const &ratio) {
    ZoneScoped;
    return ratio == toMinimalAspectRatio(value);
}

// on a line defined by 'a' and 'b' as y=ax+b, find the point closest to 'point'
// whose coordinates are round integers. 'point' should be a point on the
// line. 'limits' provides a limits rectangle
//
// as of now, a quite primitive function - iterates down over every integer value
// on X axis, starting from round(point.x()) and checking if the correlating Y
// value is an integer
std::optional<QPointF> findClosestIntegerPointOnLine(
    QPointF const &point, qreal const a, qreal const b, QRectF limits
) {
    ZoneScoped;

    // find the closest integer point that matches the ratio
    qreal x = std::round(point.x());
    while(true) {
        qreal y = a*x + b;
        QPointF point(x, y);
        if (!limits.contains(point))
            return std::nullopt;

        if (floor(y) == y)
            return point;

        ++x;
    }
}

std::optional<QPoint> findClosestPointByAspectRatio(
    QRect const &rect, QPoint const &next, QList<QSize> aspectRatios, QRect const &limits, int const cornerId
) {
    ZoneScoped;
    /*qDebug() << "------ findClosestPointByAspectRatio -----";
    qDebug() << "- LIMITS:" << limits;
    qDebug() << "- RECT:" << rect;
    qDebug() << "- NEXT:" << next;*/

    qreal closestQDist = std::numeric_limits<qreal>::max();
    std::optional<QPoint> closestPointInt;
    for (auto const &[i, ratio]: aspectRatios | std::views::enumerate) {
        // linear functon y = ax + b
        gsl_Expects(cornerId >= 0 && cornerId <= 3);
        qreal a = 0.0f;
        qreal b = 0.0f;
        switch (cornerId) {
            case 0:
                a = (static_cast<qreal>(ratio.height())) / static_cast<qreal>(ratio.width());
                b = rect.bottom() - a * rect.right();
                break;
            case 1:
                a = -(static_cast<qreal>(ratio.height())) / static_cast<qreal>(ratio.width());
                b = rect.bottom() - a * rect.left();
                break;
            case 2:
                a = -(static_cast<qreal>(ratio.height())) / static_cast<qreal>(ratio.width());
                b = rect.top() - a * rect.right();
                break;
            case 3:
                a = (static_cast<qreal>(ratio.height())) / static_cast<qreal>(ratio.width());
                b = rect.top() - a * rect.left();
                break;
        }

        auto pointF = closestPointOnLine(a, b, next);

        // TODO: better matching function
        std::optional<QPoint> point;

        int xStart = static_cast<int>(std::round(pointF.x()));

        // unexpected true means out of range
        // unexpected false means just couldn't match
        auto tryWithOffset = [&](int const xOffset)->std::expected<QPoint, bool>{
            int x = xStart + xOffset;
            if (x < limits.left() || x > limits.right())
                return std::unexpected(true);

            qreal yf = a*static_cast<qreal>(x) + b;
            int y = static_cast<int>(std::floor(yf));
            if (yf == y) {
                if (y < limits.top() || y > limits.bottom())
                    return std::unexpected(true);

                return QPoint(x, y);
            }

            return std::unexpected(false);
        };

        bool outOfRange = false;
        for (int xOffset = 0;; ++xOffset) {
            bool outOfRange1 = false;
            if (auto p = tryWithOffset(xOffset)) {
                point = *p;
                break;
            } else if (p.error()) {
                outOfRange1 = true;
            }

            bool outOfRange2 = false;
            if (auto p = tryWithOffset(-xOffset)) {
                point = *p;
                break;
            } else if (p.error()) {
                outOfRange2 = true;
            }

            if (outOfRange1 && outOfRange2) {
                outOfRange = true;
                break;
            }
        }

        if (outOfRange)
            continue;

        auto diff = *point - next;
        auto sqDist = diff.x() * diff.x() + diff.y() * diff.y();
        qDebug() << "RATIO:" << QString::fromStdString(std::format("{}:{}", ratio.width(), ratio.height()))
                 << "POINT(F):" << QString::fromStdString(std::format("({}, {})", pointF.x(), pointF.y()))
                 << "POINT(I):" << QString::fromStdString(std::format("({}, {})", point->x(), point->y()))
                 << "DIST:" << QString::fromStdString(std::format("{}", std::sqrt(sqDist)));
        if (sqDist < closestQDist) {
            qDebug() << "--- BEST SO FAR ---";
            closestQDist = sqDist;
            closestPointInt = point;
        }
    }

    qDebug() << "RETURNING POINT" << *closestPointInt;
    return closestPointInt;
}

class GraphicsSelectionActiveCorner: public QGraphicsRectItem {
public:
    explicit GraphicsSelectionActiveCorner(Corner const corner, QRectF const &rect, QGraphicsItem *parent = nullptr):
        QGraphicsRectItem(parent), corner_(corner) {
        ZoneScoped;

        Qt::CursorShape cursor = [&]{
            switch (corner_) {
                case Corner::TopLeft: return Qt::CursorShape::SizeFDiagCursor;
                case Corner::Top: return Qt::CursorShape::SizeVerCursor;
                case Corner::TopRight: return Qt::CursorShape::SizeBDiagCursor;
                case Corner::Left: return Qt::CursorShape::SizeHorCursor;
                case Corner::Right: return Qt::CursorShape::SizeHorCursor;
                case Corner::BottomLeft: return Qt::CursorShape::SizeBDiagCursor;
                case Corner::Bottom: return Qt::CursorShape::SizeVerCursor;
                case Corner::BottomRight: return Qt::CursorShape::SizeFDiagCursor;
                default: std::terminate();
            }
        }();

        setCursor(cursor);

        setAcceptedMouseButtons(Qt::MouseButton::LeftButton);

        setFlag(QGraphicsItem::ItemIsSelectable);
        setFlag(QGraphicsItem::ItemIsMovable);
        setFlag(QGraphicsItem::ItemSendsGeometryChanges);

        reposition(rect);
    }

    void reposition(QRectF const &rect) {
        ZoneScoped;

        setPos([&] {
            switch (corner_) {
                case Corner::TopLeft: return rect.topLeft();
                case Corner::Top: return QPointF(rect.left() + rect.width()/2.f, rect.top());
                case Corner::TopRight: return rect.topRight();
                case Corner::Left: return QPointF(rect.left(), rect.top() + rect.height()/2.f);
                case Corner::Right: return QPointF(rect.right(), rect.top() + rect.height()/2.f);
                case Corner::BottomLeft: return rect.bottomLeft();
                case Corner::Bottom: return QPointF(rect.left() + rect.width()/2.f, rect.bottom());
                case Corner::BottomRight: return rect.bottomRight();
                default: std::terminate();
            }
        }());

        QPointF size(ACTIVE_RECT_SIZE/scaleFactor_, ACTIVE_RECT_SIZE/scaleFactor_);
        setRect(QRectF(-size/2.f, +size/2.f));
    }

    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override {
        ZoneScoped;

        auto prevPos = pos();

        QGraphicsRectItem::mouseMoveEvent(event);

        auto newPos = pos();

        if (!enableAxisX())
            newPos.setX(prevPos.x());

        if (!enableAxisY())
            newPos.setY(prevPos.y());

        newPos = onMovedFixPos(prevPos.toPoint(), newPos.toPoint()).toPointF();

        setPos(newPos);
        onMoved(scenePos().toPoint());
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override {
        ZoneScoped;

        QGraphicsRectItem::mouseReleaseEvent(event);
        onMoveEnd();
    }

    Corner corner() const {
        return corner_;
    }

    std::function<QPoint(QPoint, QPoint)> onMovedFixPos;
    std::function<void(QPoint)> onMoved;
    std::function<void()> onMoveEnd;

    qreal scaleFactor_ = 1.0;

private:
    bool enableAxisX() const {
        return corner_ != Corner::Top && corner_ != Corner::Bottom;
    }

    bool enableAxisY() const {
        return corner_ != Corner::Left && corner_ != Corner::Right;
    }

    Corner corner_;
};

class GraphicsSelectionActiveLabel: public QGraphicsRectItem {
public:
    std::function<QPoint(QPoint, QPoint)> onMovedFixPos;
    std::function<void(QPoint)> onMoved;
    std::function<void()> onMoveEnd;

    GraphicsSelectionActiveLabel(QGraphicsItem *parent = nullptr): QGraphicsRectItem(parent) {
        ZoneScoped;

        setCursor(Qt::SizeAllCursor);

        setAcceptedMouseButtons(Qt::MouseButton::LeftButton);

        setFlag(QGraphicsItem::ItemIsSelectable);
        setFlag(QGraphicsItem::ItemIsMovable);
        setFlag(QGraphicsItem::ItemSendsGeometryChanges);
    }

    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override {
        ZoneScoped;

        auto prev = pos();

        QGraphicsRectItem::mouseMoveEvent(event);

        setPos(onMovedFixPos(prev.toPoint(), pos().toPoint()).toPointF());

        onMoved(scenePos().toPoint());
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override {
        ZoneScoped;
        QGraphicsRectItem::mouseReleaseEvent(event);
        onMoveEnd();
    }
};

GraphicsSelectionRectItem::GraphicsSelectionRectItem(QRect const rect, QRect const limitRect): limitRect_(limitRect) {
    ZoneScoped;

    mainRect = new QGraphicsRectItem(rect, this);

    outside = new QGraphicsPolygonItem(this);
    outside->setPen(QPen());

    for (std::size_t i = 0; i != activeCorners.size(); ++i) {
        auto corner = static_cast<Corner>(i);
        activeCorners[i] = new GraphicsSelectionActiveCorner(corner, rect, this);

        activeCorners[i]->onMovedFixPos = [this, i, corner](QPoint const &, QPoint next)->QPoint{
            ZoneScoped;

            if (this->aspectRatiosEnabled_) {
                auto rect = rect_;
                rect = activeCorners[i]->transform().mapRect(rect);

                switch (corner) {
                    case Corner::TopLeft: {
                        rect.setTopLeft(next);
                        if (auto p = findClosestPointByAspectRatio(rect, next, aspectRatios_, limitRect_, 0))
                            next = *p;
                        break;
                    }
                    case Corner::Top: {
                        rect.setTop(next.y());
                        auto bestRatio = closestAspectRatio(rect.size());
                        qreal bestRatioF = static_cast<qreal>(bestRatio.width()) / static_cast<qreal>(bestRatio.height());
                        qreal newHeight = rect.width() / bestRatioF;
                        next.setY(rect.bottom() - newHeight);
                        break;
                    }
                    case Corner::TopRight: {
                        rect.setTopRight(next);
                        if (auto p = findClosestPointByAspectRatio(rect, next, aspectRatios_, limitRect_, 1))
                            next = *p;
                        break;
                    }
                    case Corner::Left: {
                        rect.setLeft(next.x());
                        auto bestRatio = closestAspectRatio(rect.size());
                        qreal bestRatioF = static_cast<qreal>(bestRatio.width()) / static_cast<qreal>(bestRatio.height());
                        qreal newWidth = rect.height() * bestRatioF;
                        next.setX(rect.right() - newWidth);
                        break;
                    }
                    case Corner::Right: {
                        rect.setRight(next.x());
                        auto bestRatio = closestAspectRatio(rect.size());
                        qreal bestRatioF = static_cast<qreal>(bestRatio.width()) / static_cast<qreal>(bestRatio.height());
                        qreal newWidth = rect.height() * bestRatioF;
                        next.setX(rect.left() + newWidth);
                        break;
                    }
                    case Corner::BottomLeft: {
                        rect.setBottomLeft(next);
                        if (auto p = findClosestPointByAspectRatio(rect, next, aspectRatios_, limitRect_, 2))
                            next = *p;
                        break;
                    }
                    case Corner::Bottom: {
                        rect.setBottom(next.y());
                        auto bestRatio = closestAspectRatio(rect.size());
                        qreal bestRatioF = static_cast<qreal>(bestRatio.width()) / static_cast<qreal>(bestRatio.height());
                        qreal newHeight = rect.width() / bestRatioF;
                        next.setY(rect.top() + newHeight);
                        break;
                    }
                    case Corner::BottomRight: {
                        rect.setBottomRight(next);
                        if (auto p = findClosestPointByAspectRatio(rect, next, aspectRatios_, limitRect_, 3))
                            next = *p;
                        break;
                    }
                    default:
                        assert(false);
                }
            }

            return limitToRect(next, limitRect_);
        };

        activeCorners[i]->onMoved = [this, corner](QPoint const &point_){
            ZoneScoped;

            auto point = transform().inverted().map(point_);
            auto newRect = rect_;

            switch (corner) {
                case Corner::TopLeft:
                    newRect.setTopLeft(point);
                    break;
                case Corner::Top:
                    newRect.setTop(point.y());
                    break;
                case Corner::TopRight:
                    newRect.setTopRight(point);
                    break;
                case Corner::Left:
                    newRect.setLeft(point.x());
                    break;
                case Corner::Right:
                    newRect.setRight(point.x());
                    break;
                case Corner::BottomLeft:
                    newRect.setBottomLeft(point);
                    break;
                case Corner::Bottom:
                    newRect.setBottom(point.y());
                    break;
                case Corner::BottomRight:
                    newRect.setBottomRight(point);
                    break;
                default: assert(false);
            }

            setRect(newRect, false);
        };

        activeCorners[i]->onMoveEnd = [&]{
            ZoneScoped;
            setRect(rect_, true);
        };
    }

    labelBackgroundRect = new GraphicsSelectionActiveLabel(this);
    labelBackgroundRect->onMovedFixPos = [&](QPoint const &, QPoint next)->QPoint{
        ZoneScoped;

        next = transform().inverted().map(next);

        auto main = rect_;
        auto limitRectTR = transform().inverted().mapRect(limitRect_);

        int top = limitRectTR.top() + main.height()/2.f;
        int bottom = limitRectTR.bottom() - main.height()/2.f;
        int left = limitRectTR.left() + main.width()/2.f;
        int right = limitRectTR.right() - main.width()/2.f;
        QRect rlimit(left, top, right - left, bottom - top);
        qDebug() << "rlimit" << rlimit << transform().mapRect(rlimit);
        next = limitToRect(next, rlimit);

        next = transform().map(next);
        return next;
    };
    labelBackgroundRect->onMoved = [&](QPoint const &point_) {
        ZoneScoped;
        auto point = transform().inverted().map(point_);
        auto newRect = rect_;
        newRect.moveCenter(point);
        setRect(newRect, false);
    };
    labelBackgroundRect->onMoveEnd = [&]() {
        ZoneScoped;
        setRect(rect_, true);
    };

    label = new QGraphicsSimpleTextItem(this);

    setRect(rect);
}

GraphicsSelectionRectItem::~GraphicsSelectionRectItem() = default;

void GraphicsSelectionRectItem::setEditable(bool const editable) {
    ZoneScoped;
    editable_ = editable;
    updateStyles();
}

void GraphicsSelectionRectItem::setRect(QRect const &rect) {
    ZoneScoped;
    setRect(rect, false);
}

QRectF GraphicsSelectionRectItem::boundingRect() const {
    ZoneScoped;

    return mainRect->boundingRect();
}

void GraphicsSelectionRectItem::paint(QPainter */*painter*/, const QStyleOptionGraphicsItem */*option*/, QWidget */*widget*/) {
    ZoneScoped;
    // nothing to do here, as our children are rendered anyway. but can't get rid of this function, as its pure virtual otherwise
}

void GraphicsSelectionRectItem::setEnforcedAspectRatios(QList<QSize> const &aspectRatios) {
    ZoneScoped;
    aspectRatios_ = aspectRatios | std::views::transform(toMinimalAspectRatio) | std::ranges::to<QList>();
}

void GraphicsSelectionRectItem::setRect(QRect const &rect, bool const final) {
    ZoneScoped;
    rect_ = rect;

    mainRect->setRect(rect);
    for (auto const &activeCorner: activeCorners)
        activeCorner->reposition(rect);

    useErrorStyle = aspectRatiosEnabled_ && !goodAspectRatio(QSize(rect.right() - rect.left(), rect.bottom() - rect.top()));

    updateLabelLayout();
    updateStyles();

    QPolygonF polygon;
    polygon.append(limitRect_.topLeft());
    polygon.append(limitRect_.topRight());
    polygon.append(limitRect_.bottomRight());
    polygon.append(limitRect_.bottomLeft());
    polygon.append(limitRect_.topLeft());
    polygon.append(rect.topLeft());
    polygon.append(rect.topRight());
    polygon.append(rect.bottomRight());
    polygon.append(rect.bottomLeft());
    polygon.append(rect.topLeft());
    outside->setPolygon(polygon);

    if (onRectChanged)
        onRectChanged(rect, final);
}

void GraphicsSelectionRectItem::updateLabelLayout() {
    ZoneScoped;
    label->setPos(mainRect->rect().center() - label->boundingRect().center());
    labelBackgroundRect->setPos(mainRect->rect().center());
    labelBackgroundRect->setRect(label->boundingRect().translated(-label->boundingRect().center()));
}

void GraphicsSelectionRectItem::updateStyles() {
    ZoneScoped;

    qreal scaleCorrection = scaleCorrection_;
    scaleCorrection *= 6.f;

    auto adjustPenWidth = [&](QPen const &pen) {
        QPen newPen = pen;
        newPen.setWidthF(newPen.widthF() / scaleCorrection);
        return newPen;
    };

    auto adjustFontSize = [&](QFont const &font){
        QFont newFont = font;
        newFont.setPointSizeF(newFont.pointSizeF() / scaleCorrection);
        return newFont;
    };

    mainRect->setPen(adjustPenWidth(!editable_ ? areaPen_ : (useErrorStyle ? editErrorAreaPen_ : editAreaPen_)));
    outside->setBrush(!editable_ ? areaBrush_ : (useErrorStyle ? editErrorAreaBrush_ : editAreaBrush_));

    label->setFont(adjustFontSize(labelTextFont_));
    label->setPen(adjustPenWidth(labelTextPen_));
    label->setBrush(labelTextBrush_);
    label->setVisible(editable_);

    updateLabelLayout(); // must update the label, as the font size of the label determines the layout (pos) of the background rect

    labelBackgroundRect->setPen(adjustPenWidth(labelAreaPen_));
    labelBackgroundRect->setBrush(labelAreaBrush_);
    labelBackgroundRect->setVisible(editable_);

    QPen activeCornerPen = adjustPenWidth(activeCornerPen_);

    for (auto &activeCorner: activeCorners) {
        activeCorner->setPen(activeCornerPen);
        activeCorner->setBrush(activeCornerBrush_);

        activeCorner->scaleFactor_ = scaleCorrection;
        activeCorner->reposition(rect_);
        activeCorner->setVisible(editable_);
    }

}

bool GraphicsSelectionRectItem::goodAspectRatio(const QSize &value) {
    ZoneScoped;

    for (auto const &ratio: aspectRatios_)
        if (fitsAspectRatio(value, ratio))
            return true;

    return false;
}

QSize GraphicsSelectionRectItem::closestAspectRatio(const QSizeF &value) {
    ZoneScoped;
    qreal currentRatio = value.width() / value.height();
    auto result = std::ranges::min(aspectRatios_
            | std::views::transform([&](auto const &ratio) {
                qreal r = static_cast<qreal>(ratio.width()) / static_cast<qreal>(ratio.height());
                return std::make_tuple(ratio, std::abs(r - currentRatio));
            }),
            [](auto const &a, auto const &b) { return std::get<1>(a) < std::get<1>(b); });
    return std::get<0>(result);
}
}
