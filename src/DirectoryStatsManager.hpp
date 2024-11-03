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

class DirectoryStats;
class FileTagsManager;
class Project;

namespace TagLibrary {
class Library;
}

class DirectoryStatsManager: public QObject {
    Q_OBJECT

    friend class DirectoryStats;

    DirectoryStatsManager(DirectoryStatsManager const &other) = delete;
    DirectoryStatsManager(DirectoryStatsManager &&other) = delete;
    DirectoryStatsManager& operator=(DirectoryStatsManager const &other) = delete;
    DirectoryStatsManager& operator=(DirectoryStatsManager &&other) = delete;

public:
    explicit DirectoryStatsManager(FileTagsManager &fileTagsManager);
    ~DirectoryStatsManager();

    void setProject(Project *project);
    void setTagLibrary(TagLibrary::Library *tagLibrary);

    DirectoryStats &directoryStats(QString const &path);
    void invalidateDirectoryStatsCache();
    int cachedDirectories() const;

signals:
    void directoryStatsChanged(QString const &path);

private:
    Project *project_ = nullptr;
    TagLibrary::Library *tagLibrary_ = nullptr;
    FileTagsManager &fileTagsManager_;

    QMutex mutex_;
    std::unordered_map<QString, std::unique_ptr<DirectoryStats>> stats_;

    // TODO: this is placed AFTER directoryStats_ because at the moment, DirectoryTagsStats doesn't
    //       interrupt (and wait) pending reloads in destructor. Doing it there would be "more correct",
    //       but simpler way for now is ensuring the pool is destroyed before "directoryStats_" map.
    std::atomic_flag threadPoolInterrupt_;
    // TODO: get threadpool as a dependency? We could have a global I/O threadpool
    QThreadPool threadPool_;
};
