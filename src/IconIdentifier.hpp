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

class IconIdentifier {
public:
    class IconCache;

    IconIdentifier(IconIdentifier const &other) = default;
    IconIdentifier(IconIdentifier &&other) = default;
    IconIdentifier& operator=(IconIdentifier const &other) = default;
    IconIdentifier& operator=(IconIdentifier &&other) = default;

    IconIdentifier();
    explicit IconIdentifier(QString const &path);
    ~IconIdentifier();

    [[nodiscard]] QString name() const;

    QIcon instantiate() const;
    QIcon &instantiate(IconCache &cache) const;

private:
    QString path_;
};

class IconIdentifier::IconCache {
    friend class IconIdentifier;

    IconCache(IconCache const &other) = delete;
    IconCache(IconCache &&other) = delete;
    IconCache& operator=(IconCache const &other) = delete;
    IconCache& operator=(IconCache &&other) = delete;

public:
    IconCache();
    ~IconCache();

private:
    std::unordered_map<QString, QIcon> cache;
};
