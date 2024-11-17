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
NodeHierarchical::NodeHierarchical(Model &model, std::shared_ptr<NodeSerializable> const &parent): NodeSerializable(model), parent_(parent) {}

NodeHierarchical::~NodeHierarchical() = default;

void NodeHierarchical::deinit() {
    ZoneScoped;

    NodeSerializable::deinit();

    for (auto &child: children_)
        child->deinit();
}

std::shared_ptr<Node> NodeHierarchical::parent() const {
    auto parent = parent_.lock();
    assert(parent);
    return parent;
}

bool NodeHierarchical::canRemove() const {
    ZoneScoped;
    auto row = parent()->rowOfChild(*this, false);
    gsl_Expects(row);
    return parent()->canRemoveChildren(*row, *row);
}

std::expected<int, QString> NodeHierarchical::rowOfChild(Node const &node, bool const replaceReplaced) const {
    ZoneScoped;

    if (!replaceReplaced) {
        auto it = std::ranges::find_if(
                children_,
                [&](auto const &child) { return &*child == &node; }
        );
        if (it == children_.end())
            return std::unexpected(QObject::tr("Node %1 (%2) not found among the children of node %3 (%4)").arg(
                    node.uuid().toString(QUuid::WithoutBraces), node.name(true),
                    uuid().toString(QUuid::WithoutBraces), name(true)
            ));
        return it - children_.begin();
    } else {
        int row = 0;

        for (auto const &child: children_) {
            // NOTE: This check purposedly takes place before the child->isReplaced() check.
            // This is to allow passing a replaced node as a "node" parameter. In this case
            // the function returns the row where the first replacement would be displayed.
            // This is because NodeInheritance calls this function to find out where its
            // replacement nodes should be placed.
            if (&*child == &node)
                return row;

            if (!child->isReplaced()) {
                ++row;
            } else {
                if (auto replacedCount = child->childrenCount(true); !replacedCount)
                    return std::unexpected(replacedCount.error());
                else {
                    for (int replacedRow = 0; replacedRow != *replacedCount; ++replacedRow) {
                        if (auto replacedChild = child->childOfRow(replacedRow, true); !replacedChild) {
                            return std::unexpected(replacedChild.error());
                        } else {
                            if (&**replacedChild == &node)
                                return row;
                        }
                        ++row;
                    }
                }
            }
        }
        return std::unexpected("rowOfChild: child not found");
    }
}

std::expected<std::shared_ptr<Node>, QString> NodeHierarchical::childOfRow(int const row, bool const replaceReplaced) const {
    ZoneScoped;
    gsl_Expects(row >= 0);

    if (!replaceReplaced) {
        gsl_Expects(row < gsl::narrow<int>(children_.size()));
        return children_.at(row);
    } else {
        int childRow = 0;

        for (auto const &child: children_) {
            if (!child->isReplaced()) {
                if (childRow == row)
                    return child;

                childRow++;
            } else {
                if (auto replacedCount = child->childrenCount(true); !replacedCount) {
                    return std::unexpected(replacedCount.error());
                } else {
                    if (childRow + *replacedCount > row)
                        return child->childOfRow(row - childRow, true);

                    childRow += *replacedCount;
                }
            }
        }

        return std::unexpected(QString("childOfRow: row not found %1").arg(row));
    }
}

std::expected<int, QString> NodeHierarchical::childrenCount(bool const replaceReplaced) const {
    ZoneScoped;
    if (!replaceReplaced) {
        return children_.size();
    } else {
        int count = 0;

        for (auto const &child: children_) {
            if (!child->isReplaced()) {
                count++;
            } else {
                if (auto replacedCount = child->childrenCount(true); !replacedCount) {
                    return std::unexpected(replacedCount.error());
                } else {
                    count += *replacedCount;
                }
            }
        }

        return count;
    }
}

bool NodeHierarchical::canInsertChild(NodeType const) const {
    return false;
}

std::expected<std::shared_ptr<Node>, QString> NodeHierarchical::insertChild(int const row, std::shared_ptr<Node> &&node) {
    ZoneScoped;

    gsl_Expects(row >= 0);
    gsl_Expects(row <= gsl::narrow<int>(children_.size()));
    gsl_Expects(canInsertChild(node->type()));

    auto ptr = std::dynamic_pointer_cast<NodeHierarchical>(std::move(node));
    gsl_Expects(ptr);

    emit insertChildrenBegin(row, row);
    auto result = *children_.emplace(children_.begin() + row, std::move(ptr));
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
    emit beforeRemoveChildren(row, last);

    for (auto it = begin; it != end; ++it)
        (*it)->deinit();

    emit removeChildrenBegin(row, last);
    children_.erase(begin, end);
    emit removeChildrenEnd(row, last);
}

bool NodeHierarchical::canAcceptDrop() const {
    return true;
}

bool NodeHierarchical::canAcceptDrop(NodeType const type) const {
    return canInsertChild(type);
}

bool NodeHierarchical::canPopulate() const {
    return std::ranges::all_of(children_, [](auto const &n){ return n->canPopulate(); });
}

bool NodeHierarchical::canUnpopulate() const {
    return std::ranges::all_of(children_, [](auto const &n){ return n->canUnpopulate(); });
}

std::expected<void, Error> NodeHierarchical::populateShadowsImpl() {
    for (auto &child: children_)
        if (auto result = child->populateShadowsImpl(); !result)
            return result;
    return {};
}

std::expected<void, Error> NodeHierarchical::unpopulateShadowsImpl() {
    for (auto &child: children_)
        if (auto result = child->unpopulateShadowsImpl(); !result)
            return result;
    return {};
}

std::expected<void, QString> NodeHierarchical::repopulateShadows(RepopulationRequest const &repopulationRequest) {
    for (auto &child: children_)
        if (auto result = child->repopulateShadows(repopulationRequest); !result)
            return result;
    return {};
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

    for (int i = 0; i != childrenCount(false); ++i) {
        // only save stored children
        if (auto child = childOfRow(i, false); !child) {
            return std::unexpected(child.error());
        } else if (auto serializableChild = std::dynamic_pointer_cast<NodeSerializable>(*child)) {
            if (auto childData = serializableChild->save())
                children.append(*childData);
            else
                return std::unexpected(childData.error());
        }
    }

    return children;
}

std::expected<void, QString> NodeHierarchical::loadNodeData(QCborMap &map, bool const allowDuplicatedUuids) {
    ZoneScoped;

    if (auto result = NodeSerializable::loadNodeData(map, allowDuplicatedUuids); !result)
        return std::unexpected(result.error());

    if (auto result = loadChildrenNodes(map, allowDuplicatedUuids); !result)
        return std::unexpected(result.error());

    return {};
}

std::expected<void, QString> NodeHierarchical::loadChildrenNodes(QCborMap &map, const bool allowDuplicatedUuids) {
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
        if (auto childNode = NodeHierarchical::load(
                child, model(), std::dynamic_pointer_cast<NodeHierarchical>(shared_from_this()), allowDuplicatedUuids
        ); !childNode)
            return std::unexpected(childNode.error());
        else
            children_.emplace_back(std::dynamic_pointer_cast<NodeSerializable>(std::move(*childNode)));
    }

    return {};
}
}
