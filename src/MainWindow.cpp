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
#include "MainWindow.hpp"

#include "About.hpp"
#include "FileEditor.hpp"
#include "NewProjectDialog.hpp"
#include "Constants.hpp"
#include "StartupDialog.hpp"
#include "SettingsDialog.hpp"
#include "ImageViewer/ImageViewer.hpp"
#include "FileBrowser/FileBrowser.hpp"
#include "Tags/Tags.hpp"
#include "TagLibrary/Library.hpp"
#include "Utility.hpp"
#include "VerticalLine.hpp"

#include "ui_MainWindow.h"

using namespace std::literals;

namespace {
    constexpr qsizetype MAX_RECENT_PROJECTS = 10;
}

MainWindow::MainWindow(Settings &settings, QTranslator &translator):
    ui{std::make_unique<Ui_MainWindow>()},
    settings{settings},
    translator{translator},
    fileTagsManager(settings.system.backupOnAnyChange),
    directoryStatsManager(fileTagsManager) {}

std::expected<void, QString> MainWindow::init() {
    ZoneScoped;

    ui->setupUi(this);

    connect(&fileTagsManager, &FileTagsManager::tagsSaved, [this](auto const &count){
        showSavedStatusMessage(tr("image tags"), count);
    });

    fileEditor_.emplace(fileTagsManager);

    connect(&*fileEditor_, &FileEditor::projectSaved, this, [this](auto const &backupCount){
        showSavedStatusMessage(tr("project"), backupCount);
    });

    if (auto result = setupGeneralActions(); !result)
        return std::unexpected(result.error());

    if (auto result = setupStatusBar(); !result)
        return std::unexpected(result.error());

    if (auto result = setupDocks(); !result)
        return std::unexpected(result.error());

    readSettings();

    // let window recalculate the layout first before loading the project
    // TODO: still needed?
    QTimer::singleShot(0, [this] {
        ZoneScoped;
        loadStartupProject();
    });

    return {};
}

std::expected<void, QString> MainWindow::setupGeneralActions() {
    ZoneScoped;

    connect(ui->actionSettings, &QAction::triggered, this, [this]{
        ZoneScoped;

        SettingsDialog dialog(this->settings, this);
        dialog.exec();

        if (dialog.result() == QDialog::Accepted) {
            this->settings = dialog.settings();
            writeSettings();
            readSettings();
        }
    });

    connect(ui->actionQuit, &QAction::triggered, this, [this]{ close(); });

    connect(ui->actionCreateProject, &QAction::triggered, this, [this]{
        ZoneScoped;

        NewProjectDialog newProjectDialog(this);
        newProjectDialog.exec();

        if (auto path = newProjectDialog.result(); !path.isEmpty()) {
            if (auto result = loadProject(path); !result) {
                QMessageBox::critical(
                        this,
                        tr("Project load failed"),
                        tr("Could not load newly created project: %1").arg(result.error())
                );
            }
        }
    });

    connect(ui->actionOpenProject, &QAction::triggered, this, [this]{
        ZoneScoped;

        QSettings settings;
        if (auto path = QFileDialog::getOpenFileName(
                    this,
                    tr("Open project"),
                    settings.value(SettingsKey::LAST_PROJECT_LOCATION, QString()).toString(),
                    Constants::PROJECT_FILE_FILTER.toString()
            ); !path.isEmpty()) {
            if (auto result = loadProject(path); !result)
                QMessageBox::critical(
                        this,
                        tr("Project load failed"),
                        tr("Could not open project: %1").arg(result.error())
                );
        }
    });

    connect(ui->actionProjectSetup, &QAction::triggered, this, []{
        ZoneScoped;
        // TODO
        qWarning() << "PROJECT SETTINGS NOT YET IMPLEMENTED";
    });

    connect(ui->actionCloseProject, &QAction::triggered, this, [this]{
        ZoneScoped;
        if (auto result = loadProject(); !result)
            QMessageBox::critical(
                    this,
                    tr("Project close failed"),
                    tr("Could not close project: %1").arg(result.error())
            );
    });

    connect(ui->actionAbout, &QAction::triggered, this, [this]{
        ZoneScoped;
        auto about = About::create(this);
        if (!about)
            QMessageBox::critical(this, tr("About failed"), tr("Could not open about box: %1").arg(about.error()));
        else
            (*about)->exec();
    });

    return {};
}

