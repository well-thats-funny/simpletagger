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

namespace FileBrowser {
class DirectoryTreeProxyModel: public QSortFilterProxyModel {
public:
    using IsFileExcluded = std::function<bool(QString const &)>;

    DirectoryTreeProxyModel(DirectoryTreeProxyModel const &other) = delete;
    DirectoryTreeProxyModel(DirectoryTreeProxyModel &&other) = delete;
    DirectoryTreeProxyModel& operator=(DirectoryTreeProxyModel const &other) = delete;
    DirectoryTreeProxyModel& operator=(DirectoryTreeProxyModel &&other) = delete;

    explicit DirectoryTreeProxyModel(IsFileExcluded const & isFileExcluded, QObject *parent = nullptr);
    ~DirectoryTreeProxyModel() override;

    void setShowExcluded(bool showExcluded);

protected:
    bool filterAcceptsRow(int sourceRow, QModelIndex const &sourceParent) const override;

private:
    IsFileExcluded const isFileExcluded_;
    bool showExcluded_ = false;
};
}
