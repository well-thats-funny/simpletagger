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

class NodeShadow: public Node {
    Q_OBJECT

#ifndef NDEBUG
    friend class Node;
#endif

public:
    NodeShadow(
            Model &model,
            std::shared_ptr<Node> const &parent,
            std::shared_ptr<Node> const &target,
            std::shared_ptr<Node> const &owner,
            IconIdentifier const &linkingIcon
    );
    ~NodeShadow();

    [[nodiscard]] std::expected<void, QString> init() override;
    void deinit() override;

    // parent access
    [[nodiscard]] std::shared_ptr<Node> parent() const override;

    // children access
    [[nodiscard]] std::expected<int, QString> rowOfChild(Node const &node, bool replaceReplaced) const override;
    [[nodiscard]] std::expected<std::shared_ptr<Node>, QString> childOfRow(int row, bool replaceReplaced) const override;
    [[nodiscard]] std::expected<int, QString> childrenCount(bool replaceReplaced) const override;

    // fields
    [[nodiscard]] QString name(bool raw = false, bool editMode = false) const override;
    [[nodiscard]] std::vector<IconIdentifier> icons() const override;
    [[nodiscard]] std::optional<QUuid> linkTo() const override;

    [[nodiscard]] std::vector<Tag> generateTags(TagFlags flags = TagFlag::IncludeResolved) const override;
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

    [[nodiscard]] bool isHidden() const override;

    [[nodiscard]] std::optional<int> lastChangeVersion() const override;

    [[nodiscard]] std::vector<QBrush> background(bool editMode) const override;

    [[nodiscard]] std::expected<void, Error> populateShadowsImpl() override;
    [[nodiscard]] std::expected<void, Error> unpopulateShadowsImpl() override;

    [[nodiscard]] std::expected<void, QString> repopulateShadows(RepopulationRequest const &repopulationRequest = {}) override;

signals:
    void targetAboutToRemove();

private:
    [[nodiscard]] std::expected<std::shared_ptr<NodeShadow>, QString> createChild(int row);
    [[nodiscard]] std::shared_ptr<Node> target() const;

    std::weak_ptr<Node> const parent_;
    std::weak_ptr<Node> const target_;
    std::weak_ptr<Node> const owner_;
    std::vector<std::shared_ptr<NodeShadow>> children_;
    mutable std::optional<std::vector<IconIdentifier>> icons_;
    bool active_ = false;
    bool highlighted_ = false;
    IconIdentifier linkingIcon_;
};
}
