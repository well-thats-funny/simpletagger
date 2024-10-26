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
#include "NodeCollection.hpp"

#include "Format.hpp"

#include "../Utility.hpp"

namespace TagLibrary {
NodeCollection::NodeCollection(Model &model, NodeSerializable const *const parent):
    NodeHierarchical(model, parent), name_(parent->generateUnusedChildName(QObject::tr("New collection"))) {}

NodeCollection::~NodeCollection() = default;

bool NodeCollection::canInsertChild(NodeType const type) const {
    if (type == NodeType::Collection || type == NodeType::Object || type == NodeType::Link)
        return true;

    return false;
}

bool NodeCollection::canRemoveChildren(int const, int const) const {
    return true;
}

bool NodeCollection::canBeDragged() const {
    return parent()->type() != NodeType::Root;  // disallow dragging of the top-level collections
}

QString NodeCollection::name(bool const, bool const) const {
    return name_;
}

bool NodeCollection::canSetName() const {
    return parent()->type() != NodeType::Root;  // disallow renaming of the top-level collections
}

bool NodeCollection::setName(QString const &name) {
    name_ = name;
    emit persistentDataChanged();
    return true;
}

QString NodeCollection::comment() const {
    return comment_;
}

bool NodeCollection::canSetComment() const {
    return true;
}

std::expected<void, QString> NodeCollection::setComment(QString const &comment) {
    ZoneScoped;
    if (comment != comment_) {
        comment_ = comment;
        emit persistentDataChanged();
    }
    return {};
}

IconIdentifier NodeCollection::genericIcon() {
    return IconIdentifier(":/icons/bx-book.svg");
}

std::vector<IconIdentifier> NodeCollection::icons() const {
    ZoneScoped;
    return {genericIcon()};
}

NodeType NodeCollection::type() const {
    return NodeType::Collection;
}

std::expected<void, QString> NodeCollection::saveNodeData(QCborMap &map) const {
    ZoneScoped;

    if (auto result = NodeHierarchical::saveNodeData(map); !result)
        return std::unexpected(result.error());

    map[std::to_underlying(Format::NodeKey::Name)] = name_;

    if (!comment_.isEmpty())
        map[std::to_underlying(Format::NodeKey::Comment)] = comment_;

    return {};
}

std::expected<void, QString> NodeCollection::loadNodeData(QCborMap &map) {
    ZoneScoped;

    if (auto result = NodeHierarchical::loadNodeData(map); !result)
        return std::unexpected(result.error());

    auto name = map.take(std::to_underlying(Format::NodeKey::Name));
    if (!name.isString())
        return std::unexpected(QObject::tr("Name element is not a string but %1").arg(cborTypeToString(name.type())));

    name_ = name.toString();

    auto comment = map.take(std::to_underlying(Format::NodeKey::Comment));
    if (!comment.isUndefined()) {
        if (!comment.isString())
            return std::unexpected(
                    QObject::tr("Comment element is not a string but %1").arg(cborTypeToString(comment.type())));
        else
            comment_ = comment.toString();
    }

    return {};
}
}
