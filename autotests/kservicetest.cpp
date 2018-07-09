/*
 *  Copyright (C) 2006 David Faure   <faure@kde.org>
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

#include "kservicetest.h"

#include <locale.h>

#include <qtest.h>

#include <kconfig.h>
#include <kconfiggroup.h>
#include <kdesktopfile.h>
#include <ksycoca.h>
#include <kbuildsycoca_p.h>
#include <../src/services/kserviceutil_p.h>
#include <../src/services/ktraderparsetree_p.h>

#include <kservicegroup.h>
#include <kservicetypetrader.h>
#include <kservicetype.h>
#include <kservicetypeprofile.h>
#include <kpluginmetadata.h>
#include <kplugininfo.h>

#include <qfile.h>
#include <qstandardpaths.h>
#include <qthread.h>
#include <qsignalspy.h>

#include <QTimer>
#include <QDebug>
#include <QLoggingCategory>
#include <QMimeDatabase>

QTEST_MAIN(KServiceTest)

extern KSERVICE_EXPORT int ksycoca_ms_between_checks;

static void eraseProfiles()
{
    QString profilerc = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/profilerc";
    if (!profilerc.isEmpty()) {
        QFile::remove(profilerc);
    }

    profilerc = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/servicetype_profilerc";
    if (!profilerc.isEmpty()) {
        QFile::remove(profilerc);
    }
}


void KServiceTest::initTestCase()
{
    QStandardPaths::enableTestMode(true);

    QLoggingCategory::setFilterRules(QStringLiteral("kf5.kcoreaddons.kdirwatch.debug=true"));

    // A non-C locale is necessary for some tests.
    // This locale must have the following properties:
    //   - some character other than dot as decimal separator
    // If it cannot be set, locale-dependent tests are skipped.
    setlocale(LC_ALL, "fr_FR.utf8");
    m_hasNonCLocale = (setlocale(LC_ALL, nullptr) == QByteArray("fr_FR.utf8"));
    if (!m_hasNonCLocale) {
        qDebug() << "Setting locale to fr_FR.utf8 failed";
    }

    m_hasKde5Konsole = false;
    eraseProfiles();

    if (!KSycoca::isAvailable()) {
        runKBuildSycoca();
    }

    // Create some fake services for the tests below, and ensure they are in ksycoca.

    bool mustUpdateKSycoca = false;

    // fakeservice: deleted and recreated by testDeletingService, don't use in other tests
    const QString fakeServiceDeleteMe = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String("/kservices5/fakeservice_deleteme.desktop");
    if (!QFile::exists(fakeServiceDeleteMe)) {
        mustUpdateKSycoca = true;
        createFakeService(QStringLiteral("fakeservice_deleteme.desktop"), QString());
    }

    // fakeservice: a plugin that implements FakePluginType
    const QString fakeService = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String("/kservices5/fakeservice.desktop");
    if (!QFile::exists(fakeService)) {
        mustUpdateKSycoca = true;
        createFakeService(QStringLiteral("fakeservice.desktop"), QStringLiteral("FakePluginType"));
    }

    // fakepart: a readwrite part, like katepart
    const QString fakePart = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String("/kservices5/") + "fakepart.desktop";
    if (!QFile::exists(fakePart)) {
        mustUpdateKSycoca = true;
        KDesktopFile file(fakePart);
        KConfigGroup group = file.desktopGroup();
        group.writeEntry("Name", "FakePart");
        group.writeEntry("Type", "Service");
        group.writeEntry("X-KDE-Library", "fakepart");
        group.writeEntry("X-KDE-Protocols", "http,ftp");
        group.writeEntry("X-KDE-ServiceTypes", "FakeBasePart,FakeDerivedPart");
        group.writeEntry("MimeType", "text/plain;text/html;");
        group.writeEntry("X-KDE-FormFactors", "tablet,handset");
    }

    const QString fakePart2 = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String("/kservices5/") + "fakepart2.desktop";
    if (!QFile::exists(fakePart2)) {
        mustUpdateKSycoca = true;
        KDesktopFile file(fakePart2);
        KConfigGroup group = file.desktopGroup();
        group.writeEntry("Name", "FakePart2");
        group.writeEntry("Type", "Service");
        group.writeEntry("X-KDE-Library", "fakepart2");
        group.writeEntry("X-KDE-ServiceTypes", "FakeBasePart");
        group.writeEntry("MimeType", "text/plain;");
        group.writeEntry("X-KDE-TestList", QStringList() << QStringLiteral("item1") << QStringLiteral("item2"));
        group.writeEntry("X-KDE-FormFactors", "tablet,handset");
    }

    const QString preferredPart = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String("/kservices5/") + "preferredpart.desktop";
    if (!QFile::exists(preferredPart)) {
        mustUpdateKSycoca = true;
        KDesktopFile file(preferredPart);
        KConfigGroup group = file.desktopGroup();
        group.writeEntry("Name", "PreferredPart");
        group.writeEntry("Type", "Service");
        group.writeEntry("X-KDE-Library", "preferredpart");
        group.writeEntry("X-KDE-ServiceTypes", "FakeBasePart");
        group.writeEntry("MimeType", "text/plain;");
    }

    const QString otherPart = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String("/kservices5/") + "otherpart.desktop";
    if (!QFile::exists(otherPart)) {
        mustUpdateKSycoca = true;
        KDesktopFile file(otherPart);
        KConfigGroup group = file.desktopGroup();
        group.writeEntry("Name", "OtherPart");
        group.writeEntry("Type", "Service");
        group.writeEntry("X-KDE-Library", "otherpart");
        group.writeEntry("X-KDE-ServiceTypes", "FakeBasePart");
        group.writeEntry("MimeType", "text/plain;");
    }

    // faketextplugin: a ktexteditor plugin
    const QString fakeTextplugin = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String("/kservices5/") + "faketextplugin.desktop";
    if (!QFile::exists(fakeTextplugin)) {
        mustUpdateKSycoca = true;
        KDesktopFile file(fakeTextplugin);
        KConfigGroup group = file.desktopGroup();
        group.writeEntry("Name", "FakeTextPlugin");
        group.writeEntry("Type", "Service");
        group.writeEntry("X-KDE-Library", "faketextplugin");
        group.writeEntry("X-KDE-ServiceTypes", "FakePluginType");
        group.writeEntry("MimeType", "text/plain;");
    }

    // fakeplugintype: a servicetype
    const QString fakePluginType = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String("/kservicetypes5/") + "fakeplugintype.desktop";
    if (!QFile::exists(fakePluginType)) {
        mustUpdateKSycoca = true;
        KDesktopFile file(fakePluginType);
        KConfigGroup group = file.desktopGroup();
        group.writeEntry("Comment", "Fake Text Plugin");
        group.writeEntry("Type", "ServiceType");
        group.writeEntry("X-KDE-ServiceType", "FakePluginType");
        file.group("PropertyDef::X-KDE-Version").writeEntry("Type", "double"); // like in ktexteditorplugin.desktop
        group.writeEntry("X-KDE-FormFactors", "tablet,handset");
    }

    // fakebasepart: a servicetype (like ReadOnlyPart)
    const QString fakeBasePart = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String("/kservicetypes5/") + "fakebasepart.desktop";
    if (!QFile::exists(fakeBasePart)) {
        mustUpdateKSycoca = true;
        KDesktopFile file(fakeBasePart);
        KConfigGroup group = file.desktopGroup();
        group.writeEntry("Comment", "Fake Base Part");
        group.writeEntry("Type", "ServiceType");
        group.writeEntry("X-KDE-ServiceType", "FakeBasePart");

        KConfigGroup listGroup(&file, "PropertyDef::X-KDE-TestList");
        listGroup.writeEntry("Type", "QStringList");
    }

    // fakederivedpart: a servicetype deriving from FakeBasePart (like ReadWritePart derives from ReadOnlyPart)
    const QString fakeDerivedPart = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String("/kservicetypes5/") + "fakederivedpart.desktop";
    if (!QFile::exists(fakeDerivedPart)) {
        mustUpdateKSycoca = true;
        KDesktopFile file(fakeDerivedPart);
        KConfigGroup group = file.desktopGroup();
        group.writeEntry("Comment", "Fake Derived Part");
        group.writeEntry("Type", "ServiceType");
        group.writeEntry("X-KDE-ServiceType", "FakeDerivedPart");
        group.writeEntry("X-KDE-Derived", "FakeBasePart");
    }

    // fakekdedmodule
    const QString fakeKdedModule = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String("/kservicetypes5/") + "fakekdedmodule.desktop";
    if (!QFile::exists(fakeKdedModule)) {
        const QString src = QFINDTESTDATA("fakekdedmodule.desktop");
        QVERIFY(QFile::copy(src, fakeKdedModule));
        mustUpdateKSycoca = true;
    }

    if (mustUpdateKSycoca) {
        // Update ksycoca in ~/.qttest after creating the above
        runKBuildSycoca(true);
    }
    QVERIFY(KServiceType::serviceType(QStringLiteral("FakePluginType")));
    QVERIFY(KServiceType::serviceType(QStringLiteral("FakeBasePart")));
    QVERIFY(KServiceType::serviceType(QStringLiteral("FakeDerivedPart")));
}

void KServiceTest::runKBuildSycoca(bool noincremental)
{
    QSignalSpy spy(KSycoca::self(), SIGNAL(databaseChanged(QStringList)));
    KBuildSycoca builder;
    QVERIFY(builder.recreate(!noincremental));
    if (spy.isEmpty()) {
        qDebug() << "waiting for signal";
        QVERIFY(spy.wait(10000));
        qDebug() << "got signal";
    }
}

void KServiceTest::cleanupTestCase()
{
    // If I want the konqueror unit tests to work, then I better not have a non-working part
    // as the preferred part for text/plain...
    QStringList services; services << QStringLiteral("fakeservice.desktop") << QStringLiteral("fakepart.desktop") << QStringLiteral("faketextplugin.desktop") << QStringLiteral("fakeservice_querymustrebuild.desktop");
    Q_FOREACH (const QString &service, services) {
        const QString fakeService = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String("/kservices5/") + service;
        QFile::remove(fakeService);
    }
    QStringList serviceTypes; serviceTypes << QStringLiteral("fakeplugintype.desktop");
    Q_FOREACH (const QString &serviceType, serviceTypes) {
        const QString fakeServiceType = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String("/kservicetypes5/") + serviceType;
        //QFile::remove(fakeServiceType);
    }
    KBuildSycoca builder;
    builder.recreate();
}

void KServiceTest::testByName()
{
    if (!KSycoca::isAvailable()) {
        QSKIP("ksycoca not available");
    }

    KServiceType::Ptr s0 = KServiceType::serviceType(QStringLiteral("FakeBasePart"));
    QVERIFY2(s0, "KServiceType::serviceType(\"FakeBasePart\") failed!");
    QCOMPARE(s0->name(), QStringLiteral("FakeBasePart"));

    KService::Ptr myService = KService::serviceByDesktopPath(QStringLiteral("fakepart.desktop"));
    QVERIFY(myService);
    QCOMPARE(myService->name(), QStringLiteral("FakePart"));
}

void KServiceTest::testConstructorFullPath()
{
    // Requirement: text/html must be a known mimetype
    QVERIFY(QMimeDatabase().mimeTypeForName("text/html").isValid());
    const QString fakePart = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String("/kservices5/") + "fakepart.desktop";
    QVERIFY(QFile::exists(fakePart));
    KService service(fakePart);
    QVERIFY(service.isValid());
    QCOMPARE(service.mimeTypes(), QStringList() << "text/plain" << "text/html");
}

void KServiceTest::testConstructorKDesktopFileFullPath()
{
    const QString fakePart = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String("/kservices5/") + "fakepart.desktop";
    QVERIFY(QFile::exists(fakePart));
    KDesktopFile desktopFile(fakePart);
    KService service(&desktopFile);
    QVERIFY(service.isValid());
    QCOMPARE(service.mimeTypes(), QStringList() << "text/plain" << "text/html");
}

void KServiceTest::testConstructorKDesktopFile() // as happens inside kbuildsycoca.cpp
{
    KDesktopFile desktopFile(QStandardPaths::GenericDataLocation, "kservices5/fakepart.desktop");
    QCOMPARE(KService(&desktopFile, "kservices5/fakepart.desktop").mimeTypes(), QStringList() << "text/plain" << "text/html");
}

void KServiceTest::testProperty()
{
    ksycoca_ms_between_checks = 0;

    // Let's try creating a desktop file and ensuring it's noticed by the timestamp check
    QTest::qWait(1000);
    const QString fakeCookie = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String("/kservices5/kded/") + "fakekcookiejar.desktop";
    if (!QFile::exists(fakeCookie)) {
        KDesktopFile file(fakeCookie);
        KConfigGroup group = file.desktopGroup();
        group.writeEntry("Name", "OtherPart");
        group.writeEntry("Type", "Service");
        group.writeEntry("X-KDE-ServiceTypes", "FakeKDEDModule");
        group.writeEntry("X-KDE-Library", "kcookiejar");
        group.writeEntry("X-KDE-Kded-autoload", "false");
        group.writeEntry("X-KDE-Kded-load-on-demand", "true");
        qDebug() << "created" << fakeCookie;
    }

    KService::Ptr kdedkcookiejar = KService::serviceByDesktopPath(QStringLiteral("kded/fakekcookiejar.desktop"));
    QVERIFY(kdedkcookiejar);
    QCOMPARE(kdedkcookiejar->entryPath(), QStringLiteral("kded/fakekcookiejar.desktop"));

    QCOMPARE(kdedkcookiejar->property(QStringLiteral("ServiceTypes")).toStringList().join(QLatin1Char(',')), QString("FakeKDEDModule"));
    QCOMPARE(kdedkcookiejar->property(QStringLiteral("X-KDE-Kded-autoload")).toBool(), false);
    QCOMPARE(kdedkcookiejar->property(QStringLiteral("X-KDE-Kded-load-on-demand")).toBool(), true);
    QVERIFY(!kdedkcookiejar->property(QStringLiteral("Name")).toString().isEmpty());
    QVERIFY(!kdedkcookiejar->property(QStringLiteral("Name[fr]"), QVariant::String).isValid());

    // TODO: for this we must install a servicetype desktop file...
    //KService::Ptr kjavaappletviewer = KService::serviceByDesktopPath("kjavaappletviewer.desktop");
    //QVERIFY(kjavaappletviewer);
    //QCOMPARE(kjavaappletviewer->property("X-KDE-BrowserView-PluginsInfo").toString(), QString("kjava/pluginsinfo"));

    // Test property("X-KDE-Protocols"), which triggers the KServiceReadProperty code.
    KService::Ptr fakePart = KService::serviceByDesktopPath(QStringLiteral("fakepart.desktop"));
    QVERIFY(fakePart); // see initTestCase; it should be found.
    QVERIFY(fakePart->propertyNames().contains(QStringLiteral("X-KDE-Protocols")));
    QCOMPARE(fakePart->mimeTypes(), QStringList() << QStringLiteral("text/plain") << QStringLiteral("text/html")); // okular relies on subclasses being kept here
    const QStringList protocols = fakePart->property(QStringLiteral("X-KDE-Protocols")).toStringList();
    QCOMPARE(protocols, QStringList() << QStringLiteral("http") << QStringLiteral("ftp"));

    // Restore value
    ksycoca_ms_between_checks = 1500;
}

void KServiceTest::testAllServiceTypes()
{
    if (!KSycoca::isAvailable()) {
        QSKIP("ksycoca not available");
    }

    const KServiceType::List allServiceTypes = KServiceType::allServiceTypes();

    // A bit of checking on the allServiceTypes list itself
    KServiceType::List::ConstIterator stit = allServiceTypes.begin();
    const KServiceType::List::ConstIterator stend = allServiceTypes.end();
    for (; stit != stend; ++stit) {
        const KServiceType::Ptr servtype = (*stit);
        const QString name = servtype->name();
        QVERIFY(!name.isEmpty());
        QVERIFY(servtype->sycocaType() == KST_KServiceType);
    }
}

void KServiceTest::testAllServices()
{
    if (!KSycoca::isAvailable()) {
        QSKIP("ksycoca not available");
    }
    const KService::List lst = KService::allServices();
    QVERIFY(!lst.isEmpty());

    for (KService::List::ConstIterator it = lst.begin();
            it != lst.end(); ++it) {
        const KService::Ptr service = (*it);
        QVERIFY(service->isType(KST_KService));

        const QString name = service->name();
        const QString entryPath = service->entryPath();
        //qDebug() << name << "entryPath=" << entryPath << "menuId=" << service->menuId();
        QVERIFY(!name.isEmpty());
        QVERIFY(!entryPath.isEmpty());

        KService::Ptr lookedupService = KService::serviceByDesktopPath(entryPath);
        QVERIFY(lookedupService);   // not null
        QCOMPARE(lookedupService->entryPath(), entryPath);

        if (service->isApplication()) {
            const QString menuId = service->menuId();
            if (menuId.isEmpty()) {
                qWarning("%s has an empty menuId!", qPrintable(entryPath));
            } else if (menuId == "org.kde.konsole.desktop") {
                m_hasKde5Konsole = true;
            }
            QVERIFY(!menuId.isEmpty());
            lookedupService = KService::serviceByMenuId(menuId);
            QVERIFY(lookedupService);   // not null
            QCOMPARE(lookedupService->menuId(), menuId);
        }
    }
}

// Helper method for all the trader tests
static bool offerListHasService(const KService::List &offers,
                                const QString &entryPath)
{
    bool found = false;
    KService::List::const_iterator it = offers.begin();
    for (; it != offers.end(); ++it) {
        if ((*it)->entryPath() == entryPath) {
            if (found) {  // should be there only once
                qWarning("ERROR: %s was found twice in the list", qPrintable(entryPath));
                return false; // make test fail
            }
            found = true;
        }
    }
    return found;
}

void KServiceTest::testDBUSStartupType()
{
    if (!KSycoca::isAvailable()) {
        QSKIP("ksycoca not available");
    }
    if (!m_hasKde5Konsole) {
        QSKIP("org.kde.konsole.desktop not available");
    }
    //KService::Ptr konsole = KService::serviceByMenuId( "org.kde.konsole.desktop" );
    KService::Ptr konsole = KService::serviceByDesktopName(QStringLiteral("org.kde.konsole"));
    QVERIFY(konsole);
    QCOMPARE(konsole->menuId(), QStringLiteral("org.kde.konsole.desktop"));
    //qDebug() << konsole->entryPath();
    QCOMPARE(int(konsole->dbusStartupType()), int(KService::DBusUnique));
}

void KServiceTest::testByStorageId()
{
    if (!KSycoca::isAvailable()) {
        QSKIP("ksycoca not available");
    }
    if (QStandardPaths::locate(QStandardPaths::ApplicationsLocation, QStringLiteral("org.kde.konsole.desktop")).isEmpty()) {
        QSKIP("org.kde.konsole.desktop not available");
    }
    QVERIFY(KService::serviceByMenuId(QStringLiteral("org.kde.konsole.desktop")));
    QVERIFY(!KService::serviceByMenuId(QStringLiteral("org.kde.konsole"))); // doesn't work, extension mandatory
    QVERIFY(!KService::serviceByMenuId(QStringLiteral("konsole.desktop"))); // doesn't work, full filename mandatory
    QVERIFY(KService::serviceByStorageId(QStringLiteral("org.kde.konsole.desktop")));
    QVERIFY(KService::serviceByStorageId("org.kde.konsole"));

    // This one fails here; probably because there are two such files, so this would be too
    // ambiguous... According to the testAllServices output, the entryPaths are
    // entryPath="/d/kde/inst/kde5/share/applications/org.kde.konsole.desktop"
    // entryPath= "/usr/share/applications/org.kde.konsole.desktop"
    //
    //QVERIFY(KService::serviceByDesktopPath("org.kde.konsole.desktop"));

    QVERIFY(KService::serviceByDesktopName(QStringLiteral("org.kde.konsole")));
    QCOMPARE(KService::serviceByDesktopName(QStringLiteral("org.kde.konsole"))->menuId(), QString("org.kde.konsole.desktop"));
}

void KServiceTest::testServiceTypeTraderForReadOnlyPart()
{
    if (!KSycoca::isAvailable()) {
        QSKIP("ksycoca not available");
    }

    // Querying trader for services associated with FakeBasePart
    KService::List offers = KServiceTypeTrader::self()->query(QStringLiteral("FakeBasePart"));
    QVERIFY(offers.count() > 0);

    if (!offerListHasService(offers, QStringLiteral("fakepart.desktop"))
        || !offerListHasService(offers, QStringLiteral("fakepart2.desktop"))
        || !offerListHasService(offers, QStringLiteral("otherpart.desktop"))
        || !offerListHasService(offers, QStringLiteral("preferredpart.desktop"))) {
        foreach (KService::Ptr service, offers) {
            qDebug("%s %s", qPrintable(service->name()), qPrintable(service->entryPath()));
        }
    }

    m_firstOffer = offers[0]->entryPath();

    QVERIFY(offerListHasService(offers, QStringLiteral("fakepart.desktop")));
    QVERIFY(offerListHasService(offers, QStringLiteral("fakepart2.desktop")));
    QVERIFY(offerListHasService(offers, QStringLiteral("otherpart.desktop")));
    QVERIFY(offerListHasService(offers, QStringLiteral("preferredpart.desktop")));

    // Check ordering according to InitialPreference
    int lastPreference = -1;
    bool lastAllowedAsDefault = true;
    Q_FOREACH (KService::Ptr service, offers) {
        const QString path = service->entryPath();
        const int preference = service->initialPreference(); // ## might be wrong if we use per-servicetype preferences...
        //qDebug( "%s has preference %d, allowAsDefault=%d", qPrintable( path ), preference, service->allowAsDefault() );
        if (lastAllowedAsDefault && !service->allowAsDefault()) {
            // first "not allowed as default" offer
            lastAllowedAsDefault = false;
            lastPreference = -1; // restart
        }
        if (lastPreference != -1) {
            QVERIFY(preference <= lastPreference);
        }
        lastPreference = preference;
    }

    // Now look for any FakePluginType
    offers = KServiceTypeTrader::self()->query(QStringLiteral("FakePluginType"));
    QVERIFY(offerListHasService(offers, QStringLiteral("fakeservice.desktop")));
    QVERIFY(offerListHasService(offers, QStringLiteral("faketextplugin.desktop")));
}

void KServiceTest::testTraderConstraints()
{
    if (!KSycoca::isAvailable()) {
        QSKIP("ksycoca not available");
    }

    KService::List offers;

    // Baseline: no constraints
    offers = KServiceTypeTrader::self()->query(QStringLiteral("FakePluginType"));
    QCOMPARE(offers.count(), 2);
    QVERIFY(offerListHasService(offers, QStringLiteral("faketextplugin.desktop")));
    QVERIFY(offerListHasService(offers, QStringLiteral("fakeservice.desktop")));

    // String-based constraint
    offers = KServiceTypeTrader::self()->query(QStringLiteral("FakePluginType"), QStringLiteral("Library == 'faketextplugin'"));
    QCOMPARE(offers.count(), 1);
    QVERIFY(offerListHasService(offers, QStringLiteral("faketextplugin.desktop")));

    // Match case insensitive
    offers = KServiceTypeTrader::self()->query(QStringLiteral("FakePluginType"), QStringLiteral("Library =~ 'fAkEteXtpLuGin'"));
    QCOMPARE(offers.count(), 1);
    QVERIFY(offerListHasService(offers, QStringLiteral("faketextplugin.desktop")));

    // "contains"
    offers = KServiceTypeTrader::self()->query(QStringLiteral("FakePluginType"), QStringLiteral("'textplugin' ~ Library")); // note: "is contained in", not "contains"...
    QCOMPARE(offers.count(), 1);
    QVERIFY(offerListHasService(offers, QStringLiteral("faketextplugin.desktop")));

    // "contains" case insensitive
    offers = KServiceTypeTrader::self()->query(QStringLiteral("FakePluginType"), QStringLiteral("'teXtPluGin' ~~ Library")); // note: "is contained in", not "contains"...
    QCOMPARE(offers.count(), 1);
    QVERIFY(offerListHasService(offers, QStringLiteral("faketextplugin.desktop")));

    // sub-sequence case sensitive
    offers = KServiceTypeTrader::self()->query(QStringLiteral("FakePluginType"), QStringLiteral("'txtlug' subseq Library"));
    QCOMPARE(offers.count(), 1);
    QVERIFY(offerListHasService(offers, QStringLiteral("faketextplugin.desktop")));

    // sub-sequence case insensitive
    offers = KServiceTypeTrader::self()->query(QStringLiteral("FakePluginType"), QStringLiteral("'tXtLuG' ~subseq Library"));
    QCOMPARE(offers.count(), 1);
    QVERIFY(offerListHasService(offers, QStringLiteral("faketextplugin.desktop")));

    if (m_hasNonCLocale) {

        // Test float parsing, must use dot as decimal separator independent of locale.
        offers = KServiceTypeTrader::self()->query(QStringLiteral("FakePluginType"), QStringLiteral("([X-KDE-Version] > 4.559) and ([X-KDE-Version] < 4.561)"));
        QCOMPARE(offers.count(), 1);
        QVERIFY(offerListHasService(offers, QStringLiteral("fakeservice.desktop")));
    }

    // A test with an invalid query, to test for memleaks
    offers = KServiceTypeTrader::self()->query(QStringLiteral("FakePluginType"), QStringLiteral("A == B OR C == D AND OR Foo == 'Parse Error'"));
    QVERIFY(offers.isEmpty());
}

void KServiceTest::testSubseqConstraints()
{
  auto test = [](const char* pattern, const char* text, bool sensitive) {
    return KTraderParse::ParseTreeSubsequenceMATCH::isSubseq(
        QString(pattern),
        QString(text),
        sensitive? Qt::CaseSensitive : Qt::CaseInsensitive
    );
  };

  // Case Sensitive
  QVERIFY2(!test("", "", 1), "both empty");
  QVERIFY2(!test("", "something", 1), "empty pattern");
  QVERIFY2(!test("something", "", 1), "empty text");
  QVERIFY2(test("lngfile", "somereallylongfile", 1), "match ending");
  QVERIFY2(test("somelong", "somereallylongfile", 1), "match beginning");
  QVERIFY2(test("reallylong", "somereallylongfile", 1), "match middle");
  QVERIFY2(test("across", "a 23 c @#! r o01 o 5 s_s", 1), "match across");
  QVERIFY2(!test("nocigar", "soclosebutnociga", 1), "close but no match");
  QVERIFY2(!test("god", "dog", 1), "incorrect letter order");
  QVERIFY2(!test("mismatch", "mIsMaTcH", 1), "case sensitive mismatch");

  // Case insensitive
  QVERIFY2(test("mismatch", "mIsMaTcH", 0), "case insensitive match");
  QVERIFY2(test("tryhards", "Try Your Hardest", 0), "uppercase text");
  QVERIFY2(test("TRYHARDS", "try your hardest", 0), "uppercase pattern");
}

void KServiceTest::testHasServiceType1() // with services constructed with a full path (rare)
{
    QString fakepartPath = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QLatin1String("kservices5/") + "fakepart.desktop");
    QVERIFY(!fakepartPath.isEmpty());
    KService fakepart(fakepartPath);
    QVERIFY(fakepart.hasServiceType(QStringLiteral("FakeBasePart")));
    QVERIFY(fakepart.hasServiceType(QStringLiteral("FakeDerivedPart")));
    QCOMPARE(fakepart.mimeTypes(), QStringList() << QStringLiteral("text/plain") << QStringLiteral("text/html"));

    QString faketextPluginPath = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QLatin1String("kservices5/") + "faketextplugin.desktop");
    QVERIFY(!faketextPluginPath.isEmpty());
    KService faketextPlugin(faketextPluginPath);
    QVERIFY(faketextPlugin.hasServiceType(QStringLiteral("FakePluginType")));
    QVERIFY(!faketextPlugin.hasServiceType(QStringLiteral("FakeBasePart")));
}

void KServiceTest::testHasServiceType2() // with services coming from ksycoca
{
    KService::Ptr fakepart = KService::serviceByDesktopPath(QStringLiteral("fakepart.desktop"));
    QVERIFY(fakepart);
    QVERIFY(fakepart->hasServiceType(QStringLiteral("FakeBasePart")));
    QVERIFY(fakepart->hasServiceType(QStringLiteral("FakeDerivedPart")));
    QCOMPARE(fakepart->mimeTypes(), QStringList() << QStringLiteral("text/plain") << QStringLiteral("text/html"));

    KService::Ptr faketextPlugin = KService::serviceByDesktopPath(QStringLiteral("faketextplugin.desktop"));
    QVERIFY(faketextPlugin);
    QVERIFY(faketextPlugin->hasServiceType(QStringLiteral("FakePluginType")));
    QVERIFY(!faketextPlugin->hasServiceType(QStringLiteral("FakeBasePart")));
}

void KServiceTest::testWriteServiceTypeProfile()
{
    const QString serviceType = QStringLiteral("FakeBasePart");
    KService::List services, disabledServices;
    services.append(KService::serviceByDesktopPath(QStringLiteral("preferredpart.desktop")));
    services.append(KService::serviceByDesktopPath(QStringLiteral("fakepart.desktop")));
    disabledServices.append(KService::serviceByDesktopPath(QStringLiteral("fakepart2.desktop")));

    Q_FOREACH (const KService::Ptr &serv, services) {
        QVERIFY(serv);
    }
    Q_FOREACH (const KService::Ptr &serv, disabledServices) {
        QVERIFY(serv);
    }

    KServiceTypeProfile::writeServiceTypeProfile(serviceType, services, disabledServices);

    // Check that the file got written
    QString profilerc = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/servicetype_profilerc";
    QVERIFY(!profilerc.isEmpty());
    QVERIFY(QFile::exists(profilerc));

    KService::List offers = KServiceTypeTrader::self()->query(serviceType);
    QVERIFY(offers.count() > 0);   // not empty

    //foreach( KService::Ptr service, offers )
    //    qDebug( "%s %s", qPrintable( service->name() ), qPrintable( service->entryPath() ) );

    QVERIFY(offers.count() >= 2);
    QCOMPARE(offers[0]->entryPath(), QStringLiteral("preferredpart.desktop"));
    QCOMPARE(offers[1]->entryPath(), QStringLiteral("fakepart.desktop"));
    QVERIFY(offerListHasService(offers, QStringLiteral("otherpart.desktop")));     // should still be somewhere in there
    QVERIFY(!offerListHasService(offers, QStringLiteral("fakepart2.desktop")));     // it got disabled above
}

void KServiceTest::testDefaultOffers()
{
    // Now that we have a user-profile, let's see if defaultOffers indeed gives us the default ordering.
    const QString serviceType = QStringLiteral("FakeBasePart");
    KService::List offers = KServiceTypeTrader::self()->defaultOffers(serviceType);
    QVERIFY(offers.count() > 0);   // not empty
    QVERIFY(offerListHasService(offers, QStringLiteral("fakepart2.desktop")));     // it's here even though it's disabled in the profile
    QVERIFY(offerListHasService(offers, QStringLiteral("otherpart.desktop")));
    if (m_firstOffer.isEmpty()) {
        QSKIP("testServiceTypeTraderForReadOnlyPart not run");
    }
    QCOMPARE(offers[0]->entryPath(), m_firstOffer);
}

void KServiceTest::testDeleteServiceTypeProfile()
{
    const QString serviceType = QStringLiteral("FakeBasePart");
    KServiceTypeProfile::deleteServiceTypeProfile(serviceType);

    KService::List offers = KServiceTypeTrader::self()->query(serviceType);
    QVERIFY(offers.count() > 0);   // not empty
    QVERIFY(offerListHasService(offers, QStringLiteral("fakepart2.desktop")));     // it's back

    if (m_firstOffer.isEmpty()) {
        QSKIP("testServiceTypeTraderForReadOnlyPart not run");
    }
    QCOMPARE(offers[0]->entryPath(), m_firstOffer);
}

void KServiceTest::testActionsAndDataStream()
{
    if (QStandardPaths::locate(QStandardPaths::ApplicationsLocation, QStringLiteral("org.kde.konsole.desktop")).isEmpty()) {
        QSKIP("org.kde.konsole.desktop not available");
    }
    KService::Ptr service = KService::serviceByStorageId(QStringLiteral("org.kde.konsole.desktop"));
    QVERIFY(service);
    QVERIFY(!service->property(QStringLiteral("Name[fr]"), QVariant::String).isValid());
    const QList<KServiceAction> actions = service->actions();
    QCOMPARE(actions.count(), 2); // NewWindow, NewTab
    const KServiceAction newTabAction = actions.at(1);
    QCOMPARE(newTabAction.name(), QStringLiteral("NewTab"));
    QCOMPARE(newTabAction.exec(), QStringLiteral("konsole --new-tab"));
    QVERIFY(newTabAction.icon().isEmpty());
    QCOMPARE(newTabAction.noDisplay(), false);
    QVERIFY(!newTabAction.isSeparator());
}

void KServiceTest::testServiceGroups()
{
    KServiceGroup::Ptr root = KServiceGroup::root();
    QVERIFY(root);
    qDebug() << root->groupEntries().count();

    KServiceGroup::Ptr group = root;
    QVERIFY(group);
    const KServiceGroup::List list = group->entries(true /* sorted */,
                                     true /* exclude no display entries */,
                                     false /* allow separators */,
                                     true /* sort by generic name */);

    qDebug() << list.count();
    Q_FOREACH (KServiceGroup::SPtr s, list) {
        qDebug() << s->name() << s->entryPath();
    }

    // No unit test here yet, but at least this can be valgrinded for errors.
}

