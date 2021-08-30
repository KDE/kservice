/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2007 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <KConfigGroup>
#include <KDesktopFile>
#include <QDebug>
#include <QSignalSpy>
#include <QTest>
#include <kbuildsycoca_p.h>
#include <kservicetype.h>
#include <ksycoca.h>
#include <ksycocadict_p.h>

class KSycocaDictTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);

        // dicttestplugintype: a servicetype
        const QString dictTestPluginType = serviceTypesDir() + QLatin1String{"/dicttestplugintype.desktop"};
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
    QString serviceTypesDir()
    {
        return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String{"/kservicetypes5"};
    }

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
#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 80)
    QSignalSpy spy(KSycoca::self(), qOverload<const QStringList &>(&KSycoca::databaseChanged));
#else
    QSignalSpy spy(KSycoca::self(), &KSycoca::databaseChanged);
#endif

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
    // clang-format off
    serviceTypes << QStringLiteral("DictTestPluginType")
                 << QStringLiteral("KUriFilter/Plugin")
                 << QStringLiteral("KDataTool")
                 << QStringLiteral("KCModule")
                 << QStringLiteral("KScan/KScanDialog")
                 << QStringLiteral("Browser/View")
                 << QStringLiteral("Plasma/Applet")
                 << QStringLiteral("Plasma/Runner");
    // clang-format on

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
            for (const QString &str : std::as_const(serviceTypes)) {
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
        for (const QString &str : std::as_const(serviceTypes)) {
            int offset = loadingDict.find_string(str);
            QVERIFY(offset > 0);
            QCOMPARE(offset, KServiceType::serviceType(str)->offset());
        }
        offset = loadingDict.find_string(QStringLiteral("doesnotexist"));
        // TODO QCOMPARE(offset, 0); // could be non 0 according to the docs, too; if non 0, we should check that the pointed mimetype doesn't have this name.
    }
}

#include "ksycocadicttest.moc"