std::expected<void, QString> MainWindow::setupStatusBar() {
    ZoneScoped;

    statusBar()->addPermanentWidget(statusCache = new QLabel);
    statusBar()->addPermanentWidget(new VerticalLine);
    statusBar()->addPermanentWidget(statusBarMemory = new QLabel);

    connect(&statsTimer, &QTimer::timeout, this, [this]{
        ZoneScoped;

        if (auto size = fetchMemoryUsage())
            statusBarMemory->setText(tr("Memory usage (resident): %1").arg(locale().formattedDataSize(size->resident)));
        else
            statusBarMemory->setText(tr("Could not get memory usage"));

        statusCache->setText(tr("Cached %1 directories, %2 files")
                .arg(directoryStatsManager.cachedDirectories())
                .arg(fileTagsManager.cachedFiles())
        );
    });

    statsTimer.setInterval(1000);
    statsTimer.start();

    return {};
}

std::expected<void, QString> MainWindow::setupDocks() {
    ZoneScoped;

    dockManager = std::make_unique<ads::CDockManager>();
    centralWidget()->layout()->addWidget(&*dockManager);

    if (auto result = setupImageViewerDock(); !result)
        return std::unexpected(result.error());

    if (auto result = setupFileBrowserDock(); !result)
        return std::unexpected(result.error());

    if (auto result = setupTagsDock(); !result)
        return std::unexpected(result.error());

    if (auto result = setupTagLibraryDock(); !result)
        return std::unexpected(result.error());

    return {};
}

std::unique_ptr<ads::CDockWidget> MainWindow::addDock(std::unique_ptr<QWidget> &&widget, QString const &label, ads::DockWidgetArea const area) {
    ZoneScoped;

    auto dock = std::make_unique<ads::CDockWidget>(label);
    dock->setWidget(widget.release(), ads::CDockWidget::eInsertMode::ForceNoScrollArea);

    dockManager->addDockWidget(area, &*dock);
    ui->menuView->addAction(dock->toggleViewAction());

    return dock;
}

std::expected<void, QString> MainWindow::setupImageViewerDock() {
    ZoneScoped;

    if (auto result = ImageViewer::ImageViewer::create(*fileEditor_); !result) {
        return std::unexpected(result.error());
    } else {
        imageViewer = &**result;
        imageViewerDock = addDock(std::move(*result), tr("Image viewer"), ads::DockWidgetArea::CenterDockWidgetArea);
    }

    return {};
}