void KServiceTest::testDeletingService()
{
    // workaround unexplained inotify issue (in CI only...)
    QTest::qWait(1000);

    const QString serviceName = QStringLiteral("fakeservice_deleteme.desktop");
    KService::Ptr fakeService = KService::serviceByDesktopPath(serviceName);
    QVERIFY(fakeService); // see initTestCase; it should be found.

    // Test deleting a service
    const QString servPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String("/kservices5/") + serviceName;
    QVERIFY(QFile::exists(servPath));
    QFile::remove(servPath);
    runKBuildSycoca();
    ksycoca_ms_between_checks = 0; // need it to check the ksycoca mtime
    QVERIFY(!KService::serviceByDesktopPath(serviceName)); // not in ksycoca anymore

    // Restore value
    ksycoca_ms_between_checks = 1500;

    QVERIFY(fakeService); // the whole point of refcounting is that this KService instance is still valid.
    QVERIFY(!QFile::exists(servPath));

    // Recreate it, for future tests
    createFakeService(serviceName, QString());
    QVERIFY(QFile::exists(servPath));
    qDebug() << "executing kbuildsycoca (2)";

    runKBuildSycoca();

    if (QThread::currentThread() != QCoreApplication::instance()->thread()) {
        m_sycocaUpdateDone.ref();
    }
}

