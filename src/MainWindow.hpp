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
#include "DirectoryStatsManager.hpp"
#include "FileEditor.hpp"
#include "FileTagsManager.hpp"
#include "Project.hpp"
#include "Utility.hpp"

class Ui_MainWindow;

class Settings;

namespace ImageViewer {
class ImageViewer;
}

namespace FileBrowser {
class FileBrowser;
}

namespace Tags {
class Tags;
}

namespace TagLibrary {
class Library;
}


class MainWindow: public QMainWindow {
    Q_OBJECT

    MainWindow(MainWindow const &other) = delete;
    MainWindow(MainWindow &&other) = delete;
    MainWindow &operator=(MainWindow const &other) = delete;
    MainWindow &operator=(MainWindow &&other) = delete;

    MainWindow(Settings &settings, QTranslator &translator);
    [[nodiscard]] std::expected<void, QString> init();

    [[nodiscard]] std::expected<void, QString> setupGeneralActions();
    [[nodiscard]] std::expected<void, QString> setupStatusBar();
    [[nodiscard]] std::expected<void, QString> setupDocks();

    // takes ownership of widget!
    [[nodiscard]] std::unique_ptr<ads::CDockWidget> addDock(std::unique_ptr<QWidget> &&widget, QString const &label, ads::DockWidgetArea area);
    [[nodiscard]] std::expected<void, QString> setupImageViewerDock();
    [[nodiscard]] std::expected<void, QString> setupFileBrowserDock();
    [[nodiscard]] std::expected<void, QString> setupTagsDock();
    [[nodiscard]] std::expected<void, QString> setupTagLibraryDock();

    void loadStartupProject();

public:
    [[nodiscard]] static std::expected<std::unique_ptr<MainWindow>, QString> create(Settings &settings, QTranslator &translator);
    ~MainWindow();

private:
    void changeEvent(QEvent *) override;
    void paintEvent(QPaintEvent *event) override;

    void writeSettings();
    void readSettings();
    void refreshRecentProjects();
    [[nodiscard]] std::expected<void, QString> loadProject(QString const &filePath = QString());

    [[nodiscard]] std::expected<void, ErrorOrCancel> load(const QString &path, bool forceReopen = false);
    void loadFile(const QString &path);
    void loadDirectory(const QString &path);
    [[nodiscard]] std::expected<void, ErrorOrCancel> unload();

    void loadFileTaggerTagsToTagLibrary();
    void saveProject();
    void showSavedStatusMessage(QString const &dataType, std::optional<int> const &backupsCounter);

    std::unique_ptr<Ui_MainWindow> ui;
    QTimer statsTimer;

    Settings &settings;
    QTranslator &translator;

    QStringList recentProjects;
    std::vector<std::unique_ptr<QAction>> recentProjectsActions;
    std::optional<Project> project;

    FileTagsManager fileTagsManager;
    DirectoryStatsManager directoryStatsManager;
    std::optional<FileEditor> fileEditor_;

    std::unique_ptr<ads::CDockManager> dockManager;

    QString currentPath;

    ImageViewer::ImageViewer *imageViewer = nullptr; // owned by the dock widget
    std::unique_ptr<ads::CDockWidget> imageViewerDock;

    FileBrowser::FileBrowser *fileBrowser = nullptr; // owned by the dock widget
    std::unique_ptr<ads::CDockWidget> fileBrowserDock;

    Tags::Tags *tags_ = nullptr; // owned by the dock widget
    std::unique_ptr<ads::CDockWidget> tagsDock;

    TagLibrary::Library *tagLibrary = nullptr; // owned by the dock widget
    std::unique_ptr<ads::CDockWidget> tagLibraryDock;
    bool blockTagLibrarySetTagActive_ = false;

    QLabel *statusBarMemory = nullptr;
    QLabel *statusCache = nullptr;
    QMetaObject::Connection connectionSaveTagLibraryOnChange;
};
