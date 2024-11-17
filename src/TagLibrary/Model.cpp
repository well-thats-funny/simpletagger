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

#include "../CustomItemDataRole.hpp"
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
        auto parent = node.parent();
        if (auto row = parent->rowOfChild(node, !editMode_)) {
            return createIndex(*row, 0, &node);
        } else {
            reportError(QString("Model::toIndex: rowOfChild failed (parent %1, child %2)")
                .arg(parent->name(true, false)).arg(node.name(true, false)), row.error(), false);
            assert(false);
            return {};
        }
    }
}

QModelIndex Model::toIndex(std::shared_ptr<Node> const &node) const {
    return toIndex(*node);
}

std::pair<QModelIndex, QModelIndex> Model::toIndexRange(Node const &node) const {
    ZoneScoped;
    auto index1 = toIndex(node);
    gsl_Expects(index1.column() == 0);
    auto index2 = createIndex(index1.row(), columnCount(index1.parent()) - 1, index1.internalPointer());
    return {index1, index2};
}

std::shared_ptr<Node> Model::fromIndex(QModelIndex const &index) const {
    ZoneScoped;
    gsl_Expects(!index.isValid() || index.model() == this);

    if (!index.isValid())
        return root;

    auto ptr = reinterpret_cast<Node *>(index.internalPointer())->shared_from_this();
    gsl_Ensures(ptr);
    return ptr;
}

std::expected<std::shared_ptr<Node>, QString> Model::fromUuid(const QUuid &uuid) const {
    ZoneScoped;

    auto ptr = uuidToNode_.value(uuid).lock();
    if (!ptr)
        return std::unexpected(tr("No node with Uuid %1").arg(uuid.toString(QUuid::WithoutBraces)));
    else
        return ptr;
}

QModelIndex Model::index(int const row, int const column, QModelIndex const &parent) const {
    ZoneScoped;
    gsl_Expects(!parent.isValid() || parent.model() == this);

    QModelIndex result;

    auto parentNode = fromIndex(parent);
    assert(parentNode);
    if (auto child = parentNode->childOfRow(row, !editMode_); !child) {
        qCWarning(LoggingCategory) << "childOfRow" << child.error();
        return QModelIndex();
    } else {
        qCDebug(LoggingCategory) << "index(" << row << column << parentNode->name() << ") -> "
                                 << "createIndex(" << row << column << (*child)->name() << ")";

        result = createIndex(row, column, &**child);
        gsl_Ensures(result.model() == this);
        return result;
    }
}

QModelIndex Model::parent(QModelIndex const &child) const {
    ZoneScoped;
    gsl_Expects(!child.isValid() || child.model() == this);

    auto childNode = fromIndex(child);
    if (!child.isValid() && !childNode)
        return {};

    assert(childNode);
    if (childNode == root)
        return {};

    return toIndex(*childNode->parent());
}

