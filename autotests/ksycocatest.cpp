/* This file is part of the KDE project
   Copyright (C) 2015 David Faure <faure@kde.org>

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
#include <ksycoca_p.h>
#include <QTemporaryDir>
#include <QTest>
#include <QDebug>
#include <kservicetype.h>
#include <kdesktopfile.h>
#include <kconfiggroup.h>
#include <QSignalSpy>
#include <QProcess>
#include <kservice.h>
#include <kservicefactory_p.h>

// taken from tst_qstandardpaths
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC) && !defined(Q_OS_BLACKBERRY) && !defined(Q_OS_ANDROID)
#define Q_XDG_PLATFORM
#endif

class KSycocaTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        QStandardPaths::enableTestMode(true);

        QVERIFY(m_tempDir.isValid());

        // we don't need the services dir -> ensure there isn't one, so we can check allResourceDirs below.
        QDir(servicesDir()).removeRecursively();
    }
    void ensureCacheValidShouldCreateDB();
    void kBuildSycocaShouldEmitDatabaseChanged();
    void testAllResourceDirs();
    void testOtherAppDir();

private:
    QString servicesDir() { return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/kservices5"; }
    QString serviceTypesDir() { return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/kservicetypes5"; }

    static void runKBuildSycoca(const QProcessEnvironment &environment);

    QTemporaryDir m_tempDir;
};

QTEST_MAIN(KSycocaTest)

void KSycocaTest::ensureCacheValidShouldCreateDB()
{
#ifdef Q_XDG_PLATFORM
    // Set XDG_DATA_DIRS to avoid a global database breaking the test
    const QByteArray oldDataDirs = qgetenv("XDG_DATA_DIRS");
    const QString dataDir = m_tempDir.path();
    qputenv("XDG_DATA_DIRS", QFile::encodeName(dataDir));
#endif
    // Don't use KSycoca::self() here in order to not mess it up with a different XDG_DATA_DIRS for other tests
    KSycoca mySycoca;
    QFile::remove(KSycoca::absoluteFilePath());
    mySycoca.ensureCacheValid();
    QVERIFY(QFile::exists(KSycoca::absoluteFilePath()));
#ifdef Q_XDG_PLATFORM
    qputenv("XDG_DATA_DIRS", oldDataDirs);
#endif
}

void KSycocaTest::kBuildSycocaShouldEmitDatabaseChanged()
{
    // It used to be a DBus signal, now it's file watching
    QTest::qWait(1000); // ensure the file watching notices it's a new second
    QSignalSpy spy(KSycoca::self(), SIGNAL(databaseChanged(QStringList)));
    runKBuildSycoca(QProcessEnvironment::systemEnvironment());
    qDebug() << "waiting for signal";
    QVERIFY(spy.wait(10000));
    qDebug() << "got signal";
}

void KSycocaTest::runKBuildSycoca(const QProcessEnvironment &environment)
{
    QProcess proc;
    const QString kbuildsycoca = QStandardPaths::findExecutable(KBUILDSYCOCA_EXENAME);
    QVERIFY(!kbuildsycoca.isEmpty());
    QStringList args;
    args << "--testmode";
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
    QVERIFY2(dirs.contains(servicesDir()), qPrintable(dirs.join(',')));
    QVERIFY2(dirs.contains(serviceTypesDir()), qPrintable(dirs.join(',')));
}

void KSycocaTest::testOtherAppDir()
{
#ifndef Q_XDG_PLATFORM
    QSKIP("This test requires XDG_DATA_DIRS");
#endif

    const QString dataDir = m_tempDir.path();
    qputenv("XDG_DATA_DIRS", QFile::encodeName(dataDir));
    QCOMPARE(dataDir, QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation).last());
    QVERIFY(!KService::serviceByDesktopPath("test_app_other.desktop"));

    const QString appDir = dataDir + "/applications";

    // test_app_other: live in a different application directory
    const QString testAppOther = appDir + "/test_app_other.desktop";
    KDesktopFile file(testAppOther);
    KConfigGroup group = file.desktopGroup();
    group.writeEntry("Type", "Application");
    group.writeEntry("Exec", "kded5");
    group.writeEntry("Name", "Test App Other");
    qDebug() << "Just created" << testAppOther;
    file.sync();

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("XDG_DATA_DIRS", dataDir);
    runKBuildSycoca(env);

#if 0 // for debugging
    const KService::List lst = KService::allServices();
    QVERIFY(!lst.isEmpty());
    Q_FOREACH (const KService::Ptr &serv, lst) {
        qDebug() << serv->entryPath() << serv->storageId() /*<< serv->desktopEntryName()*/;
    }
#endif

    // This is still NOT available. kbuildsycoca created a different DB file, the one we read from hasn't changed.
    // Changing XDG_DATA_DIRS at runtime isn't supported, so this test isn't doing what apps would do.
    // The point however is that another app using different dirs cannot mess up our DB.
    QVERIFY(!KService::serviceByStorageId("test_app_other.desktop"));

    // Check here what the other app would see, by creating another sycoca instance.
    KSycoca otherAppSycoca;
    // do what serviceByStorageId does:
    otherAppSycoca.ensureCacheValid();
    QVERIFY(otherAppSycoca.d->serviceFactory()->findServiceByStorageId("test_app_other.desktop"));
    QVERIFY(otherAppSycoca.d->m_databasePath != KSycoca::self()->d->m_databasePath); // check that they use a different filename
    // check that the timestamp code works
    QVERIFY(!otherAppSycoca.d->needsRebuild());
}

#include "ksycocatest.moc"