void KServiceTest::createFakeService(const QString &filename, const QString& serviceType)
{
    const QString fakeService = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String("/kservices5/") + filename;
    KDesktopFile file(fakeService);
    KConfigGroup group = file.desktopGroup();
    group.writeEntry("Name", "FakePlugin");
    group.writeEntry("Type", "Service");
    group.writeEntry("X-KDE-Library", "fakeservice");
    group.writeEntry("X-KDE-Version", "4.56");
    group.writeEntry("ServiceTypes", serviceType);
    group.writeEntry("MimeType", "text/plain;");
}

#include <QThreadPool>
#include <QFutureSynchronizer>
#include <qtconcurrentrun.h>

// Testing for concurrent access to ksycoca from multiple threads
// It's especially interesting to run this test as ./kservicetest testThreads
// so that even the ksycoca initialization is happening from N threads at the same time.
// Use valgrind --tool=helgrind to see the race conditions.

void KServiceTest::testReaderThreads()
{
    QThreadPool::globalInstance()->setMaxThreadCount(10);
    QFutureSynchronizer<void> sync;
    sync.addFuture(QtConcurrent::run(this, &KServiceTest::testAllServices));
    sync.addFuture(QtConcurrent::run(this, &KServiceTest::testAllServices));
    sync.addFuture(QtConcurrent::run(this, &KServiceTest::testAllServices));
    sync.addFuture(QtConcurrent::run(this, &KServiceTest::testHasServiceType1));
    sync.addFuture(QtConcurrent::run(this, &KServiceTest::testAllServices));
    sync.addFuture(QtConcurrent::run(this, &KServiceTest::testAllServices));
    sync.waitForFinished();
    QThreadPool::globalInstance()->setMaxThreadCount(1); // delete those threads
}

