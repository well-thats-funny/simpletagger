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
#include "Settings.hpp"

class Ui_SettingsDialog;

class SettingsDialog: public QDialog {
    Q_OBJECT

public:
    SettingsDialog(SettingsDialog const &other) = delete;
    SettingsDialog(SettingsDialog &&other) = delete;
    SettingsDialog &operator=(SettingsDialog const &other) = delete;
    SettingsDialog &operator=(SettingsDialog &&other) = delete;

    SettingsDialog(Settings const &settings, QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    ~SettingsDialog();

    Settings const &settings() const;

private:
    Settings settings_;
    std::unique_ptr<Ui_SettingsDialog> ui;
    QStandardItemModel languagesList;
    QStandardItemModel customTagsFilesList;
};
