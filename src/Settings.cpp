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
#include "Settings.hpp"

namespace Keys {
    namespace Interface {
        static constexpr QAnyStringView LANGUAGE = "settings_interface_language";
        static constexpr QAnyStringView FONT_SIZE = "settings_interface_font_size";
        static constexpr QAnyStringView TAGS_ROW_HEIGHT = "settings_interface_tags_row_height";
        static constexpr QAnyStringView IMAGE_FIXED_ASPECT_RATIOS = "settings_interface_image_fixed_aspect_ratio";
    }
    namespace System {
        static constexpr QAnyStringView BACKUP_ON_ANY_CHANGE = "settings_system_backup_on_any_change";
    }
}

Settings::Settings() {
    load();
}

void Settings::load() {
    ZoneScoped;

    QSettings settings;

    interface.language = settings.value(Keys::Interface::LANGUAGE, interface.language_default()).toString();
    interface.fontSize = settings.value(Keys::Interface::FONT_SIZE, interface.fontSize_default).toFloat();
    interface.tagRowHeight = settings.value(Keys::Interface::TAGS_ROW_HEIGHT, interface.tagRowHeight_default).toInt();
    interface.imageFixedAspectRatios = settings.value(Keys::Interface::IMAGE_FIXED_ASPECT_RATIOS, interface.imageFixedAspectRatios).toBool();

    system.backupOnAnyChange = settings.value(Keys::System::BACKUP_ON_ANY_CHANGE, system.backupOnAnyChange_default).toBool();
}

void Settings::save() {
    ZoneScoped;

    QSettings settings;

    settings.setValue(Keys::Interface::LANGUAGE, interface.language);
    settings.setValue(Keys::Interface::FONT_SIZE, interface.fontSize);
    settings.setValue(Keys::Interface::TAGS_ROW_HEIGHT, interface.tagRowHeight);
    settings.setValue(Keys::Interface::IMAGE_FIXED_ASPECT_RATIOS, interface.imageFixedAspectRatios);

    settings.setValue(Keys::System::BACKUP_ON_ANY_CHANGE, system.backupOnAnyChange);
}

QString Settings::Interface::language_default() {
    return QLocale::system().name();
}

QList<Language> Settings::Interface::availableLanguages() const {
    ZoneScoped;

    QList<Language> result;

    // note: this can't be static, as translate should react on current language
    QHash<QString, QPair<QString, QString>> languageNames = {
            {"de", {"Deutsch", QCoreApplication::translate("language", "German")}},
            {"en", {"English", QCoreApplication::translate("language", "English")}},
            {"pl", {"polski", QCoreApplication::translate("language", "Polish")}}
    };

    QDirIterator it(":/i18n", QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString fileName = it.next();
        auto start = fileName.lastIndexOf("_");
        auto end = fileName.lastIndexOf(".");

        QString code = fileName.mid(start + 1, end - start - 1);
        //qDebug() << "Embedded translation file:" << fileName << language;
        auto name = languageNames.value(code, QPair<QString, QString>(code, code));
        result.append(Language(code, name.first, name.second));
    }

    return result;
}
