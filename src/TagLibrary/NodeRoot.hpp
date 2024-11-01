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
#include "NodeCollection.hpp"
#include "NodeSerializable.hpp"

namespace TagLibrary {
class NodeRoot: public NodeSerializable {
public:
    explicit NodeRoot(Model &model);
    ~NodeRoot();

    void deinit() override;

    // parent access
    [[nodiscard]] Node const *parent() const override;

    // children access
    [[nodiscard]] std::expected<int, QString> rowOfChild(Node const &node) const override;
    [[nodiscard]] std::expected<std::reference_wrapper<Node>, QString> childOfRow(int row) const override;
    [[nodiscard]] std::expected<int, QString> childrenCount() const override;


    [[nodiscard]] bool canInsertChild(NodeType childType) const override;
    [[nodiscard]] std::expected<Node *, QString> insertChild(int row, std::unique_ptr<Node> &&node) override;

    [[nodiscard]] NodeType type() const override;

protected:
    [[nodiscard]] std::expected<void, QString> saveNodeData(QCborMap &map) const override;
    [[nodiscard]] std::expected<void, QString> loadNodeData(QCborMap &map) override;

private:
    std::unique_ptr<NodeCollection> rootCollection_;
};
}
