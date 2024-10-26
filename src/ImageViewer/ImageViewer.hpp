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

class Ui_ImageViewer;

class FileTagger;

namespace ImageViewer {
class GraphicsSelectionRectItem;

class ImageViewer: public QWidget {
    Q_OBJECT

    ImageViewer(ImageViewer const &other) = delete;
    ImageViewer(ImageViewer &&other) = delete;
    ImageViewer& operator=(ImageViewer const &other) = delete;
    ImageViewer& operator=(ImageViewer &&other) = delete;

    explicit ImageViewer(FileTagger &fileTagger);
    [[nodiscard]] std::expected<void, QString> init();

public:
    [[nodiscard]] static std::expected<std::unique_ptr<ImageViewer>, QString> create(FileTagger &fileTagger);
    ~ImageViewer() override;

    QByteArray saveUiState();
    void restoreUiState(QByteArray const &value);

    void loadFile(QString const &name);
    void unloadFile();

    void setZoom(float zoom);
    void setZoomToFitWholeImage();
    void setZoomToFitSelectionRect();
    void extractZoomLevelFromCurrentTransformation();
    void afterZoomChange();

    void setEnforcedAspectRatiosEnabled(bool enabled);

private:
    std::unique_ptr<Ui_ImageViewer> ui;

    FileTagger &fileTagger_;

    bool enforcedAspectRatiosEnabled_ = false;

    float zoom_ = 1.0;

    QGraphicsScene viewFileScene;
    std::optional<QGraphicsPixmapItem> viewFilePixmapItem;
    GraphicsSelectionRectItem *viewSelectionRectItem = nullptr;
};
}