std::expected<void, QString> MainWindow::setupFileBrowserDock() {
    ZoneScoped;

    if (auto result = FileBrowser::FileBrowser::create(
                fileTagsManager,
                directoryStatsManager,
                *fileEditor_,
                [this](QString const &fileName)->bool{
                    ZoneScoped;
                    gsl_Expects(project);
                    gsl_Expects(QFileInfo(fileName).isRelative());
                    return project->isExcludedFile(fileName);
                },
                [this](FileTags const &fileTags)->bool{
                    ZoneScoped;
                    gsl_Expects(tagLibrary);
                    // TODO: this comparison code is repeated in multiple places. A good candidate
                    //       for a utility function?
                    if (auto tagLibraryUuid = fileTags.tagLibraryUuid();
                            !tagLibraryUuid || (*tagLibraryUuid != tagLibrary->getUuid())) {
                        return true;
                    } else if (auto tagLibraryVersionUuid = fileTags.tagLibraryVersionUuid();
                            !tagLibraryVersionUuid || (*tagLibraryVersionUuid != tagLibrary->getVersionUuid())) {
                        return true;
                    } else {
                        return false;
                    }
                }
        ); !result) {
        return std::unexpected(result.error());
    } else {
        fileBrowser = &**result;
        fileBrowserDock = addDock(std::move(*result), tr("File browser"), ads::DockWidgetArea::LeftDockWidgetArea);
    }

    connect(&*fileBrowser, &FileBrowser::FileBrowser::directoryAdded, this, [this](auto const &directory){
        ZoneScoped;
        assert(project);
        if (auto result = project->addDirectory(directory); !result)
            QMessageBox::critical(this, tr("Adding directory failed"), result.error());
        else
            saveProject();
    });

    connect(&*fileBrowser, &FileBrowser::FileBrowser::directoryRemoved, this, [this](auto const &directory){
        ZoneScoped;
        assert(project);
        if (auto result = project->removeDirectory(directory); !result)
            QMessageBox::critical(this, tr("Removing directory failed"), result.error());
        else
            saveProject();
    });

    connect(&*fileBrowser, &FileBrowser::FileBrowser::directoryLoaded, this, [this](auto const &directory){
        ZoneScoped;
        QSettings settings;
        settings.setValue(SettingsKey::LAST_VIEWED_DIRECTORY, directory);
    });

    connect(&*fileBrowser, &FileBrowser::FileBrowser::fileSelected, this, [this](auto const &file){
        ZoneScoped;
        gsl_Expects(file.isNull() || QFileInfo(file).isAbsolute());
        load(file);
    });

    connect(&*fileBrowser, &FileBrowser::FileBrowser::requestTagsCopy, this, [this](auto const &sourceFile, auto const &targetFile) {
        ZoneScoped;
        gsl_Expects(project);
        gsl_Expects(QFileInfo(sourceFile).isAbsolute());
        gsl_Expects(QFileInfo(targetFile).isAbsolute());
        auto projectRootPath = QFileInfo(project->path()).path();
        if (QMessageBox::question(
                this,
                tr("Import tags"),
                tr(
                        "Copy tags from:<br>"
                        "<br>"
                        "<tt>%1</tt><br>"
                        "<br>"
                        "to:<br>"
                        "<br>"
                        "<tt>%2</tt><br>"
                        "<br>"
                        R"(<b>This will <span style="color:red;">overwrite</span> all tags of <tt style="color:blue;">%3</tt>!</b><br>)"
                        "<br>"
                        "Are you sure?<br>"
                )
                        .arg(QDir(projectRootPath).relativeFilePath(sourceFile))
                        .arg(QDir(projectRootPath).relativeFilePath(targetFile))
                        .arg(QFileInfo(targetFile).fileName())
        ) == QMessageBox::StandardButton::Yes) {
            auto doCopy = [&]()->std::expected<void, QString> {
                auto source = fileTagsManager.forFile(sourceFile);
                if (!source)
                    return std::unexpected(source.error());

                auto target = fileTagsManager.forFile(targetFile);
                if (!target)
                    return std::unexpected(target.error());

                if (target->get().overwriteAssignedTags(source->get().assignedTags()))
                    if (auto result = target->get().save(); !result)
                        return std::unexpected(result.error());

                return {};
            };

            if (auto result = doCopy(); !result)
                QMessageBox::critical(
                        this,
                        tr("Copy tags failed"),
                        tr("Could not copy tags: %1").arg(result.error())
                );
            else
                load(targetFile, true);
        }
    });

    connect(&*fileBrowser, &FileBrowser::FileBrowser::refresh, this, [this]{
        directoryStatsManager.invalidateDirectoryStatsCache();
    });

    return {};
}

std::expected<void, QString> MainWindow::setupTagsDock() {
    ZoneScoped;

    if (auto result = Tags::Tags::create(*fileEditor_); !result) {
        return std::unexpected(result.error());
    } else {
        tags_ = &**result;
        tagsDock = addDock(std::move(*result), tr("Assigned tags"), ads::DockWidgetArea::BottomDockWidgetArea);

        connect(&*tags_, &Tags::Tags::tagsSelected, this, [this](QStringList const &tags){
            if (auto result = tagLibrary->setHighlightedTags(tags); !result)
                qWarning() << "Could not highlight tags in tag library:" << result.error();
        });
    }

    return {};
}

