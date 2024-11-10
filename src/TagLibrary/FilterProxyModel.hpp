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
class FilterProxyModel: public QSortFilterProxyModel {
public:
    using QSortFilterProxyModel::QSortFilterProxyModel;
    ~FilterProxyModel() override;

    void setEditMode(bool editMode);
    void setFilterOnlyChanged(bool onlyChanged);
    void setChangedAfterVersion(std::optional<int> const &version);

protected:
    bool filterAcceptsRow(int sourceRow, QModelIndex const &sourceParent) const override;

private:
    bool editMode_ = false;
    bool onlyChanged_ = false;
    std::optional<int> changedAfterVersion_;
};
}