int Model::rowCount(QModelIndex const &parent) const {
    ZoneScoped;
    gsl_Expects(!parent.isValid() || parent.model() == this);

    if (parent.column() > 0)
        return 0;

    auto node = fromIndex(parent);
    if (!node) {
        qCDebug(LoggingCategory) << "rowCount(<no node>) -> 0 (no node)";
        return 0;
    }

    assert(node);
    if (auto size = node->childrenCount(!editMode_); !size) {
        qCDebug(LoggingCategory) << "rowCount(" << node->name() << ") -> unexpected: " << size.error();
        return 0;
    } else {
        qCDebug(LoggingCategory) << "rowCount(" << node->name() << ") -> " << *size;
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

    Qt::ItemFlags flags = {};

    if (index.isValid()) {
        flags |= Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable;

        auto node = fromIndex(index);

        switch (static_cast<Column>(index.column())) {
            case Column::Name: {
                if (node->canSetName())
                    flags |= Qt::ItemFlag::ItemIsEditable;
                break;
            }
            case Column::Tag:
                if (node->canSetTags())
                    flags |= Qt::ItemFlag::ItemIsEditable;
                break;
        }

        if (node->canBeDragged())
            flags |= Qt::ItemFlag::ItemIsDragEnabled;

        if (node->canAcceptDrop())
            flags |= Qt::ItemFlag::ItemIsDropEnabled;
    }

    return flags;
}

QVariant Model::data(const QModelIndex &index, int role) const {
    ZoneScoped;
    gsl_Expects(!index.isValid() || index.model() == this);

    auto node = fromIndex(index);

    QVariant result;

    switch (role) {
        case Qt::ItemDataRole::DisplayRole: {
            switch (static_cast<Column>(index.column())) {
                case Column::Name:
                    result = node->name(false, editMode_);
                    break;
                case Column::Tag:
                    if (!editMode_) {
                        result = node->tags(Node::TagFlag::IncludeResolved)
                                | std::views::transform([](auto const &tag){ return tag.resolved; })
                                | std::views::join_with(QString(", "))
                                | std::ranges::to<QString>();
                    } else {
                        result = node->tags(Node::TagFlag::IncludeRaw | Node::TagFlag::IncludeResolved)
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
        case std::to_underlying(CustomItemDataRole::IconsRole): {
            switch (static_cast<Column>(index.column())) {
                case Column::Name:
                    result = QVariant::fromValue(node->icons());
                    break;
                case Column::Tag:
                    break;
            }
            break;
        }
        case Qt::ItemDataRole::EditRole: {
            switch (static_cast<Column>(index.column())) {
                case Column::Name:
                    result = node->name(true);
                    break;
                case Column::Tag:
                    result = node->tags(Node::TagFlag::IncludeRaw)
                            | std::views::transform([](auto const &tag){ return tag.raw; })
                            | std::views::join_with(QString(", "))
                            | std::ranges::to<QString>();
                    break;
            }
            break;
        }
        case Qt::ItemDataRole::ToolTipRole: {
            if (auto tooltip = node->tooltip(editMode_); !tooltip)
                qCCritical(LoggingCategory) << "Cannot generate tooltip for node" << node->uuid() << ":" << tooltip.error();
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
                if (auto active = node->active())
                    font.setBold(*active);

                if (highlightChangedAfterVersion_) {
                    if (auto highlight = node->lastChangeAfter(*highlightChangedAfterVersion_, true, true); !highlight)
                        qCCritical(LoggingCategory) << "Could not compare last change of a node:" << highlight.error();
                    else
                        font.setUnderline(*highlight);
                }
            }

            font.setItalic(node->isHidden());
            result = font;
            break;
        }
        case std::to_underlying(CustomItemDataRole::ExtendedBackgroundRole): {
            result = QVariant::fromValue(node->background(editMode_));
            break;
        }
    }

    if (role == Qt::DisplayRole)
        qCDebug(LoggingCategory) << "data(" << node->name() << ", DisplayRole) ->" << result;

    return result;
}

bool Model::setData(const QModelIndex &index, const QVariant &value, int role) {
    ZoneScoped;
    gsl_Expects(!index.isValid() || index.model() == this);

    switch (role) {
        case Qt::ItemDataRole::EditRole: {
            switch (static_cast<Column>(index.column())) {
                case Column::Name: {
                    auto node = fromIndex(index);
                    assert(node);
                    if (!node->setName(value.toString()))
                        return false;
                    emit dataChanged(index, index);
                    return true;
                }
                case Column::Tag: {
                    auto node = fromIndex(index);
                    assert(node);
                    if (!node->setTags(value.toString().split(",")
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
                case Column::Name: {
                    auto node = fromIndex(index);
                    assert(node);
                    if (node->setIcon(value.toString()))
                        emit dataChanged(index, index);
                    return true;
                }
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
    auto node = fromIndex(parent);
    assert(node);
    return node->canInsertChild(type);
}

std::expected<QModelIndex, QString> Model::insertNode(NodeType type, QModelIndex const &parent) {
    ZoneScoped;
    gsl_Expects(!parent.isValid() || parent.model() == this);

    auto parentNode = fromIndex(parent);
    assert(parentNode);
    auto parentNodeSerializable = std::dynamic_pointer_cast<NodeSerializable>(parentNode);
    if (!parentNodeSerializable)
        return std::unexpected(tr("Parent node is not a serializable node type subclass, but %1").arg(typeid(*parentNode).name()));

    gsl_Expects(parentNodeSerializable->canInsertChild(type));

    auto childNode = NodeHierarchical::createNode(type, *this, parentNodeSerializable);
    if (!childNode)
        return std::unexpected(childNode.error());

    auto row = parentNodeSerializable->childrenCount(false);
    if (!row)
        return std::unexpected(row.error());

    auto ptr = parentNodeSerializable->insertChild(*row, std::move(*childNode));
    if (!ptr)
        return std::unexpected(ptr.error());

    return createIndex(*row, 0, &**ptr);
}

[[nodiscard]] bool Model::canRemoveRow(QModelIndex const &index) {
    ZoneScoped;
    gsl_Expects(!index.isValid() || index.model() == this);
    auto node = fromIndex(index);
    assert(node);
    gsl_Expects(node->parent());
    return node->parent()->canRemoveChildren(index.row(), 1);
}

bool Model::removeRows(int const row, int const count, const QModelIndex &parent) {
    ZoneScoped;
    gsl_Expects(!parent.isValid() || parent.model() == this);
    auto node = fromIndex(parent);
    assert(node);
    node->removeChildren(row, count);
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
        auto node = fromIndex(index);
        assert(node);
        if (auto nodeSerializable = std::dynamic_pointer_cast<NodeSerializable>(node); !nodeSerializable) {
            qCCritical(LoggingCategory) << "Cannot generate MIME data for a not-stored node of type" << typeid(node).name();
            return nullptr;
        } else {
            auto result = nodeSerializable->save();

            if (!result) {
                qCCritical(LoggingCategory) << "Could not store node data:" << result.error();
                return nullptr;
            }

            QCborMap mimeEntry;
            mimeEntry[std::to_underlying(NodesMimeKey::SourceModelInstanceUuid)] = modelInstanceUuid_.toRfc4122();
            mimeEntry[std::to_underlying(NodesMimeKey::NodeData)] = *result;

            array.append(mimeEntry);
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
std::generator<std::expected<std::pair<QUuid, QCborMap>, QString>> parseMimeData(QMimeData const *const data) {
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

        auto mimeData = element.toMap();
        auto sourceModelInstaceIdValue = mimeData.take(std::to_underlying(NodesMimeKey::SourceModelInstanceUuid));
        if (!sourceModelInstaceIdValue.isByteArray()) {
            co_yield std::unexpected(
                    QObject::tr("Mime data source model uuid not a byte array but %1").arg(cborTypeToString(sourceModelInstaceIdValue.type())));
            co_return;
        }
        auto sourceModelInstanceId = QUuid::fromRfc4122(sourceModelInstaceIdValue.toByteArray());

        auto nodeData = mimeData.take(std::to_underlying(NodesMimeKey::NodeData)).toMap();
        co_yield std::make_pair(sourceModelInstanceId, nodeData);
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

    auto parentNode = fromIndex(parent);
    assert(parentNode);

    for (auto const &v: parseMimeData(data)) {
        if (!v) {
            qCCritical(LoggingCategory) << "Cannot parse mime data:" << v.error();
            return false;
        }

        QCborMap map = v->second;

        auto typeValue = map.take(std::to_underlying(Format::NodeKey::Type));
        if (!typeValue.isInteger()) {
            qCCritical(LoggingCategory) << "Mime data element type is not an integer but" << cborTypeToString(typeValue.type());
            return false;
        }

        // TODO: this won't work if we want to support multiple index drag&drop
        return parentNode->canAcceptDrop(static_cast<Format::NodeType>(typeValue.toInteger()));
    }

    return false;
}

bool Model::dropMimeData(
        QMimeData const *const data, Qt::DropAction const action, int row, int const column, QModelIndex const &parent
) {
    ZoneScoped;
    gsl_Expects(data);
    gsl_Expects(!parent.isValid() || parent.model() == this);
    (void)column; // gsl_Expects(column == -1); // TODO: which one?

    if (action != Qt::DropAction::MoveAction) // TODO: allow other actions, e.g. copy
        return false;

    auto parentNode = fromIndex(parent);
    assert(parentNode);
    auto parentNodeSerializable = std::dynamic_pointer_cast<NodeSerializable>(parentNode);
    if (!parentNodeSerializable) {
        qCCritical(LoggingCategory) << "Parent node should be a subclass of NodeStored, but got" << typeid(*parentNode).name();
        return false;
    }

    if (row == -1) {
        if (auto result = parentNodeSerializable->childrenCount(false); !result) {
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

        bool internalOp = v->first == modelInstanceUuid_;

        QCborMap map = v->second;

        // TODO: not sure what will be the exact effect if we get more than one row... which order will they appear in=
        //       perhaps on the sender side (mimeData()) we should sort by row first and then insert in the same
        //       order here?

        auto node = NodeHierarchical::load(map, *this, parentNodeSerializable, internalOp && action == Qt::MoveAction);
        if (!node) {
            qCCritical(LoggingCategory) << "Cannot create new node:" << node.error();
            return false;
        }

        if (!repopulationRequest.modifiedUuids)
            repopulationRequest.modifiedUuids.emplace();

        repopulationRequest.modifiedUuids->append(node->get()->uuid());

        if (auto result = parentNodeSerializable->insertChild(row, std::move(*node)); !result) {
            qCCritical(LoggingCategory) << "Cannot insert child node:" << result.error();
            return false;
        }
    }

    if (auto result = root->repopulateShadows(repopulationRequest); !result) {
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
        root = std::make_shared<NodeRoot>(*this);

        if (auto result = root->init(); !result)
            return std::unexpected(result.error());

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
        // prevent signals like insertChildrenBegin/End to be emitted
        QSignalBlocker block(this);
        auto result = NodeHierarchical::load(value, *this, nullptr);
        if (!result)
            return std::unexpected(result.error());

        auto newRoot = std::dynamic_pointer_cast<NodeRoot>(std::move(*result));
        if (!newRoot)
            return std::unexpected(tr("Invalid type of root node, expected Root but got %1").arg(
                    QMetaEnum::fromType<NodeType>().valueToKey(std::to_underlying((*result)->type()))
            ));

        root->deinit();
        root = std::move(newRoot);

        if (auto result = root->populateShadows(); !result)
            return std::unexpected(result.error());
    }

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
        emit layoutAboutToBeChanged();
        {
            QSignalBlocker block(this);

            if (auto result = root->unpopulateShadows(); !result)
                reportError("setEditMode unpopulateShadows", result.error());

            editMode_ = editMode;

            if (auto result = root->populateShadows(); !result)
                reportError("setEditMode populateShadows", result.error());
        }
        emit layoutChanged();
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

        if (auto result = root->visit(
                Node::VisitFlag::Recursive,
                [&](auto &&node) -> std::expected<bool, Error> {
            ZoneScoped;

            if (node->canSetActive()) {
                bool active = false;

                for (auto const &tag: tags) {
                    if (std::ranges::any_of(
                            node->tags(Node::TagFlag::IncludeResolved),
                            [&](auto const &nodeTag) { return nodeTag.resolved == tag; }
                    )) {
                        active = true;
                        break;
                    }
                }

                if (auto result = node->setActive(active); !result)
                    return std::unexpected(result.error());
            }

            return true;
        }); !result)
            return std::unexpected(result.error());
    }

    // potentially many tags might be modified in this function, that's why we block signals for the time of
    // visit and only fire dataChanged here
    if (auto result = root->visit(
            Node::VisitFlag::ExcludeSelf,
            [&](auto &&node) -> std::expected<bool, Error> {
        ZoneScoped;
        emit dataChanged(toIndex(node), toIndex(node));
        return true;
    }); !result)
        return std::unexpected(result.error());

    return result;
}

void Model::setRowHeight(int const rowHeight) {
    ZoneScoped;
    beginResetModel();
    rowHeight_ = rowHeight;
    endResetModel();
}

std::expected<QModelIndexList, QString> Model::setHighlightedTags(QStringList const &tags) {
    ZoneScoped;

    QModelIndexList collected;

    if (auto result = root->visit(
            Node::VisitFlag::Recursive,
            [&](auto &&node) -> std::expected<bool, Error> {
        ZoneScoped;

        auto nodeTags = node->tags()
                | std::views::transform([](auto const &v){ return v.resolved; })
                | std::ranges::to<QStringList>();
        if (node->canSetHighlighted()) {
            bool highlight = std::ranges::any_of(nodeTags, [&](QString const &tag) {
                return tags.contains(tag);
            });

            if (auto result = node->setHighlighted(highlight); !result)
                return std::unexpected(result.error());
            else if (highlight)
                collected.push_back(toIndex(node));
        }

        return true;
    }); !result)
        return std::unexpected(result.error());
    else
        return collected;
}

void Model::setHighlightChangedAfterVersion(std::optional<int> const &version) {
    highlightChangedAfterVersion_ = version;
}

void Model::setNextLibraryVersion(int const nextVersion) {
    nextLibraryVersion_ = nextVersion;
}

std::expected<void, Error> Model::invalidateTagCaches() const {
    allTags_ = std::nullopt;
    if (auto result = root->visit(Node::VisitFlag::Recursive, [](auto &&node)->std::expected<bool, Error>{
        node->invalidateTagCache();
        return true;
    }); !result)
        return std::unexpected(result.error());
    else
        return {};
}

QStringList Model::allTags() const {
    // TODO: this mutex only protects against multiple threads calling allTags().
    //       it doesn't however protect against a situation where on thread calls allTags()
    //       while another modifies the tags structure. To solve this, a separate mutex would
    //       be necessary to protect against all modifications of the nodes' structure.
    //       It can't be done on Library or Model level, as some modifications come from
    //       QAbstractItemModel calls, so we need to lock on them too.
    //
    // TODO: Also, one more reason to move iteration to a dedicated helper method - this
    //       method would ensure appropriate mutexes are locked in right places.
    QMutexLocker locker(&allTagsMutex_);

    if (!allTags_) {
        allTags_.emplace();
        if (auto result = root->visit(Node::VisitFlag::Recursive, [&](auto const &node)mutable->std::expected<bool, Error>{
            std::ranges::copy(
                    node->tags() | std::views::transform([](auto const &tag){ return tag.resolved; }),
                    std::back_inserter(*allTags_)
            );

            return true;
        }); !result)
            reportError("allTags visit", result.error(), false);
    }
    return *allTags_;
}

void Model::nodeUUIDRegister(std::shared_ptr<Node> const &node) {
    ZoneScoped;
    if (!node->isVirtual()) {
        qCDebug(LoggingCategory) << "UUID-to-node: add" << node->uuid() << ":" << node->name(true);
        assert(!uuidToNode_.contains(node->uuid()));
        uuidToNode_.emplace(node->uuid(), node);
    }
}

void Model::nodeUUIDChanged(std::shared_ptr<Node> const &node, QUuid const &oldUuid, bool const replaceExisting) {
    ZoneScoped;
    if (!node->isVirtual()) {
        qCDebug(LoggingCategory) << "UUID-to-node: change" << oldUuid << "->" << node->uuid() << ":" << node->name(true);
        assert(uuidToNode_.contains(oldUuid));
        assert(uuidToNode_.value(oldUuid).lock() == node);
        uuidToNode_.remove(oldUuid);

        auto newUuid = node->uuid();
        bool exists = uuidToNode_.contains(newUuid);

        if (replaceExisting) {
            if (exists) {
                assert(!uuidToNodeReplaced_.contains(newUuid));
                uuidToNodeReplaced_.insert(newUuid);
            }
        } else {
            assert(!exists);
        }

        uuidToNode_[newUuid] = node;
    }
}

void Model::nodeUUIDUnregister(std::shared_ptr<Node> const &node) {
    ZoneScoped;
    if (!node->isVirtual()) {
        auto uuid = node->uuid();

        qCDebug(LoggingCategory) << "UUID-to-node: remove" << uuid << ":" << node->name(true);

        if (uuidToNodeReplaced_.contains(uuid)) {
            uuidToNodeReplaced_.remove(uuid);
        } else {
            assert(uuidToNode_.contains(uuid));
            assert(uuidToNode_.value(uuid).lock() == node);
            uuidToNode_.remove(uuid);
        }
    }
}
}
