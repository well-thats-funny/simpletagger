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

class DirectoryStatsManager;

class DirectoryStats: public QObject {
    Q_OBJECT

    DirectoryStats(DirectoryStats const &other) = delete;
    DirectoryStats(DirectoryStats &&other) = delete;
    DirectoryStats& operator=(DirectoryStats const &other) = delete;
    DirectoryStats& operator=(DirectoryStats &&other) = delete;

    friend class DirectoryStatsManager;

    DirectoryStats(DirectoryStatsManager &manager, QString const &path);

public:
    ~DirectoryStats();

    QString path() const;

    int fileCount() const;
    int filesExcluded() const;
    int filesFlaggedComplete() const;
    int filesFlaggedCompleteWithoutExcluded() const;
    int filesWithTags() const;
    int filesWithTagsWithoutExcluded() const;
    int totalTags() const;

    bool ready() const;

    void reload();

private:
    void emitStatsUpdate();

    DirectoryStatsManager &manager_;
    QString path_;

    mutable QMutex mutex_;
    struct Stats {
        std::vector<std::reference_wrapper<DirectoryStats>> childrenStats_;

        bool loaded_ = false;
        bool isExcluded_ = false;
        int fileCount_ = 0;
        int filesExcluded_ = 0;
        int filesFlaggedComplete_ = 0;
        int filesFlaggedCompleteWithoutExcluded_ = 0;
        int filesWithTags_ = 0;
        int filesWithTagsWithoutExcluded_ = 0;
        int totalTags_ = 0;
    };

    Stats stats_;
};
