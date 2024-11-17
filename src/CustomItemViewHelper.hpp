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

class CustomItemDelegate: public QStyledItemDelegate {
Q_OBJECT

    CustomItemDelegate(CustomItemDelegate const &other) = delete;
    CustomItemDelegate(CustomItemDelegate &&other) = delete;
    CustomItemDelegate& operator=(CustomItemDelegate const &other) = delete;
    CustomItemDelegate& operator=(CustomItemDelegate &&other) = delete;

public:
    using QStyledItemDelegate::QStyledItemDelegate;
    ~CustomItemDelegate() override;

    void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override;
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    void setExtendFirstColumnBackground(bool value);
    bool extendFirstColumnBackground() const;

private:
    bool extendFirstColumnBackground_ = false;
};

class CustomItemViewHelper {
public:
    CustomItemViewHelper(QAbstractItemView *view, CustomItemDelegate *delegate);

    CustomItemDelegate *itemDelegate() const;

private:
    QAbstractItemView *view_ = nullptr;
};
