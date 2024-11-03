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
#include "DirectoryStats.hpp"

#include "DirectoryStatsManager.hpp"
#include "FileTagsManager.hpp"
#include "Project.hpp"

namespace {
// TODO: FileBrowser.cpp and FileTagsManager.cpp has a similar list (just with wildcards added), merge?
const QStringList IMAGE_FILE_SUFFIXES = {".jpg", ".png"};
}

DirectoryStats::DirectoryStats(DirectoryStatsManager &manager, QString const &path):
        manager_(manager), path_(path) {
    gsl_Expects(QFileInfo(path_).isAbsolute());
    gsl_Expects(QFileInfo(path_).isDir());
}

DirectoryStats::~DirectoryStats() = default;

QString DirectoryStats::path() const {
    return path_;
}

int DirectoryStats::fileCount() const {
    ZoneScoped;
    QMutexLocker locker(&mutex_);
    return stats_.fileCount_ + std::ranges::fold_left_first(
            stats_.childrenStats_ | std::views::transform([](auto const &v){ return v.get().fileCount(); }),
            std::plus<int>()
    ).value_or(0);
}

int DirectoryStats::filesExcluded() const {
    ZoneScoped;
    QMutexLocker locker(&mutex_);

    if (stats_.isExcluded_)
        return stats_.fileCount_;
    else
        return stats_.filesExcluded_ + std::ranges::fold_left_first(
                stats_.childrenStats_ | std::views::transform([](auto const &v){
                    return v.get().filesExcluded();
                }),
                std::plus<int>()
        ).value_or(0);
}

int DirectoryStats::filesFlaggedComplete() const {
    ZoneScoped;
    QMutexLocker locker(&mutex_);
    return stats_.filesFlaggedComplete_  + std::ranges::fold_left_first(
            stats_.childrenStats_ | std::views::transform([](auto const &v){ return v.get().filesFlaggedComplete(); }),
            std::plus<int>()
    ).value_or(0);
}

int DirectoryStats::filesFlaggedCompleteWithoutExcluded() const {
    ZoneScoped;
    QMutexLocker locker(&mutex_);
    return stats_.filesFlaggedCompleteWithoutExcluded_ + std::ranges::fold_left_first(
            stats_.childrenStats_ | std::views::transform([](auto const &v){ return v.get().filesFlaggedCompleteWithoutExcluded(); }),
            std::plus<int>()
    ).value_or(0);
}

int DirectoryStats::filesWithTags() const {
    ZoneScoped;
    QMutexLocker locker(&mutex_);
    return stats_.filesWithTags_ + std::ranges::fold_left_first(
            stats_.childrenStats_ | std::views::transform([](auto const &v){ return v.get().filesWithTags(); }),
            std::plus<int>()
    ).value_or(0);
}

int DirectoryStats::filesWithTagsWithoutExcluded() const {
    ZoneScoped;
    QMutexLocker locker(&mutex_);
    return stats_.filesWithTagsWithoutExcluded_ + std::ranges::fold_left_first(
            stats_.childrenStats_ | std::views::transform([](auto const &v){ return v.get().filesWithTagsWithoutExcluded(); }),
            std::plus<int>()
    ).value_or(0);
}

int DirectoryStats::totalTags() const {
    ZoneScoped;
    gsl_Expects(ready());

    QMutexLocker locker(&mutex_);
    return stats_.totalTags_ + std::ranges::fold_left_first(
            stats_.childrenStats_ | std::views::transform([](auto const &v){ return v.get().totalTags(); }),
            std::plus<int>()
    ).value_or(0);
}

bool DirectoryStats::ready() const {
    ZoneScoped;
    QMutexLocker locker(&mutex_);
    return stats_.loaded_ && std::ranges::all_of(stats_.childrenStats_, [](auto const &v){ return v.get().ready(); });
}

void DirectoryStats::reload() {
    gsl_Expects(manager_.project_);
    gsl_Expects(QFileInfo(path_).isAbsolute());

    {
        QMutexLocker locker(&mutex_);
        stats_.loaded_ = false;
    }

    emitStatsUpdate();

    manager_.threadPool_.start([this](){
        //qDebug() << "Loading directory stats for" << path_;

        Stats stats;

        stats.isExcluded_ = manager_.project_->isExcludedFile(QDir(manager_.project_->rootDir()).relativeFilePath(path_));

        QDirIterator iterator(path_, QDir::Filter::AllEntries | QDir::Filter::NoDotAndDotDot);
        while (iterator.hasNext()) {
            if (manager_.threadPoolInterrupt_.test())
                return;

            auto info = iterator.nextFileInfo();
            if (info.isDir()) {
                stats.childrenStats_.push_back(manager_.directoryStats(info.filePath()));
            } else if (IMAGE_FILE_SUFFIXES.contains("."+info.suffix())) {
                auto tags = manager_.fileTagsManager_.forFile(info.absoluteFilePath());
                if (!tags) {
                    qWarning() << "Couldn't get tags information for file" << info.absoluteFilePath() << "; this data won't be taken into account for statistics calculation";
                    continue;
                }

                bool isExcluded = manager_.project_->isExcludedFile(
                        QDir(manager_.project_->rootDir()).relativeFilePath(info.absoluteFilePath())
                );

                stats.fileCount_ += 1;

                if (isExcluded)
                    stats.filesExcluded_ += 1;

                auto size = tags->get().assignedTags().size();
                if (size != 0) {
                    stats.filesWithTags_ += 1;

                    if (!isExcluded)
                        stats.filesWithTagsWithoutExcluded_ += 1;
                }

                if (tags->get().isCompleteFlag()) {
                    stats.filesFlaggedComplete_ += 1;

                    if (!isExcluded)
                        stats.filesFlaggedCompleteWithoutExcluded_ += 1;
                }

                stats.totalTags_ += size;
            }
        };

        stats.loaded_ = true;

        //qDebug() << "Loaded directory stats for" << path_ << ": fileCount:" << stats.fileCount_ << "; filesWithTags:" << stats.filesWithTags_ << "; totalTags:" << stats.totalTags_ << "; childrenStats (count):" << stats.childrenStats_.size();

        gsl_Ensures(stats.filesExcluded_ >= 0);
        gsl_Ensures(stats.filesExcluded_ <= stats.fileCount_);

        gsl_Ensures(stats.filesFlaggedComplete_ >= 0);
        gsl_Ensures(stats.filesFlaggedComplete_ <= stats.fileCount_);

        gsl_Ensures(stats.filesFlaggedCompleteWithoutExcluded_ >= 0);
        gsl_Ensures(stats.filesFlaggedCompleteWithoutExcluded_ <= stats.filesFlaggedComplete_);

        gsl_Ensures(stats.filesWithTags_ >= 0);
        gsl_Ensures(stats.filesWithTags_ <= stats.fileCount_);

        gsl_Ensures(stats.filesWithTagsWithoutExcluded_ >= 0);
        gsl_Ensures(stats.filesWithTagsWithoutExcluded_ <= stats.filesWithTags_);

        gsl_Ensures(stats.totalTags_ >= 0);

        {
            QMutexLocker locker(&mutex_);
            stats_ = stats;
        }

        emitStatsUpdate();
    });
}

void DirectoryStats::emitStatsUpdate() {
    emit manager_.directoryStatsChanged(path_);
}