std::expected<void, QString> MainWindow::setupTagLibraryDock() {
    ZoneScoped;

    if (auto result = TagLibrary::Library::create(); !result) {
        return std::unexpected(result.error());
    } else {
        tagLibrary = &**result;
        tagLibraryDock = addDock(std::move(*result), tr("Tag library"), ads::DockWidgetArea::RightDockWidgetArea);
        fileTagsManager.setTagLibrary(tagLibrary);
        directoryStatsManager.setTagLibrary(tagLibrary);
    }

    QDir appData{QStandardPaths::writableLocation(QStandardPaths::StandardLocation::AppDataLocation)};
    if (!appData.exists())
        // this is the ugliest way of saying "create the directory" I've ever written...
        QFileInfo(appData.path()).dir().mkpath(appData.dirName());

    auto tagLibraryPath = appData.filePath("TagLibrary.cbor");

    {
        QFile file(tagLibraryPath);
        if (file.exists()) {
            qDebug() << "Loading tag library from" << file.fileName();
            if (auto const result = tagLibrary->loadContent(file); !result) {
                qWarning() << "Couldn't load tag library:" << result.error();
                QMessageBox::critical(
                        this,
                        tr("Could not load tags library"),
                        tr("Loading from file \"%1\" failed: %2").arg(tagLibraryPath, result.error())
                );
            }
        }
    }

    connectionSaveTagLibraryOnChange = connect(&*tagLibrary, &TagLibrary::Library::contentChanged, this, [this, tagLibraryPath]{
        ZoneScoped;

        std::optional<int> backupsCounter;

        if (this->settings.system.backupOnAnyChange)
            backupsCounter = backupFile(tagLibraryPath);

        QSaveFile file(tagLibraryPath);
        qDebug() << "Saving tag library to" << file.fileName();
        if (auto const result = [&]->std::expected<void, QString>{
                if (auto const result = tagLibrary->saveContent(file); !result)
                    return result;

                if (!file.commit())
                    return std::unexpected(QString("Failed to commit file: %1").arg(file.errorString()));

                return {};
            }(); !result) {
            qWarning() << "Couldn't save tag library:" << result.error();
            QMessageBox::critical(
                    this,
                    tr("Could not save tags library"),
                    tr("Saving to file \"%1\" failed: %2").arg(file.fileName(), result.error()));
        } else {
            qInfo() << "Tag library saved, backups count:" << (backupsCounter ? *backupsCounter : -1);
            showSavedStatusMessage(tr("tags library"), backupsCounter);
        }
    });

    connect(&*tagLibrary, &TagLibrary::Library::tagsSelected, this, [this](QStringList const &tags){
        ZoneScoped;
        tags_->setHighlightedTags(tags);
    });

    connect(&*tagLibrary, &TagLibrary::Library::tagsActiveChanged, this, [this](QStringList const &tags, bool const active){
        ZoneScoped;
        fileEditor_->setTagged(tags, active);
    });

    connect(&*fileEditor_, &FileEditor::tagsChanged, this, &MainWindow::loadFileTaggerTagsToTagLibrary);

    return {};
}

void MainWindow::loadStartupProject() {
    ZoneScoped;

    if (!recentProjects.isEmpty()) {
        if (auto result = loadProject(recentProjects.front()); !result)
            QMessageBox::critical(
                    this,
                    tr("Project load failed"),
                    tr("Could not load recently opened project: %1").arg(result.error())
            );

    } else {
        StartupDialog startupDialog(this);
        startupDialog.exec();

        if (*startupDialog.outcome == StartupDialog::Outcome::Exit) {
            QMetaObject::invokeMethod(this, &MainWindow::close, Qt::QueuedConnection);
        } else if (*startupDialog.outcome == StartupDialog::Outcome::OpenProject) {
            if (auto result = loadProject(startupDialog.project); !result)
                QMessageBox::critical(
                        this,
                        tr("Project load failed"),
                        tr("Could not load project: %1").arg(result.error())
                );
        } else {
            assert(false);
        }
    }
}

