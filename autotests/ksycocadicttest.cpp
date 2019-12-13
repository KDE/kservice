/* This file is part of the KDE project
   Copyright (C) 2007 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <ksycoca.h>
#include <QTest>
#include <QSignalSpy>
#include <QDebug>
#include <kservicetype.h>
#include <kdesktopfile.h>
#include <kconfiggroup.h>
#include <ksycocadict_p.h>
#include <kbuildsycoca_p.h>


class KSycocaDictTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);

        // dicttestplugintype: a servicetype
        const QString dictTestPluginType = serviceTypesDir() + "/dicttestplugintype.desktop";
        if (!QFile::exists(dictTestPluginType)) {
            KDesktopFile file(dictTestPluginType);
            KConfigGroup group = file.desktopGroup();
            group.writeEntry("Comment", "Fake Text Plugin");
            group.writeEntry("Type", "ServiceType");
            group.writeEntry("X-KDE-ServiceType", "DictTestPluginType");
            file.group("PropertyDef::X-KDE-Version").writeEntry("Type", "double"); // like in ktexteditorplugin.desktop
            qDebug() << "Just created" << dictTestPluginType;
        }
        runKBuildSycoca();
    }
    void testStandardDict();

private:
    QString serviceTypesDir() { return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/kservicetypes5"; }

    void add(KSycocaDict &dict, const QString &key, const QString &name)
    {
        KServiceType::Ptr ptr = KServiceType::serviceType(name);
        if (!ptr) {
            qWarning() << "serviceType not found" << name;
        }
        dict.add(key, KSycocaEntry::Ptr(ptr));
    }
    static void runKBuildSycoca();
};

QTEST_MAIN(KSycocaDictTest)

void KSycocaDictTest::runKBuildSycoca()
{
    QSignalSpy spy(KSycoca::self(), SIGNAL(databaseChanged(QStringList)));
    KBuildSycoca builder;
    QVERIFY(builder.recreate());
    if (spy.isEmpty()) {
        qDebug() << "waiting for signal";
        QVERIFY(spy.wait(10000));
        qDebug() << "got signal";
    }
}

// Standard use of KSycocaDict: mapping entry name to entry
void KSycocaDictTest::testStandardDict()
{
    QVERIFY(KSycoca::isAvailable());

    QStringList serviceTypes;
    serviceTypes << QStringLiteral("DictTestPluginType")
                 << QStringLiteral("KUriFilter/Plugin")
                 << QStringLiteral("KDataTool")
                 << QStringLiteral("KCModule")
                 << QStringLiteral("KScan/KScanDialog")
                 << QStringLiteral("Browser/View")
                 << QStringLiteral("Plasma/Applet")
                 << QStringLiteral("Plasma/Runner");

    // Skip servicetypes that are not installed
    QMutableListIterator<QString> it(serviceTypes);
    while (it.hasNext()) {
        if (!KServiceType::serviceType(it.next())) {
            it.remove();
        }
    }
    qDebug() << serviceTypes;

    QBENCHMARK {
        QByteArray buffer;
        {
            KSycocaDict dict;
            for (const QString &str : qAsConst(serviceTypes))
            {
                add(dict, str, str);
            }
            dict.remove(QStringLiteral("DictTestPluginType")); // just to test remove
            add(dict, QStringLiteral("DictTestPluginType"), QStringLiteral("DictTestPluginType"));
            QCOMPARE(int(dict.count()), serviceTypes.count());
            QDataStream saveStream(&buffer, QIODevice::WriteOnly);
            dict.save(saveStream);
        }

        QDataStream stream(buffer);
        KSycocaDict loadingDict(&stream, 0);
        int offset = loadingDict.find_string(QStringLiteral("DictTestPluginType"));
        QVERIFY(offset > 0);
        QCOMPARE(offset, KServiceType::serviceType(QStringLiteral("DictTestPluginType"))->offset());
        for (const QString &str : qAsConst(serviceTypes))
        {
            int offset = loadingDict.find_string(str);
            QVERIFY(offset > 0);
            QCOMPARE(offset, KServiceType::serviceType(str)->offset());
        }
        offset = loadingDict.find_string(QStringLiteral("doesnotexist"));
        // TODO QCOMPARE(offset, 0); // could be non 0 according to the docs, too; if non 0, we should check that the pointed mimetype doesn't have this name.
    }
}

#include "ksycocadicttest.moc"
