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

class Project {
public:
    [[nodiscard]] static std::expected<void, QString> create(QString const &path);
    [[nodiscard]] std::expected<void, QString> save(bool backup);
    [[nodiscard]] static std::expected<Project, QString> open(QString const &path);

    bool isExcludedFile(QString const &fileName);
    void setExcludedFile(QString const &fileName, bool excluded);

    // TODO: these should be private as well
    QString path;
    QStringList directories;

private:
    QStringList excludedFiles_;
};
