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
#include "Library.hpp"

#include "CommentEditor.hpp"
#include "Format.hpp"
#include "Logging.hpp"
#include "Model.hpp"
#include "NodeSerializable.hpp"
#include "SelectionHelperProxyModel.hpp"

#include "../Utility.hpp"

#include "ui_Library.h"

namespace TagLibrary {
Library::Library(Qt::WindowFlags const flags):
        QDockWidget(nullptr, flags),
        ui(std::make_unique<Ui_Library>()),
        libraryModel_(std::make_unique<Model>()),
        model_(std::make_unique<SelectionHelperProxyModel>()) {}

std::expected<void, QString> Library::init() {
    ZoneScoped;

    ui->setupUi(this);
    ui->labelTreeNotification->hide();
    ui->buttonTreeNotificationCancel->hide();
    connect(ui->buttonTreeNotificationCancel, &QToolButton::clicked, this, [this]{
        finishSelection(true);
    });

    model_->setSourceModel(&*libraryModel_);

    connect(&*libraryModel_, &Model::persistentDataChanged, this, &Library::contentChanged);

    connect(&*libraryModel_, &Model::activeChanged, this, [this](Node const &node, bool const active){
        ZoneScoped;
        emit tagsActiveChanged(node.tags(Node::TagFlag::IncludeResolved)
                               | std::views::transform([](auto const &tag){ return tag.resolved; })
                               | std::ranges::to<QStringList>()
                               , active);
    });

    ui->treeTags->setModel(&*model_);
    ui->treeTags->setRootIndex(QModelIndex());

    if (auto result = libraryModel_->resetRoot(); !result)
        return std::unexpected(result.error());

    auto createNode = [this](NodeType const nodeType){
        ZoneScoped;
        auto index = ui->treeTags->currentIndex();
        auto sourceIndex = model_->mapToSource(index);
        if (auto result = libraryModel_->insertNode(nodeType, sourceIndex); !result) {
            qCWarning(LoggingCategory);
            QMessageBox::critical(
                    this,
                    tr("Create failed"),
                    tr("Could not create a new node: %1").arg(result.error())
            );
        } else {
            ui->treeTags->expand(index);
            ui->treeTags->edit(*result);
        }
    };

    connect(ui->actionCreateCollection, &QAction::triggered, this, [createNode]{ createNode(NodeType::Collection); });
    connect(ui->actionCreateObject, &QAction::triggered, this, [createNode]{ createNode(NodeType::Object); });
    connect(ui->actionCreateLink, &QAction::triggered, this, [createNode]{ createNode(NodeType::Link); });

    connect(ui->actionDelete, &QAction::triggered, this, [this]{
        ZoneScoped;
        auto index = ui->treeTags->currentIndex();
        if (index.isValid()) {
            auto sourceIndex = model_->mapToSource(index);
            auto count = libraryModel_->recursiveRowCount(sourceIndex);

            QMessageBox messageBox;
            messageBox.setIcon(count == 0 ? QMessageBox::Question : QMessageBox::Warning);
            messageBox.setWindowTitle(tr("Delete elment?"));
            messageBox.setText((count == 0
                ? tr("Delete <b>%1</b>?")
                : tr("Delete <b>%1</b> and <b>all its %2 descendant elements</b>?").arg("%1", QString::number(count))
            ).arg(model_->data(index, Qt::ItemDataRole::DisplayRole).toString()));
            messageBox.setStandardButtons(QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No);
            messageBox.setDefaultButton(QMessageBox::StandardButton::No);
            messageBox.exec();
            if (messageBox.result() == QMessageBox::StandardButton::Yes)
                model_->removeRow(index.row(), index.parent());
        }
    });

    connect(ui->actionRename, &QAction::triggered, this, [this]{
        ZoneScoped;
        ui->treeTags->edit(ui->treeTags->currentIndex());
    });

    connect(ui->actionLinkTo, &QAction::triggered, this, [this]{
        ZoneScoped;
        ui->buttonTreeNotificationCancel->show();
        ui->labelTreeNotification->setText(tr("Select element to link to"));
        ui->labelTreeNotification->show();
        auto link = ui->treeTags->currentIndex();
        selectionJumpBackTo = link;
        auto isSelectable = [this](auto const &candidate) {
            auto sourceCandidate = model_->mapToSource(candidate);
            return libraryModel_->fromIndex(sourceCandidate).canBeLinkedTo();
        };
        model_->requestSelection(isSelectable);
        ui->treeTags->requestSelection(
                isSelectable,
                [this, link](auto const &target){
                    ZoneScoped;
                    auto &linkNode = libraryModel_->fromIndex(model_->mapToSource(link));
                    auto &targetNode = libraryModel_->fromIndex(model_->mapToSource(target));
                    if (QMessageBox::question(
                            this, tr("Link"), tr("Link <b>%1</b> to <b>%2</b>?").arg(linkNode.name(true), targetNode.name(true))
                    ) == QMessageBox::StandardButton::Yes) {
                        if (auto result = linkNode.setLinkTo(targetNode.uuid()); !result) {
                            QMessageBox::critical(
                                    this, tr("Linking failed"), tr("Could not link <b>%1</b> (%2) to <b>%3</b> (%4)<br><br>%4").arg(
                                            linkNode.name(true), linkNode.uuid().toString(QUuid::WithoutBraces),
                                            targetNode.name(true), targetNode.uuid().toString(QUuid::WithoutBraces),
                                            result.error()
                                    )
                            );
                            return false;
                        }
                        finishSelection(false);
                        return true;
                    } else {
                        return false;
                    }
                }
        );
    });

    connect(ui->actionLinkReset, &QAction::triggered, this, [this]{
        ZoneScoped;
        auto link = ui->treeTags->currentIndex();
        auto &linkNode = libraryModel_->fromIndex(model_->mapToSource(link));
        assert(linkNode.linkTo());
        auto targetNode = libraryModel_->fromUuid(*linkNode.linkTo());
        if (!targetNode) {
            qCCritical(LoggingCategory) << "unlink get target failed:" << targetNode.error();
            return;
        }

        if (QMessageBox::question(
                this, tr("Link"), tr("Unlink <b>%1</b> from <b>%2</b>?").arg(linkNode.name(true), targetNode->get().name(true))
        ) == QMessageBox::StandardButton::Yes) {
            if (auto result = linkNode.setLinkTo(QUuid()); !result) {
                QMessageBox::critical(
                        this, tr("Linking failed"),
                        tr("Could not unlink <b>%1</b> (%2) from <b>%3</b> (%4)<br><br>%4").arg(
                                linkNode.name(true), linkNode.uuid().toString(QUuid::WithoutBraces),
                                targetNode->get().name(true), targetNode->get().uuid().toString(QUuid::WithoutBraces),
                                result.error()
                        )
                );
            }
        }
    });

    connect(ui->actionComment, &QAction::triggered, this, [this] {
        ZoneScoped;

        auto index = ui->treeTags->selectionModel()->currentIndex();
        auto sourceIndex = model_->mapToSource(index);
        auto &node = libraryModel_->fromIndex(sourceIndex);

        auto dialog = CommentEditor::create(this);
        if (!dialog) {
            QMessageBox::critical(
                    this,
                    tr("Comment editor failed"),
                    tr("Could not open comment editor: %1").arg(dialog.error())
            );
        } else {
            (*dialog)->setHeaderLabel(node.name());
            (*dialog)->setText(node.comment());
            (*dialog)->exec();

            if ((*dialog)->result() == QDialog::Accepted) {
                assert(node.canSetComment());
                if (auto result = node.setComment((*dialog)->text()); !result)
                    QMessageBox::critical(
                            this,
                            tr("Comment change failed"),
                            tr("Could not change element comment: %1").arg(result.error())
                    );
            }
        }
    });

    connect(ui->actionToggleActive, &QAction::triggered, this, [this]{
        ZoneScoped;
        auto index = ui->treeTags->selectionModel()->currentIndex();
        auto sourceIndex = model_->mapToSource(index);
        auto &node = libraryModel_->fromIndex(sourceIndex);
        auto active = node.active();
        gsl_Expects(active);
        auto newValue = !*active;
        node.setActive(newValue);
        ui->actionToggleActive->setChecked(newValue);
    });

    auto addCreateActions = [this](QMenu &menu){
        ZoneScoped;
        menu.addAction(ui->actionCreateCollection);
        menu.addAction(ui->actionCreateObject);
        menu.addAction(ui->actionCreateLink);
    };

    connect(ui->treeTags, &QTreeView::customContextMenuRequested, this, [this, addCreateActions](QPoint const &pos){
        ZoneScoped;
        QMenu menu;
        if (ui->actionToggleEditMode->isChecked()) {
            addCreateActions(menu);
            menu.addSeparator();
            menu.addAction(ui->actionDelete);
            menu.addSeparator();
            menu.addAction(ui->actionRename);
            menu.addMenu(&userIconMenu);
            menu.addAction(ui->actionLinkTo);
            menu.addAction(ui->actionLinkReset);
            menu.addAction(ui->actionComment);
        } else {
            menu.addAction(ui->actionToggleActive);
        }
        menu.exec(ui->treeTags->mapToGlobal(pos));
    });

    ui->buttonEditMode->setDefaultAction(ui->actionToggleEditMode);

    connect(ui->buttonAdd, &QToolButton::clicked, this, [this, addCreateActions] {
        ZoneScoped;
        QMenu menu;
        addCreateActions(menu);
        menu.exec(ui->buttonAdd->mapToGlobal(QPoint(0, ui->buttonAdd->height())));
    });

    ui->buttonDelete->setDefaultAction(ui->actionDelete);
    ui->buttonRename->setDefaultAction(ui->actionRename);
    ui->buttonLinkTo->setDefaultAction(ui->actionLinkTo);
    ui->buttonLinkReset->setDefaultAction(ui->actionLinkReset);
    ui->buttonComment->setDefaultAction(ui->actionComment);

    ui->buttonToggleActive->setDefaultAction(ui->actionToggleActive);

    userIconMenu.setTitle(tr("Set icon"));
    userIconMenu.setIcon(ui->buttonSetIcon->icon());
    userIconMenu.addAction(tr("Reset to default"))->setData(QVariant());

    auto populateIconMenu = [](this auto &&self, QMenu &menu, QString const &dir)->void{
        ZoneScoped;
        auto it = QDirIterator(dir);
        while (it.hasNext()) {
            auto info = it.nextFileInfo();
            if (info.isDir()) {
                self(*menu.addMenu(info.fileName()), info.filePath());
            } else {
                auto path = info.filePath();
                menu.addAction(QIcon(path), info.fileName())->setData(path); // TODO: nicer icons names?
            }
        }
    };
    populateIconMenu(userIconMenu, ":/user-icons");

    connect(&userIconMenu, &QMenu::triggered, this, [this](QAction *const action){
        ZoneScoped;
        auto index = ui->treeTags->currentIndex();
        if (index.isValid())
            if (!model_->setData(index, action->data(), Qt::ItemDataRole::DecorationRole))
                QMessageBox::critical(this, tr("Change failed"), tr("Could not change the element icon"));
    });

    connect(ui->buttonSetIcon, &QToolButton::clicked, this, [this]{
        ZoneScoped;
        userIconMenu.exec(ui->buttonSetIcon->mapToGlobal(QPoint(0, ui->buttonSetIcon->height())));
    });

    auto updateSelection = [this]{
        ZoneScoped;
        bool editMode = ui->actionToggleEditMode->isChecked();

        QString editableBorderStyle;
        if (editMode)
            editableBorderStyle = QString("border: 2px solid blue; border-radius: 5px;");
        else
            editableBorderStyle = QString("padding: 2px;");

        ui->treeTags->setStyleSheet(editMode ? QString("QTreeView {%1}").arg(editableBorderStyle) : "");
        ui->buttonsEditMode->setStyleSheet(editMode ? QString("QWidget#%1 {%2}").arg(ui->buttonsEditMode->objectName(), editableBorderStyle) : "");

        libraryModel_->setEditMode(editMode);

        auto index = ui->treeTags->selectionModel()->currentIndex();
        auto indexSource = model_->mapToSource(index);
        auto &node = libraryModel_->fromIndex(indexSource);

        ui->buttonsNormalMode->setVisible(!editMode);
        ui->buttonsEditMode->setVisible(editMode);
        ui->actionCreateCollection->setEnabled(editMode && node.canInsertChild(NodeType::Collection));
        ui->actionCreateObject->setEnabled(editMode && node.canInsertChild(NodeType::Object));
        ui->actionCreateLink->setEnabled(editMode && node.canInsertChild(NodeType::Link));
        ui->buttonAdd->setEnabled(editMode && (ui->actionCreateCollection->isEnabled() || ui->actionCreateObject->isEnabled() || ui->actionCreateLink->isEnabled()));
        ui->actionDelete->setEnabled(editMode && node.canRemove());
        ui->actionRename->setEnabled(editMode && node.canSetName());
        ui->buttonSetIcon->setEnabled(editMode && node.canSetIcon());
        userIconMenu.setEnabled(ui->buttonSetIcon->isEnabled());
        ui->actionLinkTo->setEnabled(editMode && node.canLinkTo());
        ui->actionLinkReset->setEnabled(editMode && node.canLinkTo() && node.linkTo() && !node.linkTo()->isNull());
        ui->actionComment->setEnabled(editMode && node.canSetComment());

        ui->actionToggleActive->setEnabled(!editMode && node.canSetActive());
        if (ui->actionToggleActive->isEnabled()) {
            gsl_Expects(node.active());
            ui->actionToggleActive->setChecked(*node.active());
        } else {
            ui->actionToggleActive->setChecked(false);
        }

        auto editTriggers = ui->treeTags->editTriggers();
        if (editMode)
            editTriggers |= QAbstractItemView::EditTrigger::DoubleClicked;
        else
            editTriggers &= ~QAbstractItemView::EditTrigger::DoubleClicked;

        ui->treeTags->setEditTriggers(editTriggers);

        if (indexSource.isValid()) {
            ui->labelDescriptionTitle->setText(node.name());

            QString description;
            description += tr("<b>Name:</b> %1<br>").arg(node.name(true).toHtmlEscaped());

            auto tags = node.tags(Node::TagFlag::IncludeRaw | Node::TagFlag::IncludeResolved);
            QString formattedTags;

            auto formatTag = [](Node::Tag const &tag) {
                if (tag.raw == tag.resolved)
                    return tag.raw;
                else
                    return QString("%1 (<tt>%2</tt>)").arg(tag.resolved.toHtmlEscaped(), tag.raw.toHtmlEscaped());
            };

            if (tags.size() == 1)
                formattedTags = QString("%1<br>").arg(formatTag(tags.front()));
            else if (tags.size() > 1) {
                formattedTags = QString("<ul style=\"margin:0;padding:0;\">%1</ul>").arg(tags
                        | std::views::transform([&](auto const &tag){
                            return QString("<li>%1</li>").arg(formatTag(tag));
                        })
                        | std::views::join
                        | std::ranges::to<QString>());
            }
            description += tr("<b>Tag:</b> %1").arg(formattedTags);

            description += tr("<b>Type:</b> %1 (%2)%3<br>").arg(
                    QString::fromUtf8(QMetaEnum::fromType<NodeType>().valueToKey(std::to_underlying(node.type()))).toHtmlEscaped(),
                    QString::number(std::to_underlying(node.type())).toHtmlEscaped(),
                    node.isLinkingNode() ? tr(" (linking node)") : ""
            );
            description += tr("<b>Icon(s):</b> %1<br>").arg(node.icons()
                    | std::views::transform(&IconIdentifier::name)
                    | std::views::transform(&QString::toHtmlEscaped)
                    | std::views::join_with(QString::fromUtf8(", "))
                    | std::ranges::to<QString>()
            );
            description += tr("<b>UUID:</b> %1<br>").arg(node.uuid().toString(QUuid::WithoutBraces).toHtmlEscaped());

            if (auto link = node.linkTo()) {
                QString target;

                auto targetNode = libraryModel_->fromUuid(*link);

                if (targetNode)
                    target = targetNode->get().name();
                else
                    target = tr("invalid (%1)").arg(targetNode.error());

                description += tr("<b>Link target:</b> %1 (%2)<br>").arg(target, link->toString(QUuid::WithoutBraces));

                if (targetNode)
                    description += tr("<b>Target path:</b> %1<br>").arg(targetNode->get().path());
            }

            QString activeStr;
            if (auto active = node.active(); !active)
                activeStr = tr("<i>not applicable</i>");
            else
                activeStr = (*active ? tr("yes") : tr("no"));
            description += tr("<b>Active:</b> %1<br>").arg(activeStr);

            description += tr("<b>Comment: </b> %1<br>").arg(node.comment());

            ui->labelDescriptionText->setText(description);
            emit tagsSelected(node.tags()
                    | std::views::transform([](auto const &v){ return v.resolved; })
                    | std::ranges::to<QStringList>()
            );
        } else {
            ui->labelDescriptionTitle->clear();
            ui->labelDescriptionText->clear();
            emit tagsSelected({});
        }
    };

    connect(ui->actionToggleEditMode, &QAction::triggered, this, updateSelection);
    connect(ui->actionToggleEditMode, &QAction::triggered, this, [this]{
        ZoneScoped;
        finishSelection(true);
    });
    connect(ui->treeTags->selectionModel(), &QItemSelectionModel::currentRowChanged, this, updateSelection);
    connectionModelDataChanged = connect(&*libraryModel_, &Model::dataChanged, this, updateSelection);
    updateSelection();

    connect(ui->treeTags, &QTreeView::doubleClicked, ui->actionToggleActive, &QAction::trigger);

    return {};
}

void Library::finishSelection(bool const cancel) {
    ZoneScoped;

    if (!ui->treeTags->isRequestSelectionActive())
        return;

    if (cancel)
        ui->treeTags->resetRequestSelection();

    model_->resetRequestSelection();
    ui->labelTreeNotification->clear();
    ui->labelTreeNotification->hide();
    ui->buttonTreeNotificationCancel->hide();

    if (selectionJumpBackTo) {
        QTimer::singleShot(0, [&, index = *selectionJumpBackTo] {
            ui->treeTags->selectionModel()->setCurrentIndex(*selectionJumpBackTo,
                                                        QItemSelectionModel::SelectionFlag::SelectCurrent);
        });
        selectionJumpBackTo.reset();
    }
}

std::expected<std::unique_ptr<Library>, QString>
Library::create(Qt::WindowFlags const flags) {
    ZoneScoped;

    auto self = std::unique_ptr<Library>(new Library{flags});
    if (auto result = self->init(); !result)
        return std::unexpected(result.error());

    return self;
}

Library::~Library() {
    disconnect(connectionModelDataChanged);
}

std::expected<void, QString> Library::saveContent(QIODevice &io) {
    ZoneScoped;

    if (!io.open(QIODevice::WriteOnly))
        return std::unexpected(tr("Cannot open for writing: %1").arg(io.errorString()));

    QCborMap map;
    map[std::to_underlying(Format::TopLevelKey::FormatVersion)] = Format::formatVersion;
    map[std::to_underlying(Format::TopLevelKey::App)] = Format::app.toString();

    if (auto value = libraryModel_->save())
        map[std::to_underlying(Format::TopLevelKey::RootNode)] = *value;
    else
        return std::unexpected(value.error());

    QCborStreamWriter writer(&io);
    map.toCborValue().toCbor(writer);

    return {};
}

std::expected<void, QString> Library::loadContent(QIODevice &io) {
    ZoneScoped;

    if (!io.open(QIODevice::ReadOnly))
        return std::unexpected(tr("Cannot open for reading: %1").arg(io.errorString()));

    QCborStreamReader reader(&io);
    auto content = QCborValue::fromCbor(reader);
    if (!content.isMap())
        return std::unexpected(tr("Content is not a map"));

    auto map = content.toMap();

    auto formatVersion = map.take(std::to_underlying(Format::TopLevelKey::FormatVersion));
    if (formatVersion.isUndefined())
        return std::unexpected(tr("Missing format version"));
    if (!formatVersion.isInteger())
        return std::unexpected(tr("Format version is not an integer, but %1").arg(cborTypeToString(formatVersion.type())));
    if (formatVersion.toInteger() != Format::formatVersion)
        return std::unexpected(tr("Unknown format version: %1").arg(QString::number(formatVersion.toInteger())));

    auto app = map.take(std::to_underlying(Format::TopLevelKey::App));
    if (app.isUndefined())
        return std::unexpected(tr("Application key doesn't exist"));
    if (!app.isString())
        return std::unexpected(tr("Application key is not a string but %1").arg(cborTypeToString(app.type())));
    if (app.toString() != Format::app)
        qCWarning(LoggingCategory) << "Unknown application value:" << app.toString();

    auto root = map.take(std::to_underlying(Format::TopLevelKey::RootNode));
    if (root.isUndefined())
        return std::unexpected(tr("Root node not found"));

    for (auto const &v: map)
        qCWarning(LoggingCategory) << "Unhandled element" << v.first << "=" << v.second;

    if (auto result = libraryModel_->load(root); !result)
        return std::unexpected(result.error());

    return {};
}

QByteArray Library::saveUiState() const {
    ZoneScoped;

    QByteArray result;
    QBuffer buffer(&result);
    buffer.open(QIODevice::WriteOnly);

    QDataStream out(&buffer);
    out.setVersion(QDataStream::Version::Qt_6_7);
    out << ui->treeTags->header()->saveState();
    out << ui->treeTags->saveExpandedState();
    out << ui->splitter->saveState();

    buffer.close();
    return result;
}

void Library::restoreUiState(QByteArray const &value) {
    ZoneScoped;

    QBuffer buffer;
    buffer.setData(value);
    buffer.open(QIODevice::ReadOnly);

    QDataStream in(&buffer);

    QByteArray byteArray;
    in >> byteArray;
    ui->treeTags->header()->restoreState(byteArray);
    byteArray.clear();

    in >> byteArray;
    ui->treeTags->restoreExpandedState(byteArray);
    byteArray.clear();

    in >> byteArray;
    ui->splitter->restoreState(byteArray);
    byteArray.clear();
}

std::expected<void, QString> Library::setTagsActive(QStringList const &tags) {
    ZoneScoped;
    return libraryModel_->setTagsActive(tags);
}

void Library::setRowHeight(int const rowHeight) {
    ZoneScoped;
    libraryModel_->setRowHeight(rowHeight);
}

std::expected<void, QString> Library::setHighlightedTags(QStringList const &tags) {
    ZoneScoped;

    if (auto result = libraryModel_->setHighlightedTags(tags); !result) {
        return std::unexpected(result.error());
    } else {
        if (!result->empty()) {
            for (auto const &index: *result) {
                auto nodeSourceIndex = index;
                while (nodeSourceIndex.isValid()) {
                    nodeSourceIndex = nodeSourceIndex.parent(); // don't expand the provided node itself, only it's ascendants

                    auto nodeIndex = model_->mapFromSource(nodeSourceIndex);
                    ui->treeTags->setExpanded(nodeIndex, true);
                }
            }

            // scroll to the first of the provided indices
            ui->treeTags->scrollTo(model_->mapFromSource(result->front()));
        }
    }

    return {};
}
}
