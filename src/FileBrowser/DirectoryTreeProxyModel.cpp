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
#include "DirectoryTreeProxyModel.hpp"

namespace FileBrowser {
DirectoryTreeProxyModel::DirectoryTreeProxyModel(IsFileExcluded const &isFileExcluded, QObject *parent):
        QSortFilterProxyModel(parent), isFileExcluded_(isFileExcluded) {}

DirectoryTreeProxyModel::~DirectoryTreeProxyModel() = default;

void DirectoryTreeProxyModel::setShowExcluded(bool const showExcluded) {
    ZoneScoped;

    if (showExcluded != showExcluded_) {
        showExcluded_ = showExcluded;
        invalidateRowsFilter();
    }
}

bool DirectoryTreeProxyModel::filterAcceptsRow(int const sourceRow, QModelIndex const &sourceParent) const {
    ZoneScoped;

    if (!showExcluded_) {
        auto model = qobject_cast<QFileSystemModel *>(sourceModel());
        assert(model);
        auto sourceIndex = model->index(sourceRow, 0, sourceParent);
        auto path = model->filePath(sourceIndex);
        if (isFileExcluded_(path))
            return false;
    }

    return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
}
}