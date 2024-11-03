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
#include "Utility.hpp"

#include "../FileTagsManager.hpp"

namespace FileBrowser {
QString formatDirectoryStats(DirectoryTagsStats const &stats, QString const &path) {
    QStringList lines;
    lines.push_back(path);

    if (!stats.ready()) {
        lines.push_back(QObject::tr("(loading statistics...)"));
    } else {
        if (stats.fileCount() == 0)
            lines.push_back(QObject::tr("no image files"));
        else {
            auto precision = std::max(0, static_cast<int>(std::floor(std::log10(stats.fileCount()))) + 1 - 2);
            lines.push_back(QObject::tr("%1 / %2 files have tags (%3%)")
                                    .arg(stats.filesWithTags())
                                    .arg(stats.fileCount())
                                            // I don't know how to format numbers with Qt :<
                                    .arg(QString::fromStdString(std::format(
                                            std::runtime_format(std::format("{{:.{}f}}", precision)),
                                            (static_cast<float>(stats.filesWithTags()) /
                                             static_cast<float>(stats.fileCount())) * 100.f
                                    )))
            );
            lines.push_back(QObject::tr("%1 total tags").arg(stats.totalTags()));
        }
    }

    return lines.join("\n");
}
}
