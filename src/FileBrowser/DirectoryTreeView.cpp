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
#include "DirectoryTreeView.hpp"

namespace FileBrowser {
DirectoryTreeView::DirectoryTreeView(QWidget *const parent): CustomTreeView(parent) {
    ZoneScoped;

    connect(this, &DirectoryTreeView::entered, this, [this](QModelIndex const &index){
        mouseOverIndex_ = index;
    });

    viewport()->setMouseTracking(true);
}

DirectoryTreeView::~DirectoryTreeView() = default;

void DirectoryTreeView::mousePressEvent(QMouseEvent *const event) {
    ZoneScoped;

    if (event && event->button() == Qt::MouseButton::RightButton) {

    } else {
        return CustomTreeView::mousePressEvent(event);
    }
}

void DirectoryTreeView::mouseReleaseEvent(QMouseEvent *const event) {
    ZoneScoped;

    if (event && event->button() == Qt::MouseButton::RightButton) {

    } else {
        return CustomTreeView::mouseReleaseEvent(event);
    }
}

QModelIndex DirectoryTreeView::mouseOverIndex() const {
    return mouseOverIndex_;
}
}
