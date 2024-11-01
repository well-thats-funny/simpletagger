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
#include "NodeSerializable.hpp"

#include "Logging.hpp"
#include "NodeCollection.hpp"
#include "NodeLink.hpp"
#include "NodeObject.hpp"
#include "NodeRoot.hpp"

#include "../Utility.hpp"

namespace TagLibrary {
std::expected<std::unique_ptr<NodeSerializable>, QString>
NodeSerializable::createNode(NodeType const type, Model &model, NodeSerializable const *const parent) {
    ZoneScoped;

    std::unique_ptr<NodeSerializable> childNode;
    switch (type) {
        case NodeType::Root:
            gsl_Expects(!parent);
            childNode = std::make_unique<NodeRoot>(model);
            break;
        case NodeType::Collection:
            childNode = std::make_unique<NodeCollection>(model, parent);
            break;
        case NodeType::Object:
            childNode = std::make_unique<NodeObject>(model, parent);
            break;
        case NodeType::Link:
            childNode = std::make_unique<NodeLink>(model, parent);
            break;
        default:
            return std::unexpected(QObject::tr("Unknown node type: %1 (%2)").arg(
                    QString::number(std::to_underlying(type)),
                    QMetaEnum::fromType<NodeType>().valueToKey(std::to_underlying(type))
            ));
    }
    if (auto result = childNode->init(); !result)
        return std::unexpected(result.error());

    return childNode;
}

[[nodiscard]] IconIdentifier NodeSerializable::genericIcon(NodeType const type) {
    ZoneScoped;

    switch (type) {
        case NodeType::Root:
            gsl_Expects(false);
            return {};
        case NodeType::Collection:
            return NodeCollection::genericIcon();
        case NodeType::Object:
            return NodeObject::genericIcon();
        case NodeType::Link:
            return NodeLink::genericIcon();
        default:
            gsl_Expects(false);
            return {};
    }
}

NodeSerializable::NodeSerializable(Model &model) : Node(model), uuid_(QUuid::createUuid()) {}

NodeSerializable::~NodeSerializable() = default;

QString NodeSerializable::generateUnusedChildName(QString const &prefix) const {
    ZoneScoped;

    QString result = prefix;

    for (int i = 1;; ++i) {
        bool found = false;

        for (int j = 0; j != childrenCount(); ++j)
            if (auto child = childOfRow(j); !child) {
                qCCritical(LoggingCategory) << child.error();
            } else if (child->get().name(true) == result) {
                found = true;
                break;
            }

        if (!found)
            return result;

        result = QString("%1 #%2").arg(prefix, QString::number(i));
    }
}

bool NodeSerializable::isLinkingNode() const {
    return false;
}

QUuid NodeSerializable::uuid() const {
    return uuid_;
}

std::expected<QCborValue, QString> NodeSerializable::save() const {
    ZoneScoped;

    QCborMap map;

    if (auto result = saveNodeData(map); !result)
        return std::unexpected(result.error());

    return map;
}

std::expected<std::unique_ptr<NodeSerializable>, QString> NodeSerializable::load(QCborValue const &value, Model &model, NodeSerializable const *const parent) {
    ZoneScoped;

    if (!value.isMap())
        return std::unexpected(QObject::tr("Node is not a map"));

    auto map = value.toMap();

    auto typeValue = map.take(std::to_underlying(Format::NodeKey::Type));
    if (typeValue.isUndefined())
        return std::unexpected(QObject::tr("Node has no type key"));
    if (!typeValue.isInteger())
        return std::unexpected(QObject::tr("Node type is not an integer but %1").arg(cborTypeToString(typeValue.type())));

    auto type = static_cast<Format::NodeType>(typeValue.toInteger());

    std::unique_ptr<NodeSerializable> node;
    if (auto result = createNode(type, model, parent); !result)
        return std::unexpected(result.error());
    else
        node = std::move(*result);

    if (auto result = node->loadNodeData(map); !result)
        return std::unexpected(result.error());

    for (auto const &v: map)
        qCWarning(LoggingCategory) << "Unhandled element:" << v.first << "=" << v.second;

    return node;
}

std::expected<void, QString> NodeSerializable::saveNodeData(QCborMap &map) const {
    ZoneScoped;
    map[std::to_underlying(Format::NodeKey::Type)] = std::to_underlying(type());
    map[std::to_underlying(Format::NodeKey::Uuid)] = uuid_.toRfc4122();
    return {};
}

std::expected<void, QString> NodeSerializable::loadNodeData(QCborMap &map) {
    ZoneScoped;
    auto uuid = map.take(std::to_underlying(Format::NodeKey::Uuid));
    if (!uuid.isByteArray())
        return std::unexpected(QObject::tr("UUID element is not a byte array but %1").arg(cborTypeToString(uuid.type())));
    uuid_ = QUuid::fromRfc4122(uuid.toByteArray());
    return {};
}
}
