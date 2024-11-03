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

// TODO: properly organize tests

#include "../src/TagProcessor.hpp"

class TestTagProcessor: public QObject {
    Q_OBJECT

private slots:
    void testTagProcessor() {
        QFETCH(QString, tagParent);
        QFETCH(QString, tagChild);
        QFETCH(QString, result);
        QCOMPARE(TagProcessor::resolveChildTag(tagParent, tagChild), result);
    }

    void testTagProcessor_data() {
        QTest::addColumn<QString>("tagParent");
        QTest::addColumn<QString>("tagChild");
        QTest::addColumn<QString>("result");

        QString tagHuman = "human";

        QString tagFemale1 = "female(plural:s)";
        QString tagFemale2 = "wom(:a)(plural:e)n";

        QString tagChild1 = "one %";
        QString tagChild2 = "two %(plural)";

        QTest::newRow("female") << tagHuman << tagFemale1 << "female";
        QTest::newRow("woman") << tagHuman << tagFemale2 << "woman";

        QTest::newRow("one female") << tagFemale1 << tagChild1 << "one female";
        QTest::newRow("one woman") << tagFemale2 << tagChild1 << "one woman";

        QTest::newRow("two females") << tagFemale1 << tagChild2 << "two females";
        QTest::newRow("two women") << tagFemale2 << tagChild2 << "two women";
    }
};

QTEST_MAIN(TestTagProcessor)
#include "main.moc"
