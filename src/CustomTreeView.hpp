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
#include "CustomItemViewHelper.hpp"

class CustomTreeView: public QTreeView {
    Q_OBJECT

    CustomTreeView(CustomTreeView const &other) = delete;
    CustomTreeView(CustomTreeView &&other) = delete;
    CustomTreeView& operator=(CustomTreeView const &other) = delete;
    CustomTreeView& operator=(CustomTreeView &&other) = delete;

public:
    explicit CustomTreeView(QWidget *parent = nullptr, CustomItemDelegate *delegate = nullptr);
    ~CustomTreeView() override;

    CustomItemDelegate *itemDelegate() const;

    QByteArray saveExpandedState() const;
    void restoreExpandedState(QByteArray const &state);

    void setModel(QAbstractItemModel *model) override;

private:
    CustomItemViewHelper helper_;
    QByteArray expandedState_;
};
