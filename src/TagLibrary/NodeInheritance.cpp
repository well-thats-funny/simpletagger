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
#include "NodeInheritance.hpp"

#include "Model.hpp"
#include "NodeShadow.hpp"

namespace TagLibrary {
NodeInheritance::NodeInheritance(TagLibrary::Model &model, std::shared_ptr<TagLibrary::NodeSerializable> const &parent):
    NodeLink(model, parent) {}

NodeInheritance::~NodeInheritance() = default;

IconIdentifier NodeInheritance::genericIcon() {
    ZoneScoped;
    return IconIdentifier(":/icons/bx-up-arrow.svg");
}

std::vector<IconIdentifier> NodeInheritance::icons() const {
    ZoneScoped;
    if (!shadowRoot_)
        return {IconIdentifier(":/icons/bx-up-arrow-broken.svg")};
    else
        return shadowRoot_->icons();
}

NodeType NodeInheritance::type() const {
    return NodeType::Inheritance;
}

bool NodeInheritance::isReplaced() const {
    return true;
}

IconIdentifier NodeInheritance::linkingIcon() const {
    return IconIdentifier(":/icons/bx-up-arrow.svg");
}

void NodeInheritance::emitInsertChildrenBegin(int const count) {
    if (model().editMode()) {
        // if we're in edit mode, we behave like any link
        return NodeLink::emitInsertChildrenBegin(count);
    } else {
        // otherwise, we're invisible and our parent takes over our children
        if (auto first = parent()->rowOfChild(*this, true); !first)
            reportError("emitInsertChildrenBegin -> rowOfChild failed", first.error(), false);
        else
            emit std::dynamic_pointer_cast<NodeSerializable>(parent())->insertChildrenBegin(*first, *first + count - 1);
    }
}

void NodeInheritance::emitInsertChildrenEnd(int const count) {
    if (model().editMode()) {
        // if we're in edit mode, we behave like any link
        return NodeLink::emitInsertChildrenEnd(count);
    } else {
        // otherwise, we're invisible and our parent takes over our children
        if (auto first = parent()->rowOfChild(*this, true); !first)
            reportError("emitInsertChildrenEnd -> rowOfChild failed", first.error(), false);
        else
            emit std::dynamic_pointer_cast<NodeSerializable>(parent())->insertChildrenEnd(*first, *first + count - 1);
    }
}

void NodeInheritance::emitRemoveChildrenBegin(int const count) {
    if (model().editMode()) {
        // if we're in edit mode, we behave like any link
        return NodeLink::emitRemoveChildrenBegin(count);
    } else {
        // otherwise, we're invisible and our parent takes over our children
        if (auto first = parent()->rowOfChild(*this, true); !first)
            reportError("emitRemoveChildrenBegin -> rowOfChild failed", first.error(), false);
        else
            // TODO: get rid of const_cast
            emit std::dynamic_pointer_cast<NodeSerializable>(parent())->removeChildrenBegin(*first, *first + count - 1);
    }
}

void NodeInheritance::emitRemoveChildrenEnd(int const count) {
    if (model().editMode()) {
        // if we're in edit mode, we behave like any link
        return NodeLink::emitRemoveChildrenEnd(count);
    } else {
        // otherwise, we're invisible and our parent takes over our children
        if (auto first = parent()->rowOfChild(*this, true); !first)
            reportError("emitRemoveChildrenEnd -> rowOfChild failed", first.error(), false);
        else
            // TODO: get rid of const_cast
            emit std::dynamic_pointer_cast<NodeSerializable>(parent())->removeChildrenEnd(*first, *first + count - 1);
    }
}
}
