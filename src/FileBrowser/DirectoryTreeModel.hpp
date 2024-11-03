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
#include <QFileSystemModel>
#include <QStyle>

class DirectoryStatsManager;
class FileTags;
class FileTagsManager;

namespace FileBrowser {
class CustomFileIconProvider;

class DirectoryTreeModel: public QFileSystemModel {
    Q_OBJECT

    using IsFileExcluded = std::function<bool(QString const &)>;
    using IsOtherLibraryOrVersion = std::function<bool(FileTags const &)>;

public:
    DirectoryTreeModel(
            QStyle *style,
            FileTagsManager &fileTagsManager,
            DirectoryStatsManager &directoryStatsManager,
            IsFileExcluded const & isFileExcluded,
            IsOtherLibraryOrVersion const &isOtherLibraryOrVersion
    );
    ~DirectoryTreeModel() override;

    int columnCount(const QModelIndex &parent) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool event(QEvent *event) override;

    void refreshExcludedState(QString const &file);

private:
    QImage getImage(QString const &path) const;

    std::unique_ptr<CustomFileIconProvider> iconProvider;

    QStyle *style = nullptr;
    FileTagsManager &fileTagsManager_;
    DirectoryStatsManager &directoryStatsManager_;
    IsFileExcluded isFileExcluded_;
    IsOtherLibraryOrVersion isOtherLibraryOrVersion_;
    QPixmap directoryIcon;
    QString cacheDir;
    mutable std::unordered_map<QString, QImage> imageCache;
};
}
