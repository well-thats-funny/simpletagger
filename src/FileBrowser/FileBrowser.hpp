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

class Ui_FileBrowser;

class DirectoryStatsManager;
class DirectoryStats;
class FileEditor;
class FileTags;
class FileTagsManager;

namespace FileBrowser {
class ProjectDirectoryListModel;
class DirectoryTreeModel;
class DirectoryTreeProxyModel;

class FileBrowser: public QWidget {
    Q_OBJECT

    FileBrowser(FileBrowser const &other) = delete;
    FileBrowser(FileBrowser &&other) = delete;
    FileBrowser& operator=(FileBrowser const &other) = delete;
    FileBrowser& operator=(FileBrowser &&other) = delete;

    using IsFileExcluded = std::function<bool(QString const &)>;
    using IsOtherLibraryOrVersion = std::function<bool(FileTags const &)>;

    FileBrowser(
            FileTagsManager &fileTagsManager,
            DirectoryStatsManager &directoryStatsManager,
            FileEditor &fileEditor,
            IsFileExcluded const &isFileExcluded,
            IsOtherLibraryOrVersion const &isOtherLibraryOrVersion,
            Qt::WindowFlags flags = Qt::WindowFlags()
    );
    std::expected<void, QString> init();

public:
    static std::expected<std::unique_ptr<FileBrowser>, QString>
    create(
            FileTagsManager &fileTagsManager,
            DirectoryStatsManager &directoryStatsManager,
            FileEditor &fileEditor,
            IsFileExcluded const &isFileExcluded,
            IsOtherLibraryOrVersion const &isOtherLibraryOrVersion,
            Qt::WindowFlags flags = Qt::WindowFlags()
    );
    ~FileBrowser() override;

    void setProjectRootPath(QString const &projectRootPath);

    void setDirectories(QStringList const &directories);

    [[nodiscard]] std::expected<void, QString> selectDirectoryInProjectList(QString const &directory);
    [[nodiscard]] std::expected<void, QString> selectFileInTree(QString const &file);

    [[nodiscard]] QByteArray saveUiState() const;
    void restoreUiState(QByteArray const &value);

signals:
    void directoryAdded(QString const &directory);
    void directoryRemoved(QString const &directory);
    void directoryLoaded(QString const &directory);
    void fileSelected(QString const &path);
    void requestTagsCopy(QString const &sourceFile, QString const &targetFile);
    void refresh();

private:
    void openDirectory(QString const &directory);
    void closeDirectory();
    void refreshDirectoryLabel();

    void fileSelectedHandle(QString const &path);

    bool isFileExcludedAbsPath(QString const &file);

    std::unique_ptr<Ui_FileBrowser> ui;
    FileTagsManager &fileTagsManager_;
    DirectoryStatsManager &directoryStatsManager_;
    FileEditor &fileEditor_;
    IsFileExcluded isFileExcluded_;

    std::unique_ptr<ProjectDirectoryListModel> projectDirectoryListModel;
    std::unique_ptr<DirectoryTreeModel> directoryTreeModel;
    std::unique_ptr<DirectoryTreeProxyModel> directoryTreeProxyModel;

    QString projectRootPath_;
    QString currentDirectory_;

    QMetaObject::Connection currentChangedConnection;
};
}