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
#include "CommentEditor.hpp"

#include "ui_CommentEditor.h"

namespace TagLibrary {
CommentEditor::CommentEditor(QWidget *const parent): QDialog(parent) {}

std::expected<std::unique_ptr<CommentEditor>, QString> CommentEditor::create(QWidget *const parent) {
    ZoneScoped;

    auto self = std::unique_ptr<CommentEditor>(new CommentEditor(parent));

    self->ui = std::make_unique<Ui_CommentEditor>();
    self->ui->setupUi(&*self);
    connect(self->ui->editor, &QPlainTextEdit::textChanged, &*self, [t=&*self]{
        ZoneScoped;
        t->ui->labelPreview->setText(t->ui->editor->toPlainText());
    });
    return self;
}

CommentEditor::~CommentEditor() = default;

void CommentEditor::setHeaderLabel(QString const &text) {
    ZoneScoped;
    ui->labelHeader->setText(text);
}

QString CommentEditor::text() const {
    ZoneScoped;
    return ui->editor->toPlainText();
}

void CommentEditor::setText(QString const &text) {
    ZoneScoped;
    ui->editor->setPlainText(text);
}
}
