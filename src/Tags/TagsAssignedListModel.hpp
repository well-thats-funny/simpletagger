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

class FileEditor;

namespace Tags {
class TagsAssignedListModel: public QAbstractListModel {
    Q_OBJECT

public:
    static constexpr auto TagRole = Qt::ItemDataRole::UserRole + 1;

    explicit TagsAssignedListModel(FileEditor &fileEditor);
    ~TagsAssignedListModel();

    bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const override;

    [[nodiscard]] int rowCount(QModelIndex const &parent = QModelIndex()) const override;
    [[nodiscard]] Qt::ItemFlags flags(QModelIndex const &index) const override;
    [[nodiscard]] QVariant data(QModelIndex const &index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] Qt::DropActions supportedDropActions() const override;
    [[nodiscard]] bool moveRows(QModelIndex const &sourceParent, int sourceRow, int count, QModelIndex const &destinationParent, int destinationChild) override;

    QModelIndexList setHighlightedTags(QStringList const &tags);
    void setKnownTags(QStringList const &tags);

private:
    FileEditor &fileEditor_;
    QStringList highlightedTags_;
    QStringList knownTags_;
};
}
