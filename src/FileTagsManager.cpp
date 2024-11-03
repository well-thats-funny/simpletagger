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
#include "Utility.hpp"

namespace {
    // TODO: FileBrowser.cpp has a similar list (just with wildcards added), merge?
    const QStringList IMAGE_FILE_SUFFIXES = {".jpg", ".png"};

    enum class Key {
        FORMAT_VERSION = 1,
        APP = 2,
        TAGS = 3,
        REGION = 4,
        COMPLETE_FLAG = 5
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

FileTags::FileTags(FileTagsManager &manager, QString const &tagsFilePath, bool const backupOnSave):
    manager_(manager), tagsFilePath_(tagsFilePath), backupOnSave_(backupOnSave) {
    ZoneScoped;

    load();
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
        save(backupOnSave_);

    return changed;
}

bool FileTags::setTagsState(std::unordered_map<QString, bool> const &state) {
    ZoneScoped;

    bool changed = false;

    for (auto const &[tag, value]: state)
        if (setTag_(tag, value))
            changed = true;

    if (changed)
        save(backupOnSave_);

    return changed;
}

std::expected<void, QString> FileTags::overwriteAssignedTags(QStringList const &activeTags) {
    ZoneScoped;

    assignedTags_ = activeTags;
    save(backupOnSave_);
    return {};
}

bool FileTags::moveAssignedTag(int const sourcePositon, int const targetPosition) {
    ZoneScoped;
    gsl_Expects(sourcePositon >= 0);
    gsl_Expects(sourcePositon < assignedTags_.size());
    gsl_Expects(targetPosition >= 0);
    gsl_Expects(targetPosition < assignedTags_.size());

    if (sourcePositon == targetPosition)
        return false;

    assignedTags_.move(sourcePositon, targetPosition);
    save(backupOnSave_);
    return true;
}

bool FileTags::clearTags() {
    ZoneScoped;

    if (!assignedTags_.empty()) {
        assignedTags_.clear();
        save(backupOnSave_);
        return true;
    } else {
        return false;
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

    if (rect == imageRegion_)
        return false;

    imageRegion_ = rect;
    save(backupOnSave_);
    return true;
}

bool FileTags::isCompleteFlag() const {
    return completeFlag_;
}

void FileTags::setCompleteFlag(bool value) {
    ZoneScoped;

    if (value != completeFlag_) {
        completeFlag_ = value;
        save(backupOnSave_);
    }
}

void FileTags::load() {
    ZoneScoped;

    assignedTags_.clear();
    imageRegion_.reset();

    if (QFileInfo{tagsFilePath_}.exists()) {
        qDebug() << "Loading tags from" << tagsFilePath_;

        auto res = loadTagsFile(tagsFilePath_);
        if (!res.has_value()) {
            qWarning() << "Could not load tags from" << tagsFilePath_ << ":" << res.error();
            return;
        }

        if (auto itTags = res->map.find(std::to_underlying(Key::TAGS)); itTags != res->map.end()) {
            if (!itTags->isArray())
                qWarning() << "Tags value in" << tagsFilePath_ << "is not an array";
            else {
                auto array = itTags->toArray();
                for(auto const &tag: array) {
                    if (!tag.isString())
                        qWarning() << "Tag" << tag << "is not a string";
                    else
                        assignedTags_.append(tag.toString());
                }
            }
        }

        if (auto region = res->map.take(std::to_underlying(Key::REGION)); !region.isUndefined()) {
            if (!region.isArray()
                || region.toArray().size() != 4
                || !std::ranges::all_of(region.toArray(), [](auto const &v){ return v.isInteger(); })) {
                qWarning() << "Region value in" << tagsFilePath_ << "is not a 4-element integer array";
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
                qWarning() << "Complete flag in" << tagsFilePath_ << "is not a bool, but" << cborTypeToString(completeFlag.type());
            else
                completeFlag_ = completeFlag.toBool();
        }

        qDebug() << "Loading tags from" << tagsFilePath_<< ": done";
    }
}

void FileTags::save(bool backup) {
    ZoneScoped;

    // First, open the original file, to see whether it can be opened without warnings. In case there were warnings,
    // we first backup the original file
    QFileInfo tagsFileInfo{tagsFilePath_};

    if (tagsFileInfo.exists()) {
        auto res = loadTagsFile(tagsFilePath_);
        backup = backup || !res.has_value() || res->warningsOccurred;
    }

    std::optional<int> backupCount;

    if (backup)
        backupCount = backupFile(tagsFilePath_);

    QElapsedTimer saveTimer;
    saveTimer.start();

    qDebug() << "Saving tags to" << tagsFilePath_;

    QSaveFile saveFile{tagsFilePath_};
    if (!saveFile.open(QIODevice::WriteOnly)) {
        qWarning() << "Could not open for writing:" << tagsFilePath_;
    }

    QCborMap map;
    map[std::to_underlying(Key::FORMAT_VERSION)] = valueFormatVersion;
    map[std::to_underlying(Key::APP)] = valueApp.toString();

    map[std::to_underlying(Key::TAGS)] = assignedTags_ | std::ranges::to<QCborArray>();

    if (imageRegion_)
        map[std::to_underlying(Key::REGION)] = QCborArray({
            imageRegion_->left(), imageRegion_->top(), imageRegion_->right(), imageRegion_->bottom()
        });

    map[std::to_underlying(Key::COMPLETE_FLAG)] = completeFlag_;

    QCborStreamWriter writer(&saveFile);
    map.toCborValue().toCbor(writer);

    qDebug() << "Preparatory saving time:" << saveTimer.elapsed() << "ms";

    if (!saveFile.commit())
        qWarning() << "Failed to write file" << tagsFilePath_;

    qDebug() << "Total saving time:" << saveTimer.elapsed() << "ms";
    emit manager_.tagsSaved(backupCount);
}

DirectoryTagsStats::DirectoryTagsStats(FileTagsManager &manager, QString const &path):
    manager_(manager), path_(path) {
    gsl_Expects(QFileInfo(path_).isAbsolute());
    gsl_Expects(QFileInfo(path_).isDir());
}

DirectoryTagsStats::~DirectoryTagsStats() = default;

QString DirectoryTagsStats::path() const {
    return path_;
}

int DirectoryTagsStats::fileCount() const {
    ZoneScoped;
    QMutexLocker locker(&mutex_);
    return stats_.fileCount_ + std::ranges::fold_left_first(
            stats_.childrenStats_ | std::views::transform([](auto const &v){ return v.get().fileCount(); }),
            std::plus<int>()
    ).value_or(0);
}

int DirectoryTagsStats::filesFlaggedComplete() const {
    ZoneScoped;
    QMutexLocker locker(&mutex_);
    return stats_.filesFlaggedComplete_  + std::ranges::fold_left_first(
            stats_.childrenStats_ | std::views::transform([](auto const &v){ return v.get().filesFlaggedComplete(); }),
            std::plus<int>()
    ).value_or(0);
}

int DirectoryTagsStats::filesWithTags() const {
    ZoneScoped;
    QMutexLocker locker(&mutex_);
    return stats_.filesWithTags_ + std::ranges::fold_left_first(
            stats_.childrenStats_ | std::views::transform([](auto const &v){ return v.get().filesWithTags(); }),
            std::plus<int>()
    ).value_or(0);
}

int DirectoryTagsStats::totalTags() const {
    ZoneScoped;
    gsl_Expects(ready());

    QMutexLocker locker(&mutex_);
    return stats_.totalTags_ + std::ranges::fold_left_first(
            stats_.childrenStats_ | std::views::transform([](auto const &v){ return v.get().totalTags(); }),
            std::plus<int>()
    ).value_or(0);
}

bool DirectoryTagsStats::ready() const {
    ZoneScoped;
    QMutexLocker locker(&mutex_);
    return stats_.loaded_ && std::ranges::all_of(stats_.childrenStats_, [](auto const &v){ return v.get().ready(); });
}

void DirectoryTagsStats::reload() {
    {
        QMutexLocker locker(&mutex_);
        stats_.loaded_ = false;
    }

    emitStatsUpdate();

    manager_.directoryStatsThreadPool_.start([this](){
        //qDebug() << "Loading directory stats for" << path_;

        Stats stats;

        QDirIterator iterator(path_, QDir::Filter::AllEntries | QDir::Filter::NoDotAndDotDot);
        while (iterator.hasNext()) {
            if (manager_.directoryStatsThreadPoolInterrupt_.test())
                return;

            auto info = iterator.nextFileInfo();
            if (info.isDir()) {
                stats.childrenStats_.push_back(manager_.directoryStats(info.filePath()));
            } else if (IMAGE_FILE_SUFFIXES.contains("."+info.suffix())) {
                auto &tags = manager_.forFile(info.absoluteFilePath());
                stats.fileCount_ += 1;

                auto size = tags.assignedTags().size();
                if (size != 0)
                    stats.filesWithTags_ += 1;

                if (tags.isCompleteFlag())
                    stats.filesFlaggedComplete_ += 1;

                stats.totalTags_ += size;
            }
        };

        stats.loaded_ = true;

        //qDebug() << "Loaded directory stats for" << path_ << ": fileCount:" << stats.fileCount_ << "; filesWithTags:" << stats.filesWithTags_ << "; totalTags:" << stats.totalTags_ << "; childrenStats (count):" << stats.childrenStats_.size();

        {
            QMutexLocker locker(&mutex_);
            stats_ = stats;
        }

        emitStatsUpdate();
    });
}

void DirectoryTagsStats::emitStatsUpdate() {
    emit manager_.directoryStatsChanged(path_);
}

FileTagsManager::FileTagsManager(bool const backupOnSave): backupOnSave_(backupOnSave) {
    directoryStatsThreadPool_.setMaxThreadCount(8); // TODO: configurable?
}

FileTagsManager::~FileTagsManager() {
    directoryStatsThreadPoolInterrupt_.test_and_set();
}

void FileTagsManager::setBackupOnSave(bool value) {
    this->backupOnSave_ = value;
}

FileTags &FileTagsManager::forFile(const QString &path) {
    ZoneScoped;
    gsl_Expects(!QFileInfo(path).isDir());
    gsl_Expects(QFileInfo(path).isAbsolute());
    gsl_Expects(IMAGE_FILE_SUFFIXES.contains("."+QFileInfo(path).suffix()));

    QMutexLocker locker(&fileTagsMutex_);

    auto it = fileTags_.find(path);
    if (it == fileTags_.end()) {
        QFileInfo fileInfo{path};

        auto tagsFilePath = fileInfo.dir().filePath(fileInfo.fileName() + Constants::TAGS_FILE_SUFFIX.toString());

        std::tie(it, std::ignore) = fileTags_.emplace(path, std::make_unique<FileTags>(*this, tagsFilePath, backupOnSave_));
    }

    return *it->second;
}

DirectoryTagsStats &FileTagsManager::directoryStats(QString const &path) {
    ZoneScoped;
    gsl_Expects(QFileInfo(path).isAbsolute());
    gsl_Expects(QFileInfo(path).isDir());

    QMutexLocker locker(&directoryStatsMutex_);

    auto it = directoryStats_.find(path);
    if (it == directoryStats_.end()) {
        std::tie(it, std::ignore) = directoryStats_.emplace(path, std::unique_ptr<DirectoryTagsStats>(
                new DirectoryTagsStats(*this, path)));
        it->second->reload();
    }

    return *it->second;
}

void FileTagsManager::invalidateDirectoryStatsCache() {
    ZoneScoped;

    QMutexLocker locker(&directoryStatsMutex_);

    for (auto const &stats: directoryStats_)
        stats.second->reload();
}

int FileTagsManager::cachedFiles() const {
    return fileTags_.size();
}

int FileTagsManager::cachedDirectories() const {
    return directoryStats_.size();
}
