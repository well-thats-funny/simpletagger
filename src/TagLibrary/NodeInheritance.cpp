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

namespace TagLibrary {
NodeInheritance::NodeInheritance(TagLibrary::Model &model, TagLibrary::NodeSerializable const *const parent): NodeHierarchical(model, parent) {}

NodeInheritance::~NodeInheritance() = default;

QString NodeInheritance::name(bool const raw, bool const editMode) const {
    ZoneScoped;

    // TODO: this is similar to NodeLink::name, merge to a common function?
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
/*    } else if (linkSubtreeRoot_) {
        return linkSubtreeRoot_->name();*/
    } else {
        return {};
    }
}

bool NodeInheritance::canSetName() const {
    return true;
}

bool NodeInheritance::setName(QString const &name) {
    ZoneScoped;
    name_ = name;
    emit persistentDataChanged();
    return true;
}

IconIdentifier NodeInheritance::genericIcon() {
    ZoneScoped;
    return IconIdentifier(":/icons/bx-up-arrow.svg");
}

std::vector<IconIdentifier> NodeInheritance::icons() const {
    ZoneScoped;
//    if (!linkSubtreeRoot_)
        return {IconIdentifier(":/icons/bx-up-arrow-broken.svg")};
//    else
//        return linkSubtreeRoot_->icons();
}

std::optional<QUuid> NodeInheritance::linkTo() const {
    return linkTo_;
}

bool NodeInheritance::canLinkTo() const {
    return true;
}

std::expected<void, QString> NodeInheritance::setLinkTo(QUuid const &uuid) {
    ZoneScoped;

    if (uuid != linkTo_) {
/*        if (auto result = unpopulateLinked(); !result)
            return std::unexpected(result.error());*/

        linkTo_ = uuid;

/*        if (auto result = populateLinked(); !result)
            return std::unexpected(result.error());*/
    }

    emit persistentDataChanged();
    return {};
}

QString NodeInheritance::comment() const {
    return comment_;
}

bool NodeInheritance::canSetComment() const {
    return true;
}

std::expected<void, QString> NodeInheritance::setComment(QString const &comment) {
    ZoneScoped;
    if (comment != comment_) {
        comment_ = comment;
        emit persistentDataChanged();
    }
    return {};
}

NodeType NodeInheritance::type() const {
    return NodeType::Inheritance;
}

bool NodeInheritance::isReplaced() const {
    return true;
}

/*std::expected<int, Error> NodeInheritance::replacedCount() const {
    ZoneScoped;
    if (auto t = target(); !t)
        return std::unexpected(t.error());
    else
        return t->get().childrenCount();
}

std::expected<std::reference_wrapper<Node>, Error> NodeInheritance::replacedNode(int const row) const {
    ZoneScoped;
    if (auto t = target(); !t)
        return std::unexpected(t.error());
    else
        return t->get().childOfRow(row);
}*/

std::expected<std::reference_wrapper<Node>, QString> NodeInheritance::target() const {
    ZoneScoped;
    return model().fromUuid(linkTo_);
}

std::expected<void, QString> NodeInheritance::saveNodeData(QCborMap &map) const {
    ZoneScoped;

    if (auto result = NodeHierarchical::saveNodeData(map); !result)
        return std::unexpected(result.error());

    map[std::to_underlying(Format::NodeKey::Name)] = name_;
    map[std::to_underlying(Format::NodeKey::LinkTo)] = linkTo_.toRfc4122();

    if (!comment_.isEmpty())
        map[std::to_underlying(Format::NodeKey::Comment)] = comment_;

    return {};
}

std::expected<void, QString> NodeInheritance::loadNodeData(QCborMap &map) {
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
}
