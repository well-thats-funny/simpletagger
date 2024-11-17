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
#include "NodeLink.hpp"

namespace TagLibrary {
class NodeLinkSubtree;

class NodeInheritance : public NodeLink {
public:
    NodeInheritance(Model &model, NodeSerializable const *parent);
    ~NodeInheritance() override;

    [[nodiscard]] static IconIdentifier genericIcon();
    [[nodiscard]] std::vector<IconIdentifier> icons() const override;

    [[nodiscard]] NodeType type() const override;

    [[nodiscard]] bool isReplaced() const override;

protected:
    void emitInsertChildrenBegin(int count) override;
    void emitInsertChildrenEnd(int count) override;
    void emitRemoveChildrenBegin(int count) override;
    void emitRemoveChildrenEnd(int count) override;
};
}
