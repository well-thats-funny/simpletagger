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
#include "FileTagsManager.hpp"
#include "Project.hpp"

FileEditor::FileEditor(FileTagsManager &fileTagsManager): fileTagsManager(fileTagsManager) {
    connect(&fileTagsManager, &FileTagsManager::modifiedStateChanged, this, [this](QString const &imageFilePath, bool const modified){
        if (imageFilePath == currentFile_)
            emit modifiedStateChanged(modified);
    });
}

FileEditor::~FileEditor() = default;

void FileEditor::setBackupOnEverySave(bool const enable) {
    backupOnEverySave_ = enable;
}

bool FileEditor::isBackupOnEverySave() const {
    return backupOnEverySave_;
}

void FileEditor::setProject(Project &project) {
    ZoneScoped;

    if (&project != project_) {
        resetProject();

        project_ = &project;
    }
}

void FileEditor::resetProject() {
    ZoneScoped;

    if (project_) {
        project_ = nullptr;
    }
}

std::expected<void, ErrorOrCancel> FileEditor::setFile(QString const &file) {
    ZoneScoped;
    gsl_Expects(!file.isEmpty());

    if (file != currentFile_) {
        if (auto result = resetFile(); !result)
            return result;

        currentFile_ = file;

        if (QFileInfo(file).isDir()) {
            emit modifiedStateChanged(false);
        } else {
            if (auto result = fileTagsManager.forFile(file); !result)
                return std::unexpected(result.error());
            else
                fileTags = *result;

            emit modifiedStateChanged(fileTags->get().isModified());
        }

        emit tagsChanged();
        emit imageRegionChanged();
    }

    gsl_Ensures(!currentFile_.isEmpty());
    return {};
}

std::expected<void, ErrorOrCancel> FileEditor::resetFile() {
    ZoneScoped;

    if (!currentFile_.isEmpty()) {
        if (fileTags && fileTags->get().isModified()) {
            switch (QMessageBox::question(
                    qApp->activeWindow(), tr("Unsaved changes"), tr("Save changes in tags data?"),
                    QMessageBox::StandardButtons(
                            QMessageBox::StandardButton::Yes
                            | QMessageBox::StandardButton::No
                            | QMessageBox::StandardButton::Cancel
                    )
            )) {
                case QMessageBox::StandardButton::Yes:
                    if (auto result = fileTags->get().save(); !result)
                        return std::unexpected(result.error());
                    break;
                case QMessageBox::StandardButton::No:
                    if (auto result = fileTags->get().rollbackChanges(); !result)
                        return std::unexpected(result.error());
                    break;
                case QMessageBox::StandardButton::Cancel:
                    return std::unexpected(CancelOperation{});
                default:
                    gsl_Expects(false); // unreachable
            }
        }

        currentFile_.clear();
        fileTags.reset();
        emit tagsChanged();
        emit imageRegionChanged();
        emit modifiedStateChanged(false);
    }

    gsl_Ensures(currentFile_.isEmpty());
    gsl_Ensures(!fileTags);
    return {};
}

std::optional<bool> FileEditor::isTagged(QString const &tag) const {
    ZoneScoped;
    // TODO: gsl_Expects(fileTags); and direct access below
    return assignedTags().transform([&](auto const &tags){ return tags.contains(tag); });
}

void FileEditor::setTagged(QStringList const &tags, bool const value) {
    ZoneScoped;
    // TODO: gsl_Expects(fileTags); and direct access below
    if (fileTags && fileTags->get().setTags(tags, value))
        emit tagsChanged();
}

void FileEditor::setTagsState(std::unordered_map<QString, bool> const &state) {
    ZoneScoped;
    // TODO: gsl_Expects(fileTags); and direct access below
    if (fileTags && fileTags->get().setTagsState(state))
        emit tagsChanged();
}

void FileEditor::clearTags() {
    ZoneScoped;
    // TODO: gsl_Expects(fileTags); and direct access below
    if (fileTags.transform([&](auto &f){ return f.get().clearTags(); }).value_or(false))
        emit tagsChanged();
}

std::optional<QStringList> FileEditor::assignedTags() const {
    ZoneScoped;
    // TODO: gsl_Expects(fileTags); and direct access below
    return fileTags.transform([](auto const &v){ return v.get().assignedTags(); });
}

std::expected<bool, QString> FileEditor::moveAssignedTag(int const sourcePositon, int const targetPosition) {
    ZoneScoped;
    // TODO: gsl_Expects(fileTags); and direct access below
    return fileTags.transform([&](auto &v){ return v.get().moveAssignedTag(sourcePositon, targetPosition); }).value_or(false);
}

void FileEditor::setImageRegion(std::optional<QRect> const &rect) {
    ZoneScoped;
    // TODO: gsl_Expects(fileTags); and direct access below
    if (fileTags.transform([&](auto &v){ return v.get().setImageRegion(rect); }).value_or(false))
        emit imageRegionChanged();
}

std::optional<QRect> FileEditor::imageRegion() const {
    ZoneScoped;
    // TODO: gsl_Expects(fileTags); and direct access below
    return fileTags.and_then([&](auto const &v){ return v.get().imageRegion(); });
}

bool FileEditor::setCompleteFlag(bool const complete) {
    ZoneScoped;
    gsl_Expects(fileTags);
    return fileTags->get().setCompleteFlag(complete);
}

bool FileEditor::isCompleteFlag() const {
    ZoneScoped;
    gsl_Expects(fileTags);
    return fileTags->get().isCompleteFlag();
}

std::expected<void, QString> FileEditor::setFileExcluded(bool const excluded) {
    ZoneScoped;
    gsl_Expects(project_);
    gsl_Expects(QFileInfo(currentFile_).isAbsolute());

    auto relative = QDir(project_->rootDir()).relativeFilePath(currentFile_);
    project_->setExcludedFile(relative, excluded);
    if (auto result = project_->save(backupOnEverySave_); !result) {
        return std::unexpected(result.error());
    } else {
        emit projectSaved(*result);
        return {};
    }

}

bool FileEditor::isFileExcluded() const {
    ZoneScoped;
    gsl_Expects(project_);
    gsl_Expects(!currentFile_.isEmpty());
    gsl_Expects(QFileInfo(currentFile_).isAbsolute());

    auto relative = QDir(project_->rootDir()).relativeFilePath(currentFile_);
    return project_->isExcludedFile(relative);
}

std::optional<QUuid> FileEditor::imageTagLibraryUuid() const {
    ZoneScoped;
    gsl_Expects(fileTags);
    return fileTags->get().tagLibraryUuid();
}

std::optional<int> FileEditor::imageTagLibraryVersion() const {
    ZoneScoped;
    gsl_Expects(fileTags);
    return fileTags->get().tagLibraryVersion();
}

std::optional<QUuid> FileEditor::imageTagLibraryVersionUuid() const {
    ZoneScoped;
    gsl_Expects(fileTags);
    return fileTags->get().tagLibraryVersionUuid();
}

std::expected<void, QString> FileEditor::save() const {
    ZoneScoped;
    gsl_Expects(fileTags);
    return fileTags->get().save();
}
