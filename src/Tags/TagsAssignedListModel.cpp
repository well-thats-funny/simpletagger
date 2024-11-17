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

#include "../CustomItemDataRole.hpp"
#include "../FileEditor.hpp"

namespace Tags {
TagsAssignedListModel::TagsAssignedListModel(FileEditor &fileEditor): fileEditor_{fileEditor} {
    ZoneScoped;

    connect(&fileEditor, &FileEditor::tagsChanged, this, [this]{
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
    return fileEditor_.assignedTags().transform([](auto const &v){ return v.size(); }).value_or(0);
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

    auto tag = fileEditor_.assignedTags()
            .transform([&](auto const &v)->QVariant{ return v.at(index.row()); })
            .value_or(QVariant());

    switch (role) {
        case Qt::ItemDataRole::DisplayRole:
        case std::to_underlying(CustomItemDataRole::TagRole):
            return tag;
        case Qt::ItemDataRole::BackgroundRole: {
            std::vector<QBrush> result;
            if (highlightedTags_.contains(tag))
                result.emplace_back(QColor(64, 64, 255, 128), Qt::BrushStyle::SolidPattern);
            if (!knownTags_.contains(tag))
                result.emplace_back(QColor(255, 0,   0, 128), Qt::BrushStyle::FDiagPattern);
            return QVariant::fromValue(result);
        }
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
    if (auto result = fileEditor_.moveAssignedTag(sourceRow, destinationChild); !result) {
        QMessageBox::critical(qApp->activeWindow(), tr("Move tags failed"), result.error());
        return false;
    } else {
        return *result;
    }
}

QModelIndexList TagsAssignedListModel::setHighlightedTags(QStringList const &tags) {
    ZoneScoped;

    auto previousTags = highlightedTags_;

    highlightedTags_ = tags;

    auto tagToIndex = [&](QString const &tag)->QModelIndex{
        if (auto assignedTags = fileEditor_.assignedTags())
            if (auto pos = assignedTags->indexOf(tag); pos != -1)
                return this->index(pos, 0);

        return QModelIndex{};
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

void TagsAssignedListModel::setKnownTags(QStringList const &tags) {
    knownTags_ = tags;
    emit dataChanged(QModelIndex(), QModelIndex());
}
}
