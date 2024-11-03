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

std::expected<void, QString> NodeLink::afterDrop() {
    if (auto result = NodeHierarchical::afterDrop(); !result)
        return std::unexpected(result.error());

    resolveLink();
    return {};
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

    std::unique_ptr<NodeLinkSubtree> root;
    QMetaObject::Connection newSubtreeRootAboutToRemoveConnection;

    if (!uuid.isNull()) {
        auto target = model().fromUuid(uuid);
        if (!target)
            return std::unexpected(target.error());

        root = std::make_unique<NodeLinkSubtree>(const_cast<Model &>(model()), this, target->get(), this);
        if (auto result = root->init(); !result)
            return std::unexpected(result.error());

        newSubtreeRootAboutToRemoveConnection = connect(&*root, &NodeLinkSubtree::targetAboutToRemove, this, [this, uuid]mutable{
            if (!linkSubtreeRoot_->deinitialized())
                linkSubtreeRoot_->deinit();
            linkSubtreeRoot_.reset();
        });

        newSubtreeRootAboutToRemoveConnection = connect(&*root, &NodeLinkSubtree::aboutToRemove, this, [this, uuid]mutable{
        });
    } else {
        qCDebug(LoggingCategory) << "Setting link" << path(PathFlag::IncludeEverything) << "to zero-uuid";
    }

    int oldChildrenCount = 0;
    int newChildrenCount = 0;

    if (linkSubtreeRoot_) {
        if (auto result = linkSubtreeRoot_->childrenCount(); !result)
            return std::unexpected(result.error());
        else
            oldChildrenCount = *result;
    }

    if (root) {
        if (auto result = root->childrenCount(); !result)
            return std::unexpected(result.error());
        else
            newChildrenCount = *result;
    }


    // here we assume we cannot fail anymore. This is, among others, because Qt doesn't support failures between remove/insert begin and end

    QObject::disconnect(subtreeRootAboutToRemoveConnection);

    // notify about all children removal
    if (oldChildrenCount != 0)
        emit removeChildrenBegin(0, oldChildrenCount - 1);

    linkTo_ = QUuid();

    if (linkSubtreeRoot_)
        linkSubtreeRoot_->deinit();

    linkSubtreeRoot_ = nullptr;

    if (oldChildrenCount != 0)
        emit removeChildrenEnd(0, oldChildrenCount - 1);

    if (!uuid.isNull()) {
        if (newChildrenCount != 0)
            emit insertChildrenBegin(0, newChildrenCount - 1);

        linkTo_ = uuid;
        linkSubtreeRoot_ = dynamicPtrCast<NodeLinkSubtree>(std::move(root));
        subtreeRootAboutToRemoveConnection = newSubtreeRootAboutToRemoveConnection;

        if (newChildrenCount != 0)
            emit insertChildrenEnd(0, newChildrenCount - 1);
    }

    emit persistentDataChanged();
    return {};
}

std::vector<Node::Tag> NodeLink::tags(TagFlags const flags) const {
    ZoneScoped;

    std::vector<Tag> result;

    if (linkSubtreeRoot_) {
        auto tags = linkSubtreeRoot_->tags(flags);

        if (!(flags & TagFlag::IncludeResolved)) {
            std::ranges::move(tags, std::back_inserter(result));
        } else {
            for (auto const &tag: tags)
                for (auto const &resolved: parent()->resolveChildTag(tag.resolved))
                    result.emplace_back(tag.raw, resolved);
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

    connect(&model(), &Model::loadComplete, this, [this]{
        resolveLink();
    });

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

void NodeLink::resolveLink() {
    // force copy of QUuid, as setLinkTo overwrites the field linkTo_ :D
    if (auto result = setLinkTo(QUuid(linkTo_)); !result)
        qCCritical(LoggingCategory) << "Could not set link to" << linkTo_ << "at load complete:" << result.error();
}
}
