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
#include "NodeLinkSubtree.hpp"

#include "Logging.hpp"
#include "Model.hpp"
#include "../Utility.hpp"

namespace TagLibrary {
NodeLinkSubtree::NodeLinkSubtree(Model &model, Node const *parent, Node const &target, Node *subtreeRootOwner):
    Node(model), parent_(parent), target_(target), subtreeRootOwner_(subtreeRootOwner) {
}

NodeLinkSubtree::~NodeLinkSubtree() = default;

std::expected<void, QString> NodeLinkSubtree::init() {
    ZoneScoped;

    connect(&target_, &Node::dataChanged, this, [this]{
        ZoneScoped;
        icons_.reset();
        if (subtreeRootOwner_)
            emit subtreeRootOwner_->dataChanged();
        else
            emit dataChanged();
    });

    connect(&target_, &Node::aboutToRemove, this, [this]{
        ZoneScoped;
        // NOTE: in contrast to other signals, which forward do subtreeRootOwner (if set), this signal does not.
        //       the reason is, in contrast to other operations, removal of the link's target shouldn't lead to
        //       the removal of the link itself
        emit targetAboutToRemove();
    });

    connect(&target_, &Node::insertChildrenEnd, this, [this](int const first, int const last){
        ZoneScoped;

        // we can only do our insertions after the target has completed its
        if (subtreeRootOwner_)
                emit subtreeRootOwner_->insertChildrenBegin(first, last);
        else
                emit insertChildrenBegin(first, last);
        // TODO[C++23]
        /*auto &&rg = std::views::repeat(nullptr)
                | std::views::take(last - first + 1);
        children_.insert_range(children_.begin() + first, rg);*/
        for (int row = first; row <= last; ++row) {
            if (auto child = createChild(row); !child)
                qCCritical(LoggingCategory) << "Could not create a children in a linking subtree; subtree will get rendered inconsitently:" << child.error();
            else
                children_.emplace(children_.begin() + row, dynamicPtrCast<NodeLinkSubtree>(std::move(*child)));
        }

        if (subtreeRootOwner_)
            emit subtreeRootOwner_->insertChildrenEnd(first, last);
        else
            emit insertChildrenEnd(first, last);
    });

    connect(&target_, &Node::removeChildrenBegin, this, [this](int const first, int const last) {
        ZoneScoped;

        // we must do our removals before the target has completed its
        if (subtreeRootOwner_)
            emit subtreeRootOwner_->removeChildrenBegin(first, last);
        else
            emit removeChildrenBegin(first, last);

        for (auto i = first; i <= last; ++i)
            children_[i]->deinit();
        children_.erase(children_.begin() + first, children_.begin() + last + 1);

        if (subtreeRootOwner_)
            emit subtreeRootOwner_->removeChildrenEnd(first, last);
        else
            emit removeChildrenEnd(first, last);
    });

    auto count = target_.childrenCount();
    if (!count)
        return std::unexpected(count.error());

    children_.reserve(*count);
    for (int i = 0; i != *count; ++i) {
        if (auto result = createChild(i); !result)
            return std::unexpected(result.error());
        else
            children_.emplace_back(dynamicPtrCast<NodeLinkSubtree>(std::move(*result)));
    }

    return {};
};

void NodeLinkSubtree::deinit() {
    ZoneScoped;

    Node::deinit();

    for (auto &child: children_)
        child->deinit();
}

Node const *NodeLinkSubtree::parent() const {
    ZoneScoped;

    if (auto nlsParent = dynamic_cast<NodeLinkSubtree const *>(parent_))
        if (nlsParent->subtreeRootOwner_)
            return nlsParent->subtreeRootOwner_;

    return parent_;
}

std::expected<int, QString> NodeLinkSubtree::rowOfChild(Node const &node) const {
    ZoneScoped;

    auto it = std::ranges::find_if(
            children_,
            [&](auto const &child){ return &*child == &node; }
    );
    if (it == children_.end())
        return std::unexpected(QObject::tr("Node %1 not found among the children of linked subtree node %2").arg(
                node.path(PathFlag::IncludeEverything), path(PathFlag::IncludeEverything)
        ));

    return it - children_.begin();
}

std::expected<std::reference_wrapper<Node>, QString> NodeLinkSubtree::childOfRow(int const row) const {
    ZoneScoped;
    auto &child = children_.at(row);
    gsl_Ensures(child);
    return *child;
}

std::expected<int, QString> NodeLinkSubtree::childrenCount() const {
    ZoneScoped;
    return children_.size();
}

QString NodeLinkSubtree::name(bool const raw, bool const editMode) const {
    ZoneScoped;
    return target_.name(raw, editMode);
}

std::vector<IconIdentifier> NodeLinkSubtree::icons() const {
    ZoneScoped;

    if (!icons_) {
        icons_.emplace({IconIdentifier(":/icons/bx-link.svg")});

        auto targetIcons = target_.icons();
        icons_->reserve(icons_->size() + targetIcons.size());
        std::ranges::move(targetIcons, std::back_inserter(*icons_));
    }
    return *icons_;
}

std::optional<QUuid> NodeLinkSubtree::linkTo() const {
    ZoneScoped;
    return target_.linkTo();
}

bool NodeLinkSubtree::isLinkingNode() const {
    return true;
}

QUuid NodeLinkSubtree::uuid() const {
    ZoneScoped;
    return target_.uuid();
}

std::vector<Node::Tag> NodeLinkSubtree::tags(TagFlags const flags) const {
    ZoneScoped;

    std::vector<Tag> result;

    auto tags = target_.tags(flags);

    if (!(flags & TagFlag::IncludeResolved)) {
        std::ranges::move(tags, std::back_inserter(result));
    } else {
        // TODO: this is the same like in NodeLink::tags -> helper?
        for (auto const &tag: tags)
            for (auto const &resolved: parent()->resolveChildTag(tag.resolved))
                result.emplace_back(tag.raw, resolved);
    }

    return result;
}

QStringList NodeLinkSubtree::resolveChildTag(QString const &tag) const {
    ZoneScoped;
    if (subtreeRootOwner_)
        return subtreeRootOwner_->resolveChildTag(tag);
    else
        return parent()->resolveChildTag(tag);
}

QString NodeLinkSubtree::comment() const {
    ZoneScoped;
    return target_.comment();
}

std::optional<bool> NodeLinkSubtree::active() const {
    return active_;
}

bool NodeLinkSubtree::canSetActive() const {
    return !tags().empty();
}

std::expected<void, QString> NodeLinkSubtree::setActive(bool const active) {
    ZoneScoped;

    active_ = active;

    if (subtreeRootOwner_) {
        emit subtreeRootOwner_->activeChanged(active);
        emit subtreeRootOwner_->dataChanged();
    } else {
        emit activeChanged(active);
        emit dataChanged();
    }

    return {};
}

std::optional<bool> NodeLinkSubtree::highlighted() const {
    return highlighted_;
}

bool NodeLinkSubtree::canSetHighlighted() const {
    return true;
}

std::expected<void, QString> NodeLinkSubtree::setHighlighted(bool const highlighted) {
    ZoneScoped;
    if (highlighted != highlighted_) {
        highlighted_ = highlighted;
        emit dataChanged();
    }
    return {};
}

NodeType NodeLinkSubtree::type() const {
    ZoneScoped;
    return target_.type();
}

bool NodeLinkSubtree::isVirtual() const {
    return true;
}

std::vector<QBrush> NodeLinkSubtree::background(bool const editMode) const {
    ZoneScoped;
    auto result = Node::background(editMode);
    std::ranges::move(target_.background(false), std::back_inserter(result));
    if (editMode)
        result.emplace_back(QBrush(QColor(0, 0, 0, 32), Qt::BrushStyle::SolidPattern));
    return result;
}

[[nodiscard]] std::expected<std::unique_ptr<NodeLinkSubtree>, QString> NodeLinkSubtree::createChild(int const row) {
    ZoneScoped;

    auto targetChildNode = target_.childOfRow(row);
    if (!targetChildNode)
        return std::unexpected(targetChildNode.error());

    auto linkChild = std::make_unique<NodeLinkSubtree>(model(), this, targetChildNode->get(), nullptr);
    if (auto result = linkChild->init(); !result)
        return std::unexpected(result.error());

    return linkChild;
}
}