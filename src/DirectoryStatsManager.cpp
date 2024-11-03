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
#include "DirectoryStatsManager.hpp"

#include "DirectoryStats.hpp"

DirectoryStatsManager::DirectoryStatsManager(FileTagsManager &fileTagsManager): fileTagsManager_(fileTagsManager) {
    threadPool_.setMaxThreadCount(8); // TODO: configurable?
}

DirectoryStatsManager::~DirectoryStatsManager() {
    threadPoolInterrupt_.test_and_set();
}

void DirectoryStatsManager::setProject(Project *const project) {
    project_ = project;
}

void DirectoryStatsManager::setTagLibrary(TagLibrary::Library *const tagLibrary) {
    tagLibrary_ = tagLibrary;
}

DirectoryStats &DirectoryStatsManager::directoryStats(QString const &path) {
    ZoneScoped;
    gsl_Expects(QFileInfo(path).isAbsolute());
    gsl_Expects(QFileInfo(path).isDir());

    QMutexLocker locker(&mutex_);

    auto it = stats_.find(path);
    if (it == stats_.end()) {
        std::tie(it, std::ignore) = stats_.emplace(path, std::unique_ptr<DirectoryStats>(new DirectoryStats(*this, path)));
        it->second->reload();
    }

    return *it->second;
}

void DirectoryStatsManager::invalidateDirectoryStatsCache() {
    ZoneScoped;

    QMutexLocker locker(&mutex_);

    for (auto const &stats: stats_)
        stats.second->reload();
}

int DirectoryStatsManager::cachedDirectories() const {
    return stats_.size();
}
