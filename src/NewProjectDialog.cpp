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
#include "NewProjectDialog.hpp"

#include "Project.hpp"
#include "Constants.hpp"
#include "ui_NewProjectDialog.h"

NewProjectDialog::NewProjectDialog(QWidget *parent, Qt::WindowFlags f):
    QDialog{parent, f},
    ui{std::make_unique<Ui_NewProjectDialog>()}
{
    ZoneScoped;

    ui->setupUi(this);

    connect(ui->buttonProjectFileBrowse, &QPushButton::clicked, this, [this]{
        ZoneScoped;
        QSettings settings;
        if (auto path = QFileDialog::getSaveFileName(
                this,
                "New project file name",
                settings.value(SettingsKey::LAST_PROJECT_LOCATION, QString()).toString(),
                Constants::PROJECT_FILE_FILTER.toString()
        ); !path.isEmpty())
            ui->lineEditProjectFile->setText(path);
    });

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, [this]{
        ZoneScoped;
        auto path = ui->lineEditProjectFile->text();

        if (auto result = Project::create(path); !result) {
            QMessageBox::critical(
                    this,
                    tr("Project creation failed"),
                    tr("Could not create project: %1").arg(result.error())
            );
        } else {
            result_ = path;
            close();
        }
    });
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, [this]{ close(); });
}

NewProjectDialog::~NewProjectDialog() = default;

QString NewProjectDialog::result() const {
    return result_;
}
