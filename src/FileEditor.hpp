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

class FileTags;
class FileTagsManager;
class Project;

class FileEditor: public QObject {
    Q_OBJECT

public:
    FileEditor(FileEditor const &other) = delete;
    FileEditor(FileEditor &&other) = delete;
    FileEditor &operator=(FileEditor const &other) = delete;
    FileEditor &operator=(FileEditor &&other) = delete;

    explicit FileEditor(FileTagsManager &fileTagsManager);
    ~FileEditor();

    void setBackupOnEveryChange(bool enable);
    bool isBackupOnEveryChange() const;

    void setProject(Project &project);
    void resetProject();

    [[nodiscard]] std::expected<void, QString> setFile(QString const &file);
    void resetFile();

    [[nodiscard]] std::optional<bool> isTagged(QString const &tag) const;
    void setTagged(QStringList const &tag, bool const value);
    void setTagsState(std::unordered_map<QString, bool> const &state);
    void clearTags();

    [[nodiscard]] std::optional<QStringList> assignedTags() const;
    [[nodiscard]] std::expected<bool, QString> moveAssignedTag(int sourcePositon, int targetPosition);

    void setImageRegion(std::optional<QRect> const &rect);
    [[nodiscard]] std::optional<QRect> imageRegion() const;

    [[nodiscard]] std::expected<void, QString> setCompleteFlag(bool complete);
    [[nodiscard]] bool isCompleteFlag() const;

    [[nodiscard]] std::expected<void, QString> setFileExcluded(bool excluded);
    [[nodiscard]] bool isFileExcluded() const;

    [[nodiscard]] std::optional<QUuid> imageTagLibraryUuid() const;
    [[nodiscard]] std::optional<int> imageTagLibraryVersion() const;
    [[nodiscard]] std::optional<QUuid> imageTagLibraryVersionUuid() const;

signals:
    void projectSaved(std::optional<int> backupCount);

    void tagsChanged();
    void imageRegionChanged();

private:
    Project *project_ = nullptr;

    QString currentFile_;

    FileTagsManager &fileTagsManager;
    std::optional<std::reference_wrapper<FileTags>> fileTags;

    bool backupOnEveryChange_ = false;
};
