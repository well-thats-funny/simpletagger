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
class NodeInheritance : public NodeHierarchical {
public:
    NodeInheritance(Model &model, NodeSerializable const *parent);
    ~NodeInheritance() override;

//    void deinit() override;

    [[nodiscard]] QString name(bool raw = false, bool editMode = false) const override;
    [[nodiscard]] bool canSetName() const override;
    [[nodiscard]] bool setName(QString const &name) override;

    [[nodiscard]] static IconIdentifier genericIcon();
    [[nodiscard]] std::vector<IconIdentifier> icons() const override;

    [[nodiscard]] std::optional<QUuid> linkTo() const override;
    [[nodiscard]] bool canLinkTo() const override;
    [[nodiscard]] std::expected<void, QString> setLinkTo(QUuid const &uuid) override;

    [[nodiscard]] QString comment() const override;
    [[nodiscard]] bool canSetComment() const override;
    [[nodiscard]] std::expected<void, QString> setComment(QString const &comment) override;

    [[nodiscard]] NodeType type() const override;

    [[nodiscard]] bool isReplaced() const override;
/*    [[nodiscard]] std::expected<int, Error> replacedCount() const override;
    std::expected<std::reference_wrapper<Node>, Error> replacedNode(int row) const override;*/

    [[nodiscard]] std::expected<std::reference_wrapper<Node>, QString> target() const;

protected:
    [[nodiscard]] std::expected<void, QString> saveNodeData(QCborMap &map) const override;
    [[nodiscard]] std::expected<void, QString> loadNodeData(QCborMap &map) override;

    QString name_;
    QUuid linkTo_;
    QString comment_;
};
}