std::expected<std::unique_ptr<MainWindow>, QString> MainWindow::create(Settings &settings, QTranslator &translator) {
    ZoneScoped;

    auto self = std::unique_ptr<MainWindow>(new MainWindow(settings, translator));
    if (auto result = self->init(); !result)
        return std::unexpected(result.error());

    return self;
}

MainWindow::~MainWindow() {
    ZoneScoped;

    writeSettings();

    // to prevent update signals related to destruction from triggering save
    disconnect(connectionSaveTagLibraryOnChange);
}

void MainWindow::changeEvent(QEvent *event) {
    ZoneScoped;

    assert(event);
    switch(event->type()) {
        case QEvent::LanguageChange:
            ui->retranslateUi(this);
            break;
        default:
            break;
    }

    QMainWindow::changeEvent(event);
}

void MainWindow::paintEvent(QPaintEvent *event) {
    FrameMarkNamed("MainWindow");
    ZoneScoped;

    QMainWindow::paintEvent(event);
}

void MainWindow::writeSettings() {
    ZoneScoped;

    qDebug() << __PRETTY_FUNCTION__;

    QSettings settings;
    settings.setValue(SettingsKey::MAINWINDOW_GEOMETRY, saveGeometry());
    settings.setValue(SettingsKey::MAINWINDOW_STATE, saveState());
    settings.setValue(SettingsKey::MAINWINDOW_DOCKMANAGER_STATE, dockManager->saveState());
    settings.setValue(SettingsKey::MAINWINDOW_IMAGEVIEWER_STATE, imageViewer->saveUiState());
    settings.setValue(SettingsKey::MAINWINDOW_FILEBROWSER_STATE, fileBrowser->saveUiState());
    settings.setValue(SettingsKey::MAINWINDOW_TAGLIBRARY_STATE, tagLibrary->saveUiState());
    this->settings.save();

    qDebug() << __PRETTY_FUNCTION__ << "done";
}

void MainWindow::readSettings() {
    ZoneScoped;

    qDebug() << __PRETTY_FUNCTION__;

    QSettings settings;
    restoreGeometry(settings.value(SettingsKey::MAINWINDOW_GEOMETRY, QByteArray()).toByteArray());
    restoreState(settings.value(SettingsKey::MAINWINDOW_STATE, QByteArray()).toByteArray());
    dockManager->restoreState(settings.value(SettingsKey::MAINWINDOW_DOCKMANAGER_STATE, QByteArray()).toByteArray());
    imageViewer->restoreUiState(settings.value(SettingsKey::MAINWINDOW_IMAGEVIEWER_STATE, QByteArray()).toByteArray());
    fileBrowser->restoreUiState(settings.value(SettingsKey::MAINWINDOW_FILEBROWSER_STATE, QByteArray()).toByteArray());
    tagLibrary->restoreUiState(settings.value(SettingsKey::MAINWINDOW_TAGLIBRARY_STATE, QByteArray()).toByteArray());
    this->settings.load();

    if(this->settings.interface.language != translator.language()) {
        QApplication::removeTranslator(&translator);

        if (!translator.load(QLocale(this->settings.interface.language), QLatin1String("simpletagger-cxx"), QLatin1String("_"), QLatin1String(":/i18n")))
            qWarning() << "Could not load language" << this->settings.interface.language;
        else
            QApplication::installTranslator(&translator);
    }

    setStyleSheet(QString("Font: %1pt").arg(this->settings.interface.fontSize));

    fileEditor_->setBackupOnEverySave(this->settings.system.backupOnAnyChange);
    fileTagsManager.setBackupOnSave(this->settings.system.backupOnAnyChange);

    QFont font;
    font.setPointSizeF(this->settings.interface.fontSize);
    QFontMetrics fontMetrics(font);
    tagLibrary->setRowHeight(fontMetrics.height());

    imageViewer->setEnforcedAspectRatiosEnabled(this->settings.interface.imageFixedAspectRatios);

    recentProjects = settings.value(SettingsKey::RECENT_PROJECTS, QStringList{}).toStringList();
    refreshRecentProjects();
}

