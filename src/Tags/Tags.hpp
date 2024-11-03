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

class Ui_Tags;

class FileEditor;

namespace Tags {
class TagsAssignedListModel;

class Tags: public QWidget {
    Q_OBJECT

    Tags(Tags const &other) = delete;
    Tags(Tags &&other) = delete;
    Tags& operator=(Tags const &other) = delete;
    Tags& operator=(Tags &&other) = delete;

    Tags();
    [[nodiscard]] std::expected<void, QString> init(FileEditor &fileEditor);

public:
    static std::expected<std::unique_ptr<Tags>, QString> create(FileEditor &fileEditor);
    ~Tags() override;

    void setHighlightedTags(QStringList const &tags);

signals:
    void tagsSelected(QStringList const &tags);

private:
    [[nodiscard]] QString indexToTag(QModelIndex const &index) const;
    [[nodiscard]] QStringList indexesToTags(QModelIndexList const &indexes) const;

    std::unique_ptr<Ui_Tags> ui;

    std::unique_ptr<TagsAssignedListModel> assignedTagsListModel;
};
}