void KServiceTest::testThreads()
{
    QThreadPool::globalInstance()->setMaxThreadCount(10);
    QFutureSynchronizer<void> sync;
    sync.addFuture(QtConcurrent::run(this, &KServiceTest::testAllServices));
    sync.addFuture(QtConcurrent::run(this, &KServiceTest::testHasServiceType1));
    sync.addFuture(QtConcurrent::run(this, &KServiceTest::testDeletingService));
    sync.addFuture(QtConcurrent::run(this, &KServiceTest::testTraderConstraints));
    // process events (DBus, inotify...), until we get all expected signals
    QTRY_COMPARE_WITH_TIMEOUT(m_sycocaUpdateDone.load(), 1, 15000); // not using a bool, just to silence helgrind
    qDebug() << "Joining all threads";
    sync.waitForFinished();
}

void KServiceTest::testOperatorKPluginName()
{
    KService fservice(QFINDTESTDATA("fakeplugin.desktop"));
    KPluginName fname(fservice);
    QVERIFY(fname.isValid());
    QCOMPARE(fname.name(), QStringLiteral("fakeplugin"));
    KPluginLoader fplugin(fservice);
    QVERIFY(fplugin.factory());

    // make sure constness doesn't break anything
    const KService const_fservice(QFINDTESTDATA("fakeplugin.desktop"));
    KPluginName const_fname(const_fservice);
    QVERIFY(const_fname.isValid());
    QCOMPARE(const_fname.name(), QStringLiteral("fakeplugin"));
    KPluginLoader const_fplugin(const_fservice);
    QVERIFY(const_fplugin.factory());

    KService nservice(QFINDTESTDATA("noplugin.desktop"));
    KPluginName nname(nservice);
    QVERIFY(!nname.isValid());
    QVERIFY2(nname.name().isEmpty(), qPrintable(nname.name()));
    QVERIFY(!nname.errorString().isEmpty());
    KPluginLoader nplugin(nservice);
    QVERIFY(!nplugin.factory());

    KService iservice(QStringLiteral("idonotexist.desktop"));
    KPluginName iname(iservice);
    QVERIFY(!iname.isValid());
    QVERIFY2(iname.name().isEmpty(), qPrintable(iname.name()));
    QVERIFY(!iname.errorString().isEmpty());
    KPluginLoader iplugin(iservice);
    QVERIFY(!iplugin.factory());
}

