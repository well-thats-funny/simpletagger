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
#include "NodeShadow.hpp"

#include "Logging.hpp"
#include "Model.hpp"
#include "../Utility.hpp"

namespace TagLibrary {
NodeShadow::NodeShadow(
        Model &model,
        std::shared_ptr<Node> const &parent,
        std::shared_ptr<Node> const &target,
        std::shared_ptr<Node> const &owner,
        IconIdentifier const &linkingIcon
):
    Node(model), parent_(parent), target_(target), targetUuid_(target->uuid()), owner_(owner), linkingIcon_(linkingIcon) {}

NodeShadow::~NodeShadow() = default;

std::expected<void, QString> NodeShadow::init() {
    ZoneScoped;

    if (auto result = Node::init(); !result)
        return std::unexpected(result.error());

    auto target = target_.lock();
    assert(target);

    connect(&*target, &Node::dataChanged, this, [this]{
        ZoneScoped;
        icons_.reset();
        if (auto owner = owner_.lock())
            emit owner->dataChanged();
        else
            emit dataChanged();
    });

    connect(&*target, &Node::aboutToRemove, this, [this]{
        ZoneScoped;
        // NOTE: in contrast to other signals, which forward do subtreeRootOwner (if set), this signal does not.
        //       the reason is, in contrast to other operations, removal of the link's target shouldn't lead to
        //       the removal of the link itself
        emit targetAboutToRemove();
    });

    connect(&*target, &Node::insertChildrenEnd, this, [this](int const first, int const last){
        ZoneScoped;

        // we can only do our insertions after the target has completed its
        if (auto owner = owner_.lock())
            emit owner->insertChildrenBegin(first, last);
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
                children_.emplace(children_.begin() + row, std::move(*child));
        }

        if (auto owner = owner_.lock())
            emit owner->insertChildrenEnd(first, last);
        else
            emit insertChildrenEnd(first, last);
    });

    connect(&*target, &Node::beforeRemoveChildren, this, [this](int const first, int const last) {
        ZoneScoped;

        // we must do our removals before the target has completed its
        if (auto owner = owner_.lock())
            emit owner->removeChildrenBegin(first, last);
        else
            emit removeChildrenBegin(first, last);

        for (auto i = first; i <= last; ++i)
            children_[i]->deinit();
        children_.erase(children_.begin() + first, children_.begin() + last + 1);

        if (auto owner = owner_.lock())
            emit owner->removeChildrenEnd(first, last);
        else
            emit removeChildrenEnd(first, last);
    });

    auto count = target->childrenCount(!model().editMode());
    if (!count)
        return std::unexpected(count.error());

    children_.reserve(*count);
    for (int i = 0; i != *count; ++i) {
        if (auto result = createChild(i); !result)
            return std::unexpected(result.error());
        else
            children_.emplace_back(std::move(*result));
    }

    return {};
};

void NodeShadow::deinit() {
    ZoneScoped;

    Node::deinit();

    for (auto &child: children_)
        child->deinit();
}

std::shared_ptr<Node> NodeShadow::parent() const {
    ZoneScoped;

    auto parent = parent_.lock();
    assert(parent);

    if (auto shadowParent = std::dynamic_pointer_cast<NodeShadow const>(parent)) {
        if (auto owner = shadowParent->owner_.lock()) {
            if (owner->isReplaced() && !model().editMode())
                return owner->parent();
            else
                return owner;
        }
    }

    return parent;
}

