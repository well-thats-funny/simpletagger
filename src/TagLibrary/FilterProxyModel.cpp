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
#include "FilterProxyModel.hpp"

#include "Logging.hpp"
#include "Model.hpp"

namespace TagLibrary {
FilterProxyModel::~FilterProxyModel() = default;

void FilterProxyModel::setEditMode(bool const editMode) {
    if (editMode != editMode_) {
        editMode_ = editMode;
        invalidateRowsFilter();
    }
}

void FilterProxyModel::setFilterOnlyChanged(bool const onlyChanged) {
    if (onlyChanged != onlyChanged_) {
        onlyChanged_ = onlyChanged;
        invalidateRowsFilter();
    }
}

void FilterProxyModel::setChangedAfterVersion(std::optional<int> const &version) {
    if (version != changedAfterVersion_) {
        changedAfterVersion_ = version;
        invalidateRowsFilter();
    }
}

bool FilterProxyModel::filterAcceptsRow(int const sourceRow, QModelIndex const &sourceParent) const {
    auto source = qobject_cast<Model*>(sourceModel());
    gsl_Expects(source);

    auto &node = source->fromIndex(source->index(sourceRow, 0, sourceParent));

    if (changedAfterVersion_ && onlyChanged_) {
        if (auto lastChangeAfter = node.lastChangeAfter(*changedAfterVersion_, true); !lastChangeAfter)
            qWarning() << "lastChangeAfter error:" << lastChangeAfter.error();
        else if (!*lastChangeAfter)
            return false;
    }

    if (editMode_) {
        return true;
    } else {
        return !node.isHidden();
    }
}
}