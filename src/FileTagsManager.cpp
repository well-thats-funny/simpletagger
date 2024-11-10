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
#include "FileTagsManager.hpp"

#include "Constants.hpp"
#include "Project.hpp"
#include "Utility.hpp"

#include "TagLibrary/Library.hpp"

namespace {
    // TODO: FileBrowser.cpp and DirectoryStats.cpp has a similar list (just with wildcards added), merge?
    const QStringList IMAGE_FILE_SUFFIXES = {".jpg", ".png"};

    enum class Key {
        FORMAT_VERSION = 1,
        APP = 2,
        TAGS = 3,
        REGION = 4,
        COMPLETE_FLAG = 5,
        TAG_LIBRARY_UUID = 6,
        TAG_LIBRARY_VERSION = 7,
        TAG_LIBRARY_VERSION_UUID = 8
    };

    constexpr int valueFormatVersion = 1;
    constexpr QAnyStringView valueApp = "SIMPLETAGGER-CXX";

    struct LoadTagsFileResult {
        QCborMap map;
        bool warningsOccurred;
    };

    std::expected<LoadTagsFileResult, QString> loadTagsFile(QString const &fileName) {
        ZoneScoped;

        bool warnings = false;

        QFile file{fileName};
        if (!file.open(QIODevice::ReadOnly))
            return std::unexpected("Cannot open");

        QCborStreamReader reader(&file);
        auto value = QCborValue::fromCbor(reader);

        if (!value.isMap())
            return std::unexpected("Root element is not a map");

        auto map = value.toMap();

        auto itFormatVersion = map.find(std::to_underlying(Key::FORMAT_VERSION));
        if (itFormatVersion == map.end() || !itFormatVersion->isInteger())
            return std::unexpected("No format version information or not an integer");

        if (itFormatVersion->toInteger() != valueFormatVersion)
            return std::unexpected(QString("Unrecognized format version: %1").arg(itFormatVersion->toInteger()));

        auto itApp = map.find(std::to_underlying(Key::APP));
        if (itApp == map.end() || !itApp->isString()) {
            warnings = true;
            qWarning() << "File" << fileName << "has no application key or value is not a string";
        } else {
            if (itApp->toString() != valueApp) {
                warnings = true;
                qWarning() << "File" << fileName << "was created by an unknown app:" << itApp->toString();
            }
        }

        return LoadTagsFileResult{std::move(map), warnings};
    }
}

FileTags::FileTags(
        FileTagsManager &manager,
        QString const &imageFilePath,
        QString const &tagsFilePath,
        bool const backupOnSave
):
    manager_(manager), imageFilePath_(imageFilePath), tagsFilePath_(tagsFilePath), backupOnSave_(backupOnSave) {}

std::expected<void, QString> FileTags::init() {
    ZoneScoped;
    return load();
}

std::expected<std::unique_ptr<FileTags>, QString>
FileTags::create(FileTagsManager &manager, QString const &imageFilePath, QString const &tagsFilePath, bool const backupOnSave) {
    std::unique_ptr<FileTags> self{new FileTags(manager, imageFilePath, tagsFilePath, backupOnSave)};
    if (auto result = self->init(); !result)
        return std::unexpected(result.error());
    else
        return self;
}

FileTags::~FileTags() = default;

QStringList FileTags::assignedTags() const {
    ZoneScoped;

    return assignedTags_;
}

bool FileTags::setTags(QStringList const &tags, bool const value) {
    ZoneScoped;

    bool changed = false;

    for (auto const &tag: tags)
        if (setTag_(tag, value))
            changed = true;

    if (changed)
        setModified_(true);

    return changed;
}

bool FileTags::setTagsState(std::unordered_map<QString, bool> const &state) {
    ZoneScoped;

    bool changed = false;

    for (auto const &[tag, value]: state)
        if (setTag_(tag, value))
            changed = true;

    if (changed)
        setModified_(true);

    return changed;
}

bool FileTags::overwriteAssignedTags(QStringList const &activeTags) {
    ZoneScoped;

    if (assignedTags_ == activeTags) {
        return false;
    } else {
        assignedTags_ = activeTags;
        setModified_(true);
        return true;
    }
}

bool FileTags::moveAssignedTag(int const sourcePositon, int const targetPosition) {
    ZoneScoped;
    gsl_Expects(sourcePositon >= 0);
    gsl_Expects(sourcePositon < assignedTags_.size());
    gsl_Expects(targetPosition >= 0);
    gsl_Expects(targetPosition < assignedTags_.size());

    if (sourcePositon == targetPosition) {
        return false;
    } else {
        assignedTags_.move(sourcePositon, targetPosition);
        setModified_(true);
        return true;
    }
}

bool FileTags::clearTags() {
    ZoneScoped;

    if (assignedTags_.empty()) {
        return false;
    } else {
        assignedTags_.clear();
        setModified_(true);
        return true;
    }
}

