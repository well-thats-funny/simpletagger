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
#include "Tags.hpp"

#include "TagsAssignedListModel.hpp"

#include "../CustomItemDataRole.hpp"
#include "../FileEditor.hpp"

#include "ui_Tags.h"

namespace Tags {
Tags::Tags(): ui(std::make_unique<Ui_Tags>()) {}

std::expected<void, QString> Tags::init(FileEditor &fileEditor) {
    ZoneScoped;

    ui->setupUi(this);

    ui->buttonAssignedTagsAdd->setDefaultAction(ui->actionAssignedTagsAdd);
    ui->buttonAssignedTagsDelete->setDefaultAction(ui->actionAssignedTagsDelete);
    ui->buttonAssignedTagsClear->setDefaultAction(ui->actionAssignedTagsClear);

    assignedTagsListModel = std::make_unique<TagsAssignedListModel>(fileEditor);
    ui->listAssignedTags->setModel(&*assignedTagsListModel);

    connect(ui->listAssignedTags->selectionModel(), &QItemSelectionModel::currentChanged, this, [this](QModelIndex const &current){
        ZoneScoped;
        emit tagsSelected(QStringList{indexToTag(current)});
    });

    connect(ui->listAssignedTags, &QTreeView::customContextMenuRequested, this, [this](QPoint const &pos){
        ZoneScoped;

        if (!ui->listAssignedTags->selectionModel()->selectedIndexes().empty()) {
            QMenu menu;
            menu.addAction(ui->actionAssignedTagsDelete);
            menu.exec(ui->listAssignedTags->mapToGlobal(pos));
        }
    });

    connect(&fileEditor, &FileEditor::tagsChanged, this, [&]{
        ZoneScoped;
        if (auto size = fileEditor.assignedTags()->size(); size == 0)
            ui->labelAssignedTags->setText(tr("Assigned tags:"));
        else
            ui->labelAssignedTags->setText(tr("Assigned tags (%1):").arg(size));
    });

    connect(ui->actionAssignedTagsDelete, &QAction::triggered, this, [this, &fileEditor]{
        ZoneScoped;
        fileEditor.setTagged(indexesToTags(ui->listAssignedTags->selectionModel()->selectedIndexes()), false);
    });

    connect(ui->actionAssignedTagsClear, &QAction::triggered, this, [this, &fileEditor]{
        ZoneScoped;
        if (QMessageBox::warning(
                this,
                tr("Clear all tags?"),
                tr("All currently assigned tags will be cleared! Are you sure?"),
                QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No
        ) == QMessageBox::StandardButton::Yes) {
            ZoneScoped;
            fileEditor.clearTags();
        }
    });

    return {};
}

std::expected<std::unique_ptr<Tags>, QString> Tags::create(FileEditor &fileEditor) {
    ZoneScoped;
    std::unique_ptr<Tags> self(new Tags);
    if (auto result = self->init(fileEditor); !result)
        return std::unexpected(result.error());
    return self;
}

Tags::~Tags() = default;

void Tags::setHighlightedTags(QStringList const &tags) {
    ZoneScoped;
    auto indexes = assignedTagsListModel->setHighlightedTags(tags);
    if (!indexes.isEmpty())
        ui->listAssignedTags->scrollTo(indexes.front());
}

void Tags::setKnownTags(QStringList const &tags) {
    ZoneScoped;
    assignedTagsListModel->setKnownTags(tags);
}

QString Tags::indexToTag(QModelIndex const &index) const {
    ZoneScoped;
    auto v = assignedTagsListModel->data(index, std::to_underlying(CustomItemDataRole::TagRole));
    assert(v.isValid());
    return v.toString();
}

QStringList Tags::indexesToTags(QModelIndexList const &indexes) const {
    ZoneScoped;
    return indexes
        | std::views::transform([&](auto const &index){ return indexToTag(index); })
        | std::ranges::to<QStringList>();
}
}
