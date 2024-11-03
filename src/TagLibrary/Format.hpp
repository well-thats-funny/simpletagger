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

namespace TagLibrary::Format {
Q_NAMESPACE

enum class TopLevelKey {
    FormatVersion = 1,
    App = 2,
    RootNode = 3,
    LibraryUuid = 4,
    LibraryVersion = 5,
    LibraryVersionUuid = 6
};

constexpr unsigned int formatVersion = 1;
static constexpr QAnyStringView app = "SIMPLETAGGER-CXX";

enum class NodeKey {
    Type = 1,
    Children = 2,
    Name = 3,
    Icon = 4,
    Uuid = 5,
    LinkTo = 6,
    Tags = 7,
    Comment = 8,
    Hidden = 9,
};

enum class NodeType {
    Root = 1,
    Collection = 2,
    Object = 3,
    Link = 4
};
Q_ENUM_NS(NodeType);

}
