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
#include "Utility.hpp"

#include "../DirectoryStats.hpp"
#include "../DirectoryStatsManager.hpp"
#include "../FileTagsManager.hpp"
#include "../FileEditor.hpp"

#include "ui_FileBrowser.h"

namespace {
const QStringList NAME_FILTERS = {"*.jpg", "*.png"};
}

namespace FileBrowser {
FileBrowser::FileBrowser(
        FileTagsManager &fileTagsManager,
        DirectoryStatsManager &directoryStatsManager,
        FileEditor &fileEditor,
        IsFileExcluded const &isFileExcluded,
        IsOtherLibraryOrVersion const &isOtherLibraryOrVersion,
        Qt::WindowFlags const flags
):
    QWidget(nullptr, flags),
    ui(std::make_unique<Ui_FileBrowser>()),
    fileTagsManager_(fileTagsManager),
    directoryStatsManager_(directoryStatsManager),
    fileEditor_(fileEditor),
    isFileExcluded_(isFileExcluded),
    projectDirectoryListModel(std::make_unique<ProjectDirectoryListModel>()),
    directoryTreeModel(std::make_unique<DirectoryTreeModel>(
            style(),
            fileTagsManager,
            directoryStatsManager_,
            [this](auto const &file) { return isFileExcludedAbsPath(file); },
            isOtherLibraryOrVersion
    )),
    directoryTreeProxyModel(std::make_unique<DirectoryTreeProxyModel>(
            [this](auto const &file) { return isFileExcludedAbsPath(file); }
    )) {}

std::expected<void, QString> FileBrowser::init() {
    ZoneScoped;

    ui->setupUi(this);

    connect(&directoryStatsManager_, &DirectoryStatsManager::directoryStatsChanged, this, [this](auto const &){
        refreshDirectoryLabel();
    }, Qt::QueuedConnection);

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
        if (index != -1)
            openDirectory(projectDirectoryListModel->directory(index));
        else
            closeDirectory();
    });

    ui->buttonAddDirectory->setDefaultAction(ui->actionAddDirectory);
    connect(ui->actionAddDirectory, &QAction::triggered, this, [this]{
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

    ui->buttonRemoveDirectory->setDefaultAction(ui->actionRemoveDirectory);
    connect(ui->actionRemoveDirectory, &QAction::triggered, this, [this]{
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
        connect(actionCopyTagsFrom, &QAction::triggered, this, [&]{
            ZoneScoped;
            emit requestTagsCopy(fileOver, fileCurrent);
        });

        auto refreshStatisticsAction = menu.addAction(tr("Refresh statistics"));
        refreshStatisticsAction->setEnabled(QFileInfo(fileOver).isDir());
        connect(refreshStatisticsAction, &QAction::triggered, this, [&]{
            ZoneScoped;
            gsl_Expects(QFileInfo(fileOver).isDir());
            directoryStatsManager_.directoryStats(fileOver).reload();
        });

        menu.exec(ui->treeViewDirectories->mapToGlobal(pos));
    });

    ui->buttonRefresh->setDefaultAction(ui->actionRefresh);
    ui->buttonMarkComplete->setDefaultAction(ui->actionMarkComplete);
    ui->buttonExclude->setDefaultAction(ui->actionExclude);
    ui->buttonShowExcluded->setDefaultAction(ui->actionShowExcluded);

    ui->actionShowExcluded->setChecked(false);

    connect(ui->actionRefresh, &QAction::triggered, this, [this]{
        emit refresh();
        emit directoryTreeModel->dataChanged(QModelIndex(), QModelIndex());
    });

    connect(ui->actionMarkComplete, &QAction::triggered, this, [this]{
        fileEditor_.setCompleteFlag(ui->actionMarkComplete->isChecked());
    });

    connect(ui->actionExclude, &QAction::triggered, this, [this]{
        ZoneScoped;
        if (auto result = fileEditor_.setFileExcluded(ui->actionExclude->isChecked()); !result) {
            QMessageBox::critical(this, tr("Could not set exclusion"), result.error());
        } else {
            ui->actionExclude->setChecked(fileEditor_.isFileExcluded());

            auto indexCurrent = ui->treeViewDirectories->currentIndex();
            auto sourceIndexCurrent = directoryTreeProxyModel->mapToSource(indexCurrent);
            auto currentAbsolutePath = directoryTreeModel->filePath(sourceIndexCurrent);
            directoryTreeModel->refreshExcludedState(currentAbsolutePath);
        }
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
        FileTagsManager &fileTagsManager,
        DirectoryStatsManager &directoryStatsManager,
        FileEditor &fileEditor,
        IsFileExcluded const &isFileExcluded,
        IsOtherLibraryOrVersion const &isOtherLibraryOrVersion,
        Qt::WindowFlags flags
) {
    ZoneScoped;

    auto self = std::unique_ptr<FileBrowser>(new FileBrowser{fileTagsManager, directoryStatsManager, fileEditor, isFileExcluded, isOtherLibraryOrVersion, flags});
    if (auto result = self->init(); !result)
        return std::unexpected(result.error());

    return self;
}

FileBrowser::~FileBrowser() = default;

void FileBrowser::setProjectRootPath(QString const &projectRootPath) {
    ZoneScoped;
    gsl_Expects(QFileInfo(projectRootPath).isDir());
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

void FileBrowser::openDirectory(QString const &directory) {
    ZoneScoped;
    gsl_Expects(!directory.isEmpty());

    if (directory != currentDirectory_) {
        currentDirectory_ = directory;
        currentDirectoryStats_ = &directoryStatsManager_.directoryStats(QFileInfo(QDir(projectRootPath_), currentDirectory_).filePath());

        ui->buttonRemoveDirectory->setEnabled(true);
        ui->actionShowExcluded->setEnabled(true);

        auto absoluteDirectory = QFileInfo(QDir(projectRootPath_), directory).filePath();
        auto sourceIndex = directoryTreeModel->setRootPath(absoluteDirectory);

        directoryTreeProxyModel->setSourceModel(&*directoryTreeModel);
        directoryTreeProxyModel->setFilterRole(std::to_underlying(DirectoryTreeModel::Roles::FileNameRole));
        directoryTreeProxyModel->setSortRole(std::to_underlying(DirectoryTreeModel::Roles::FileNameRole));

        auto index = directoryTreeProxyModel->mapFromSource(sourceIndex);

        ui->treeViewDirectories->setModel(&*directoryTreeProxyModel);
        ui->treeViewDirectories->setRootIndex(index);

        currentChangedConnection = connect(ui->treeViewDirectories->selectionModel(), &QItemSelectionModel::currentChanged, this, [this](QModelIndex const &current){
            ZoneScoped;

            auto currentSource = directoryTreeProxyModel->mapToSource(current);
            auto filePath = currentSource.isValid() ? directoryTreeModel->filePath(currentSource) : QString();
            fileSelectedHandle(filePath);
        });

        refreshDirectoryLabel();
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

    currentDirectory_ = {};
    currentDirectoryStats_ = nullptr;
}

void FileBrowser::refreshDirectoryLabel() {
    gsl_Expects(!currentDirectory_.isNull() == static_cast<bool>(currentDirectoryStats_));

    if (!currentDirectory_.isNull())
        ui->labelDirectory->setText(formatDirectoryStats(*currentDirectoryStats_, QDir(projectRootPath_).relativeFilePath(currentDirectory_)));
    else
        ui->labelDirectory->clear();
}

void FileBrowser::fileSelectedHandle(QString const &path) {
    ZoneScoped;

    bool valid = !path.isNull();
    bool isDir = valid ? QFileInfo(path).isDir() : false;

    if (valid)
        emit fileSelected(path);

    ui->actionMarkComplete->setEnabled(valid && !isDir);
    ui->actionMarkComplete->setChecked(false);
    ui->actionExclude->setEnabled(valid);

    if (valid) {
        if (!isDir)
            ui->actionMarkComplete->setChecked(fileEditor_.isCompleteFlag());

        ui->actionExclude->setChecked(fileEditor_.isFileExcluded());
    }
}

bool FileBrowser::isFileExcludedAbsPath(QString const &file) {
    ZoneScoped;
    gsl_Expects(QFileInfo(file).isAbsolute());
    gsl_Expects(!projectRootPath_.isEmpty());
    gsl_Expects(QFileInfo(projectRootPath_).isAbsolute());
    gsl_Expects(QFileInfo(currentDirectory_).isRelative());

    if (QFileInfo(file).isRoot())
        return false;

    auto currentDirectoryAbs = QDir(projectRootPath_).filePath(currentDirectory_);
    gsl_Expects(QFileInfo(currentDirectoryAbs).isAbsolute());

    auto parent = currentDirectoryAbs;
    while(!QFileInfo(parent).isRoot()) {
        if (file == parent)
            return false;

        parent = QFileInfo(parent).path();
    }

    auto relativePath = QDir(projectRootPath_).relativeFilePath(file);
    return isFileExcluded_(relativePath);
}
}
