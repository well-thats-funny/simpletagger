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
#include "NodeHierarchical.hpp"

namespace TagLibrary {
class NodeLinkSubtree;

class NodeLink: public NodeHierarchical {
public:
    NodeLink(Model &model, NodeSerializable const *parent);
    ~NodeLink() override;

    void deinit() override;

    // children access
    [[nodiscard]] std::expected<int, QString> rowOfChild(const Node &node) const override;
    [[nodiscard]] std::expected<std::reference_wrapper<Node>, QString> childOfRow(int row) const override;
    [[nodiscard]] std::expected<int, QString> childrenCount() const override;

    // children modification
    [[nodiscard]] bool canBeDragged() const override;
    [[nodiscard]] std::expected<void, QString> afterDrop() override;

    [[nodiscard]] QString name(bool raw = false, bool editMode = false) const override;
    [[nodiscard]] bool canSetName() const override;
    [[nodiscard]] bool setName(QString const &name) override;

    [[nodiscard]] static IconIdentifier genericIcon();
    [[nodiscard]] std::vector<IconIdentifier> icons() const override;

    [[nodiscard]] std::optional<QUuid> linkTo() const override;
    [[nodiscard]] bool canLinkTo() const override;
    [[nodiscard]] std::expected<void, QString> setLinkTo(QUuid const &uuid) override;

    [[nodiscard]] std::vector<Tag> tags(TagFlags flags = TagFlag::IncludeResolved) const override;
    [[nodiscard]] QStringList resolveChildTag(QString const &tag) const override;

    [[nodiscard]] QString comment() const override;
    [[nodiscard]] bool canSetComment() const override;
    [[nodiscard]] std::expected<void, QString> setComment(QString const &comment) override;

    [[nodiscard]] std::optional<bool> active() const override;
    [[nodiscard]] bool canSetActive() const override;
    [[nodiscard]] std::expected<void, QString> setActive(bool) override;

    [[nodiscard]] std::optional<bool> highlighted() const override;
    [[nodiscard]] bool canSetHighlighted() const override;
    [[nodiscard]] std::expected<void, QString> setHighlighted(bool highlighted) override;

    [[nodiscard]] NodeType type() const override;

    [[nodiscard]] std::expected<std::reference_wrapper<Node>, QString> target() const;

protected:
    [[nodiscard]] std::expected<void, QString> saveNodeData(QCborMap &map) const override;
    [[nodiscard]] std::expected<std::optional<QCborArray>, QString> saveChildrenNodes() const override;
    [[nodiscard]] std::expected<void, QString> loadNodeData(QCborMap &map) override;
    [[nodiscard]] std::expected<void, QString> loadChildrenNodes(QCborMap &map) override;
    void resolveLink();

private:
    QString name_;
    QUuid linkTo_;
    QString comment_;
    bool active_ = false;
    std::unique_ptr<NodeLinkSubtree> linkSubtreeRoot_;
    QMetaObject::Connection subtreeRootAboutToRemoveConnection;
};
}