bool FileTags::setTag_(const QString &tag, bool value) {
    ZoneScoped;

    if (value != assignedTags_.contains(tag)) {
        if (value)
            assignedTags_.append(tag);
        else
            assignedTags_.removeAll(tag);

        return true;
    }

    return false;
}

std::optional<QRect> FileTags::imageRegion() const {
    ZoneScoped;

    return imageRegion_;
}

bool FileTags::setImageRegion(const std::optional<QRect> &rect) {
    ZoneScoped;

    if (rect == imageRegion_) {
        return false;
    } else {
        imageRegion_ = rect;
        setModified_(true);
        return true;
    }
}

bool FileTags::isCompleteFlag() const {
    return completeFlag_;
}

bool FileTags::setCompleteFlag(bool const value) {
    ZoneScoped;

    if (value == completeFlag_) {
        return false;
    } else {
        completeFlag_ = value;
        return true;
    }
}

std::optional<QUuid> FileTags::tagLibraryUuid() const {
    return tagLibraryUuid_;
}

std::optional<int> FileTags::tagLibraryVersion() const {
    return tagLibraryVersion_;
}

std::optional<QUuid> FileTags::tagLibraryVersionUuid() const {
    return tagLibraryVersionUuid_;
}

bool FileTags::isModified() const {
    return modified_;
}

std::expected<void, QString> FileTags::rollbackChanges() {
    return load();
}

std::expected<void, QString> FileTags::load() {
    ZoneScoped;

    assignedTags_.clear();
    imageRegion_.reset();
    completeFlag_ = false;
    tagLibraryUuid_ = std::nullopt;
    tagLibraryVersion_ = std::nullopt;
    tagLibraryVersionUuid_ = std::nullopt;

    setModified_(false);

    if (QFileInfo{tagsFilePath_}.exists()) {
        qDebug() << "Loading tags from" << tagsFilePath_;

        auto res = loadTagsFile(tagsFilePath_);
        if (!res)
            return std::unexpected(res.error());

        if (auto itTags = res->map.find(std::to_underlying(Key::TAGS)); itTags != res->map.end()) {
            if (!itTags->isArray()) {
                return std::unexpected(QObject::tr("Tags value in %1 is not an array").arg(tagsFilePath_));
            } else {
                auto array = itTags->toArray();
                for(auto const &tag: array) {
                    if (!tag.isString())
                        return std::unexpected(QObject::tr("Tag %1 is not a string, but ").arg(cborTypeToString(tag.type())));
                    else
                        assignedTags_.append(tag.toString());
                }
            }
        }

        if (auto region = res->map.take(std::to_underlying(Key::REGION)); !region.isUndefined()) {
            if (!region.isArray()
                || region.toArray().size() != 4
                || !std::ranges::all_of(region.toArray(), [](auto const &v){ return v.isInteger(); })) {
                return std::unexpected(QObject::tr("Region value in %1 is not a 4-element integer array").arg(tagsFilePath_));
            } else {
                auto array = region.toArray();
                auto left = array.at(0).toInteger();
                auto top = array.at(1).toInteger();
                auto right = array.at(2).toInteger();
                auto bottom = array.at(3).toInteger();
                imageRegion_ = QRect(left, top, right - left + 1, bottom - top + 1);
            }
        }

        if (auto completeFlag = res->map.take(std::to_underlying(Key::COMPLETE_FLAG)); !completeFlag.isUndefined()) {
            if (!completeFlag.isBool())
                return std::unexpected(QObject::tr("Complete flag in %1 is not a bool, but %2").arg(tagsFilePath_).arg(cborTypeToString(completeFlag.type())));
            else
                completeFlag_ = completeFlag.toBool();
        }

        if (auto libraryUuid = res->map.take(std::to_underlying(Key::TAG_LIBRARY_UUID)); !libraryUuid.isUndefined()) {
            if (!libraryUuid.isByteArray())
                return std::unexpected(QObject::tr("Library UUID is not a byte array but %1").arg(
                        cborTypeToString(libraryUuid.type())));
            tagLibraryUuid_ = QUuid::fromRfc4122(libraryUuid.toByteArray());
        }

        if (auto libraryVersion = res->map.take(std::to_underlying(Key::TAG_LIBRARY_VERSION)); !libraryVersion.isUndefined()) {
            if (!libraryVersion.isInteger())
                return std::unexpected(QObject::tr("Library version is not an integer but %1").arg(
                        cborTypeToString(libraryVersion.type())));
            tagLibraryVersion_ = libraryVersion.toInteger();
        }

        if (auto libraryVersionUuid = res->map.take(std::to_underlying(Key::TAG_LIBRARY_VERSION_UUID)); !libraryVersionUuid.isUndefined()) {
            if (!libraryVersionUuid.isByteArray())
                return std::unexpected(QObject::tr("Library version UUID is not a byte array but %1").arg(
                        cborTypeToString(libraryVersionUuid.type())));
            tagLibraryVersionUuid_ = QUuid::fromRfc4122(libraryVersionUuid.toByteArray());
        }

        qDebug() << "Loading tags from" << tagsFilePath_<< ": done";
    }

    return {};
}

