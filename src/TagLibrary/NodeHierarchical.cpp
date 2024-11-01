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
#include "NodeHierarchical.hpp"

#include "Format.hpp"
#include "Logging.hpp"

#include "../Utility.hpp"

namespace TagLibrary {
NodeHierarchical::NodeHierarchical(Model &model, NodeSerializable const *const parent): NodeSerializable(model), parent_(parent) {}

NodeHierarchical::~NodeHierarchical() = default;

void NodeHierarchical::deinit() {
    ZoneScoped;

    NodeSerializable::deinit();

    for (auto &child: children_)
        child->deinit();
}

Node const *NodeHierarchical::parent() const {
    return parent_;
}

bool NodeHierarchical::canRemove() const {
    ZoneScoped;
    auto row = parent()->rowOfChild(*this);
    gsl_Expects(row);
    return parent()->canRemoveChildren(*row, *row);
}

std::expected<int, QString> NodeHierarchical::rowOfChild(Node const &node) const {
    ZoneScoped;

    auto it = std::ranges::find_if(
            children_,
            [&](auto const &child){ return &*child == &node; }
    );
    if (it == children_.end())
        return std::unexpected(QObject::tr("Node %1 (%2) not found among the children of node %3 (%4)").arg(
                node.uuid().toString(QUuid::WithoutBraces), node.name(true),
                uuid().toString(QUuid::WithoutBraces), name(true)
        ));

    return it - children_.begin();
}

std::expected<std::reference_wrapper<Node>, QString> NodeHierarchical::childOfRow(int const row) const {
    ZoneScoped;

    assert(row >= 0);
    assert(row < gsl::narrow<int>(children_.size()));
    return *children_.at(row);
}

std::expected<int, QString> NodeHierarchical::childrenCount() const {
    return children_.size();
}

bool NodeHierarchical::canInsertChild(NodeType const) const {
    return false;
}

std::expected<Node *, QString> NodeHierarchical::insertChild(int row, std::unique_ptr<Node> &&node) {
    ZoneScoped;

    gsl_Expects(row >= 0);
    gsl_Expects(row <= gsl::narrow<int>(children_.size()));
    gsl_Expects(canInsertChild(node->type()));

    auto ptr = dynamicPtrCast<NodeHierarchical>(std::move(node));
    gsl_Expects(ptr);

    emit insertChildrenBegin(row, row);
    auto result = &**children_.emplace(children_.begin() + row, dynamicPtrCast<NodeSerializable>(std::move(ptr)));
    emit insertChildrenEnd(row, row);

    return result;
}

void NodeHierarchical::removeChildren(int const row, int const count) {
    // NOTE: This class implements removeChildren, but doesn't override canRemoveChildren, so it still returns false
    //       Override canRemoveChildren() in a subclass to enable children removal
    ZoneScoped;

    gsl_Expects(canRemoveChildren(row, count));
    gsl_Expects(row >= 0);
    gsl_Expects(count >= 0);
    gsl_Expects(row + count <= gsl::narrow<int>(children_.size()));

    auto begin = children_.begin() + row;
    auto end = begin + count;

    int last = row + count - 1;
    emit removeChildrenBegin(row, last);
    for (auto it = begin; it != end; ++it)
        (*it)->deinit();

    children_.erase(begin, end);
    emit removeChildrenEnd(row, last);
}

bool NodeHierarchical::canAcceptDrop() const {
    return true;
}

bool NodeHierarchical::canAcceptDrop(NodeType const type) const {
    return canInsertChild(type);
}

std::expected<void, QString> NodeHierarchical::saveNodeData(QCborMap &map) const {
    ZoneScoped;

    if (auto result = NodeSerializable::saveNodeData(map); !result)
        return std::unexpected(result.error());

    if (auto array = saveChildrenNodes(); !array)
        return std::unexpected(array.error());
    else if (*array)
        map[std::to_underlying(Format::NodeKey::Children)] = **array;

    return {};
}

std::expected<std::optional<QCborArray>, QString> NodeHierarchical::saveChildrenNodes() const {
    ZoneScoped;

    QCborArray children;

    for (int i = 0; i != childrenCount(); ++i) {
        // only save stored children
        if (auto child = childOfRow(i); !child) {
            return std::unexpected(child.error());
        } else if (auto storedChild = dynamic_cast<NodeSerializable *>(&child->get())) {
            if (auto childData = storedChild->save())
                children.append(*childData);
            else
                return std::unexpected(childData.error());
        }
    }

    return children;
}

std::expected<void, QString> NodeHierarchical::loadNodeData(QCborMap &map) {
    ZoneScoped;

    if (auto result = NodeSerializable::loadNodeData(map); !result)
        return std::unexpected(result.error());

    if (auto result = loadChildrenNodes(map); !result)
        return std::unexpected(result.error());

    return {};
}

std::expected<void, QString> NodeHierarchical::loadChildrenNodes(QCborMap &map) {
    ZoneScoped;

    auto children = map.take(std::to_underlying(Format::NodeKey::Children));
    if (children.isUndefined())
        return std::unexpected(QObject::tr("Node has no children key"));
    if (!children.isArray())
        return std::unexpected(QObject::tr("Node children is not an array but %1").arg(cborTypeToString(children.type())));

    auto childrenArray = children.toArray();
    children_.clear();
    children_.reserve(childrenArray.size());

    for (auto const &child: childrenArray) {
        if (auto childNode = NodeHierarchical::load(child, model(), this); !childNode)
            return std::unexpected(childNode.error());
        else
            children_.emplace_back(dynamicPtrCast<NodeSerializable>(std::move(*childNode)));
    }

    return {};
}
}
