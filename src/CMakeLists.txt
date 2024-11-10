#
# Copyright (C) 2024 fdresufdresu@gmail.com
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#
configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/icons.qrc.in"
        "${CMAKE_CURRENT_BINARY_DIR}/icons.qrc"
)

configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/user-icons.qrc.in"
        "${CMAKE_CURRENT_BINARY_DIR}/user-icons.qrc"
)

qt_standard_project_setup(I18N_TRANSLATED_LANGUAGES de pl)

qt_add_resources(QT_ICONS_QRC_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/icons.qrc")
qt_add_resources(QT_USER_ICONS_QRC_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/user-icons.qrc")

add_executable(simpletagger-cxx
        ${QT_ICONS_QRC_SOURCES}
        ${QT_USER_ICONS_QRC_SOURCES}
        About.cpp
        About.hpp
        Constants.hpp
        CustomTreeView.cpp
        CustomTreeView.hpp
        FileBrowser/DirectoryTreeModel.cpp
        FileBrowser/DirectoryTreeModel.hpp
        FileBrowser/DirectoryTreeProxyModel.cpp
        FileBrowser/DirectoryTreeProxyModel.hpp
        FileBrowser/DirectoryTreeView.cpp
        FileBrowser/DirectoryTreeView.hpp
        FileBrowser/FileBrowser.cpp
        FileBrowser/FileBrowser.hpp
        FileBrowser/ProjectDirectoryListModel.cpp
        FileBrowser/ProjectDirectoryListModel.hpp
        FileEditor.cpp
        FileEditor.hpp
        FileTagsManager.cpp
        FileTagsManager.hpp
        IconIdentifier.cpp
        IconIdentifier.hpp
        ImageViewer/GraphicsSelectionRectItem.cpp
        ImageViewer/GraphicsSelectionRectItem.hpp
        ImageViewer/ImageGraphicsView.cpp
        ImageViewer/ImageGraphicsView.hpp
        ImageViewer/ImageViewer.cpp
        ImageViewer/ImageViewer.hpp
        MainWindow.cpp
        MainWindow.hpp
        MainWindow.ui
        NewProjectDialog.cpp
        NewProjectDialog.hpp
        NewProjectDialog.ui
        Project.cpp
        Project.hpp
        Settings.cpp
        Settings.hpp
        SettingsDialog.cpp
        SettingsDialog.hpp
        SettingsDialog.ui
        StartupDialog.cpp
        StartupDialog.hpp
        StartupDialog.ui
        TagLibrary/CommentEditor.cpp
        TagLibrary/CommentEditor.hpp
        TagLibrary/Format.hpp
        TagLibrary/Library.cpp
        TagLibrary/Library.hpp
        TagLibrary/Logging.cpp
        TagLibrary/Logging.hpp
        TagLibrary/Model.cpp
        TagLibrary/Model.hpp
        TagLibrary/Node.cpp
        TagLibrary/Node.hpp
        TagLibrary/NodeCollection.cpp
        TagLibrary/NodeCollection.cpp
        TagLibrary/NodeCollection.hpp
        TagLibrary/NodeCollection.hpp
        TagLibrary/NodeHierarchical.cpp
        TagLibrary/NodeHierarchical.cpp
        TagLibrary/NodeHierarchical.hpp
        TagLibrary/NodeHierarchical.hpp
        TagLibrary/NodeLink.cpp
        TagLibrary/NodeLink.hpp
        TagLibrary/NodeLinkSubtree.cpp
        TagLibrary/NodeLinkSubtree.hpp
        TagLibrary/NodeObject.cpp
        TagLibrary/NodeObject.hpp
        TagLibrary/NodeRoot.cpp
        TagLibrary/NodeRoot.hpp
        TagLibrary/NodeSerializable.cpp
        TagLibrary/NodeSerializable.hpp
        TagLibrary/SelectionHelperProxyModel.cpp
        TagLibrary/SelectionHelperProxyModel.hpp
        TagLibrary/TreeView.cpp
        TagLibrary/TreeView.hpp
        Tags/Tags.cpp
        Tags/Tags.hpp
        Tags/TagsAssignedListModel.cpp
        Tags/TagsAssignedListModel.hpp
        Utility.cpp
        Utility.hpp
        main.cpp
        TagProcessor.cpp
        TagProcessor.hpp
        FileBrowser/Utility.hpp
        FileBrowser/Utility.cpp
        VerticalLine.cpp
        VerticalLine.hpp
        TagLibrary/FilterProxyModel.cpp
        TagLibrary/FilterProxyModel.hpp
        DirectoryStatsManager.cpp
        DirectoryStatsManager.hpp
        DirectoryStats.cpp
        DirectoryStats.hpp
        TagLibrary/LibraryInfoDialog.cpp
        TagLibrary/LibraryInfoDialog.hpp
)
target_compile_options(simpletagger-cxx PRIVATE -Wall -Wextra -Werror)

target_compile_definitions(simpletagger-cxx PRIVATE gsl_CONFIG_CONTRACT_VIOLATION_ASSERTS)

target_include_directories(simpletagger-cxx PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(simpletagger-cxx PRIVATE gsl::gsl-lite-v1 Qt6::Widgets TracyClient ads::qt6advanceddocking)
target_precompile_headers(simpletagger-cxx PRIVATE
        <expected>
        <experimental/scope>
        <format>
        <generator>
        <ranges>
        <set>
        <syncstream>
        <unordered_map>
        <unordered_set>

        <gsl/gsl-lite.hpp>

        <QAnyStringView>
        <QAbstractFileIconProvider>
        <QApplication>
        <QBrush>
        <QBuffer>
        <QCborArray>
        <QCborMap>
        <QCborStreamReader>
        <QCborStreamWriter>
        <QCborValue>
        <QCommandLineParser>
        <QCryptographicHash>
        <QDesktopServices>
        <QDebug>
        <QDialog>
        <QDirIterator>
        <QDockWidget>
        <QFile>
        <QFileDialog>
        <QFileSystemModel>
        <QGraphicsItemGroup>
        <QGraphicsPixmapItem>
        <QGraphicsPolygonItem>
        <QGraphicsRectItem>
        <QGraphicsScene>
        <QGraphicsSceneMouseEvent>
        <QGraphicsSimpleTextItem>
        <QGraphicsView>
        <QHash>
        <QIdentityProxyModel>
        <QItemSelection>
        <QJsonArray>
        <QJsonDocument>
        <QJsonObject>
        <QJsonValue>
        <QLabel>
        <QLoggingCategory>
        <QMainWindow>
        <QMenu>
        <QMessageBox>
        <QMetaEnum>
        <QMimeData>
        <QMutex>
        <QMutexLocker>
        <QPushButton>
        <QSaveFile>
        <QScrollBar>
        <QSettings>
        <QSortFilterProxyModel>
        <QSpinBox>
        <QStandardItemModel>
        <QStandardPaths>
        <QStatusBar>
        <QStyle>
        <QStyledItemDelegate>
        <QString>
        <QThreadPool>
        <QTimer>
        <QToolButton>
        <QToolTip>
        <QTranslator>
        <QTreeView>
        <QWheelEvent>

        <DockManager.h>

        <tracy/Tracy.hpp>
)

qt_add_translations(
        TARGETS simpletagger-cxx
        SOURCE_TARGETS simpletagger-cxx
        TS_FILE_DIR i18n
        LUPDATE_OPTIONS -noobsolete
)
