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
#include "NodeSerializable.hpp"

namespace TagLibrary {
class NodeHierarchical: public NodeSerializable {
public:
    NodeHierarchical(NodeHierarchical const &other) = delete;
    NodeHierarchical(NodeHierarchical &&other) = delete;
    NodeHierarchical& operator=(NodeHierarchical const &other) = delete;
    NodeHierarchical& operator=(NodeHierarchical &&other) = delete;

    NodeHierarchical(Model &model, std::shared_ptr<NodeSerializable> const &parent);
    virtual ~NodeHierarchical();

    void deinit() override;

    // parent access
    [[nodiscard]] std::shared_ptr<Node> parent() const override;

    // parent modification
    [[nodiscard]] bool canRemove() const override;

    // children access
    [[nodiscard]] std::expected<int, QString> rowOfChild(Node const &node, bool replaceReplaced) const override;
    [[nodiscard]] std::expected<std::shared_ptr<Node>, QString> childOfRow(int row, bool replaceReplaced) const override;
    [[nodiscard]] std::expected<int, QString> childrenCount(bool replaceReplaced) const override;

    // children modification
    [[nodiscard]] bool canInsertChild(NodeType childType) const override;
    [[nodiscard]] std::expected<std::shared_ptr<Node>, QString> insertChild(int row, std::shared_ptr<Node> &&node) override;
    void removeChildren(int row, int count) override;
    [[nodiscard]] bool canAcceptDrop() const override; // does it accept drops at all?
    [[nodiscard]] bool canAcceptDrop(NodeType type) const override; // does it accept drops of this node type?

    [[nodiscard]] bool canPopulate() const override;
    [[nodiscard]] bool canUnpopulate() const override;

    [[nodiscard]] std::expected<void, Error> populateShadowsImpl() override;
    [[nodiscard]] std::expected<void, Error> unpopulateShadowsImpl() override;

    [[nodiscard]] std::expected<void, QString> repopulateShadows(RepopulationRequest const &repopulationRequest = {}) override;

protected:
    [[nodiscard]] std::expected<void, QString> saveNodeData(QCborMap &map) const override;
    [[nodiscard]] virtual std::expected<std::optional<QCborArray>, QString> saveChildrenNodes() const;
    [[nodiscard]] std::expected<void, QString> loadNodeData(QCborMap &map, bool allowDuplicatedUuids) override;
    [[nodiscard]] virtual std::expected<void, QString> loadChildrenNodes(QCborMap &map, bool allowDuplicatedUuids);

    friend class Model;

    std::weak_ptr<NodeSerializable> const parent_;
    std::vector<std::shared_ptr<NodeSerializable>> children_;
};
}