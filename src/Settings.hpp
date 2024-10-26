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

struct Language {
    QString code;
    QString nativeName;
    QString translatedName;
};

class Settings {
public:
    Settings(Settings const &other) = default;
    Settings(Settings &&other) = default;
    Settings &operator=(Settings const &other) = default;
    Settings &operator=(Settings &&other) = default;

    Settings();
    ~Settings() = default;

    void load();
    void save();

    struct Interface {
        static QString language_default();
        QString language = language_default();
        QList<Language> availableLanguages() const;

        static constexpr float fontSize_default = 7.0;
        float fontSize = fontSize_default;

        static constexpr int tagRowHeight_default = 11;
        int tagRowHeight = tagRowHeight_default;

        static constexpr bool imageFixedAspectRatios_default = true;
        bool imageFixedAspectRatios = imageFixedAspectRatios_default;
    } interface;

    struct System {
        static constexpr bool backupOnAnyChange_default = false;
        bool backupOnAnyChange = backupOnAnyChange_default;
    } system;
};
