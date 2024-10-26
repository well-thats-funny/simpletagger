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
#include "About.hpp"

#include "ui_About.h"

About::About(QWidget *const parent): QDialog(parent), ui(std::make_unique<Ui_About>()) {}

std::expected<std::unique_ptr<About>, QString> About::create(QWidget *const parent) {
    ZoneScoped;
    auto self = std::unique_ptr<About>(new About(parent));
    self->ui->setupUi(&*self);
    return self;
}

About::~About() = default;
