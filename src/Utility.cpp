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
#include "Utility.hpp"

#include <unistd.h>

std::optional<MemoryState> fetchMemoryUsage() {
    // NOTE: this is linux specific!

    ZoneScoped;

    static std::optional<int> pageSize;
    if (!pageSize)
        pageSize = ::getpagesize();

    QFile file{"/proc/self/statm"};
    if (!file.open(QIODevice::ReadOnly))
        return std::nullopt;

    QTextStream stream(&file);

    std::uint64_t total, resident, shared, text, library, data, dirty;
    stream >> total >> resident >> shared >> text >> library >> data >> dirty;
//    qDebug() << "statm: total:" << total << "; resident:" << resident << "; shared:" << shared << "; text:" << text << "; library (unused):" << library << "; data:" << data << "; dirty pages (unused):" << dirty;

    MemoryState result;
    result.total = total * (*pageSize);
    result.resident = resident * (*pageSize);
    result.shared = shared * (*pageSize);
    result.text = text * (*pageSize);
    result.data = data * (*pageSize);

//    qDebug() << "statm (bytes): total:" << result.total << "; resident:" << result.resident << "; shared:" << result.shared << "; text:" << result.text << "; data:" << result.data;
    return result;
}

std::optional<int> backupFile(QString const &fileName) {
    ZoneScoped;

    QFileInfo fileInfo(fileName);

    QString backupFileName = fileInfo.dir().filePath(QString("%1.bak").arg(fileInfo.fileName()));

    std::optional<int> result;

    for(int i = 0; QFileInfo::exists(backupFileName); ++i) {
        backupFileName = fileInfo.dir().filePath(QString("%1.bak%2").arg(fileInfo.fileName(), QString::number(i)));
        result = i;
    }

    qDebug() << "Backing up" << fileName << "as" << backupFileName;
    QFile::copy(fileName, backupFileName);
    qDebug() << "Backing up" << fileName << "as" << backupFileName << ": done";
    return result.transform([](auto const &v){ return v+2; });
}

QSize toMinimalAspectRatio(QSize const &aspectRatio) {
    ZoneScoped;

    // https://stackoverflow.com/a/1186465
    auto w = static_cast<int>(aspectRatio.width());
    auto h = static_cast<int>(aspectRatio.height());
    auto d = std::gcd(w, h);
    auto rw = w/d;
    auto rh = h/d;
    return QSize(rw, rh);
}

QPointF closestPointOnLine(qreal a, qreal b, qreal c, QPointF const &p) {
    ZoneScoped;

    // https://en.wikipedia.org/wiki/Distance_from_a_point_to_a_line#Line_defined_by_an_equation
    qreal x = (b*( b*p.x() - a*p.y()) - a*c) / (a*a + b*b);
    qreal y = (a*(-b*p.x() + a*p.y()) - b*c) / (a*a + b*b);
    return {x, y};
}

QPointF closestPointOnLine(qreal a, qreal b, QPointF const &p) {
    ZoneScoped;

    // convert from linear function y = ax+b to linear equation Ax + By + C = 0
    // y = ax + b   <=>   ax - y + b = 0
    // so: A = a, B = -1, C = b
    return closestPointOnLine(a, -1, b, p);
}

QString cborTypeToString(QCborValue::Type const type) {
    ZoneScoped;
    return QMetaEnum::fromType<QCborValue::Type>().valueToKey(type);
}

void reportError(QString const &title, Error const &content, bool const messageBox) {
    qCritical() << "Error" << title << ":" << content;

    if (messageBox)
        QMessageBox::critical(qApp->activeWindow(), title, content);

    assert(false && "error occurred");
}