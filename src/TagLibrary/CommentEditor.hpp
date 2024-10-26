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

class Ui_CommentEditor;

namespace TagLibrary {
class CommentEditor: public QDialog {
    Q_OBJECT

    CommentEditor(CommentEditor const &other) = delete;
    CommentEditor(CommentEditor &&other) = delete;
    CommentEditor& operator=(CommentEditor const &other) = delete;
    CommentEditor& operator=(CommentEditor &&other) = delete;

    CommentEditor(QWidget *parent);

public:
    static std::expected<std::unique_ptr<CommentEditor>, QString> create(QWidget *parent);
    ~CommentEditor() override;

    void setHeaderLabel(QString const &text);

    [[nodiscard]] QString text() const;
    void setText(QString const &text);

private:
    std::unique_ptr<Ui_CommentEditor> ui;
};
}
