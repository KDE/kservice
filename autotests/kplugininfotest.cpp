/*
 *  Copyright 2014 Alex Richardson <arichardson.kde@gmail.com>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License version 2 as published by the Free Software Foundation;
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include <QtTest/QTest>
#include <QLocale>
#include <QJsonDocument>
#include <QFileInfo>

#include <KAboutData>
#include <KPluginMetaData>

#include <kplugininfo.h>
#include <kservice.h>

Q_DECLARE_METATYPE(KPluginInfo)

class KPluginInfoTest : public QObject
{
    Q_OBJECT
private:
    static KPluginInfo withCustomProperty(const KPluginInfo &info)
    {
        QVariantMap result;
        QVariantMap metaData = info.properties();
        metaData["X-Foo-Bar"] = QStringLiteral("Baz");
        result["MetaData"] = metaData;
        return KPluginInfo(QVariantList() << result, info.libraryPath());
    }
private Q_SLOTS:
    void testLoadDesktop_data()
    {
        QTest::addColumn<QString>("desktopFilePath");
        QTest::addColumn<KPluginInfo>("info");
        QTest::addColumn<KPluginInfo>("infoGerman");
        QTest::addColumn<QVariant>("customValue");


        QString fakepluginDesktop = QFINDTESTDATA("fakeplugin.desktop");
        QVERIFY2(!fakepluginDesktop.isEmpty(), "Could not find fakeplugin.desktop");
        // translations are performed when the object is constructed, not later
        QLocale::setDefault(QLocale::c());
        KPluginInfo info(fakepluginDesktop);
        KService::Ptr fakepluginService(new KService(fakepluginDesktop));
        KPluginInfo infoFromService(fakepluginService);
        QLocale::setDefault(QLocale(QLocale::German, QLocale::Germany));
        KPluginInfo infoGerman(fakepluginDesktop);
        KService::Ptr fakepluginServiceGerman(new KService(fakepluginDesktop));
        KPluginInfo infoFromServiceGerman(fakepluginServiceGerman);
        QLocale::setDefault(QLocale::c());

        QTest::newRow("no custom property") << fakepluginDesktop << info << infoGerman << QVariant();
        // when adding the custom property entryPath() cannot be copied -> expect empty string
        QTest::newRow("with custom property") << QString() << withCustomProperty(info)
            << withCustomProperty(infoGerman) << QVariant("Baz");
        QTest::newRow("from KService::Ptr") << fakepluginDesktop << infoFromService << infoFromServiceGerman << QVariant();
    }

    void testLoadDesktop()
    {
        QFETCH(KPluginInfo, info);
        QFETCH(KPluginInfo, infoGerman);
        QFETCH(QVariant, customValue);
        QFETCH(QString, desktopFilePath);

        QVERIFY(info.isValid());
        QVERIFY(infoGerman.isValid());
        QVERIFY(!KPluginInfo("/this/path/does/not/exist.desktop").isValid());

        // check the translatable keys first
        QCOMPARE(info.comment(), QStringLiteral("Test Plugin Spy"));
        QCOMPARE(infoGerman.comment(), QStringLiteral("Test-Spionagemodul"));
        QCOMPARE(info.name(), QStringLiteral("NSA Plugin"));
        QCOMPARE(infoGerman.name(), QStringLiteral("NSA-Modul"));

        QCOMPARE(info.author(), QStringLiteral("Sebastian Kügler"));
        QCOMPARE(info.category(), QStringLiteral("Examples"));
        QCOMPARE(info.dependencies(), QStringList());
        QCOMPARE(info.email(), QStringLiteral("sebas@kde.org"));
        QCOMPARE(info.entryPath(), desktopFilePath);
        QCOMPARE(info.icon(), QStringLiteral("preferences-system-time"));
        QCOMPARE(info.isHidden(), false);
        QCOMPARE(info.isPluginEnabled(), false);
        QCOMPARE(info.isPluginEnabledByDefault(), true);
        QCOMPARE(info.libraryPath(), QStringLiteral("fakeplugin"));
        QCOMPARE(info.license(), QStringLiteral("LGPL"));
        QCOMPARE(info.pluginName(), QStringLiteral("fakeplugin"));
        QCOMPARE(info.serviceTypes(), QStringList());
        QCOMPARE(info.version(), QStringLiteral("1.0"));
        QCOMPARE(info.website(), QStringLiteral("http://kde.org/"));

        QCOMPARE(info.property("X-Foo-Bar"), customValue);

    }

    void testToMetaData()
    {
        QString fakepluginDesktop = QFINDTESTDATA("fakeplugin.desktop");
        QVERIFY2(!fakepluginDesktop.isEmpty(), "Could not find fakeplugin.desktop");
        // translations are performed when the object is constructed, not later
        QLocale::setDefault(QLocale::c());
        KPluginInfo info(fakepluginDesktop);
        QLocale::setDefault(QLocale(QLocale::German, QLocale::Germany));
        KPluginInfo infoGerman(fakepluginDesktop);
        QVERIFY(info.isValid());
        KPluginMetaData meta = info.toMetaData();
        // translations will be broken (since not all are read), it will always return the german version even with QLocale::c()
        KPluginMetaData metaGerman = infoGerman.toMetaData();
        QLocale::setDefault(QLocale::c());

        // check the translatable keys first
        QCOMPARE(meta.description(), QStringLiteral("Test Plugin Spy"));
        QCOMPARE(metaGerman.description(), QStringLiteral("Test-Spionagemodul"));
        QCOMPARE(meta.name(), QStringLiteral("NSA Plugin"));
        QCOMPARE(metaGerman.name(), QStringLiteral("NSA-Modul"));

        QCOMPARE(meta.authors().size(), 1);
        QCOMPARE(meta.authors()[0].name(), QStringLiteral("Sebastian Kügler"));
        QCOMPARE(meta.authors()[0].emailAddress(), QStringLiteral("sebas@kde.org"));
        QCOMPARE(meta.category(), QStringLiteral("Examples"));
        QCOMPARE(meta.dependencies(), QStringList());
        QCOMPARE(meta.fileName(), QFileInfo(QStringLiteral("fakeplugin")).absoluteFilePath());
        QCOMPARE(meta.pluginId(), QStringLiteral("fakeplugin"));
        QCOMPARE(meta.iconName(), QStringLiteral("preferences-system-time"));
        QCOMPARE(meta.isEnabledByDefault(), true);
        QCOMPARE(meta.license(), QStringLiteral("LGPL"));
        QCOMPARE(meta.serviceTypes(), QStringList());
        QCOMPARE(meta.version(), QStringLiteral("1.0"));
        QCOMPARE(meta.website(), QStringLiteral("http://kde.org/"));

        // also test the static version
        QCOMPARE(meta, KPluginInfo::toMetaData(info));
        QCOMPARE(metaGerman, KPluginInfo::toMetaData(infoGerman));
    }

    void testFromMetaData()
    {
        QJsonParseError e;
        QJsonObject jo = QJsonDocument::fromJson("{\n"
            " \"KPlugin\": {\n"
                " \"Name\": \"NSA Plugin\",\n"
                " \"Name[de]\": \"NSA-Modul\",\n"
                " \"Description\": \"Test Plugin Spy\",\n"
                " \"Description[de]\": \"Test-Spionagemodul\",\n"
                " \"Icon\": \"preferences-system-time\",\n"
                " \"Authors\": { \"Name\": \"Sebastian Kügler\", \"Email\": \"sebas@kde.org\" },\n"
                " \"Category\": \"Examples\",\n"
                " \"Dependencies\": [],\n"
                " \"EnabledByDefault\": \"true\",\n"
                " \"License\": \"LGPL\",\n"
                " \"Id\": \"fakeplugin\",\n" // not strictly required
                " \"Version\": \"1.0\",\n"
                " \"Website\": \"http://kde.org/\",\n"
                " \"ServiceTypes\": []\n"
            " }\n}\n", &e).object();
        QCOMPARE(e.error, QJsonParseError::NoError);
        KPluginMetaData meta(jo, "fakeplugin");

        QLocale::setDefault(QLocale::c());
        KPluginInfo info = KPluginInfo::fromMetaData(meta);
        QLocale::setDefault(QLocale(QLocale::German, QLocale::Germany));
        KPluginInfo infoGerman = KPluginInfo::fromMetaData(meta);
        QLocale::setDefault(QLocale::c());

        QCOMPARE(info.comment(), QStringLiteral("Test Plugin Spy"));
        QCOMPARE(infoGerman.comment(), QStringLiteral("Test-Spionagemodul"));
        QCOMPARE(info.name(), QStringLiteral("NSA Plugin"));
        QCOMPARE(infoGerman.name(), QStringLiteral("NSA-Modul"));

        QCOMPARE(info.author(), QStringLiteral("Sebastian Kügler"));
        QCOMPARE(info.category(), QStringLiteral("Examples"));
        QCOMPARE(info.dependencies(), QStringList());
        QCOMPARE(info.email(), QStringLiteral("sebas@kde.org"));
        QCOMPARE(info.entryPath(), QString());
        QCOMPARE(info.icon(), QStringLiteral("preferences-system-time"));
        QCOMPARE(info.isHidden(), false);
        QCOMPARE(info.isPluginEnabled(), false);
        QCOMPARE(info.isPluginEnabledByDefault(), true);
        QCOMPARE(info.libraryPath(), QFileInfo(QStringLiteral("fakeplugin")).absoluteFilePath());
        QCOMPARE(info.license(), QStringLiteral("LGPL"));
        QCOMPARE(info.pluginName(), QStringLiteral("fakeplugin"));
        QCOMPARE(info.serviceTypes(), QStringList());
        QCOMPARE(info.version(), QStringLiteral("1.0"));
        QCOMPARE(info.website(), QStringLiteral("http://kde.org/"));
    }
};

QTEST_MAIN(KPluginInfoTest)

#include "kplugininfotest.moc"