std::expected<int, QString> NodeShadow::rowOfChild(Node const &node, bool const replaceReplaced) const {
    ZoneScoped;

    // TODO: this is basically the same as in NodeHierarchical::rowOfChild, merge?
    if (!replaceReplaced) {
        auto it = std::ranges::find_if(
                children_,
                [&](auto const &child) { return &*child == &node; }
        );
        if (it == children_.end())
            return std::unexpected(QObject::tr("Node %1 not found among the children of linked subtree node %2").arg(
                    node.path(PathFlag::IncludeEverything), path(PathFlag::IncludeEverything)
            ));
        return it - children_.begin();
    } else {
        int row = 0;

        for (auto const &child: children_) {
            // NOTE: This check purposedly takes place before the child->isReplaced() check.
            // This is to allow passing a replaced node as a "node" parameter. In this case
            // the function returns the row where the first replacement would be displayed.
            // This is because NodeInheritance calls this function to find out where its
            // replacement nodes should be placed.
            if (&*child == &node)
                return row;

            if (!child->isReplaced()) {
                ++row;
            } else {
                if (auto replacedCount = child->childrenCount(true); !replacedCount)
                    return std::unexpected(replacedCount.error());
                else {
                    for (int replacedRow = 0; replacedRow != *replacedCount; ++replacedRow) {
                        if (auto replacedChild = child->childOfRow(replacedRow, true); !replacedChild) {
                            return std::unexpected(replacedChild.error());
                        } else {
                            if (&**replacedChild == &node)
                                return row;
                        }
                        ++row;
                    }
                }
            }
        }
        return std::unexpected("rowOfChild: child not found");
    }
}

std::expected<std::shared_ptr<Node>, QString> NodeShadow::childOfRow(int const row, bool const replaceReplaced) const {
    // TODO: this is basically the same as in NodeHierarchical::childOfRow, merge?
    ZoneScoped;
    gsl_Expects(row >= 0);

    if (!replaceReplaced) {
        gsl_Expects(row < gsl::narrow<int>(children_.size()));
        return children_.at(row);
    } else {
        int childRow = 0;

        for (auto const &child: children_) {
            if (!child->isReplaced()) {
                if (childRow == row)
                    return child;

                childRow++;
            } else {
                if (auto replacedCount = child->childrenCount(true); !replacedCount) {
                    return std::unexpected(replacedCount.error());
                } else {
                    if (childRow + *replacedCount > row)
                        return child->childOfRow(row - childRow, true);

                    childRow += *replacedCount;
                }
            }
        }

        return std::unexpected(QString("childOfRow: row not found %1").arg(row));
    }
}

std::expected<int, QString> NodeShadow::childrenCount(bool const replaceReplaced) const {
    // TODO: this is basically the same as in NodeHierarchical::childrenCount, merge?
    ZoneScoped;
    if (!replaceReplaced) {
        return children_.size();
    } else {
        int count = 0;

        for (auto const &child: children_) {
            if (!child->isReplaced()) {
                count++;
            } else {
                if (auto replacedCount = child->childrenCount(true); !replacedCount) {
                    return std::unexpected(replacedCount.error());
                } else {
                    count += *replacedCount;
                }
            }
        }

        return count;
    }
}

QString NodeShadow::name(bool const raw, bool const editMode) const {
    ZoneScoped;
    return target()->name(raw, editMode);
}

std::vector<IconIdentifier> NodeShadow::icons() const {
    ZoneScoped;

    if (!icons_) {
        icons_.emplace({linkingIcon_});

        auto targetIcons = target()->icons();
        icons_->reserve(icons_->size() + targetIcons.size());
        std::ranges::move(targetIcons, std::back_inserter(*icons_));
    }
    return *icons_;
}

std::optional<QUuid> NodeShadow::linkTo() const {
    ZoneScoped;
    return target()->linkTo();
}

bool NodeShadow::isLinkingNode() const {
    return true;
}

QUuid NodeShadow::uuid() const {
    ZoneScoped;
    return target()->uuid();
}

std::vector<Node::Tag> NodeShadow::generateTags(TagFlags const flags) const {
    ZoneScoped;

    std::vector<Tag> result;

    auto tags = target()->tags(flags);

    if (!(flags & TagFlag::IncludeResolved)) {
        std::ranges::move(tags, std::back_inserter(result));
    } else {
        // TODO: this is the same like in NodeLink::tags -> helper?
        for (auto const &tag: tags) {
            auto resolvedList = parent()->resolveChildTag(tag.resolved);
            for (auto const &resolved: withoutDuplicates(resolvedList))
                result.emplace_back(tag.raw, resolved);
        }
    }

    return result;
}

