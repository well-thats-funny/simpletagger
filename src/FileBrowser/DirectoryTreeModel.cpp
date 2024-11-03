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
#include "DirectoryTreeModel.hpp"

#include "FileTagsManager.hpp"
#include "Utility.hpp"

namespace FileBrowser {
namespace {
    constexpr unsigned int ROW_HEIGHT = 80;
    constexpr unsigned int CACHE_QUALITY = 50;
}

// this provider never returns any correct icon. this accelerates the model a lot on slow disks (incl. preventing
// UI hangs). the reason is probably that the default implementation tries to inspect MIME type of every file
class CustomFileIconProvider: public QAbstractFileIconProvider {
public:
    mutable std::unordered_map<QString, QIcon> iconCache;
    mutable std::unordered_map<QString, QString> typeCache;

    QIcon icon(QAbstractFileIconProvider::IconType const type) const override {
        ZoneScoped;
        return QAbstractFileIconProvider::icon(type);
    }

    QIcon icon(QFileInfo const &fileInfo) const override {
        ZoneScoped;

        auto path = fileInfo.absoluteFilePath();
        auto it = iconCache.find(path);
        if (it == iconCache.end())
            std::tie(it, std::ignore) = iconCache.emplace(path, QAbstractFileIconProvider::icon(fileInfo));

        return it->second;
    }

