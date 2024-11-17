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
#include "NodeRoot.hpp"
#include "NodeCollection.hpp"

#include "../Utility.hpp"

namespace TagLibrary {
NodeRoot::NodeRoot(Model &model): NodeSerializable(model) {}

NodeRoot::~NodeRoot() = default;

void NodeRoot::deinit() {
    ZoneScoped;
    NodeSerializable::deinit();

    if (rootCollection_)
        rootCollection_->deinit();
}

Node const *NodeRoot::parent() const {
    return nullptr;
}

std::expected<int, QString> NodeRoot::rowOfChild(Node const &node, bool const) const {
    ZoneScoped;

    if (&node == dynamic_cast<Node *>(&*rootCollection_))
        return 0;
    else
        return std::unexpected(QObject::tr("Node %1 (%2) is not the root collection node of the root node %3 (%4)").arg(
                node.uuid().toString(QUuid::WithoutBraces), node.name(true),
                uuid().toString(QUuid::WithoutBraces), name(true)
        ));
}

std::expected<std::reference_wrapper<Node>, QString> NodeRoot::childOfRow(int const row, bool const) const {
    gsl_Expects(row == 0);
    return *rootCollection_;
}

std::expected<int, QString> NodeRoot::childrenCount(bool const) const {
    return rootCollection_ ? 1 : 0;
}

bool NodeRoot::canInsertChild(NodeType const childType) const {
    return !rootCollection_ && childType == NodeType::Collection;
}

std::expected<Node *, QString> NodeRoot::insertChild(int const row, std::unique_ptr<Node> &&node) {
    ZoneScoped;

    gsl_Expects(row == 0);
    gsl_Ensures(node);
    gsl_Expects(!rootCollection_);

    auto ptr = dynamicPtrCast<NodeCollection>(std::move(node));
    gsl_Expects(ptr);

    emit insertChildrenBegin(row, row);
    rootCollection_ = std::move(ptr);
    emit insertChildrenEnd(row, row);

    gsl_Ensures(!node);
    gsl_Ensures(rootCollection_);
    return &*rootCollection_;
}

NodeType NodeRoot::type() const {
    return NodeType::Root;
}

std::expected<void, QString> NodeRoot::repopulateLinked(RepopulationRequest const &repopulationRequest) {
    if (rootCollection_)
        return rootCollection_->repopulateLinked(repopulationRequest);
    else
        return {};
}

std::expected<void, QString> NodeRoot::saveNodeData(QCborMap &map) const {
    ZoneScoped;

    if (auto result = NodeSerializable::saveNodeData(map); !result)
        return std::unexpected(result.error());

    QCborArray children;

    if (rootCollection_) {
        if (auto childData = rootCollection_->save())
            children.append(*childData);
        else
            return std::unexpected(childData.error());
    }

    map[std::to_underlying(Format::NodeKey::Children)] = children;
    return {};
}

std::expected<void, QString> NodeRoot::loadNodeData(QCborMap &map) {
    ZoneScoped;

    if (auto result = NodeSerializable::loadNodeData(map); !result)
        return std::unexpected(result.error());

    auto children = map.take(std::to_underlying(Format::NodeKey::Children));
    if (children.isUndefined())
        return std::unexpected(QObject::tr("Node has no children key"));
    if (!children.isArray())
        return std::unexpected(QObject::tr("Node children is not an array but %1").arg(cborTypeToString(children.type())));

    auto childrenArray = children.toArray();
    if (childrenArray.empty()) {
        rootCollection_.reset();
    } else {
        for (auto const &child: childrenArray) {
            if (auto childNode = NodeHierarchical::load(child, model(), this); !childNode)
                return std::unexpected(childNode.error());
            else
                if (auto childCollection = dynamicPtrCast<NodeCollection>(std::move(*childNode)); !childCollection)
                    return std::unexpected(QObject::tr("Root collection should be NodeCollection, not %1").arg(typeid(*childNode).name()));
                else
                    rootCollection_ = std::move(childCollection);
        }
    }

    return {};
}
}