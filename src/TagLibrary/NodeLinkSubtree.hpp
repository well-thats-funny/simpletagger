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
#include "Node.hpp"

namespace TagLibrary {
class Model;

class NodeLinkSubtree: public Node {
    Q_OBJECT

public:
    NodeLinkSubtree(Model &model, Node const *parent, Node const &target, Node *subtreeRootOwner);
    ~NodeLinkSubtree();

    [[nodiscard]] std::expected<void, QString> init() override;
    void deinit() override;

    // parent access
    [[nodiscard]] Node const *parent() const override;

    // children access
    [[nodiscard]] std::expected<int, QString> rowOfChild(Node const &node) const override;
    [[nodiscard]] std::expected<std::reference_wrapper<Node>, QString> childOfRow(int row) const override;
    [[nodiscard]] std::expected<int, QString> childrenCount() const override;

    // fields
    [[nodiscard]] QString name(bool raw = false, bool editMode = false) const override;
    [[nodiscard]] std::vector<IconIdentifier> icons() const override;
    [[nodiscard]] std::optional<QUuid> linkTo() const override;

    [[nodiscard]] std::vector<Tag> tags(TagFlags flags = TagFlag::IncludeResolved) const override;
    [[nodiscard]] QStringList resolveChildTag(QString const &tag) const override;

    [[nodiscard]] QString comment() const override;

    [[nodiscard]] std::optional<bool> active() const override;
    [[nodiscard]] bool canSetActive() const override;
    [[nodiscard]] std::expected<void, QString> setActive(bool) override;

    [[nodiscard]] std::optional<bool> highlighted() const override;
    [[nodiscard]] bool canSetHighlighted() const override;
    [[nodiscard]] std::expected<void, QString> setHighlighted(bool highlighted) override;

    [[nodiscard]] bool isLinkingNode() const override;
    [[nodiscard]] QUuid uuid() const override;

    [[nodiscard]] NodeType type() const override;
    [[nodiscard]] bool isVirtual() const override;

    [[nodiscard]] std::vector<QBrush> background(bool editMode) const override;

signals:
    void targetAboutToRemove();

private:
    [[nodiscard]] std::expected<std::unique_ptr<NodeLinkSubtree>, QString> createChild(int row);

    Node const *parent_ = nullptr;
    Node const & target_;
    Node *subtreeRootOwner_ = nullptr;
    std::vector<std::unique_ptr<NodeLinkSubtree>> children_;
    mutable std::optional<std::vector<IconIdentifier>> icons_;
    bool active_ = false;
    bool highlighted_ = false;
};
}
