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
class NodeShadow;

class NodeLink: public NodeHierarchical {
public:
    NodeLink(Model &model, std::shared_ptr<NodeSerializable> const &parent);
    ~NodeLink() override;

    void deinit() override;

    // children access
    [[nodiscard]] std::expected<int, QString> rowOfChild(const Node &node, bool replaceReplaced) const override;
    [[nodiscard]] std::expected<std::shared_ptr<Node>, QString> childOfRow(int row, bool replaceReplaced) const override;
    [[nodiscard]] std::expected<int, QString> childrenCount(bool replaceReplaced) const override;

    // children modification
    [[nodiscard]] bool canBeDragged() const override;

    [[nodiscard]] QString name(bool raw = false, bool editMode = false) const override;
    [[nodiscard]] bool canSetName() const override;
    [[nodiscard]] bool setName(QString const &name) override;

    [[nodiscard]] static IconIdentifier genericIcon();
    [[nodiscard]] std::vector<IconIdentifier> icons() const override;

    [[nodiscard]] std::optional<QUuid> linkTo() const override;
    [[nodiscard]] bool canLinkTo() const override;
    [[nodiscard]] std::expected<void, QString> setLinkTo(QUuid const &uuid) override;

    [[nodiscard]] std::vector<Tag> generateTags(TagFlags flags = TagFlag::IncludeResolved) const override;
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

    [[nodiscard]] std::expected<std::shared_ptr<Node>, QString> target() const;

protected:
    [[nodiscard]] std::expected<void, QString> saveNodeData(QCborMap &map) const override;
    [[nodiscard]] std::expected<std::optional<QCborArray>, QString> saveChildrenNodes() const override;
    [[nodiscard]] std::expected<void, QString> loadNodeData(QCborMap &map) override;
    [[nodiscard]] std::expected<void, QString> loadChildrenNodes(QCborMap &map) override;

    [[nodiscard]] virtual IconIdentifier linkingIcon() const;
    virtual void emitInsertChildrenBegin(int count);
    virtual void emitInsertChildrenEnd(int count);
    virtual void emitRemoveChildrenBegin(int count);
    virtual void emitRemoveChildrenEnd(int count);

public:
    [[nodiscard]] std::expected<void, Error> populateShadowsImpl() override;
    [[nodiscard]] std::expected<void, Error> unpopulateShadowsImpl() override;

    [[nodiscard]] std::expected<void, QString> repopulateShadows(RepopulationRequest const &repopulationRequest) override;

private:
    QString name_;
    QUuid linkTo_;
    QString comment_;
    bool active_ = false;
protected:
    std::shared_ptr<NodeShadow> shadowRoot_;
};
}
