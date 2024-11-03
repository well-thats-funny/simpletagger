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
#include "Project.hpp"

#include "Utility.hpp"

namespace {
    enum class Keys: int {
        INVALID = 0,
        FORMAT_VERSION = 1,
        APP = 2,
        DIRECTORIES = 3,
        EXCLUDED_FILES = 4
    };

    constexpr int valueVersion = 1;
    constexpr QAnyStringView valueApp = "SIMPLETAGGER-CXX";
}

std::expected<void, QString> Project::create(const QString &path) {
    ZoneScoped;

    qDebug() << "Wrtiting basic project file: " << path;

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return std::unexpected(QObject::tr("Could not open file for writing: %1 (%2)").arg(path).arg(file.errorString()));

    QCborMap map;
    map[std::to_underlying(Keys::FORMAT_VERSION)] = valueVersion;
    map[std::to_underlying(Keys::APP)] = valueApp.toString();
    map[std::to_underlying(Keys::DIRECTORIES)] = QCborArray();

    file.write(map.toCborValue().toCbor());

    if (!file.commit())
        return std::unexpected(QObject::tr("Could not write file: %1 (%2)").arg(path).arg(file.errorString()));

    return {};
}

std::expected<std::optional<int>, QString> Project::save(bool const backup) {
    ZoneScoped;

    std::optional<int> backupCount;

    if (backup)
        backupCount = backupFile(path);

    qDebug() << "Writing project file: " << path;

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return std::unexpected(QObject::tr("Could not open file for writing: %1 (%2)").arg(path).arg(file.errorString()));

    QCborMap map;
    map[std::to_underlying(Keys::FORMAT_VERSION)] = valueVersion;
    map[std::to_underlying(Keys::APP)] = valueApp.toString();

    QCborArray cDirectories;
    for (auto const &d: directories)
        cDirectories.append(d);

    map[std::to_underlying(Keys::DIRECTORIES)] = cDirectories;

    QCborArray cExcludedFiles;
    for (auto const &f: excludedFiles_)
        cExcludedFiles.append(f);
    map[std::to_underlying(Keys::EXCLUDED_FILES)] = cExcludedFiles;

    file.write(map.toCborValue().toCbor());

    if (!file.commit())
        return std::unexpected(QObject::tr("Could not write file: %1 (%2)").arg(path).arg(file.errorString()));

    qDebug() << "Writing project file done";

    return backupCount;
}

std::expected<Project, QString> Project::open(QString const &path) {
    ZoneScoped;

    qDebug() << "Opening project: " << path;

    QFile file{path};
    if (!file.open(QIODevice::ReadOnly))
        return std::unexpected(QObject::tr("Could not open file for reading: %1 (%2)").arg(path).arg(file.errorString()));

    QByteArray content = file.readAll();
    auto map = QCborValue::fromCbor(content).toMap();

    auto project = Project();
    project.path = path;

    auto formatVersion = map.take(std::to_underlying(Keys::FORMAT_VERSION));
    if (formatVersion.isUndefined())
        return std::unexpected(QObject::tr("Missing format version"));
    if (!formatVersion.isInteger())
        return std::unexpected(QObject::tr("Format version is not an integer, but %1").arg(cborTypeToString(formatVersion.type())));
    if (formatVersion.toInteger() != valueVersion)
        return std::unexpected(QObject::tr("Unknown format version: %1").arg(formatVersion.toInteger()));

    auto app = map.take(std::to_underlying(Keys::APP));
    if (app.isUndefined())
        return std::unexpected(QObject::tr("Application key doesn't exist"));
    if (!app.isString())
        return std::unexpected(QObject::tr("Application key is not a string but %1").arg(cborTypeToString(app.type())));
    if (app.toString() != valueApp.toString())
        qWarning() << "Unknown application value:" << app.toString();

    auto directories = map.take(std::to_underlying(Keys::DIRECTORIES));
    if (directories.isUndefined())
        return std::unexpected(QObject::tr("Directories key doesn't exist"));
    if (!directories.isArray())
        return std::unexpected(QObject::tr("Directories key is not a string but %1").arg(cborTypeToString(directories.type())));

    for (auto const &d: directories.toArray()) {
        if (!d.isString())
            return std::unexpected(QObject::tr("Directories element is not a string but %1").arg(cborTypeToString(d.type())));
        project.directories.append(d.toString());
    }

    auto excludedFiles = map.take(std::to_underlying(Keys::EXCLUDED_FILES));
    if (excludedFiles.isUndefined())
        return std::unexpected(QObject::tr("Excluded files key doesn't exist"));
    if (!excludedFiles.isArray())
        return std::unexpected(QObject::tr("Excluded files key is not a string but %1").arg(cborTypeToString(excludedFiles.type())));

    for (auto const &f: excludedFiles.toArray()) {
        if (!f.isString())
            return std::unexpected(QObject::tr("Excluded files element is not a string but %1").arg(cborTypeToString(f.type())));
        project.excludedFiles_.append(f.toString());
    }

    for (auto const &v: map)
        qWarning() << "Project file: unhandled element" << v.first << "=" << v.second;

    return project;
}

bool Project::isExcludedFile(const QString &fileName) {
    ZoneScoped;
    gsl_Expects(QFileInfo(fileName).isRelative());
    return excludedFiles_.contains(fileName);
}

void Project::setExcludedFile(QString const &fileName, bool const excluded) {
    ZoneScoped;
    gsl_Expects(QFileInfo(fileName).isRelative());
    if (excluded && !excludedFiles_.contains(fileName))
        excludedFiles_.push_back(fileName);
    else if (!excluded && excludedFiles_.contains(fileName))
        excludedFiles_.removeAll(fileName);
}
