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
#include "MainWindow.hpp"

int main(int argc, char * argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("SimpleTagger");
    app.setOrganizationName("SimpleTagger");

    QCommandLineParser parser;
    parser.setApplicationDescription("SimpleTagger");
    parser.addHelpOption();
    parser.addVersionOption();

    parser.process(app);

    Settings settings;

    QTranslator translator;
    qDebug() << "Loading translation" << settings.interface.language;
    if (translator.load(QLocale(settings.interface.language), QLatin1String("simpletagger-cxx"), QLatin1String("_"), QLatin1String(":/i18n"))) {
        QCoreApplication::installTranslator(&translator);
    }

    QDirIterator it(":", QDirIterator::Subdirectories);
    while (it.hasNext()) {
        qDebug() << "Embedded file:" << it.next();
    }


    std::unique_ptr<MainWindow> mainWindow;
    if (auto result = MainWindow::create(settings, translator); !result) {
        QMessageBox::critical(
                nullptr,
                QObject::tr("Cannot create main window"),
                QObject::tr("Could not create main window: %1").arg(result.error())
        );
        return -1;
    } else {
        mainWindow = std::move(*result);
    }

    mainWindow->show();

    return app.exec();
}
