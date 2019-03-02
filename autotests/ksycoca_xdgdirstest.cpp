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
#include <kservicetypefactory_p.h>

// taken from tst_qstandardpaths
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC) && !defined(Q_OS_BLACKBERRY) && !defined(Q_OS_ANDROID)
#define Q_XDG_PLATFORM
#endif

class KSycocaXdgDirsTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);

        QVERIFY(m_tempDir.isValid());
    }
    void cleanupTestCase()
    {
        QFile::remove(KSycoca::absoluteFilePath());
    }
    void testOtherAppDir();

private:
    static void runKBuildSycoca(const QProcessEnvironment &environment);

    QTemporaryDir m_tempDir;
};

QTEST_MAIN(KSycocaXdgDirsTest)

void KSycocaXdgDirsTest::runKBuildSycoca(const QProcessEnvironment &environment)
{
    QProcess proc;
    const QString kbuildsycoca = QStringLiteral(KBUILDSYCOCAEXE);
    QVERIFY(!kbuildsycoca.isEmpty());
    QStringList args;
    args << QStringLiteral("--testmode");
    //proc.setProcessChannelMode(QProcess::ForwardedChannels);
    proc.start(kbuildsycoca, args);
    proc.setProcessEnvironment(environment);

    proc.waitForFinished();
    QCOMPARE(proc.exitStatus(), QProcess::NormalExit);
}

// Ensure two apps with different XDG_DATA_DIRS/XDG_DATA_HOME don't use the same sycoca.
void KSycocaXdgDirsTest::testOtherAppDir()
{
#ifndef Q_XDG_PLATFORM
    QSKIP("This test requires XDG_DATA_DIRS");
#endif

    // KSycoca::self() represents application 1, running with one set of dirs
    KSycoca::self()->ensureCacheValid();

    // Create another xdg data dir
    const QString dataDir = m_tempDir.path();
    qputenv("XDG_DATA_DIRS", QFile::encodeName(dataDir));
    QCOMPARE(dataDir, QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation).last());
    QVERIFY(!KService::serviceByDesktopPath(QStringLiteral("test_app_other.desktop")));

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
    env.insert(QStringLiteral("XDG_DATA_DIRS"), dataDir);
    runKBuildSycoca(env);

#if 0 // for debugging
    const KService::List lst = KService::allServices();
    QVERIFY(!lst.isEmpty());
    for (const KService::Ptr &serv : lst) {
        qDebug() << serv->entryPath() << serv->storageId() /*<< serv->desktopEntryName()*/;
    }
#endif

    // This is still NOT available to application 1.
    // kbuildsycoca created a different DB file, the one we read from hasn't changed.
    // Changing XDG_DATA_DIRS at runtime isn't supported, so this test isn't doing what apps would do.
    // The point however is that another app using different dirs cannot mess up our DB.
    QVERIFY(!KService::serviceByStorageId(QStringLiteral("test_app_other.desktop")));

    // Check here what the application 2 would see, by creating another sycoca instance.
    KSycoca otherAppSycoca;
    // do what serviceByStorageId does:
    otherAppSycoca.ensureCacheValid();
    QVERIFY(otherAppSycoca.d->serviceFactory()->findServiceByStorageId(QStringLiteral("test_app_other.desktop")));
    QVERIFY(otherAppSycoca.d->m_databasePath != KSycoca::self()->d->m_databasePath); // check that they use a different filename
    // check that the timestamp code works
    QVERIFY(!otherAppSycoca.d->needsRebuild());
}

#include "ksycoca_xdgdirstest.moc"
