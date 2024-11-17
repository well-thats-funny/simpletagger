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
class NodeObject: public NodeHierarchical {
public:
    NodeObject(Model &model, std::shared_ptr<NodeSerializable> const &parent);
    ~NodeObject() override;

    [[nodiscard]] bool canInsertChild(NodeType type) const override;
    [[nodiscard]] bool canRemoveChildren(int row, int count) const override;
    [[nodiscard]] bool canBeDragged() const override;

    [[nodiscard]] QString name(bool raw = false, bool editMode = false) const override;
    [[nodiscard]] bool canSetName() const override;
    [[nodiscard]] bool setName(const QString &name) override;

    [[nodiscard]] static IconIdentifier genericIcon();
    [[nodiscard]] std::vector<IconIdentifier> icons() const override;
    [[nodiscard]] bool canSetIcon() const override;
    [[nodiscard]] bool setIcon(const QString &path) override;

    [[nodiscard]] std::vector<Tag> generateTags(TagFlags flags = TagFlag::IncludeResolved) const override;
    [[nodiscard]] bool canSetTags() const override;
    [[nodiscard]] std::expected<void, QString> setTags(QStringList const &tags) override;
    [[nodiscard]] QStringList resolveChildTag(QString const &tag) const override;

    [[nodiscard]] QString comment() const override;
    [[nodiscard]] bool canSetComment() const override;
    [[nodiscard]] std::expected<void, QString> setComment(QString const &comment) override;

    [[nodiscard]] std::optional<bool> active() const override;
    [[nodiscard]] bool canSetActive() const override;
    [[nodiscard]] std::expected<void, QString> setActive(bool active) override;

    [[nodiscard]] std::optional<bool> highlighted() const override;
    [[nodiscard]] bool canSetHighlighted() const override;
    [[nodiscard]] std::expected<void, QString> setHighlighted(bool highlighted) override;

    [[nodiscard]] bool canBeLinkedTo() const override;

    [[nodiscard]] NodeType type() const override;

protected:
    [[nodiscard]] std::expected<void, QString> saveNodeData(QCborMap &map) const override;
    [[nodiscard]] std::expected<void, QString> loadNodeData(QCborMap &map, bool allowDuplicatedUuids) override;

private:
    QString name_;
    QString icon_;
    QStringList tags_;
    QString comment_;
    bool active_ = false;
    bool highlighted_ = false;
};
}