void MainWindow::refreshRecentProjects() {
    ZoneScoped;

    QSettings settings;
    settings.setValue(SettingsKey::RECENT_PROJECTS, recentProjects);

    ui->menuRecentProjects->setEnabled(!recentProjects.isEmpty());
    recentProjectsActions.clear();
    for (auto const &v: recentProjects) {
        auto &action = recentProjectsActions.emplace_back(std::make_unique<QAction>(this));
        action->setText(QFileInfo{v}.fileName());
        connect(action.get(), &QAction::triggered, this, [this, v]{
            if (auto result = loadProject(v); !result)
                QMessageBox::critical(
                        this,
                        tr("Project load failed"),
                        tr("Could not load project: %1").arg(result.error())
                );
        });
        ui->menuRecentProjects->addAction(action.get());
    }
}

std::expected<void, QString> MainWindow::loadProject(QString const &filePath) {
    ZoneScoped;

    if (filePath.isNull())
        project.reset();
    else {
        if (auto p = Project::open(filePath); !p)
            return std::unexpected(tr("Could not load project from file \"%1\": %2").arg(filePath, p.error()));
        else
            project.emplace(std::move(*p));
    }

    QSettings settings;
    settings.setValue(SettingsKey::LAST_PROJECT_LOCATION, QFileInfo{filePath}.dir().path());

    if (project)
        fileEditor_->setProject(*project);
    else
        fileEditor_->resetProject();

    bool enabled = project.has_value();
    ui->widgetCentral->setEnabled(enabled);
    directoryStatsManager.setProject(&*project);
    fileBrowser->setEnabled(enabled);
    tags_->setEnabled(enabled);
    ui->actionProjectSetup->setEnabled(enabled);
    ui->actionCloseProject->setEnabled(enabled);
    fileBrowser->setEnabled(enabled);

    if (enabled) {
        setWindowTitle(QString("SimpleTagger - %1").arg(project->path()));

        fileBrowser->setProjectRootPath(project->rootDir());
        fileBrowser->setDirectories(project->directories());

        auto lastDir = settings.value(SettingsKey::LAST_VIEWED_DIRECTORY, QString()).toString();

        static_cast<void>(fileBrowser->selectDirectoryInProjectList(lastDir));

        auto lastFile = settings.value(SettingsKey::LAST_VIEWED_FILE, QString()).toString();
        static_cast<void>(fileBrowser->selectFileInTree(lastFile));

        if (auto index = recentProjects.indexOf(project->path()); index != -1) {
            // move this project to the front of the list
            recentProjects.move(index, 0);
        } else {
            // add this project to the list
            recentProjects.push_front(project->path());
            if (recentProjects.size() > MAX_RECENT_PROJECTS)
                recentProjects.pop_back();
        }

        refreshRecentProjects();
    }

    return {};
}

void MainWindow::load(QString const &path, bool forceReopen) {
    ZoneScoped;
    gsl_Expects(path.isEmpty() || QFileInfo(path).isAbsolute());

    if (path != currentPath || forceReopen) {
        unload();
        fileEditor_->resetFile();
        currentPath.clear();

        if (!path.isEmpty()) {
            // must be set first, as many components rely on it
            if (auto result = fileEditor_->setFile(path); !result) {
                QMessageBox::critical(this, tr("Cannot load file"), result.error());
                return;
            }

            if (!QFileInfo(path).isDir())
                loadFile(path);

            currentPath = path;
        }
    }

    gsl_Ensures(currentPath == path);
}

