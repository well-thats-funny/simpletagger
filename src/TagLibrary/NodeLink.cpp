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
#include "NodeLink.hpp"

#include "Logging.hpp"
#include "Model.hpp"
#include "NodeLinkSubtree.hpp"

#include "../Utility.hpp"

namespace TagLibrary {
NodeLink::NodeLink(Model &model, NodeSerializable const *const parent): NodeHierarchical(model, parent) {}

NodeLink::~NodeLink() = default;

void NodeLink::deinit() {
    ZoneScoped;

    NodeHierarchical::deinit();

    // this signal leads to premature destruction of linkSubtreeRoot_
    disconnect(subtreeRootAboutToRemoveConnection);

    if (linkSubtreeRoot_ && !linkSubtreeRoot_->deinitialized())
        linkSubtreeRoot_->deinit();
}

std::expected<int, QString> NodeLink::rowOfChild(Node const &node) const {
    ZoneScoped;
    gsl_Expects(linkSubtreeRoot_);
    return linkSubtreeRoot_->rowOfChild(node);
}

std::expected<std::reference_wrapper<Node>, QString> NodeLink::childOfRow(int const row) const {
    ZoneScoped;
    gsl_Expects(linkSubtreeRoot_);
    return linkSubtreeRoot_->childOfRow(row);
}

std::expected<int, QString> NodeLink::childrenCount() const {
    ZoneScoped;

    if (!linkSubtreeRoot_)
        return 0;
    else
        return linkSubtreeRoot_->childrenCount();
}

bool NodeLink::canBeDragged() const {
    return true;
}

QString NodeLink::name(bool const raw, bool const editMode) const {
    ZoneScoped;

    if (raw) {
        return name_;
    } else if (editMode) {
        QString targetName;
        if (linkTo_.isNull())
            targetName = QObject::tr("<no linked element>");
        else if (auto targetNode = target(); !targetNode)
            targetName = QObject::tr("<invalid target> (%1; %2)").arg(linkTo_.toString(QUuid::WithoutBraces), targetNode.error());
        else
            targetName = targetNode->get().name();

        return QString("%1 -> %2").arg(name_, targetName);
    } else if (!name_.isEmpty()) {
        return name_;
    } else if (linkSubtreeRoot_) {
        return linkSubtreeRoot_->name();
    } else {
        return {};
    }
}

bool NodeLink::canSetName() const {
    return true;
}

bool NodeLink::setName(QString const &name) {
    ZoneScoped;
    name_ = name;
    emit persistentDataChanged();
    return true;
}

IconIdentifier NodeLink::genericIcon() {
    ZoneScoped;
    return IconIdentifier(":/icons/bx-link.svg");
}

std::vector<IconIdentifier> NodeLink::icons() const {
    ZoneScoped;
    if (!linkSubtreeRoot_)
        return {IconIdentifier(":/icons/bx-unlink.svg")};
    else
        return linkSubtreeRoot_->icons();
}

std::optional<QUuid> NodeLink::linkTo() const {
    return linkTo_;
}

bool NodeLink::canLinkTo() const {
    return true;
}

std::expected<void, QString> NodeLink::setLinkTo(QUuid const &uuid) {
    ZoneScoped;

    if (uuid != linkTo_) {
        if (auto result = unpopulateLinked(); !result)
            return std::unexpected(result.error());

        linkTo_ = uuid;

        if (auto result = populateLinked(); !result)
            return std::unexpected(result.error());
    }

    emit persistentDataChanged();
    return {};
}

std::vector<Node::Tag> NodeLink::generateTags(TagFlags const flags) const {
    ZoneScoped;

    std::vector<Tag> result;

    if (linkSubtreeRoot_) {
        auto tags = linkSubtreeRoot_->tags(flags);

        if (!(flags & TagFlag::IncludeResolved)) {
            std::ranges::move(tags, std::back_inserter(result));
        } else {
            for (auto const &tag: tags) {
                auto resolvedList = parent()->resolveChildTag(tag.resolved);
                for (auto const &resolved: withoutDuplicates(resolvedList))
                    result.emplace_back(tag.raw, resolved);
            }
        }
    }

    return result;
}

QStringList NodeLink::resolveChildTag(QString const &tag) const {
    ZoneScoped;
    return parent()->resolveChildTag(tag);
}

QString NodeLink::comment() const {
    return comment_;
}

bool NodeLink::canSetComment() const {
    return true;
}

std::expected<void, QString> NodeLink::setComment(QString const &comment) {
    ZoneScoped;
    if (comment != comment_) {
        comment_ = comment;
        emit persistentDataChanged();
    }
    return {};
}

std::optional<bool> NodeLink::active() const {
    return active_;
}

bool NodeLink::canSetActive() const {
    return !tags().empty();
}

std::expected<void, QString> NodeLink::setActive(bool const active) {
    ZoneScoped;
    if (active != active_) {
        active_ = active;
        emit activeChanged(active);
        emit dataChanged();
    }
    return {};
}

std::optional<bool> NodeLink::highlighted() const {
    ZoneScoped;
    if (linkSubtreeRoot_)
        return linkSubtreeRoot_->highlighted();
    else
        return std::nullopt;
}

bool NodeLink::canSetHighlighted() const {
    ZoneScoped;
    if (linkSubtreeRoot_)
        return linkSubtreeRoot_->canSetHighlighted();
    else
        return false;
}

std::expected<void, QString> NodeLink::setHighlighted(bool const highlighted) {
    ZoneScoped;
    if (linkSubtreeRoot_)
        return linkSubtreeRoot_->setHighlighted(highlighted);
    else
        return Node::setHighlighted(highlighted);
}

NodeType NodeLink::type() const {
    return NodeType::Link;
}

std::expected<std::reference_wrapper<Node>, QString> NodeLink::target() const {
    ZoneScoped;
    return model().fromUuid(linkTo_);
}

std::expected<void, QString> NodeLink::saveNodeData(QCborMap &map) const {
    ZoneScoped;

    if (auto result = NodeHierarchical::saveNodeData(map); !result)
        return std::unexpected(result.error());

    map[std::to_underlying(Format::NodeKey::Name)] = name_;
    map[std::to_underlying(Format::NodeKey::LinkTo)] = linkTo_.toRfc4122();

    if (!comment_.isEmpty())
        map[std::to_underlying(Format::NodeKey::Comment)] = comment_;

    return {};
}

std::expected<std::optional<QCborArray>, QString> NodeLink::saveChildrenNodes() const {
    return std::nullopt;
}

std::expected<void, QString> NodeLink::loadNodeData(QCborMap &map) {
    ZoneScoped;

    if (auto result = NodeHierarchical::loadNodeData(map); !result)
        return std::unexpected(result.error());

    auto name = map.take(std::to_underlying(Format::NodeKey::Name));
    if (!name.isString())
        return std::unexpected(QObject::tr("Name element is not a string but %1").arg(cborTypeToString(name.type())));
    name_ = name.toString();

    auto linkTo = map.take(std::to_underlying(Format::NodeKey::LinkTo));
    if (!linkTo.isByteArray())
        return std::unexpected(QObject::tr("Link target element is not a byte array but %1").arg(cborTypeToString(linkTo.type())));
    linkTo_ = QUuid::fromRfc4122(linkTo.toByteArray());

    auto comment = map.take(std::to_underlying(Format::NodeKey::Comment));
    if (!comment.isUndefined()) {
        if (!comment.isString())
            return std::unexpected(
                    QObject::tr("Comment element is not a string but %1").arg(cborTypeToString(comment.type())));
        else
            comment_ = comment.toString();
    }

    return {};
}

std::expected<void, QString> NodeLink::loadChildrenNodes([[maybe_unused]] QCborMap &map) {
    ZoneScoped;
    assert(!map.contains(std::to_underlying(Format::NodeKey::Children)));
    return {};
}

std::expected<void, QString> NodeLink::populateLinked() {
    gsl_Expects(!linkSubtreeRoot_);

    if (!linkTo_.isNull()) {
        auto target = model().fromUuid(linkTo_);
        if (!target)
            return std::unexpected(target.error());

        auto subtreeRoot = std::make_unique<NodeLinkSubtree>(model(), this, target->get(), this);
        if (auto result = subtreeRoot->init(); !result)
            return std::unexpected(result.error());

        connect(&*subtreeRoot, &NodeLinkSubtree::targetAboutToRemove, this, [this]{
            if (auto result = unpopulateLinked(); !result)
                qCCritical(LoggingCategory) << "targetAboutToRemove -> unpopulateLinked error:" << result.error();
        });

        auto childrenCount = subtreeRoot->childrenCount();
        if (!childrenCount)
            return std::unexpected(childrenCount.error());

        if (*childrenCount > 0)
            emit insertChildrenBegin(0, *childrenCount - 1);

        linkSubtreeRoot_ = dynamicPtrCast<NodeLinkSubtree>(std::move(subtreeRoot));

        if (*childrenCount > 0)
            emit insertChildrenEnd(0, *childrenCount - 1);
    }

    gsl_Ensures(!linkTo_.isNull() == static_cast<bool>(linkSubtreeRoot_));
    return {};
}

std::expected<void, QString> NodeLink::unpopulateLinked() {
    if (linkSubtreeRoot_) {
        auto childrenCount = linkSubtreeRoot_->childrenCount();
        if (!childrenCount)
            return std::unexpected(childrenCount.error());

        if (*childrenCount > 0)
            emit removeChildrenBegin(0, *childrenCount - 1);

        if (!linkSubtreeRoot_->deinitialized())
            linkSubtreeRoot_->deinit();

        linkSubtreeRoot_.reset();

        if (*childrenCount > 0)
            emit removeChildrenEnd(0, *childrenCount - 1);
    }

    gsl_Ensures(!linkSubtreeRoot_);
    return {};
}

std::expected<void, QString> NodeLink::repopulateLinked(RepopulationRequest const &repopulationRequest) {
    ZoneScoped;

    if (auto result = Node::repopulateLinked(repopulationRequest); !result)
        return std::unexpected(result.error());

    // build a list of all UUIDs for which change we need to repopulate
    QSet<QUuid> allRelatedUuids;

    // our UUID and all our our parents
    Node const *node = this;
    while(node) {
        allRelatedUuids.insert(node->uuid());
        node = node->parent();
    }

    // target UUID and all its parents
    if (!linkTo_.isNull()) {
        if (auto target = model().fromUuid(linkTo_); !target) {
            // TODO: this can be because target is invalid - but does not have to! There should be a way to
            //       distinguish such situations
            qCWarning(LoggingCategory) << "Could not find node with UUID" << linkTo_ << ":" << target.error();
        } else {
            node = &target->get();
            while(node) {
                allRelatedUuids.insert(node->uuid());
                node = node->parent();
            }
        }
    }

    if (!repopulationRequest.modifiedUuids  // no specific UUIDs requested -> all are relevant
        || std::ranges::any_of(*repopulationRequest.modifiedUuids, [&](auto const &uuid){ return allRelatedUuids.contains(uuid); })
    ) {
        if (auto result = unpopulateLinked(); !result)
            return std::unexpected(result.error());

        if (auto result = populateLinked(); !result)
            return std::unexpected(result.error());
    }

    return {};
}
}
