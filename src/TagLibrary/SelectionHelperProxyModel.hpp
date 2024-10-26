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

namespace TagLibrary {
class SelectionHelperProxyModel: public QIdentityProxyModel {
public:
    SelectionHelperProxyModel(SelectionHelperProxyModel const &other) = delete;
    SelectionHelperProxyModel(SelectionHelperProxyModel &&other) = delete;
    SelectionHelperProxyModel& operator=(SelectionHelperProxyModel const &other) = delete;
    SelectionHelperProxyModel& operator=(SelectionHelperProxyModel &&other) = delete;

    explicit SelectionHelperProxyModel(QObject *parent = nullptr);
    ~SelectionHelperProxyModel() override;

    [[nodiscard]] QVariant data(QModelIndex const &proxyIndex, int role = Qt::DisplayRole) const override;

    void requestSelection(std::function<bool(QModelIndex const &)> const &isSelectable);
    void resetRequestSelection();

private:
    struct SelectionOperation {
        std::function<bool(QModelIndex const &)> isSelectable;
    };
    std::optional<SelectionOperation> selectionOperation;

    QBrush brushNotSelectable;
};
}
