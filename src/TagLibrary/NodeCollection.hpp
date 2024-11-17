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
class NodeCollection: public NodeHierarchical {
public:
    NodeCollection(Model &model, std::shared_ptr<NodeSerializable> const &parent);
    ~NodeCollection() override;

    [[nodiscard]] bool canInsertChild(NodeType type) const override;
    [[nodiscard]] bool canRemoveChildren(int row, int count) const override;
    [[nodiscard]] bool canBeDragged() const override;

    [[nodiscard]] QString name(bool raw = false, bool editMode = false) const override;
    [[nodiscard]] bool canSetName() const override;
    bool setName(QString const &name) override;

    [[nodiscard]] QString comment() const override;
    [[nodiscard]] bool canSetComment() const override;
    [[nodiscard]] std::expected<void, QString> setComment(const QString &comment) override;

    [[nodiscard]] static IconIdentifier genericIcon();
    [[nodiscard]] std::vector<IconIdentifier> icons() const override;

    [[nodiscard]] NodeType type() const override;

protected:
    [[nodiscard]] std::expected<void, QString> saveNodeData(QCborMap &map) const override;
    [[nodiscard]] std::expected<void, QString> loadNodeData(QCborMap &map, bool allowDuplicatedUuids) override;

private:
    QString name_;
    QString comment_;
};
}
