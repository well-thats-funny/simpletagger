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

class FileTags {
public:
    FileTags(FileTags const &other) = delete;
    FileTags(FileTags &&other) = delete;
    FileTags &operator=(FileTags const &other) = delete;
    FileTags &operator=(FileTags &&other) = delete;

    FileTags(FileTagsManager &manager, QString const &tagsFilePath, bool backupOnSave);
    ~FileTags();

    [[nodiscard]] QStringList assignedTags() const;
    [[nodiscard]] bool setTags(QStringList const &tag, bool value);
    [[nodiscard]] bool setTagsState(std::unordered_map<QString, bool> const &state);
    [[nodiscard]] std::expected<void, QString> overwriteAssignedTags(QStringList const &activeTags);
    [[nodiscard]] bool moveAssignedTag(int sourcePositon, int targetPosition);
    [[nodiscard]] bool clearTags();

private:
    bool setTag_(QString const &tag, bool value);

public:
    [[nodiscard]] std::optional<QRect> imageRegion() const;
    bool setImageRegion(std::optional<QRect> const &rect);

    [[nodiscard]] bool isCompleteFlag() const;
    void setCompleteFlag(bool value);

private:
    void load();
    void save(bool backup);

    FileTagsManager &manager_;
    QString tagsFilePath_;
    bool backupOnSave_ = false;
    QStringList assignedTags_;
    std::optional<QRect> imageRegion_;
    bool completeFlag_ = false;
};

class FileTagsManager: public QObject {
    Q_OBJECT

    friend class DirectoryTagsStats;

public:
    FileTagsManager(FileTagsManager const &other) = delete;
    FileTagsManager(FileTagsManager &&other) = delete;
    FileTagsManager &operator=(FileTagsManager const &other) = delete;
    FileTagsManager &operator=(FileTagsManager &&other) = delete;

    explicit FileTagsManager(bool backupOnSave);
    ~FileTagsManager();

    void setBackupOnSave(bool value);

    FileTags &forFile(QString const &path);

    int cachedFiles() const;

signals:
    void tagsSaved(std::optional<int> const &backupCount);

private:
    bool backupOnSave_ = false;

    mutable QMutex mutex_;
    std::unordered_map<QString, std::unique_ptr<FileTags>> fileTags_;
};