void MainWindow::loadFile(QString const &path) {
    ZoneScoped;
    gsl_Expects(!path.isNull() && !QFileInfo(path).isDir() && QFileInfo(path).isAbsolute());

    qDebug() << "MainWindow::loadFile: " << path;

    imageViewer->loadFile(path);
    tags_->setEnabled(true);

    tagLibrary->setEnabled(true);

    loadFileTaggerTagsToTagLibrary();

    QString notification;
    QString notificationTooltip;

    std::optional<int> highlightSinceVersion;

    if (auto imageLibraryUuid = fileEditor_->imageTagLibraryUuid(); !imageLibraryUuid || imageLibraryUuid != tagLibrary->getUuid()) {
        notification = tr("This image was edited using a different Tag Library. You may want to review the tags according to the current library");
        notificationTooltip = tr("Image was edited with Tag Library with UUID: %1<br>Current Tag Library has UUID: %2")
                .arg(imageLibraryUuid ? imageLibraryUuid->toString(QUuid::WithoutBraces) : tr("(no library UUID saved)"))
                .arg(tagLibrary->getUuid().toString(QUuid::WithoutBraces));
    } else {
        auto imageLibraryVersion = fileEditor_->imageTagLibraryVersion();
        if (auto imageLibraryVersionUuid = fileEditor_->imageTagLibraryVersionUuid(); !imageLibraryVersionUuid || imageLibraryVersionUuid != tagLibrary->getVersionUuid()) {
            notification = tr("This image was edited using a different Tag Library version. You may want to review the tags according to the current library");

            notificationTooltip = tr("Image was edited with Tag Library version: <b>%1</b> (<small>%2</small>)<br>Current Tag Library version: <b>%3</b> (<small>%4</small>)")
                    .arg(imageLibraryVersion ? QString::number(*imageLibraryVersion) : "(no library version saved)")
                    .arg(imageLibraryUuid ? imageLibraryVersionUuid->toString(QUuid::WithoutBraces) : tr("no library UUID saved"))
                    .arg(tagLibrary->getVersion())
                    .arg(tagLibrary->getVersionUuid().toString(QUuid::WithoutBraces));
        }

        if (!imageLibraryVersion)
            highlightSinceVersion = -1; // basically all tags
        else
            highlightSinceVersion = *imageLibraryVersion;
    }

    tagLibrary->setHighlightChangedAfterVersion(highlightSinceVersion);

    if (!notification.isEmpty())
        imageViewer->showNotification(notification, notificationTooltip);
    else
        imageViewer->hideNotification();

    QSettings settings;
    settings.setValue(SettingsKey::LAST_VIEWED_FILE, currentPath);
}

void MainWindow::unload() {
    ZoneScoped;

    tagLibrary->setEnabled(false);

    tags_->setEnabled(false);
    imageViewer->unloadFile();
}

void MainWindow::loadFileTaggerTagsToTagLibrary() {
    ZoneScoped;

    if (auto tags = fileEditor_->assignedTags()) {
        if (auto result = tagLibrary->setTagsActive(*tags); !result)
            QMessageBox::critical(
                    this,
                    tr("Tags load failed"),
                    tr("Could not load active tags to the library: %1").arg(result.error())
            );
    }
}

void MainWindow::saveProject() {
    if (auto result = project->save(this->settings.system.backupOnAnyChange); !result)
        QMessageBox::critical(
                this,
                tr("Project saving failed"),
                tr("Could not save project: %1").arg(result.error())
        );
    else
        showSavedStatusMessage("project", *result);

}

void MainWindow::showSavedStatusMessage(QString const &dataType, std::optional<int> const &backupsCounter) {
    if (!backupsCounter)
        statusBar()->showMessage(tr("Saved %1").arg(dataType));
    else
        statusBar()->showMessage(tr("Saved %1 (%2 backups found)").arg(dataType).arg(*backupsCounter));
}
