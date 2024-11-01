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
#include "TagsAssignedListModel.hpp"

#include "FileTagger.hpp"

namespace Tags {
TagsAssignedListModel::TagsAssignedListModel(FileTagger &fileTagger): fileTagger{fileTagger} {
    ZoneScoped;

    connect(&fileTagger, &FileTagger::tagsChanged, this, [this]{
        ZoneScoped;
        beginResetModel();
        endResetModel();
    });
}

TagsAssignedListModel::~TagsAssignedListModel() = default;

bool TagsAssignedListModel::canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column,
                                            const QModelIndex &parent) const {
    ZoneScoped;
    return QAbstractListModel::canDropMimeData(data, action, row, column, parent);
}

int TagsAssignedListModel::rowCount(const QModelIndex &parent) const {
    ZoneScoped;
    gsl_Expects(!parent.isValid());
    return fileTagger.assignedTags().transform([](auto const &v){ return v.size(); }).value_or(0);
}

Qt::ItemFlags TagsAssignedListModel::flags(QModelIndex const &index) const {
    ZoneScoped;

    auto flags = QAbstractListModel::flags(index);

    if (index.isValid())
        flags |= Qt::ItemFlag::ItemIsDragEnabled;
    else
        flags |= Qt::ItemFlag::ItemIsDropEnabled;

    return flags;
}

QVariant TagsAssignedListModel::data(const QModelIndex &index, int role) const {
    ZoneScoped;

    if (!index.isValid() || index.parent().isValid())
        return {};

    auto tag = fileTagger.assignedTags()
            .transform([&](auto const &v)->QVariant{ return v.at(index.row()); })
            .value_or(QVariant());

    switch (role) {
        case Qt::ItemDataRole::DisplayRole:
        case TagRole:
            return tag;
        case Qt::ItemDataRole::BackgroundRole:
            return highlightedTags_.contains(tag)
                ? QVariant::fromValue(QBrush(QColor(64, 64, 255, 128)))
                : QVariant();
        default:
            return {};
    }
}

Qt::DropActions TagsAssignedListModel::supportedDropActions() const {
    return Qt::MoveAction;
}

bool TagsAssignedListModel::moveRows(QModelIndex const &sourceParent, int sourceRow, int count, QModelIndex const &destinationParent, int destinationChild) {
    ZoneScoped;
    gsl_Expects(!sourceParent.isValid());
    gsl_Expects(!destinationParent.isValid());
    gsl_Expects(count == 1); // NOTE: this will only support moving single items for now
    return fileTagger.moveAssignedTag(sourceRow, destinationChild);
}

QModelIndexList TagsAssignedListModel::setHighlightedTags(QStringList const &tags) {
    ZoneScoped;

    auto previousTags = highlightedTags_;

    highlightedTags_ = tags;

    auto tagToIndex = [&](QString const &tag)->QModelIndex{
        if (auto pos = fileTagger.assignedTags()->indexOf(tag); pos != -1)
            return this->index(pos, 0);
        else
            return {};
    };

    auto emitTagUpdate = [&](QString const &tag){
        if (auto idx = tagToIndex(tag); idx.isValid())
            emit dataChanged(idx, idx);
    };

    for (auto const &previousTag: previousTags)
        if (!highlightedTags_.contains(previousTag))
            emitTagUpdate(previousTag);

    for (auto const &newTag: highlightedTags_)
        if (!previousTags.contains(newTag))
            emitTagUpdate(newTag);

    return highlightedTags_
        | std::views::transform(tagToIndex)
        | std::ranges::to<QModelIndexList>();
}
}
