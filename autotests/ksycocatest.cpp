/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2015 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <KConfigGroup>
#include <KDesktopFile>
#include <QDebug>
#include <QProcess>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <kbuildsycoca_p.h>
#include <kservice.h>
#include <kservicefactory_p.h>
#include <kservicetype.h>
#include <kservicetypefactory_p.h>
#include <ksycoca.h>
#include <ksycoca_p.h>

// ## use QFile::setFileTime when it lands in Qt
#include <time.h>
#ifdef Q_OS_UNIX
#include <sys/time.h>
#include <utime.h>
#endif

// taken from tst_qstandardpaths
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC) && !defined(Q_OS_BLACKBERRY) && !defined(Q_OS_ANDROID)
#define Q_XDG_PLATFORM
#endif

// On Unix, lastModified() finally returns milliseconds as well, since Qt 5.8.0
// Not sure about the situation on Windows though.
static const int s_waitDelay = 10;

extern KSERVICE_EXPORT int ksycoca_ms_between_checks;

class KSycocaTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);

        QVERIFY(m_tempDir.isValid());

        // we don't need the services dir -> ensure there isn't one, so we can check allResourceDirs below.
        QDir(servicesDir()).removeRecursively();

        QDir(menusDir()).removeRecursively();
        QDir().mkpath(menusDir() + QLatin1String{"/fakeSubserviceDirectory"});

#ifdef Q_XDG_PLATFORM
        qputenv("XDG_DATA_DIRS", QFile::encodeName(m_tempDir.path()));

        // so that vfolder_menu doesn't go look into /etc and /usr
        qputenv("XDG_CONFIG_DIRS", QFile::encodeName(m_tempDir.path()));
#else
        // We need to make changes to a global dir without messing up the system
        QSKIP("This test requires XDG_DATA_DIRS");
#endif
        createGlobalServiceType();
    }

    void cleanupTestCase()
    {
        QFile::remove(serviceTypesDir() + QLatin1String{"/fakeLocalServiceType.desktop"});
        QFile::remove(KSycoca::absoluteFilePath());
    }
    void ensureCacheValidShouldCreateDB();
    void kBuildSycocaShouldEmitDatabaseChanged();
    void dirInFutureShouldRebuildSycocaOnce();
    void dirTimestampShouldBeCheckedRecursively();
    void recursiveCheckShouldIgnoreLinksGoingUp();
    void testAllResourceDirs();
    void testDeletingSycoca();
    void testNonReadableSycoca();
    void extraFileInFutureShouldRebuildSycocaOnce();

private:
    void createGlobalServiceType()
    {
        KDesktopFile file(serviceTypesDir() + QLatin1String{"/fakeGlobalServiceType.desktop"});
        KConfigGroup group = file.desktopGroup();
        group.writeEntry("Comment", "Fake Global ServiceType");
        group.writeEntry("Type", "ServiceType");
        group.writeEntry("X-KDE-ServiceType", "FakeGlobalServiceType");
        file.sync();
        qDebug() << "created" << serviceTypesDir() + QLatin1String{"/fakeGlobalServiceType.desktop"};
    }
    QString servicesDir() const
    {
        return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String{"/kservices5"};
    }

    QString serviceTypesDir() const
    {
        return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String{"/kservicetypes5"};
    }

    QString extraFile() const
    {
        return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String{"/mimeapps.list"};
    }

    QString menusDir() const
    {
        return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QLatin1String{"/menus"};
    }

    QString appsDir() const
    {
        return QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation) + QLatin1Char('/');
    }

    static void runKBuildSycoca(const QProcessEnvironment &environment, bool global = false);

    QTemporaryDir m_tempDir;
};

QTEST_MAIN(KSycocaTest)

void KSycocaTest::ensureCacheValidShouldCreateDB() // this is what kded does on startup
{
    QFile::remove(KSycoca::absoluteFilePath());
    KSycoca::self()->ensureCacheValid();
    QVERIFY(QFile::exists(KSycoca::absoluteFilePath()));
    QVERIFY(KServiceType::serviceType(QStringLiteral("FakeGlobalServiceType")));
}

void KSycocaTest::kBuildSycocaShouldEmitDatabaseChanged()
{
    // It used to be a DBus signal, now it's file watching
    QTest::qWait(s_waitDelay);
    // Ensure kbuildsycoca has something to do
    QVERIFY(QFile::remove(serviceTypesDir() + QLatin1String{"/fakeGlobalServiceType.desktop"}));
    // Run kbuildsycoca
#if KSERVICE_BUILD_DEPRECATED_SINCE(5, 80)
    QSignalSpy spy(KSycoca::self(), qOverload<const QStringList &>(&KSycoca::databaseChanged));
#else
    QSignalSpy spy(KSycoca::self(), &KSycoca::databaseChanged);
#endif

    runKBuildSycoca(QProcessEnvironment::systemEnvironment());
    qDebug() << "waiting for signal";
    QVERIFY(spy.wait(20000));
    qDebug() << "got signal";
    // Put it back for other tests
    createGlobalServiceType();
}

