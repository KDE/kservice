/*
    SPDX-FileCopyrightText: 2014 Alex Richardson <arichardson.kde@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include <QDebug>
#include <QJsonDocument>
#include <QLocale>
#include <QTest>

#include <KAboutData>
#include <KPluginMetaData>

#include <kplugininfo.h>
#include <kservice.h>

Q_DECLARE_METATYPE(KPluginInfo)

static const QString pluginName = QStringLiteral("fakeplugin"); // clazy:exclude=non-pod-global-static

class KPluginInfoTest : public QObject
{
    Q_OBJECT
private:
    static KPluginInfo withCustomProperty(const KPluginInfo &info)
    {
        KPluginMetaData metaData = info.toMetaData();
        QJsonObject json = metaData.rawData();
        json[QStringLiteral("X-Foo-Bar")] = QStringLiteral("Baz");
        return KPluginInfo(KPluginMetaData(json, info.libraryPath()));
    }

private Q_SLOTS:
    void testLoadDesktop_data()
    {
        QTest::addColumn<QString>("desktopFilePath");
        QTest::addColumn<KPluginInfo>("info");
        QTest::addColumn<KPluginInfo>("infoGerman");
        QTest::addColumn<QVariant>("customValue");
        QTest::addColumn<bool>("translationsWhenLoading");

        QString fakepluginDesktop = QFINDTESTDATA("fakeplugin.desktop");
        QVERIFY2(!fakepluginDesktop.isEmpty(), "Could not find fakeplugin.desktop");
        QString fakePluginJsonPath = QFINDTESTDATA("fakeplugin_json_new.json");
        QVERIFY2(!fakePluginJsonPath.isEmpty(), "Could not find fakeplugin_json_new.json");
        QString fakePluginCompatJsonPath = QFINDTESTDATA("fakeplugin_json_old.json");
        QVERIFY2(!fakePluginCompatJsonPath.isEmpty(), "Could not find fakeplugin_json_old.json");

        QJsonParseError jsonError;
        QFile jsonFile(fakePluginJsonPath);
        QVERIFY(jsonFile.open(QFile::ReadOnly));
        QJsonObject json = QJsonDocument::fromJson(jsonFile.readAll(), &jsonError).object();
        QCOMPARE(jsonError.error, QJsonParseError::NoError);
        QVERIFY(!json.isEmpty());
        KPluginMetaData metaData(json, fakePluginJsonPath);
        QVERIFY(metaData.isValid());
        QVERIFY(!metaData.name().isEmpty());
        QVERIFY(!metaData.authors().isEmpty());
        QVERIFY(!metaData.formFactors().isEmpty());
        QCOMPARE(metaData.formFactors(), QStringList() << QStringLiteral("tablet") << QStringLiteral("handset"));
        KPluginInfo jsonInfo(KPluginMetaData(json, pluginName));

        QFile compatJsonFile(fakePluginCompatJsonPath);
        QVERIFY(compatJsonFile.open(QFile::ReadOnly));
        QJsonObject compatJson = QJsonDocument::fromJson(compatJsonFile.readAll(), &jsonError).object();
        QCOMPARE(jsonError.error, QJsonParseError::NoError);
        QVERIFY(!compatJson.isEmpty());

        // for most constructors translations are performed when the object is constructed and not at runtime!
        QLocale::setDefault(QLocale::c());
        KPluginInfo info(fakepluginDesktop);
#if KSERVICE_ENABLE_DEPRECATED_SINCE(5, 0)
        KService::Ptr fakepluginService(new KService(fakepluginDesktop));
        KPluginInfo infoFromService(fakepluginService);
#endif
        KPluginInfo compatJsonInfo(KPluginMetaData(compatJson, pluginName));
        QLocale::setDefault(QLocale(QLocale::German, QLocale::Germany));
        KPluginInfo infoGerman(fakepluginDesktop);
#if KSERVICE_ENABLE_DEPRECATED_SINCE(5, 0)
        KService::Ptr fakepluginServiceGerman(new KService(fakepluginDesktop));
        KPluginInfo infoFromServiceGerman(fakepluginServiceGerman);
#endif
        KPluginInfo compatJsonInfoGerman(KPluginMetaData(compatJson, pluginName));
        QLocale::setDefault(QLocale::c());

        QTest::ignoreMessage(QtWarningMsg, "\"/this/path/does/not/exist.desktop\" has no desktop group, cannot construct a KPluginInfo object from it.");
        QVERIFY(!KPluginInfo(QStringLiteral("/this/path/does/not/exist.desktop")).isValid());

        QTest::newRow("from .desktop") << fakepluginDesktop << info << infoGerman << QVariant() << false;
        QTest::newRow("with custom property") << info.libraryPath() << withCustomProperty(info) << withCustomProperty(infoGerman)
                                              << QVariant(QStringLiteral("Baz")) << false;
#if KSERVICE_ENABLE_DEPRECATED_SINCE(5, 0)
        QTest::newRow("from KService::Ptr") << fakepluginDesktop << infoFromService << infoFromServiceGerman << QVariant() << true;
        QTest::newRow("from KService::Ptr + custom property")
            << pluginName << withCustomProperty(infoFromService) << withCustomProperty(infoFromServiceGerman) << QVariant(QStringLiteral("Baz")) << true;
#endif
        QTest::newRow("from JSON file") << pluginName << jsonInfo << jsonInfo << QVariant() << false;
        QTest::newRow("from JSON file + custom property")
            << pluginName << withCustomProperty(jsonInfo) << withCustomProperty(jsonInfo) << QVariant(QStringLiteral("Baz")) << false;
        QTest::newRow("from JSON file (compatibility)") << pluginName << compatJsonInfo << compatJsonInfoGerman << QVariant() << true;
        QTest::newRow("from JSON file (compatibility) + custom property")
            << pluginName << withCustomProperty(compatJsonInfo) << withCustomProperty(compatJsonInfoGerman) << QVariant(QStringLiteral("Baz")) << true;
    }

    void testLoadDesktop()
    {
        QFETCH(KPluginInfo, info);
        QFETCH(KPluginInfo, infoGerman);
        QFETCH(QVariant, customValue);
        QFETCH(QString, desktopFilePath);
        QFETCH(bool, translationsWhenLoading);

        QVERIFY(info.isValid());
        QVERIFY(infoGerman.isValid());
        // check the translatable keys first
        if (translationsWhenLoading) {
            // translations are only performed once in the constructor, not when requesting them
            QCOMPARE(info.comment(), QStringLiteral("Test Plugin Spy"));
            QCOMPARE(infoGerman.comment(), QStringLiteral("Test-Spionagemodul"));
            QCOMPARE(info.name(), QStringLiteral("NSA Plugin"));
            QCOMPARE(infoGerman.name(), QStringLiteral("NSA-Modul"));
        } else {
            // translations work correctly here since they are not fixed at load time
            QLocale::setDefault(QLocale::c());
            QCOMPARE(info.name(), QStringLiteral("NSA Plugin"));
            QCOMPARE(infoGerman.name(), QStringLiteral("NSA Plugin"));
            QCOMPARE(info.comment(), QStringLiteral("Test Plugin Spy"));
            QCOMPARE(infoGerman.comment(), QStringLiteral("Test Plugin Spy"));
            QLocale::setDefault(QLocale(QLocale::German, QLocale::Germany));
            QCOMPARE(info.comment(), QStringLiteral("Test-Spionagemodul"));
            QCOMPARE(infoGerman.comment(), QStringLiteral("Test-Spionagemodul"));
            QCOMPARE(info.name(), QStringLiteral("NSA-Modul"));
            QCOMPARE(infoGerman.name(), QStringLiteral("NSA-Modul"));
        }

        QCOMPARE(info.author(), QString::fromUtf8("Sebastian Kügler"));
        QCOMPARE(info.category(), QStringLiteral("Examples"));
#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 79) && KCOREADDONS_BUILD_DEPRECATED_SINCE(5, 79)
        if (!info.dependencies().isEmpty()) {
            qWarning() << "Got dependencies" << info.dependencies();
        }
        QCOMPARE(info.dependencies(), QStringList());
#endif
        QCOMPARE(info.email(), QStringLiteral("sebas@kde.org"));
        QCOMPARE(info.entryPath(), desktopFilePath);
        QCOMPARE(info.icon(), QStringLiteral("preferences-system-time"));
        QCOMPARE(info.isHidden(), false);
        QCOMPARE(info.isPluginEnabled(), false);
        QCOMPARE(info.isPluginEnabledByDefault(), true);
        QCOMPARE(info.libraryPath(), pluginName);
        QCOMPARE(info.license(), QStringLiteral("LGPL"));
        QCOMPARE(info.pluginName(), pluginName);
        // KService/KPluginInfo merges X-KDE-ServiceTypes and MimeTypes
        QCOMPARE(info.serviceTypes(), QStringList() << QStringLiteral("KService/NSA") << QStringLiteral("text/plain") << QStringLiteral("image/png"));
#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 70)
        if (!info.service()) {
#endif
            // KService does not include X-My-Custom-Property since there is no service type installed that defines it
            QCOMPARE(info.property(QStringLiteral("X-My-Custom-Property")), QVariant(QStringLiteral("foo")));
#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 70)
        }
#endif
        // Now check that converting to KPluginMetaData has the separation
        KPluginMetaData asMetaData = info.toMetaData();
        QCOMPARE(asMetaData.serviceTypes(), QStringList() << QStringLiteral("KService/NSA"));
        QCOMPARE(asMetaData.mimeTypes(), QStringList() << QStringLiteral("text/plain") << QStringLiteral("image/png"));

        QCOMPARE(info.version(), QStringLiteral("1.0"));
        QCOMPARE(info.website(), QStringLiteral("http://kde.org/"));

        QCOMPARE(info.property(QStringLiteral("X-Foo-Bar")), customValue);
    }

    void testToMetaData()
    {
        QString fakepluginDesktop = QFINDTESTDATA("fakeplugin.desktop");
        QVERIFY2(!fakepluginDesktop.isEmpty(), "Could not find fakeplugin.desktop");
        KPluginInfo info = withCustomProperty(KPluginInfo(fakepluginDesktop));
        QVERIFY(info.isValid());
        KPluginMetaData meta = info.toMetaData();
        QVERIFY(meta.isValid());

        // check the translatable keys first
        // as of 5.7 translations are performed at runtime and not when the file is read
        QLocale::setDefault(QLocale::c());
        QCOMPARE(meta.description(), QStringLiteral("Test Plugin Spy"));
        QCOMPARE(meta.name(), QStringLiteral("NSA Plugin"));
        QLocale::setDefault(QLocale(QLocale::German));
        QCOMPARE(meta.description(), QStringLiteral("Test-Spionagemodul"));
        QCOMPARE(meta.name(), QStringLiteral("NSA-Modul"));
        QLocale::setDefault(QLocale::c());

        QCOMPARE(meta.authors().size(), 1);
        QCOMPARE(meta.authors().at(0).name(), QString::fromUtf8("Sebastian Kügler"));
        QCOMPARE(meta.authors().at(0).emailAddress(), QStringLiteral("sebas@kde.org"));
        QCOMPARE(meta.category(), QStringLiteral("Examples"));
#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 79) && KCOREADDONS_BUILD_DEPRECATED_SINCE(5, 79)
        QT_WARNING_PUSH
        QT_WARNING_DISABLE_CLANG("-Wdeprecated-declarations")
        QT_WARNING_DISABLE_GCC("-Wdeprecated-declarations")
        QCOMPARE(meta.dependencies(), QStringList());
        QT_WARNING_POP
#endif
        QCOMPARE(meta.fileName(), pluginName);
        QCOMPARE(meta.pluginId(), pluginName);
        QCOMPARE(meta.iconName(), QStringLiteral("preferences-system-time"));
        QCOMPARE(meta.isEnabledByDefault(), true);
        QCOMPARE(meta.license(), QStringLiteral("LGPL"));
        QCOMPARE(meta.serviceTypes(), QStringList() << QStringLiteral("KService/NSA"));
        QCOMPARE(meta.formFactors(), QStringList() << QStringLiteral("tablet") << QStringLiteral("handset"));
        QCOMPARE(meta.version(), QStringLiteral("1.0"));
        QCOMPARE(meta.website(), QStringLiteral("http://kde.org/"));

        // also test the static version
        QCOMPARE(meta, KPluginInfo::toMetaData(info));

        // make sure custom values are also retained
        QCOMPARE(info.property(QStringLiteral("X-Foo-Bar")), QVariant(QStringLiteral("Baz")));
        QCOMPARE(meta.rawData().value(QStringLiteral("X-Foo-Bar")).toString(), QStringLiteral("Baz"));

        KPluginInfo::List srcList = KPluginInfo::List() << info << info;
        QVector<KPluginMetaData> convertedList = KPluginInfo::toMetaData(srcList);
        QCOMPARE(convertedList.size(), 2);
        QCOMPARE(convertedList[0], meta);
        QCOMPARE(convertedList[1], meta);
    }

    void testFromMetaData()
    {
        QJsonParseError e;
        QJsonObject jo = QJsonDocument::fromJson(
                             "{\n"
                             " \"KPlugin\": {\n"
                             " \"Name\": \"NSA Plugin\",\n"
                             " \"Name[de]\": \"NSA-Modul\",\n"
                             " \"Description\": \"Test Plugin Spy\",\n"
                             " \"Description[de]\": \"Test-Spionagemodul\",\n"
                             " \"Icon\": \"preferences-system-time\",\n"
                             " \"Authors\": { \"Name\": \"Sebastian Kügler\", \"Email\": \"sebas@kde.org\" },\n"
                             " \"Category\": \"Examples\",\n"
#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 79) && KCOREADDONS_BUILD_DEPRECATED_SINCE(5, 79)
                             " \"Dependencies\": [],\n"
#endif
                             " \"EnabledByDefault\": \"true\",\n"
                             " \"License\": \"LGPL\",\n"
                             " \"Id\": \"fakeplugin\",\n" // not strictly required
                             " \"Version\": \"1.0\",\n"
                             " \"Website\": \"http://kde.org/\",\n"
                             " \"ServiceTypes\": [\"KService/NSA\"],\n"
                             " \"FormFactors\": [\"tablet\", \"handset\"]\n"
                             " },\n"
                             " \"X-Foo-Bar\": \"Baz\"\n"
                             "}",
                             &e)
                             .object();
        QCOMPARE(e.error, QJsonParseError::NoError);
        KPluginMetaData meta(jo, pluginName);

        QLocale::setDefault(QLocale::c());
        KPluginInfo info = KPluginInfo::fromMetaData(meta);
        QLocale::setDefault(QLocale(QLocale::German, QLocale::Germany));
        KPluginInfo infoGerman = KPluginInfo::fromMetaData(meta);
        QLocale::setDefault(QLocale::c());

        // translations work correctly here since they are not fixed at load time
        QCOMPARE(info.name(), QStringLiteral("NSA Plugin"));
        QCOMPARE(infoGerman.name(), QStringLiteral("NSA Plugin"));
        QCOMPARE(info.comment(), QStringLiteral("Test Plugin Spy"));
        QCOMPARE(infoGerman.comment(), QStringLiteral("Test Plugin Spy"));
        QLocale::setDefault(QLocale(QLocale::German, QLocale::Germany));
        QCOMPARE(info.comment(), QStringLiteral("Test-Spionagemodul"));
        QCOMPARE(infoGerman.comment(), QStringLiteral("Test-Spionagemodul"));
        QCOMPARE(info.name(), QStringLiteral("NSA-Modul"));
        QCOMPARE(infoGerman.name(), QStringLiteral("NSA-Modul"));
        QLocale::setDefault(QLocale::c());

        QCOMPARE(info.author(), QString::fromUtf8("Sebastian Kügler"));
        QCOMPARE(info.category(), QStringLiteral("Examples"));
#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 79) && KCOREADDONS_BUILD_DEPRECATED_SINCE(5, 79)
        QCOMPARE(info.dependencies(), QStringList());
#endif
        QCOMPARE(info.email(), QStringLiteral("sebas@kde.org"));
        QCOMPARE(info.entryPath(), pluginName);
        QCOMPARE(info.icon(), QStringLiteral("preferences-system-time"));
        QCOMPARE(info.isHidden(), false);
        QCOMPARE(info.isPluginEnabled(), false);
        QCOMPARE(info.isPluginEnabledByDefault(), true);
        QCOMPARE(info.libraryPath(), pluginName);
        QCOMPARE(info.license(), QStringLiteral("LGPL"));
        QCOMPARE(info.pluginName(), pluginName);
        QCOMPARE(info.serviceTypes(), QStringList() << QStringLiteral("KService/NSA"));
        QCOMPARE(info.version(), QStringLiteral("1.0"));
        QCOMPARE(info.website(), QStringLiteral("http://kde.org/"));
        QCOMPARE(info.toMetaData().formFactors(), QStringList() << QStringLiteral("tablet") << QStringLiteral("handset"));
        QCOMPARE(info.formFactors(), QStringList() << QStringLiteral("tablet") << QStringLiteral("handset"));

        // make sure custom values are also retained
        QCOMPARE(meta.rawData().value(QStringLiteral("X-Foo-Bar")).toString(), QStringLiteral("Baz"));
        QCOMPARE(info.property(QStringLiteral("X-Foo-Bar")), QVariant(QStringLiteral("Baz")));

        QVector<KPluginMetaData> srcList = QVector<KPluginMetaData>() << meta;
        KPluginInfo::List convertedList = KPluginInfo::fromMetaData(srcList);
        QCOMPARE(convertedList.size(), 1);
        // KPluginInfo::operator== compares identity of the d-pointer -> manual structural comparison
        QCOMPARE(convertedList[0].comment(), info.comment());
        QCOMPARE(convertedList[0].name(), info.name());
        QCOMPARE(convertedList[0].author(), info.author());
        QCOMPARE(convertedList[0].category(), info.category());
#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 79) && KCOREADDONS_BUILD_DEPRECATED_SINCE(5, 79)
        QCOMPARE(convertedList[0].dependencies(), info.dependencies());
#endif
        QCOMPARE(convertedList[0].email(), info.email());
        QCOMPARE(convertedList[0].entryPath(), info.entryPath());
        QCOMPARE(convertedList[0].icon(), info.icon());
        QCOMPARE(convertedList[0].isHidden(), info.isHidden());
        QCOMPARE(convertedList[0].isPluginEnabled(), info.isPluginEnabled());
        QCOMPARE(convertedList[0].isPluginEnabledByDefault(), info.isPluginEnabledByDefault());
        QCOMPARE(convertedList[0].libraryPath(), info.libraryPath());
        QCOMPARE(convertedList[0].license(), info.license());
        QCOMPARE(convertedList[0].pluginName(), info.pluginName());
        QCOMPARE(convertedList[0].serviceTypes(), info.serviceTypes());
        QCOMPARE(convertedList[0].version(), info.version());
        QCOMPARE(convertedList[0].website(), info.website());
        QCOMPARE(convertedList[0].formFactors(), info.formFactors());
    }
};

QTEST_MAIN(KPluginInfoTest)

#include "kplugininfotest.moc"
