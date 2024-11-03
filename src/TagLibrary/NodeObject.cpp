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
#include "NodeObject.hpp"

#include "Logging.hpp"

#include "../TagProcessor.hpp"
#include "../Utility.hpp"

namespace TagLibrary {
NodeObject::NodeObject(Model &model, NodeSerializable const *const parent):
    NodeHierarchical(model, parent), name_(parent->generateUnusedChildName(QObject::tr("New object"))) {}

NodeObject::~NodeObject() = default;

bool NodeObject::canInsertChild(NodeType const type) const {
    if (type == NodeType::Object || type == NodeType::Link)
        return true;

    return false;
}

bool NodeObject::canRemoveChildren(int const, int const) const {
    return true;
}

bool NodeObject::canBeDragged() const {
    return true;
}

QString NodeObject::name(bool const, bool const) const {
    return name_;
}

bool NodeObject::canSetName() const {
    return true;
}

bool NodeObject::setName(QString const &name) {
    ZoneScoped;
    name_ = name;
    emit persistentDataChanged();
    return true;
}

IconIdentifier NodeObject::genericIcon() {
    return IconIdentifier(":/icons/bx-cube.svg");
}

std::vector<IconIdentifier> NodeObject::icons() const {
    ZoneScoped;
    return {icon_.isEmpty() ? genericIcon() : IconIdentifier(icon_)};
}

bool NodeObject::canSetIcon() const {
    return true;
}

bool NodeObject::setIcon(const QString &path) {
    ZoneScoped;
    icon_ = path;
    emit persistentDataChanged();
    return true;
}

std::vector<Node::Tag> NodeObject::tags(TagFlags const flags) const {
    ZoneScoped;

    std::vector<Tag> result;
    result.reserve(tags_.size());

    for (auto const &tag: tags_) {
        gsl_Expects(!tag.isEmpty());
        std::vector<Tag> elements;

        if (flags & TagFlag::IncludeResolved) {
            elements = parent()->resolveChildTag(tag)
                    | std::views::transform([](auto const &resolved){
                    gsl_Expects(!resolved.isEmpty());

                        Tag t;
                        t.resolved = resolved;
                        return t;
                    })
                    | std::ranges::to<std::vector>();
        }

        if (flags & TagFlag::IncludeRaw) {
            if (elements.empty())
                elements.resize(1);

            for (auto &element: elements)
                element.raw = tag;
        }

        std::ranges::move(elements, std::back_inserter(result));
    }

    if (flags & TagFlag::IncludeRaw)
        gsl_Ensures(std::ranges::all_of(result, [](auto const &v){ return !v.raw.isEmpty(); }));

    if (flags & TagFlag::IncludeResolved)
        gsl_Ensures(std::ranges::all_of(result, [](auto const &v){ return !v.resolved.isEmpty(); }));

    return withoutDuplicates(result) | std::ranges::to<std::vector>();
}

bool NodeObject::canSetTags() const {
    return true;
}

std::expected<void, QString> NodeObject::setTags(QStringList const &tags) {
    ZoneScoped;
    if (tags != tags_) {
        if (std::ranges::any_of(tags, &QString::isEmpty))
            return std::unexpected("Cannot set empty tag");

        tags_ = tags;
        emit persistentDataChanged();
    }
    return {};
}

QStringList NodeObject::resolveChildTag(QString const &tag) const {
    ZoneScoped;
    QStringList result;

    if (tags_.empty())
        result.push_back(tag);
    else
        for (auto const &ownTag: tags_) {
            result.push_back(TagProcessor::resolveChildTag(ownTag, tag));
        }

    QStringList result2;
    for (auto const &resultTag: result)
        result2 += parent()->resolveChildTag(resultTag);
    return result2;
}

QString NodeObject::comment() const {
    return comment_;
}

bool NodeObject::canSetComment() const {
    return true;
}

std::expected<void, QString> NodeObject::setComment(QString const &comment) {
    ZoneScoped;
    if (comment != comment_) {
        comment_ = comment;
        emit persistentDataChanged();
    }
    return {};
}

std::optional<bool> NodeObject::active() const {
    return active_;
}

bool NodeObject::canSetActive() const {
    return !tags().empty();
}

std::expected<void, QString> NodeObject::setActive(bool const active) {
    ZoneScoped;
    if (active != active_) {
        active_ = active;
        emit activeChanged(active);
        emit dataChanged();
    }
    return {};
}

std::optional<bool> NodeObject::highlighted() const {
    return highlighted_;
}

bool NodeObject::canSetHighlighted() const {
    return true;
}

std::expected<void, QString> NodeObject::setHighlighted(bool const highlighted) {
    ZoneScoped;
    if (highlighted != highlighted_) {
        highlighted_ = highlighted;
        emit dataChanged();
    }
    return {};
}

bool NodeObject::canBeLinkedTo() const {
    return true;
}

NodeType NodeObject::type() const {
    return NodeType::Object;
}

std::expected<void, QString> NodeObject::saveNodeData(QCborMap &map) const {
    ZoneScoped;

    if (auto result = NodeHierarchical::saveNodeData(map); !result)
        return std::unexpected(result.error());

    map[std::to_underlying(Format::NodeKey::Name)] = name_;

    if (!icon_.isEmpty())
        map[std::to_underlying(Format::NodeKey::Icon)] = icon_;

    if (!tags_.isEmpty()) {
        QCborArray array;
        std::ranges::transform(tags_, std::back_inserter(array), [](QString const &s){ return QCborValue(s); });
        map[std::to_underlying(Format::NodeKey::Tags)] = array;
    }

    if (!comment_.isEmpty())
        map[std::to_underlying(Format::NodeKey::Comment)] = comment_;

    return {};
}

std::expected<void, QString> NodeObject::loadNodeData(QCborMap &map) {
    ZoneScoped;

    if (auto result = NodeHierarchical::loadNodeData(map); !result)
        return std::unexpected(result.error());

    auto name = map.take(std::to_underlying(Format::NodeKey::Name));
    if (!name.isString())
        return std::unexpected(QObject::tr("Name element is not a string but %1").arg(cborTypeToString(name.type())));
    name_ = name.toString();

    auto icon = map.take(std::to_underlying(Format::NodeKey::Icon));
    if (!icon.isUndefined()) {
        if (!icon.isString())
            return std::unexpected(
                    QObject::tr("Icon element is not a string but %1").arg(cborTypeToString(icon.type())));
        else
            icon_ = icon.toString();
    }

    auto tags = map.take(std::to_underlying(Format::NodeKey::Tags));
    if (!tags.isUndefined()) {
        /*if (tags.isString()) { // TODO: outdated, remove this branch
            qWarning() << "Tag is a single string, this is deprecated and will be removed later";
            tags_ = QStringList() << tags.toString();
        } else*/ {

            if (!tags.isArray())
                return std::unexpected(QObject::tr("Tag element is not an array but %1").arg(cborTypeToString(tags.type())));
            else {
                auto array = tags.toArray();
                tags_.clear();
                tags_.reserve(array.size());
                for (auto const &v: array) {
                    if (!v.isString())
                        return std::unexpected(QObject::tr("Tag array element is a string but %1").arg(cborTypeToString(tags.type())));
                    else
                        tags_.push_back(v.toString());
                }
            }

        }
    }

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
