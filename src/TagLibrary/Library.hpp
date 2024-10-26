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
class SelectionHelperProxyModel;
class NodeRoot;

class Library : public QDockWidget {
    Q_OBJECT

    Library(Library const &other) = delete;
    Library(Library &&other) = delete;
    Library &operator=(Library const &other) = delete;
    Library &operator=(Library &&other) = delete;

    Library(Qt::WindowFlags flags = Qt::WindowFlags());
    std::expected<void, QString> init();

public:
    void finishSelection(bool cancel);

    static std::expected<std::unique_ptr<Library>, QString> create(Qt::WindowFlags flags = Qt::WindowFlags());
    ~Library() override;

    std::expected<void, QString> saveContent(QIODevice &io);
    std::expected<void, QString> loadContent(QIODevice &io);

    [[nodiscard]] QByteArray saveUiState() const;
    void restoreUiState(QByteArray const &value);

    // this overwrites all tags! tags not on the list will be set to inactive
    [[nodiscard]] std::expected<void, QString> setTagsActive(QStringList const &tags);

    void setRowHeight(int rowHeight);
    [[nodiscard]] std::expected<void, QString> setHighlightedTags(QStringList const &tags);

signals:
    void contentChanged();
    void tagsSelected(QStringList const &tags);
    void tagsActiveChanged(QStringList const &tags, bool active);

private:
    std::unique_ptr<Ui_Library> ui;
    std::unique_ptr<Model> libraryModel_;
    std::unique_ptr<SelectionHelperProxyModel> model_;
    QString transferLabel_;
    QIcon transferIcon_;
    QMenu userIconMenu;

    std::optional<QModelIndex> selectionJumpBackTo;

    QMetaObject::Connection connectionModelDataChanged;
};
}