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
#include "IconIdentifier.hpp"

IconIdentifier::IconIdentifier() = default;

IconIdentifier::IconIdentifier(QString const &path): path_(path) {}

IconIdentifier::~IconIdentifier() = default;

QString IconIdentifier::name() const {
    return path_;
}

QIcon IconIdentifier::instantiate() const {
    ZoneScoped;
    return QIcon(path_);
}

QIcon &IconIdentifier::instantiate(IconIdentifier::IconCache &cache) const {
    ZoneScoped;
    auto it = cache.cache.find(path_);
    if (it == cache.cache.end())
        std::tie(it, std::ignore) = cache.cache.emplace(path_, path_);
    return it->second;
}

IconIdentifier::IconCache::IconCache() = default;

IconIdentifier::IconCache::~IconCache() = default;
