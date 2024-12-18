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
#include "Node.hpp"

#include "Logging.hpp"
#include "Model.hpp"
#include "NodeRoot.hpp"
#include "NodeCollection.hpp"
#include "NodeObject.hpp"
#include "NodeLink.hpp"
#include "NodeShadow.hpp"

namespace TagLibrary {
Node::Node(Model &model): model_(model) {
    ZoneScoped;
    connect(this, &Node::dataChanged, this, [this]{
        ZoneScoped;
        auto [index1, index2] = model_.toIndexRange(*this);
        emit model_.dataChanged(index1, index2);
    });
    connect(this, &Node::persistentDataChanged, this, [this]{
        ZoneScoped;
        emit dataChanged();
        setLastChangeVersion(model_.nextLibraryVersion_);
        emit model_.persistentDataChanged();
    });
    connect(this, &Node::activeChanged, this, [this](bool const active){
        ZoneScoped;
        emit model_.activeChanged(*this, active);
    });
    connect(this, &Node::insertChildrenBegin, this, [this](int const first, int const last){
        ZoneScoped;
        emit model_.beginInsertRows(model_.toIndex(*this), first, last);
    });
    connect(this, &Node::insertChildrenEnd, this, [this]{
        ZoneScoped;
        emit model_.endInsertRows();
        emit model_.persistentDataChanged();
    });
    connect(this, &Node::removeChildrenBegin, this, [this](int const first, int const last){
        ZoneScoped;
        emit model_.beginRemoveRows(model_.toIndex(*this), first, last);
    });
    connect(this, &Node::removeChildrenEnd, this, [this]{
        ZoneScoped;
        emit model_.endRemoveRows();
        emit model_.persistentDataChanged();
    });
}

Node::~Node() {
    assert(deinitialized_ && "deinit() should be called before destroying a node");
}

std::expected<void, QString> Node::init() {
    ZoneScoped;
    assert(!initialized_);
    assert(!deinitialized_);
    model().nodeUUIDRegister(shared_from_this());
    initialized_ = true;
    return {};
}

void Node::deinit() {
    ZoneScoped;
    assert(initialized_);
    assert(!deinitialized_);
    emit aboutToRemove();
    model().nodeUUIDUnregister(shared_from_this());
    deinitialized_ = true;
}

bool Node::canRemove() const {
    return false;
}

bool Node::canInsertChild(NodeType const) const {
    return false;
}

std::expected<std::shared_ptr<Node>, QString> Node::insertChild(int const, [[maybe_unused]] std::shared_ptr<Node> &&node) {
    assert(!canInsertChild(node->type()) && "If canSet* has been overriden to return true, also set* should be overriden");
    assert(canInsertChild(node->type()) && "This function shouldn't be called if the respective canSet* returned false");
    return std::unexpected("insertChild not implemented");
}

bool Node::canRemoveChildren(int const, int const) const {
    return false;
}

void Node::removeChildren([[maybe_unused]] int const row, [[maybe_unused]] int const count) {
    assert(!canRemoveChildren(row, count) && "If canSet* has been overriden to return true, also set* should be overriden");
    assert(canRemoveChildren(row, count) && "This function shouldn't be called if the respective canSet* returned false");
}

bool Node::canBeDragged() const {
    return false;
}

bool Node::canAcceptDrop() const {
    return false;
}

bool Node::canAcceptDrop(NodeType const) const {
    return false;
}

QString Node::name(bool const, bool const) const {
    return QString();
}

bool Node::canSetName() const {
    return false;
}

bool Node::setName(QString const &) {
    assert(!canSetName() && "If canSet* has been overriden to return true, also set* should be overriden");
    assert(canSetName() && "This function shouldn't be called if the respective canSet* returned false");
    return false;
}

std::vector<IconIdentifier> Node::icons() const {
    return {};
}

bool Node::canSetIcon() const {
    return false;
}

bool Node::setIcon(const QString &) {
    assert(!canSetIcon() && "If canSet* has been overriden to return true, also set* should be overriden");
    assert(canSetIcon() && "This function shouldn't be called if the respective canSet* returned false");
    return false;
}

std::optional<QUuid> Node::linkTo() const {
    return std::nullopt;
}

bool Node::canLinkTo() const {
    return false;
}

bool Node::canBeLinkedTo() const {
    return false;
}

std::expected<void, QString> Node::setLinkTo(QUuid const &) {
    assert(!canLinkTo() && "If canSet* has been overriden to return true, also set* should be overriden");
    assert(canLinkTo() && "This function shouldn't be called if the respective canSet* returned false");
    return std::unexpected(tr("Cannot set link on this object type"));
}

bool Node::Tag::operator==(Tag const &other) const {
    return raw == other.raw && resolved == other.resolved;
}

std::vector<Node::Tag> Node::tags(TagFlags const flags) const {
    auto it = tagCache_.find(flags);
    if (it == tagCache_.end())
        it = tagCache_.emplace(flags, generateTags(flags));
    return *it;
}

std::vector<Node::Tag> Node::generateTags(TagLibrary::Node::TagFlags const) const {
    return {};
}

void Node::invalidateTagCache() const {
    tagCache_.clear();
}

bool Node::canSetTags() const {
    return false;
}

std::expected<void, QString> Node::setTags(QStringList const &) {
    assert(!canSetTags() && "If canSet* has been overriden to return true, also set* should be overriden");
    assert(canSetTags() && "This function shouldn't be called if the respective canSet* returned false");
    return std::unexpected(tr("Cannot set tags on this object type"));
}

QStringList Node::resolveChildTag(QString const &tag) const {
    return {tag};
}

QString Node::comment() const {
    return {};
}

bool Node::canSetComment() const {
    return false;
}

std::expected<void, QString> Node::setComment(QString const &) {
    assert(!canSetComment() && "If canSet* has been overriden to return true, also set* should be overriden");
    assert(canSetComment() && "This function shouldn't be called if the respective canSet* returned false");
    return std::unexpected(tr("Cannot set comment on this object type"));
}

std::optional<bool> Node::active() const {
    return std::nullopt;
}

bool Node::canSetActive() const {
    return false;
}

std::expected<void, QString> Node::setActive(bool const) {
    assert(!canSetActive() && "If canSet* has been overriden to return true, also set* should be overriden");
    assert(canSetActive() && "This function shouldn't be called if the respective canSet* returned false");
    return std::unexpected(tr("Cannot set active on this object type"));
}

std::optional<bool> Node::highlighted() const {
    return std::nullopt;
}

bool Node::canSetHighlighted() const {
    return false;
}

std::expected<void, QString> Node::setHighlighted(bool const) {
    assert(!canSetHighlighted() && "If canSet* has been overriden to return true, also set* should be overriden");
    assert(canSetHighlighted() && "This function shouldn't be called if the respective canSet* returned false");
    return std::unexpected(tr("Cannot set highlighted on this object type"));
}

bool Node::isVirtual() const {
    return false;
}

bool Node::isReplaced() const {
    return false;
}

bool Node::isHidden() const {
    return false;
}

bool Node::canSetHidden() const {
    return false;
}

std::expected<void, QString> Node::setHidden(bool const) {
    assert(!canSetHidden() && "If canSet* has been overriden to return true, also set* should be overriden");
    assert(canSetHidden() && "This function shouldn't be called if the respective canSet* returned false");
    return std::unexpected(tr("Cannot set hidde flag on this object type"));
}

std::optional<int> Node::lastChangeVersion() const {
    return std::nullopt;
}

void Node::setLastChangeVersion(int const) {}

std::expected<bool, Error> Node::lastChangeAfter(int const version, bool const anyChild, bool const anyParent) const {
    if (anyParent && parent()) {
        if (auto v = parent()->lastChangeAfter(version, false, true); !v)
            return v;
        else if (*v)
            return true;
    }

    if (!anyChild) {
        // non-recursive - only check if we were changed after the "version"
        if (auto v = lastChangeVersion())
            return *v > version;
        else
            // no information about last change version, so assume there was NO change
            return false;
    } else {
        // recursive - first, use the non-recursive approach and if we were changed, no point
        // in checking the children too
        if (auto result = lastChangeAfter(version, false, anyParent); !result)
            return result;
        else if (*result)
            return true;

        // nope, we were not changed, but perhaps out child was? Go check
        if (auto result = find(VisitFlag::Recursive | VisitFlag::ExcludeSelf, [&](auto &&node)->std::expected<bool, Error>{
            if (auto result = node->lastChangeAfter(version, true, false); !result)
                return result;
            else
                return *result;
        }); !result)
            return std::unexpected(result.error());
        else
            return static_cast<bool>(result);
    }
}

std::vector<QBrush> Node::background(bool const editMode) const {
    ZoneScoped;

    std::vector<QBrush> result;

    if (!editMode) {
        auto active = false;
        auto highlighted = false;

        if (auto a = this->active(); a && *a)
            active = true;

        if (auto h = this->highlighted(); h && *h)
            highlighted = true;

        bool anyDescendantActive = false;
        bool anyDescendantHighlighted = false;

        if (auto result = visit(VisitFlag::Recursive, [&](auto const &node)->std::expected<bool, Error>{
            if (!anyDescendantActive) {
                if (auto active = node->active(); active && *active)
                    anyDescendantActive = true;
            }

            if (!anyDescendantHighlighted) {
                if (auto highlighted = node->highlighted(); highlighted && *highlighted)
                    anyDescendantHighlighted = true;
            }

            // if we already know we've got an active descendant and a highlighted descendant,
            // then we don't have to look for anything more
            return !anyDescendantActive || !anyDescendantHighlighted;
        }); !result) {
            reportError("Node::background visit", result.error());
            return {};
        }

        static constexpr QColor activeColor = QColor(0, 255, 0, 128);

        if (active)
            result.emplace_back(QBrush(activeColor));

        if (anyDescendantActive)
            result.emplace_back(QBrush(activeColor, Qt::BrushStyle::FDiagPattern));

        static constexpr QColor highlightColor = QColor(0, 0, 255, 128);

        if (highlighted)
            result.emplace_back(QBrush(highlightColor));

        if (anyDescendantHighlighted)
            result.emplace_back(QBrush(highlightColor, Qt::BrushStyle::BDiagPattern));
    }

    return result;
}

std::expected<QString, QString> Node::tooltip(bool const editMode) const {
    ZoneScoped;

    if (!editMode) {
        QString tooltip;

        if (auto comment = this->comment(); !comment.isEmpty())
            tooltip += QString("%1").arg(comment);

        QStringList activeTagsLines;
        if (auto result = visit(VisitFlag::Recursive, [&](auto &&node)->std::expected<bool, Error>{
                if (auto active = node->active(); active && *active)
                    activeTagsLines.emplace_back(node->tags()
                            | std::views::transform([](auto const &tag){ return tag.resolved; })
                            | std::views::join_with(QString(", "))
                            | std::ranges::to<QString>());
                return true;
        }); !result) {
            return std::unexpected(result.error());
        } else {
            auto activeTags = activeTagsLines
                    | std::views::join_with(QString("<br>"))
                    | std::ranges::to<QString>();

            if (!activeTags.isEmpty()) {
                if (!tooltip.isEmpty())
                    tooltip += "<br><br>";
                tooltip += QString("Active tags:<br>%1").arg(activeTags);
            }
        }

        return tooltip;
    } else {
        return {};
    }
}

QString Node::path(PathFlags flags) const {
    ZoneScoped;

    QString result;

    if (auto p = parent())
        result += QString("%1/").arg(p->path(flags));

    if (flags & PathFlag::IncludeTypes)
        result += QString("<%1>").arg(QMetaEnum::fromType<NodeType>().valueToKey(std::to_underlying(type())));

    if (flags & PathFlag::IncludeNames)
        result += name(true);

    if (flags & PathFlag::IncludeUuids)
        result += QString("[%1]").arg(uuid().toString(QUuid::WithoutBraces));

    return result;
}

bool Node::deinitialized() const {
    return deinitialized_;
}

std::expected<void, Error> Node::populateShadows() {
    return populateShadowsImpl();
}

std::expected<void, Error> Node::unpopulateShadows() {
    return unpopulateShadowsImpl();
}

void Node::uuidChanged(QUuid const &oldUuid, bool const replaceExisting) {
    model().nodeUUIDChanged(shared_from_this(), oldUuid, replaceExisting);
}

std::expected<void, QStringList> Node::verify(VerifyContext &context) const {
    gsl_Expects(&model_);  // this is not an acceptable circumstance ever
    gsl_Expects(!deinitialized_);  // this one too

    QStringList unexpected;

    if (!isVirtual()) {
        if (context.uuids.contains(uuid()))
            unexpected.append(tr("UUID occurs more than once: %1").arg(uuid().toString(QUuid::WithoutBraces)));
        else
            context.uuids.insert(uuid());

        for (auto const &tag: tags() | std::views::transform([&](auto const &v){ return v.resolved; })) {
            if (context.resolvedTags.contains(tag))
                unexpected.append(tr("Tag (resolved) occurs more than once: %1").arg(tag));
            else
                context.resolvedTags.insert(tag);
        }
    }

    if (!unexpected.isEmpty())
        return std::unexpected(unexpected);
    else
        return {};
}

std::expected<void, QStringList> Node::verifyRecursive(VerifyContext &context) const {
    QStringList unexpected;

    if (auto result = visit(VisitFlag::Recursive, [&](auto &&node)->std::expected<bool, Error>{
        if (auto result = node->verify(context); !result)
            unexpected.append(result.error());
        return true;
    }); !result)
        unexpected.emplace_back(result.error());

    if (!unexpected.isEmpty())
        return std::unexpected(unexpected);
    else
        return {};
}

std::expected<void, QStringList> Node::verify() const {
    VerifyContext context;
    return verifyRecursive(context);
}
}
