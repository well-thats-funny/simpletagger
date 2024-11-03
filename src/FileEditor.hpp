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

#include "FileTagsManager.hpp"

class FileEditor: public QObject {
    Q_OBJECT

public:
    FileEditor(FileEditor const &other) = delete;
    FileEditor(FileEditor &&other) = delete;
    FileEditor &operator=(FileEditor const &other) = delete;
    FileEditor &operator=(FileEditor &&other) = delete;

    explicit FileEditor(FileTagsManager &fileTagsManager);
    ~FileEditor();

    void setFile(QString const &file);
    void resetFile();

    [[nodiscard]] std::optional<bool> isTagged(QString const &tag) const;
    void setTagged(QStringList const &tag, bool const value);
    void setTagsState(std::unordered_map<QString, bool> const &state);
    void clearTags();

    [[nodiscard]] std::optional<QStringList> assignedTags() const;
    [[nodiscard]] bool moveAssignedTag(int sourcePositon, int targetPosition);

    void setImageRegion(std::optional<QRect> const &rect);
    [[nodiscard]] std::optional<QRect> imageRegion() const;

    void setCompleteFlag(bool complete);
    [[nodiscard]] bool isCompleteFlag() const;

signals:
    void tagsChanged();
    void imageRegionChanged();

private:
    FileTagsManager &fileTagsManager;
    std::optional<std::reference_wrapper<FileTags>> fileTags;
};
