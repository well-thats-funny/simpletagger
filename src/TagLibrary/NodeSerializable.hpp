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
class NodeSerializable: public Node {
public:
    [[nodiscard]] static std::expected<std::shared_ptr<NodeSerializable>, QString>
    createNode(NodeType type, Model &model, std::shared_ptr<NodeSerializable> const &parent);
    [[nodiscard]] static IconIdentifier genericIcon(NodeType type);

    explicit NodeSerializable(Model &model);
    ~NodeSerializable();

    // children modification
    [[nodiscard]] QString generateUnusedChildName(QString const &prefix) const;

    // fields
    [[nodiscard]] bool isLinkingNode() const override;

    [[nodiscard]] QUuid uuid() const;

    [[nodiscard]] bool isHidden() const override;
    [[nodiscard]] bool canSetHidden() const override;
    [[nodiscard]] std::expected<void, QString> setHidden(bool hidden) override;

    [[nodiscard]] std::optional<int> lastChangeVersion() const override;
    void setLastChangeVersion(int version) override;

    // persistence
    [[nodiscard]] std::expected<QCborValue, QString> save() const;
    [[nodiscard]] static std::expected<std::shared_ptr<NodeSerializable>, QString>
    load(QCborValue const &value, Model &model, std::shared_ptr<NodeSerializable> const &parent);

protected:
    [[nodiscard]] virtual std::expected<void, QString> saveNodeData(QCborMap &map) const;
    [[nodiscard]] virtual std::expected<void, QString> loadNodeData(QCborMap &map);

private:
    QUuid uuid_;
    bool hidden_ = false;
    std::optional<int> lastChangeVersion_;
};
}