void KSycocaTest::dirInFutureShouldRebuildSycocaOnce()
{
    const QDateTime oldTimestamp = QFileInfo(KSycoca::absoluteFilePath()).lastModified();

    // ### use QFile::setFileTime when it lands in Qt...
#ifdef Q_OS_UNIX
    const QString path = serviceTypesDir();
    struct timeval tp;
    gettimeofday(&tp, nullptr);
    struct utimbuf utbuf;
    utbuf.actime = tp.tv_sec;
    utbuf.modtime = tp.tv_sec + 60; // 60 second in the future
    QCOMPARE(utime(QFile::encodeName(path).constData(), &utbuf), 0);
    qDebug("Time changed for %s", qPrintable(path));
    qDebug() << QDateTime::currentDateTime() << QFileInfo(path).lastModified();
#else
    QSKIP("This test requires utime");
#endif
    ksycoca_ms_between_checks = 0;

    QTest::qWait(s_waitDelay);

    KSycoca::self()->ensureCacheValid();
    const QDateTime newTimestamp = QFileInfo(KSycoca::absoluteFilePath()).lastModified();
    QVERIFY(newTimestamp > oldTimestamp);

    QTest::qWait(s_waitDelay);

    KSycoca::self()->ensureCacheValid();
    const QDateTime againTimestamp = QFileInfo(KSycoca::absoluteFilePath()).lastModified();
    QCOMPARE(againTimestamp, newTimestamp); // same mtime, it didn't get rebuilt

    // Ensure we don't pollute the other tests, with our dir in the future.
#ifdef Q_OS_UNIX
    utbuf.modtime = tp.tv_sec;
    QCOMPARE(utime(QFile::encodeName(path).constData(), &utbuf), 0);
    qDebug("Time changed back for %s", qPrintable(path));
    qDebug() << QDateTime::currentDateTime() << QFileInfo(path).lastModified();
#endif
}

void KSycocaTest::dirTimestampShouldBeCheckedRecursively()
{
#ifndef Q_OS_UNIX
    QSKIP("This test requires utime");
#endif
    const QDateTime oldTimestamp = QFileInfo(KSycoca::absoluteFilePath()).lastModified();

    const QString path = menusDir() + QLatin1String("/fakeSubserviceDirectory");

    // ### use QFile::setFileTime when it lands in Qt...
#ifdef Q_OS_UNIX
    struct timeval tp;
    gettimeofday(&tp, nullptr);
    struct utimbuf utbuf;
    utbuf.actime = tp.tv_sec;
    utbuf.modtime = tp.tv_sec + 60; // 60 second in the future
    QCOMPARE(utime(QFile::encodeName(path).constData(), &utbuf), 0);
    qDebug("Time changed for %s", qPrintable(path));
    qDebug() << QDateTime::currentDateTime() << QFileInfo(path).lastModified();
#endif

    ksycoca_ms_between_checks = 0;
    QTest::qWait(s_waitDelay);

    qDebug() << "Waited 1s, calling ensureCacheValid (should rebuild)";
    KSycoca::self()->ensureCacheValid();
    const QDateTime newTimestamp = QFileInfo(KSycoca::absoluteFilePath()).lastModified();
    if (newTimestamp <= oldTimestamp) {
        qWarning() << "oldTimestamp=" << oldTimestamp << "newTimestamp=" << newTimestamp;
    }
    QVERIFY(newTimestamp > oldTimestamp);

    QTest::qWait(s_waitDelay);

    qDebug() << "Waited 1s, calling ensureCacheValid (should not rebuild)";
    KSycoca::self()->ensureCacheValid();
    const QDateTime againTimestamp = QFileInfo(KSycoca::absoluteFilePath()).lastModified();
    QCOMPARE(againTimestamp, newTimestamp); // same mtime, it didn't get rebuilt

    // Ensure we don't pollute the other tests
    QDir(path).removeRecursively();
}

