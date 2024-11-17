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
#pragma once
#include "Format.hpp"

#include "../IconIdentifier.hpp"
#include "../Utility.hpp"

namespace TagLibrary {
class Model;

using NodeType = Format::NodeType;

class Node: public QObject {
    Q_OBJECT

public:
    Node(Node const &other) = delete;
    Node(Node &&other) = delete;
    Node& operator=(Node const &other) = delete;
    Node& operator=(Node &&other) = delete;

    Node(Model &model);
    virtual ~Node();

    [[nodiscard]] virtual std::expected<void, QString> init();
    virtual void deinit();

    // model access
    [[nodiscard]] auto &model(this auto &&self) {
        return self.model_;
    }

    // parent access
    [[nodiscard]] virtual Node const *parent() const = 0;

    // parent modification
    // TODO: would be logical to also have remove() here, but our Library code currently relies on Model on that
    [[nodiscard]] virtual bool canRemove() const;

    // children access
    [[nodiscard]] virtual std::expected<int, QString> rowOfChild(Node const &node) const = 0;
    [[nodiscard]] virtual std::expected<std::reference_wrapper<Node>, QString> childOfRow(int row) const = 0;
    [[nodiscard]] virtual std::expected<int, QString> childrenCount() const = 0;

    // children modification
    [[nodiscard]] virtual bool canInsertChild(NodeType childType) const;
    [[nodiscard]] virtual std::expected<Node *, QString> insertChild(int row, std::unique_ptr<Node> &&node);
    [[nodiscard]] virtual bool canRemoveChildren(int row, int count) const;
    virtual void removeChildren(int row, int count);
    [[nodiscard]] virtual bool canBeDragged() const;
    [[nodiscard]] virtual bool canAcceptDrop() const; // does it accept drops at all?
    [[nodiscard]] virtual bool canAcceptDrop(NodeType type) const; // does it accept drops of this node type?

    // fields
    [[nodiscard]] virtual QString name(bool raw = false, bool editMode = false) const;
    [[nodiscard]] virtual bool canSetName() const;
    [[nodiscard]] virtual bool setName(QString const &name);

    [[nodiscard]] virtual std::vector<IconIdentifier> icons() const;
    [[nodiscard]] virtual bool canSetIcon() const;
    [[nodiscard]] virtual bool setIcon(QString const &path);

    [[nodiscard]] virtual std::optional<QUuid> linkTo() const;
    [[nodiscard]] virtual bool canLinkTo() const;
    [[nodiscard]] virtual bool canBeLinkedTo() const;
    [[nodiscard]] virtual std::expected<void, QString> setLinkTo(QUuid const &uuid);

    enum class TagFlag {
        NoFlags = 0x0,
        IncludeRaw = 0x1,
        IncludeResolved = 0x2,
    };
    Q_DECLARE_FLAGS(TagFlags, TagFlag);
    struct Tag {
        QString raw;
        QString resolved;

        bool operator==(Tag const &other) const;
    };
    [[nodiscard]] std::vector<Tag> tags(TagFlags flags = TagFlag::IncludeResolved) const;
protected:
    [[nodiscard]] virtual std::vector<Tag> generateTags(TagFlags flags = TagFlag::IncludeResolved) const;
public:
    void invalidateTagCache() const;
    [[nodiscard]] virtual bool canSetTags() const;
    [[nodiscard]] virtual std::expected<void, QString> setTags(QStringList const &tags);
    [[nodiscard]] virtual QStringList resolveChildTag(QString const &tag) const;

public:
    [[nodiscard]] virtual QString comment() const;
    [[nodiscard]] virtual bool canSetComment() const;
    [[nodiscard]] virtual std::expected<void, QString> setComment(QString const &comment);

    [[nodiscard]] virtual std::optional<bool> active() const;
    [[nodiscard]] virtual bool canSetActive() const;
    [[nodiscard]] virtual std::expected<void, QString> setActive(bool active);

    [[nodiscard]] virtual std::optional<bool> highlighted() const;
    [[nodiscard]] virtual bool canSetHighlighted() const;
    [[nodiscard]] virtual std::expected<void, QString> setHighlighted(bool highlighted);

    [[nodiscard]] virtual bool isLinkingNode() const = 0;

    [[nodiscard]] virtual QUuid uuid() const = 0;

    [[nodiscard]] virtual NodeType type() const = 0;
    [[nodiscard]] virtual bool isVirtual() const;
    [[nodiscard]] virtual bool isReplaced() const;

    [[nodiscard]] virtual bool isHidden() const;
    [[nodiscard]] virtual bool canSetHidden() const;
    [[nodiscard]] virtual std::expected<void, QString> setHidden(bool hidden);

    [[nodiscard]] virtual std::optional<int> lastChangeVersion() const;
    virtual void setLastChangeVersion(int version);
    [[nodiscard]] std::expected<bool, Error> lastChangeAfter(int version, bool anyChild, bool anyParent) const;

    [[nodiscard]] virtual std::vector<QBrush> background(bool editMode) const;

    [[nodiscard]] std::expected<QString, QString> tooltip(bool editMode) const;

    enum class PathFlag {
        NoFlags = 0x0,
        IncludeTypes = 0x1,
        IncludeNames = 0x2,
        IncludeUuids = 0x4,
        IncludeEverything = IncludeTypes | IncludeNames | IncludeUuids
    };
    Q_DECLARE_FLAGS(PathFlags, PathFlag);
    [[nodiscard]] QString path(PathFlags flags = PathFlag::IncludeNames) const;

    bool deinitialized() const;

    struct RepopulationRequest {
        // nullopt means there's no specific UUIDs modified - meaning all links should be considered outdated
        // empty list means no nodes were modified - meaning no links should be considered outdated
        // non-empty list means only these nodes were modified - meaning only links to these (or links themselves) should be considered outdated
        std::optional<QList<QUuid>> modifiedUuids;
    };
    [[nodiscard]] virtual std::expected<void, QString> repopulateLinked(RepopulationRequest const &repopulationRequest = {});
    [[nodiscard]] std::expected<void, QString> repopulateLinkedRecursive(RepopulationRequest const &repopulationRequest = {});

protected:
    struct VerifyContext {
        QSet<QUuid> uuids;
        QSet<QString> resolvedTags;
    };
    [[nodiscard]] virtual std::expected<void, QStringList> verify(VerifyContext &context) const;
    [[nodiscard]] std::expected<void, QStringList> verifyRecursive(VerifyContext &context) const;

public:
    [[nodiscard]] virtual std::expected<void, QStringList> verify() const;

signals:
    void aboutToRemove();
    void persistentDataChanged();
    void dataChanged();
    void activeChanged(bool active);
    void insertChildrenBegin(int first, int last);
    void insertChildrenEnd(int first, int last);
    void removeChildrenBegin(int first, int last);
    void removeChildrenEnd(int first, int last);

private:
    Model &model_;
    bool deinitialized_ = false;

    mutable QHash<TagFlags, std::vector<Tag>> tagCache_;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(Node::TagFlags);
Q_DECLARE_OPERATORS_FOR_FLAGS(Node::PathFlags);
}
