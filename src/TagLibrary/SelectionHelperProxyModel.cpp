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
#include "SelectionHelperProxyModel.hpp"

#include "../CustomItemDataRole.hpp"

namespace TagLibrary {
SelectionHelperProxyModel::SelectionHelperProxyModel(QObject *parent):
        QIdentityProxyModel(parent), brushNotSelectable(Qt::GlobalColor::red, Qt::BrushStyle::BDiagPattern) {}

SelectionHelperProxyModel::~SelectionHelperProxyModel() = default;

QVariant SelectionHelperProxyModel::data(QModelIndex const &proxyIndex, int const role) const {
    ZoneScoped;

    if (role == std::to_underlying(CustomItemDataRole::ExtendedBackgroundRole)
            && selectionOperation && !selectionOperation->isSelectable(proxyIndex)) {
        auto brushVariant = QIdentityProxyModel::data(proxyIndex, role);
        assert(brushVariant.canConvert<std::vector<QBrush>>());
        auto brushes = brushVariant.value<std::vector<QBrush>>();
        brushes.emplace_back(brushNotSelectable);
        return QVariant::fromValue(brushes);
    }

    return QIdentityProxyModel::data(proxyIndex, role);
}

void SelectionHelperProxyModel::requestSelection(std::function<bool(const QModelIndex &)> const &isSelectable) {
    ZoneScoped;

    selectionOperation.emplace(isSelectable);

    // consider all items outdated, so that they get repainted
    emit dataChanged(QModelIndex(), QModelIndex());
}

void SelectionHelperProxyModel::resetRequestSelection() {
    ZoneScoped;

    selectionOperation.reset();

    // consider all items outdated, so that they get repainted
    emit dataChanged(QModelIndex(), QModelIndex());
}
}
