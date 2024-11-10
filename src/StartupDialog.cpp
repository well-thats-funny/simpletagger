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
#include "StartupDialog.hpp"

#include "ui_StartupDialog.h"
#include "NewProjectDialog.hpp"
#include "Constants.hpp"

StartupDialog::StartupDialog(QWidget *const parent, Qt::WindowFlags const f):
        QDialog{parent, f},
        ui{std::make_unique<Ui_StartupDialog>()} {
    ZoneScoped;

    ui->setupUi(this);

    connect(ui->buttonCreateNewProject, &QPushButton::clicked, this, [this]{
        ZoneScoped;

        NewProjectDialog newProjectDialog(this);
        newProjectDialog.exec();

        if (auto path = newProjectDialog.result(); !path.isEmpty()) {
            project = path;
            outcome = Outcome::OpenProject;
            close();
        }
    });

    connect(ui->buttonOpenExistingProject, &QPushButton::clicked, this, [this]{
        ZoneScoped;

        QSettings settings;
        if (auto path = QFileDialog::getOpenFileName(
                    this,
                    tr("Open project"),
                    settings.value(SettingsKey::LAST_PROJECT_LOCATION, QString()).toString(),
                    Constants::PROJECT_FILE_FILTER.toString()
            ); !path.isEmpty()) {
            project = path;
            outcome = Outcome::OpenProject;
            close();
        }
    });

    connect(ui->buttonExit, &QPushButton::clicked, this, [this]{ outcome = Outcome::Exit; close(); });
}

StartupDialog::~StartupDialog() = default;