QStringList NodeShadow::resolveChildTag(QString const &tag) const {
    ZoneScoped;
    if (auto owner = owner_.lock())
        return owner->resolveChildTag(tag);
    else
        return parent()->resolveChildTag(tag);
}

QString NodeShadow::comment() const {
    ZoneScoped;
    return target()->comment();
}

std::optional<bool> NodeShadow::active() const {
    return active_;
}

bool NodeShadow::canSetActive() const {
    return !tags().empty();
}

std::expected<void, QString> NodeShadow::setActive(bool const active) {
    ZoneScoped;

    active_ = active;

    if (auto owner = owner_.lock()) {
        emit owner->activeChanged(active);
        emit owner->dataChanged();
    } else {
        emit activeChanged(active);
        emit dataChanged();
    }

    return {};
}

std::optional<bool> NodeShadow::highlighted() const {
    return highlighted_;
}

bool NodeShadow::canSetHighlighted() const {
    return true;
}

std::expected<void, QString> NodeShadow::setHighlighted(bool const highlighted) {
    ZoneScoped;
    if (highlighted != highlighted_) {
        highlighted_ = highlighted;
        emit dataChanged();
    }
    return {};
}

NodeType NodeShadow::type() const {
    ZoneScoped;
    return target()->type();
}

bool NodeShadow::isVirtual() const {
    return true;
}

bool NodeShadow::isHidden() const {
    ZoneScoped;
    return target()->isHidden();
}

std::optional<int> NodeShadow::lastChangeVersion() const {
    ZoneScoped;
    return target()->lastChangeVersion();
}

std::vector<QBrush> NodeShadow::background(bool const editMode) const {
    ZoneScoped;
    auto result = Node::background(editMode);
    std::ranges::move(target()->background(false), std::back_inserter(result));
    if (editMode)
        result.emplace_back(QBrush(QColor(0, 0, 0, 32), Qt::BrushStyle::SolidPattern));
    return result;
}

bool NodeShadow::canPopulate() const {
    return true;
}

bool NodeShadow::canUnpopulate() const {
    return true;
}

std::expected<void, Error> NodeShadow::populateShadowsImpl() {
    ZoneScoped;
    for (auto &child: children_)
        if (auto result = child->populateShadowsImpl(); !result)
            return result;
    return {};
}

std::expected<void, Error> NodeShadow::unpopulateShadowsImpl() {
    ZoneScoped;
    for (auto &child: children_)
        if (auto result = child->unpopulateShadowsImpl(); !result)
            return result;
    return {};
}

std::expected<void, QString> NodeShadow::repopulateShadows(RepopulationRequest const &repopulationRequest) {
    ZoneScoped;
    for (auto &child: children_)
        if (auto result = child->repopulateShadows(repopulationRequest); !result)
            return result;
    return {};
}

std::expected<std::shared_ptr<NodeShadow>, QString> NodeShadow::createChild(int const row) {
    ZoneScoped;

    auto targetChildNode = target()->childOfRow(row, !model().editMode());
    if (!targetChildNode)
        return std::unexpected(targetChildNode.error());

    auto linkChild = std::make_shared<NodeShadow>(
            model(), shared_from_this(), *targetChildNode, nullptr, linkingIcon_
    );
    if (auto result = linkChild->init(); !result)
        return std::unexpected(result.error());

    return linkChild;
}

std::shared_ptr<Node> NodeShadow::target() const {
    auto target = target_.lock();
    if (!target) {
        // it might happen that the node has been recreated (destroyed and created
        // at another place), so we should try to find its new pointer
        if (auto newTarget = model().fromUuid(targetUuid_); !newTarget)
            qCCritical(LoggingCategory) << "Could not find new pointer to target" << targetUuid_ << ":" << newTarget.error();
        else
            target = *newTarget;
    }

    assert(target);
    return target;
}
}