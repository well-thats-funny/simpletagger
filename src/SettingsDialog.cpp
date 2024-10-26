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
#include "SettingsDialog.hpp"

#include "ui_SettingsDialog.h"

SettingsDialog::SettingsDialog(Settings const &settings, QWidget *parent, Qt::WindowFlags f):
    QDialog(parent, f), settings_(settings), ui{std::make_unique<Ui_SettingsDialog>()} {
    ZoneScoped;

    ui->setupUi(this);

    for (auto const &language: settings.interface.availableLanguages()) {
        auto label = (language.nativeName != language.translatedName)
                ? QString("%1 (%2) [%3]").arg(language.nativeName, language.translatedName, language.code)
                : QString("%1 [%2]").arg(language.nativeName, language.code);
        auto item = std::make_unique<QStandardItem>(label);
        item->setData(language.code, Qt::ItemDataRole::UserRole);
        languagesList.appendRow(item.release());
    }

    // interface
    ui->comboBoxLanguages->setModel(&languagesList);
    ui->comboBoxLanguages->setCurrentIndex(ui->comboBoxLanguages->findData(settings_.interface.language, Qt::ItemDataRole::UserRole));
    connect(ui->comboBoxLanguages, &QComboBox::currentIndexChanged, this, [this](int const index){
        settings_.interface.language = ui->comboBoxLanguages->itemData(index, Qt::ItemDataRole::UserRole).toString();
    });
    ui->spinBoxFontSize->setValue(settings_.interface.fontSize);
    connect(ui->spinBoxFontSize, &QDoubleSpinBox::valueChanged, this, [this](double const value){
        settings_.interface.fontSize = value;
    });
    ui->spinBoxTagRowHeight->setValue(settings_.interface.tagRowHeight);
    connect(ui->spinBoxTagRowHeight, &QSpinBox::valueChanged, this, [this](int const value){
        settings_.interface.tagRowHeight = value;
    });
    ui->checkBoxImageFixedAspectRatios->setCheckState(settings_.interface.imageFixedAspectRatios ? Qt::Checked : Qt::CheckState::Unchecked);
    connect(ui->checkBoxImageFixedAspectRatios, &QCheckBox::checkStateChanged, this, [this](Qt::CheckState const value){
        settings_.interface.imageFixedAspectRatios = (value == Qt::CheckState::Checked);
    });

    // system
    ui->checkBoxBackupOnAnyChange->setCheckState(settings_.system.backupOnAnyChange ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
    connect(ui->checkBoxBackupOnAnyChange, &QCheckBox::checkStateChanged, this, [this](Qt::CheckState const value){
        settings_.system.backupOnAnyChange = (value == Qt::CheckState::Checked);
    });
}

SettingsDialog::~SettingsDialog() = default;

Settings const &SettingsDialog::settings() const {
    return settings_;
}
