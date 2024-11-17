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
#include "CustomItemViewHelper.hpp"

#include "CustomItemDataRole.hpp"
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

    if (extendFirstColumnBackground_ && index.column() == 0) {
        // not sure whether it's the best way, but hey!
        rect.setLeft(0);
    }

    if (auto brushes = index.data(std::to_underlying(CustomItemDataRole::ExtendedBackgroundRole)); !brushes.isNull()) {
        gsl_Expects(brushes.canConvert<std::vector<QBrush>>());
        auto brushesVector = brushes.value<std::vector<QBrush>>();
        for (auto const &brush: withoutDuplicates(brushesVector))
            painter->fillRect(rect, brush);
    }

    return QStyledItemDelegate::paint(painter, option, index);
}

void CustomItemDelegate::setExtendFirstColumnBackground(bool value) {
    extendFirstColumnBackground_ = value;
}

bool CustomItemDelegate::extendFirstColumnBackground() const {
    return extendFirstColumnBackground_;
}

CustomItemViewHelper::CustomItemViewHelper(QAbstractItemView *const view, CustomItemDelegate *delegate):
        view_(view) {
    assert(view_);

    if (!delegate)
        delegate = new CustomItemDelegate();

    delegate->setParent(view_);
    view_->setItemDelegate(delegate);
}

CustomItemDelegate *CustomItemViewHelper::itemDelegate() const {
    auto delegate = dynamic_cast<CustomItemDelegate *>(view_->itemDelegate());
    gsl_Expects(delegate);
    return delegate;
}
