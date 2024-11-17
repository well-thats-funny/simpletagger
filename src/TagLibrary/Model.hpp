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
#include "Node.hpp"

namespace TagLibrary {
class NodeRoot;

static constexpr QStringView mimeType = u"application/x.simpletagger.taglibrary.nodes";

class Model: public QAbstractItemModel {
    Q_OBJECT

    friend class Node;

public:
    Model(Model const &other) = delete;
    Model(Model &&other) = delete;
    Model& operator=(Model const &other) = delete;
    Model& operator=(Model &&other) = delete;

    Model();
    ~Model() override;

    enum class Column {
        Name = 0,
        Tag = 1
    };
    Q_ENUM(Column);

    [[nodiscard]] QModelIndex toIndex(Node const &node) const;
    [[nodiscard]] std::pair<QModelIndex, QModelIndex> toIndexRange(Node const &node) const;
    [[nodiscard]] Node &fromIndex(QModelIndex const &index) const;

    [[nodiscard]] std::expected<std::reference_wrapper<Node>, QString> fromUuid(QUuid const &uuid) const;

    [[nodiscard]] QModelIndex index(int row, int column, QModelIndex const &parent = QModelIndex()) const override;
    [[nodiscard]] QModelIndex parent(QModelIndex const &child) const override;
    [[nodiscard]] int rowCount(QModelIndex const &parent) const override;
    [[nodiscard]] int recursiveRowCount(QModelIndex const &parent) const;
    [[nodiscard]] int columnCount(QModelIndex const &parent) const override;
    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    [[nodiscard]] Qt::ItemFlags flags(const QModelIndex &index) const override;
    [[nodiscard]] QVariant data(QModelIndex const &index, int role) const override;
    [[nodiscard]] bool setData(QModelIndex const &index, QVariant const &value, int role = Qt::EditRole) override;
    [[nodiscard]] bool canInsertNode(NodeType type, QModelIndex const &parent) const;
    [[nodiscard]] std::expected<QModelIndex, QString> insertNode(NodeType type, QModelIndex const &parent);
    [[nodiscard]] bool canRemoveRow(QModelIndex const &index);
    // NOTE: we need removeRows at least because move drag-and-drop requires it. So we can probably simply implement it
    //       as our main way of removing nodes.
    // TODO: Perhaps it'd be good to also provide propert insertRows implementation? Reasons:
    //        - symmetry with removeRows, that we apparently require
    //        - some yet unknown places in Qt that make use of it (perhaps we could just rely on builtin d-n-d with that?)
    //        -
    [[nodiscard]] bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    [[nodiscard]] QStringList mimeTypes() const override;
    [[nodiscard]] QMimeData *mimeData(const QModelIndexList &indexes) const override;
    [[nodiscard]] Qt::DropActions supportedDropActions() const override;
    [[nodiscard]] bool canDropMimeData(QMimeData const *data, Qt::DropAction action, int row, int column, QModelIndex const &parent) const override;
    [[nodiscard]] bool dropMimeData(QMimeData const *data, Qt::DropAction action, int row, int column, QModelIndex const &parent) override;

    [[nodiscard]] std::expected<void, QString> resetRoot();
    [[nodiscard]] std::expected<QCborValue, QString> save() const;
    [[nodiscard]] std::expected<void, QString> load(QCborValue const &value);

    void setEditMode(bool editMode);
    [[nodiscard]] bool editMode() const;

    [[nodiscard]] std::expected<void, QString> setTagsActive(QStringList const &tags);
    void setRowHeight(int rowHeight);
    [[nodiscard]] std::expected<QModelIndexList, QString> setHighlightedTags(QStringList const &tags);

    void setHighlightChangedAfterVersion(std::optional<int> const &version);

    void setNextLibraryVersion(int nextVersion);

    [[nodiscard]] std::expected<void, Error> invalidateTagCaches() const;

    QStringList allTags() const;

signals:
    void loadComplete();
    void activeChanged(Node const &node, bool active);
    void persistentDataChanged();

private:
    std::unique_ptr<NodeRoot> root;
    bool editMode_ = false;
    int rowHeight_ = 0;
    int nextLibraryVersion_ = -1;
    std::optional<int> highlightChangedAfterVersion_;

    mutable QMutex allTagsMutex_;
    mutable std::optional<QStringList> allTags_;
};
}
