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
#include "FileEditor.hpp"

#include "Constants.hpp"

FileEditor::FileEditor(FileTagsManager &fileTagsManager): fileTagsManager(fileTagsManager) {}

FileEditor::~FileEditor() = default;

void FileEditor::setFile(QString const &file) {
    ZoneScoped;

    if (!file.isNull())
        fileTags = std::ref(fileTagsManager.forFile(file));

    emit tagsChanged();
    emit imageRegionChanged();
}

void FileEditor::resetFile() {
    ZoneScoped;

    fileTags.reset();
    emit tagsChanged();
    emit imageRegionChanged();
}

std::optional<bool> FileEditor::isTagged(QString const &tag) const {
    ZoneScoped;

    return assignedTags().transform([&](auto const &tags){ return tags.contains(tag); });
}

void FileEditor::setTagged(QStringList const &tags, bool const value) {
    ZoneScoped;

    if (fileTags.transform([&](auto &f){ return f.get().setTags(tags, value); }).value_or(false))
        emit tagsChanged();
}

void FileEditor::setTagsState(std::unordered_map<QString, bool> const &state) {
    ZoneScoped;

    if (fileTags.transform([&](auto &f){ return f.get().setTagsState(state); }).value_or(false))
        emit tagsChanged();
}

void FileEditor::clearTags() {
    ZoneScoped;

    if (fileTags.transform([&](auto &f){ return f.get().clearTags(); }).value_or(false))
        emit tagsChanged();
}

std::optional<QStringList> FileEditor::assignedTags() const {
    ZoneScoped;

    return fileTags.transform([](auto const &v){ return v.get().assignedTags(); });
}

bool FileEditor::moveAssignedTag(int const sourcePositon, int const targetPosition) {
    return fileTags.transform([&](auto &v){ return v.get().moveAssignedTag(sourcePositon, targetPosition); }).value_or(false);
}

void FileEditor::setImageRegion(std::optional<QRect> const &rect) {
    ZoneScoped;

    if (fileTags.transform([&](auto &v){ return v.get().setImageRegion(rect); }).value_or(false))
        emit imageRegionChanged();
}

std::optional<QRect> FileEditor::imageRegion() const {
    ZoneScoped;

    return fileTags.and_then([&](auto const &v){ return v.get().imageRegion(); });
}

void FileEditor::setCompleteFlag(bool const complete) {
    ZoneScoped;
    gsl_Expects(fileTags);
    fileTags->get().setCompleteFlag(complete);
}

bool FileEditor::isCompleteFlag() const {
    ZoneScoped;
    gsl_Expects(fileTags);
    return fileTags->get().isCompleteFlag();
}
