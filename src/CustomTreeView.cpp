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
