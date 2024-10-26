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
#pragma once

namespace ImageViewer {
class GraphicsSelectionActiveCorner;
class GraphicsSelectionActiveLabel;

class GraphicsSelectionRectItem: public QGraphicsItem {
public:
    GraphicsSelectionRectItem(QRect const rect, QRect const limitRect);
    ~GraphicsSelectionRectItem();

    void setEditable(bool editable);

    void setRect(QRect const &rect);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    QPen const &areaPen() const {
        return areaPen_;
    }

    void setAreaPen(QPen const &pen) {
        areaPen_ = pen;
        updateStyles();
    }

    QBrush const &areaBrush() const {
        return areaBrush_;
    }

    void setAreaBrush(QBrush const &brush) {
        areaBrush_ = brush;
        updateStyles();
    }

    QPen const &editAreaPen() const {
        return editAreaPen_;
    }

    void setEditAreaPen(QPen const &pen) {
        editAreaPen_ = pen;
        updateStyles();
    }

    QBrush const &editAreaBrush() const {
        return editAreaBrush_;
    }

    void setEditAreaBrush(QBrush const &brush) {
        editAreaBrush_ = brush;
        updateStyles();
    }

    QPen const &editErrorAreaPen() const {
        return editErrorAreaPen_;
    }

    void setEditErrorAreaPen(QPen const &pen) {
        editErrorAreaPen_ = pen;
        updateStyles();
    }

    QBrush const &editErrorAreaBrush() const {
        return editErrorAreaBrush_;
    }

    void setEditErrorAreaBrush(QBrush const &brush) {
        editErrorAreaBrush_ = brush;
        updateStyles();
    }

    QString labelText() const {
        return label->text();
    }

    void setLabelText(QString const &text) {
        label->setText(text);
        updateLabelLayout();
    }

    QFont const &labelTextFont() const {
        return labelTextFont_;
    }

    void setLabelTextFont(QFont const &font) {
        labelTextFont_ = font;
        updateStyles();
    }

    QPen const &labelTextPen() const {
        return labelTextPen_;
    }

    void setLabelTextPen(QPen const &pen) {
        labelTextPen_ = pen;
        updateStyles();
    }

    QBrush const &labelTextBrush() const {
        return labelTextBrush_;
    }

    void setLabelTextBrush(QBrush const &brush) {
        labelTextBrush_ = brush;
        updateStyles();
    }

    QPen const &labelAreaPen() const {
        return labelAreaPen_;
    }

    void setLabelAreaPen(QPen const &pen) {
        labelAreaPen_ = pen;
        updateStyles();
    }

    QBrush const &labelAreaBrush() const {
        return labelAreaBrush_;
    }

    void setLabelAreaBrush(QBrush const &brush) {
        labelAreaBrush_ = brush;
        updateStyles();
    }

    QPen const &activeCornerPen() const {
        return activeCornerPen_;
    }

    void setActiveCornerPen(QPen const &pen) {
        activeCornerPen_ = pen;
        updateStyles();
    }

    QBrush const &activeCornerBrush() const {
        return activeCornerBrush_;
    }

    void setActiveCornerBrush(QBrush const &brush) {
        activeCornerBrush_ = brush;
        updateStyles();
    }

    float scaleCorrection() const {
        return scaleCorrection_;
    }

    void setScaleCorrection(float const value) {
        scaleCorrection_ = value;
        updateStyles();
    }

    void setEnforcedAspectRatios(QList<QSize> const &aspectRatios);

    void setEnforcedAspectRatiosEnabled(bool enabled) {
        aspectRatiosEnabled_ = enabled;
    }

    std::function<void(QRect const &, bool)> onRectChanged;

private:
    void setRect(QRect const &rect, bool final);
    void updateLabelLayout();
    void updateStyles();
    bool goodAspectRatio(QSize const &value);
    QSize closestAspectRatio(QSizeF const &value);

    bool editable_ = true;

    QRect rect_;
    QRect limitRect_;

    QPen areaPen_;
    QBrush areaBrush_;

    QPen editAreaPen_;
    QBrush editAreaBrush_;

    QPen editErrorAreaPen_;
    QBrush editErrorAreaBrush_;

    QFont labelTextFont_;
    QPen labelTextPen_;
    QBrush labelTextBrush_;
    QPen labelAreaPen_;
    QBrush labelAreaBrush_;

    QPen activeCornerPen_;
    QBrush activeCornerBrush_;

    QList<QSize> aspectRatios_;
    bool aspectRatiosEnabled_ = false;

    bool useErrorStyle = false;

    QGraphicsRectItem* mainRect = nullptr;
    std::array<GraphicsSelectionActiveCorner *, 8> activeCorners = {};
    GraphicsSelectionActiveLabel* labelBackgroundRect = nullptr;
    QGraphicsSimpleTextItem *label = nullptr;
    QGraphicsPolygonItem *outside = nullptr;

    float scaleCorrection_ = 1.0;

    QString styleSheet_;
};
}