    QString type(const QFileInfo &fileInfo) const override {
        ZoneScoped;

        auto path = fileInfo.absoluteFilePath();
        auto it = typeCache.find(path);
        if (it == typeCache.end())
            std::tie(it, std::ignore) = typeCache.emplace(path, QAbstractFileIconProvider::type(fileInfo));

        return it->second;
    }
};

DirectoryTreeModel::DirectoryTreeModel(
        QStyle *const style,
        FileTagsManager &fileTagsManager,
        IsFileExcluded const & isFileExcluded
):
        style{style},
        fileTagsManager_{fileTagsManager},
        isFileExcluded_{isFileExcluded},
        directoryIcon{style->standardPixmap(QStyle::SP_DirIcon)},
        cacheDir{QStandardPaths::standardLocations(QStandardPaths::StandardLocation::CacheLocation).at(0)+"/DirectoryTreeModelCache"} {
    ZoneScoped;

    iconProvider = std::make_unique<CustomFileIconProvider>();
    setIconProvider(&*iconProvider);

    if (!QDir{}.exists(cacheDir))
        if(!QDir{}.mkpath(cacheDir))
            qWarning() << "DirectoryTreeModel: could not create directory: " << cacheDir;

    connect(&fileTagsManager, &FileTagsManager::directoryStatsChanged, this, [this](auto const &path){
                ZoneScoped;
                gsl_Expects(QFileInfo(path).isAbsolute());
                gsl_Expects(QFileInfo(path).isDir());
                auto idx = index(path);
                emit dataChanged(idx, idx);
            },
            Qt::ConnectionType::QueuedConnection
    );
}

DirectoryTreeModel::~DirectoryTreeModel() = default;

int DirectoryTreeModel::columnCount(const QModelIndex &) const {
    return 1;
}

QVariant DirectoryTreeModel::headerData(int section, Qt::Orientation, int role) const {
    ZoneScoped;

    if (section == 0) {
        switch (role) {
            case Qt::ItemDataRole::DisplayRole:
                return tr("Image");
            default:
                return QVariant();
        }
    } else {
        qWarning() << "DirectoryTreeModel::headerData: invalid section " << section;
        return QVariant();
    }
}

QVariant DirectoryTreeModel::data(const QModelIndex &index, int role) const {
    ZoneScoped;

    auto path = filePath(index);
    QFileInfo pathInfo{path};

    if (index.column() == 0) {
        switch (role) {
            case Qt::ItemDataRole::DisplayRole:
                if (pathInfo.isDir()) {
                    if (auto &stats = fileTagsManager_.directoryStats(path); stats.ready()) {
                        return QString(
                                "%1\n"
                                "%2 / %3 files have tags (%4%)\n"
                                "%5 total tags")
                                .arg(pathInfo.fileName())
                                .arg(stats.filesWithTags())
                                .arg(stats.fileCount())
                                        // I don't know how to format numbers with Qt :<
                                .arg(QString::fromStdString(std::format(
                                        "{:.2f}",
                                        (static_cast<float>(stats.filesWithTags()) /
                                         static_cast<float>(stats.fileCount())) * 100.f
                                )))
                                .arg(stats.totalTags());
                    } else {
                        return QString("%1\n%2")
                            .arg(pathInfo.fileName())
                            .arg(tr("(loading statistics...)"));
                    }
                } else {
                    auto &fileTags = fileTagsManager_.forFile(path);
                    QString label = pathInfo.fileName() + "\n";
                    if (auto rect = fileTags.imageRegion()) {
                        int w = rect->right() - rect->left();
                        int h = rect->bottom() - rect->top();
                        auto ar = toMinimalAspectRatio(QSize(w, h));
                        label += QString("region: %1x%2 (%3:%4)\n").arg(
                                QString::number(w), QString::number(h),
                                QString::number(ar.width()), QString::number(ar.height())
                        );
                    }
                    else
                        label += "no assigned region\n";

                    static constexpr int maxTags = 5;
                    if (fileTags.assignedTags().isEmpty())
                        label += "no assigned tags";
                    else if (fileTags.assignedTags().size() <= maxTags)
                        label += QString("%1 tags: %2")
                                .arg(fileTags.assignedTags().size())
                                .arg(fileTags.assignedTags().join(", "));
                    else
                        label += QString("%1 tags: %2, ...")
                                .arg(fileTags.assignedTags().size())
                                .arg(fileTags.assignedTags().sliced(0, maxTags).join(", "));
                    return label;
                }
            case Qt::ItemDataRole::DecorationRole:
                if (pathInfo.isDir())
                    return style->standardPixmap(QStyle::SP_DirIcon);
                else
                    return getImage(path);
            case Qt::ItemDataRole::SizeHintRole:
                if (pathInfo.isDir())
                    return {};
                else
                    return QSize(10, ROW_HEIGHT);
            case Qt::ItemDataRole::FontRole: {
                QFont font;
                font.setItalic(isFileExcluded_(path));
                return font;
            }
            case Qt::ItemDataRole::BackgroundRole:
                if (pathInfo.isDir()) {
                    if (auto &stats = fileTagsManager_.directoryStats(path); stats.ready()) {
                        if (stats.filesWithTags() == stats.fileCount())
                            return QBrush(QColor(0, 255, 0, 64), Qt::BrushStyle::SolidPattern);
                        else if (stats.filesWithTags() != 0)
                            return QBrush(Qt::GlobalColor::green, Qt::BrushStyle::FDiagPattern);
                        else
                            return {};
                    } else {
                        return QBrush(Qt::GlobalColor::cyan, Qt::BrushStyle::FDiagPattern);
                    }
                } else {
                    if (isFileExcluded_(path))
                        return QBrush(Qt::GlobalColor::gray, Qt::BrushStyle::SolidPattern);
                    else if (fileTagsManager_.forFile(path).assignedTags().size() != 0)
                        return QBrush(QColor(0, 255, 0, 64), Qt::BrushStyle::SolidPattern);
                    else
                        return QBrush();
                }
                break;
            default:
                return {};
        }
    } else {
        qWarning() << "DirectoryTreeModel::data: invalid column " << index.column();
        return {};
    }
}

bool DirectoryTreeModel::event(QEvent *event) {
    ZoneScoped;

    QElapsedTimer timer;
    timer.start();

    auto result = QFileSystemModel::event(event);

    if (auto ms = timer.elapsed(); ms > 100)
        qWarning() << "DirectoryTreeModel event" << event << "took" << ms << "ms (event:" << event << ")";

    return result;
}

void DirectoryTreeModel::refreshExcludedState(QString const &file) {
    ZoneScoped;
    gsl_Expects(QFileInfo(file).isAbsolute());
    auto idx = index(file);
    assert(idx.isValid());
    emit dataChanged(idx, idx);
}

QImage DirectoryTreeModel::getImage(QString const &path) const {
    ZoneScoped;

    // TODO: clear cache if it becomes too huge

    auto it = imageCache.find(path);
    if (it == imageCache.end()) {
        auto cacheEntry = QCryptographicHash::hash(path.toUtf8(), QCryptographicHash::Algorithm::Sha256);
        auto cachePath = QString("%1/%2.%3.%4.jpg").arg(
                cacheDir, QString::fromLatin1(cacheEntry.toHex()), QString::number(ROW_HEIGHT), QString::number(CACHE_QUALITY)
        );

        QFileInfo cachePathInfo{cachePath};
        QImage image;
        if (!cachePathInfo.exists()) {
            qDebug() << "DirectoryTreeModel::getImage: Loading: " << path;
            image = QImage{path};

            qDebug() << "DirectoryTreeModel::getImage: Scaling to height " << ROW_HEIGHT;
            image = image.scaledToHeight(ROW_HEIGHT, Qt::TransformationMode::SmoothTransformation);

            qDebug() << "DirectoryTreeModel::getImage: Saving: " << cachePath;
            if (!image.save(cachePath, nullptr, CACHE_QUALITY))
                qWarning() << "DirectoryTreeModel::getImage: Could not save cache file: " << cachePath;
        } else
            image = QImage{cachePath};

        std::tie(it, std::ignore) = imageCache.emplace(path, image);
    }

    return it->second;
}
}