std::expected<void, QString> FileTags::save(bool const forceSave, bool const forceBackup) {
    ZoneScoped;
    gsl_Expects(manager_.tagLibrary_);
    tagLibraryUuid_ = manager_.tagLibrary_->getUuid();
    tagLibraryVersion_ = manager_.tagLibrary_->getVersion();
    tagLibraryVersionUuid_ = manager_.tagLibrary_->getVersionUuid();

    if (!forceSave && !modified_)
        return {};

    bool backup = forceBackup || backupOnSave_;

    if (!backup) {
        // First, open the original file, to see whether it can be opened without warnings. In case there were warnings,
        // we first backup the original file
        QFileInfo tagsFileInfo{tagsFilePath_};

        if (tagsFileInfo.exists()) {
            auto res = loadTagsFile(tagsFilePath_);
            backup = backup || !res.has_value() || res->warningsOccurred;
        }
    }

    std::optional<int> backupCount;

    if (backup)
        backupCount = backupFile(tagsFilePath_);

    QElapsedTimer saveTimer;
    saveTimer.start();

    qDebug() << "Saving tags to" << tagsFilePath_;

    QSaveFile saveFile{tagsFilePath_};
    if (!saveFile.open(QIODevice::WriteOnly))
        return std::unexpected(QObject::tr("Could not open %1 for writing: %2").arg(tagsFilePath_).arg(saveFile.errorString()));

    QCborMap map;
    map[std::to_underlying(Key::FORMAT_VERSION)] = valueFormatVersion;
    map[std::to_underlying(Key::APP)] = valueApp.toString();

    map[std::to_underlying(Key::TAGS)] = assignedTags_ | std::ranges::to<QCborArray>();

    if (imageRegion_)
        map[std::to_underlying(Key::REGION)] = QCborArray({
            imageRegion_->left(), imageRegion_->top(), imageRegion_->right(), imageRegion_->bottom()
        });

    map[std::to_underlying(Key::COMPLETE_FLAG)] = completeFlag_;

    // optionals always set at the beginning of this function
    map[std::to_underlying(Key::TAG_LIBRARY_UUID)] = tagLibraryUuid_->toRfc4122();
    map[std::to_underlying(Key::TAG_LIBRARY_VERSION)] = *tagLibraryVersion_;
    map[std::to_underlying(Key::TAG_LIBRARY_VERSION_UUID)] = tagLibraryVersionUuid_->toRfc4122();

    QCborStreamWriter writer(&saveFile);
    map.toCborValue().toCbor(writer);

    qDebug() << "Preparatory saving time:" << saveTimer.elapsed() << "ms";

    if (!saveFile.commit())
        return std::unexpected(QObject::tr("Failed to write file %1: %2").arg(tagsFilePath_).arg(saveFile.errorString()));

    setModified_(false);

    qDebug() << "Total saving time:" << saveTimer.elapsed() << "ms";
    emit manager_.tagsSaved(backupCount);
    return {};
}

void FileTags::setModified_(bool const modified) {
    modified_ = modified;
    emit manager_.modifiedStateChanged(imageFilePath_, modified);
}

FileTagsManager::FileTagsManager(bool const backupOnSave): backupOnSave_(backupOnSave) {}

FileTagsManager::~FileTagsManager() = default;

void FileTagsManager::setTagLibrary(TagLibrary::Library *const library) {
    tagLibrary_ = library;
}

void FileTagsManager::setBackupOnSave(bool value) {
    this->backupOnSave_ = value;
}

std::expected<std::reference_wrapper<FileTags>, QString> FileTagsManager::forFile(const QString &path) {
    ZoneScoped;
    gsl_Expects(!QFileInfo(path).isDir());
    gsl_Expects(QFileInfo(path).isAbsolute());
    gsl_Expects(IMAGE_FILE_SUFFIXES.contains("."+QFileInfo(path).suffix()));

    QMutexLocker locker(&mutex_);

    auto it = fileTags_.find(path);
    if (it == fileTags_.end()) {
        QFileInfo fileInfo{path};

        auto tagsFilePath = fileInfo.dir().filePath(fileInfo.fileName() + Constants::TAGS_FILE_SUFFIX.toString());

        if (auto result = FileTags::create(*this, path, tagsFilePath, backupOnSave_); !result)
            return std::unexpected(result.error());
        else
            std::tie(it, std::ignore) = fileTags_.emplace(path, std::move(*result));
    }

    return *it->second;
}

int FileTagsManager::cachedFiles() const {
    QMutexLocker locker(&mutex_);
    return fileTags_.size();
}

