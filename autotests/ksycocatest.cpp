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
#include <kbuildsycoca_p.h>
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
#include <kservicetypefactory_p.h>

// taken from tst_qstandardpaths
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC) && !defined(Q_OS_BLACKBERRY) && !defined(Q_OS_ANDROID)
#define Q_XDG_PLATFORM
#endif

extern KSERVICE_EXPORT int ksycoca_ms_between_checks;

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

#ifdef Q_XDG_PLATFORM
        qputenv("XDG_DATA_DIRS", QFile::encodeName(m_tempDir.path()));
#else
        // We need to make changes to a global dir without messing up the system
        QSKIP("This test requires XDG_DATA_DIRS");
#endif
        KDesktopFile file(serviceTypesDir() + "/fakeGlobalServiceType.desktop");
        KConfigGroup group = file.desktopGroup();
        group.writeEntry("Comment", "Fake Global ServiceType");
        group.writeEntry("Type", "ServiceType");
        group.writeEntry("X-KDE-ServiceType", "FakeGlobalServiceType");
        file.sync();
        qDebug() << "created" << serviceTypesDir() + "/fakeGlobalServiceType.desktop";
    }
    void cleanupTestCase()
    {
        QFile::remove(serviceTypesDir() + "/fakeLocalServiceType.desktop");
        QFile::remove(KSycoca::absoluteFilePath());
    }
    void ensureCacheValidShouldCreateDB();
    void kBuildSycocaShouldEmitDatabaseChanged();
    void testAllResourceDirs();
    void testDeletingSycoca();
    void testGlobalSycoca();
    void testNonReadableSycoca();

private:
    QString servicesDir() { return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/kservices5"; }
    QString serviceTypesDir() { return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/kservicetypes5"; }

    static void runKBuildSycoca(const QProcessEnvironment &environment, bool global = false);

    QTemporaryDir m_tempDir;
};

QTEST_MAIN(KSycocaTest)

void KSycocaTest::ensureCacheValidShouldCreateDB() // this is what kded does on startup
{
    QFile::remove(KSycoca::absoluteFilePath());
    KSycoca::self()->ensureCacheValid();
    QVERIFY(QFile::exists(KSycoca::absoluteFilePath()));
    QVERIFY(KServiceType::serviceType("FakeGlobalServiceType"));
}

void KSycocaTest::kBuildSycocaShouldEmitDatabaseChanged()
{
    // It used to be a DBus signal, now it's file watching
    QTest::qWait(1000); // ensure the file watching notices it's a new second
    // Ensure kbuildsycoca has something to do
    QFile file(serviceTypesDir() + "/fake_just_to_touch_the_dir.desktop");
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.close();
    QVERIFY(file.remove());
    // Run kbuildsycoca
    QSignalSpy spy(KSycoca::self(), SIGNAL(databaseChanged(QStringList)));
    runKBuildSycoca(QProcessEnvironment::systemEnvironment());
    qDebug() << "waiting for signal";
    QVERIFY(spy.wait(20000));
    qDebug() << "got signal";
}

void KSycocaTest::runKBuildSycoca(const QProcessEnvironment &environment, bool global)
{
    QProcess proc;
    const QString kbuildsycoca = QStandardPaths::findExecutable(KBUILDSYCOCA_EXENAME);
    QVERIFY(!kbuildsycoca.isEmpty());
    QStringList args;
    args << "--testmode";
    if (global) {
        args << "--global";
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
    QVERIFY2(dirs.contains(servicesDir()), qPrintable(dirs.join(',')));
    QVERIFY2(dirs.contains(serviceTypesDir()), qPrintable(dirs.join(',')));
}

void KSycocaTest::testDeletingSycoca()
{
    // Mostly the same as ensureCacheValidShouldCreateDB, but KSycoca::self() already exists
    // So this is a check that deleting sycoca doesn't make apps crash (bug 343618).
    QFile::remove(KSycoca::absoluteFilePath());
    ksycoca_ms_between_checks = 0;
    QVERIFY(KServiceType::serviceType("FakeGlobalServiceType"));
    QVERIFY(QFile::exists(KSycoca::absoluteFilePath()));
}

void KSycocaTest::testGlobalSycoca()
{
    // No local DB
    QFile::remove(KSycoca::absoluteFilePath());

    // Build global DB
    // We could do it in-process, but let's check what a sysadmin would do: run kbuildsycoca5 --global
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("XDG_DATA_DIRS", m_tempDir.path());
    runKBuildSycoca(env, true /*global*/);
    QVERIFY(QFile::exists(KSycoca::absoluteFilePath(KSycoca::GlobalDatabase)));

    KSycoca::self()->ensureCacheValid();
    QVERIFY(!QFile::exists(KSycoca::absoluteFilePath()));

    // Now create a local file, after a 1s delay, until QDateTime includes ms...
    QTest::qWait(1000);
    KDesktopFile file(serviceTypesDir() + "/fakeLocalServiceType.desktop");
    KConfigGroup group = file.desktopGroup();
    group.writeEntry("Comment", "Fake Local ServiceType");
    group.writeEntry("Type", "ServiceType");
    group.writeEntry("X-KDE-ServiceType", "FakeLocalServiceType");
    file.sync();
    qDebug() << "created" << serviceTypesDir() + "/fakeLocalServiceType.desktop";

    // Using ksycoca should now build a local one
    ksycoca_ms_between_checks = 0;
    QVERIFY(KServiceType::serviceType("FakeLocalServiceType"));
    QVERIFY(QFile::exists(KSycoca::absoluteFilePath()));
}

void KSycocaTest::testNonReadableSycoca()
{
    // Lose readability (to simulate e.g. owned by root)
    QFile(KSycoca::absoluteFilePath()).setPermissions(QFile::WriteOwner);
    ksycoca_ms_between_checks = 0;
    KBuildSycoca builder;
    QVERIFY(builder.recreate());
    QVERIFY(KServiceType::serviceType("FakeGlobalServiceType"));

    // cleanup
    QFile::remove(KSycoca::absoluteFilePath());
}

#include "ksycocatest.moc"
