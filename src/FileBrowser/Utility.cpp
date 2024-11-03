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

#include "../DirectoryStats.hpp"

namespace FileBrowser {
QString formatDirectoryStats(DirectoryStats const &stats, QString const &path) {
    QStringList lines;
    lines.push_back(path);

    if (!stats.ready()) {
        lines.push_back(QObject::tr("(loading statistics...)"));
    } else {
        if (stats.fileCount() == 0)
            lines.push_back(QObject::tr("no image files"));
        else {
            auto fileCount = stats.fileCount() - stats.filesExcluded();

            auto precision = std::max(0, static_cast<int>(std::floor(std::log10(stats.fileCount()))) + 1 - 2);
            auto percentFiles = [&](auto const &v) {
                // I don't know how to format numbers with Qt :<
                return QString::fromStdString(std::format(
                        std::runtime_format(std::format("{{:.{}f}}", precision)),
                        (static_cast<float>(v) / static_cast<float>(fileCount)) * 100.f
                ));
            };

            if (fileCount > 0) {
                QString secondLine = QObject::tr(
                            "%1 / %2 (%3%) file(s) completed", "", stats.filesFlaggedCompleteWithoutExcluded()
                        )
                        .arg(stats.filesFlaggedCompleteWithoutExcluded())
                        .arg(fileCount)
                        .arg(percentFiles(stats.filesFlaggedCompleteWithoutExcluded()));

                if (auto filesIncomplete =
                            stats.filesWithTagsWithoutExcluded() - stats.filesFlaggedCompleteWithoutExcluded();
                        filesIncomplete > 0) {
                    secondLine += QObject::tr(
                                ", %1 / %2 (%3%) file(s) started but not completed",
                                "",
                                filesIncomplete
                            )
                            .arg(filesIncomplete)
                            .arg(fileCount)
                            .arg(percentFiles(filesIncomplete));
                }

                lines.push_back(secondLine);
            }

            QStringList thirdLine;
            if (auto excluded = stats.filesExcluded(); excluded != 0) {
                if (fileCount == 0)
                    thirdLine.push_back(QObject::tr("%1 excluded file(s)", "", excluded).arg(excluded));
                else
                    thirdLine.push_back(QObject::tr(
                                    "%1 excluded file(s) (not included in the counters above)",
                                    "",
                                    excluded
                            )
                            .arg(excluded)
                    );
            }

            auto otherTagLibrary = stats.filesOtherTagLibrary();
            auto otherTagLibraryVersion = stats.filesOtherTagLibraryVersion();
            auto otherTagLibraryCount = otherTagLibrary + otherTagLibraryVersion;
            if (otherTagLibraryCount != 0)
                thirdLine.push_back(QObject::tr(
                        "%1 file(s) last edited with different tag library/version",
                        "",
                        otherTagLibraryCount
                ).arg(otherTagLibraryCount));

            if (!thirdLine.isEmpty())
                lines.push_back(thirdLine.join(", "));

            lines.push_back(QObject::tr("%1 total tags").arg(stats.totalTags()));
        }
    }

    return lines.join("\n");
}
}
