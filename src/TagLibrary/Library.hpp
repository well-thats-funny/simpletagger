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

class Ui_Library;

namespace TagLibrary {
class Model;
class FilterProxyModel;
class SelectionHelperProxyModel;
class NodeRoot;

class Library : public QWidget {
    Q_OBJECT

    Library(Library const &other) = delete;
    Library(Library &&other) = delete;
    Library &operator=(Library const &other) = delete;
    Library &operator=(Library &&other) = delete;

    Library(QString const &libraryPath, Qt::WindowFlags flags = Qt::WindowFlags());
    std::expected<void, QString> init();

public:
    void finishSelection(bool cancel);

    static std::expected<std::unique_ptr<Library>, QString> create(
            QString const &libraryPath, Qt::WindowFlags flags = Qt::WindowFlags()
    );
    ~Library() override;

    [[nodiscard]] QUuid getUuid() const;
    [[nodiscard]] int getVersion() const;
    [[nodiscard]] QUuid getVersionUuid() const;

    [[nodiscard]] std::expected<void, QString> saveContent(QIODevice &io);
    [[nodiscard]] std::expected<void, QString> loadContent(QIODevice &io);

    [[nodiscard]] QByteArray saveUiState() const;
    void restoreUiState(QByteArray const &value);

    // this overwrites all tags! tags not on the list will be set to inactive
    [[nodiscard]] std::expected<void, QString> setTagsActive(QStringList const &tags);

    void setRowHeight(int rowHeight);
    [[nodiscard]] std::expected<void, QString> setHighlightedTags(QStringList const &tags);

    void setHighlightChangedAfterVersion(std::optional<int> const &version);

    QStringList allTags() const;

signals:
    void contentChanged();
    void editModeChanged(bool editMode);
    void tagsSelected(QStringList const &tags);
    void tagsActiveChanged(QStringList const &tags, bool active);

private:
    QModelIndex toLibraryModelIndex(QModelIndex const &viewModelIndex);
    QModelIndex toViewModelIndex(QModelIndex const &libraryModelIndex);
    void updateLibraryVersion(int currentLibraryVersion);

    std::unique_ptr<Ui_Library> ui;
    std::unique_ptr<Model> libraryModel_;
    std::unique_ptr<FilterProxyModel> filterModel_;

    std::unique_ptr<SelectionHelperProxyModel> model_;
    QString const libraryPath_; // informational only, as MainWindow is atm responsible for reading/writing the lib file
    QUuid libraryUuid_ = QUuid::createUuid();
    int currentLibraryVersion_ = 1;
    int nextLibraryVersion_ = 1;
    QUuid currentLibraryVersionUuid_;

    QString transferLabel_;
    QIcon transferIcon_;
    QMenu userIconMenu;

    std::optional<QModelIndex> selectionJumpBackTo;

    QMetaObject::Connection connectionModelDataChanged;
};
}