void KSycocaTest::recursiveCheckShouldIgnoreLinksGoingUp()
{
#ifndef Q_OS_UNIX
    QSKIP("This test requires symlinks and utime");
#endif
    ksycoca_ms_between_checks = 0;
    const QString link = menusDir() + QLatin1String("/linkGoingUp");
    QVERIFY(QFile::link(QStringLiteral(".."), link));
    QTest::qWait(s_waitDelay);
    KSycoca::self()->ensureCacheValid();
    QVERIFY2(QFile::exists(KSycoca::absoluteFilePath()), qPrintable(KSycoca::absoluteFilePath()));
    const QDateTime oldTimestamp = QFileInfo(KSycoca::absoluteFilePath()).lastModified();
    QVERIFY(oldTimestamp.isValid());

    const QString path = QFileInfo(menusDir()).absolutePath(); // the parent of the menus dir

    // ### use QFile::setFileTime when it lands in Qt...
#ifdef Q_OS_UNIX
    struct timeval tp;
    gettimeofday(&tp, nullptr);
    struct utimbuf utbuf;
    utbuf.actime = tp.tv_sec;
    utbuf.modtime = tp.tv_sec + 60; // 60 second in the future
    QCOMPARE(utime(QFile::encodeName(path).constData(), &utbuf), 0);
    qDebug("Time changed for %s", qPrintable(path));
    qDebug() << QDateTime::currentDateTime() << QFileInfo(path).lastModified();
#endif

    ksycoca_ms_between_checks = 0;
    QTest::qWait(s_waitDelay);

    qDebug() << "Waited 1s, calling ensureCacheValid (should not rebuild)";
    KSycoca::self()->ensureCacheValid();
    const QDateTime againTimestamp = QFileInfo(KSycoca::absoluteFilePath()).lastModified();
    QCOMPARE(againTimestamp, oldTimestamp); // same mtime, it didn't get rebuilt

    // Ensure we don't pollute the other tests
    QFile(link).remove();
}

void KSycocaTest::runKBuildSycoca(const QProcessEnvironment &environment, bool global)
{
    QProcess proc;
    const QString kbuildsycoca = QStringLiteral(KBUILDSYCOCAEXE);
    QVERIFY(!kbuildsycoca.isEmpty());
    QStringList args;
    args << QStringLiteral("--testmode");
    if (global) {
        args << QStringLiteral("--global");
    }
    proc.setProcessChannelMode(QProcess::ForwardedChannels);
    proc.start(kbuildsycoca, args);
    proc.setProcessEnvironment(environment);

    proc.waitForFinished();
    QCOMPARE(proc.exitStatus(), QProcess::NormalExit);
}

void KSycocaTest::testAllResourceDirs()
{
    // Dirs that exist and dirs that don't exist, should both be in allResourceDirs().
    const QStringList dirs = KSycoca::self()->allResourceDirs();
    QVERIFY2(dirs.contains(servicesDir()), qPrintable(dirs.join(QLatin1Char{','})));
    QVERIFY2(dirs.contains(serviceTypesDir()), qPrintable(dirs.join(QLatin1Char{','})));
}

void KSycocaTest::testDeletingSycoca()
{
    // Mostly the same as ensureCacheValidShouldCreateDB, but KSycoca::self() already exists
    // So this is a check that deleting sycoca doesn't make apps crash (bug 343618).
    QFile::remove(KSycoca::absoluteFilePath());
    ksycoca_ms_between_checks = 0;
    QVERIFY(KServiceType::serviceType(QStringLiteral("FakeGlobalServiceType")));
    QVERIFY(QFile::exists(KSycoca::absoluteFilePath()));
}

void KSycocaTest::testNonReadableSycoca()
{
    // Lose readability (to simulate e.g. owned by root)
    QFile(KSycoca::absoluteFilePath()).setPermissions(QFile::WriteOwner);
    ksycoca_ms_between_checks = 0;
    KBuildSycoca builder;
    QVERIFY(builder.recreate());
    QVERIFY(!KServiceType::serviceType(QStringLiteral("FakeGlobalServiceType")));

    // cleanup
    QFile::remove(KSycoca::absoluteFilePath());
}

void KSycocaTest::extraFileInFutureShouldRebuildSycocaOnce()
{
    const QDateTime oldTimestamp = QFileInfo(KSycoca::absoluteFilePath()).lastModified();

    auto path = extraFile();
    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly));
    auto beginning = f.fileTime(QFileDevice::FileModificationTime);
    auto newdate = beginning.addSecs(60);
    qDebug() << "Time changed for " << newdate << path;
    QVERIFY(f.setFileTime(newdate, QFileDevice::FileModificationTime));

    ksycoca_ms_between_checks = 0;

    QTest::qWait(s_waitDelay);

    KSycoca::self()->ensureCacheValid();
    const QDateTime newTimestamp = QFileInfo(KSycoca::absoluteFilePath()).lastModified();
    QVERIFY(newTimestamp > oldTimestamp);

    QTest::qWait(s_waitDelay);

    KSycoca::self()->ensureCacheValid();
    const QDateTime againTimestamp = QFileInfo(KSycoca::absoluteFilePath()).lastModified();
    QCOMPARE(againTimestamp, newTimestamp); // same mtime, it didn't get rebuilt

    // Ensure we don't pollute the other tests, with our extra file in the future.
    QVERIFY(QFile::remove(path));
}

#include "ksycocatest.moc"
