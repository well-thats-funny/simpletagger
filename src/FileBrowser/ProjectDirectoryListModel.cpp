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
#include "ProjectDirectoryListModel.hpp"

namespace FileBrowser {
ProjectDirectoryListModel::ProjectDirectoryListModel() = default;

ProjectDirectoryListModel::~ProjectDirectoryListModel() = default;

void ProjectDirectoryListModel::setDirectories(QStringList const &directories) {
    ZoneScoped;
    beginResetModel();
    directories_ = directories;
    endResetModel();
}

void ProjectDirectoryListModel::addDirectory(QString const &directory) {
    ZoneScoped;
    beginInsertRows(QModelIndex(), directories_.size(), directories_.size());
    directories_.push_back(directory);
    endInsertRows();
}

void ProjectDirectoryListModel::removeDirectory(int const row) {
    ZoneScoped;
    beginRemoveRows(QModelIndex(), row, row);
    directories_.erase(directories_.begin() + row);
    endRemoveRows();
}

QStringList const &ProjectDirectoryListModel::directories() const {
    return directories_;
}

QString ProjectDirectoryListModel::directory(int const row) const {
    return directories_.at(row);
}

QModelIndex ProjectDirectoryListModel::index(int const row, int const column, QModelIndex const &) const {
    ZoneScoped;
    return createIndex(row, column, nullptr);
}

QModelIndex ProjectDirectoryListModel::parent(QModelIndex const &) const {
    ZoneScoped;
    return QModelIndex();
}

int ProjectDirectoryListModel::rowCount(QModelIndex const &parent) const {
    ZoneScoped;
    if (!parent.isValid())
        return directories_.size();
    else
        return 0;
}

int ProjectDirectoryListModel::columnCount(QModelIndex const &) const {
    return 1;
}

QVariant ProjectDirectoryListModel::data(QModelIndex const &index, int const role) const {
    ZoneScoped;
    gsl_Expects(index.row() >= -1);
    gsl_Expects(index.row() < directories_.size());

    if (index.row() == -1)
        return {};

    QVariant result;

    switch (role) {
        case Qt::ItemDataRole::DisplayRole: {
            result = directories_[index.row()];
            break;
        }
    }

    return result;
}
}
