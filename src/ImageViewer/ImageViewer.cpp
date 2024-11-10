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
#include "ImageViewer.hpp"

#include "GraphicsSelectionRectItem.hpp"

#include "../FileEditor.hpp"
#include "../Utility.hpp"

#include "ui_ImageViewer.h"

namespace {
constexpr float ZOOM_STEP_BUTTONS = 0.1f;
constexpr float ZOOM_STEP_WHEEL = 0.1f/120.0f;
}

namespace ImageViewer {
ImageViewer::ImageViewer(FileEditor &fileEditor): ui(std::make_unique<Ui_ImageViewer>()), fileEditor_(fileEditor) {}

std::expected<void, QString> ImageViewer::init() {
    ZoneScoped;
    ui->setupUi(this);

    connect(&fileEditor_, &FileEditor::modifiedStateChanged, this, [this](bool const modified){
        ui->actionSaveData->setEnabled(modified);
    });

    ui->buttonSaveData->setDefaultAction(ui->actionSaveData);
    connect(ui->actionSaveData, &QAction::triggered, this, [this]{
        ZoneScoped;
        if (auto result = fileEditor_.save(); !result)
            QMessageBox::critical(this, tr("Save failed"), result.error());
    });

    ui->buttonOpenExternal->setDefaultAction(ui->actionOpenExternal);
    connect(ui->actionOpenExternal, &QAction::triggered, this, [this]{
        ZoneScoped;
        gsl_Expects(!name_.isEmpty());
        if (!QDesktopServices::openUrl(QUrl::fromLocalFile(name_)))
            QMessageBox::critical(this, tr("Open failed"), tr("Could not open file in external program: %1").arg(name_));
    });

    ui->buttonRegionEdit->setDefaultAction(ui->actionRegionEdit);
    connect(ui->actionRegionEdit, &QAction::triggered, this, [this]{
        ZoneScoped;
        if (viewSelectionRectItem)
            viewSelectionRectItem->setEditable(ui->actionRegionEdit->isChecked());
    });

    ui->buttonZoomIn->setDefaultAction(ui->actionZoomIn);
    connect(ui->actionZoomIn, &QAction::triggered, this, [this]{
        ZoneScoped;
        setZoom(zoom_ + ZOOM_STEP_BUTTONS);
    });

    ui->buttonZoomOut->setDefaultAction(ui->actionZoomOut);
    connect(ui->actionZoomOut, &QAction::triggered, this, [this]{
        ZoneScoped;
        setZoom(zoom_ - ZOOM_STEP_BUTTONS);
    });

    ui->buttonZoomToFitWholeImage->setDefaultAction(ui->actionZoomToFitWholeImage);
    connect(ui->actionZoomToFitWholeImage, &QAction::triggered, this, [this]{
        ZoneScoped;
        setZoomToFitWholeImage();
    });

    ui->buttonZoomToFitRegion->setDefaultAction(ui->actionZoomToFitRegion);
    connect(ui->actionZoomToFitRegion, &QAction::triggered, this, [this]{
        ZoneScoped;
        setZoomToFitSelectionRect();
    });

    ui->buttonZoomReset->setDefaultAction(ui->actionZoomReset);
    connect(ui->actionZoomReset, &QAction::triggered, this, [this]{
        ZoneScoped;
        setZoom(1.0);
    });

    connect(ui->buttonNotificationCancel, &QToolButton::clicked, this, [this]{
        ZoneScoped;
        hideNotification();
    });

    ui->imageView->setScene(&viewFileScene);
    connect(ui->imageView, &ImageGraphicsView::wheelZoom, this, [this](QPoint const &angle){
        ZoneScoped;
        setZoom(zoom_ + static_cast<float>(angle.y()) * ZOOM_STEP_WHEEL);
    });

    hideNotification();
    return {};
}

std::expected<std::unique_ptr<ImageViewer>, QString> ImageViewer::create(FileEditor &fileEditor) {
    ZoneScoped;

    std::unique_ptr<ImageViewer> self(new ImageViewer(fileEditor));
    if (auto result = self->init(); !result)
        return std::unexpected(result.error());

    return self;
}

ImageViewer::~ImageViewer() = default;

QByteArray ImageViewer::saveUiState() {
    ZoneScoped;

    QByteArray result;
    QBuffer buffer(&result);
    buffer.open(QIODevice::WriteOnly);

    QDataStream out(&buffer);
    out.setVersion(QDataStream::Version::Qt_6_7);
    out << ui->actionRegionEdit->isChecked();

    buffer.close();
    return result;
}

void ImageViewer::restoreUiState(QByteArray const &value) {
    ZoneScoped;

    QBuffer buffer;
    buffer.setData(value);
    buffer.open(QIODevice::ReadOnly);

    QDataStream in(&buffer);
    bool checked;
    in >> checked;
    ui->actionRegionEdit->setChecked(checked);
}

void ImageViewer::loadFile(QString const &name) {
    ZoneScoped;

    if (name != name_) {
        unloadFile();

        name_ = name;
        setEnabled(true);

        ui->labelImagePath->setText(name);

        QString toolTip;
        toolTip += tr("%1<br>").arg(name);
        toolTip += "<br>";

        if (auto libraryUuid = fileEditor_.imageTagLibraryUuid())
            toolTip += tr("Library: %1<br>").arg(libraryUuid->toString(QUuid::StringFormat::WithoutBraces));
        else
            toolTip += tr("Library: no data<br>");

        auto libraryVersion = fileEditor_.imageTagLibraryVersion();
        auto libraryVersionUuid = fileEditor_.imageTagLibraryVersionUuid();

        QString libraryVersionStr;
        if (libraryVersion && libraryVersionUuid)
            libraryVersionStr = tr("Library version: %1 (%2)")
                    .arg(*libraryVersion)
                    .arg(libraryVersionUuid->toString(QUuid::StringFormat::WithoutBraces));
        else if (libraryVersion)
            libraryVersionStr = tr("Library version: %1").arg(*libraryVersion);
        else if (libraryVersionUuid)
            libraryVersionStr = tr("Library version: %1").arg(libraryVersionUuid->toString(QUuid::StringFormat::WithoutBraces));
        else
            libraryVersionStr = tr("Library version: no data");


        toolTip += QString("%1<br>").arg(libraryVersionStr);
        ui->labelImagePath->setToolTip(toolTip);

        QImage image(name);
        viewFilePixmapItem.emplace(QPixmap::fromImage(image));
        viewFilePixmapItem->setTransformationMode(Qt::SmoothTransformation);

        viewFileScene.addItem(&*viewFilePixmapItem);

        auto viewSelectionRectItemUnique = std::make_unique<GraphicsSelectionRectItem>(image.rect(), image.rect());

        auto a = [](QColor const &color, float const alpha) {
            QColor color2(color);
            color2.setAlphaF(alpha);
            return color2;
        };

        QColor colorArea(QColorConstants::DarkGray);
        viewSelectionRectItemUnique->setAreaPen(QPen(colorArea, 10, Qt::PenStyle::SolidLine));
        viewSelectionRectItemUnique->setAreaBrush(QBrush(a(colorArea, 0.75f), Qt::BrushStyle::FDiagPattern));

        QColor editColorArea(QColorConstants::Red);
        viewSelectionRectItemUnique->setEditAreaPen(QPen(editColorArea, 10, Qt::PenStyle::SolidLine));
        viewSelectionRectItemUnique->setEditAreaBrush(QBrush(a(editColorArea, 0.5f), Qt::BrushStyle::FDiagPattern));

        QColor editColorAreaError(QColorConstants::Yellow);
        viewSelectionRectItemUnique->setEditErrorAreaPen(QPen(editColorAreaError, 10, Qt::PenStyle::SolidLine));
        viewSelectionRectItemUnique->setEditErrorAreaBrush(
                QBrush(a(editColorAreaError, 0.5f), Qt::BrushStyle::SolidPattern));

        QColor selectionCornerColor(QColorConstants::DarkRed);
        viewSelectionRectItemUnique->setActiveCornerPen(QPen(selectionCornerColor, 10, Qt::PenStyle::SolidLine));
        viewSelectionRectItemUnique->setActiveCornerBrush(
                QBrush(a(selectionCornerColor, 0.75f), Qt::BrushStyle::SolidPattern));

        QFont selectionFont;
        selectionFont.setPointSize(50);
        selectionFont.setBold(true);
        viewSelectionRectItemUnique->setLabelTextFont(selectionFont);
        viewSelectionRectItemUnique->setLabelTextBrush(QBrush(QColorConstants::White, Qt::BrushStyle::SolidPattern));
        viewSelectionRectItemUnique->setLabelAreaPen(viewSelectionRectItemUnique->editAreaPen());
        viewSelectionRectItemUnique->setLabelAreaBrush(QBrush(a(editColorArea, 0.5f), Qt::BrushStyle::SolidPattern));

        // TODO: configurable via settings/project settings?
        QList<QSize> aspectRatios;
        aspectRatios.append(QSize(1, 1));
        aspectRatios.append(QSize(1, 2));
        aspectRatios.append(QSize(2, 1));
        aspectRatios.append(QSize(2, 3));
        aspectRatios.append(QSize(3, 2));
        aspectRatios.append(QSize(3, 4));
        aspectRatios.append(QSize(4, 3));
        aspectRatios.append(QSize(8, 5));
        aspectRatios.append(QSize(5, 8));
        aspectRatios.append(QSize(9, 16));
        aspectRatios.append(QSize(16, 9));
        aspectRatios.append(QSize(9, 21));
        aspectRatios.append(QSize(21, 9));
        viewSelectionRectItemUnique->setEnforcedAspectRatios(aspectRatios);
        viewSelectionRectItemUnique->setEnforcedAspectRatiosEnabled(enforcedAspectRatiosEnabled_);

        viewFileScene.addItem(viewSelectionRectItem = viewSelectionRectItemUnique.release());
        viewSelectionRectItem->onRectChanged = [this](QRect const &rect, bool const final) {
            ZoneScoped;

            if (final)
                fileEditor_.setImageRegion(rect);

            auto left = rect.left();
            auto top = rect.top();
            auto right = rect.right();
            auto bottom = rect.bottom();
            auto w = rect.right() - rect.left();
            auto h = rect.bottom() - rect.top();

            auto aspectRatio = toMinimalAspectRatio(QSize(w, h));
            auto rw = aspectRatio.width();
            auto rh = aspectRatio.height();

            QString label;
            label += QString("rect (%1, %2)(%3, %4)\n").arg(QString::number(left), QString::number(top),
                                                            QString::number(right), QString::number(bottom));
            label += QString("size %1x%2\n").arg(QString::number(w), QString::number(h));
            label += QString("ratio %1:%2").arg(QString::number(rw), QString::number(rh));
            viewSelectionRectItem->setLabelText(label);
        };

        if (fileEditor_.imageRegion())
            viewSelectionRectItem->setRect(*fileEditor_.imageRegion());
        else
            viewSelectionRectItem->setRect(image.rect());

        viewSelectionRectItem->setEditable(false);

        if (fileEditor_.imageRegion())
            setZoomToFitSelectionRect();
        else
            setZoomToFitWholeImage();
    }
}

void ImageViewer::unloadFile() {
    ZoneScoped;

    if (viewSelectionRectItem) {
        viewFileScene.removeItem(viewSelectionRectItem);
        // TODO: Memory leak. Can't delete the object as it's apparently still held somewhere in the scene (despite of
        //       removeItem call above) and it tries to access it later. Will have to revisit it at some point, but
        //       for now I'll leave it leaking, as it's still better than a crash.
        //
        // NOTE: delayed delete didn't help too :<
        /*   QTimer::singleShot(0, [ptr=viewSelectionRectItem]{ delete ptr; });
           //delete viewSelectionRectItem;*/
        viewSelectionRectItem = nullptr;
    }

    if (viewFilePixmapItem) {
        viewFileScene.removeItem(&*viewFilePixmapItem);
        viewFilePixmapItem.reset();
    }

    ui->labelImagePath->clear();
    ui->labelImagePath->setToolTip(QString());

    name_.clear();

    setEnabled(false);
}

void ImageViewer::setZoom(float const zoom) {
    ZoneScoped;

    zoom_ = std::clamp(zoom, 0.1f, 10.f);

    float scale = zoom_*zoom_;

    qDebug() << "setZoom" << zoom_ << "(requested" << zoom << ") -> scale" << scale;

    ui->imageView->resetTransform();
    ui->imageView->scale(scale, scale);

    afterZoomChange();
}

void ImageViewer::setZoomToFitWholeImage() {
    ZoneScoped;

    if (viewFilePixmapItem) {
        ui->imageView->fitInView(&*viewFilePixmapItem, Qt::AspectRatioMode::KeepAspectRatio);
        extractZoomLevelFromCurrentTransformation();
        afterZoomChange();
    }
}

void ImageViewer::setZoomToFitSelectionRect() {
    ZoneScoped;

    if (viewSelectionRectItem) {
        if (auto region = fileEditor_.imageRegion()) {
            ui->imageView->fitInView(&*viewSelectionRectItem, Qt::AspectRatioMode::KeepAspectRatio);
            extractZoomLevelFromCurrentTransformation();
            afterZoomChange();
        }
    }
}

void ImageViewer::extractZoomLevelFromCurrentTransformation() {
    ZoneScoped;

    // this assumes that no rotation or shearing is applied to the image
    // based on https://math.stackexchange.com/a/13165

    auto a = ui->imageView->transform().m11();
    auto b = ui->imageView->transform().m12();
    auto c = ui->imageView->transform().m21();
    auto d = ui->imageView->transform().m22();

    auto scaleX = std::sqrt(a * a + b * b);
    scaleX = std::copysign(scaleX, a);

    auto scaleY = std::sqrt(c * c + d * d);
    scaleY = std::copysign(scaleY, d);

    qDebug() << "scaleX:" << scaleX << "; scaleY:" << scaleY;

    if (viewSelectionRectItem)
        viewSelectionRectItem->setScaleCorrection(scaleX);

    zoom_ = std::sqrt(scaleX);  // TODO: this doesn't really work like expected
}

void ImageViewer::afterZoomChange() {
    ZoneScoped;

    if (viewFilePixmapItem)
        viewFileScene.setSceneRect(viewFilePixmapItem->sceneBoundingRect());

    if (viewSelectionRectItem)
        viewSelectionRectItem->setScaleCorrection(zoom_ * zoom_);

    auto r = viewFileScene.sceneRect();
#ifdef NDEBUG
    ui->labelZoom->setText(QString("%1%").arg(std::round(zoom_ * 100.0f)).arg(r.x()).arg(r.y()).arg(r.width()).arg(r.height()));
#else
    ui->labelZoom->setText(QString("%1% | (%2 %3 %4 %5)").arg(std::round(zoom_ * 100.0f)).arg(r.x()).arg(r.y()).arg(r.width()).arg(r.height()));
#endif
}

void ImageViewer::setEnforcedAspectRatiosEnabled(bool const enabled) {
    ZoneScoped;

    enforcedAspectRatiosEnabled_ = enabled;

    if (viewSelectionRectItem)
        viewSelectionRectItem->setEnforcedAspectRatiosEnabled(enabled);
}

void ImageViewer::showNotification(QString const &notification, QString const &notificationTooltip) {
    ZoneScoped;

    ui->labelNotification->setText(notification);
    ui->labelNotification->setToolTip(notificationTooltip);

    ui->labelNotification->show();
    ui->buttonNotificationCancel->show();
}

void ImageViewer::hideNotification() {
    ui->labelNotification->hide();
    ui->buttonNotificationCancel->hide();
}
}
