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

class FileTagsManager;
class Project;

namespace TagLibrary {
class Library;
}

class FileTags {
    FileTags(
            FileTagsManager &manager,
            QString const &imageFilePath,
            QString const &tagsFilePath,
            bool backupOnSave
    );

    [[nodiscard]] std::expected<void, QString> init();

public:
    FileTags(FileTags const &other) = delete;
    FileTags(FileTags &&other) = delete;
    FileTags &operator=(FileTags const &other) = delete;
    FileTags &operator=(FileTags &&other) = delete;

    [[nodiscard]] static std::expected<std::unique_ptr<FileTags>, QString>
    create(
            FileTagsManager &manager,
            QString const &imageFilePath,
            QString const &tagsFilePath,
            bool backupOnSave
    );
    ~FileTags();

    [[nodiscard]] QStringList assignedTags() const;
    [[nodiscard]] bool setTags(QStringList const &tag, bool value);
    [[nodiscard]] bool setTagsState(std::unordered_map<QString, bool> const &state);
    [[nodiscard]] bool overwriteAssignedTags(QStringList const &activeTags);
    [[nodiscard]] bool moveAssignedTag(int sourcePositon, int targetPosition);
    [[nodiscard]] bool clearTags();

private:
    bool setTag_(QString const &tag, bool value);

public:
    [[nodiscard]] std::optional<QRect> imageRegion() const;
    [[nodiscard]] bool setImageRegion(std::optional<QRect> const &rect);

    [[nodiscard]] bool isCompleteFlag() const;
    [[nodiscard]] bool setCompleteFlag(bool value);

    [[nodiscard]] std::optional<QUuid> tagLibraryUuid() const;
    [[nodiscard]] std::optional<int> tagLibraryVersion() const;
    [[nodiscard]] std::optional<QUuid> tagLibraryVersionUuid() const;

    [[nodiscard]] bool isModified() const;

private:
    [[nodiscard]] std::expected<void, QString> load();

public:
    [[nodiscard]] std::expected<void, QString> save(bool forceSave = false, bool forceBackup = false);

private:
    void setModified_(bool modified);

    FileTagsManager &manager_;
    QString imageFilePath_;
    QString tagsFilePath_;
    bool backupOnSave_ = false;
    bool modified_ = false;

    QStringList assignedTags_;
    std::optional<QRect> imageRegion_;
    bool completeFlag_ = false;

    std::optional<QUuid> tagLibraryUuid_;
    std::optional<int> tagLibraryVersion_ = -1;
    std::optional<QUuid> tagLibraryVersionUuid_;
};

class FileTagsManager: public QObject {
    Q_OBJECT

    friend class FileTags;

public:
    FileTagsManager(FileTagsManager const &other) = delete;
    FileTagsManager(FileTagsManager &&other) = delete;
    FileTagsManager &operator=(FileTagsManager const &other) = delete;
    FileTagsManager &operator=(FileTagsManager &&other) = delete;

    explicit FileTagsManager(bool backupOnSave);
    ~FileTagsManager();

    void setTagLibrary(TagLibrary::Library *library);

    void setBackupOnSave(bool value);

    [[nodiscard]] std::expected<std::reference_wrapper<FileTags>, QString> forFile(QString const &path);

    int cachedFiles() const;

signals:
    void tagsSaved(std::optional<int> const &backupCount);
    void modifiedStateChanged(QString const &imageFileName, bool modified);

private:
    TagLibrary::Library *tagLibrary_ = nullptr;

    bool backupOnSave_ = false;

    mutable QMutex mutex_;
    std::unordered_map<QString, std::unique_ptr<FileTags>> fileTags_;
};
