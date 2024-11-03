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
#include "CustomTreeView.hpp"

#include "Utility.hpp"

CustomItemDelegate::~CustomItemDelegate() = default;

void CustomItemDelegate::initStyleOption(QStyleOptionViewItem *const option, QModelIndex const &index) const {
    ZoneScoped;

    QStyledItemDelegate::initStyleOption(option, index);

    option->backgroundBrush = QBrush(); // remove the background brush, as we handle it manually in paint()
}

void CustomItemDelegate::paint(QPainter *const painter, QStyleOptionViewItem const &option, QModelIndex const &index) const {
    ZoneScoped;

    // draw background not only under the item but all the way to the left border of the widget
    auto rect = option.rect;

    if (index.column() == 0) {
        // not sure whether it's the best way, but hey!
        rect.setLeft(0);
    }

    if (auto brushes = index.data(Qt::ItemDataRole::BackgroundRole); !brushes.isNull()) {
        if (brushes.canConvert<std::vector<QBrush>>()) {
            auto brushesVector = brushes.value<std::vector<QBrush>>();
            for (auto const &brush: withoutDuplicates(brushesVector))
                painter->fillRect(rect, brush);
        } else if (brushes.canConvert<QBrush>()) {
            painter->fillRect(rect, brushes.value<QBrush>());
        } else
            gsl_Expects(false);
    }

    return QStyledItemDelegate::paint(painter, option, index);
}

CustomTreeView::CustomTreeView(CustomItemDelegate *const delegate, QWidget *const parent): QTreeView(parent) {
    ZoneScoped;
    gsl_Expects(delegate);
    delegate->setParent(this);
    setItemDelegate(delegate);
}

CustomTreeView::~CustomTreeView() = default;

CustomItemDelegate *CustomTreeView::itemDelegate() const {
    auto delegate = dynamic_cast<CustomItemDelegate *>(QTreeView::itemDelegate());
    gsl_Expects(delegate);
    return delegate;
}

QByteArray CustomTreeView::saveExpandedState() const {
    ZoneScoped;

    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_6_7);

    [&, t=this](this auto &&self, QModelIndex const &index)->void{
        stream << t->isExpanded(index);

        for (int row = 0; row != t->model()->rowCount(index); ++row)
            self(t->model()->index(row, 0, index));
    }(rootIndex());

    return qCompress(data);
}

void CustomTreeView::restoreExpandedState(QByteArray const &state) {
    ZoneScoped;

    if (!model()) {
        // this should only happen if restoring happens before model gets assigned
        // if so, save the expanded state for later and apply it after the model is set
        expandedState_ = state;
        return;
    }

    QByteArray sd = qUncompress(state);
    QDataStream stream(&sd, QIODevice::ReadOnly);

    [&, t=this](this auto &&self, QModelIndex const &index)->void{
        bool isExpanded = false;
        stream >> isExpanded;
        t->setExpanded(index, isExpanded);

        for (int row = 0; row != t->model()->rowCount(index); ++row)
            self(t->model()->index(row, 0, index));
    }(rootIndex());
}

void CustomTreeView::setModel(QAbstractItemModel *const model) {
    ZoneScoped;

    QTreeView::setModel(model);

    if (model && !expandedState_.isNull()) {
        restoreExpandedState(expandedState_);
        expandedState_.clear();
    }
}
