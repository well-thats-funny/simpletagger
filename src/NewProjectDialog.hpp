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

class Ui_NewProjectDialog;

class NewProjectDialog: public QDialog {
    Q_OBJECT

public:
    explicit NewProjectDialog(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    ~NewProjectDialog();

    QString result() const;

private:
    std::unique_ptr<Ui_NewProjectDialog> ui;
    QString result_;
};
