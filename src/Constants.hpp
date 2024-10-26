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

namespace Constants {
    constexpr QAnyStringView PROJECT_FILE_FILTER = "*.simtagproj";
    constexpr QAnyStringView TAGS_FILE_SUFFIX = ".simtags.cbor";
}

namespace SettingsKey {
    // saved geometry of the main window
    constexpr QAnyStringView MAINWINDOW_GEOMETRY = "mainwindow_geometry";

    // saved state of the main window
    constexpr QAnyStringView MAINWINDOW_STATE = "mainwindow_state";

    constexpr QAnyStringView MAINWINDOW_DOCKMANAGER_STATE = "mainwindow_dockmanager_state";

    constexpr QAnyStringView MAINWINDOW_IMAGEVIEWER_STATE = "mainwindow_imageviewer_state";
    constexpr QAnyStringView MAINWINDOW_FILEBROWSER_STATE = "mainwindow_filebrowser_state";
    constexpr QAnyStringView MAINWINDOW_TAGLIBRARY_STATE = "mainwindow_taglibrary_state";

    // list of the recently opened projects
    constexpr QAnyStringView RECENT_PROJECTS = "recent_projects";

    // directory of the most recently opened or created project
    constexpr QAnyStringView LAST_PROJECT_LOCATION = "last_project_location";

    // last directory viewed in the main window combo box
    constexpr QAnyStringView LAST_VIEWED_DIRECTORY = "last_viewed_directory";

    // last file viewed in the main window
    constexpr QAnyStringView LAST_VIEWED_FILE = "last_viewed_file";
}
