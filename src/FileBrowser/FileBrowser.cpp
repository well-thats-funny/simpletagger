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
#include "FileBrowser.hpp"

#include "ProjectDirectoryListModel.hpp"
#include "DirectoryTreeModel.hpp"
#include "DirectoryTreeProxyModel.hpp"

#include "ui_FileBrowser.h"

namespace {
const QStringList NAME_FILTERS = {"*.jpg", "*.png"};
}

namespace FileBrowser {
FileBrowser::FileBrowser(
        FileTagsProvider const &fileTagsProvider,
        DirectoryStatsProvider const &directoryStatsProvider,
        IsFileExcluded const &isFileExcluded,
        Qt::WindowFlags const flags
):
    QDockWidget(nullptr, flags),
    ui(std::make_unique<Ui_FileBrowser>()),
    isFileExcluded_(isFileExcluded),
    projectDirectoryListModel(std::make_unique<ProjectDirectoryListModel>()),
    directoryTreeModel(std::make_unique<DirectoryTreeModel>(
            style(),
            fileTagsProvider,
            directoryStatsProvider,
            [this](auto const &file) { return isFileExcludedAbsPath(file); }
    )),
    directoryTreeProxyModel(std::make_unique<DirectoryTreeProxyModel>(
            [this](auto const &file) { return isFileExcludedAbsPath(file); }
    )) {}

std::expected<void, QString> FileBrowser::init() {
    ZoneScoped;

    ui->setupUi(this);

    ui->comboBoxDirectories->setModel(&*projectDirectoryListModel);

    connect(&*projectDirectoryListModel, &ProjectDirectoryListModel::rowsInserted, this, [this](
            QModelIndex const &parent, int const first, int const last){
        ZoneScoped;
        gsl_Expects(!parent.isValid());

        for (int row = first; row <= last; ++row)
            emit directoryAdded(projectDirectoryListModel->directory(row));

        if (currentDirectory_.isNull())
            openDirectory(projectDirectoryListModel->directory(first));
    });

    connect(&*projectDirectoryListModel, &ProjectDirectoryListModel::rowsAboutToBeRemoved, this, [this](
            QModelIndex const &parent, int const first, int const last){
        ZoneScoped;
        gsl_Expects(!parent.isValid());

        bool currentRemoved = false;

        for (int row = first; row <= last; ++row) {
            auto directory = projectDirectoryListModel->directory(row);
            emit directoryRemoved(directory);

            if (directory == currentDirectory_)
                currentRemoved = true;
        }

        if (currentRemoved) {
            auto &directories = projectDirectoryListModel->directories();
            if (directories.empty()) {
                closeDirectory();
            } else {
                auto row = std::min(first, gsl::narrow<int>(directories.size() - 1));
                openDirectory(directories.at(row));
            }
        }
    });

    connect(&*ui->comboBoxDirectories, &QComboBox::currentIndexChanged, this, [this](int const index){
        ZoneScoped;
        openDirectory(projectDirectoryListModel->directory(index));
    });

    connect(ui->buttonAddDirectory, &QToolButton::clicked, this, [this]{
        ZoneScoped;
        assert(!projectRootPath_.isEmpty());

        auto startDir = projectDirectoryListModel->directories().empty()
                        ? projectRootPath_
                        : QFileInfo(QDir(projectRootPath_), QFileInfo{projectDirectoryListModel->directories().last()}.dir().path()).filePath();
        if (auto path = QFileDialog::getExistingDirectory(
                    this,
                    tr("Select a directory to add to the project"),
                    startDir,
                    QFileDialog::ShowDirsOnly
            ); !path.isEmpty()) {
            path = QDir{projectRootPath_}.relativeFilePath(path);
            projectDirectoryListModel->addDirectory(path);
        }
    });

    connect(ui->buttonRemoveDirectory, &QToolButton::clicked, this, [this]{
        ZoneScoped;
        int row = ui->comboBoxDirectories->currentIndex();
        auto directory = projectDirectoryListModel->directory(row);
        if (QMessageBox::question(
                this, tr("Remove from the project"),
                QString(tr("Do you want to remove directory <b>%1</b> from this project?<br>"
                           "<br>"
                           "This will only remove the directory from the list; data on disk won't be affected"
                )).arg(directory)) == QMessageBox::StandardButton::Yes) {
            projectDirectoryListModel->removeDirectory(row);
        }
    });

    directoryTreeModel->setNameFilters(NAME_FILTERS);
    directoryTreeModel->setNameFilterDisables(false);

    connect(ui->treeViewDirectories, &CustomTreeView::customContextMenuRequested, this, [this](auto const &pos){
        ZoneScoped;

        auto indexCurrent = ui->treeViewDirectories->currentIndex();
        auto sourceIndexCurrent = directoryTreeProxyModel->mapToSource(indexCurrent);
        auto fileCurrent = directoryTreeModel->filePath(sourceIndexCurrent);

        auto indexOver = ui->treeViewDirectories->mouseOverIndex();
        auto sourceIndexOver = directoryTreeProxyModel->mapToSource(indexOver);
        auto fileOver = directoryTreeModel->filePath(sourceIndexOver);

        QMenu menu;

        auto actionCopyTagsFrom = menu.addAction(tr("Copy tags from"));
        actionCopyTagsFrom->setEnabled(indexCurrent != indexOver);
        connect(actionCopyTagsFrom, &QAction::triggered, this, [&](){
            emit requestTagsCopy(fileOver, fileCurrent);
        });
        menu.exec(ui->treeViewDirectories->mapToGlobal(pos));
    });

    ui->buttonRefresh->setDefaultAction(ui->actionRefresh);
    ui->buttonExclude->setDefaultAction(ui->actionExclude);
    ui->buttonShowExcluded->setDefaultAction(ui->actionShowExcluded);

    ui->actionShowExcluded->setChecked(false);

    connect(ui->actionRefresh, &QAction::triggered, this, [this]{
        emit directoryTreeModel->dataChanged(QModelIndex(), QModelIndex());
    });

    connect(ui->actionExclude, &QAction::triggered, this, [this]{
        ZoneScoped;

        auto indexCurrent = ui->treeViewDirectories->currentIndex();
        auto sourceIndexCurrent = directoryTreeProxyModel->mapToSource(indexCurrent);
        auto fileCurrent = QDir(projectRootPath_).relativeFilePath(directoryTreeModel->filePath(sourceIndexCurrent));
        emit requestExcludeFileToggle(fileCurrent);
        ui->actionExclude->setChecked(isFileExcluded_(fileCurrent));
    });

    connect(ui->actionShowExcluded, &QAction::triggered, this, [this]{
        ZoneScoped;
        directoryTreeProxyModel->setShowExcluded(ui->actionShowExcluded->isChecked());
    });

    fileSelectedHandle({});

    return {};
}

std::expected<std::unique_ptr<FileBrowser>, QString>
FileBrowser::create(
        FileTagsProvider const &fileTagsProvider,
        DirectoryStatsProvider const &directoryStatsProvider,
        IsFileExcluded const &isFileExcluded,
        Qt::WindowFlags flags
) {
    ZoneScoped;

    auto self = std::unique_ptr<FileBrowser>(new FileBrowser{
        fileTagsProvider, directoryStatsProvider, isFileExcluded, flags
    });
    if (auto result = self->init(); !result)
        return std::unexpected(result.error());

    return self;
}

FileBrowser::~FileBrowser() = default;

void FileBrowser::setProjectRootPath(QString const &projectRootPath) {
    ZoneScoped;
    projectRootPath_ = projectRootPath;
}

void FileBrowser::setDirectories(QStringList const &directories) {
    ZoneScoped;
    projectDirectoryListModel->setDirectories(directories);

    if (!directories.empty()) {
        ui->comboBoxDirectories->setCurrentIndex(0);
    }
}

std::expected<void, QString> FileBrowser::selectDirectoryInProjectList(QString const &directory) {
    ZoneScoped;
    for (int i = 0; i != projectDirectoryListModel->directories().size(); ++i) {
        if (projectDirectoryListModel->directory(i) == directory) {
            ui->comboBoxDirectories->setCurrentIndex(i);
            return {};
        }
    }
    return std::unexpected("Directory not found in the project list");
}

std::expected<void, QString> FileBrowser::selectFileInTree(QString const &file) {
    ZoneScoped;
    auto sourceIndex = directoryTreeModel->index(file);
    if (!sourceIndex.isValid())
        return std::unexpected("File not found in the tree");

    if (directoryTreeProxyModel->sourceModel()) {
        auto index = directoryTreeProxyModel->mapFromSource(sourceIndex);
        ui->treeViewDirectories->selectionModel()->setCurrentIndex(
                index, QItemSelectionModel::SelectionFlag::SelectCurrent
        );
    }

    return {};
}

QByteArray FileBrowser::saveUiState() const {
    ZoneScoped;

    QByteArray result;
    QBuffer buffer(&result);
    buffer.open(QIODevice::WriteOnly);

    QDataStream out(&buffer);
    out.setVersion(QDataStream::Version::Qt_6_7);
    out << ui->treeViewDirectories->header()->saveState();
    out << ui->treeViewDirectories->saveExpandedState();

    buffer.close();
    return result;
}

void FileBrowser::restoreUiState(QByteArray const &value) {
    ZoneScoped;

    QBuffer buffer;
    buffer.setData(value);
    buffer.open(QIODevice::ReadOnly);

    QDataStream in(&buffer);

    QByteArray byteArray;
    in >> byteArray;
    ui->treeViewDirectories->header()->restoreState(byteArray);
    byteArray.clear();

    in >> byteArray;
    ui->treeViewDirectories->restoreExpandedState(byteArray);
    byteArray.clear();
}

void FileBrowser::refreshExcludedState(QString const &file) {
    ZoneScoped;
    gsl_Expects(QFileInfo(file).isRelative());
    directoryTreeModel->refreshExcludedState(QFileInfo(QDir(projectRootPath_), file).filePath());
    ui->actionExclude->setChecked(isFileExcluded_(file));
}

void FileBrowser::openDirectory(QString const &directory) {
    ZoneScoped;

    if (directory != currentDirectory_) {
        currentDirectory_ = directory;

        ui->buttonRemoveDirectory->setEnabled(true);
        ui->labelDirectory->setText(directory);
        ui->actionShowExcluded->setEnabled(true);

        auto absoluteDirectory = QFileInfo(QDir(projectRootPath_), directory).filePath();
        auto sourceIndex = directoryTreeModel->setRootPath(absoluteDirectory);

        directoryTreeProxyModel->setSourceModel(&*directoryTreeModel);
        auto index = directoryTreeProxyModel->mapFromSource(sourceIndex);

        ui->treeViewDirectories->setModel(&*directoryTreeProxyModel);
        ui->treeViewDirectories->setRootIndex(index);

        currentChangedConnection = connect(ui->treeViewDirectories->selectionModel(), &QItemSelectionModel::currentChanged, this, [this](QModelIndex const &current){
            ZoneScoped;

            auto currentSource = directoryTreeProxyModel->mapToSource(current);
            auto filePath = currentSource.isValid() ? directoryTreeModel->filePath(currentSource) : QString();
            fileSelectedHandle(filePath);
        });
    }
}

void FileBrowser::closeDirectory() {
    ZoneScoped;

    disconnect(currentChangedConnection);

    ui->treeViewDirectories->setModel(nullptr);

    directoryTreeProxyModel->setSourceModel(nullptr);

    ui->actionShowExcluded->setEnabled(true);
    ui->labelDirectory->clear();
    ui->buttonRemoveDirectory->setEnabled(false);
}

void FileBrowser::fileSelectedHandle(QString const &path) {
    ZoneScoped;

    bool valid = !path.isNull();

    if (valid)
        emit fileSelected(path);

    ui->actionExclude->setEnabled(valid);

    if (valid) {
        auto relativeFilePath = QDir(projectRootPath_).relativeFilePath(path);
        ui->actionExclude->setChecked(isFileExcluded_(relativeFilePath));
    }
}

bool FileBrowser::isFileExcludedAbsPath(QString const &file) {
    ZoneScoped;
    gsl_Expects(QFileInfo(file).isAbsolute());
    gsl_Expects(!projectRootPath_.isEmpty());
    return isFileExcluded_(QDir(projectRootPath_).relativeFilePath(file));
}
}
