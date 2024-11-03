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
#include "../CustomTreeView.hpp"
#include "../IconIdentifier.hpp"

namespace TagLibrary {
class TreeViewDelegate: public CustomItemDelegate {
    Q_OBJECT

public:
    using CustomItemDelegate::CustomItemDelegate;
    ~TreeViewDelegate() override;

    void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override;

    bool helpEvent(QHelpEvent *event, QAbstractItemView *view, QStyleOptionViewItem const &option, QModelIndex const &index) override;

    mutable IconIdentifier::IconCache iconCache;
};

class TreeView: public CustomTreeView {
    Q_OBJECT

public:
    TreeView(TreeView const &other) = delete;
    TreeView(TreeView &&other) = delete;
    TreeView& operator=(TreeView const &other) = delete;
    TreeView& operator=(TreeView &&other) = delete;

    explicit TreeView(QWidget *parent = nullptr);
    ~TreeView();

    TreeViewDelegate *itemDelegate() const;

    void requestSelection(
            std::function<bool(QModelIndex const &)> const &isSelectable,
            std::function<bool(QModelIndex const &)> const &onSelection
    );
    void resetRequestSelection();
    [[nodiscard]] bool isRequestSelectionActive() const;

private:
    struct SelectionOperation {
        std::function<bool(QModelIndex const &)> isSelectable;
        std::function<bool(QModelIndex const &)> onSelection;
        QMetaObject::Connection selectionModelConnection;
    };
    std::optional<SelectionOperation> selectionOperation;
};
}