void KServiceTest::testKPluginInfoQuery()
{
    KPluginInfo info(KPluginMetaData(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String("/kservices5/") + "fakepart2.desktop"));

    QCOMPARE(info.property(QStringLiteral("X-KDE-TestList")).toStringList().size(), 2);
}

void KServiceTest::testCompleteBaseName()
{
    QCOMPARE(KServiceUtilPrivate::completeBaseName(QStringLiteral("/home/x/.qttest/share/kservices5/fakepart2.desktop")), QStringLiteral("fakepart2"));
    // dots in filename before .desktop extension:
    QCOMPARE(KServiceUtilPrivate::completeBaseName(QStringLiteral("/home/x/.qttest/share/kservices5/org.kde.fakeapp.desktop")), QStringLiteral("org.kde.fakeapp"));
}

void KServiceTest::testEntryPathToName()
{
    QCOMPARE(KService(QStringLiteral("c.desktop")).name(), QStringLiteral("c"));
    QCOMPARE(KService(QStringLiteral("a.b.c.desktop")).name(), QStringLiteral("a.b.c")); // dots in filename before .desktop extension
    QCOMPARE(KService(QStringLiteral("/hallo/a.b.c.desktop")).name(), QStringLiteral("a.b.c"));
}

void KServiceTest::testKPluginMetaData()
{
    const QString fakePart = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String("/kservices5/") + "fakepart.desktop";
    KPluginMetaData md(fakePart);
    KService::Ptr service(new KService(fakePart));
    KPluginInfo info(service);
    auto info_md = info.toMetaData();
    QCOMPARE(info_md.formFactors(), md.formFactors());
}

void KServiceTest::testTraderQueryMustRebuildSycoca()
{
    QVERIFY(!KServiceTypeProfile::hasProfile(QStringLiteral("FakeBasePart")));
    QTest::qWait(1000);
    createFakeService(QStringLiteral("fakeservice_querymustrebuild.desktop"), QString()); // just to touch the dir
    KService::List offers = KServiceTypeTrader::self()->query(QStringLiteral("FakeBasePart"));
    QVERIFY(offers.count() > 0);
}
