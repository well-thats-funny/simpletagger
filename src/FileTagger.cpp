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
#include "FileTagger.hpp"

#include "Constants.hpp"

FileTagger::FileTagger(FileTagsManager &fileTagsManager): fileTagsManager(fileTagsManager) {}

FileTagger::~FileTagger() = default;

void FileTagger::setFile(QString const &file) {
    ZoneScoped;

    if (!file.isNull())
        fileTags = std::ref(fileTagsManager.forFile(file));

    emit tagsChanged();
    emit imageRegionChanged();
}

void FileTagger::resetFile() {
    ZoneScoped;

    fileTags.reset();
    emit tagsChanged();
    emit imageRegionChanged();
}

std::optional<bool> FileTagger::isTagged(QString const &tag) const {
    ZoneScoped;

    return assignedTags().transform([&](auto const &tags){ return tags.contains(tag); });
}

void FileTagger::setTagged(QStringList const &tags, bool const value) {
    ZoneScoped;

    if (fileTags.transform([&](auto &f){ return f.get().setTags(tags, value); }).value_or(false))
        emit tagsChanged();
}

void FileTagger::setTagsState(std::unordered_map<QString, bool> const &state) {
    ZoneScoped;

    if (fileTags.transform([&](auto &f){ return f.get().setTagsState(state); }).value_or(false))
        emit tagsChanged();
}

void FileTagger::clearTags() {
    ZoneScoped;

    if (fileTags.transform([&](auto &f){ return f.get().clearTags(); }).value_or(false))
        emit tagsChanged();
}

std::optional<QStringList> FileTagger::assignedTags() const {
    ZoneScoped;

    return fileTags.transform([](auto const &v){ return v.get().assignedTags(); });
}

bool FileTagger::moveAssignedTag(int const sourcePositon, int const targetPosition) {
    return fileTags.transform([&](auto &v){ return v.get().moveAssignedTag(sourcePositon, targetPosition); }).value_or(false);
}

void FileTagger::setImageRegion(std::optional<QRect> const &rect) {
    ZoneScoped;

    if (fileTags.transform([&](auto &v){ return v.get().setImageRegion(rect); }).value_or(false))
        emit imageRegionChanged();
}

std::optional<QRect> FileTagger::imageRegion() const {
    ZoneScoped;

    return fileTags.and_then([&](auto const &v){ return v.get().imageRegion(); });
}
