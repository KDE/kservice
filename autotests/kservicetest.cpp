/*
    SPDX-FileCopyrightText: 2006 David Faure <faure@kde.org>
    SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "kservicetest.h"

#include "setupxdgdirs.h"

#include <locale.h>

#include <QTest>

#include <../src/services/kserviceutil_p.h> // for KServiceUtilPrivate
#include <KConfig>
#include <KConfigGroup>
#include <KDesktopFile>
#include <kapplicationtrader.h>
#include <kbuildsycoca_p.h>
#include <ksycoca.h>

#include <KPluginMetaData>
#include <kplugininfo.h>
#include <kservicegroup.h>
#include <kservicetype.h>
#include <kservicetypeprofile.h>
#include <kservicetypetrader.h>

#include <QFile>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QThread>

#include <QDebug>
#include <QLoggingCategory>
#include <QMimeDatabase>

QTEST_MAIN(KServiceTest)

extern KSERVICE_EXPORT int ksycoca_ms_between_checks;

static void eraseProfiles()
{
    QString profilerc = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QLatin1String{"/profilerc"};
    if (!profilerc.isEmpty()) {
        QFile::remove(profilerc);
    }

    profilerc = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QLatin1String{"/servicetype_profilerc"};
    if (!profilerc.isEmpty()) {
        QFile::remove(profilerc);
    }
}

void KServiceTest::initTestCase()
{
    // Set up a layer in the bin dir so ksycoca finds the KPluginInfo and Application servicetypes
    setupXdgDirs();
    QStandardPaths::setTestModeEnabled(true);

    QLoggingCategory::setFilterRules(QStringLiteral("*.debug=true"));

    // A non-C locale is necessary for some tests.
    // This locale must have the following properties:
    //   - some character other than dot as decimal separator
    // If it cannot be set, locale-dependent tests are skipped.
    setlocale(LC_ALL, "fr_FR.utf8");
    m_hasNonCLocale = (setlocale(LC_ALL, nullptr) == QByteArray("fr_FR.utf8"));
    if (!m_hasNonCLocale) {
        qDebug() << "Setting locale to fr_FR.utf8 failed";
    }

    eraseProfiles();

    if (!KSycoca::isAvailable()) {
        runKBuildSycoca();
    }

    // Create some fake services for the tests below, and ensure they are in ksycoca.

    bool mustUpdateKSycoca = false;

    // fakeservice: deleted and recreated by testDeletingService, don't use in other tests
    const QString fakeServiceDeleteMe =
        QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String("/kservices5/fakeservice_deleteme.desktop");
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
    const QString fakePart = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String{"/kservices5/fakepart.desktop"};
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

    const QString fakePart2 = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String{"/kservices5/fakepart2.desktop"};
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

    const QString preferredPart = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String{"/kservices5/preferredpart.desktop"};
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

    const QString otherPart = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String{"/kservices5/otherpart.desktop"};
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
    const QString fakeTextplugin = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String{"/kservices5/faketextplugin.desktop"};
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
    const QString fakePluginType =
        QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String{"/kservicetypes5/fakeplugintype.desktop"};
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
    const QString fakeBasePart = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String{"/kservicetypes5/fakebasepart.desktop"};
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
    const QString fakeDerivedPart =
        QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String{"/kservicetypes5/fakederivedpart.desktop"};
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
    const QString fakeKdedModule =
        QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String{"/kservicetypes5/fakekdedmodule.desktop"};
    if (!QFile::exists(fakeKdedModule)) {
        const QString src = QFINDTESTDATA("fakekdedmodule.desktop");
        QVERIFY(QFile::copy(src, fakeKdedModule));
        mustUpdateKSycoca = true;
    }

    // faketestapp.desktop
    const QString testApp = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation) + QLatin1String("/org.kde.faketestapp.desktop");
    if (!QFile::exists(testApp)) {
        QVERIFY(QDir().mkpath(QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation)));
        const QString src = QFINDTESTDATA("org.kde.faketestapp.desktop");
        QVERIFY(!src.isEmpty());
        QVERIFY2(QFile::copy(src, testApp), qPrintable(testApp));
        qDebug() << "Created" << testApp;
        mustUpdateKSycoca = true;
    }

    if (mustUpdateKSycoca) {
        // Update ksycoca in ~/.qttest after creating the above
        runKBuildSycoca(true);
    }
    QVERIFY(KServiceType::serviceType(QStringLiteral("FakePluginType")));
    QVERIFY(KServiceType::serviceType(QStringLiteral("FakeBasePart")));
    QVERIFY(KServiceType::serviceType(QStringLiteral("FakeDerivedPart")));
    QVERIFY(KService::serviceByDesktopName(QStringLiteral("org.kde.faketestapp")));
}

void KServiceTest::runKBuildSycoca(bool noincremental)
{
#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 80)
    QSignalSpy spy(KSycoca::self(), qOverload<const QStringList &>(&KSycoca::databaseChanged));
#else
    QSignalSpy spy(KSycoca::self(), &KSycoca::databaseChanged);
#endif

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
    const QStringList services = QStringList() << QStringLiteral("fakeservice.desktop") << QStringLiteral("fakepart.desktop")
                                               << QStringLiteral("faketextplugin.desktop") << QStringLiteral("fakeservice_querymustrebuild.desktop");
    for (const QString &service : services) {
        const QString fakeService = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String("/kservices5/") + service;
        QFile::remove(fakeService);
    }
    const QStringList serviceTypes = QStringList() << QStringLiteral("fakeplugintype.desktop");
    for (const QString &serviceType : serviceTypes) {
        const QString fakeServiceType = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String("/kservicetypes5/") + serviceType;
        // QFile::remove(fakeServiceType);
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
    QVERIFY(QMimeDatabase().mimeTypeForName(QStringLiteral("text/html")).isValid());
    const QString fakePart = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String{"/kservices5/fakepart.desktop"};
    QVERIFY(QFile::exists(fakePart));
    KService service(fakePart);
    QVERIFY(service.isValid());
    QCOMPARE(service.mimeTypes(), (QStringList{QStringLiteral("text/plain"), QStringLiteral("text/html")}));
}

void KServiceTest::testConstructorKDesktopFileFullPath()
{
    const QString fakePart = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String{"/kservices5/fakepart.desktop"};
    QVERIFY(QFile::exists(fakePart));
    KDesktopFile desktopFile(fakePart);
    KService service(&desktopFile);
    QVERIFY(service.isValid());
    QCOMPARE(service.mimeTypes(), (QStringList{QStringLiteral("text/plain"), QStringLiteral("text/html")}));
}

void KServiceTest::testConstructorKDesktopFile() // as happens inside kbuildsycoca.cpp
{
    KDesktopFile desktopFile(QStandardPaths::GenericDataLocation, QStringLiteral("kservices5/fakepart.desktop"));
    QCOMPARE(KService(&desktopFile, QStringLiteral("kservices5/fakepart.desktop")).mimeTypes(),
             (QStringList{QStringLiteral("text/plain"), QStringLiteral("text/html")}));
}

void KServiceTest::testCopyConstructor()
{
    const QString fakePart = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String{"/kservices5/fakepart.desktop"};
    QVERIFY(QFile::exists(fakePart));
    KDesktopFile desktopFile(fakePart);
    // KRun needs to make a copy of a KService that will go out of scope, let's test that here.
    KService::Ptr service;
    {
        KService origService(&desktopFile);
        service = new KService(origService);
    }
    QVERIFY(service->isValid());
    QCOMPARE(service->mimeTypes(), (QStringList{QStringLiteral("text/plain"), QStringLiteral("text/html")}));
}

void KServiceTest::testCopyInvalidService()
{
    KService::Ptr service;
    {
        KService origService{QString()}; // this still sets a d_ptr, so no problem;
        QVERIFY(!origService.isValid());
        service = new KService(origService);
    }
    QVERIFY(!service->isValid());
}

void KServiceTest::testProperty()
{
    ksycoca_ms_between_checks = 0;

    // Let's try creating a desktop file and ensuring it's noticed by the timestamp check
    QTest::qWait(1000);
    const QString fakeCookie = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String{"/kservices5/kded/fakekcookiejar.desktop"};
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

    QCOMPARE(kdedkcookiejar->property(QStringLiteral("ServiceTypes")).toStringList().join(QLatin1Char(',')), QStringLiteral("FakeKDEDModule"));
    QCOMPARE(kdedkcookiejar->property(QStringLiteral("X-KDE-Kded-autoload")).toBool(), false);
    QCOMPARE(kdedkcookiejar->property(QStringLiteral("X-KDE-Kded-load-on-demand")).toBool(), true);
    QVERIFY(!kdedkcookiejar->property(QStringLiteral("Name")).toString().isEmpty());
    QVERIFY(!kdedkcookiejar->property(QStringLiteral("Name[fr]"), QMetaType::QString).isValid());

    // TODO: for this we must install a servicetype desktop file...
    // KService::Ptr kjavaappletviewer = KService::serviceByDesktopPath("kjavaappletviewer.desktop");
    // QVERIFY(kjavaappletviewer);
    // QCOMPARE(kjavaappletviewer->property("X-KDE-BrowserView-PluginsInfo").toString(), QString("kjava/pluginsinfo"));

    // Test property("X-KDE-Protocols"), which triggers the KServiceReadProperty code.
    KService::Ptr fakePart = KService::serviceByDesktopPath(QStringLiteral("fakepart.desktop"));
    QVERIFY(fakePart); // see initTestCase; it should be found.
    QVERIFY(fakePart->propertyNames().contains(QLatin1String("X-KDE-Protocols")));
    QCOMPARE(fakePart->mimeTypes(),
             QStringList() << QStringLiteral("text/plain") << QStringLiteral("text/html")); // okular relies on subclasses being kept here
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
    for (const KServiceType::Ptr &servtype : allServiceTypes) {
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
    bool foundTestApp = false;

    for (const KService::Ptr &service : lst) {
        QVERIFY(service->isType(KST_KService));

        const QString name = service->name();
        const QString entryPath = service->entryPath();
        if (entryPath.contains(QLatin1String{"fake"})) {
            qDebug() << name << "entryPath=" << entryPath << "menuId=" << service->menuId();
        }
        QVERIFY(!name.isEmpty());
        QVERIFY(!entryPath.isEmpty());

        KService::Ptr lookedupService = KService::serviceByDesktopPath(entryPath);
        QVERIFY(lookedupService); // not null
        QCOMPARE(lookedupService->entryPath(), entryPath);

        if (service->isApplication()) {
            const QString menuId = service->menuId();
            if (menuId.isEmpty()) {
                qWarning("%s has an empty menuId!", qPrintable(entryPath));
            } else if (menuId == QLatin1String{"org.kde.faketestapp.desktop"}) {
                foundTestApp = true;
            }
            QVERIFY(!menuId.isEmpty());
            lookedupService = KService::serviceByMenuId(menuId);
            QVERIFY(lookedupService); // not null
            QCOMPARE(lookedupService->menuId(), menuId);
        }
    }
    QVERIFY(foundTestApp);
}

// Helper method for all the trader tests
static bool offerListHasService(const KService::List &offers, const QString &entryPath)
{
    bool found = false;
    for (const auto &servicePtr : offers) {
        if (servicePtr->entryPath() == entryPath) {
            if (found) { // should be there only once
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
    KService::Ptr testapp = KService::serviceByDesktopName(QStringLiteral("org.kde.faketestapp"));
    QVERIFY(testapp);
    QCOMPARE(testapp->menuId(), QStringLiteral("org.kde.faketestapp.desktop"));
    // qDebug() << testapp->entryPath();
#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 102)
    QCOMPARE(int(testapp->dbusStartupType()), int(KService::DBusUnique));
#endif
}

void KServiceTest::testByStorageId()
{
    if (!KSycoca::isAvailable()) {
        QSKIP("ksycoca not available");
    }
    QVERIFY(!QStandardPaths::locate(QStandardPaths::ApplicationsLocation, QStringLiteral("org.kde.faketestapp.desktop")).isEmpty());
    QVERIFY(KService::serviceByMenuId(QStringLiteral("org.kde.faketestapp.desktop")));
    QVERIFY(!KService::serviceByMenuId(QStringLiteral("org.kde.faketestapp"))); // doesn't work, extension mandatory
    QVERIFY(!KService::serviceByMenuId(QStringLiteral("faketestapp.desktop"))); // doesn't work, full filename mandatory
    QVERIFY(KService::serviceByStorageId(QStringLiteral("org.kde.faketestapp.desktop")));
    QVERIFY(KService::serviceByStorageId(QStringLiteral("org.kde.faketestapp")));

    QVERIFY(KService::serviceByDesktopName(QStringLiteral("org.kde.faketestapp")));
    QCOMPARE(KService::serviceByDesktopName(QStringLiteral("org.kde.faketestapp"))->menuId(), QStringLiteral("org.kde.faketestapp.desktop"));
}

#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 90)
void KServiceTest::testServiceTypeTraderForReadOnlyPart()
{
    if (!KSycoca::isAvailable()) {
        QSKIP("ksycoca not available");
    }

    // Querying trader for services associated with FakeBasePart
    KService::List offers = KServiceTypeTrader::self()->query(QStringLiteral("FakeBasePart"));
    QVERIFY(offers.count() > 0);

    if (!offerListHasService(offers, QStringLiteral("fakepart.desktop")) //
        || !offerListHasService(offers, QStringLiteral("fakepart2.desktop")) //
        || !offerListHasService(offers, QStringLiteral("otherpart.desktop")) //
        || !offerListHasService(offers, QStringLiteral("preferredpart.desktop"))) {
        for (KService::Ptr service : std::as_const(offers)) {
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
#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 67)
    bool lastAllowedAsDefault = true;
#endif
    for (KService::Ptr service : std::as_const(offers)) {
        const int preference = service->initialPreference(); // ## might be wrong if we use per-servicetype preferences...
        // qDebug( "%s has preference %d, allowAsDefault=%d", qPrintable( path ), preference, service->allowAsDefault() );
#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 67)
        if (lastAllowedAsDefault && !service->allowAsDefault()) {
            // first "not allowed as default" offer
            lastAllowedAsDefault = false;
            lastPreference = -1; // restart
        }
#endif
        if (lastPreference != -1) {
            QVERIFY(preference <= lastPreference);
        }
        lastPreference = preference;
    }

#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 90)
    // Now look for any FakePluginType
    offers = KServiceTypeTrader::self()->query(QStringLiteral("FakePluginType"));
    QVERIFY(offerListHasService(offers, QStringLiteral("fakeservice.desktop")));
    QVERIFY(offerListHasService(offers, QStringLiteral("faketextplugin.desktop")));
#endif
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
    offers = KServiceTypeTrader::self()->query(QStringLiteral("FakePluginType"),
                                               QStringLiteral("'textplugin' ~ Library")); // note: "is contained in", not "contains"...
    QCOMPARE(offers.count(), 1);
    QVERIFY(offerListHasService(offers, QStringLiteral("faketextplugin.desktop")));

    // "contains" case insensitive
    offers = KServiceTypeTrader::self()->query(QStringLiteral("FakePluginType"),
                                               QStringLiteral("'teXtPluGin' ~~ Library")); // note: "is contained in", not "contains"...
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
#endif

void KServiceTest::testSubseqConstraints()
{
    auto test = [](const char *pattern, const char *text, bool sensitive) {
        return KApplicationTrader::isSubsequence(QString::fromLatin1(pattern), QString::fromLatin1(text), sensitive ? Qt::CaseSensitive : Qt::CaseInsensitive);
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
    QString fakepartPath = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("kservices5/fakepart.desktop"));
    QVERIFY(!fakepartPath.isEmpty());
    KService fakepart(fakepartPath);
    QVERIFY(fakepart.hasServiceType(QStringLiteral("FakeBasePart")));
    QVERIFY(fakepart.hasServiceType(QStringLiteral("FakeDerivedPart")));
    QCOMPARE(fakepart.mimeTypes(), QStringList() << QStringLiteral("text/plain") << QStringLiteral("text/html"));

    QString faketextPluginPath = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("kservices5/faketextplugin.desktop"));
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

#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 66)
void KServiceTest::testWriteServiceTypeProfile()
{
    const QString serviceType = QStringLiteral("FakeBasePart");
    KService::List services;
    KService::List disabledServices;
    services.append(KService::serviceByDesktopPath(QStringLiteral("preferredpart.desktop")));
    services.append(KService::serviceByDesktopPath(QStringLiteral("fakepart.desktop")));
    disabledServices.append(KService::serviceByDesktopPath(QStringLiteral("fakepart2.desktop")));

    for (const KService::Ptr &serv : std::as_const(services)) {
        QVERIFY(serv);
    }
    for (const KService::Ptr &serv : std::as_const(disabledServices)) {
        QVERIFY(serv);
    }

    KServiceTypeProfile::writeServiceTypeProfile(serviceType, services, disabledServices);

    // Check that the file got written
    QString profilerc = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QLatin1String{"/servicetype_profilerc"};
    QVERIFY(!profilerc.isEmpty());
    QVERIFY(QFile::exists(profilerc));

    KService::List offers = KServiceTypeTrader::self()->query(serviceType);
    QVERIFY(offers.count() > 0); // not empty

    // foreach( KService::Ptr service, offers )
    //    qDebug( "%s %s", qPrintable( service->name() ), qPrintable( service->entryPath() ) );

    QVERIFY(offers.count() >= 2);
    QCOMPARE(offers[0]->entryPath(), QStringLiteral("preferredpart.desktop"));
    QCOMPARE(offers[1]->entryPath(), QStringLiteral("fakepart.desktop"));
    QVERIFY(offerListHasService(offers, QStringLiteral("otherpart.desktop"))); // should still be somewhere in there
    QVERIFY(!offerListHasService(offers, QStringLiteral("fakepart2.desktop"))); // it got disabled above
}
#endif

#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 90)
void KServiceTest::testDefaultOffers()
{
    // Now that we have a user-profile, let's see if defaultOffers indeed gives us the default ordering.
    const QString serviceType = QStringLiteral("FakeBasePart");
    KService::List offers = KServiceTypeTrader::self()->defaultOffers(serviceType);
    QVERIFY(offers.count() > 0); // not empty
    QVERIFY(offerListHasService(offers, QStringLiteral("fakepart2.desktop"))); // it's here even though it's disabled in the profile
    QVERIFY(offerListHasService(offers, QStringLiteral("otherpart.desktop")));
    if (m_firstOffer.isEmpty()) {
        QSKIP("testServiceTypeTraderForReadOnlyPart not run");
    }
    QCOMPARE(offers[0]->entryPath(), m_firstOffer);
}
#endif

#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 66)
void KServiceTest::testDeleteServiceTypeProfile()
{
    const QString serviceType = QStringLiteral("FakeBasePart");
    KServiceTypeProfile::deleteServiceTypeProfile(serviceType);

    KService::List offers = KServiceTypeTrader::self()->query(serviceType);
    QVERIFY(offers.count() > 0); // not empty
    QVERIFY(offerListHasService(offers, QStringLiteral("fakepart2.desktop"))); // it's back

    if (m_firstOffer.isEmpty()) {
        QSKIP("testServiceTypeTraderForReadOnlyPart not run");
    }
    QCOMPARE(offers[0]->entryPath(), m_firstOffer);
}
#endif

void KServiceTest::testActionsAndDataStream()
{
    KService::Ptr service = KService::serviceByStorageId(QStringLiteral("org.kde.faketestapp.desktop"));
    QVERIFY(service);
    QVERIFY(!service->property(QStringLiteral("Name[fr]"), QMetaType::QString).isValid());
    const QList<KServiceAction> actions = service->actions();
    QCOMPARE(actions.count(), 2); // NewWindow, NewTab
    const KServiceAction newTabAction = actions.at(1);
    QCOMPARE(newTabAction.name(), QStringLiteral("NewTab"));
    QCOMPARE(newTabAction.exec(), QStringLiteral("konsole --new-tab"));
    QCOMPARE(newTabAction.icon(), QStringLiteral("tab-new"));
    QCOMPARE(newTabAction.noDisplay(), false);
    QVERIFY(!newTabAction.isSeparator());
    QCOMPARE(newTabAction.service()->name(), service->name());
}

void KServiceTest::testServiceGroups()
{
    KServiceGroup::Ptr root = KServiceGroup::root();
    QVERIFY(root);
    qDebug() << root->groupEntries().count();

    KServiceGroup::Ptr group = root;
    QVERIFY(group);
    const KServiceGroup::List list = group->entries(true, // sorted
                                                    true, // exclude no display entries,
                                                    false, // allow separators
                                                    true); // sort by generic name

    qDebug() << list.count();
    for (KServiceGroup::SPtr s : list) {
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

void KServiceTest::createFakeService(const QString &filename, const QString &serviceType)
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

#include <QFutureSynchronizer>
#include <QThreadPool>
#include <QtConcurrentRun>

// Testing for concurrent access to ksycoca from multiple threads
// It's especially interesting to run this test as ./kservicetest testThreads
// so that even the ksycoca initialization is happening from N threads at the same time.
// Use valgrind --tool=helgrind to see the race conditions.

void KServiceTest::testReaderThreads()
{
    QThreadPool::globalInstance()->setMaxThreadCount(10);
    QFutureSynchronizer<void> sync;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    sync.addFuture(QtConcurrent::run(&KServiceTest::testAllServices, this));
    sync.addFuture(QtConcurrent::run(&KServiceTest::testAllServices, this));
    sync.addFuture(QtConcurrent::run(&KServiceTest::testAllServices, this));
    sync.addFuture(QtConcurrent::run(&KServiceTest::testHasServiceType1, this));
    sync.addFuture(QtConcurrent::run(&KServiceTest::testAllServices, this));
    sync.addFuture(QtConcurrent::run(&KServiceTest::testAllServices, this));
#else
    sync.addFuture(QtConcurrent::run(this, &KServiceTest::testAllServices));
    sync.addFuture(QtConcurrent::run(this, &KServiceTest::testAllServices));
    sync.addFuture(QtConcurrent::run(this, &KServiceTest::testAllServices));
    sync.addFuture(QtConcurrent::run(this, &KServiceTest::testHasServiceType1));
    sync.addFuture(QtConcurrent::run(this, &KServiceTest::testAllServices));
    sync.addFuture(QtConcurrent::run(this, &KServiceTest::testAllServices));
#endif
    sync.waitForFinished();
    QThreadPool::globalInstance()->setMaxThreadCount(1); // delete those threads
}

void KServiceTest::testThreads()
{
    QThreadPool::globalInstance()->setMaxThreadCount(10);
    QFutureSynchronizer<void> sync;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    sync.addFuture(QtConcurrent::run(&KServiceTest::testAllServices, this));
    sync.addFuture(QtConcurrent::run(&KServiceTest::testHasServiceType1, this));
    sync.addFuture(QtConcurrent::run(&KServiceTest::testDeletingService, this));
#else
    sync.addFuture(QtConcurrent::run(this, &KServiceTest::testAllServices));
    sync.addFuture(QtConcurrent::run(this, &KServiceTest::testHasServiceType1));
    sync.addFuture(QtConcurrent::run(this, &KServiceTest::testDeletingService));

#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 90)
    sync.addFuture(QtConcurrent::run(this, &KServiceTest::testTraderConstraints));
#endif
#endif // QT_VERSION

    // process events (DBus, inotify...), until we get all expected signals
    QTRY_COMPARE_WITH_TIMEOUT(m_sycocaUpdateDone.loadRelaxed(), 1, 15000); // not using a bool, just to silence helgrind
    qDebug() << "Joining all threads";
    sync.waitForFinished();
}

#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 86)
void KServiceTest::testOperatorKPluginName()
{
    QT_WARNING_PUSH
    QT_WARNING_DISABLE_DEPRECATED
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
    QT_WARNING_POP
}
#endif

#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 90)
void KServiceTest::testKPluginInfoQuery()
{
    KPluginInfo info(KPluginMetaData::fromDesktopFile(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
                                                      + QLatin1String{"/kservices5/fakepart2.desktop"}));

    QCOMPARE(info.property(QStringLiteral("X-KDE-TestList")).toStringList().size(), 2);
}
#endif

void KServiceTest::testCompleteBaseName()
{
    QCOMPARE(KServiceUtilPrivate::completeBaseName(QStringLiteral("/home/x/.qttest/share/kservices5/fakepart2.desktop")), QStringLiteral("fakepart2"));
    // dots in filename before .desktop extension:
    QCOMPARE(KServiceUtilPrivate::completeBaseName(QStringLiteral("/home/x/.qttest/share/kservices5/org.kde.fakeapp.desktop")),
             QStringLiteral("org.kde.fakeapp"));
}

void KServiceTest::testEntryPathToName()
{
    QCOMPARE(KService(QStringLiteral("c.desktop")).name(), QStringLiteral("c"));
    QCOMPARE(KService(QStringLiteral("a.b.c.desktop")).name(), QStringLiteral("a.b.c")); // dots in filename before .desktop extension
    QCOMPARE(KService(QStringLiteral("/hallo/a.b.c.desktop")).name(), QStringLiteral("a.b.c"));
}

#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 0)
void KServiceTest::testKPluginMetaData()
{
    const QString fakePart = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String{"/kservices5/fakepart.desktop"};
    KPluginMetaData md = KPluginMetaData::fromDesktopFile(fakePart);
    KService::Ptr service(new KService(fakePart));
    KPluginInfo info(service);
    auto info_md = info.toMetaData();
    QCOMPARE(info_md.formFactors(), md.formFactors());
}
#endif

#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 90)
void KServiceTest::testTraderQueryMustRebuildSycoca()
{
    QVERIFY(!KServiceTypeProfile::hasProfile(QStringLiteral("FakeBasePart")));
    QTest::qWait(1000);
    createFakeService(QStringLiteral("fakeservice_querymustrebuild.desktop"), QString()); // just to touch the dir
    KService::List offers = KServiceTypeTrader::self()->query(QStringLiteral("FakeBasePart"));
    QVERIFY(offers.count() > 0);
}
#endif

void KServiceTest::testAliasFor()
{
    if (!KSycoca::isAvailable()) {
        QSKIP("ksycoca not available");
    }
    KService::Ptr testapp = KService::serviceByDesktopName(QStringLiteral("org.kde.faketestapp"));
    QVERIFY(testapp);
    QCOMPARE(testapp->aliasFor(), QStringLiteral("org.kde.okular"));
}
