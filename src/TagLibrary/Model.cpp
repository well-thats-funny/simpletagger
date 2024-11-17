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
#include "Model.hpp"

#include "Logging.hpp"
#include "NodeHierarchical.hpp"
#include "NodeRoot.hpp"

#include "../Utility.hpp"

namespace TagLibrary {
Model::Model() {
    connect(this, &Model::persistentDataChanged, this, [this]{
        if (auto result = invalidateTagCaches(); !result)
            reportError(QObject::tr("Tags cache invalidation failed"), result.error());
    });
}

Model::~Model() {
    root->deinit();
}

QModelIndex Model::toIndex(Node const &node) const {
    ZoneScoped;

    if (&node == &*root) {
        return {};
    } else {
        if (auto row = node.parent()->rowOfChild(node)) {
            return createIndex(*row, 0, &node);
        } else {
            qCCritical(LoggingCategory) << "Could not get row of child:" << row.error();
            assert(false);
            return {};
        }
    }
}

std::pair<QModelIndex, QModelIndex> Model::toIndexRange(Node const &node) const {
    ZoneScoped;
    auto index1 = toIndex(node);
    gsl_Expects(index1.column() == 0);
    auto index2 = createIndex(index1.row(), columnCount(index1.parent()) - 1, index1.internalPointer());
    return {index1, index2};
}

Node &Model::fromIndex(QModelIndex const &index) const {
    ZoneScoped;
    gsl_Expects(!index.isValid() || index.model() == this);
    gsl_Expects(root);

    if (!index.isValid())
        return *root;

    auto ptr = reinterpret_cast<Node *>(index.internalPointer());
    gsl_Ensures(ptr);
    return *ptr;
}

std::expected<std::reference_wrapper<Node>, QString> Model::fromUuid(const QUuid &uuid) const {
    ZoneScoped;

    auto find = [&](this auto &&self, Node &parent)->std::expected<Node*, QString> {
        ZoneScoped;

        if (parent.isVirtual())
            return nullptr;

        if (parent.uuid() == uuid)
            return &parent;

        for (int i = 0; i != parent.childrenCount(); ++i)
            if (auto child = parent.childOfRow(i); !child)
                return std::unexpected(child.error());
            else if (auto found = self(child->get()); !found)
                return std::unexpected(found.error());
            else if (*found)
                return *found;

        return nullptr;
    };
    if (auto node = find(*root); !node)
        return std::unexpected(node.error());
    else if (*node)
        return **node;
    else
        return std::unexpected(tr("No node with Uuid %1").arg(uuid.toString(QUuid::WithoutBraces)));
}

QModelIndex Model::index(int const row, int const column, QModelIndex const &parent) const {
    ZoneScoped;
    gsl_Expects(!parent.isValid() || parent.model() == this);

    QModelIndex result;

    auto &parentNode = fromIndex(parent);
    if (auto child = parentNode.childOfRow(row); !child) {
        qCWarning(LoggingCategory) << "childOfRow" << child.error();
        return QModelIndex();
    } else {
        qCDebug(LoggingCategory) << "index(" << row << column << parentNode.name() << ") -> "
                                 << "createIndex(" << row << column << child->get().name() << ")";

        result = createIndex(row, column, &child->get());
        gsl_Ensures(result.model() == this);
        return result;
    }
}

QModelIndex Model::parent(QModelIndex const &child) const {
    ZoneScoped;
    gsl_Expects(!child.isValid() || child.model() == this);

    auto &childNode = fromIndex(child);
    if (&childNode == &*root)
        return {};

    return toIndex(*childNode.parent());
}

int Model::rowCount(QModelIndex const &parent) const {
    ZoneScoped;
    gsl_Expects(!parent.isValid() || parent.model() == this);

    auto &node = fromIndex(parent);
    if (auto size = node.childrenCount(); !size) {
        qCDebug(LoggingCategory) << "rowCount(" << node.name() << ") -> unexpected: " << size.error();
        return 0;
    } else {
        qCDebug(LoggingCategory) << "rowCount(" << node.name() << ") -> " << *size;
        return *size;
    }
}

int Model::recursiveRowCount(const QModelIndex &parent) const {
    ZoneScoped;
    gsl_Expects(!parent.isValid() || parent.model() == this);

    int directCount = rowCount(parent);
    int count = directCount;

    for (int i = 0; i != directCount; ++i)
        count += recursiveRowCount(index(i, 0, parent));

    return count;
}

int Model::columnCount(const QModelIndex &parent) const {
    ZoneScoped;
    gsl_Expects(!parent.isValid() || parent.model() == this);
    return QMetaEnum::fromType<Column>().keyCount();
}

QVariant Model::headerData(int section, Qt::Orientation orientation, int role) const {
    ZoneScoped;

    if (orientation != Qt::Orientation::Horizontal)
        return {};

    switch (role) {
        case Qt::ItemDataRole::DisplayRole: {
            switch (static_cast<Column>(section)) {
                case Column::Name:
                    return tr("Name");
                case Column::Tag:
                    return tr("Tag");
            }
            break;
        }
    }
    return {};
}

Qt::ItemFlags Model::flags(QModelIndex const &index) const {
    ZoneScoped;
    gsl_Expects(!index.isValid() || index.model() == this);

    Qt::ItemFlags flags = Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable;

    if (index.isValid()) {
        auto &node = fromIndex(index);

        switch (static_cast<Column>(index.column())) {
            case Column::Name: {
                if (node.canSetName())
                    flags |= Qt::ItemFlag::ItemIsEditable;
                break;
            }
            case Column::Tag:
                if (node.canSetTags())
                    flags |= Qt::ItemFlag::ItemIsEditable;
                break;
        }

        if (node.canBeDragged())
            flags |= Qt::ItemFlag::ItemIsDragEnabled;

        if (node.canAcceptDrop())
            flags |= Qt::ItemFlag::ItemIsDropEnabled;
    }

    return flags;
}

QVariant Model::data(const QModelIndex &index, int role) const {
    ZoneScoped;
    gsl_Expects(!index.isValid() || index.model() == this);

    auto &node = fromIndex(index);

    QVariant result;

    switch (role) {
        case Qt::ItemDataRole::DisplayRole: {
            switch (static_cast<Column>(index.column())) {
                case Column::Name:
                    result = node.name(false, editMode_);
                    break;
                case Column::Tag:
                    if (!editMode_) {
                        result = node.tags(Node::TagFlag::IncludeResolved)
                                | std::views::transform([](auto const &tag){ return tag.resolved; })
                                | std::views::join_with(QString(", "))
                                | std::ranges::to<QString>();
                    } else {
                        result = node.tags(Node::TagFlag::IncludeRaw | Node::TagFlag::IncludeResolved)
                                | std::views::transform([](auto const &tag){
                                    assert(!tag.raw.isEmpty());
                                    assert(!tag.resolved.isEmpty());
                                    if (tag.raw == tag.resolved)
                                        return tag.raw;
                                    else
                                        return QString("%1 [%2]").arg(tag.resolved, tag.raw);
                                })
                                | std::views::join_with(QString(", "))
                                | std::ranges::to<QString>();
                    }
                    break;
            }
            break;
        }
        case Qt::ItemDataRole::DecorationRole: {
            switch (static_cast<Column>(index.column())) {
                case Column::Name:
                    result = QVariant::fromValue(node.icons());
                    break;
                case Column::Tag:
                    break;
            }
            break;
        }
        case Qt::ItemDataRole::EditRole: {
            switch (static_cast<Column>(index.column())) {
                case Column::Name:
                    result = node.name(true);
                    break;
                case Column::Tag:
                    result = node.tags(Node::TagFlag::IncludeRaw)
                            | std::views::transform([](auto const &tag){ return tag.raw; })
                            | std::views::join_with(QString(", "))
                            | std::ranges::to<QString>();
                    break;
            }
            break;
        }
        case Qt::ItemDataRole::ToolTipRole: {
            if (auto tooltip = node.tooltip(editMode_); !tooltip)
                qCCritical(LoggingCategory) << "Cannot generate tooltip for node" << node.uuid() << ":" << tooltip.error();
            else if (!tooltip->isEmpty())
                result = *tooltip;
            break;
        }
        case Qt::ItemDataRole::SizeHintRole: {
            result = QSize(rowHeight_, rowHeight_);
            break;
        }
        case Qt::ItemDataRole::FontRole: {
            QFont font;

            if (!editMode_) {
                if (auto active = node.active())
                    font.setBold(*active);

                if (highlightChangedAfterVersion_) {
                    if (auto highlight = node.lastChangeAfter(*highlightChangedAfterVersion_, true); !highlight)
                        qCCritical(LoggingCategory) << "Could not compare last change of a node:" << highlight.error();
                    else
                        font.setUnderline(*highlight);
                }
            }

            font.setItalic(node.isHidden());
            result = font;
            break;
        }
        case Qt::ItemDataRole::BackgroundRole: {
            result = QVariant::fromValue(node.background(editMode_));
            break;
        }
    }

    if (role == Qt::DisplayRole)
        qCDebug(LoggingCategory) << "data(" << node.name() << ", DisplayRole) ->" << result;

    return result;
}

bool Model::setData(const QModelIndex &index, const QVariant &value, int role) {
    ZoneScoped;
    gsl_Expects(!index.isValid() || index.model() == this);

    switch (role) {
        case Qt::ItemDataRole::EditRole: {
            switch (static_cast<Column>(index.column())) {
                case Column::Name:
                    if (!fromIndex(index).setName(value.toString()))
                        return false;
                    emit dataChanged(index, index);
                    return true;
                case Column::Tag: {
                    if (!fromIndex(index).setTags(value.toString().split(",")
                            | std::views::transform([](QString const &s){ return s.trimmed(); })
                            | std::views::filter([](QString const &s){ return !s.isEmpty(); })
                            | std::ranges::to<QStringList>()
                    ))
                        return false;
                    emit dataChanged(index, index);
                    return true;
                }
            }
            break;
        }
        case Qt::ItemDataRole::DecorationRole: {
            switch (static_cast<Column>(index.column())) {
                case Column::Name:
                    if (fromIndex(index).setIcon(value.toString()))
                        emit dataChanged(index, index);
                    return true;
                case Column::Tag:
                    return false;
            }
            break;
        }
    }

    return false;
}

bool Model::canInsertNode(NodeType const type, QModelIndex const &parent) const {
    ZoneScoped;
    gsl_Expects(!parent.isValid() || parent.model() == this);
    return fromIndex(parent).canInsertChild(type);
}

std::expected<QModelIndex, QString> Model::insertNode(NodeType type, QModelIndex const &parent) {
    ZoneScoped;
    gsl_Expects(!parent.isValid() || parent.model() == this);

    auto parentNode = &fromIndex(parent);
    auto parentNodeStored = dynamic_cast<NodeSerializable *>(parentNode);
    if (!parentNodeStored)
        return std::unexpected(tr("Parent node is not a stored node type subclass, but %1").arg(typeid(*parentNode).name()));

    gsl_Expects(parentNodeStored->canInsertChild(type));

    auto childNode = NodeHierarchical::createNode(type, *this, parentNodeStored);
    if (!childNode)
        return std::unexpected(childNode.error());

    auto row = parentNodeStored->childrenCount();
    if (!row)
        return std::unexpected(row.error());

    auto ptr = parentNodeStored->insertChild(*row, dynamicPtrCast<Node>(std::move(*childNode)));
    if (!ptr)
        return std::unexpected(ptr.error());

    return createIndex(*row, 0, *ptr);
}

[[nodiscard]] bool Model::canRemoveRow(QModelIndex const &index) {
    ZoneScoped;
    gsl_Expects(!index.isValid() || index.model() == this);
    auto &node = fromIndex(index);
    gsl_Expects(node.parent());
    return node.parent()->canRemoveChildren(index.row(), 1);
}

bool Model::removeRows(int const row, int const count, const QModelIndex &parent) {
    ZoneScoped;
    gsl_Expects(!parent.isValid() || parent.model() == this);
    auto &node = fromIndex(parent);
    node.removeChildren(row, count);
    return true;
}

QStringList Model::mimeTypes() const {
    return QStringList(mimeType.toString());
}

QMimeData *Model::mimeData(QModelIndexList const &indexes) const {
    ZoneScoped;
    gsl_Expects(std::ranges::all_of(indexes, [&](auto const &i){ return i.model() == this; }));

    if (indexes.empty())
        return nullptr;

    qCDebug(LoggingCategory) << "mimeData(" << indexes << ")";

    QCborArray array;

    QModelIndexList uniqueRowsIndexes; // ignore multiple indexes on the same row (differing by column only)
    for (auto const &index: indexes) {
        if (std::ranges::find_if(uniqueRowsIndexes, [&](auto const &index2){
            return index.row() == index2.row() && index.parent() == index2.parent();
        }) == uniqueRowsIndexes.end())
            uniqueRowsIndexes.append(index);
    }

    for(auto const &index: uniqueRowsIndexes) {
        auto &node = fromIndex(index);
        if (auto nodeStored = dynamic_cast<NodeSerializable *>(&node); !nodeStored) {
            qCCritical(LoggingCategory) << "Cannot generate MIME data for a not-stored node of type" << typeid(node).name();
            return nullptr;
        } else {
            auto result = nodeStored->save();

            if (!result) {
                qCCritical(LoggingCategory) << "Could not store node data:" << result.error();
                return nullptr;
            }

            array.append(*result);
        }
    }

    auto result = std::make_unique<QMimeData>();

    QByteArray data;
    {
        QBuffer buffer(&data);
        if (!buffer.open(QIODevice::WriteOnly)) {
            qCCritical(LoggingCategory) << "Buffer open failed:" << buffer.errorString();
            return nullptr;
        }

        QCborStreamWriter writer(&buffer);
        array.toCborValue().toCbor(writer);
    }

    result->setData(mimeType.toString(), data);
    return result.release();
}

Qt::DropActions Model::supportedDropActions() const {
    return editMode_ ? Qt::DropAction::MoveAction : Qt::DropActions{};
}

namespace {
std::generator<std::expected<QCborMap, QString>> parseMimeData(QMimeData const *const data) {
    ZoneScoped;

    if (!data->hasFormat(mimeType.toString())) {
        co_yield std::unexpected(QObject::tr("No supported mime type"));
        co_return;
    }

    QBuffer buffer;
    buffer.setData(data->data(mimeType.toString()));
    if (!buffer.open(QIODevice::ReadOnly)) {
        co_yield std::unexpected(QObject::tr("Buffer open failed: %1").arg(buffer.errorString()));
        co_return;
    }

    QCborStreamReader reader(&buffer);
    auto content = QCborValue::fromCbor(reader);

    qCDebug(LoggingCategory) << "Mime data content:" << content;

    if (!content.isArray()) {
        co_yield std::unexpected(QObject::tr("Mime data content root is not an array but %1").arg(cborTypeToString(content.type())));
        co_return;
    }

    auto array = content.toArray();
    if (array.size() > 1) {  // TODO: lift that requirement? (requires careful adjustments in the callers)
        co_yield std::unexpected(QObject::tr("Only one node drag&drop supported now, but got %1").arg(QString::number(array.size())));
        co_return;
    }

    for (auto const &element: array) {
        if (!element.isMap()) {
            co_yield std::unexpected(
                    QObject::tr("Mime data element is not a map but %1").arg(cborTypeToString(element.type())));
            co_return;
        }

        auto nodeData = element.toMap();
        co_yield nodeData;
    }
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmismatched-new-delete" // TODO: appears in MinSizeRel build
}
#pragma GCC diagnostic pop
}

bool Model::canDropMimeData(
        QMimeData const *const data, Qt::DropAction const action, int const /*row*/, int const /*column*/,
        QModelIndex const &parent
) const {
    ZoneScoped;
    gsl_Expects(data);
    gsl_Expects(!parent.isValid() || parent.model() == this);

    if (action != Qt::DropAction::MoveAction)
        return false;

    auto &parentNode = fromIndex(parent);

    for (auto const &v: parseMimeData(data)) {
        if (!v) {
            qCCritical(LoggingCategory) << "Cannot parse mime data:" << v.error();
            return false;
        }

        QCborMap map = *v;

        auto typeValue = map.take(std::to_underlying(Format::NodeKey::Type));
        if (!typeValue.isInteger()) {
            qCCritical(LoggingCategory) << "Mime data element type is not an integer but" << cborTypeToString(typeValue.type());
            return false;
        }

        // TODO: this won't work if we want to support multiple index drag&drop
        return parentNode.canAcceptDrop(static_cast<Format::NodeType>(typeValue.toInteger()));
    }

    return false;
}

bool Model::dropMimeData(
        QMimeData const *const data, Qt::DropAction const action, int row, int const column,
        QModelIndex const &parent
) {
    ZoneScoped;
    gsl_Expects(data);
    gsl_Expects(!parent.isValid() || parent.model() == this);
    (void)column; // gsl_Expects(column == -1); // TODO: which one?

    if (action != Qt::DropAction::MoveAction)
        return false;

    auto &parentNode = fromIndex(parent);
    auto parentNodeStored = dynamic_cast<NodeSerializable *>(&parentNode);
    if (!parentNodeStored) {
        qCCritical(LoggingCategory) << "Parent node should be a subclass of NodeStored, but got" << typeid(parentNode).name();
        return false;
    }

    if (row == -1) {
        if (auto result = parentNodeStored->childrenCount(); !result) {
            qCCritical(LoggingCategory) << ":" << result.error();
            return false;
        } else {
            row = *result;
        }
    }

    Node::RepopulationRequest repopulationRequest;

    for (auto const &v: parseMimeData(data)) {
        if (!v) {
            qCCritical(LoggingCategory) << "Cannot parse mime data:" << v.error();
            return false;
        }

        QCborMap map = *v;

        // TODO: not sure what will be the exact effect if we get more than one row... which order will they appear in=
        //       perhaps on the sender side (mimeData()) we should sort by row first and then insert in the same
        //       order here?

        auto node = NodeHierarchical::load(map, *this, parentNodeStored);
        if (!node) {
            qCCritical(LoggingCategory) << "Cannot create new node:" << node.error();
            return false;
        }

        if (!repopulationRequest.modifiedUuids)
            repopulationRequest.modifiedUuids.emplace();

        repopulationRequest.modifiedUuids->append(node->get()->uuid());

        if (auto result = parentNodeStored->insertChild(
                row, dynamicPtrCast<Node>(std::move(*node))
        ); !result) {
            qCCritical(LoggingCategory) << "Cannot insert child node:" << result.error();
            return false;
        }
    }

    if (auto result = root->repopulateLinkedRecursive(repopulationRequest); !result) {
        qCCritical(LoggingCategory) << "Cannot repopulate links:" << result.error();
        return false;
    }

    return true;
}

std::expected<void, QString> Model::resetRoot() {
    ZoneScoped;

    beginResetModel();
    auto _ = gsl::finally([this]{ endResetModel(); });

    {
        QSignalBlocker block(this);
        root.reset(new NodeRoot(*this));

        if (auto index = insertNode(NodeType::Collection, QModelIndex()); !index)
            return std::unexpected(index.error());
        else
            setData(createIndex(index->row(), std::to_underlying(Column::Name), index->internalPointer()), tr("Main collection"));
    }

    return {};
}

std::expected<QCborValue, QString> Model::save() const {
    ZoneScoped;
    return root->save();
}

std::expected<void, QString> Model::load(QCborValue const &value) {
    ZoneScoped;

    beginResetModel();
    auto _ = gsl::finally([this]{ endResetModel(); });

    {
        QSignalBlocker block(this);
        auto result = NodeHierarchical::load(value, *this, nullptr);
        if (!result)
            return std::unexpected(result.error());

        auto newRoot = dynamicPtrCast<NodeRoot>(std::move(*result));
        if (!newRoot)
            return std::unexpected(tr("Invalid type of root node, expected Root but got %1").arg(
                    QMetaEnum::fromType<NodeType>().valueToKey(std::to_underlying((*result)->type()))
            ));

        root->deinit();
        root = std::move(newRoot);
    }

    if (auto result = root->repopulateLinkedRecursive(); !result)
        return std::unexpected(result.error());

    emit loadComplete();

    if (auto result = root->verify(); !result) {
        // TODO: should failed verification be considered a hard error?
        //return std::unexpected(QString("Library verification failed: %1").arg(result.error().join("\n")));
        qCWarning(LoggingCategory) << "Library verification failed:";
        for (auto const &line: result.error())
            qCWarning(LoggingCategory) << line;
    }

    return {};
}

void Model::setEditMode(bool const editMode) {
    ZoneScoped;
    if (editMode_ != editMode) {
        editMode_ = editMode;
        emit dataChanged(QModelIndex(), QModelIndex()); // force redraw
    }
}

bool Model::editMode() const {
    return editMode_;
}

std::expected<void, QString> Model::setTagsActive(QStringList const &tags) {
    ZoneScoped;

    std::expected<void, QString> result;

    {
        QSignalBlocker blocker(*this);

        auto visit = [&](this auto &&self, Node &node) -> std::expected<void, QString> {
            ZoneScoped;

            if (node.canSetActive()) {
                bool active = false;

                for (auto const &tag: tags) {
                    if (std::ranges::any_of(
                            node.tags(Node::TagFlag::IncludeResolved),
                            [&](auto const &nodeTag) { return nodeTag.resolved == tag; }
                    )) {
                        active = true;
                        break;
                    }
                }

                if (auto result = node.setActive(active); !result)
                    return std::unexpected(result.error());
            }

            auto count = node.childrenCount();
            if (!count)
                return std::unexpected(count.error());

            for (int i = 0; i != *count; ++i) {
                auto childNode = node.childOfRow(i);
                if (!childNode)
                    return std::unexpected(childNode.error());

                if (auto result = self(childNode->get()); !result)
                    return std::unexpected(result.error());
            }

            return {};
        };
        result = visit(*root);
    }

    // potentially many tags might be modified in this function, that's why we block signals for the time of
    // visit and only fire global dataChanged here
    emit dataChanged(QModelIndex(), QModelIndex());
    return result;
}

void Model::setRowHeight(int const rowHeight) {
    ZoneScoped;
    rowHeight_ = rowHeight;
    emit dataChanged(QModelIndex(), QModelIndex(), QList<int>() << Qt::ItemDataRole::SizeHintRole);
}

std::expected<QModelIndexList, QString> Model::setHighlightedTags(QStringList const &tags) {
    ZoneScoped;

    auto visit = [&, t=this](this auto &&self, Node &node)->std::expected<QModelIndexList, QString>{
        ZoneScoped;

        QModelIndexList resultList;

        auto nodeTags = node.tags()
                | std::views::transform([](auto const &v){ return v.resolved; })
                | std::ranges::to<QStringList>();
        if (node.canSetHighlighted()) {
            bool highlight = std::ranges::any_of(nodeTags, [&](QString const &tag) {
                return tags.contains(tag);
            });

            if (auto result = node.setHighlighted(highlight); !result)
                return std::unexpected(result.error());
            else if (highlight)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds" // TODO: error or false positive in Qt ?
                resultList.push_back(t->toIndex(node));
#pragma GCC diagnostic pop
        }

        // TODO: this iteration stuff is repeated in many places. Could become a method of Node ?
        auto count = node.childrenCount();
        if (!count)
            return std::unexpected(count.error());

        for (int i = 0; i != *count; ++i) {
            auto childNode = node.childOfRow(i);
            if (!childNode)
                return std::unexpected(childNode.error());

            if (auto result = self(childNode->get()); !result)
                return std::unexpected(result.error());
            else
                resultList += *result;
        }

        return resultList;
    };
    return visit(*root);
}

void Model::setHighlightChangedAfterVersion(std::optional<int> const &version) {
    highlightChangedAfterVersion_ = version;
}

void Model::setNextLibraryVersion(int const nextVersion) {
    nextLibraryVersion_ = nextVersion;
}

std::expected<void, Error> Model::invalidateTagCaches() const {
    allTags_ = std::nullopt;

    auto visit = [](this auto &&self, Node &node)->std::expected<void, Error> {
        node.invalidateTagCache();

        // TODO: this iteration stuff is repeated in many places. Could become a method of Node ?
        auto count = node.childrenCount();
        if (!count)
            return std::unexpected(count.error());

        for (int i = 0; i != *count; ++i) {
            auto childNode = node.childOfRow(i);
            if (!childNode)
                return std::unexpected(childNode.error());

            if (auto result = self(childNode->get()); !result)
                return std::unexpected(result.error());
        }

        return {};
    };
    return visit(*root);
}

QStringList Model::allTags() const {
    if (!allTags_) {
        allTags_.emplace();

        auto visit = [t=this](this auto &&self, Node &node) -> std::expected<void, Error> {
            std::ranges::copy(
                    node.tags() | std::views::transform([](auto const &tag){ return tag.resolved; }),
                    std::back_inserter(*t->allTags_)
            );

            // TODO: this iteration stuff is repeated in many places. Could become a method of Node ?
            auto count = node.childrenCount();
            if (!count)
                return std::unexpected(count.error());

            for (int i = 0; i != *count; ++i) {
                auto childNode = node.childOfRow(i);
                if (!childNode)
                    return std::unexpected(childNode.error());

                if (auto result = self(childNode->get()); !result)
                    return std::unexpected(result.error());
            }

            return {};
        };
        visit(*root);
    }
    return *allTags_;
}
}
