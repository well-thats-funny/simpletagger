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
class ProjectDirectoryListModel: public QAbstractItemModel {
public:
    ProjectDirectoryListModel(ProjectDirectoryListModel const &other) = delete;
    ProjectDirectoryListModel(ProjectDirectoryListModel &&other) = delete;
    ProjectDirectoryListModel& operator=(ProjectDirectoryListModel const &other) = delete;
    ProjectDirectoryListModel& operator=(ProjectDirectoryListModel &&other) = delete;

    ProjectDirectoryListModel();
    ~ProjectDirectoryListModel() override;

    void setDirectories(QStringList const &directories);
    void addDirectory(QString const &directory);
    void removeDirectory(int row);
    [[nodiscard]] QStringList const &directories() const;
    [[nodiscard]] QString directory(int row) const;

    [[nodiscard]] QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] QModelIndex parent(const QModelIndex &child) const override;
    [[nodiscard]] int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

private:
    QStringList directories_;
};
}
