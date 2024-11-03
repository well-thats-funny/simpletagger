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
#include "TreeView.hpp"

#include "Logging.hpp"
#include "../IconIdentifier.hpp"

namespace TagLibrary {
TreeViewDelegate::~TreeViewDelegate() = default;

void TreeViewDelegate::initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const {
    ZoneScoped;

    CustomItemDelegate::initStyleOption(option, index);

    if (option->icon.isNull()) {
        // perhaps we want to draw a vector of icons?
        if (auto decoration = index.data(Qt::ItemDataRole::DecorationRole); decoration.canConvert<std::vector<IconIdentifier>>()) {
            auto icons = decoration.value<std::vector<IconIdentifier>>();

            // TODO: in case there's multiple same consecutive icons, we can replace it by a single icon with
            //       a number

            // note: both parameters come from height(). this is correct, as we want each icon
            //       to be square and have a height of the row
            QSize iconSize{option->decorationSize.height(), option->decorationSize.height()};

            QPixmap pixmap(iconSize.width() * icons.size(), iconSize.height());
            pixmap.fill(QColor(0, 0, 0, 0));

            QPainter painter(&pixmap);

            for (auto const &[i, icon]: icons | std::views::enumerate)
                icon.instantiate(iconCache).paint(&painter, QRect(QPoint(i * iconSize.width(), 0), iconSize));

            option->icon = QIcon(pixmap);

            // use pixmap size
            option->decorationSize = pixmap.size();
        }
    }
}

bool TreeViewDelegate::helpEvent(
        QHelpEvent *const event, QAbstractItemView *const view,
        QStyleOptionViewItem const &option, QModelIndex const &index
) {
    ZoneScoped;

    switch (event->type()) {
        case QEvent::ToolTip: {
            auto tooltip = index.isValid() ? index.data(Qt::ItemDataRole::ToolTipRole) : QString{};
            if (tooltip.isValid()) {
                QToolTip::showText(
                        event->globalPos(),
                        QString("<html>%1</html>").arg(tooltip.toString()),
                        view->viewport(),
                        option.rect
                );
            }

            event->setAccepted(tooltip.isValid());
            break;
        }
        default:
            return QStyledItemDelegate::helpEvent(event, view, option, index);
    }

    return event->isAccepted();
}

TreeView::TreeView(QWidget *const parent): CustomTreeView(new TreeViewDelegate(), parent) {}

TreeView::~TreeView() = default;

TreeViewDelegate *TreeView::itemDelegate() const {
    auto delegate = dynamic_cast<TreeViewDelegate *>(CustomTreeView::itemDelegate());
    gsl_Expects(delegate);
    return delegate;
}

void TreeView::requestSelection(
        std::function<bool(QModelIndex const &)> const &isSelectable,
        std::function<bool(const QModelIndex &)> const &onSelection
) {
    ZoneScoped;

    selectionModel()->clear();

    selectionOperation.emplace(isSelectable, onSelection);

    selectionOperation->selectionModelConnection = connect(selectionModel(), &QItemSelectionModel::currentChanged, this, [this](QModelIndex const &current, QModelIndex const &/*previous*/){
        gsl_Expects(selectionOperation);
        if (current.isValid()) {
            if (selectionOperation->isSelectable(current) && selectionOperation->onSelection(current)) {
                resetRequestSelection();
            } else {
                selectionModel()->clear();
            }
        }
    });
}

void TreeView::resetRequestSelection() {
    ZoneScoped;
    disconnect(selectionOperation->selectionModelConnection);
    selectionOperation.reset();
}

bool TreeView::isRequestSelectionActive() const {
    return selectionOperation.has_value();
}
}
